#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

#include "rtpi.h"

static pthread_barrier_t b;
static DEFINE_PI_MUTEX(m, 0);
static DEFINE_PI_COND(c, &m, 0);

static void cl(void *arg)
{
	pi_mutex_unlock(&m);
}

static void *tf(void *arg)
{
	if (pi_mutex_lock(&m) != 0) {
		printf("%s: mutex_lock failed\n", __func__);
		exit(1);
	}
	int e = pthread_barrier_wait(&b);
	if (e != 0 && e != PTHREAD_BARRIER_SERIAL_THREAD) {
		printf("%s: barrier_wait failed\n", __func__);
		exit(1);
	}
	pthread_cleanup_push(cl, NULL);
	/* We have to loop here because the cancellation might come after
	   the cond_wait call left the cancelable area and is then waiting
	   on the mutex.  In this case the beginning of the second cond_wait
	   call will cause the cancellation to happen.  */
	do
		if (pi_cond_wait(&c) != 0) {
			printf("%s: cond_wait failed\n", __func__);
			exit(1);
		}
	while (arg == NULL) ;
	pthread_cleanup_pop(0);
	if (pi_mutex_unlock(&m) != 0) {
		printf("%s: mutex_unlock failed\n", __func__);
		exit(1);
	}
	return NULL;
}

static int do_test(void)
{
	int status = 0;

	if (pthread_barrier_init(&b, NULL, 2) != 0) {
		puts("barrier_init failed");
		return 1;
	}

	pthread_t th;
	if (pthread_create(&th, NULL, tf, NULL) != 0) {
		puts("1st create failed");
		return 1;
	}
	int e = pthread_barrier_wait(&b);
	if (e != 0 && e != PTHREAD_BARRIER_SERIAL_THREAD) {
		puts("1st barrier_wait failed");
		return 1;
	}
	if (pi_mutex_lock(&m) != 0) {
		puts("1st mutex_lock failed");
		return 1;
	}
	if (pi_cond_signal(&c) != 0) {
		puts("1st cond_signal failed");
		return 1;
	}
	if (pthread_cancel(th) != 0) {
		puts("cancel failed");
		return 1;
	}
	if (pi_mutex_unlock(&m) != 0) {
		puts("1st mutex_unlock failed");
		return 1;
	}
	void *res;
	if (pthread_join(th, &res) != 0) {
		puts("1st join failed");
		return 1;
	}
	if (res != PTHREAD_CANCELED) {
		puts("first thread not canceled");
		status = 1;
	}

	if (pthread_create(&th, NULL, tf, (void *)1l) != 0) {
		puts("2nd create failed");
		return 1;
	}
	e = pthread_barrier_wait(&b);
	if (e != 0 && e != PTHREAD_BARRIER_SERIAL_THREAD) {
		puts("2nd barrier_wait failed");
		return 1;
	}
	if (pi_mutex_lock(&m) != 0) {
		puts("2nd mutex_lock failed");
		return 1;
	}
	if (pi_cond_signal(&c) != 0) {
		puts("2nd cond_signal failed");
		return 1;
	}
	if (pi_mutex_unlock(&m) != 0) {
		puts("2nd mutex_unlock failed");
		return 1;
	}
	if (pthread_join(th, &res) != 0) {
		puts("2nd join failed");
		return 1;
	}
	if (res != NULL) {
		puts("2nd thread canceled");
		status = 1;
	}

	return status;
}

#include "test-driver.c"
