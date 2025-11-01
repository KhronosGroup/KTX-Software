# 📘 Руководство по использованию KTX-Software для Web (PlayCanvas/Unity)

## 🎯 Что нового

### ✅ Добавлены новые свойства текстуры

Теперь доступны все важные свойства для работы с текстурами:

```javascript
const texture = new ktx.texture(ktx2Data);

// Размеры
texture.baseWidth      // uint32 - ширина базового уровня
texture.baseHeight     // uint32 - высота базового уровня
texture.baseDepth      // uint32 - глубина (для 3D текстур)

// Количество уровней и слоёв
texture.numLevels      // uint32 - количество мипмапов
texture.numLayers      // uint32 - количество слоёв (для array текстур)
texture.numFaces       // uint32 - количество граней (6 для cubemap)
texture.numDimensions  // uint32 - размерность (1D/2D/3D)

// Типы
texture.isArray        // bool - является ли массивом текстур
texture.isCubemap      // bool - является ли кубической картой
texture.isCompressed   // bool - сжатая ли текстура
texture.generateMipmaps // bool - нужно ли генерировать мипмапы

// Формат и метаданные
texture.vkFormat       // uint32 - Vulkan формат
texture.dataSize       // uint32 - общий размер данных
texture.numComponents  // uint32 - количество компонентов
texture.needsTranscoding // bool - нужен ли транскодинг
```

### ✅ ES6 модули (.mjs)

Теперь все файлы генерируются с расширением `.mjs` для нативной поддержки ES6:

- ✅ `libktx.mjs` + `libktx.wasm` (полная версия)
- ✅ `libktx_read.mjs` + `libktx_read.wasm` (только чтение)
- ✅ `msc_basis_transcoder.mjs` + `msc_basis_transcoder.wasm` (транскодер)

---

## 🚀 Как собрать

### Вариант 1: Через скрипт (рекомендуется)

```cmd
REM Активировать Emscripten
C:\emsdk\emsdk_env.bat

REM Перейти в папку проекта
cd D:\sourceProject\repos\KTX-Software

REM Запустить сборку
build_emscripten.bat

REM Для Debug сборки:
build_emscripten.bat debug

REM Для минимальной сборки (только чтение, без GL):
build_emscripten.bat release minimal
```

### Вариант 2: Вручную

```cmd
C:\emsdk\emsdk_env.bat
cd D:\sourceProject\repos\KTX-Software

emcmake cmake -B build-emscripten -G Ninja ^
  -DCMAKE_BUILD_TYPE=Release ^
  -DKTX_FEATURE_GL_UPLOAD=OFF ^
  -DKTX_FEATURE_VK_UPLOAD=OFF ^
  -DKTX_FEATURE_WRITE=ON

cmake --build build-emscripten
```

---

## 💡 Примеры использования

### Пример 1: Загрузка и распаковка KTX2 для PlayCanvas

```javascript
import createKtxModule from './libktx.mjs';

async function loadKTX2ToPlayCanvas(url, device) {
    // 1. Инициализация модуля
    const ktx = await createKtxModule();
    await ktx.ready;

    // 2. Загрузка файла
    const response = await fetch(url);
    const ktx2Data = new Uint8Array(await response.arrayBuffer());

    // 3. Создание KTX текстуры
    const texture = new ktx.texture(ktx2Data);

    console.log('Loaded texture:');
    console.log('  Size:', texture.baseWidth, 'x', texture.baseHeight);
    console.log('  Levels:', texture.numLevels);
    console.log('  Format:', texture.vkFormat);
    console.log('  Compressed:', texture.isCompressed);
    console.log('  Needs transcoding:', texture.needsTranscoding);

    // 4. Транскодирование в формат GPU
    if (texture.needsTranscoding) {
        const targetFormat = selectBestFormat(device);
        const result = texture.transcodeBasis(targetFormat, 0);

        if (result !== ktx.error_code.SUCCESS) {
            console.error('Transcode failed:', result);
            texture.delete();
            return null;
        }
    }

    // 5. Создание PlayCanvas текстуры
    const pcTexture = new pc.Texture(device, {
        width: texture.baseWidth,
        height: texture.baseHeight,
        format: translateVkFormat(texture.vkFormat),
        mipmaps: texture.numLevels > 1,
        minFilter: texture.numLevels > 1 ? pc.FILTER_LINEAR_MIPMAP_LINEAR : pc.FILTER_LINEAR,
        magFilter: pc.FILTER_LINEAR
    });

    // 6. Загрузка всех мипмапов
    for (let level = 0; level < texture.numLevels; level++) {
        const compressedData = texture.getImage(level, 0, 0);
        pcTexture._levels[level] = compressedData;
    }

    pcTexture.upload();

    // 7. Очистка
    texture.delete();

    return pcTexture;
}

// Выбор лучшего формата для GPU
function selectBestFormat(device) {
    if (device.extCompressedTextureASTC) {
        return ktx.transcode_fmt.ASTC_4x4_RGBA;
    } else if (device.extCompressedTextureS3TC) {
        return ktx.transcode_fmt.BC7_RGBA;
    } else if (device.extCompressedTextureETC) {
        return ktx.transcode_fmt.ETC2_RGBA;
    } else {
        return ktx.transcode_fmt.RGBA32;
    }
}

// Перевод VkFormat в PlayCanvas формат
function translateVkFormat(vkFormat) {
    const VK_FORMAT = {
        VK_FORMAT_ASTC_4x4_SRGB_BLOCK: 158,
        VK_FORMAT_BC7_SRGB_BLOCK: 145,
        VK_FORMAT_ETC2_R8G8B8A8_SRGB_BLOCK: 151
    };

    switch (vkFormat) {
        case VK_FORMAT.VK_FORMAT_ASTC_4x4_SRGB_BLOCK:
            return pc.PIXELFORMAT_ASTC_4x4;
        case VK_FORMAT.VK_FORMAT_BC7_SRGB_BLOCK:
            return pc.PIXELFORMAT_BC7;
        case VK_FORMAT.VK_FORMAT_ETC2_R8G8B8A8_SRGB_BLOCK:
            return pc.PIXELFORMAT_ETC2;
        default:
            return pc.PIXELFORMAT_RGBA8;
    }
}
```

### Пример 2: Прямая загрузка в WebGL (с compressedTexImage2D)

```javascript
async function loadKTX2ToWebGL(url, gl) {
    const ktx = await createKtxModule();
    await ktx.ready;

    const response = await fetch(url);
    const ktx2Data = new Uint8Array(await response.arrayBuffer());
    const texture = new ktx.texture(ktx2Data);

    // Выбор формата
    let targetFormat, glInternalFormat;

    if (gl.getExtension('WEBGL_compressed_texture_astc')) {
        targetFormat = ktx.transcode_fmt.ASTC_4x4_RGBA;
        glInternalFormat = 0x93B0; // GL_COMPRESSED_RGBA_ASTC_4x4_KHR
    } else if (gl.getExtension('WEBGL_compressed_texture_s3tc')) {
        targetFormat = ktx.transcode_fmt.BC7_RGBA;
        glInternalFormat = 0x8E8C; // GL_COMPRESSED_RGBA_S3TC_DXT5_EXT
    } else if (gl.getExtension('WEBGL_compressed_texture_etc')) {
        targetFormat = ktx.transcode_fmt.ETC2_RGBA;
        glInternalFormat = 0x9278; // GL_COMPRESSED_RGBA8_ETC2_EAC
    } else {
        targetFormat = ktx.transcode_fmt.RGBA32;
        glInternalFormat = gl.RGBA;
    }

    // Транскодирование
    if (texture.needsTranscoding) {
        texture.transcodeBasis(targetFormat, 0);
    }

    // Создание GL текстуры
    const glTexture = gl.createTexture();
    gl.bindTexture(gl.TEXTURE_2D, glTexture);

    // Загрузка всех мипмапов
    for (let level = 0; level < texture.numLevels; level++) {
        const compressedData = texture.getImage(level, 0, 0);
        const mipWidth = Math.max(1, texture.baseWidth >> level);
        const mipHeight = Math.max(1, texture.baseHeight >> level);

        if (targetFormat === ktx.transcode_fmt.RGBA32) {
            // Несжатый
            gl.texImage2D(
                gl.TEXTURE_2D, level, gl.RGBA,
                mipWidth, mipHeight, 0,
                gl.RGBA, gl.UNSIGNED_BYTE, compressedData
            );
        } else {
            // Сжатый
            gl.compressedTexImage2D(
                gl.TEXTURE_2D, level, glInternalFormat,
                mipWidth, mipHeight, 0, compressedData
            );
        }
    }

    // Параметры фильтрации
    gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MIN_FILTER,
        texture.numLevels > 1 ? gl.LINEAR_MIPMAP_LINEAR : gl.LINEAR);
    gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MAG_FILTER, gl.LINEAR);

    // BASE/MAX LOD (WebGL 2.0)
    if (gl.texParameteri) {
        gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_BASE_LEVEL, 0);
        gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MAX_LEVEL, texture.numLevels - 1);
    }

    texture.delete();
    return glTexture;
}
```

### Пример 3: Прогрессивная загрузка мипмапов

```javascript
async function loadKTX2Progressive(url, gl, onProgress) {
    const ktx = await createKtxModule();
    await ktx.ready;

    const response = await fetch(url);
    const ktx2Data = new Uint8Array(await response.arrayBuffer());
    const texture = new ktx.texture(ktx2Data);

    if (texture.needsTranscoding) {
        texture.transcodeBasis(ktx.transcode_fmt.ASTC_4x4_RGBA, 0);
    }

    const glTexture = gl.createTexture();
    gl.bindTexture(gl.TEXTURE_2D, glTexture);

    // Сначала загружаем базовый (самый детальный) уровень
    const baseData = texture.getImage(0, 0, 0);
    gl.compressedTexImage2D(
        gl.TEXTURE_2D, 0, 0x93B0,
        texture.baseWidth, texture.baseHeight, 0, baseData
    );

    if (onProgress) onProgress(1, texture.numLevels);

    // Затем асинхронно загружаем остальные мипы
    for (let level = 1; level < texture.numLevels; level++) {
        await new Promise(resolve => setTimeout(resolve, 0)); // Не блокировать UI

        const mipData = texture.getImage(level, 0, 0);
        const mipWidth = Math.max(1, texture.baseWidth >> level);
        const mipHeight = Math.max(1, texture.baseHeight >> level);

        gl.compressedTexImage2D(
            gl.TEXTURE_2D, level, 0x93B0,
            mipWidth, mipHeight, 0, mipData
        );

        if (onProgress) onProgress(level + 1, texture.numLevels);
    }

    gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MIN_FILTER, gl.LINEAR_MIPMAP_LINEAR);
    gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MAG_FILTER, gl.LINEAR);

    texture.delete();
    return glTexture;
}

// Использование
loadKTX2Progressive('texture.ktx2', gl, (loaded, total) => {
    console.log(`Loaded ${loaded}/${total} mip levels`);
});
```

### Пример 4: Работа с массивами текстур (Texture Arrays)

```javascript
async function loadTextureArray(url, gl) {
    const ktx = await createKtxModule();
    await ktx.ready;

    const response = await fetch(url);
    const ktx2Data = new Uint8Array(await response.arrayBuffer());
    const texture = new ktx.texture(ktx2Data);

    if (!texture.isArray) {
        console.error('Not a texture array!');
        texture.delete();
        return null;
    }

    console.log('Texture Array Info:');
    console.log('  Layers:', texture.numLayers);
    console.log('  Size:', texture.baseWidth, 'x', texture.baseHeight);
    console.log('  Levels:', texture.numLevels);

    if (texture.needsTranscoding) {
        texture.transcodeBasis(ktx.transcode_fmt.ASTC_4x4_RGBA, 0);
    }

    const glTexture = gl.createTexture();
    gl.bindTexture(gl.TEXTURE_2D_ARRAY, glTexture);

    // Загрузка всех слоёв и уровней
    for (let level = 0; level < texture.numLevels; level++) {
        for (let layer = 0; layer < texture.numLayers; layer++) {
            const layerData = texture.getImage(level, layer, 0);
            const mipWidth = Math.max(1, texture.baseWidth >> level);
            const mipHeight = Math.max(1, texture.baseHeight >> level);

            // ... загрузка в GL_TEXTURE_2D_ARRAY
        }
    }

    texture.delete();
    return glTexture;
}
```

### Пример 5: Работа с кубическими картами (Cubemaps)

```javascript
async function loadCubemap(url, gl) {
    const ktx = await createKtxModule();
    await ktx.ready;

    const response = await fetch(url);
    const ktx2Data = new Uint8Array(await response.arrayBuffer());
    const texture = new ktx.texture(ktx2Data);

    if (!texture.isCubemap) {
        console.error('Not a cubemap!');
        texture.delete();
        return null;
    }

    console.log('Cubemap Info:');
    console.log('  Faces:', texture.numFaces); // Должно быть 6
    console.log('  Size:', texture.baseWidth, 'x', texture.baseHeight);
    console.log('  Levels:', texture.numLevels);

    if (texture.needsTranscoding) {
        texture.transcodeBasis(ktx.transcode_fmt.ASTC_4x4_RGBA, 0);
    }

    const glTexture = gl.createTexture();
    gl.bindTexture(gl.TEXTURE_CUBE_MAP, glTexture);

    const faces = [
        gl.TEXTURE_CUBE_MAP_POSITIVE_X,
        gl.TEXTURE_CUBE_MAP_NEGATIVE_X,
        gl.TEXTURE_CUBE_MAP_POSITIVE_Y,
        gl.TEXTURE_CUBE_MAP_NEGATIVE_Y,
        gl.TEXTURE_CUBE_MAP_POSITIVE_Z,
        gl.TEXTURE_CUBE_MAP_NEGATIVE_Z
    ];

    // Загрузка всех граней и уровней
    for (let level = 0; level < texture.numLevels; level++) {
        for (let face = 0; face < 6; face++) {
            const faceData = texture.getImage(level, 0, face);
            const mipWidth = Math.max(1, texture.baseWidth >> level);
            const mipHeight = Math.max(1, texture.baseHeight >> level);

            gl.compressedTexImage2D(
                faces[face], level, 0x93B0,
                mipWidth, mipHeight, 0, faceData
            );
        }
    }

    gl.texParameteri(gl.TEXTURE_CUBE_MAP, gl.TEXTURE_MIN_FILTER, gl.LINEAR_MIPMAP_LINEAR);
    gl.texParameteri(gl.TEXTURE_CUBE_MAP, gl.TEXTURE_MAG_FILTER, gl.LINEAR);

    texture.delete();
    return glTexture;
}
```

---

## 📊 Доступные форматы транскодирования

```javascript
ktx.transcode_fmt.ETC1_RGB        // ETC1 RGB (без альфы)
ktx.transcode_fmt.BC1_RGB         // BC1/DXT1 RGB
ktx.transcode_fmt.BC3_RGBA        // BC3/DXT5 RGBA
ktx.transcode_fmt.BC4_R           // BC4 одноканальный
ktx.transcode_fmt.BC5_RG          // BC5 двухканальный
ktx.transcode_fmt.BC7_RGBA        // BC7 RGBA (высокое качество)
ktx.transcode_fmt.ETC2_RGBA       // ETC2 RGBA
ktx.transcode_fmt.ASTC_4x4_RGBA   // ASTC 4x4 RGBA
ktx.transcode_fmt.PVRTC1_4_RGB    // PVRTC1 RGB (iOS старые)
ktx.transcode_fmt.PVRTC1_4_RGBA   // PVRTC1 RGBA (iOS старые)
ktx.transcode_fmt.RGBA32          // RGBA8888 несжатый (fallback)
ktx.transcode_fmt.RGB565          // RGB565 несжатый
ktx.transcode_fmt.RGBA4444        // RGBA4444 несжатый
```

---

## 🔧 Устранение проблем

### Проблема: `texture.numLevels is undefined`

**Причина:** Используете старую версию без патча.

**Решение:** Пересоберите проект с новыми изменениями:
```cmd
C:\emsdk\emsdk_env.bat
cd D:\sourceProject\repos\KTX-Software
build_emscripten.bat
```

### Проблема: `Cannot find module './libktx.mjs'`

**Причина:** Bundler не распознаёт .mjs файлы.

**Решение для Vite:**
```javascript
// vite.config.js
export default {
  resolve: {
    extensions: ['.mjs', '.js', '.ts']
  }
}
```

**Решение для Webpack:**
```javascript
// webpack.config.js
module.exports = {
  resolve: {
    extensions: ['.mjs', '.js']
  }
}
```

### Проблема: Большой размер файла

**Решение:** Используйте минимальную сборку:
```cmd
build_emscripten.bat release minimal
```

Это создаст только `libktx_read.mjs` без GL-зависимостей (~200 KB вместо ~1.8 MB).

---

## 📦 Размеры файлов

| Файл | Полная сборка | Минимальная сборка |
|------|--------------|-------------------|
| libktx.mjs | ~200 KB | - |
| libktx.wasm | ~1.8 MB | - |
| libktx_read.mjs | ~50 KB | ~50 KB |
| libktx_read.wasm | ~200 KB | ~200 KB |
| **Всего** | **~2 MB** | **~250 KB** |
| **Gzip** | **~600 KB** | **~70 KB** |

---

## 🎓 Дополнительные ресурсы

- **KTX Specification:** https://github.khronos.org/KTX-Specification/
- **Basis Universal:** https://github.com/BinomialLLC/basis_universal
- **PlayCanvas Engine:** https://developer.playcanvas.com/
- **WebGL Compressed Textures:** https://www.khronos.org/webgl/wiki/Using_Compressed_Textures_in_WebGL

---

## ✅ Чек-лист для интеграции

- [ ] Собрал проект с новыми флагами
- [ ] Проверил что файлы имеют расширение .mjs
- [ ] Проверил доступность `texture.numLevels`
- [ ] Протестировал транскодирование в целевой формат
- [ ] Проверил загрузку всех мипмапов
- [ ] Настроил правильные параметры фильтрации
- [ ] Освобождаю память через `texture.delete()`
- [ ] Работает в целевом движке (PlayCanvas/Unity/etc)

---

**Готово! Теперь у вас есть полнофункциональная библиотека для работы с KTX2 текстурами в Web! 🎉**
