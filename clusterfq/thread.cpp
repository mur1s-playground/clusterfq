#include "thread.h"

#include <stdlib.h>
#include <stdio.h>

void thread_pool_init(struct ThreadPool* thread_pool, int size) {
	int i;
	thread_pool->threads = (struct Thread*)malloc(sizeof(struct Thread) * size);
	if (thread_pool->threads == NULL) {
		return;
	}
	for (i = 0; i < size; i++) {
		thread_pool->threads[i].state = TS_NULL;
	}
	thread_pool->size = size;
}

void thread_pool_destroy(struct ThreadPool* thread_pool) {
	if (thread_pool->size > 0) {
		free(thread_pool->threads);
	}
}

int thread_create(struct ThreadPool* thread_pool, void* func, void* arg) {
	int i = 0;
	for (; i < thread_pool->size; i++) {
		if (thread_pool->threads[i].state == TS_NULL) {
			thread_pool->threads[i].state = TS_CLAIMED;
#ifdef _WIN32
			struct ThreadParamsWin* tpw = (struct ThreadParamsWin*)malloc(sizeof(struct ThreadParamsWin));
			tpw->func = func;
			tpw->arg = arg;
			thread_pool->threads[i].thread = CreateThread(NULL, 0, thread_func, tpw, 0, NULL);
			if (thread_pool->threads[i].thread != NULL) {
#else
			if (pthread_create(&(thread_pool->threads[i].thread), NULL, (void *(*)(void*))func, arg) == 0) {
#endif
				thread_pool->threads[i].state = TS_RUNNING;
				break;
			} else {
				thread_pool->threads[i].state = TS_NULL;
			}
		}
	}
	return i;
}

void thread_terminated(struct ThreadPool* thread_pool, int id) {
	thread_pool->threads[id].state = TS_NULL;
}

#ifdef _WIN32
DWORD WINAPI thread_func(LPVOID lpParam) {
	struct ThreadParamsWin* tpw = (struct ThreadParamsWin*)lpParam;
	((void(*)(void *))tpw->func)(tpw->arg);
	return 0;
}
#endif
