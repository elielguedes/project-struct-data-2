#include "grafo.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char *copiar_rotulo(const char *rotulo)
{
    char *copia;
    size_t tamanho;

    tamanho = strlen(rotulo) + 1;
    copia = (char *)malloc(tamanho);
    if (copia != NULL)
    {
        memcpy(copia, rotulo, tamanho);
    }
    return copia;
}

Grafo *grafo_criar(int direcionado)
{
    Grafo *grafo;

    grafo = (Grafo *)calloc(1, sizeof(Grafo));
    if (grafo != NULL)
    {
        grafo->direcionado = direcionado ? 1 : 0;
    }
    return grafo;
}

void grafo_destruir(Grafo *grafo)
{
    size_t i;
    No *atual;
    No *proximo;

    if (grafo == NULL)
    {
        return;
    }

    for (i = 0; i < grafo->quantidade_vertices; ++i)
    {
        free(grafo->vertices[i].rotulo);
        atual = grafo->vertices[i].adjacencias.primeiro;
        while (atual != NULL)
        {
            proximo = atual->proximo;
            free(atual);
            atual = proximo;
        }
    }
    free(grafo->vertices);
    free(grafo);
}

int grafo_localizar_vertice(const Grafo *grafo, const char *rotulo)
{
    size_t i;

    if (grafo == NULL || rotulo == NULL || rotulo[0] == '\0')
    {
        return -1;
    }

    for (i = 0; i < grafo->quantidade_vertices; ++i)
    {
        if (strcmp(grafo->vertices[i].rotulo, rotulo) == 0)
        {
            return (int)i;
        }
    }
    return -1;
}

int grafo_adicionar_vertice(Grafo *grafo, const char *rotulo)
{
    Vertice *novos_vertices;
    size_t nova_capacidade;
    char *copia;
    int existente;

    if (grafo == NULL || rotulo == NULL || rotulo[0] == '\0')
    {
        return -1;
    }

    existente = grafo_localizar_vertice(grafo, rotulo);
    if (existente >= 0)
    {
        return existente;
    }

    if (grafo->quantidade_vertices >= grafo->capacidade_vertices)
    {
        nova_capacidade = grafo->capacidade_vertices == 0
                              ? 8
                              : grafo->capacidade_vertices * 2;
        if (nova_capacidade < grafo->capacidade_vertices ||
            nova_capacidade > ((size_t)-1) / sizeof(Vertice))
        {
            return -1;
        }

        novos_vertices = (Vertice *)realloc(
            grafo->vertices, nova_capacidade * sizeof(Vertice));
        if (novos_vertices == NULL)
        {
            return -1;
        }
        grafo->vertices = novos_vertices;
        grafo->capacidade_vertices = nova_capacidade;
    }

    copia = copiar_rotulo(rotulo);
    if (copia == NULL)
    {
        return -1;
    }

    grafo->vertices[grafo->quantidade_vertices].rotulo = copia;
    grafo->vertices[grafo->quantidade_vertices].adjacencias.primeiro = NULL;
    ++grafo->quantidade_vertices;
    return (int)(grafo->quantidade_vertices - 1);
}

static No *encontrar_aresta(const Grafo *grafo, int origem, int destino)
{
    No *atual;

    if (grafo == NULL || origem < 0 || destino < 0 ||
        (size_t)origem >= grafo->quantidade_vertices ||
        (size_t)destino >= grafo->quantidade_vertices)
    {
        return NULL;
    }

    atual = grafo->vertices[origem].adjacencias.primeiro;
    while (atual != NULL)
    {
        if (atual->destino == destino)
        {
            return atual;
        }
        atual = atual->proximo;
    }
    return NULL;
}

int grafo_adicionar_aresta(Grafo *grafo, int origem, int destino, float peso)
{
    No *novo;
    No *reverso;

    if (grafo == NULL || origem < 0 || destino < 0 ||
        (size_t)origem >= grafo->quantidade_vertices ||
        (size_t)destino >= grafo->quantidade_vertices)
    {
        return -1;
    }

    if (encontrar_aresta(grafo, origem, destino) != NULL)
    {
        return 0;
    }

    novo = (No *)malloc(sizeof(No));
    if (novo == NULL)
    {
        return -1;
    }
    novo->destino = destino;
    novo->peso = peso;
    novo->proximo = grafo->vertices[origem].adjacencias.primeiro;
    grafo->vertices[origem].adjacencias.primeiro = novo;

    if (!grafo->direcionado && origem != destino)
    {
        reverso = (No *)malloc(sizeof(No));
        if (reverso == NULL)
        {
            grafo->vertices[origem].adjacencias.primeiro = novo->proximo;
            free(novo);
            return -1;
        }
        reverso->destino = origem;
        reverso->peso = peso;
        reverso->proximo = grafo->vertices[destino].adjacencias.primeiro;
        grafo->vertices[destino].adjacencias.primeiro = reverso;
    }

    return 1;
}

int grafo_consultar_aresta(const Grafo *grafo, int origem, int destino)
{
    if (grafo == NULL || origem < 0 || destino < 0 ||
        (size_t)origem >= grafo->quantidade_vertices ||
        (size_t)destino >= grafo->quantidade_vertices)
    {
        return -1;
    }
    return encontrar_aresta(grafo, origem, destino) != NULL ? 1 : 0;
}

int grafo_consultar_peso(const Grafo *grafo, int origem, int destino,
                         float *peso)
{
    No *aresta;

    if (peso == NULL)
    {
        return -1;
    }

    aresta = encontrar_aresta(grafo, origem, destino);
    if (aresta == NULL)
    {
        if (grafo == NULL || origem < 0 || destino < 0 ||
            (size_t)origem >= grafo->quantidade_vertices ||
            (size_t)destino >= grafo->quantidade_vertices)
        {
            return -1;
        }
        return 0;
    }
    *peso = aresta->peso;
    return 1;
}

void grafo_exibir(const Grafo *grafo)
{
    size_t i;
    No *atual;

    if (grafo == NULL)
    {
        printf("(grafo nulo)\n");
        return;
    }

    printf("Grafo %s\n",
           grafo->direcionado ? "direcionado" : "nao direcionado");
    for (i = 0; i < grafo->quantidade_vertices; ++i)
    {
        printf("[%lu] %s:", (unsigned long)i,
               grafo->vertices[i].rotulo);
        atual = grafo->vertices[i].adjacencias.primeiro;
        while (atual != NULL)
        {
            printf(" -> %s (peso %.2f)",
                   grafo->vertices[atual->destino].rotulo,
                   atual->peso);
            atual = atual->proximo;
        }
        printf("\n");
    }
}
