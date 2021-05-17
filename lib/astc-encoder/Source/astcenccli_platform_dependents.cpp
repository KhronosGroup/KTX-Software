// SPDX-License-Identifier: Apache-2.0
// ----------------------------------------------------------------------------
// Copyright 2011-2021 Arm Limited
//
// Licensed under the Apache License, Version 2.0 (the "License"); you may not
// use this file except in compliance with the License. You may obtain a copy
// of the License at:
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
// WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
// License for the specific language governing permissions and limitations
// under the License.
// ----------------------------------------------------------------------------

/**
 * @brief Platform-specific function implementations.
 *
 * This module contains functions with strongly OS-dependent implementations:
 *
 *  * CPU count queries
 *  * Threading
 *  * Time
 *
 * In addition to the basic thread abstraction (which is native pthreads on
 * all platforms, except Windows where it is an emulation of pthreads), a
 * utility function to create N threads and wait for them to complete a batch
 * task has also been provided.
 */

#include "astcenccli_internal.h"

/* ============================================================================
   Platform code for Windows using the Win32 APIs.
============================================================================ */
#if defined(_WIN32) && !defined(__CYGWIN__)

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

typedef HANDLE pthread_t;
typedef int pthread_attr_t;

/* Public function, see header file for detailed documentation */
static int pthread_create(
	pthread_t* thread,
	const pthread_attr_t* attribs,
	void* (*threadfunc)(void*),
	void* thread_arg
) {
	(void)attribs;
	LPTHREAD_START_ROUTINE func = (LPTHREAD_START_ROUTINE)threadfunc;
	*thread = CreateThread(nullptr, 0, func, thread_arg, 0, nullptr);
	return 0;
}

/* Public function, see header file for detailed documentation */
static int pthread_join(
	pthread_t thread,
	void** value
) {
	(void)value;
	WaitForSingleObject(thread, INFINITE);
	return 0;
}

/* Public function, see header file for detailed documentation */
int get_cpu_count()
{
	SYSTEM_INFO sysinfo;
	GetSystemInfo(&sysinfo);
	return sysinfo.dwNumberOfProcessors;
}

/* Public function, see header file for detailed documentation */
double get_time()
{
	FILETIME tv;
	GetSystemTimePreciseAsFileTime(&tv);
	unsigned long long ticks = tv.dwHighDateTime;
	ticks = (ticks << 32) | tv.dwLowDateTime;
	return ((double)ticks) / 1.0e7;
}

/* ============================================================================
   Platform code for an platform using POSIX APIs.
============================================================================ */
#else

#include <pthread.h>
#include <sys/time.h>
#include <unistd.h>

/* Public function, see header file for detailed documentation */
int get_cpu_count()
{
	return sysconf(_SC_NPROCESSORS_ONLN);
}

/* Public function, see header file for detailed documentation */
double get_time()
{
	timeval tv;
	gettimeofday(&tv, 0);
	return (double)tv.tv_sec + (double)tv.tv_usec * 1.0e-6;
}

#endif

/**
 * @brief Worker thread helper payload for launch_threads.
 */
struct launch_desc
{
	/** The native thread handle. */
	pthread_t thread_handle;
	/** The total number of threads in the thread pool. */
	int thread_count;
	/** The thread index in the thread pool. */
	int thread_id;
	/** The user thread function to execute. */
	void (*func)(int, int, void*);
	/** The user thread payload. */
	void* payload;
};

/**
 * @brief Helper function to translate thread entry points.
 *
 * Convert a (void*) thread entry to an (int, void*) thread entry, where the
 * integer contains the thread ID in the thread pool.
 *
 * @param p The thread launch helper payload.
 */
static void* launch_threads_helper(void *p)
{
	launch_desc* ltd = (launch_desc*)p;
	ltd->func(ltd->thread_count, ltd->thread_id, ltd->payload);
	return nullptr;
}

/* Public function, see header file for detailed documentation */
void launch_threads(
	int thread_count,
	void (*func)(int, int, void*),
	void *payload
) {
	// Directly execute single threaded workloads on this thread
	if (thread_count <= 1)
	{
		func(1, 0, payload);
		return;
	}

	// Otherwise spawn worker threads
	launch_desc *thread_descs = new launch_desc[thread_count];
	for (int i = 0; i < thread_count; i++)
	{
		thread_descs[i].thread_count = thread_count;
		thread_descs[i].thread_id = i;
		thread_descs[i].payload = payload;
		thread_descs[i].func = func;

		pthread_create(&(thread_descs[i].thread_handle), nullptr,
		               launch_threads_helper, (void*)&(thread_descs[i]));
	}

	// ... and then wait for them to complete
	for (int i = 0; i < thread_count; i++)
	{
		pthread_join(thread_descs[i].thread_handle, nullptr);
	}

	delete[] thread_descs;
}
