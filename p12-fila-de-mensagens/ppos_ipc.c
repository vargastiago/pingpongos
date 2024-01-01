#include <stdlib.h>
#include <string.h>

#include "ppos.h"

extern task_t *current_task;
extern task_t *ready_queue;
extern task_t dispatcher_task;

static int lock = 0;

static void enter_cs(int *lock) {
    while (__sync_fetch_and_or(lock, 1))
        ;
}

static void leave_cs(int *lock) {
    (*lock) = 0;
}

int sem_create(semaphore_t *s, int value) {
    if (s == NULL || s->active) {
        return -1;
    }

    // inicializa os campos do semáforo
    s->active = 1;
    s->counter = value;
    s->task_queue = NULL;

    return 0;
}

int sem_down(semaphore_t *s) {
    if (s == NULL || s->active == 0) {
        return -1;
    }

    enter_cs(&lock);
    s->counter--;
    leave_cs(&lock);

    // contador negativo: chamada bloqueante
    if (s->counter < 0) {
        current_task->status = SUSPENDED;

        enter_cs(&lock);
        queue_append((queue_t **)&(s->task_queue), (queue_t *)current_task);
        leave_cs(&lock);

        task_switch(&dispatcher_task);
    }

    // caso semáforo tenha sido destruído
    if (s->active == 0) {
        return -1;
    }

    return 0;
}

int sem_up(semaphore_t *s) {
    if (s == NULL || s->active == 0) {
        return -1;
    }

    enter_cs(&lock);
    s->counter++;
    leave_cs(&lock);

    // caso haja tarefas aguardando na fila do semáforo
    if (s->counter <= 0) {
        enter_cs(&lock);
        task_t *task = (task_t *)s->task_queue;
        queue_remove((queue_t **)&(s->task_queue), (queue_t *)(s->task_queue));
        leave_cs(&lock);

        // acorda a primeira tarefa da fila e retorna à fila de prontas
        task->status = READY;
        queue_append((queue_t **)&ready_queue, (queue_t *)task);
    }

    return 0;
}

int sem_destroy(semaphore_t *s) {
    if (s == NULL || s->active == 0) {
        return -1;
    }

    while (s->task_queue != NULL) {
        sem_up(s);
    }

    enter_cs(&lock);
    s->active = 0;
    leave_cs(&lock);

    return 0;
}

int mqueue_create(mqueue_t *queue, int max, int size) {
    if (queue == NULL || queue->active) {
        return -1;
    }

    if ((queue->buffer = malloc(max * size)) == NULL) {
        return -1;
    }

    // inicializa os campos da fila de mensagens
    queue->buf_start = 0;
    queue->buf_end = 0;
    queue->capacity = max;
    queue->length = 0;
    queue->item_size = size;
    queue->active = 1;

    sem_create(&(queue->s_buffer), 1);
    sem_create(&(queue->s_space), max);
    sem_create(&(queue->s_item), 0);

    return 0;
}

int mqueue_send(mqueue_t *queue, void *msg) {
    if (queue == NULL || queue->active == 0) {
        return -1;
    }

    int size = queue->item_size;

    sem_down(&queue->s_space);
    sem_down(&queue->s_buffer);

    // copia a mensagem para o fim da fila
    void *dest = queue->buffer + queue->buf_end * size;
    memcpy(dest, msg, size);

    // atualiza o tamanho e o final da fila
    queue->buf_end = (queue->buf_end + 1) % queue->capacity;
    queue->length++;

    sem_up(&queue->s_buffer);
    sem_up(&queue->s_item);

    return 0;
}

int mqueue_recv(mqueue_t *queue, void *msg) {
    if (queue == NULL || queue->active == 0) {
        return -1;
    }

    int size = queue->item_size;

    sem_down(&queue->s_item);
    sem_down(&queue->s_buffer);

    // recebe a mensagem do início da fila e a deposita no buffer msg
    void *src = queue->buffer + size * queue->buf_start;
    memcpy(msg, src, size);

    // atualiza o tamanho e o início da fila
    queue->buf_start = (queue->buf_start + 1) % queue->capacity;
    queue->length--;

    sem_up(&queue->s_buffer);
    sem_up(&queue->s_space);

    return 0;
}

int mqueue_destroy(mqueue_t *queue) {
    if (queue == NULL || queue->active == 0) {
        return -1;
    }

    queue->active = 0;

    sem_destroy(&queue->s_buffer);
    sem_destroy(&queue->s_space);
    sem_destroy(&queue->s_item);

    free(queue->buffer);

    return 0;
}

int mqueue_msgs(mqueue_t *queue) {
    if (queue == NULL || queue->active == 0) {
        return -1;
    }

    return queue->length;
}
