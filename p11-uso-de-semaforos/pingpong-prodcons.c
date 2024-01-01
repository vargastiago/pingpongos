#include <stdio.h>
#include <stdlib.h>

#include "ppos.h"

#define NUM_PROD 3
#define NUM_CONS 2
#define VAGAS 5

typedef struct object_t {
    struct object_t *prev;
    struct object_t *next;
    int item;
} object_t;

object_t *buffer;

task_t prod[NUM_PROD];
task_t cons[NUM_CONS];

semaphore_t s_buffer;
semaphore_t s_item, s_vaga;

void produtor(void *p) {
    while (1) {
        task_sleep(1000);

        object_t *obj = (object_t *)malloc(sizeof(object_t));
        obj->item = rand() % 20;
        obj->prev = NULL;
        obj->next = NULL;

        sem_down(&s_vaga);

        sem_down(&s_buffer);
        queue_append((queue_t **)&buffer, (queue_t *)obj);

        printf("P%ld produziu %d (tem %d)\n", (long)p + 1, obj->item, queue_size((queue_t *)buffer));
        sem_up(&s_buffer);

        sem_up(&s_item);
    }
}

void consumidor(void *c) {
    while (1) {
        sem_down(&s_item);

        sem_down(&s_buffer);
        object_t *obj = buffer;
        queue_remove((queue_t **)&buffer, (queue_t *)buffer);

        free(obj);
        printf("\t\t\t\tC%ld consumiu %d (tem %d)\n", (long)c + 1, obj->item, queue_size((queue_t *)buffer));
        sem_up(&s_buffer);

        sem_up(&s_vaga);

        task_sleep(1000);
    }
}

int main(void) {
    ppos_init();

    // inicializa os semáforos
    sem_create(&s_buffer, 1);
    sem_create(&s_vaga, VAGAS);
    sem_create(&s_item, 0);

    // cria os produtores
    for (long i = 0; i < NUM_PROD; i++) {
        if (task_create(&prod[i], produtor, (void *)i) == -1) {
            perror("task_create() failed");
            exit(1);
        }
    }

    // cria os consumidores
    for (long i = 0; i < NUM_CONS; i++) {
        if (task_create(&cons[i], consumidor, (void *)i) == -1) {
            perror("task_create() failed");
            exit(1);
        }
    }

    task_join(&prod[0]);
    task_exit(0);
}