# Estrutura do grafo

Esta implementacao representa grafos por **listas de adjacencias dinamicas**:
cada vertice possui uma lista encadeada de vizinhos, e cada lista contem
arestas ponderadas com ponteiros.

## Estruturas

- `struct No`: representa uma aresta. Guarda `destino`, `peso` e `proximo`.
- `struct Lista`: guarda o primeiro `No` da adjacencia de um vertice.
- `struct Vertice`: guarda o rotulo em string e sua `Lista`.
- `struct Grafo`: guarda o vetor dinamico de vertices, a quantidade, a
  capacidade e a informacao se o grafo e direcionado.

Os vertices sao identificados internamente por indices inteiros. Os rotulos
evitam que os modulos da aplicacao dependam de posicoes fixas.

## Politica de arestas duplicadas

Quando uma aresta ja existe, `grafo_adicionar_aresta` retorna `0` e nao altera
nem cria outra conexao. O peso nao sera substituido.

## Criacao e adicao

```c
#include "grafo.h"

Grafo *g = grafo_criar(0); /* 0 = nao direcionado */
int df = grafo_adicionar_vertice(g, "DF");
int go = grafo_adicionar_vertice(g, "GO");
int mg = grafo_adicionar_vertice(g, "MG");

grafo_adicionar_aresta(g, df, go, 1.0f);
grafo_adicionar_aresta(g, go, mg, 0.75f);
grafo_exibir(g);
grafo_destruir(g);
```

Para grafo direcionado, use `grafo_criar(1)`. Nesse modo, uma chamada de
origem para destino cria somente o sentido informado.

## Consultas

```c
int indice = grafo_localizar_vertice(g, "DF");

if (grafo_consultar_aresta(g, df, go) == 1) {
    float peso;
    if (grafo_consultar_peso(g, df, go, &peso) == 1) {
        /* peso contem o valor da aresta */
    }
}
```

`grafo_consultar_aresta` retorna `1` se existir, `0` se nao existir e `-1`
para indices ou grafo invalidos. `grafo_consultar_peso` retorna `1` se o peso
foi encontrado, `0` se nao existe aresta e `-1` para entrada invalida.

## Memoria

`grafo_adicionar_vertice` utiliza reallocacao dinamica do vetor de vertices.
Cada rotulo e copiado com `malloc`. Cada aresta e alocada com `malloc`.
`grafo_destruir` percorre todas as listas, libera os nos, os rotulos e os
buffers de vertices e por fim libera o proprio grafo.

Esta estrutura nao implementa BFS, DFS, Kruskal ou carregamento de CSV.
