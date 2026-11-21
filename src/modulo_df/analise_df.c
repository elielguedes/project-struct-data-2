#include "analise_df.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#define MAX_LINHA 4096
#define MAX_CAMPOS 128
#define EPSILON 1e-6f

static int quantidade_candidatos = 0;

static void remover_quebra(char *texto)
{
    size_t tamanho = strlen(texto);
    while (tamanho > 0 &&
           (texto[tamanho - 1] == '\n' || texto[tamanho - 1] == '\r'))
        texto[--tamanho] = '\0';
}

static char *aparar(char *texto)
{
    char *fim;

    while (*texto == ' ' || *texto == '\t')
        ++texto;
    fim = texto + strlen(texto);
    while (fim > texto &&
           (fim[-1] == ' ' || fim[-1] == '\t'))
        *--fim = '\0';
    return texto;
}

static int dividir_csv(char *linha, char *campos[], int maximo)
{
    int quantidade = 0;
    char *cursor = linha;
    char *separador;

    while (quantidade < maximo)
    {
        campos[quantidade++] = aparar(cursor);
        separador = strchr(cursor, ';');
        if (separador == NULL)
            separador = strchr(cursor, ',');
        if (separador == NULL)
            break;
        *separador = '\0';
        cursor = separador + 1;
    }
    return quantidade;
}

Grafo *construir_grafo_bipartido_df(const char *arquivo_csv)
{
    FILE *arquivo;
    char linha[MAX_LINHA];
    char *campos[MAX_CAMPOS];
    Grafo *grafo;
    int numero_cabecalho;
    int i;
    int ra_coluna;
    int candidato_coluna;
    int percentual_coluna;
    int votos_coluna;
    int numero_linha;
    char *fim;
    float percentual;

    if (arquivo_csv == NULL)
        return NULL;

    arquivo = fopen(arquivo_csv, "r");
    if (arquivo == NULL)
    {
        perror("Erro ao abrir o CSV do DF");
        return NULL;
    }

    if (fgets(linha, sizeof(linha), arquivo) == NULL)
    {
        fprintf(stderr, "CSV vazio: %s\n", arquivo_csv);
        fclose(arquivo);
        return NULL;
    }
    remover_quebra(linha);
    numero_cabecalho = dividir_csv(linha, campos, MAX_CAMPOS);

    ra_coluna = -1;
    candidato_coluna = -1;
    percentual_coluna = -1;
    votos_coluna = -1;

    for (i = 0; i < numero_cabecalho; ++i)
    {
        char *nome = aparar(campos[i]);
        if (strcmp(nome, "RA") == 0)
            ra_coluna = i;
        else if (strcmp(nome, "CANDIDATO") == 0)
            candidato_coluna = i;
        else if (strcmp(nome, "PERCENTUAL") == 0)
            percentual_coluna = i;
        else if (strcmp(nome, "VOTOS") == 0)
            votos_coluna = i;
    }

    grafo = grafo_criar(0);
    if (grafo == NULL)
    {
        fclose(arquivo);
        return NULL;
    }
    quantidade_candidatos = 0;

    if (ra_coluna >= 0 && candidato_coluna >= 0 &&
        (percentual_coluna >= 0 || votos_coluna >= 0))
    {
        rewind(arquivo);
        if (fgets(linha, sizeof(linha), arquivo) != NULL)
        {
            while (fgets(linha, sizeof(linha), arquivo) != NULL)
            {
                int quantidade;
                int candidato;
                char *nome_candidato;

                remover_quebra(linha);
                if (linha[0] == '\0')
                    continue;
                quantidade =
                    dividir_csv(linha, campos, MAX_CAMPOS);
                if (ra_coluna >= quantidade ||
                    candidato_coluna >= quantidade)
                    continue;
                nome_candidato =
                    aparar(campos[candidato_coluna]);
                if (nome_candidato[0] == '\0')
                    continue;
                candidato =
                    grafo_adicionar_vertice(grafo, nome_candidato);
                if (candidato < 0)
                {
                    grafo_destruir(grafo);
                    fclose(arquivo);
                    return NULL;
                }
                if (candidato >= quantidade_candidatos)
                    quantidade_candidatos = candidato + 1;
            }
        }

        rewind(arquivo);
        if (fgets(linha, sizeof(linha), arquivo) == NULL)
        {
            grafo_destruir(grafo);
            fclose(arquivo);
            return NULL;
        }

        numero_linha = 1;
        while (fgets(linha, sizeof(linha), arquivo) != NULL)
        {
            int quantidade;
            int ra;
            int candidato;
            char *nome_ra;
            char *nome_candidato;
            char *valor;

            ++numero_linha;
            remover_quebra(linha);
            if (linha[0] == '\0')
                continue;

            quantidade = dividir_csv(linha, campos, MAX_CAMPOS);
            if (ra_coluna >= quantidade ||
                candidato_coluna >= quantidade ||
                ((percentual_coluna >= 0 &&
                  percentual_coluna >= quantidade) &&
                 (votos_coluna >= 0 &&
                  votos_coluna >= quantidade)))
            {
                fprintf(stderr, "Linha %d ignorada: estrutura invalida\n",
                        numero_linha);
                continue;
            }

            nome_ra = aparar(campos[ra_coluna]);
            nome_candidato = aparar(campos[candidato_coluna]);
            if (nome_ra[0] == '\0' || nome_candidato[0] == '\0')
                continue;

            if (percentual_coluna >= 0 &&
                percentual_coluna < quantidade)
            {
                valor = campos[percentual_coluna];
                percentual = (float)strtod(aparar(valor), &fim);
            }
            else
            {
                valor = campos[votos_coluna];
                percentual = (float)strtod(aparar(valor), &fim);
            }
            if (fim == aparar(valor) || percentual < 0.0f)
            {
                fprintf(stderr, "Linha %d ignorada: percentual invalido\n",
                        numero_linha);
                continue;
            }

            candidato = grafo_adicionar_vertice(grafo, nome_candidato);
            ra = grafo_adicionar_vertice(grafo, nome_ra);
            if (candidato < 0 || ra < 0)
            {
                grafo_destruir(grafo);
                fclose(arquivo);
                return NULL;
            }
            if (candidato >= quantidade_candidatos)
                quantidade_candidatos = candidato + 1;

            if (grafo_adicionar_aresta(grafo, ra, candidato, percentual) == -1)
            {
                grafo_destruir(grafo);
                fclose(arquivo);
                return NULL;
            }
        }
    }
    else
    {
        if (numero_cabecalho < 2 ||
            strcmp(aparar(campos[0]), "RA") != 0)
        {
            fprintf(stderr,
                    "Cabecalho invalido. Use RA;CANDIDATO;VOTOS;PERCENTUAL "
                    "ou RA,candidato1,candidato2,...\n");
            grafo_destruir(grafo);
            fclose(arquivo);
            return NULL;
        }

        quantidade_candidatos = numero_cabecalho - 1;
        for (i = 1; i < numero_cabecalho; ++i)
        {
            if (grafo_adicionar_vertice(grafo, aparar(campos[i])) < 0)
            {
                grafo_destruir(grafo);
                fclose(arquivo);
                return NULL;
            }
        }

        numero_linha = 1;
        while (fgets(linha, sizeof(linha), arquivo) != NULL)
        {
            int quantidade;
            int ra;
            ++numero_linha;
            remover_quebra(linha);
            if (linha[0] == '\0')
                continue;
            quantidade = dividir_csv(linha, campos, MAX_CAMPOS);
            if (quantidade != numero_cabecalho)
            {
                fprintf(stderr, "Linha %d ignorada: colunas inesperadas\n",
                        numero_linha);
                continue;
            }
            ra = grafo_adicionar_vertice(grafo, aparar(campos[0]));
            if (ra < 0)
            {
                grafo_destruir(grafo);
                fclose(arquivo);
                return NULL;
            }
            for (i = 1; i < numero_cabecalho; ++i)
            {
                percentual = (float)strtod(aparar(campos[i]), &fim);
                if (fim == aparar(campos[i]) || percentual < 0.0f)
                    continue;
                if (grafo_adicionar_aresta(grafo, ra, i - 1,
                                           percentual) == -1)
                {
                    grafo_destruir(grafo);
                    fclose(arquivo);
                    return NULL;
                }
            }
        }
    }

    fclose(arquivo);
    return grafo;
}

void candidatos_dominantes_por_ra(Grafo *grafo)
{
    size_t i;
    No *aresta;
    float maior;

    if (grafo == NULL)
        return;

    printf("=== Candidato dominante por Regiao Administrativa ===\n");
    for (i = (size_t)quantidade_candidatos;
         i < grafo->quantidade_vertices; ++i)
    {
        maior = -1.0f;
        aresta = grafo->vertices[i].adjacencias.primeiro;
        while (aresta != NULL)
        {
            if (aresta->destino < quantidade_candidatos &&
                aresta->peso > maior)
                maior = aresta->peso;
            aresta = aresta->proximo;
        }

        if (maior < 0.0f)
        {
            printf("%-20s -> sem votos registrados\n",
                   grafo->vertices[i].rotulo);
            continue;
        }

        printf("%-20s -> ", grafo->vertices[i].rotulo);
        {
            int primeiro = 1;
            int empate = 0;
            aresta = grafo->vertices[i].adjacencias.primeiro;
            while (aresta != NULL)
            {
                if (aresta->destino < quantidade_candidatos &&
                    fabsf(aresta->peso - maior) < EPSILON)
                {
                    printf("%s%s", primeiro ? "" : " / ",
                           grafo->vertices[aresta->destino].rotulo);
                    if (!primeiro)
                        empate = 1;
                    primeiro = 0;
                }
                aresta = aresta->proximo;
            }
            printf(" (%.1f%%)%s\n", maior,
                   empate ? " [EMPATE]" : "");
        }
    }
}

Grafo *gerar_grafo_similaridade_ra(Grafo *grafo_bipartido,
                                   float limiar_similaridade)
{
    Grafo *similaridade;
    size_t quantidade_ra;
    size_t i;
    size_t j;
    float *vetores;
    double *normas;

    if (grafo_bipartido == NULL ||
        quantidade_candidatos <= 0 ||
        limiar_similaridade < 0.0f ||
        limiar_similaridade > 1.0f ||
        grafo_bipartido->quantidade_vertices <
            (size_t)quantidade_candidatos)
        return NULL;

    quantidade_ra =
        grafo_bipartido->quantidade_vertices -
        (size_t)quantidade_candidatos;
    similaridade = grafo_criar(0);
    vetores = (float *)calloc(
        quantidade_ra * (size_t)quantidade_candidatos, sizeof(float));
    normas = (double *)calloc(quantidade_ra, sizeof(double));
    if (similaridade == NULL ||
        (quantidade_ra > 0 &&
         (vetores == NULL || normas == NULL)))
    {
        free(vetores);
        free(normas);
        grafo_destruir(similaridade);
        return NULL;
    }

    for (i = 0; i < quantidade_ra; ++i)
    {
        size_t vertice_ra =
            (size_t)quantidade_candidatos + i;
        No *aresta;

        if (grafo_adicionar_vertice(
                similaridade,
                grafo_bipartido->vertices[vertice_ra].rotulo) < 0)
        {
            free(vetores);
            free(normas);
            grafo_destruir(similaridade);
            return NULL;
        }

        aresta =
            grafo_bipartido->vertices[vertice_ra].adjacencias.primeiro;
        while (aresta != NULL)
        {
            if (aresta->destino < quantidade_candidatos)
            {
                vetores[i * (size_t)quantidade_candidatos +
                        (size_t)aresta->destino] = aresta->peso;
                normas[i] += (double)aresta->peso *
                             (double)aresta->peso;
            }
            aresta = aresta->proximo;
        }
    }

    for (i = 0; i < quantidade_ra; ++i)
    {
        for (j = i + 1; j < quantidade_ra; ++j)
        {
            double produto = 0.0;
            double norma_i;
            double norma_j;
            float cosseno;
            int candidato;

            norma_i = sqrt(normas[i]);
            norma_j = sqrt(normas[j]);
            if (norma_i == 0.0 || norma_j == 0.0)
                continue;

            for (candidato = 0;
                 candidato < quantidade_candidatos;
                 ++candidato)
            {
                produto +=
                    (double)vetores[i * (size_t)quantidade_candidatos +
                                    (size_t)candidato] *
                    (double)vetores[j * (size_t)quantidade_candidatos +
                                    (size_t)candidato];
            }
            cosseno = (float)(produto / (norma_i * norma_j));
            if (cosseno >= limiar_similaridade &&
                grafo_adicionar_aresta(similaridade, (int)i, (int)j,
                                       cosseno) == -1)
            {
                free(vetores);
                free(normas);
                grafo_destruir(similaridade);
                return NULL;
            }
        }
    }

    free(vetores);
    free(normas);
    return similaridade;
}