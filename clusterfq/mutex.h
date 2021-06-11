#pragma once

#ifdef _WIN32
#include <Windows.h>
#else
#include <pthread.h>
#endif

struct mutex {
#ifdef _WIN32
	HANDLE mutex;
#else
	pthread_mutex_t mutex;
#endif
};

void mutex_init(struct mutex* m);
void mutex_wait_for(struct mutex* m);
void mutex_release(struct mutex* m);