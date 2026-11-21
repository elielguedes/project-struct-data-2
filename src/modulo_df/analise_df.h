#ifndef ANALISE_DF_H
#define ANALISE_DF_H
#include "grafos/grafo.h"

Grafo *construir_grafo_bipartido_df(const char *arquivo_csv);
void candidatos_dominantes_por_ra(Grafo *grafo);
Grafo *gerar_grafo_similaridade_ra(Grafo *grafo_bipartido,
                                   float limiar_similaridade);

#endif