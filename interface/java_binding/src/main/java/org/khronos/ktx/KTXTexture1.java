package org.khronos.ktx;

public class KTXTexture1 extends KTXTexture {
    protected KTXTexture1(long instance) {
        super(instance);
    }

    public native int getGlFormat();
    public native int getGlInternalformat();
    public native int getGlBaseInternalformat();
    public native int getGlType();

    /**
     * Create a fresh {@link KTXTexture1}
     *
     * @param createInfo - Paramaters for the texture
     * @param storageAllocation - Pass {@link KTXCreateStorage.ALLOC} if you will write image data.
     */
    public static native KTXTexture1 create(KTXTextureCreateInfo info,
                                            int storageAllocation);

    /**
     * Create a {@link KTXTexture1} from a file.
     *
     * @param filename - The name of the file to read.
     * @param createFlags - Pass {@link KTXTextureCreateFlagBits.LOAD_IMAGE_DATA_BIT} if you
     *                   want to read image data! Otherwise, {@link KTXTexture.getData()} will
     *                    return null.
     */
    public static native KTXTexture1 createFromNamedFile(String filename,
                                                         int createFlags);

    public static KTXTexture1 createFromNamedFile(String filename) {
        return createFromNamedFile(filename, KTXTextureCreateFlagBits.LOAD_IMAGE_DATA_BIT);
    }
}
