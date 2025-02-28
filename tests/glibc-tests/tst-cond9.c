/* Copyright (C) 2003-2019 Free Software Foundation, Inc.
   This file is part of the GNU C Library.
   Contributed by Ulrich Drepper <drepper@redhat.com>, 2003.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with the GNU C Library; if not, see
   <http://www.gnu.org/licenses/>.  */

#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "rtpi.h"

static DEFINE_PI_MUTEX(mut, 0);
static DEFINE_PI_COND(cond, &mut, 0);

static void *tf(void *arg)
{
	int err = pi_cond_wait(&cond);
	if (err == 0) {
		puts("cond_wait did not fail");
		exit(1);
	}

	if (err != EPERM) {
		printf("cond_wait didn't return EPERM but %d\n", err);
		exit(1);
	}

	struct timespec ts;
	err = clock_gettime(CLOCK_MONOTONIC, &ts);
	if (err != 0) {
		puts("child: clock_gettime failed");
		exit(1);
	}
	ts.tv_sec += 1000;

	err = pi_cond_timedwait(&cond, &ts);
	if (err == 0) {
		puts("cond_timedwait did not fail");
		exit(1);
	}

	if (err != EPERM) {
		printf("cond_timedwait didn't return EPERM but %d\n", err);
		exit(1);
	}

	return (void *)1l;
}

static int do_test(void)
{
	struct timespec ts;
	pthread_t th;
	int err;

	printf("&cond = %p\n&mut = %p\n", &cond, &mut);

	err = pi_cond_wait(&cond);
	if (err == 0) {
		puts("cond_wait did not fail");
		exit(1);
	}

	if (err != EPERM) {
		printf("cond_wait didn't return EPERM but %d\n", err);
		exit(1);
	}

	/* Current time.  */
	err = clock_gettime(CLOCK_MONOTONIC, &ts);
	if (err != 0) {
		puts("clock_gettime failed");
		exit(1);
	}
	ts.tv_sec += 1000;

	err = pi_cond_timedwait(&cond, &ts);
	if (err == 0) {
		puts("cond_timedwait did not fail");
		exit(1);
	}

	if (err != EPERM) {
		printf("cond_timedwait didn't return EPERM but %d\n", err);
		exit(1);
	}

	if (pi_mutex_lock(&mut) != 0) {
		puts("parent: mutex_lock failed");
		exit(1);
	}

	puts("creating thread");

	if (pthread_create(&th, NULL, tf, NULL) != 0) {
		puts("create failed");
		exit(1);
	}

	void *r;
	if (pthread_join(th, &r) != 0) {
		puts("join failed");
		exit(1);
	}
	if (r != (void *)1l) {
		puts("thread has wrong return value");
		exit(1);
	}

	puts("done");

	return 0;
}

#include "test-driver.c"
