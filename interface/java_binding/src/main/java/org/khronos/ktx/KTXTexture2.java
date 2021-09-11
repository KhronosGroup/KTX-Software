package org.khronos.ktx;

public class KTXTexture2 extends KTXTexture {
    protected KTXTexture2(long instance) {
        super(instance);
    }

    public native int getOETF();
    public native boolean getPremultipliedAlpha();
    public native boolean needsTranscoding();
    public native long getDataSizeUncompressed();
    public native long getImageSize(int level);

    public native int compressBasisEx(KTXBasisParams params);
    public native int compressBasis(int quality);
    public native int transcodeBasis(int outputFormat, int transcodeFlags);

    public static native KTXTexture2 create(KTXTextureCreateInfo createInfo,
                                            KTXCreateStorage storageAllocation);
}
