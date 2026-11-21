#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include "analise_nacional.h"

typedef struct
{
    int origem;
    int destino;
    float peso;
} Aresta;

void carregar_estados(Estado estados[], int quantidade)
{

    FILE *arquivo = fopen("dados/processados/votacao_presidente_estados_2026.csv", "r");
    if (arquivo == NULL)
    {
        printf("Erro ao abrir o arquivo CSV!\n");
        return;
    }

    char linha[512];
    char uf[3];
    char candidato[128];
    int votos;
    float percentual;

    int total_carregados = 0;

    fgets(linha, sizeof(linha), arquivo);

    while (fgets(linha, sizeof(linha), arquivo) != NULL)
    {

        if (sscanf(linha, "%2[^;];%127[^;];%d;%f",
                   uf, candidato, &votos, &percentual) != 4)
        {
            continue;
        }

        int indice_estado = -1;

        int i;
        for (i = 0; i < total_carregados; i++)
        {
            if (strcmp(estados[i].uf, uf) == 0)
            {
                indice_estado = i;
                break;
            }
        }

        if (indice_estado == -1)
        {

            if (total_carregados >= quantidade)
            {
                printf("Limite de estados excedido!\n");
                break;
            }

            indice_estado = total_carregados;

            strcpy(estados[indice_estado].uf, uf);

            estados[indice_estado].quantidade_candidatos = 0;
            for (i = 0; i < TOTAL_CANDIDATOS; i++)
            {
                estados[indice_estado].candidatos[i][0] = '\0';
                estados[indice_estado].votos[i] = 0.0f;
            }

            total_carregados++;
        }

        {
            int indice_candidato = -1;
            int i;
            for (i = 0;
                 i < estados[indice_estado].quantidade_candidatos;
                 i++)
            {
                if (strcmp(estados[indice_estado].candidatos[i],
                           candidato) == 0)
                {
                    indice_candidato = i;
                    break;
                }
            }

            if (indice_candidato == -1)
            {
                if (estados[indice_estado].quantidade_candidatos
                    >= TOTAL_CANDIDATOS)
                {
                    printf("Limite de candidatos excedido para %s!\n", uf);
                    continue;
                }
                indice_candidato =
                    estados[indice_estado].quantidade_candidatos;
                strncpy(estados[indice_estado].candidatos[indice_candidato],
                        candidato,
                        TAMANHO_NOME_CANDIDATO - 1);
                estados[indice_estado]
                    .candidatos[indice_candidato]
                    [TAMANHO_NOME_CANDIDATO - 1] = '\0';
                estados[indice_estado].quantidade_candidatos++;
            }

            estados[indice_estado].votos[indice_candidato] = percentual;
        }
    }

    fclose(arquivo);

    printf("Total de estados carregados: %d\n", total_carregados);
}

void busa_largura_fronteiras(Grafo *g, int estado_origem)
{
    if (g == NULL)
    {
        printf("Grafo invalido!\n");
        return;
    }
    if (g->quantidade_vertices == 0)
    {
        printf("Grafo vazio!\n");
        return;
    }

    bool *visitado = (bool *)malloc(g->quantidade_vertices * sizeof(bool));
    int *distancia = (int *)malloc(g->quantidade_vertices * sizeof(int));
    int *antecessor = (int *)malloc(g->quantidade_vertices * sizeof(int));
    int *fila = (int *)malloc(g->quantidade_vertices * sizeof(int));
    int start = 0, end = 0;
    size_t i;

    if (visitado == NULL || distancia == NULL || antecessor == NULL || fila == NULL)
    {
        printf("erro de alocaocao memoria !\n");
        free(visitado);
        free(distancia);
        free(antecessor);
        free(fila);
        return;
    }

    for (i = 0; i < g->quantidade_vertices; i++)
    {
        visitado[i] = false;
        distancia[i] = -1;
        antecessor[i] = -1;
    }

    if (estado_origem < 0 || (size_t)estado_origem >= g->quantidade_vertices)
    {
        printf("Estado de origem invalido!\n");

        free(visitado);
        free(distancia);
        free(antecessor);
        free(fila);

        return;
    }

    visitado[estado_origem] = true;
    distancia[estado_origem] = 0;
    fila[end++] = estado_origem;

    printf("BFS: ");
    while (start < end)
    {
        int v = fila[start++];

        printf("Estado: %s | Distancia: %d\n",
               g->vertices[v].rotulo,
               distancia[v]);

        No *atual = g->vertices[v].adjacencias.primeiro;

        while (atual != NULL)
        {
            int vizinho = atual->destino;

            if (vizinho >= 0 && (size_t)vizinho < g->quantidade_vertices && !visitado[vizinho])
            {
                if (end < (int)g->quantidade_vertices)
                {
                    visitado[vizinho] = true;
                    distancia[vizinho] = distancia[v] + 1;
                    antecessor[vizinho] = v;
                    fila[end++] = vizinho;
                }
            }
            atual = atual->proximo;
        }
    }

    printf("\n");

    free(visitado);
    free(distancia);
    free(antecessor);
    free(fila);
}

void busca_profundidade_clusters(Grafo *g, float limiar_corte)
{
    bool *visitado = calloc(g->quantidade_vertices, sizeof(bool));

    if (visitado == NULL)
    {
        printf("Erro na alocacao de memoria!\n");
        return;
    }

    size_t i;

    for (i = 0; i < g->quantidade_vertices; i++)
    {
        if (!visitado[i])
        {
            printf("Novo cluster:\n");
            DFS_REC(g, (int)i, limiar_corte, visitado);
            printf("\n");
        }
    }

    free(visitado);
}
void DFS_REC(Grafo *g, int vertice, float limiar_corte, bool visitado[])
{
    visitado[vertice] = true;

    printf("%s ", g->vertices[vertice].rotulo);

    No *atual = g->vertices[vertice].adjacencias.primeiro;

    while (atual != NULL)
    {
        int vizinho = atual->destino;
        if (atual->peso >= limiar_corte && !visitado[vizinho])
        {
            DFS_REC(g, vizinho, limiar_corte, visitado);
        }
        atual = atual->proximo;
    }
}

int comparar_arestas(const void *a, const void *b)
{
    const Aresta *aresta_a = (const Aresta *)a;
    const Aresta *aresta_b = (const Aresta *)b;

    if (aresta_a->peso < aresta_b->peso)
        return -1;

    if (aresta_a->peso > aresta_b->peso)
        return 1;

    return 0;
}

int encontrar(int pai[], int vertice)
{
    if (pai[vertice] != vertice)
    {
        pai[vertice] = encontrar(pai, pai[vertice]);
    }

    return pai[vertice];
}

void unir(int pai[], int rank[], int a, int b)
{
    int raiz_a = encontrar(pai, a);
    int raiz_b = encontrar(pai, b);

    if (raiz_a == raiz_b)
        return;

    if (rank[raiz_a] < rank[raiz_b])
    {
        pai[raiz_a] = raiz_b;
    }
    else if (rank[raiz_a] > rank[raiz_b])
    {
        pai[raiz_b] = raiz_a;
    }
    else
    {
        pai[raiz_b] = raiz_a;
        rank[raiz_a]++;
    }
}

void calcular_mst_eleitoral_kruskal(Grafo *g_pres)
{
    if (g_pres == NULL)
    {
        printf("Grafo invalido!\n");
        return;
    }

    size_t quantidade = g_pres->quantidade_vertices;

    if (quantidade == 0)
        return;
    if (quantidade == 1)
    {
        printf("\nARVORE DE AFINIDADE ELEITORAL\n");
        printf("Arestas selecionadas: 0\n");
        printf("Arvore geradora completa!\n");
        return;
    }

    size_t total_arestas = 0;
    size_t k;

    for (k = 0; k < quantidade; k++)
    {
        No *atual = g_pres->vertices[k].adjacencias.primeiro;

        while (atual != NULL)
        {
            if (atual->destino >= 0 &&
                (size_t)atual->destino < quantidade &&
                (size_t)atual->destino > k)
            {
                total_arestas++;
            }

            atual = atual->proximo;
        }
    }

    Aresta *arestas = malloc(total_arestas * sizeof(Aresta));

    int *pai = malloc(quantidade * sizeof(int));
    int *rank = calloc(quantidade, sizeof(int));

    if ((total_arestas > 0 && arestas == NULL) ||
        pai == NULL || rank == NULL)
    {
        printf("Erro na alocacao de memoria!\n");

        free(arestas);
        free(pai);
        free(rank);

        return;
    }

    size_t indice = 0;
    size_t i;
    for (i = 0; i < quantidade; i++)
    {
        No *atual = g_pres->vertices[i].adjacencias.primeiro;

        while (atual != NULL)
        {
            if (atual->destino >= 0 &&
                (size_t)atual->destino < quantidade &&
                (size_t)atual->destino > i)
            {
                arestas[indice].origem = (int)i;
                arestas[indice].destino = atual->destino;
                /* g_pres guarda similaridade. Kruskal minimiza
                   divergencia, entao converte o peso em distancia:
                   distancia = 1 - similaridade. */
                arestas[indice].peso = 1.0f - atual->peso;

                indice++;
            }

            atual = atual->proximo;
        }
    }

    qsort(arestas, total_arestas, sizeof(Aresta), comparar_arestas);

    size_t f;
    for (f = 0; f < quantidade; f++)
    {
        pai[f] = (int)f;
    }

    size_t selecionadas = 0;

    printf("\nARVORE ELEITORAL MINIMA (KRUSKAL)\n");
    float peso_total = 0.0f;
    size_t j;
    for (j = 0; j < total_arestas && selecionadas < quantidade - 1; j++)
    {
        int origem = arestas[j].origem;
        int destino = arestas[j].destino;

        int raiz_origem = encontrar(pai, origem);
        int raiz_destino = encontrar(pai, destino);

        if (raiz_origem != raiz_destino)
        {
            unir(pai, rank, origem, destino);

            printf("%s -- %s | Distancia: %.6f\n",
                   g_pres->vertices[origem].rotulo,
                   g_pres->vertices[destino].rotulo,
                   arestas[j].peso);

            peso_total += arestas[j].peso;
            selecionadas++;
        }
    }

    printf("\nArestas selecionadas: %lu\n", (unsigned long)selecionadas);
    printf("Peso total da AGM: %.6f\n", peso_total);

    if (selecionadas == quantidade - 1)
    {
        printf("Arvore geradora completa!\n");
    }
    else
    {
        printf("Grafo desconexo: foi gerada uma floresta.\n");
    }

    free(arestas);
    free(pai);
    free(rank);
}

Grafo *construir_grafo_similaridade(Estado estados[], size_t total)
{
    Grafo *g_pres = grafo_criar(0);
    size_t i, j;

    if (g_pres == NULL)
        return NULL;

    for (i = 0; i < total; i++)
    {
        grafo_adicionar_vertice(g_pres, estados[i].uf);
    }

    for (i = 0; i < total; i++)
    {
        for (j = i + 1; j < total; j++)
        {
            float diferenca_acumulada = 0.0f;
            float distancia_normalizada;
            float similaridade;
            size_t candidato;
            int quantidade =
                estados[i].quantidade_candidatos;

            if (estados[j].quantidade_candidatos < quantidade)
                quantidade =
                    estados[j].quantidade_candidatos;

            for (candidato = 0;
                 candidato < (size_t)quantidade;
                 candidato++)
            {
                float diferenca =
                    estados[i].votos[candidato] -
                    estados[j].votos[candidato];
                if (diferenca < 0.0f)
                    diferenca = -diferenca;
                diferenca_acumulada += diferenca;
            }

            if (quantidade > 0)
            {
                distancia_normalizada =
                    diferenca_acumulada /
                    ((float)quantidade * 100.0f);
                if (distancia_normalizada > 1.0f)
                    distancia_normalizada = 1.0f;
                similaridade = 1.0f - distancia_normalizada;
            }
            else
            {
                similaridade = 0.0f;
            }

            printf("%s -- %s | Similaridade: %.6f\n",
                   estados[i].uf,
                   estados[j].uf,
                   similaridade);

            grafo_adicionar_aresta(g_pres, i, j, similaridade);
        }
    }

    return g_pres;
}

Grafo *construir_grafo_territorial(void)
{
    Grafo *g_terr = grafo_criar(0);

    if (g_terr == NULL)
    {
        printf("Erro ao criar grafo territorial!\n");
        return NULL;
    }

    const char *estados[TOTAL_ESTADOS] = {
        "AC", "AL", "AP", "AM", "BA", "CE", "DF",
        "ES", "GO", "MA", "MT", "MS", "MG", "PA",
        "PB", "PR", "PE", "PI", "RJ", "RN", "RS",
        "RO", "RR", "SC", "SP", "SE", "TO"};

    int i;
    for (i = 0; i < TOTAL_ESTADOS; i++)
    {
        int resultado = grafo_adicionar_vertice(g_terr, estados[i]);

        if (resultado == -1)
        {
            printf("Erro ao adicionar o vertice: %s\n", estados[i]);
            grafo_destruir(g_terr);
            return NULL;
        }
    }

    const char *fronteiras[][2] = {
        {"AC", "AM"}, {"AC", "RO"}, {"AL", "BA"}, {"AL", "PE"}, {"AL", "SE"}, {"AP", "PA"}, {"AM", "MT"}, {"AM", "PA"}, {"AM", "RO"}, {"AM", "RR"}, {"BA", "ES"}, {"BA", "GO"}, {"BA", "MG"}, {"BA", "PE"}, {"BA", "PI"}, {"BA", "SE"}, {"BA", "TO"}, {"CE", "PB"}, {"CE", "PE"}, {"CE", "PI"}, {"CE", "RN"}, {"DF", "GO"}, {"DF", "MG"}, {"ES", "MG"}, {"ES", "RJ"}, {"GO", "MG"}, {"GO", "MS"}, {"GO", "MT"}, {"GO", "TO"}, {"MA", "PA"}, {"MA", "PI"}, {"MA", "TO"}, {"MT", "MS"}, {"MT", "PA"}, {"MT", "RO"}, {"MT", "TO"}, {"MS", "MG"}, {"MS", "PR"}, {"MS", "SP"}, {"MG", "RJ"}, {"MG", "SP"}, {"PA", "AP"}, {"PA", "RR"}, {"PA", "TO"}, {"PB", "PE"}, {"PB", "RN"}, {"PE", "PI"}, {"PI", "TO"}, {"PR", "SC"}, {"PR", "SP"}, {"RJ", "SP"}, {"RS", "SC"}};

    size_t total = sizeof(fronteiras) / sizeof(fronteiras[0]);

    size_t j;
    for (j = 0; j < total; j++)
    {
        int origem = grafo_localizar_vertice(g_terr, fronteiras[j][0]);
        int destino = grafo_localizar_vertice(g_terr, fronteiras[j][1]);

        if (origem == -1 || destino == -1)
        {
            printf("Erro ao localizar estado!\n");
            grafo_destruir(g_terr);
            return NULL;
        }

        grafo_adicionar_aresta(g_terr, origem, destino, 1.0f);
    }

    return g_terr;
}
