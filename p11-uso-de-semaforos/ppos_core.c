#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>

#include "ppos.h"

task_t dispatcher_task; // descritor da tarefa dispatcher
task_t *current_task;   // ponteiro para a tarefa corrente
task_t *ready_queue;    // ponteiro para a fila de tarefas prontas

static task_t main_task;        // descritor da tarefa main
static task_t *sleep_queue;     // ponteiro para a fila de tarefas adormecidas
static int user_tasks = 0;      // contador de tarefas do usuário
static int next_id = 0;         // id da próxima tarefa
static unsigned int clock;      // relógio do sistema
static struct sigaction action; // tratador de sinal
static struct itimerval timer;  // inicialização do timer

static void tick_handler(void) {
    clock++; // incrementa o relógio do sistema

    // calcula o tempo parcial de processamento da tarefa corrente
    current_task->proc_time += clock - current_task->proc_marker;
    current_task->proc_marker = clock;

    if (!current_task->is_sys_task) {
        current_task->quantum--;

        if (current_task->quantum == 0) {
            task_switch(&dispatcher_task);
        }
    }
}

static task_t *scheduler(void) {
    if (ready_queue == NULL) {
        return NULL;
    }

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

    // envelhecimento das tarefas não escolhidas
    aux = next_task->next;

    while (aux != next_task) {
        if (aux->dynamic_prio > MAX_PRIORITY) {
            aux->dynamic_prio -= AGING_FACTOR;
        }

        aux = aux->next;
    }

    // reseta a prioridade dinâmica da nova tarefa a ser executada
    next_task->dynamic_prio = next_task->static_prio;

    return next_task;
}

static void wake_tasks(void) {
    if (sleep_queue == NULL) {
        return;
    }

    task_t *task = sleep_queue;

    do {
        task_t *next = task->next;

        if (clock >= task->wakeup_time) {
            queue_remove((queue_t **)&sleep_queue, (queue_t *)task);
            task->status = READY;
            queue_append((queue_t **)&ready_queue, (queue_t *)task);
        }

        task = next;
    } while (task != sleep_queue && sleep_queue != NULL);
}

static void dispatcher(void) {

    // continua a execução enquanto houver tarefas não finalizadas
    while (user_tasks > 0) {
        wake_tasks(); // acorda as tarefas adormecidas

        // escolhe a próxima tarefa a ser executada
        task_t *task = scheduler();

        if (task != NULL) {
            task->quantum = TICKS; // tarefa recebe um quantum de ticks

            queue_remove((queue_t **)&ready_queue, (queue_t *)task);
            task->status = RUNNING;
            task_switch(task); // transfere o controle para a nova tarefa

            switch (task->status) {
            case RUNNING:
                task->status = READY;
                queue_append((queue_t **)&ready_queue, (queue_t *)task);
                break;
            case FINISHED:
                // acorda as tarefas suspensas
                while (task->suspend_queue != NULL) {
                    task_t *t = task->suspend_queue;

                    queue_remove((queue_t **)&(task->suspend_queue), (queue_t *)t);
                    task->status = READY;
                    queue_append((queue_t **)&ready_queue, (queue_t *)t);
                }

                free(task->context.uc_stack.ss_sp);
                break;
            default:
                break;
            }
        }
    }

    task_exit(0); // encerra o dispatcher
}

void ppos_init() {
    // desativa o buffer da saída padrão (stdout)
    setvbuf(stdout, NULL, _IONBF, 0);

    // registra a ação para o sinal de timer SIGALRM
    action.sa_handler = (void *)tick_handler;
    sigemptyset(&action.sa_mask);
    action.sa_flags = 0;

    if (sigaction(SIGALRM, &action, NULL) < 0) {
        perror("Erro ao configurar ação");
        exit(1);
    }

    // programa e arma um temporizador para disparar a cada 1 milissegundo
    timer.it_value.tv_usec = 1000;
    timer.it_value.tv_sec = 0;
    timer.it_interval.tv_usec = 1000;
    timer.it_interval.tv_sec = 0;

    if (setitimer(ITIMER_REAL, &timer, NULL) < 0) {
        perror("Erro ao armar o temporizador");
        exit(1);
    }

    task_create(&main_task, NULL, NULL); // tarefa main (task 0)
    current_task = &main_task;           // tarefa main é a corrente

    // cria a tarefa dispatcher
    task_create(&dispatcher_task, (void *)dispatcher, NULL);
    task_switch(&dispatcher_task);
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

    // inicializa as propriedades da nova tarefa
    task->prev = NULL;
    task->next = NULL;
    task->id = next_id++;
    task->status = NEW;
    task->static_prio = 0;
    task->dynamic_prio = 0;
    task->activations = 0;
    task->exec_start = clock;

    // se dispatcher (id = 1) a tarefa é do sistema; senão tarefa do usuário
    task->is_sys_task = task->id == 1 ? 1 : 0;

    task->context.uc_stack.ss_sp = stack;
    task->context.uc_stack.ss_size = STACKSIZE;
    task->context.uc_stack.ss_flags = 0;
    task->context.uc_link = NULL;

    // insere as tarefas do usuário na fila de prontas
    if (!task->is_sys_task) {
        user_tasks++;
        task->status = READY;
        queue_append((queue_t **)&ready_queue, (queue_t *)task);
    }

    makecontext(&(task->context), (void *)start_func, 1, (char *)arg);

    return task->id;
}

int task_switch(task_t *task) {
    task_t *t = current_task;
    current_task = task;

    task->activations++;
    task->proc_marker = clock;

    if (swapcontext(&(t->context), &(task->context)) == -1) {
        perror("Erro ao trocar de contexto");
        return -1;
    }

    return 0;
}

void task_exit(int exit_code) {
    current_task->status = FINISHED;
    current_task->exit_code = exit_code;
    current_task->exec_end = clock;

    unsigned int exec_time = current_task->exec_end - current_task->exec_start;

    printf("Task %d exit: execution time %4u ms, processor time %4u ms, %d activations\n",
           current_task->id, exec_time, current_task->proc_time, current_task->activations);

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

void task_yield() {
    task_switch(&dispatcher_task);
}

void task_setprio(task_t *task, int prio) {
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

int task_join(task_t *task) {
    if (task->status == FINISHED || task == NULL) {
        return -1;
    }

    current_task->status = SUSPENDED;
    queue_append((queue_t **)&(task->suspend_queue), (queue_t *)current_task);
    task_switch(&dispatcher_task);

    return task->exit_code;
}

void task_sleep(int t) {
    current_task->wakeup_time = clock + t;
    current_task->status = SLEEPING;
    queue_append((queue_t **)&sleep_queue, (queue_t *)current_task);
    task_switch(&dispatcher_task);
}

unsigned int systime() {
    return clock;
}
