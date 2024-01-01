#include <stddef.h>

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

    if (s->counter < 0) {
        current_task->status = SUSPENDED;

        enter_cs(&lock);
        queue_append((queue_t **)&(s->task_queue), (queue_t *)current_task);
        leave_cs(&lock);

        task_switch(&dispatcher_task);
    }

    // semáforo destruído
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

    if (s->counter <= 0) {
        enter_cs(&lock);
        task_t *task = (task_t *)s->task_queue;
        queue_remove((queue_t **)&(s->task_queue), (queue_t *)(s->task_queue));
        leave_cs(&lock);

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
