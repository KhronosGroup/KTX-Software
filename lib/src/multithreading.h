/*
 * Copyright 2026 The Khronos Group Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef MULTITHREADING_H
#define MULTITHREADING_H

#if !defined(_WIN32) || defined(WIN32_HAS_PTHREADS)
    #include <pthread.h>
#else
    // Provide pthreads support on windows
    #define WIN32_LEAN_AND_MEAN
    #include <Windows.h>

typedef HANDLE pthread_t;
typedef int pthread_attr_t;

/* Public function, see header file for detailed documentation */
static int
pthread_create(pthread_t* thread, const pthread_attr_t* attribs, void* (*threadfunc)(void*),
               void* thread_arg) {
    (void)attribs;
    #ifdef __clang__
        #pragma clang diagnostic push
        #pragma clang diagnostic ignored "-Wcast-function-type-mismatch"
    #endif
    LPTHREAD_START_ROUTINE func = (LPTHREAD_START_ROUTINE)(threadfunc);
    #ifdef __clang__
        #pragma clang diagnostic pop
    #endif
    *thread = CreateThread(nullptr, 0, func, thread_arg, 0, nullptr);
    return 0;
}

/* Public function, see header file for detailed documentation */
static int
pthread_join(pthread_t thread, void** value) {
    (void)value;
    WaitForSingleObject(thread, INFINITE);
    return 0;
}
#endif

/**
 * @internal
 * @~English
 * @brief Worker thread helper payload for launchThreads.
 */
struct LaunchDesc {
    /** The native thread handle. */
    pthread_t threadHandle;
    /** The total number of threads in the thread pool. */
    int threadCount;
    /** The thread index in the thread pool. */
    int threadId;
    /** The user thread function to execute. */
    void (*func)(int, int, void*);
    /** The user thread payload. */
    void* payload;
};

void* launchThreadsHelper(void* p);
void launchThreads(int threadCount, void (*func)(int, int, void*), void* payload);

#endif
