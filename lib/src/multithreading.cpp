/*
 * Copyright 2026 The Khronos Group Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "multithreading.h"

/**
 * @internal
 * @~English
 * @brief Helper function to translate thread entry points.
 *
 * Convert a (void*) thread entry to an (int, void*) thread entry, where the
 * integer contains the thread ID in the thread pool.
 *
 * @param p The thread launch helper payload.
 */
void*
launchThreadsHelper(void* p) {
    LaunchDesc* ltd = (LaunchDesc*)p;
    ltd->func(ltd->threadCount, ltd->threadId, ltd->payload);
    return nullptr;
}

void
launchThreads(int threadCount, void (*func)(int, int, void*), void* payload) {
    // Directly execute single threaded workloads on this thread
    if (threadCount <= 1) {
        func(1, 0, payload);
        return;
    }

    // Otherwise spawn worker threads
    LaunchDesc* threadDescs = new LaunchDesc[threadCount];
    for (int i = 0; i < threadCount; i++) {
        threadDescs[i].threadCount = threadCount;
        threadDescs[i].threadId = i;
        threadDescs[i].payload = payload;
        threadDescs[i].func = func;

        pthread_create(&(threadDescs[i].threadHandle), nullptr, launchThreadsHelper,
                       (void*)&(threadDescs[i]));
    }

    // ... and then wait for them to complete
    for (int i = 0; i < threadCount; i++) {
        pthread_join(threadDescs[i].threadHandle, nullptr);
    }

    delete[] threadDescs;
}
