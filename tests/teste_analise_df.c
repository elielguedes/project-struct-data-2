#include "../src/modulo_df/analise_df.h"

#include <stdio.h>
#include <stdlib.h>

int main(void)
{
    Grafo *bipartido;
    Grafo *similaridade;
    int plano;
    int taguatinga;
    int candidato_a;
    float peso;

    bipartido = construir_grafo_bipartido_df(
        "testes/fixtures/votacao_governador_df_2026.csv");
    if (bipartido == NULL)
    {
        fprintf(stderr, "Falha ao carregar o CSV sintetico do DF.\n");
        return EXIT_FAILURE;
    }

    plano = grafo_localizar_vertice(bipartido, "Plano Piloto");
    taguatinga = grafo_localizar_vertice(bipartido, "Taguatinga");
    candidato_a = grafo_localizar_vertice(bipartido, "Cand_A");

    if (plano < 0 || taguatinga < 0 || candidato_a < 0)
    {
        fprintf(stderr, "Vertice do DF nao encontrado.\n");
        grafo_destruir(bipartido);
        return EXIT_FAILURE;
    }

    if (grafo_consultar_peso(bipartido, plano, candidato_a, &peso) != 1 ||
        peso != 45.0f)
    {
        fprintf(stderr, "Peso bipartido incorreto.\n");
        grafo_destruir(bipartido);
        return EXIT_FAILURE;
    }

    candidatos_dominantes_por_ra(bipartido);

    similaridade = gerar_grafo_similaridade_ra(bipartido, 0.99f);
    if (similaridade == NULL)
    {
        fprintf(stderr, "Falha ao gerar a similaridade do DF.\n");
        grafo_destruir(bipartido);
        return EXIT_FAILURE;
    }

    int taguatinga_sim =
        grafo_localizar_vertice(similaridade, "Taguatinga");
    int ceilandia_sim =
        grafo_localizar_vertice(similaridade, "Ceilandia");
    if (taguatinga_sim < 0 || ceilandia_sim < 0 ||
        grafo_consultar_aresta(similaridade,
                               taguatinga_sim,
                               ceilandia_sim) != 1)
    {
        fprintf(stderr, "Similaridade Taguatinga-Ceilandia nao encontrada.\n");
        grafo_destruir(similaridade);
        grafo_destruir(bipartido);
        return EXIT_FAILURE;
    }

    grafo_destruir(similaridade);
    grafo_destruir(bipartido);
    puts("TODOS OS TESTES DO MODULO DF PASSARAM.");
    return EXIT_SUCCESS;
}
