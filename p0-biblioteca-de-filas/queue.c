#include "queue.h"
#include <stdio.h>

// Pesquisa sequencialmente se um elemento pertence a fila.
// Retorno: 1 se o elemento está na fila, 0 caso contrário.
static int queue_search(queue_t *queue, queue_t *elem) {
    if (queue == NULL || elem == NULL)
        return 0;

    queue_t *first = queue;

    // elem é o primeiro elemento da fila
    if (elem == first)
        return 1;

    queue_t *aux = first->next;

    // percore o restante da fila em busca do elemento
    while (aux != elem && aux != first)
        aux = aux->next;

    // percorreu toda fila e não achou o elemento
    if (aux == first)
        return 0;

    return 1;
}

int queue_size(queue_t *queue) {
    if (queue == NULL)
        return 0;

    queue_t *aux = queue->next;
    int count = 1;

    // percorre a fila e conta seus elementos
    while (aux != queue) {
        aux = aux->next;
        count++;
    }

    return count;
}

void queue_print(char *name, queue_t *queue, void print_elem(void *)) {
    printf("%s", name);

    if (queue == NULL) {
        printf(": []\n");
        return;
    }

    queue_t *elem = queue;
    printf(": [");
    print_elem(elem); // imprime o primeiro elemento

    // percorre a fila e imprime o restante dos elementos
    while ((elem = elem->next) != queue) {
        putchar(' ');
        print_elem(elem);
    }

    printf("]\n");
}

int queue_append(queue_t **queue, queue_t *elem) {
    if (queue == NULL) {
        fprintf(stderr, "### Erro: a fila não existe\n");
        return -1;
    }

    if (elem == NULL) {
        fprintf(stderr, "### Erro: o elemento não existe\n");
        return -2;
    }

    if (elem->prev != NULL || elem->next != NULL) {
        fprintf(stderr, "### Erro: tentou inserir um elemento que já está em outra fila\n");
        return -3;
    }

    if (*queue == NULL) { // fila vazia
        *queue = elem;

        // todos os ponteiros apontam para o primeiro elemento da fila
        elem->prev = elem;
        elem->next = elem;
    } else {
        // guarda o endereço do primeiro e do último elemento da fila atual
        queue_t *first = *queue;
        queue_t *last = first->prev;

        // define elem como o último elemento
        first->prev = elem;
        last->next = elem;

        // ajusta os ponteiros do novo último elemento
        elem->prev = last;
        elem->next = first;
    }

    return 0;
}

int queue_remove(queue_t **queue, queue_t *elem) {
    if (queue == NULL) {
        fprintf(stderr, "### Erro: a fila não existe\n");
        return -1;
    }

    if (*queue == NULL) {
        fprintf(stderr, "### Erro: a fila está vazia\n");
        return -2;
    }

    if (elem == NULL || (elem->prev == NULL && elem->next == NULL)) {
        fprintf(stderr, "### Erro: tentou remover um elemento inexistente\n");
        return -3;
    }

    // verifica se o elemento a ser removido pertence a fila
    if (!queue_search(*queue, elem)) {
        fprintf(stderr, "### Erro: tentou remover um elemento de outra fila\n");
        return -4;
    }

    queue_t *first = *queue;

    // a fila possui um único elemento
    if (first->prev == first && first->next == first)
        *queue = NULL;
    else {
        // o elemento a ser removido é o primeiro da fila
        if (first == elem) {
            *queue = first->next; // novo primeiro elemento
        }

        // salva o endereço dos elementos adjacentes de elem
        queue_t *left = elem->prev;
        queue_t *right = elem->next;

        // ajusta os ponteiros dos elementos adjacentes, concluindo a remoção
        left->next = right;
        right->prev = left;
    }

    elem->prev = NULL;
    elem->next = NULL;

    return 0;
}
