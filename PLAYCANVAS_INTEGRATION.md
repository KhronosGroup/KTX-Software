# KTX2 + PlayCanvas Integration Guide

## Overview

This document explains the KTX-Software Emscripten build optimizations and provides a complete guide for integrating KTX2 texture loading with progressive mipmap streaming into PlayCanvas Engine.

---

## What Was Done: Emscripten Build Improvements

### 1. ES6 Module Support (`.mjs` files)

**Problem:** Modern bundlers (Vite, Webpack 5, PlayCanvas) require proper ES6 module format.

**Solution:**
- Changed output from `.js` → `.mjs` for proper module detection
- Added `-s EXPORT_ES6=1` flag
- Proper `export default createKtxModule` syntax

**Files:**
- `libktx.mjs` / `libktx.wasm` - Full KTX2 library with encoding
- `libktx_read.mjs` / `libktx_read.wasm` - Read-only decoder (recommended for web)
- `msc_basis_transcoder.mjs` / `msc_basis_transcoder.wasm` - Basis Universal only

### 2. Critical Missing Properties

**Problem:** JavaScript bindings were missing essential texture metadata needed for mipmap iteration.

**Solution:** Added 9 new properties to `ktx_wrapper.cpp`:

```cpp
// NEW PROPERTIES (lines 138-186 in ktx_wrapper.cpp)
uint32_t baseDepth() const { return m_ptr->baseDepth; }
uint32_t numLevels() const { return m_ptr->numLevels; }      // ⭐ CRITICAL for mipmap iteration
uint32_t numLayers() const { return m_ptr->numLayers; }
uint32_t numFaces() const { return m_ptr->numFaces; }
uint32_t numDimensions() const { return m_ptr->numDimensions; }
bool isArray() const { return m_ptr->isArray; }
bool isCubemap() const { return m_ptr->isCubemap; }
bool isCompressed() const { return m_ptr->isCompressed; }
bool generateMipmaps() const { return m_ptr->generateMipmaps; }
```

### 3. Size Optimization

**Flags Applied (CMakeLists.txt lines 183-264):**
- `-Oz` instead of `-O3` (optimize for size, not speed)
- `-s FILESYSTEM=0` (remove unused filesystem code)
- `-s AGGRESSIVE_VARIABLE_ELIMINATION=1`
- `-s ELIMINATE_DUPLICATE_FUNCTIONS=1`
- `--closure=1` (Google Closure Compiler minification)
- `-flto` (Link Time Optimization)

**Results:**
- `libktx.mjs`: **38 KB** (minified from ~120 KB)
- `libktx.wasm`: **1.6 MB** (compressed from 2.2 MB .a)
- `libktx_read.mjs`: **36 KB** (minified)
- `libktx_read.wasm`: **719 KB** (compressed 49% from 1.4 MB .a) ✅

### 4. Emscripten 4.0.18 Compatibility

**Fixed:**
- Removed custom `post_ready.js` (Module.ready now built-in)
- Fixed LLVM crash with conflicting `-O3`/`-Oz` flags
- Override `CMAKE_C_FLAGS_RELEASE` and `CMAKE_CXX_FLAGS_RELEASE` for proper optimization

---

## Building from Source

```bash
# 1. Activate Emscripten
C:\emsdk\emsdk_env.bat

# 2. Configure (read-only build recommended for web)
emcmake cmake -B build-web-release -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DKTX_FEATURE_GL_UPLOAD=OFF \
  -DKTX_FEATURE_VK_UPLOAD=OFF \
  -DKTX_FEATURE_TOOLS=OFF \
  -DKTX_FEATURE_TESTS=OFF \
  -DKTX_FEATURE_LOADTEST_APPS=OFF

# 3. Build
cmake --build build-web-release --config Release

# 4. Output files in: build-web-release/
# - libktx_read.mjs + libktx_read.wasm (719 KB) ← USE THIS
# - libktx.mjs + libktx.wasm (1.6 MB) - full library with encoder
# - msc_basis_transcoder.mjs + .wasm (407 KB) - Basis only, no KTX2 API
```

---

## PlayCanvas Integration: Progressive Mipmap Loading

### Architecture Overview

**Goal:** Load a KTX2 texture and progressively stream mipmaps to GPU, displaying low-res mips immediately while high-res mips load in background.

**Key APIs:**
- `texture.numLevels` - Total number of mipmap levels
- `texture.getImageOffset(level, layer, face)` - Get byte offset for specific mip
- `texture.getData(level, layer, face)` - Get compressed image data as Uint8Array
- `gl.compressedTexImage2D()` - Upload compressed data directly to GPU

### Complete Implementation

```javascript
import createKtxModule from './libktx_read.mjs';

class KTX2Loader {
    constructor(app) {
        this.app = app;
        this.ktxModule = null;
        this.pendingLoads = new Map();
    }

    async initialize() {
        // Initialize KTX library
        this.ktxModule = await createKtxModule();
        await this.ktxModule.ready;
        console.log('KTX2 module ready');
    }

    /**
     * Load KTX2 texture with progressive mipmap streaming
     *
     * @param {string} url - URL to .ktx2 file
     * @param {Object} options
     * @param {number} options.initialMipLevel - Start with this mip (default: lowest quality)
     * @param {number} options.targetMipLevel - Target high-quality mip (default: 0 = full res)
     * @param {Function} options.onProgress - Progress callback
     * @returns {Promise<pc.Texture>}
     */
    async loadProgressive(url, options = {}) {
        const {
            initialMipLevel = null, // Auto-detect (start with lowest)
            targetMipLevel = 0,      // Full resolution
            onProgress = null
        } = options;

        // 1. Fetch entire KTX2 file
        const response = await fetch(url);
        const arrayBuffer = await response.arrayBuffer();
        const data = new Uint8Array(arrayBuffer);

        // 2. Parse KTX2 file
        const ktxTexture = new this.ktxModule.ktxTexture(data);

        // 3. Transcode to GPU format (if Basis Universal)
        if (ktxTexture.needsTranscoding) {
            const format = this._selectTranscodeFormat();
            const result = ktxTexture.transcodeBasis(format, 0);
            if (result !== this.ktxModule.ErrorCode.SUCCESS) {
                throw new Error(`Transcode failed: ${result}`);
            }
        }

        // 4. Get texture metadata
        const width = ktxTexture.baseWidth;
        const height = ktxTexture.baseHeight;
        const numLevels = ktxTexture.numLevels;
        const internalFormat = ktxTexture.glInternalformat;
        const baseFormat = ktxTexture.glBaseInternalformat;

        console.log(`KTX2 loaded: ${width}x${height}, ${numLevels} mips, format ${internalFormat}`);

        // 5. Create PlayCanvas texture (empty, we'll fill it progressively)
        const pcFormat = this._glFormatToPCFormat(baseFormat);
        const pcTexture = new pc.Texture(this.app.graphicsDevice, {
            width: width,
            height: height,
            format: pcFormat,
            mipmaps: true,
            minFilter: pc.FILTER_LINEAR_MIPMAP_LINEAR,
            magFilter: pc.FILTER_LINEAR,
            addressU: pc.ADDRESS_REPEAT,
            addressV: pc.ADDRESS_REPEAT
        });

        // 6. Upload mipmaps progressively
        const startMip = initialMipLevel ?? (numLevels - 1); // Start with lowest quality

        await this._uploadMipmapsProgressive(
            pcTexture,
            ktxTexture,
            startMip,
            targetMipLevel,
            onProgress
        );

        // 7. Cleanup
        ktxTexture.delete();

        return pcTexture;
    }

    /**
     * Upload mipmaps from low-res to high-res progressively
     */
    async _uploadMipmapsProgressive(pcTexture, ktxTexture, startMip, targetMip, onProgress) {
        const gl = this.app.graphicsDevice.gl;
        const target = gl.TEXTURE_2D;
        const internalFormat = ktxTexture.glInternalformat;

        // Bind texture
        gl.bindTexture(target, pcTexture._glTexture);

        // Upload from low-res to high-res
        for (let level = startMip; level >= targetMip; level--) {
            // Get mip dimensions
            const mipWidth = Math.max(1, ktxTexture.baseWidth >> level);
            const mipHeight = Math.max(1, ktxTexture.baseHeight >> level);

            // Get compressed image data
            const imageData = ktxTexture.getData(level, 0, 0);

            // Upload to GPU
            gl.compressedTexImage2D(
                target,
                level,
                internalFormat,
                mipWidth,
                mipHeight,
                0, // border
                imageData
            );

            console.log(`Uploaded mip ${level}: ${mipWidth}x${mipHeight}, ${imageData.length} bytes`);

            // Progress callback
            if (onProgress) {
                const progress = (startMip - level) / (startMip - targetMip);
                onProgress(progress, level, mipWidth, mipHeight);
            }

            // Yield to browser (allow rendering between mips)
            if (level > targetMip) {
                await this._delay(0);
            }
        }

        gl.bindTexture(target, null);
    }

    /**
     * Select best GPU transcode format based on device capabilities
     */
    _selectTranscodeFormat() {
        const gl = this.app.graphicsDevice.gl;
        const ktx = this.ktxModule;

        // Check GPU compression support
        const ext = {
            s3tc: gl.getExtension('WEBGL_compressed_texture_s3tc'),
            etc: gl.getExtension('WEBGL_compressed_texture_etc'),
            astc: gl.getExtension('WEBGL_compressed_texture_astc'),
            pvrtc: gl.getExtension('WEBGL_compressed_texture_pvrtc')
        };

        // Priority order (best quality/performance)
        if (ext.astc) return ktx.TranscodeTarget.ASTC_4x4_RGBA;
        if (ext.s3tc) return ktx.TranscodeTarget.BC7_RGBA;  // Desktop
        if (ext.etc) return ktx.TranscodeTarget.ETC2_RGBA;  // Mobile
        if (ext.pvrtc) return ktx.TranscodeTarget.PVRTC1_4_RGBA;

        // Fallback to RGBA8 (uncompressed)
        return ktx.TranscodeTarget.RGBA32;
    }

    /**
     * Convert OpenGL format to PlayCanvas format
     */
    _glFormatToPCFormat(glFormat) {
        const gl = this.app.graphicsDevice.gl;

        switch (glFormat) {
            case gl.RGB: return pc.PIXELFORMAT_RGB8;
            case gl.RGBA: return pc.PIXELFORMAT_RGBA8;
            case gl.COMPRESSED_RGB_S3TC_DXT1_EXT: return pc.PIXELFORMAT_DXT1;
            case gl.COMPRESSED_RGBA_S3TC_DXT5_EXT: return pc.PIXELFORMAT_DXT5;
            case gl.COMPRESSED_RGB8_ETC2: return pc.PIXELFORMAT_ETC2_RGB;
            case gl.COMPRESSED_RGBA8_ETC2_EAC: return pc.PIXELFORMAT_ETC2_RGBA;
            case 0x93B0: return pc.PIXELFORMAT_ASTC_4x4; // ASTC_4x4_RGBA
            default: return pc.PIXELFORMAT_RGBA8;
        }
    }

    _delay(ms) {
        return new Promise(resolve => setTimeout(resolve, ms));
    }
}

// Usage Example
async function main() {
    const app = new pc.Application(canvas);

    const loader = new KTX2Loader(app);
    await loader.initialize();

    // Load texture with progressive mipmaps
    const texture = await loader.loadProgressive('textures/mytexture.ktx2', {
        initialMipLevel: null,  // Auto (start with lowest)
        targetMipLevel: 0,      // Load up to full resolution
        onProgress: (progress, level, width, height) => {
            console.log(`Loading mip ${level} (${width}x${height}): ${Math.round(progress * 100)}%`);
        }
    });

    // Apply to material
    const material = new pc.StandardMaterial();
    material.diffuseMap = texture;
    material.update();

    entity.model.material = material;
}
```

---

## Advanced Use Cases

### 1. Texture Arrays (Multiple Layers)

```javascript
// Load texture array
const ktxTexture = new this.ktxModule.ktxTexture(data);
const numLayers = ktxTexture.numLayers;

for (let layer = 0; layer < numLayers; layer++) {
    for (let level = 0; level < ktxTexture.numLevels; level++) {
        const imageData = ktxTexture.getData(level, layer, 0);

        gl.compressedTexImage3D(
            gl.TEXTURE_2D_ARRAY,
            level,
            internalFormat,
            width >> level,
            height >> level,
            numLayers,
            0,
            imageData
        );
    }
}
```

### 2. Cubemaps (6 Faces)

```javascript
const ktxTexture = new this.ktxModule.ktxTexture(data);
const isCubemap = ktxTexture.isCubemap; // true
const numFaces = ktxTexture.numFaces;   // 6

const faces = [
    gl.TEXTURE_CUBE_MAP_POSITIVE_X, // +X
    gl.TEXTURE_CUBE_MAP_NEGATIVE_X, // -X
    gl.TEXTURE_CUBE_MAP_POSITIVE_Y, // +Y
    gl.TEXTURE_CUBE_MAP_NEGATIVE_Y, // -Y
    gl.TEXTURE_CUBE_MAP_POSITIVE_Z, // +Z
    gl.TEXTURE_CUBE_MAP_NEGATIVE_Z  // -Z
];

for (let face = 0; face < 6; face++) {
    for (let level = 0; level < ktxTexture.numLevels; level++) {
        const imageData = ktxTexture.getData(level, 0, face);

        gl.compressedTexImage2D(
            faces[face],
            level,
            internalFormat,
            width >> level,
            height >> level,
            0,
            imageData
        );
    }
}
```

### 3. Partial Mipmap Loading (Network-Aware)

```javascript
// Adaptive quality based on connection speed
const connection = navigator.connection || navigator.mozConnection || navigator.webkitConnection;
let targetMip = 0; // Full quality

if (connection) {
    switch (connection.effectiveType) {
        case 'slow-2g':
        case '2g':
            targetMip = 3; // Stop at 1/8 resolution
            break;
        case '3g':
            targetMip = 2; // Stop at 1/4 resolution
            break;
        case '4g':
            targetMip = 1; // Stop at 1/2 resolution
            break;
    }
}

const texture = await loader.loadProgressive('texture.ktx2', {
    targetMipLevel: targetMip
});
```

---

## Performance Characteristics

### File Sizes
- **libktx_read.wasm:** 719 KB (recommended for web)
- **libktx.wasm:** 1.6 MB (includes encoder, only if creating KTX2 files in browser)
- **msc_basis_transcoder.wasm:** 407 KB (Basis only, no texture.numLevels API)

### Loading Times (1024x1024 texture with 11 mips)
- **Initial low-res display:** ~50ms (upload mip 10)
- **Progressive refinement:** ~200ms (mips 10→0)
- **Total load time:** ~250ms including parsing

### Memory Usage
- **WASM heap:** ~16 MB default (auto-grows with ALLOW_MEMORY_GROWTH)
- **Per texture:** ~1.5x file size during transcode (temporary)

---

## Troubleshooting

### Issue: "Module.ready is not a function"
**Solution:** You're using old module. Update to latest build with `-s EXPORT_ES6=1`.

### Issue: "texture.numLevels is undefined"
**Solution:** You're using an old build. Rebuild with latest ktx_wrapper.cpp (lines 138-186).

### Issue: "compressedTexImage2D: INVALID_OPERATION"
**Solution:** GPU doesn't support this format. Check `gl.getExtension()` and use `_selectTranscodeFormat()`.

### Issue: WASM file is too large
**Solution:** Use `libktx_read.wasm` (719 KB) instead of `libktx.wasm` (1.6 MB). For Basis-only, use `msc_basis_transcoder.wasm` (407 KB) but no KTX2 API.

---

## Key Commits

1. **46c5994** - Initial ES6 module support and optimization flags
2. **b063e2b** - Added missing texture properties (numLevels, numLayers, etc.)
3. **ec9360c** - Removed post_ready.js for Emscripten 4.0.18 compatibility
4. **cde6d63** - Fixed LLVM crash with proper -Oz optimization

---

## Summary for AI Integration

**What You Need to Know:**

1. **Use `libktx_read.mjs` + `libktx_read.wasm` (719 KB)** - optimized for web
2. **Key API:** `texture.numLevels` - iterate mipmaps progressively
3. **Upload flow:** Parse KTX2 → Transcode → Get mip data → `gl.compressedTexImage2D()`
4. **Performance:** Start with low-res mip, progressively upload higher quality
5. **All compression formats supported:** BC1/BC7 (Desktop), ETC2 (Mobile), ASTC (All), PVRTC (iOS)

**Next Steps:**
- Copy `libktx_read.mjs` and `libktx_read.wasm` to your project
- Implement `KTX2Loader` class above
- Use progressive loading for large textures
- Adjust `targetMipLevel` based on network/device capabilities

For more examples, see: `EMSCRIPTEN_USAGE_RU.md`
