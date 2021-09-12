package org.khronos.ktx;

public class KTXTexture2 extends KTXTexture {
    protected KTXTexture2(long instance) {
        super(instance);
    }

    public native int getOETF();
    public native boolean getPremultipliedAlpha();
    public native boolean needsTranscoding();
    public native int getVkFormat();
    public native int getSupercompressionScheme();

    public native int compressBasisEx(KTXBasisParams params);
    public native int compressBasis(int quality);
    public native int transcodeBasis(int outputFormat, int transcodeFlags);

    /**
     * Create a fresh {@link KTXTexture2}
     *
     * @param createInfo - Paramaters for the texture
     * @param storageAllocation - Pass {@link KTXCreateStorage.ALLOC} if you will write image data.
     */
    public static native KTXTexture2 create(KTXTextureCreateInfo createInfo,
                                            int storageAllocation);

    /**
     * Create a {@link KTXTexture2} from a file.
     *
     * @param filename - The name of the file to read.
     * @param createFlags - Pass {@link KTXTextureCreateFlagBits.LOAD_IMAGE_DATA_BIT} if you
     *                   want to read image data! Otherwise, {@link KTXTexture.getData()} will
     *                    return null.
     */
    public static native KTXTexture2 createFromNamedFile(String filename,
                                                         int createFlags);
}
