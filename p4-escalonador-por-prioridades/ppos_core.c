#include <stdio.h>
#include <stdlib.h>

#include "ppos.h"

static task_t main_task;       // descritor da tarefa main
static task_t dispatcher_task; // descritor da tarefa dispatcher
static task_t *current_task;   // ponteiro para a tarefa corrente
static task_t *ready_queue;    // ponteiro para a fila de prontas
static int user_tasks = 0;     // contador de tarefas do usuário
static int next_id = 0;        // id da próxima tarefa

// print_elem é passada para a função queue_print, utilizada em mensagens de
// depuração, para acompanhar o uso da fila de tarefas prontas.
__attribute__((unused)) static void print_elem(void *ptr) {
    printf("%d", ((task_t *)ptr)->id);
}

static task_t *scheduler(void) {
    task_t *next_task = ready_queue;

    // percorre a fila de prontas em busca da tarefa com maior prioridade
    task_t *aux = ready_queue->next;

    while (aux != ready_queue) {

        // a próxima tarefa será aquela com a maior prioridade dinâmica;
        // em caso de empate, a próxima tarefa será aquela com maior
        // prioridade estática
        if ((aux->dynamic_prio < next_task->dynamic_prio) ||
            (aux->dynamic_prio == next_task->dynamic_prio &&
             aux->static_prio < next_task->static_prio)) {

            next_task = aux;
        }

        aux = aux->next;
    }

    aux = next_task->next;

    // envelhecimento das tarefas não escolhidas
    while (aux != next_task) {
        if (aux->dynamic_prio > MAX_PRIORITY) {
            aux->dynamic_prio += AGING_FACTOR;
        }

        aux = aux->next;
    }

    // reseta a prioridade dinâmica da nova tarefa a ser executada
    next_task->dynamic_prio = next_task->static_prio;

    return next_task;
}

static void dispatcher(void) {
#if DEBUG
    printf("%-18s: tarefa dispatcher lançada\n", "### (dispatcher)");
#endif

    // continua a execução enquanto houver tarefas não finalizadas
    while (user_tasks > 0) {
#ifdef DEBUG
        queue_print("### [ready_queue] ", (queue_t *)ready_queue, print_elem);
#endif

        // escolhe a próxima tarefa para ser executada
        task_t *task = scheduler();

        if (task != NULL) {
            queue_remove((queue_t **)&ready_queue, (queue_t *)task);
            task->status = RUNNING;
            task_switch(task); // transfere o controle para a nova tarefa

            switch (task->status) {
            case RUNNING:
                task->status = READY;
                queue_append((queue_t **)&ready_queue, (queue_t *)task);
                break;
            case FINISHED:
                free(task->context.uc_stack.ss_sp);
                break;
            default:
                break;
            }
        }
    }

#ifdef DEBUG
    queue_print("### [ready_queue] ", (queue_t *)ready_queue, print_elem);
#endif

    task_exit(0); // encerra o dispatcher
}

void ppos_init() {
    // desativa o buffer da saída padrão (stdout)
    setvbuf(stdout, NULL, _IONBF, 0);

    main_task.id = next_id++;  // tarefa main (task 0)
    current_task = &main_task; // tarefa main é a corrente

    // cria a tarefa dispatcher
    task_create(&dispatcher_task, (void *)dispatcher, NULL);

#ifdef DEBUG
    printf("%-18s: sistema inicializado\n", "### (ppos_init)");
#endif
}

int task_create(task_t *task, void (*start_func)(void *), void *arg) {
    // aloca memória para a pilha utilizada pelo contexto
    char *stack = malloc(STACKSIZE);

    if (stack == NULL) {
        perror("Erro ao criar a pilha");
        return -1;
    }

    if (getcontext(&(task->context)) == -1) {
        perror("Erro ao armazenar o contexto atual");
        return -1;
    }

    // inicializa as propriedades da nova tarefa
    task->prev = NULL;
    task->next = NULL;
    task->id = next_id++;
    task->status = NEW;
    task->static_prio = 0;
    task->dynamic_prio = 0;

    task->context.uc_stack.ss_sp = stack;
    task->context.uc_stack.ss_size = STACKSIZE;
    task->context.uc_stack.ss_flags = 0;
    task->context.uc_link = NULL;

    // insere as tarefas do usuário (id > 1) na fila de prontas
    if (task->id > 1) {
        user_tasks++;
        task->status = READY;
        queue_append((queue_t **)&ready_queue, (queue_t *)task);
    }

    makecontext(&(task->context), (void *)start_func, 1, (char *)arg);

#ifdef DEBUG
    printf("%-18s: tarefa %d criada pela tarefa %d (função: %p)\n", "### (task_create)",
           task->id, current_task->id, start_func);
#endif

    return task->id;
}

int task_switch(task_t *task) {
    task_t *t = current_task;
    current_task = task;

#ifdef DEBUG
    printf("%-18s: tarefa %d -> tarefa %d\n", "### (task_switch)", t->id, task->id);
#endif

    if (swapcontext(&(t->context), &(task->context)) == -1) {
        perror("Erro ao trocar de contexto");
        return -1;
    }

    return 0;
}

void task_exit(int exit_code) {
#ifdef DEBUG
    printf("%-18s: tarefa %d finalizada\n", "### (task_exit)", current_task->id);
#endif

    current_task->status = FINISHED;

    if (current_task == &dispatcher_task) {
        task_switch(&main_task);
    } else {
        user_tasks--;
        task_switch(&dispatcher_task);
    }
}

int task_id() {
    return current_task->id;
}

void task_yield(void) {
#ifdef DEBUG
    printf("%-18s: tarefa %d liberou a CPU\n", "### (task_yield)", current_task->id);
#endif

    task_switch(&dispatcher_task);
}

void task_setprio(task_t *task, int prio) {
    // verifica os limites da prioridade fornecida
    if (prio < MAX_PRIORITY) {
        prio = MAX_PRIORITY;
    } else if (prio > MIN_PRIORITY) {
        prio = MIN_PRIORITY;
    }

    if (task == NULL) {
        current_task->static_prio = prio;
        current_task->dynamic_prio = prio;
    } else {
        task->static_prio = prio;
        task->dynamic_prio = prio;
    }
}

int task_getprio(task_t *task) {
    if (task == NULL) {
        return current_task->static_prio;
    }

    return task->static_prio;
}
