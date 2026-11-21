#include "../include/grafo.h"

#include <stdio.h>
#include <stdlib.h>

static void exigir(int condicao, const char *mensagem)
{
    if (!condicao) {
        fprintf(stderr, "FALHOU: %s\n", mensagem);
        exit(EXIT_FAILURE);
    }
}

static void testar_nao_direcionado(void)
{
    Grafo *grafo;
    int df;
    int go;
    int mg;
    float peso;

    grafo = grafo_criar(0);
    exigir(grafo != NULL, "criacao do grafo nao direcionado");

    df = grafo_adicionar_vertice(grafo, "DF");
    go = grafo_adicionar_vertice(grafo, "GO");
    mg = grafo_adicionar_vertice(grafo, "MG");
    exigir(df >= 0 && go >= 0 && mg >= 0,
           "adicao dos vertices DF, GO e MG");
    exigir(grafo->quantidade_vertices == 3,
           "quantidade de vertices no grafo");

    exigir(grafo_localizar_vertice(grafo, "DF") == df,
           "localizacao do vertice DF");
    exigir(grafo_localizar_vertice(grafo, "GO") == go,
           "localizacao do vertice GO");
    exigir(grafo_localizar_vertice(grafo, "inexistente") == -1,
           "vertice inexistente");

    exigir(grafo_adicionar_aresta(grafo, df, go, 1.25f) == 1,
           "adicao da aresta DF-GO");
    exigir(grafo_adicionar_aresta(grafo, df, go, 9.0f) == 0,
           "prevencao de aresta duplicada");
    exigir(grafo_adicionar_aresta(grafo, go, mg, 0.75f) == 1,
           "adicao da aresta GO-MG");

    exigir(grafo_consultar_aresta(grafo, df, go) == 1,
           "consulta da aresta DF-GO");
    exigir(grafo_consultar_aresta(grafo, go, df) == 1,
           "aresta reversa em grafo nao direcionado");
    exigir(grafo_consultar_aresta(grafo, df, mg) == 0,
           "consulta de aresta inexistente");

    exigir(grafo_consultar_peso(grafo, df, go, &peso) == 1,
           "consulta do peso DF-GO");
    exigir(peso == 1.25f, "valor do peso DF-GO");
    exigir(grafo_consultar_peso(grafo, go, df, &peso) == 1,
           "consulta do peso reverso DF-GO");
    exigir(peso == 1.25f, "peso reverso deve ser identico");

    grafo_exibir(grafo);
    grafo_destruir(grafo);
}

static void testar_direcionado(void)
{
    Grafo *grafo;
    int origem;
    int destino;

    grafo = grafo_criar(1);
    exigir(grafo != NULL, "criacao do grafo direcionado");

    origem = grafo_adicionar_vertice(grafo, "Origem");
    destino = grafo_adicionar_vertice(grafo, "Destino");
    exigir(grafo_adicionar_aresta(grafo, origem, destino, 5.5f) == 1,
           "adicao da aresta direcionada");
    exigir(grafo_consultar_aresta(grafo, origem, destino) == 1,
           "aresta na direcao informada");
    exigir(grafo_consultar_aresta(grafo, destino, origem) == 0,
           "ausencia da aresta reversa");

    grafo_destruir(grafo);
}

static void testar_entradas_invalidas(void)
{
    Grafo *grafo;
    float peso;

    grafo = grafo_criar(0);
    exigir(grafo_adicionar_vertice(grafo, "") == -1,
           "rejeicao de rotulo vazio");
    exigir(grafo_adicionar_vertice(grafo, NULL) == -1,
           "rejeicao de rotulo nulo");
    exigir(grafo_adicionar_aresta(grafo, -1, 0, 1.0f) == -1,
           "rejeicao de indice invalido");
    exigir(grafo_consultar_aresta(grafo, -1, 0) == -1,
           "consulta com indice invalido");
    exigir(grafo_consultar_peso(grafo, 0, 0, &peso) == 0 ||
           grafo_consultar_peso(grafo, 0, 0, &peso) == -1,
           "consulta segura em grafo vazio");

    grafo_destruir(grafo);
    grafo_destruir(NULL);
}

int main(void)
{
    testar_nao_direcionado();
    testar_direcionado();
    testar_entradas_invalidas();
    puts("TODOS OS TESTES DO MODULO 4.1 PASSARAM.");
    return EXIT_SUCCESS;
}
