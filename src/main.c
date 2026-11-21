#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "grafos/grafo.h"
#include "modulo_df/analise_df.h"
#include "modulo_nacional/analise_nacional.h"

#define ARQUIVO_DF_PROCESSADO "dados/processados/votacao_governador_df_2026.csv"
#define ARQUIVO_DF_SINTETICO "testes/fixtures/votacao_governador_df_2026.csv"
#define LIMIAR_SIMILARIDADE_DF 0.99f
#define LIMIAR_CLUSTER_NACIONAL 0.95f

static void exibir_cabecalho(void)
{
    puts("==============================================================");
    puts("           ANALISE ELEITORAL 2026 - GRAFOS");
    puts("==============================================================");
}

static void exibir_menu(void)
{
    puts("");
    puts("Selecione a analise:");
    puts("  1 - Analise Distrital: Governador do DF");
    puts("  2 - Analise Nacional: Presidente");
    puts("  3 - Executar as duas analises");
    puts("  0 - Sair");
    puts("");
    printf("Opcao: ");
}

static int executar_analise_df(void)
{
    Grafo *bipartido;
    Grafo *similaridade;
    const char *arquivo_df;
    FILE *arquivo_teste;

    arquivo_teste = fopen(ARQUIVO_DF_PROCESSADO, "r");
    if (arquivo_teste != NULL)
    {
        fclose(arquivo_teste);
        arquivo_df = ARQUIVO_DF_PROCESSADO;
    }
    else
    {
        arquivo_df = ARQUIVO_DF_SINTETICO;
    }

    puts("");
    puts("--------------------------------------------------------------");
    puts("ANALISE 1 - DISTRITO FEDERAL: GOVERNADOR");
    puts("--------------------------------------------------------------");
    printf("Arquivo de dados: %s\n", arquivo_df);

    bipartido = construir_grafo_bipartido_df(arquivo_df);
    if (bipartido == NULL)
    {
        fprintf(stderr,
                "Nao foi possivel construir o grafo bipartido do DF.\n");
        return 0;
    }

    puts("");
    puts("[DF 1/3] Candidatos dominantes por Regiao Administrativa");
    candidatos_dominantes_por_ra(bipartido);

    puts("");
    puts("[DF 2/3] Grafo bipartido RA-Candidato");
    grafo_exibir(bipartido);

    puts("");
    puts("[DF 3/3] Projecao de similaridade entre RAs");
    printf("Limiar de similaridade: %.2f\n", LIMIAR_SIMILARIDADE_DF);
    similaridade = gerar_grafo_similaridade_ra(
        bipartido, LIMIAR_SIMILARIDADE_DF);

    if (similaridade == NULL)
    {
        fprintf(stderr,
                "Nao foi possivel construir a projecao de similaridade.\n");
        grafo_destruir(bipartido);
        return 0;
    }

    grafo_exibir(similaridade);

    grafo_destruir(similaridade);
    grafo_destruir(bipartido);
    puts("");
    puts("Analise do DF concluida.");
    return 1;
}

static int executar_analise_nacional(void)
{
    Estado estados[TOTAL_ESTADOS];
    Grafo *g_pres;
    Grafo *g_terr;
    int origem;
    int i;

    puts("");
    puts("--------------------------------------------------------------");
    puts("ANALISE 2 - BRASIL: PRESIDENTE");
    puts("--------------------------------------------------------------");

    memset(estados, 0, sizeof(estados));
    carregar_estados(estados, TOTAL_ESTADOS);

    puts("");
    puts("[NACIONAL 1/4] Dados presidenciais carregados por UF");
    for (i = 0; i < TOTAL_ESTADOS; ++i)
    {
        int candidato;
        printf("%s |", estados[i].uf);
        for (candidato = 0;
             candidato < estados[i].quantidade_candidatos;
             ++candidato)
        {
            printf(" %s: %.2f%%",
                   estados[i].candidatos[candidato],
                   estados[i].votos[candidato]);
        }
        printf("\n");
    }

    g_pres = construir_grafo_similaridade(
        estados, (size_t)TOTAL_ESTADOS);
    if (g_pres == NULL)
    {
        fprintf(stderr,
                "Nao foi possivel construir o grafo presidencial.\n");
        return 0;
    }

    g_terr = construir_grafo_territorial();
    if (g_terr == NULL)
    {
        fprintf(stderr,
                "Nao foi possivel construir o grafo territorial.\n");
        grafo_destruir(g_pres);
        return 0;
    }

    puts("");
    puts("[NACIONAL 2/4] Grafo territorial e BFS a partir do DF");
    origem = grafo_localizar_vertice(g_terr, "DF");
    if (origem != -1)
    {
        busa_largura_fronteiras(g_terr, origem);
    }
    else
    {
        fprintf(stderr, "Vertice DF nao encontrado no grafo territorial.\n");
    }

    puts("");
    puts("[NACIONAL 3/4] Grafo presidencial e clusters por DFS");
    printf("Limiar de similaridade: %.2f\n",
           LIMIAR_CLUSTER_NACIONAL);
    busca_profundidade_clusters(
        g_pres, LIMIAR_CLUSTER_NACIONAL);

    puts("");
    puts("[NACIONAL 4/4] Arvore Geradora Minima - Kruskal");
    calcular_mst_eleitoral_kruskal(g_pres);

    grafo_destruir(g_terr);
    grafo_destruir(g_pres);

    puts("");
    puts("Analise nacional concluida.");
    return 1;
}

static int ler_opcao(int *opcao)
{
    char linha[32];

    if (fgets(linha, sizeof(linha), stdin) == NULL)
        return 0;
    if (sscanf(linha, "%d", opcao) != 1)
        *opcao = -1;
    return 1;
}

int main(int argc, char *argv[])
{
    int opcao;
    int executou = 0;

    exibir_cabecalho();

    if (argc > 1)
    {
        if (strcmp(argv[1], "--df") == 0)
        {
            opcao = 1;
        }
        else if (strcmp(argv[1], "--nacional") == 0)
        {
            opcao = 2;
        }
        else if (strcmp(argv[1], "--todos") == 0)
        {
            opcao = 3;
        }
        else
        {
            fprintf(stderr,
                    "Uso: %s [--df | --nacional | --todos]\n",
                    argv[0]);
            return EXIT_FAILURE;
        }
    }
    else
    {
        exibir_menu();
        if (!ler_opcao(&opcao))
            return EXIT_FAILURE;
    }

    switch (opcao)
    {
    case 1:
        executou = executar_analise_df();
        break;
    case 2:
        executou = executar_analise_nacional();
        break;
    case 3:
        executou = executar_analise_df();
        if (!executar_analise_nacional())
            executou = 0;
        break;
    case 0:
        puts("Encerrando o programa.");
        return EXIT_SUCCESS;
    default:
        fprintf(stderr, "Opcao invalida.\n");
        return EXIT_FAILURE;
    }

    return executou ? EXIT_SUCCESS : EXIT_FAILURE;
}
