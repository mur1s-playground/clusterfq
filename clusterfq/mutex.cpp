#include "mutex.h"

void mutex_init(struct mutex *m) {
#ifdef _WIN32
	m->mutex = CreateMutex(NULL, FALSE, NULL);
#else
	pthread_mutex_init(&m->mutex, NULL);
#endif
}

void mutex_wait_for(struct mutex *m) {
#ifdef _WIN32
	WaitForSingleObject(m->mutex, INFINITE);
#else
	pthread_mutex_lock(&m->mutex);
#endif
}

void mutex_release(struct mutex *m) {
#ifdef _WIN32
	ReleaseMutex(m->mutex);
#else
	pthread_mutex_unlock(&m->mutex);
#endif
}