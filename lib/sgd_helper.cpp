#include "ktx.h"
#include "ktxint.h"
#include "texture2.h"
#include "basis_sgd.h"
#include "basisu/transcoder/basisu_file_headers.h"

bool ktxTexture2_getHasAlpha(ktxTexture2* This) {
    ktxTexture2_private* priv = This->_private;
    uint8_t* bgd = priv->_supercompressionGlobalData;
    ktxBasisGlobalHeader* bgdh = (ktxBasisGlobalHeader*) bgd;
    return (bgdh->globalFlags & basist::cBASISHeaderFlagHasAlphaSlices) != 0;
}
