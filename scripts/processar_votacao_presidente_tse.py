import argparse
import csv
import sys
from pathlib import Path

import pandas as pd


UFs_ESPERADAS = [
    "AC", "AL", "AP", "AM", "BA", "CE", "DF", "ES", "GO",
    "MA", "MT", "MS", "MG", "PA", "PB", "PR", "PE", "PI",
    "RJ", "RN", "RS", "RO", "RR", "SC", "SP", "SE", "TO",
]

CARGOS_NOMINAIS = {"95", "96", "97", "98"}


def coluna_segura(linha, nome):
    if nome not in linha:
        return None
    return linha[nome]


def processar(entrada, saida, turno):
    entrada = Path(entrada)
    saida = Path(saida)
    if not entrada.is_file():
        raise FileNotFoundError(f"Arquivo de origem nao encontrado: {entrada}")

    totais = {}
    candidatos_por_uf = set()

    usecols = [
        "ANO_ELEICAO",
        "NR_TURNO",
        "DS_CARGO",
        "SG_UF",
        "NR_VOTAVEL",
        "NM_VOTAVEL",
        "QT_VOTOS",
    ]

    try:
        leitor = pd.read_csv(
            entrada,
            sep=";",
            quotechar='"',
            encoding="utf-8-sig",
            chunksize=500_000,
            usecols=usecols,
            dtype={
                "ANO_ELEICAO": "Int64",
                "NR_TURNO": "Int64",
                "SG_UF": "string",
                "NR_VOTAVEL": "string",
                "NM_VOTAVEL": "string",
                "QT_VOTOS": "Int64",
            },
        )
    except UnicodeDecodeError:
        leitor = pd.read_csv(
            entrada,
            sep=";",
            quotechar='"',
            encoding="latin1",
            chunksize=500_000,
            usecols=usecols,
            dtype={
                "ANO_ELEICAO": "Int64",
                "NR_TURNO": "Int64",
                "SG_UF": "string",
                "NR_VOTAVEL": "string",
                "NM_VOTAVEL": "string",
                "QT_VOTOS": "Int64",
            },
        )

    linhas_processadas = 0
    for bloco in leitor:
        linhas_processadas += len(bloco)
        filtro = (
            bloco["ANO_ELEICAO"].eq(2026)
            & bloco["NR_TURNO"].eq(turno)
            & bloco["DS_CARGO"].fillna("").str.strip().str.upper()
            .eq("PRESIDENTE")
        )
        bloco = bloco.loc[filtro].copy()
        if bloco.empty:
            continue

        bloco["SG_UF"] = bloco["SG_UF"].str.strip().str.upper()
        bloco["NR_VOTAVEL"] = bloco["NR_VOTAVEL"].fillna("").str.strip()
        bloco["NM_VOTAVEL"] = bloco["NM_VOTAVEL"].fillna("").str.strip()

        bloco = bloco.loc[
            ~bloco["NR_VOTAVEL"].isin(CARGOS_NOMINAIS)
            & ~bloco["NR_VOTAVEL"].isin({"", "0"})
            & ~bloco["NM_VOTAVEL"].str.upper()
            .isin({"VOTO BRANCO", "VOTO NULO", "#NULO", "#NE"})
            & bloco["NM_VOTAVEL"].str.len().gt(0)
        ].copy()
        if bloco.empty:
            continue

        bloco["QT_VOTOS"] = pd.to_numeric(
            bloco["QT_VOTOS"], errors="coerce"
        ).fillna(0)
        bloco = bloco.loc[bloco["QT_VOTOS"].gt(0)]
        if bloco.empty:
            continue

        candidatos_chunk = (
            bloco.groupby(["SG_UF", "NM_VOTAVEL"], as_index=False)["QT_VOTOS"]
            .sum()
        )
        for uf, candidato, votos in candidatos_chunk.itertuples(
            index=False, name=None
        ):
            candidatos_por_uf.add((str(uf), str(candidato)))
            chave = (str(uf), str(candidato))
            totais[chave] = totais.get(chave, 0) + int(votos)

    if linhas_processadas == 0:
        raise ValueError("Nenhuma linha foi lida do arquivo de origem.")

    candidatos = sorted(
        {candidato for _uf, candidato in candidatos_por_uf},
        key=lambda nome: (
            -sum(votos for (uf, candidato), votos in totais.items()
                 if candidato == nome),
            nome,
        ),
    )
    ufs_presentes = sorted(
        {
            uf
            for uf, _candidato in candidatos_por_uf
            if uf in UFs_ESPERADAS
        }
    )
    faltantes = [uf for uf in UFs_ESPERADAS if uf not in ufs_presentes]
    extras = [
        uf
        for uf in ufs_presentes
        if uf not in UFs_ESPERADAS and uf != "ZZ"
    ]

    if extras:
        raise ValueError("UF desconhecida encontrada: " + ", ".join(extras))
    if len(ufs_presentes) != 27:
        raise ValueError(
            "O arquivo nao contem os 27 Estados/DF no turno "
            f"{turno}. Faltantes: {', '.join(faltantes) or 'nenhum'}"
        )

    saida.parent.mkdir(parents=True, exist_ok=True)
    if saida.exists():
        reservado = saida.with_name(
            saida.stem + ".sintetico" + saida.suffix
        )
        if not reservado.exists():
            reservado.write_bytes(saida.read_bytes())
        print(
            f"Arquivo previo preservado como sintético: {reservado}",
            file=sys.stderr,
        )

    with saida.open("w", encoding="utf-8", newline="") as arquivo:
        escritor = csv.writer(
            arquivo, delimiter=";", lineterminator="\n"
        )
        escritor.writerow(["UF", "CANDIDATO", "VOTOS", "PERCENTUAL"])

        for uf in UFs_ESPERADAS:
            total_uf = sum(
                totais.get((uf, candidato), 0)
                for candidato in candidatos
            )
            if total_uf <= 0:
                raise ValueError(f"UF sem votos nominais validos: {uf}")

            for candidato in candidatos:
                votos = totais.get((uf, candidato), 0)
                percentual = votos * 100.0 / total_uf
                escritor.writerow([
                    uf,
                    candidato,
                    votos,
                    f"{percentual:.6f}",
                ])

    resultados = []
    with saida.open("r", encoding="utf-8", newline="") as arquivo:
        resultados = list(csv.DictReader(arquivo, delimiter=";"))

    por_uf = {}
    for linha in resultados:
        por_uf.setdefault(linha["UF"], []).append(linha)

    for uf, linhas in por_uf.items():
        soma = sum(float(linha["PERCENTUAL"]) for linha in linhas)
        if abs(soma - 100.0) > 0.00051:
            raise ValueError(
                f"Percentual inconsistente em {uf}: {soma:.6f}%"
            )
        for linha in linhas:
            if int(linha["VOTOS"]) < 0:
                raise ValueError(
                    f"Voto negativo em {uf}/{linha['CANDIDATO']}"
                )

    representacao = {
        candidato
        for uf in UFs_ESPERADAS
        for candidato in (
            linha["CANDIDATO"]
            for linha in por_uf[uf]
        )
    }

    print("Processamento oficial concluído")
    print(f"Arquivo de origem: {entrada}")
    print(f"Arquivo consolidado: {saida}")
    print(f"Turno utilizado: {turno}")
    print(f"Linhas lidas: {linhas_processadas}")
    print(f"UFs validadas: {len(por_uf)}")
    print(f"Candidatos oficiais encontrados: {len(candidatos)}")
    print(f"Representacao: {len(representacao)} candidatos")
    print(f"Registros gerados: {len(resultados)}")


def main():
    parser = argparse.ArgumentParser(
        description=(
            "Processa dados oficiais de votacao por secao do TSE "
            "para o grafo presidencial."
        )
    )
    parser.add_argument(
        "entrada",
        nargs="?",
        default=r"dados\originais\votacao_secao_2026_BR.csv",
    )
    parser.add_argument(
        "-s", "--saida",
        default=r"dados\processados\votacao_presidente_estados_2026.csv",
    )
    parser.add_argument(
        "-t", "--turno",
        type=int,
        choices=[1, 2],
        default=1,
    )
    argumentos = parser.parse_args()

    try:
        processar(argumentos.entrada, argumentos.saida, argumentos.turno)
    except Exception as erro:
        print(f"ERRO: {erro}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
