#include "../src/grafos/grafo.h"
#include "../src/modulo_nacional/analise_nacional.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void exigir(int condicao, const char *mensagem)
{
    if (!condicao)
    {
        fprintf(stderr, "FALHOU: %s\n", mensagem);
        exit(EXIT_FAILURE);
    }
}

static size_t contar_arestas_direcionadas(const Grafo *grafo)
{
    size_t i;
    size_t total;

    total = 0;
    for (i = 0; i < grafo->quantidade_vertices; ++i)
    {
        No *atual = grafo->vertices[i].adjacencias.primeiro;
        while (atual != NULL)
        {
            ++total;
            atual = atual->proximo;
        }
    }
    return total;
}

int main(void)
{
    Estado estados[TOTAL_ESTADOS];
    Grafo *g_pres;
    Grafo *g_terr;
    int df;
    size_t i;

    memset(estados, 0, sizeof(estados));
    carregar_estados(estados, TOTAL_ESTADOS);

    for (i = 0; i < TOTAL_ESTADOS; ++i)
    {
        exigir(estados[i].uf[0] != '\0',
               "carregamento de todas as UFs");
    }

    g_pres = construir_grafo_similaridade(
        estados, (size_t)TOTAL_ESTADOS);
    exigir(g_pres != NULL,
           "construcao do grafo de similaridade presidencial");
    exigir(g_pres->quantidade_vertices == 27,
           "quantidade de UFs no grafo presidencial");
    exigir(contar_arestas_direcionadas(g_pres) == 351 * 2,
           "grafo presidencial completo nao direcionado");

    g_terr = construir_grafo_territorial();
    exigir(g_terr != NULL,
           "construcao do grafo territorial");
    exigir(g_terr->quantidade_vertices == 27,
           "quantidade de UFs no grafo territorial");

    df = grafo_localizar_vertice(g_terr, "DF");
    exigir(df >= 0, "localizacao do DF no grafo territorial");

    busa_largura_fronteiras(g_terr, df);
    busca_profundidade_clusters(g_pres, 0.95f);
    calcular_mst_eleitoral_kruskal(g_pres);

    grafo_destruir(g_terr);
    grafo_destruir(g_pres);

    puts("TODOS OS TESTES DO MODULO NACIONAL PASSARAM.");
    return EXIT_SUCCESS;
}
