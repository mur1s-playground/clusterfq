#pragma once

#ifdef _WIN32
#include <winsock2.h>
#else
#include <pthread.h>
#endif

enum ThreadState {
	TS_CLAIMED,
	TS_RUNNING,
	TS_FINISHED,
	TS_ERROR,
	TS_NULL
};

#ifdef _WIN32
struct ThreadParamsWin {
	void* func;
	void* arg;
};
#endif

struct Thread {
#ifdef _WIN32
	HANDLE thread;
#else
	pthread_t thread;
#endif
	enum ThreadState state;
};

struct ThreadPool {
	struct Thread* threads;
	int size;
};

void thread_pool_init(struct ThreadPool* thread_pool, int size);
void thread_pool_destroy(struct ThreadPool* thread_pool);

int thread_create(struct ThreadPool* thread_pool, void* func, void* arg);
void thread_terminated(struct ThreadPool* thread_pool, int id);

#ifdef _WIN32
DWORD WINAPI thread_func(LPVOID lpParam);
#endif