#ifndef ANALISE_NACIONAL_H
#define ANALISE_NACIONAL_H

#include "grafos/grafo.h"
#include <stdbool.h>

#define TOTAL_ESTADOS 27
#define TOTAL_CANDIDATOS 16
#define TAMANHO_NOME_CANDIDATO 128

typedef struct
{
    char uf[3];
    char candidatos[TOTAL_CANDIDATOS][TAMANHO_NOME_CANDIDATO];
    int quantidade_candidatos;
    float votos[TOTAL_CANDIDATOS];
} Estado;

void carregar_estados(Estado estados[], int quantidade);
void busa_largura_fronteiras(Grafo *g, int estado_origem);
void busca_profundidade_clusters(Grafo *g, float limiar_corte);
void DFS_REC(Grafo *g, int vertice, float limiar_corte, bool visitado[]);
int comparar_arestas(const void *a, const void *b);
void calcular_mst_eleitoral_kruskal(Grafo *g_pres);
int encontrar(int pai[], int vertice);
void unir(int pai[], int rank[], int a, int b);
Grafo *construir_grafo_similaridade(Estado estados[], size_t total);
Grafo *construir_grafo_territorial(void);

#endif