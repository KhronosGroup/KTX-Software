package org.khronos.ktx;

public class KTXTexture1 extends KTXTexture{
    protected KTXTexture1(long instance) {
        super(instance);
    }

    public native int getGlFormat();
    public native int getInternalformat();
    public native int getGlBaseInternalformat();
    public native int getGlType();

    public static native KTXTexture1 create(KTXTextureCreateInfo info,
                                            int storageAllocation);
}
