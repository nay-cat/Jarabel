#include "queue.h"

BOOL Queue_Init(WorkQueue* queue) {
    queue->capacity = 65536;
    queue->paths = (WCHAR**)calloc(queue->capacity, sizeof(WCHAR*));
    if (!queue->paths) return FALSE;
    for (LONG i = 0; i < queue->capacity; ++i) {
        queue->paths[i] = (WCHAR*)malloc(sizeof(WCHAR) * MAX_PATH);
        if (!queue->paths[i]) {
            for (LONG j = 0; j < i; j++) free(queue->paths[j]);
            free(queue->paths);
            queue->paths = NULL;
            return FALSE;
        }
    }
    queue->head = 0;
    queue->tail = 0;
    queue->finished_seeding = FALSE;
    return TRUE;
}

void Queue_Destroy(WorkQueue* queue) {
    if (queue->paths) {
        for (LONG i = 0; i < queue->capacity; ++i) {
            if (queue->paths[i]) {
                free(queue->paths[i]);
            }
        }
        free(queue->paths);
    }
}

void Queue_Push(WorkQueue* queue, const WCHAR* path) {
    LONG current_tail;
    while (TRUE) {
        current_tail = queue->tail;
        // This is an approximate check. If the queue is full, we spin
        if (current_tail - queue->head >= queue->capacity) {
            Sleep(1);
            continue;
        }
        // Attempt to atomically increment the tail. The original value is returned
        if (InterlockedCompareExchange(&queue->tail, current_tail + 1, current_tail) == current_tail) {
            // so this thread now owns the slot at current_tail
            wcscpy_s(queue->paths[current_tail % queue->capacity], MAX_PATH, path);
            return;
        }
        // If the CAS failed, another thread pushed. Loop and retry
    }
}

BOOL Queue_Pop(WorkQueue* queue, WCHAR* outPath) {
    const int spin_retries = 2048;
    LONG current_head;

    while (TRUE) {
        current_head = queue->head;
        // If head and tail are the same, the queue is likely empty
        if (current_head >= queue->tail) {
            // Check if producers are done. If not, the queue might get more items
            if (!queue->finished_seeding) {
                // Spin for a short time, then yield
                for (int i = 0; i < spin_retries; ++i) {
                    _mm_pause(); // Hint to CPU we are in a spin-wait loop
                    if (queue->head < queue->tail) {
                        goto retry_pop; // An item was added, try to pop it
                    }
                }
                Sleep(0); // Yield thread slice
                continue; // Re-evaluate the loop condition
            }
            else {
                // Seeding is finished. We must re-check if tail has advanced one last time
                // to prevent a race condition where we decide to exit just as the last item is added
                if (current_head >= queue->tail) {
                    return FALSE; // Queue is empty and no more items will ever be added
                }
            }
        }

    retry_pop:
        // Attempt to atomically increment the head
        if (InterlockedCompareExchange(&queue->head, current_head + 1, current_head) == current_head) {
            wcscpy_s(outPath, MAX_PATH, queue->paths[current_head % queue->capacity]);
            return TRUE;
        }
        // If CAS failed, another thread popped. Loop and try again like in Queue_Push
    }
}