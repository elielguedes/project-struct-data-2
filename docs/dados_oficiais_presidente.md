# Dados oficiais presidenciais — Eleições 2026

## Origem

Os dados consolidados foram produzidos a partir do arquivo oficial:

- `dados/originais/votacao_secao_2026_BR.csv`
- Fonte: Repositório de Dados Abertos do TSE
- Arquivo: votação por seção eleitoral das eleições de 2026
- Turno processado: primeiro turno (`NR_TURNO = 1`)

O arquivo sintético anterior foi preservado em:

- `dados/processados/votacao_presidente_estados_2026.sintetico.csv`

O arquivo atual não contém candidatos ou percentuais sintéticos:

- `dados/processados/votacao_presidente_estados_2026.csv`

## Filtros aplicados

O pré-processamento considera somente registros em que:

- `ANO_ELEICAO = 2026`
- `NR_TURNO = 1`
- `DS_CARGO = Presidente`
- `SG_UF` pertence às 27 UFs brasileiras
- `NR_VOTAVEL` não representa branco, nulo ou código especial
- `QT_VOTOS` é maior que zero

O código especial `ZZ` representa exterior e foi excluído porque o objetivo
deve contemplar as 27 UFs brasileiras.

## Metodologia

Os votos oficiais são somados por:

`SG_UF + NM_VOTAVEL`

Para cada UF, o percentual de cada candidato é:

`percentual = votos_do_candidato / total_de_votos_nominais_validos * 100`

O denominador exclui as opções de branco, nulo e registros especiais. Portanto,
os percentuais somam 100 por UF.

O script mantém todos os candidatos presentes no arquivo oficial e cria uma
representação consistente: cada UF recebe o mesmo conjunto de candidatos,
usando zero quando não houver votos naquela UF.

## Resultado da validação

- UFs: 27
- Candidatos oficiais encontrados: 13
- Registros consolidados: 351
- 13 registros por UF
- Soma mínima dos percentuais por UF: aproximadamente 99,999998%
- Soma máxima dos percentuais por UF: aproximadamente 100,000004%

## Script

Execute a partir da raiz do projeto:

```powershell
python scripts\processar_votacao_presidente_tse.py dados\originais\votacao_secao_2026_BR.csv --turno 1
```

Para outro turno disponível no mesmo tipo de arquivo:

```powershell
python scripts\processar_votacao_presidente_tse.py dados\originais\votacao_secao_2026_BR.csv --turno 2
```

Ao substituir um CSV consolidado existente, o script preserva os dados
anteriores como `*.sintetico.csv`.
