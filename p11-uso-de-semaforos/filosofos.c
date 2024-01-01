#include <stdio.h>
#include <stdlib.h>

#include "ppos.h"

#define NUMFILO 5

task_t filosofo[NUMFILO];
semaphore_t hashi[NUMFILO];

char *space[] = {"", "\t", "\t\t", "\t\t\t", "\t\t\t\t"};

// filósofo comendo
void come(int f) {
    printf("%sF%d COMENDO\n", space[f], f);
    // task_sleep(1000);
}

// filósofo meditando
void medita(int f) {
    printf("%sF%d meditando\n", space[f], f);
    // task_sleep(1000);
}

// pega o hashi
void pega_hashi(int f, int h) {
    printf("%sF%d quer h%d\n", space[f], f, h);
    sem_down(&hashi[h]);
    printf("%sF%d pegou h%d\n", space[f], f, h);
}

// larga o hashi
void larga_hashi(int f, int h) {
    printf("%sF%d larga h%d\n", space[f], f, h);
    sem_up(&hashi[h]);
}

// corpo da thread filosofo
void tarefa_filosofo(void *arg) {
    int i = (long)arg;

    while (1) {
        medita(i);
        pega_hashi(i, i);
        pega_hashi(i, (i + 1) % NUMFILO);
        come(i);
        larga_hashi(i, i);
        larga_hashi(i, (i + 1) % NUMFILO);
    }

    task_exit(0);
}

int main(void) {
    ppos_init();

    // inicia os hashis
    for (long i = 0; i < NUMFILO; i++) {
        if (sem_create(&hashi[i], 1) == -1) {
            fprintf(stderr, "%s\n", "sem_create() failed");
            exit(1);
        }
    }

    // inicia os filósofos
    for (long i = 0; i < NUMFILO; i++) {
        if (task_create(&filosofo[i], tarefa_filosofo, (void *)i) == -1) {
            fprintf(stderr, "%s\n", "task_create() failed");
            exit(1);
        }
    }

    // a main encerra aqui
    task_exit(0);
}