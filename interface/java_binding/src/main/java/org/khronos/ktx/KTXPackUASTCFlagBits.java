package org.khronos.ktx;

public class KTXPackUASTCFlagBits {
    public static final int LEVEL_FASTEST = 0;
    public static final int LEVEL_FASTER = 1;
    public static final int LEVEL_DEFAULT = 2;
    public static final int LEVEL_SLOWER = 3;
    public static final int LEVEL_VERYSLOW = 4;
    public static final int MAX_LEVEL = LEVEL_VERYSLOW;
    public static final int LEVEL_MASK = 0xF;
    public static final int FAVOR_UASTC_ERROR = 8;
    public static final int FAVOR_BC7_ERROR = 16;
    public static final int ETC1_FASTER_HINTS = 64;
    public static final int ETC1_FASTEST_HINTS = 128;
    public static final int ETC1_DISABLE_FLIP_AND_INDIVIDUAL = 256;
}
