/*
 * Copyright (c) 2023, Shukant Pal, robnugent, and Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

package org.khronos.ktx.test;

import org.junit.jupiter.api.Test;
import org.junit.jupiter.api.extension.ExtendWith;
import org.khronos.ktx.*;
import java.util.Random;
import java.util.logging.Level;
import java.util.logging.Logger;

@ExtendWith({ KtxTestLibraryLoader.class })
public class KtxParallelTest {
    private static final int NUM_THREADS = 2;
    private static final Logger logger = Logger.getLogger(KtxParallelTest.class.getCanonicalName());

    @Test
    public void testParallelAstcConversion() throws InterruptedException {
        final Thread[] runThreads = new Thread[NUM_THREADS];

        for (int i = 0; i < NUM_THREADS; i++) {
            final KtxTestRun run = new KtxTestRun(i);
            final Thread runThread = new Thread(run);
            runThread.setDaemon(false);
            runThread.start();

            runThreads[i] = runThread;
        }

        for (Thread thread : runThreads) {
            thread.join();
        }
    }

    private static class KtxTestRun implements Runnable {
        private final int id;
        private final Random testRandomizer = new Random();

        public KtxTestRun(int id) {
            this.id = id;
        }

        public void run() {
            // Repeatedly create a compress an image.
            for (int i = 0; i < 300; i++) {
                final int w = (testRandomizer.nextInt() % 512) + 1024;
                final int h = w;
                final int size = convertToASTC(w, h);

                // Change level to INFO for logging
                logger.log(Level.FINE,id + " iteration: " + i + ", size: " + w + "x" + h +  ", compressed data size is " + size);
            }
        }

        public int convertToASTC(int w, int h) {
            // Create Uncompressed texture
            final KtxTextureCreateInfo info = new KtxTextureCreateInfo();
            info.setBaseWidth(w);
            info.setBaseHeight(h);
            info.setVkFormat(VkFormat.VK_FORMAT_R8G8B8_SRGB); // Uncompressed
            final KtxTexture2 t = KtxTexture2.create(info, KtxCreateStorage.ALLOC);

            // Pass the uncompressed data
            int bufferSize = w * h * 3;
            final byte[] rgbBA = new byte[bufferSize];
            t.setImageFromMemory(0, 0, 0, rgbBA);

            // Compress the data
            final KtxAstcParams p = new KtxAstcParams();
            p.setBlockDimension(KtxPackAstcBlockDimension.D8x8);
            p.setMode(KtxPackAstcEncoderMode.LDR);
            p.setQualityLevel(KtxPackAstcQualityLevel.EXHAUSTIVE);
            final int rc = t.compressAstcEx(p);
            if (rc != KtxErrorCode.SUCCESS) {
                throw new RuntimeException("ASTC error " + rc);
            }
            final int retDataLen = (int) t.getDataSize();

            // Free things up - segfault usually occurs inside this destroy() call
            t.destroy();

            return retDataLen;
        }
    }
}
