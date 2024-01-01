#include <stdio.h>
#include <stdlib.h>

#include "ppos.h"

static task_t main_task;     // descritor da tarefa main
static task_t *current_task; // ponteiro para a tarefa corrente
static int tasks = 0;        // contador de tarefas

void ppos_init() {
    // desativa o buffer da saída padrão (stdout)
    setvbuf(stdout, NULL, _IONBF, 0);

    main_task.id = 0; // tarefa main (task 0)
    main_task.prev = NULL;
    main_task.next = NULL;

    current_task = &main_task; // tarefa main é a corrente

#ifdef DEBUG
    printf("%s\n", "### (ppos_init): sistema inicializado");
#endif
}

int task_create(task_t *task, void (*start_func)(void *), void *arg) {
    // aloca memória para a pilha de sinais (signal stack)
    char *stack = malloc(STACKSIZE);

    if (stack == NULL) {
        perror("Erro ao criar a pilha de sinais");
        return -1;
    }

    if (getcontext(&(task->context)) == -1) {
        perror("Erro ao armazenar o contexto atual");
        return -1;
    }

    // configura os campos da nova tarefa
    task->id = ++tasks;
    task->prev = NULL;
    task->next = NULL;

    // configura campos do contexto da nova tarefa
    task->context.uc_stack.ss_sp = stack;
    task->context.uc_stack.ss_size = STACKSIZE;
    task->context.uc_stack.ss_flags = 0;
    task->context.uc_link = NULL;

    makecontext(&(task->context), (void *)start_func, 1, (char *)arg);

#ifdef DEBUG
    printf("### (task_create): tarefa %d criada pela tarefa %d (função: %p)\n",
           task->id, current_task->id, start_func);
#endif

    return task->id;
}

int task_switch(task_t *task) {
    task_t *t = current_task;
    current_task = task;

#ifdef DEBUG
    printf("### (task_switch): tarefa %d -> tarefa %d\n", t->id, task->id);
#endif

    if (swapcontext(&(t->context), &(task->context)) == -1) {
        perror("Erro ao trocar de contexto");
        return -1;
    }

    return 0;
}

void task_exit(int exit_code) {
#ifdef DEBUG
    printf("### (task_exit): tarefa %d finalizada\n", current_task->id);
#endif

    task_switch(&main_task);
}

int task_id() {
    return current_task->id;
}
