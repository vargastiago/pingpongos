// Estruturas de dados internas do sistema operacional

#ifndef __PPOS_DATA__
#define __PPOS_DATA__

#include "queue.h"    // biblioteca de filas genéricas
#include <ucontext.h> // biblioteca POSIX de trocas de contexto

#define STACKSIZE 32768 // tamanho da pilha de threads
#define AGING_FACTOR 1
#define MIN_PRIORITY 20
#define MAX_PRIORITY -20
#define TICKS 20

typedef enum { NEW,
               READY,
               RUNNING,
               SUSPENDED,
               FINISHED } status_t;

// Estrutura que define um Task Control Block (TCB)
typedef struct task_t {
    struct task_t *prev, *next;   // ponteiros para usar em filas
    struct task_t *suspend_queue; // fila de tarefas suspensas
    int id;                       // identificador da tarefa
    ucontext_t context;           // contexto armazenado da tarefa
    status_t status;              // status da tarefa
    int static_prio;              // prioridade estática
    int dynamic_prio;             // prioridade dinâmica
    int is_sys_task;              // flag de tarefa do sistema
    int quantum;                  // total de ticks do relógio
    int activations;              // contador de ativações
    int exit_code;                // código de encerramento da tarefa
    unsigned int exec_start;      // tempo de início de execução da tarefa
    unsigned int exec_end;        // tempo de término de execução da tarefa
    unsigned int proc_marker;     // marcador de tempo parcial de processamento
    unsigned int proc_time;       // tempo total de processamento
    // ... (outros campos serão adicionados mais tarde)
} task_t;

// estrutura que define um semáforo
typedef struct
{
    // preencher quando necessário
} semaphore_t;

// estrutura que define um mutex
typedef struct
{
    // preencher quando necessário
} mutex_t;

// estrutura que define uma barreira
typedef struct
{
    // preencher quando necessário
} barrier_t;

// estrutura que define uma fila de mensagens
typedef struct
{
    // preencher quando necessário
} mqueue_t;

#endif
