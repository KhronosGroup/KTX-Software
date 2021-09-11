package org.khronos.ktx.test;

import org.junit.Test;

import java.nio.file.Path;
import java.nio.file.Paths;

public class KTXTexture1Test {

    @Test
    public void testKTX1Load() {
        Path workingDirectory = Paths.get("").toAbsolutePath();
        Path testKTXFile = Paths.get(workingDirectory.toString(), "../../testimages/etc1.ktx");

    }
}
