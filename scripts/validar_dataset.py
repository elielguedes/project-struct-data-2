import csv
import sys
from collections import defaultdict
from pathlib import Path


UFs_ESPERADAS = {
    "AC", "AL", "AP", "AM", "BA", "CE", "DF", "ES", "GO", "MA",
    "MT", "MS", "MG", "PA", "PB", "PR", "PE", "PI", "RJ", "RN",
    "RS", "RO", "RR", "SC", "SP", "SE", "TO",
}
CANDIDATOS_ESPERADOS = ["Pres_X", "Pres_Y"]
COLUNAS_ESPERADAS = ["UF", "CANDIDATO", "VOTOS", "PERCENTUAL"]


def validar(caminho):
    linhas = []
    falhas = []
    registros_por_uf = defaultdict(list)
    combinacoes_vistas = set()
    combinacoes_duplicadas = set()
    ufs_vistas = set()

    with caminho.open("r", encoding="utf-8", newline="") as arquivo:
        leitor = csv.DictReader(arquivo, delimiter=";")
        if leitor.fieldnames != COLUNAS_ESPERADAS:
            falhas.append(
                "Estrutura incorreta: esperado "
                + ";".join(COLUNAS_ESPERADAS)
            )
            return falhas, registros_por_uf, 0, 0

        for numero_linha, registro in enumerate(leitor, start=2):
            uf = registro["UF"].strip()
            candidato = registro["CANDIDATO"].strip()
            ufs_vistas.add(uf)
            combinacao = (uf, candidato)
            if combinacao in combinacoes_vistas:
                combinacoes_duplicadas.add(combinacao)
            combinacoes_vistas.add(combinacao)

            try:
                votos = int(registro["VOTOS"])
            except (TypeError, ValueError):
                falhas.append(
                    f"Linha {numero_linha}: VOTOS nao inteiro."
                )
                continue

            try:
                percentual = float(registro["PERCENTUAL"])
            except (TypeError, ValueError):
                falhas.append(
                    f"Linha {numero_linha}: PERCENTUAL nao numerico."
                )
                continue

            linhas.append(registro)
            registros_por_uf[uf].append(
                {
                    "candidato": candidato,
                    "votos": votos,
                    "percentual": percentual,
                    "linha": numero_linha,
                }
            )

    if len(ufs_vistas) != 27:
        falhas.append(
            f"Quantidade de UFs distinta: {len(ufs_vistas)}."
        )
    if ufs_vistas != UFs_ESPERADAS:
        faltantes = sorted(UFs_ESPERADAS - ufs_vistas)
        extras = sorted(ufs_vistas - UFs_ESPERADAS)
        if faltantes:
            falhas.append("UFs faltantes: " + ", ".join(faltantes) + ".")
        if extras:
            falhas.append("UFs extras: " + ", ".join(extras) + ".")
    if combinacoes_duplicadas:
        falhas.append(
            "Combinacoes UF + CANDIDATO duplicadas: "
            + ", ".join(
                uf + "/" + candidato
                for uf, candidato in sorted(combinacoes_duplicadas)
            )
            + "."
        )

    if len(linhas) != 54:
        falhas.append(f"Quantidade de registros distinta: {len(linhas)}.")

    candidatos = {
        registro["CANDIDATO"].strip()
        for registro in linhas
    }
    if candidatos != set(CANDIDATOS_ESPERADOS):
        falhas.append(
            "Candidatos encontrados: "
            + ", ".join(sorted(candidatos))
            + "."
        )

    for uf in sorted(UFs_ESPERADAS):
        registros = registros_por_uf.get(uf, [])
        candidatos_da_uf = [item["candidato"] for item in registros]

        if candidatos_da_uf != CANDIDATOS_ESPERADOS:
            falhas.append(
                f"{uf}: candidatos/ordem incorretos: "
                + ", ".join(candidatos_da_uf)
                + "."
            )

        if len(registros) != 2:
            falhas.append(
                f"{uf}: quantidade incorreta de registros "
                f"({len(registros)})."
            )
            continue

        soma_votos = sum(item["votos"] for item in registros)
        soma_percentuais = sum(item["percentual"] for item in registros)

        for item in registros:
            if item["votos"] < 0:
                falhas.append(
                    f"Linha {item['linha']}: voto negativo."
                )
            if item["percentual"] < 0 or item["percentual"] > 100:
                falhas.append(
                    f"Linha {item['linha']}: percentual fora de 0 a 100."
                )
            if item["votos"] == 0:
                falhas.append(
                    f"Linha {item['linha']}: voto nulo ou zero."
                )
            if soma_votos > 0:
                percentual_recalculado = (
                    item["votos"] * 100.0 / soma_votos
                )
                if abs(percentual_recalculado - item["percentual"]) > 0.00051:
                    falhas.append(
                        f"{uf}/{item['candidato']}: percentual nao "
                        "corresponde aos votos."
                    )

        if abs(soma_percentuais - 100.0) > 0.00051:
            falhas.append(
                f"{uf}: soma dos percentuais = "
                f"{soma_percentuais:.6f}%."
            )

    return (
        falhas,
        registros_por_uf,
        len(combinacoes_duplicadas),
        len(linhas),
    )


def gerar_relatorio(caminho, falhas, registros_por_uf,
                    combinacoes_duplicadas, registros_validos):
    linhas = []
    linhas.append("RELATORIO DE VALIDACAO DO DATASET SINTETICO")
    linhas.append("")
    linhas.append("Arquivo: " + caminho.name)
    linhas.append(
        "Natureza: dados exclusivamente sintéticos; "
        "não representam resultados eleitorais reais."
    )
    linhas.append("")

    ufs = sorted(registros_por_uf)
    candidatos = sorted({
        item["candidato"]
        for registros in registros_por_uf.values()
        for item in registros
    })
    total_registros = registros_validos

    linhas.append(
        "1. Estrutura em quatro colunas: "
        + ("OK" if not any("Estrutura incorreta" in f for f in falhas)
           else "FALHOU")
    )
    linhas.append(
        "2. Quantidade de UFs: "
        + str(len(ufs))
        + (" OK" if len(ufs) == 27 else " FALHOU")
    )
    linhas.append(
        "3. Quantidade de candidatos: "
        + str(len(candidatos))
        + (" OK" if set(candidatos) == set(CANDIDATOS_ESPERADOS)
           else " FALHOU")
    )
    linhas.append(
        "4. Quantidade de registros: "
        + str(total_registros)
        + (" OK" if total_registros == 54 else " FALHOU")
    )
    linhas.append(
        "UFs duplicadas indevidamente: "
        + ("nenhuma." if len(ufs) == len(set(ufs)) else "detectadas.")
    )
    linhas.append(
        "Combinacoes UF + CANDIDATO duplicadas: "
        + str(combinacoes_duplicadas)
        + ("." if combinacoes_duplicadas == 0
           else " FALHOU.")
    )
    linhas.append(
        "Total de registros validos: "
        + str(registros_validos)
        + ("." if registros_validos == 54 else " FALHOU.")
    )
    linhas.append(
        "Total de UFs: "
        + str(len(ufs))
        + ("." if len(ufs) == 27 else " FALHOU.")
    )
    linhas.append(
        "Total de candidatos: "
        + str(len(candidatos))
        + ("." if len(candidatos) == 2 else " FALHOU.")
    )

    representacao_ok = all(
        len(registros_por_uf[uf]) == 2
        and [item["candidato"] for item in registros_por_uf[uf]]
        == CANDIDATOS_ESPERADOS
        for uf in UFs_ESPERADAS
    )
    linhas.append(
        "5. Cada UF possui Pres_X e Pres_Y na ordem correta: "
        + ("OK" if representacao_ok else "FALHOU")
    )
    votos_validos = all(
        item["votos"] > 0
        for registros in registros_por_uf.values()
        for item in registros
    )
    linhas.append(
        "6. Votos positivos: " + ("OK" if votos_validos else "FALHOU")
    )

    percentuais_validos = all(
        0.0 <= item["percentual"] <= 100.0
        for registros in registros_por_uf.values()
        for item in registros
    )
    linhas.append(
        "7. Percentuais entre 0 e 100: "
        + ("OK" if percentuais_validos else "FALHOU")
    )

    somas_validas = all(
        abs(
            sum(item["percentual"] for item in registros_por_uf[uf])
            - 100.0
        ) <= 0.00051
        for uf in UFs_ESPERADAS
        if uf in registros_por_uf
    )
    linhas.append(
        "8. Soma dos percentuais aproximadamente 100% por UF: "
        + ("OK" if somas_validas else "FALHOU")
    )

    linhas.append(
        "9. Percentual recalculado a partir de VOTOS: "
        + ("OK" if not any("percentual nao" in f for f in falhas)
           else "FALHOU")
    )

    linhas.append("")
    if falhas:
        linhas.append("FALHAS DETECTADAS:")
        linhas.extend("- " + falha for falha in falhas)
    else:
        linhas.append("RESULTADO FINAL: TODAS AS VALIDACOES PASSARAM.")

    return "\n".join(linhas) + "\n"


def main():
    caminho = Path(
        sys.argv[1]
        if len(sys.argv) > 1
        else "votacao_presidente_estados_2026.csv"
    )
    relatorio = Path(
        sys.argv[2]
        if len(sys.argv) > 2
        else "relatorio_validacao.txt"
    )

    if not caminho.is_file():
        print("Arquivo nao encontrado: " + str(caminho))
        return 1

    falhas, registros_por_uf, combinacoes_duplicadas, registros_validos = \
        validar(caminho)
    texto = gerar_relatorio(
        caminho,
        falhas,
        registros_por_uf,
        combinacoes_duplicadas,
        registros_validos,
    )
    relatorio.write_text(texto, encoding="utf-8")
    print(texto, end="")
    return 1 if falhas else 0


if __name__ == "__main__":
    raise SystemExit(main())
