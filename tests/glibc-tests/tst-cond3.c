/* Copyright (C) 2002-2019 Free Software Foundation, Inc.
   This file is part of the GNU C Library.
   Contributed by Ulrich Drepper <drepper@redhat.com>, 2002.

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

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "rtpi.h"

static int do_test(void);
#include "test-driver.c"

/* Note that this test requires more than the standard.  It is
   required that there are no spurious wakeups if only more readers
   are added.  This is a reasonable demand.  */

static DEFINE_PI_MUTEX(mut, 0);
static DEFINE_PI_COND(cond, &mut, 0);

#define N 10

static void *tf(void *arg)
{
	int i = (long int)arg;
	int err;

	/* Get the mutex.  */
	err = pi_mutex_lock(&mut);
	if (err != 0) {
		printf("child %d mutex_lock failed: %s\n", i, strerror(err));
		exit(1);
	}

	/* This call should never return.  */
	err = pi_cond_wait(&cond);
	if (err != 0) {
		printf("child %d pi_cond_wait failed to wait: %s\n",
		       i, strerror(err));
		exit(1);
	}
	puts("error: pi_cond_wait in tf returned");

	/* We should never get here.  */
	exit(1);

	return NULL;
}

static int do_test(void)
{
	int err;
	int i;

	for (i = 0; i < N; ++i) {
		pthread_t th;

		if (i != 0) {
			/* Release the mutex.  */
			err = pi_mutex_unlock(&mut);
			if (err != 0) {
				printf("mutex_unlock %d failed: %s\n", i,
				       strerror(err));
				return 1;
			}
		}

		err = pthread_create(&th, NULL, tf, (void *)(long int)i);
		if (err != 0) {
			printf("create %d failed: %s\n", i, strerror(err));
			return 1;
		}

		/* Get the mutex.  */
		err = pi_mutex_lock(&mut);
		if (err != 0) {
			printf("mutex_lock %d failed: %s\n", i, strerror(err));
			return 1;
		}
	}

	delayed_exit(1);

	/* This call should never return.  */
	err = pi_cond_wait(&cond);
	if (err != 0) {
		puts("error: pi_cond_wait in do_test failed to wait");
		return 1;
	}

	puts("error: pi_cond_wait in do_test returned");
	return 1;
}
