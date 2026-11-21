import argparse
import csv
import sys
from pathlib import Path

import pandas as pd


UFs = "DF"
UF_COLUNA = "SG_UF"
TURNO_COLUNA = "NR_TURNO"
CARGO_COLUNA = "DS_CARGO"
ZONA_COLUNA = "NR_ZONA"
SECAO_COLUNA = "NR_SECAO"
LOCAL_COLUNA = "NM_LOCAL_VOTACAO"
ENDERECO_COLUNA = "DS_LOCAL_VOTACAO_ENDERECO"


def ler_mapeamento(caminho):
    caminho = Path(caminho)
    if not caminho.is_file():
        return None, f"arquivo nao encontrado: {caminho}"

    try:
        tabela = pd.read_csv(
            caminho,
            sep=";",
            encoding="utf-8-sig",
            dtype=str,
        )
    except UnicodeDecodeError:
        tabela = pd.read_csv(
            caminho,
            sep=";",
            encoding="latin1",
            dtype=str,
        )

    obrigatorias = {ZONA_COLUNA, SECAO_COLUNA, "RA"}
    faltantes = sorted(obrigatorias - set(tabela.columns))
    if faltantes:
        return None, (
            "cabecalho invalido; faltantes: "
            + ", ".join(faltantes)
        )

    tabela = tabela.copy()
    tabela[ZONA_COLUNA] = tabela[ZONA_COLUNA].str.strip()
    tabela[SECAO_COLUNA] = tabela[SECAO_COLUNA].fillna("").str.strip()
    tabela["RA"] = tabela["RA"].fillna("").str.strip()

    invalidas = tabela[
        tabela[ZONA_COLUNA].eq("")
        | tabela["RA"].eq("")
    ]
    if not invalidas.empty:
        return None, "existem linhas sem NR_ZONA ou RA"

    repetidas = tabela.duplicated(
        [ZONA_COLUNA, SECAO_COLUNA],
        keep=False,
    )
    if repetidas.any():
        return None, "existem pares duplicados no arquivo de mapeamento"

    conflito_geral = (
        tabela[tabela[SECAO_COLUNA].eq("")]
        .duplicated(ZONA_COLUNA, keep=False)
    )
    if conflito_geral.any():
        return None, "zona possui mais de uma RA geral"

    tabela["TIPO"] = tabela[SECAO_COLUNA].where(
        tabela[SECAO_COLUNA].ne(""),
        "GERAL_DA_ZONA",
    )
    return tabela, None


def ler_votacao_df(caminho, turno):
    caminho = Path(caminho)
    if not caminho.is_file():
        raise FileNotFoundError(
            f"arquivo de votacao nao encontrado: {caminho}"
        )

    usecols = [
        UF_COLUNA,
        TURNO_COLUNA,
        CARGO_COLUNA,
        ZONA_COLUNA,
        SECAO_COLUNA,
        LOCAL_COLUNA,
        ENDERECO_COLUNA,
    ]

    lista = []
    try:
        leitor = pd.read_csv(
            caminho,
            sep=";",
            quotechar='"',
            encoding="utf-8-sig",
            chunksize=500_000,
            usecols=usecols,
            dtype=str,
        )
    except UnicodeDecodeError:
        leitor = pd.read_csv(
            caminho,
            sep=";",
            quotechar='"',
            encoding="latin1",
            chunksize=500_000,
            usecols=usecols,
            dtype=str,
        )

    linhas = 0
    for bloco in leitor:
        linhas += len(bloco)
        bloco = bloco[
            bloco[UF_COLUNA].fillna("").str.strip().str.upper().eq("DF")
            & bloco[TURNO_COLUNA].fillna("").str.strip().eq(str(turno))
            & bloco[CARGO_COLUNA].fillna("").str.strip()
            .str.upper().eq("GOVERNADOR")
        ]
        if not bloco.empty:
            lista.append(bloco)

    if not lista:
        return pd.DataFrame(
            columns=usecols
        ), linhas

    dados = pd.concat(lista, ignore_index=True)
    dados[ZONA_COLUNA] = dados[ZONA_COLUNA].fillna("").str.strip()
    dados[SECAO_COLUNA] = dados[SECAO_COLUNA].fillna("").str.strip()
    dados[LOCAL_COLUNA] = dados[LOCAL_COLUNA].fillna("").str.strip()
    dados[ENDERECO_COLUNA] = dados[
        ENDERECO_COLUNA
    ].fillna("").str.strip()
    dados = dados[
        dados[ZONA_COLUNA].ne("")
        & dados[SECAO_COLUNA].ne("")
    ]
    return dados, linhas


def validar(tabela, dados):
    pares_votacao = set(
        dados[[ZONA_COLUNA, SECAO_COLUNA]]
        .drop_duplicates()
        .itertuples(index=False, name=None)
    )

    if tabela is None:
        return {
            "validavel": False,
            "pares_votacao": pares_votacao,
            "pares_mapeados": set(),
            "pares_sem_mapeamento": pares_votacao,
            "zonas_votacao": set(dados[ZONA_COLUNA].dropna()),
            "locais": set(dados[LOCAL_COLUNA].dropna()),
        }

    pares_mapeados = set(
        tabela[tabela[SECAO_COLUNA].ne("")]
        [[ZONA_COLUNA, SECAO_COLUNA]]
        .drop_duplicates()
        .itertuples(index=False, name=None)
    )
    zonas_mapeadas = set(
        tabela[ZONA_COLUNA].dropna()
    )

    pares_sem_mapeamento = set()
    for zona, secao in pares_votacao:
        especifico = (zona, secao) in pares_mapeados
        geral = bool(
            tabela[
                tabela[ZONA_COLUNA].eq(zona)
                & tabela[SECAO_COLUNA].eq("")
            ].shape[0]
        )
        if not especifico and not geral:
            pares_sem_mapeamento.add((zona, secao))

    zonas_votacao = set(dados[ZONA_COLUNA].dropna())
    pares_extra = set()
    for zona, secao in pares_mapeados:
        if (zona, secao) not in pares_votacao:
            pares_extra.add((zona, secao))

    return {
        "validavel": True,
        "pares_votacao": pares_votacao,
        "pares_mapeados": pares_mapeados,
        "pares_sem_mapeamento": pares_sem_mapeamento,
        "pares_extra": pares_extra,
        "zonas_votacao": zonas_votacao,
        "zonas_mapeadas": zonas_mapeadas,
        "locais": set(dados[LOCAL_COLUNA].dropna()),
    }


def formatar_pares(pares):
    if not pares:
        return "nenhum"
    pares_ordenados = sorted(
        pares,
        key=lambda item: (
            int(item[0]),
            int(item[1]) if item[1] else 0,
        ),
    )
    limite = 20
    texto = ", ".join(
        f"{zona}/{secao}"
        for zona, secao in pares_ordenados[:limite]
    )
    if len(pares_ordenados) > limite:
        texto += (
            f" ... ({len(pares_ordenados) - limite} pares adicionais)"
        )
    return texto


def main():
    parser = argparse.ArgumentParser(
        description=(
            "Valida mapeamento de zona/secao eleitoral para RA no DF."
        )
    )
    parser.add_argument(
        "votacao",
        nargs="?",
        default=r"dados\originais\votacao_secao_2026_DF.csv",
    )
    parser.add_argument(
        "--mapeamento",
        default=r"dados\territoriais\mapeamento_zona_ra_df.csv",
    )
    parser.add_argument(
        "--turno",
        type=int,
        choices=[1, 2],
        default=1,
    )
    parser.add_argument(
        "--saida",
        default=r"dados\territoriais\relatorio_validacao_mapeamento_df.txt",
    )
    args = parser.parse_args()

    tabela, erro_mapeamento = ler_mapeamento(args.mapeamento)
    dados, linhas_lidas = ler_votacao_df(args.votacao, args.turno)
    resultado = validar(tabela, dados)

    relatorio = []
    relatorio.append(
        "RELATORIO DE VALIDACAO: ZONA/SECAO ELEITORAL -> RA (DF)"
    )
    relatorio.append("")
    relatorio.append(f"Arquivo de votacao: {args.votacao}")
    relatorio.append(f"Arquivo de mapeamento: {args.mapeamento}")
    relatorio.append(f"Turno: {args.turno}")
    relatorio.append(f"Linhas lidas no arquivo: {linhas_lidas}")
    relatorio.append(
        f"Linhas de Governador/DF/turno {args.turno}: {len(dados)}"
    )
    relatorio.append(
        f"Zonas distintas no arquivo: {len(resultado['zonas_votacao'])}"
    )
    relatorio.append(
        f"Pares zona/secao distintos: {len(resultado['pares_votacao'])}"
    )
    relatorio.append(
        f"Locais de votacao distintos: {len(resultado['locais'])}"
    )
    relatorio.append("")

    if erro_mapeamento:
        relatorio.append(
            "STATUS: NAO VALIDAVEL - mapeamento oficial nao fornecido ou invalido"
        )
        relatorio.append(f"Detalhe: {erro_mapeamento}")
        relatorio.append(
            "Pares sem mapeamento: "
            + formatar_pares(resultado["pares_sem_mapeamento"])
        )
        relatorio.append("")
        relatorio.append(
            "Observacao: o CSV de votacao identifica zona, secao e local, "
            "mas nao identifica a RA diretamente."
        )
        relatorio.append(
            "Nao foi criada equivalencia inferida por endereco ou nome "
            "de local, pois isso poderia atribuir votos incorretamente."
        )
    else:
        cobertura = 0.0
        if resultado["pares_votacao"]:
            cobertura = (
                100.0
                * (len(resultado["pares_votacao"])
                   - len(resultado["pares_sem_mapeamento"]))
                / len(resultado["pares_votacao"])
            )
        relatorio.append(
            f"Pares mapeados especificamente: "
            f"{len(resultado['pares_mapeados'])}"
        )
        relatorio.append(
            f"Pares sem mapeamento: "
            f"{len(resultado['pares_sem_mapeamento'])}"
        )
        relatorio.append(f"Cobertura: {cobertura:.2f}%")
        relatorio.append(
            "Pares sem mapeamento: "
            + formatar_pares(resultado["pares_sem_mapeamento"])
        )
        relatorio.append(
            "Pares do mapeamento sem uso no arquivo: "
            + formatar_pares(resultado["pares_extra"])
        )

    saida = Path(args.saida)
    saida.parent.mkdir(parents=True, exist_ok=True)
    saida.write_text("\n".join(relatorio) + "\n", encoding="utf-8")

    print("\n".join(relatorio))
    print(f"\nRelatorio gerado: {saida}")
    return 0 if tabela is not None and len(resultado.get(
        "pares_sem_mapeamento", set()
    )) == 0 else 1


if __name__ == "__main__":
    raise SystemExit(main())
