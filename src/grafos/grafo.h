#ifndef GRAFO_H
#define GRAFO_H

#include <stddef.h>

typedef struct No
{
    int destino;
    float peso;
    struct No *proximo;
} No;

typedef struct Lista
{
    No *primeiro;
} Lista;

typedef struct Vertice
{
    char *rotulo;
    Lista adjacencias;
} Vertice;

typedef struct Grafo
{
    Vertice *vertices;
    size_t quantidade_vertices;
    size_t capacidade_vertices;
    int direcionado;
} Grafo;

Grafo *grafo_criar(int direcionado);
void grafo_destruir(Grafo *grafo);

int grafo_adicionar_vertice(Grafo *grafo, const char *rotulo);
int grafo_localizar_vertice(const Grafo *grafo, const char *rotulo);

int grafo_adicionar_aresta(Grafo *grafo, int origem, int destino, float peso);

int grafo_consultar_aresta(const Grafo *grafo, int origem, int destino);
int grafo_consultar_peso(const Grafo *grafo, int origem, int destino, float *peso);

void grafo_exibir(const Grafo *grafo);

#endif
