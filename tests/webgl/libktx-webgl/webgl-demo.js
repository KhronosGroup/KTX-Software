/*
 * Copyright 2015-2020 MDN Contributors
 * SPDX-License-Identifier: CC0-1.0
 */

/*
This code originated from sample 7 in the MDN WebGL examples
at https://github.com/mdn/webgl-examples which is licensed
under Creative Commons Zero v1.0 Universal. Modifications
made here are also licensed under CC0v1.
*/

var cubeRotation = 0.0;
var gl;
var defaultTexture;

var astcSupported = false;
var etcSupported = false;
var dxtSupported = false;
var pvrtcSupported = false;

main();

//
// Start here
//
function main() {
  const canvas = document.querySelector('#glcanvas');
  gl = canvas.getContext('webgl2');

  // If we don't have a GL context, give up now

  if (!gl) {
    alert('Unable to initialize WebGL. Your browser or machine may not support it.');
    return;
  }

  astcSupported = !!gl.getExtension('WEBGL_compressed_texture_astc');
  etcSupported = !!gl.getExtension('WEBGL_compressed_texture_etc1');
  dxtSupported = !!gl.getExtension('WEBGL_compressed_texture_s3tc');
  pvrtcSupported = !!(gl.getExtension('WEBGL_compressed_texture_pvrtc')) || !!(gl.getExtension('WEBKIT_WEBGL_compressed_texture_pvrtc'));

  // Vertex shader program

  const vsSource = `
    attribute vec4 aVertexPosition;
    attribute vec3 aVertexNormal;
    attribute vec3 aTextureCoord;

    uniform mat4 uNormalMatrix;
    uniform mat4 uModelViewMatrix;
    uniform mat4 uProjectionMatrix;
    uniform mat3 uUVMatrix;

    varying highp vec2 vTextureCoord;
    varying highp vec3 vLighting;

    void main(void) {
      gl_Position = uProjectionMatrix * uModelViewMatrix * aVertexPosition;
      //vTextureCoord.x = aTextureCoord.x;
      // Invert Y coordinate to account for PNG top-left origin.
      //vTextureCoord.y = aTextureCoord.y * -1.0 + 1.0;
      vTextureCoord = vec2(uUVMatrix * aTextureCoord);

      // Apply lighting effect

      highp vec3 ambientLight = vec3(0.3, 0.3, 0.3);
      highp vec3 directionalLightColor = vec3(1, 1, 1);
      highp vec3 directionalVector = normalize(vec3(0.85, 0.8, 0.75));

      highp vec4 transformedNormal = uNormalMatrix * vec4(aVertexNormal, 1.0);

      highp float directional = max(dot(transformedNormal.xyz, directionalVector), 0.0);
      vLighting = ambientLight + (directionalLightColor * directional);
    }
  `;

  // Fragment shader program

  const fsSource = `
    varying highp vec2 vTextureCoord;
    varying highp vec3 vLighting;

    uniform sampler2D uSampler;

    highp vec3 srgb_encode(highp vec3 color) {
        highp float r = color.r < 0.0031308 ? 12.92 * color.r : 1.055 * pow(color.r, 1.0/2.4) - 0.055;
        highp float g = color.g < 0.0031308 ? 12.92 * color.g : 1.055 * pow(color.g, 1.0/2.4) - 0.055;
        highp float b = color.b < 0.0031308 ? 12.92 * color.b : 1.055 * pow(color.b, 1.0/2.4) - 0.055;
        return vec3(r, g, b);
    }

    void main(void) {
      highp vec3 vertexColor = vec3(0.9, 0.9, 0.9);
      highp vec4 texelColor = texture2D(uSampler, vTextureCoord);
      highp vec4 fragcolor;
      // DECAL
      fragcolor.rgb = vertexColor.rgb * (1.0 - texelColor.a) + texelColor.rgb * texelColor.a;
      fragcolor.a = texelColor.a;
      fragcolor.rgb *= vLighting;
      fragcolor.rgb = srgb_encode(fragcolor.rgb);
      gl_FragColor = fragcolor;
    }
  `;

  // Initialize a shader program; this is where all the lighting
  // for the vertices and so forth is established.
  const shaderProgram = initShaderProgram(gl, vsSource, fsSource);

  // Collect all the info needed to use the shader program.
  // Look up which attributes our shader program is using
  // for aVertexPosition, aVertexNormal, aTextureCoord,
  // and look up uniform locations.
  const programInfo = {
    program: shaderProgram,
    attribLocations: {
      vertexPosition: gl.getAttribLocation(shaderProgram, 'aVertexPosition'),
      vertexNormal: gl.getAttribLocation(shaderProgram, 'aVertexNormal'),
      textureCoord: gl.getAttribLocation(shaderProgram, 'aTextureCoord'),
    },
    uniformLocations: {
      projectionMatrix: gl.getUniformLocation(shaderProgram, 'uProjectionMatrix'),
      modelViewMatrix: gl.getUniformLocation(shaderProgram, 'uModelViewMatrix'),
      normalMatrix: gl.getUniformLocation(shaderProgram, 'uNormalMatrix'),
      uvMatrix: gl.getUniformLocation(shaderProgram, 'uUVMatrix'),
      uSampler: gl.getUniformLocation(shaderProgram, 'uSampler'),
    },
  };

  // Here's where we call the routine that builds all the
  // objects we'll be drawing.
  const buffers = initBuffers(gl);

  var then = 0;

  // Draw the scene repeatedly
  function render(now) {
    now *= 0.001;  // convert to seconds
    const deltaTime = now - then;
    then = now;

    gl.enable(gl.CULL_FACE);
    gl.enable(gl.DEPTH_TEST);
    gl.enable(gl.SCISSOR_TEST);
    // In case the source image has translucent parts ...
    gl.enable(gl.BLEND);
    gl.blendFunc(gl.SRC_ALPHA, gl.ONE_MINUS_SRC_ALPHA);


    for (const [index, value] of items.entries()) {
      if (index == 0) continue;

      const {element, color, texture} = value;
      const rect = element.getBoundingClientRect(); // Includes padding and border.
      var style = window.getComputedStyle(element, null);

      const borderLeft = parseFloat(style.getPropertyValue("border-left-width"));
      const borderRight = parseFloat(style.getPropertyValue("border-right-width"));
      const borderTop = parseFloat(style.getPropertyValue("border-top-width"));
      const borderBottom = parseFloat(style.getPropertyValue("border-bottom-width"));
      // TODO: Handle padding as well.

      // CSS has 0 padding, 0 border so no need for the above style heroics.
      const cvrect = gl.canvas.getBoundingClientRect();

      const width  = rect.right - borderRight - rect.left - borderLeft;
      const height = rect.bottom - borderBottom - rect.top - borderTop;
      const left   = rect.left + borderLeft - cvrect.left;
      const bottom = cvrect.bottom - rect.bottom + borderBottom;

      if (bottom < 0 || bottom + height  > gl.canvas.clientHeight ||
          left + width  < 0 || left > gl.canvas.clientWidth) {
        continue;  // it's off screen
      }

      gl.viewport(left, bottom, width, height);
      // To limit clearing to the viewport.
      gl.scissor(left, bottom, width, height);
      gl.clearColor(...color);
      // Tell WebGL we want to affect texture unit 0
      gl.activeTexture(gl.TEXTURE0);
      // Bind the texture to texture unit 0
      gl.bindTexture(texture.target, texture.texture);

      // Create a perspective projection matrix.
      // Our field of view is 45 degrees, with a width/height
      // ratio that matches the size of the current item, and
      // we only want to see objects between 0.1 units and
      // 100 units away from the camera.

      const fieldOfView = 45 * Math.PI / 180;   // in radians
      const aspect = width / height;
      const zNear = 0.1;
      const zFar = 100.0;
      const projectionMatrix = mat4.create();

      // note: glmatrix.js always has the first argument
      // as the destination to receive the result.
      mat4.perspective(projectionMatrix,
                       fieldOfView,
                       aspect,
                       zNear,
                       zFar);

      drawScene(gl, projectionMatrix, texture.uvMatrix, programInfo, buffers, texture, deltaTime);
    }
    requestAnimationFrame(render);
  }
  requestAnimationFrame(render);
}

//
// initBuffers
//
// Initialize the buffers we'll need. For this demo, we just
// have one object -- a simple three-dimensional cube.
//
function initBuffers(gl) {

  // Create a buffer for the cube's vertex positions.

  const positionBuffer = gl.createBuffer();

  // Select the positionBuffer as the one to apply buffer
  // operations to from here out.

  gl.bindBuffer(gl.ARRAY_BUFFER, positionBuffer);

  // Now create an array of positions for the cube.

  const positions = [
    // Front face
    -1.0, -1.0,  1.0,
     1.0, -1.0,  1.0,
     1.0,  1.0,  1.0,
    -1.0,  1.0,  1.0,

    // Back face
    -1.0, -1.0, -1.0,
    -1.0,  1.0, -1.0,
     1.0,  1.0, -1.0,
     1.0, -1.0, -1.0,

    // Top face
    -1.0,  1.0, -1.0,
    -1.0,  1.0,  1.0,
     1.0,  1.0,  1.0,
     1.0,  1.0, -1.0,

    // Bottom face
    -1.0, -1.0, -1.0,
     1.0, -1.0, -1.0,
     1.0, -1.0,  1.0,
    -1.0, -1.0,  1.0,

    // Right face
     1.0, -1.0, -1.0,
     1.0,  1.0, -1.0,
     1.0,  1.0,  1.0,
     1.0, -1.0,  1.0,

    // Left face
    -1.0, -1.0, -1.0,
    -1.0, -1.0,  1.0,
    -1.0,  1.0,  1.0,
    -1.0,  1.0, -1.0,
  ];

  // Now pass the list of positions into WebGL to build the
  // shape. We do this by creating a Float32Array from the
  // JavaScript array, then use it to fill the current buffer.

  gl.bufferData(gl.ARRAY_BUFFER, new Float32Array(positions), gl.STATIC_DRAW);

  // Set up the normals for the vertices, so that we can compute lighting.

  const normalBuffer = gl.createBuffer();
  gl.bindBuffer(gl.ARRAY_BUFFER, normalBuffer);

  const vertexNormals = [
    // Front
     0.0,  0.0,  1.0,
     0.0,  0.0,  1.0,
     0.0,  0.0,  1.0,
     0.0,  0.0,  1.0,

    // Back
     0.0,  0.0, -1.0,
     0.0,  0.0, -1.0,
     0.0,  0.0, -1.0,
     0.0,  0.0, -1.0,

    // Top
     0.0,  1.0,  0.0,
     0.0,  1.0,  0.0,
     0.0,  1.0,  0.0,
     0.0,  1.0,  0.0,

    // Bottom
     0.0, -1.0,  0.0,
     0.0, -1.0,  0.0,
     0.0, -1.0,  0.0,
     0.0, -1.0,  0.0,

    // Right
     1.0,  0.0,  0.0,
     1.0,  0.0,  0.0,
     1.0,  0.0,  0.0,
     1.0,  0.0,  0.0,

    // Left
    -1.0,  0.0,  0.0,
    -1.0,  0.0,  0.0,
    -1.0,  0.0,  0.0,
    -1.0,  0.0,  0.0
  ];

  gl.bufferData(gl.ARRAY_BUFFER, new Float32Array(vertexNormals),
                gl.STATIC_DRAW);

  // Now set up the texture coordinates for the faces.

  const textureCoordBuffer = gl.createBuffer();
  gl.bindBuffer(gl.ARRAY_BUFFER, textureCoordBuffer);

  const textureCoordinates = [
    // Front
    0.0,  0.0,
    1.0,  0.0,
    1.0,  1.0,
    0.0,  1.0,
    // Back
    0.0,  0.0,
    1.0,  0.0,
    1.0,  1.0,
    0.0,  1.0,
    // Top
    0.0,  0.0,
    1.0,  0.0,
    1.0,  1.0,
    0.0,  1.0,
    // Bottom
    0.0,  0.0,
    1.0,  0.0,
    1.0,  1.0,
    0.0,  1.0,
    // Right
    0.0,  0.0,
    1.0,  0.0,
    1.0,  1.0,
    0.0,  1.0,
    // Left
    0.0,  0.0,
    1.0,  0.0,
    1.0,  1.0,
    0.0,  1.0,
  ];

  gl.bufferData(gl.ARRAY_BUFFER, new Float32Array(textureCoordinates),
                gl.STATIC_DRAW);

  // Build the element array buffer; this specifies the indices
  // into the vertex arrays for each face's vertices.

  const indexBuffer = gl.createBuffer();
  gl.bindBuffer(gl.ELEMENT_ARRAY_BUFFER, indexBuffer);

  // This array defines each face as two triangles, using the
  // indices into the vertex array to specify each triangle's
  // position.

  const indices = [
    0,  1,  2,      0,  2,  3,    // front
    4,  5,  6,      4,  6,  7,    // back
    8,  9,  10,     8,  10, 11,   // top
    12, 13, 14,     12, 14, 15,   // bottom
    16, 17, 18,     16, 18, 19,   // right
    20, 21, 22,     20, 22, 23,   // left
  ];

  // Now send the element array to GL

  gl.bufferData(gl.ELEMENT_ARRAY_BUFFER,
      new Uint16Array(indices), gl.STATIC_DRAW);

  return {
    position: positionBuffer,
    normal: normalBuffer,
    textureCoord: textureCoordBuffer,
    indices: indexBuffer,
  };
}

function elem(id) {
  return document.getElementById(id);
}

// Upload content of a ktxTexture to WebGL.
// Sets texture parameters suitable for the loaded texture, which requires
// binding and unbinding the texture.
// Returns the created WebGL texture object and texture target.
function uploadTextureToGl(gl, ktexture) {
  const { ktxTexture, TranscodeTarget, OrientationX, OrientationY } = LIBKTX;
  var formatString;

  if (ktexture.needsTranscoding) {
    var format;
    if (astcSupported) {
      formatString = 'ASTC';
      format = TranscodeTarget.ASTC_4x4_RGBA;
    } else if (dxtSupported) {
      formatString = ktexture.numComponents == 4 ? 'BC3' : 'BC1';
      format = TranscodeTarget.BC1_OR_3;
    } else if (pvrtcSupported) {
      formatString = 'PVRTC1';
      format = TranscodeTarget.PVRTC1_4_RGBA;
    } else if (etcSupported) {
      formatString = 'ETC';
      format = TranscodeTarget.ETC;
    } else {
      formatString = 'RGBA4444';
      format = TranscodeTarget.RGBA4444;
    }
    if (ktexture.transcodeBasis(format, 0) != LIBKTX.ErrorCode.SUCCESS) {
        alert('Texture transcode failed. See console for details.');
        return undefined;
    }
//    elem('format').innerText = formatString;
  }

  const result = ktexture.glUpload();
  const {target, error} = result;
  const texture = result.texture;
  if (error != gl.NO_ERROR) {
    alert('WebGL error when uploading texture, code = ' + error.toString(16));
    return undefined;
  }
  if (texture === undefined) {
    alert('Texture upload failed. See console for details.');
    return undefined;
  }
  if (target != gl.TEXTURE_2D) {
    alert('Loaded texture is not a TEXTURE2D.');
    return undefined;
  }

  gl.bindTexture(target, texture);

  if (ktexture.numLevels > 1 || ktexture.generateMipmaps)
     // Enable bilinear mipmapping.
     gl.texParameteri(target,
                      gl.TEXTURE_MIN_FILTER, gl.LINEAR_MIPMAP_NEAREST);
  else
    gl.texParameteri(target, gl.TEXTURE_MIN_FILTER, gl.LINEAR);
  gl.texParameteri(target, gl.TEXTURE_MAG_FILTER, gl.LINEAR);

  //gl.bindTexture(target);

  return {
    texture: texture,
    target: target,
    format: formatString
  }
}

function createPlaceholderTexture(gl, color)
{
//  // Must create texture via Emscripten so it knows of it.
//  var texName;
//  LIBKTX.GL._glGenTextures(1, texName);
//  texture = LIBKTX.GL.textures[texName];
  // Since it doesn't seem possible to get the above to work
  // use a placeholder texture object to hold the temporary
  // image.
  const placeholder = gl.createTexture();
  gl.bindTexture(gl.TEXTURE_2D, placeholder);

  const level = 0;
  const internalFormat = gl.RGBA;
  const width = 1;
  const height = 1;
  const border = 0;
  const srcFormat = gl.RGBA;
  const srcType = gl.UNSIGNED_BYTE;
  const pixel = new Uint8Array(color);
  //const pixel = new Uint8Array([0, 0, 255, 255]);  // opaque blue

  gl.texImage2D(gl.TEXTURE_2D, level, internalFormat,
                width, height, border, srcFormat, srcType,
                pixel);
  return {
    target: gl.TEXTURE_2D,
    texture: placeholder,
    uvMatrix: mat3.create()
  };
}

function loadTexture(gl, url)
{
  // Because images have to be downloaded over the internet
  // they might take a moment until they are ready. Until
  // then temporarily fill the texture with a single pixel image
  // so we can use it immediately. When the image has finished
  // downloading we'll update texture to the new contents

//  // Must create texture via Emscripten so it knows of it.
//  var texName;
//  LIBKTX.GL._glGenTextures(1, texName);
//  texture = LIBKTX.GL.textures[texName];
  // Since it doesn't seem possible to get the above to work
  // use a placeholder texture object to hold the temporary
  // image.
  const placeholder = gl.createTexture();
  gl.bindTexture(gl.TEXTURE_2D, placeholder);

  const level = 0;
  const internalFormat = gl.RGBA;
  const width = 1;
  const height = 1;
  const border = 0;
  const srcFormat = gl.RGBA;
  const srcType = gl.UNSIGNED_BYTE;
  const pixel = new Uint8Array([0, 0, 255, 255]);  // opaque blue
  gl.texImage2D(gl.TEXTURE_2D, level, internalFormat,
                width, height, border, srcFormat, srcType,
                pixel);

  var xhr = new XMLHttpRequest();
  xhr.open('GET', url);
  xhr.responseType = "arraybuffer";
  xhr.onload = function(){
    const { ktxTexture, TranscodeTarget, OrientationX, OrientationY } = LIBKTX;
    var ktxdata = new Uint8Array(this.response);
    ktexture = new ktxTexture(ktxdata);

    const result = uploadTextureToGl(gl, ktexture);
    const { target, format } = result;

    defaultTexture = result.texture;
    items[2].texture = result.texture;
    items[2].target = target;
    gl.bindTexture(target, defaultTexture);
    gl.deleteTexture(placeholder);

    if (ktexture.orientation.x == OrientationX.LEFT) {
        mat3.translate(uvMatrix, uvMatrix, [1.0, 0.0]);
        mat3.scale(uvMatrix, uvMatrix, [-1.0, 1.0]);
    }
    if (ktexture.orientation.y == OrientationY.DOWN) {
        mat3.translate(uvMatrix, uvMatrix, [0.0, 1.0]);
        mat3.scale(uvMatrix, uvMatrix, [1.0, -1.0]);
    }
    elem('format').innerText = format;
    ktexture.delete();
  };
  //xhr.onprogress = runProgress;
  //xhr.onloadstart = openProgress;
  xhr.send();

  return {
    target: gl.TEXTURE_2D,
    texture: placeholder
  };
}

function isPowerOf2(value) {
  return (value & (value - 1)) == 0;
}

//
// Draw the scene.
//
function drawScene(gl, projectionMatrix, uvMatrix, programInfo, buffers, texture, deltaTime) {
  gl.clearDepth(1.0);                 // Clear everything
  gl.depthFunc(gl.LEQUAL);            // Near things obscure far things

  // Clear the canvas before we start drawing on it.

  gl.clear(gl.COLOR_BUFFER_BIT | gl.DEPTH_BUFFER_BIT);

  // Set the drawing position to the "identity" point, which is
  // the center of the scene.
  const modelViewMatrix = mat4.create();

  // Now move the drawing position a bit to where we want to
  // start drawing the square.

  mat4.translate(modelViewMatrix,     // destination matrix
                 modelViewMatrix,     // matrix to translate
                 [-0.0, 0.0, -6.0]);  // amount to translate
  mat4.rotate(modelViewMatrix,  // destination matrix
              modelViewMatrix,  // matrix to rotate
              cubeRotation,     // amount to rotate in radians
              [0, 0, 1]);       // axis to rotate around (Z)
  mat4.rotate(modelViewMatrix,  // destination matrix
              modelViewMatrix,  // matrix to rotate
              cubeRotation * .7,// amount to rotate in radians
              [0, 1, 0]);       // axis to rotate around (X)

  const normalMatrix = mat4.create();
  mat4.invert(normalMatrix, modelViewMatrix);
  mat4.transpose(normalMatrix, normalMatrix);

  // Tell WebGL how to pull out the positions from the position
  // buffer into the vertexPosition attribute
  {
    const numComponents = 3;
    const type = gl.FLOAT;
    const normalize = false;
    const stride = 0;
    const offset = 0;
    gl.bindBuffer(gl.ARRAY_BUFFER, buffers.position);
    gl.vertexAttribPointer(
        programInfo.attribLocations.vertexPosition,
        numComponents,
        type,
        normalize,
        stride,
        offset);
    gl.enableVertexAttribArray(
        programInfo.attribLocations.vertexPosition);
  }

  // Tell WebGL how to pull out the texture coordinates from
  // the texture coordinate buffer into the textureCoord attribute.
  {
    const numComponents = 2;
    const type = gl.FLOAT;
    const normalize = false;
    const stride = 0;
    const offset = 0;
    gl.bindBuffer(gl.ARRAY_BUFFER, buffers.textureCoord);
    gl.vertexAttribPointer(
        programInfo.attribLocations.textureCoord,
        numComponents,
        type,
        normalize,
        stride,
        offset);
    gl.enableVertexAttribArray(
        programInfo.attribLocations.textureCoord);
  }

  // Tell WebGL how to pull out the normals from
  // the normal buffer into the vertexNormal attribute.
  {
    const numComponents = 3;
    const type = gl.FLOAT;
    const normalize = false;
    const stride = 0;
    const offset = 0;
    gl.bindBuffer(gl.ARRAY_BUFFER, buffers.normal);
    gl.vertexAttribPointer(
        programInfo.attribLocations.vertexNormal,
        numComponents,
        type,
        normalize,
        stride,
        offset);
    gl.enableVertexAttribArray(
        programInfo.attribLocations.vertexNormal);
  }

  // Tell WebGL which indices to use to index the vertices
  gl.bindBuffer(gl.ELEMENT_ARRAY_BUFFER, buffers.indices);

  // Tell WebGL to use our program when drawing

  gl.useProgram(programInfo.program);

  // Set the shader uniforms

  gl.uniformMatrix4fv(
      programInfo.uniformLocations.projectionMatrix,
      false,
      projectionMatrix);
  gl.uniformMatrix4fv(
      programInfo.uniformLocations.modelViewMatrix,
      false,
      modelViewMatrix);
  gl.uniformMatrix4fv(
      programInfo.uniformLocations.normalMatrix,
      false,
      normalMatrix);
  gl.uniformMatrix3fv(
    programInfo.uniformLocations.uvMatrix,
    false,
    uvMatrix);

  // Tell the shader we bound the texture to texture unit 0
  gl.uniform1i(programInfo.uniformLocations.uSampler, 0);

  {
    const vertexCount = 36;
    const type = gl.UNSIGNED_SHORT;
    const offset = 0;
    gl.drawElements(gl.TRIANGLES, vertexCount, type, offset);
  }

  // Update the rotation for the next draw

  cubeRotation += deltaTime;
}

//
// Initialize a shader program, so WebGL knows how to draw our data
//
function initShaderProgram(gl, vsSource, fsSource) {
  const vertexShader = loadShader(gl, gl.VERTEX_SHADER, vsSource);
  const fragmentShader = loadShader(gl, gl.FRAGMENT_SHADER, fsSource);

  // Create the shader program

  const shaderProgram = gl.createProgram();
  gl.attachShader(shaderProgram, vertexShader);
  gl.attachShader(shaderProgram, fragmentShader);
  gl.linkProgram(shaderProgram);

  // If creating the shader program failed, alert

  if (!gl.getProgramParameter(shaderProgram, gl.LINK_STATUS)) {
    alert('Unable to initialize the shader program: ' + gl.getProgramInfoLog(shaderProgram));
    return null;
  }

  return shaderProgram;
}

//
// creates a shader of the given type, uploads the source and
// compiles it.
//
function loadShader(gl, type, source) {
  const shader = gl.createShader(type);

  // Send the source to the shader object

  gl.shaderSource(shader, source);

  // Compile the shader program

  gl.compileShader(shader);

  // See if it compiled successfully

  if (!gl.getShaderParameter(shader, gl.COMPILE_STATUS)) {
    alert('An error occurred compiling the shaders: ' + gl.getShaderInfoLog(shader));
    gl.deleteShader(shader);
    return null;
  }

  return shader;
}

async function updateItem(item, ktexture, texture, target) {
  const { ktxTexture, ktxBasisParams, SupercmpScheme, TranscodeTarget, OrientationX, OrientationY } = LIBKTX;

  gl.deleteTexture(item.texture.texture);
  item.texture.target = target;
  item.texture.texture = texture;
  uvMatrix = mat3.create();
  if (ktexture.orientation.x == OrientationX.LEFT) {
      mat3.translate(uvMatrix, uvMatrix, [1.0, 0.0]);
      mat3.scale(uvMatrix, uvMatrix, [-1.0, 1.0]);
  }
  if (ktexture.orientation.y == OrientationY.DOWN) {
      mat3.translate(uvMatrix, uvMatrix, [0.0, 1.0]);
      mat3.scale(uvMatrix, uvMatrix, [1.0, -1.0]);
  }
  item.texture.uvMatrix = uvMatrix;
}

async function encode(raw_data, width, height) {
//  LIBKTX().then(function(Module) {
    //const texture = new Module.ktxTexture(raw_data);
    const { ktxTexture, ktxBasisParams, SupercmpScheme, TranscodeTarget, OrientationX, OrientationY } = LIBKTX;
    const basisu_options = new ktxBasisParams();
    const ktexture = new ktxTexture(raw_data, width, height, 4 /* components */, true/* srgb */);

    const {target, texture} = uploadTextureToGl(gl, ktexture);
    updateItem(items[1], ktexture, texture, target);

    basisu_options.uastc = false;
    basisu_options.noSSE = true;
    basisu_options.verbose = false;
    basisu_options.qualityLevel = 200;
    basisu_options.compressionLevel = 2;

    const result = ktexture.compressBasisU(basisu_options, SupercmpScheme.BASIS_LZ, 0 /*for zlibv or zstd*/);
    // result is a KTX file in memory with the compressed image.
    // Not needed when calling upload but presence indicates
    // compression was successful.
    if (result) {
        const {target, texture, format} = uploadTextureToGl(gl, ktexture);
        updateItem(items[2], ktexture, texture, target);
        elem('format').innerText = format;
    }
    ktexture.delete();

    console.log(result);

// });
  return;
}


async function loadImageData (img, flip = false) {
  const canvas    = document.createElement("canvas");
  const context   = canvas.getContext("2d");
  canvas.height = img.height;
  canvas.width  = img.width;

  if (flip) {
    context.translate(0, img.height);
    context.scale(1, -1);
  }
  context.drawImage(img, 0, 0, img.width, img.height);

  const rgba = context.getImageData(0, 0, img.width, img.height).data;
  return rgba;
};

async function loadImage(src){
  return new Promise((resolve, reject) => {
    let img = new Image();
    div = items[0].element;
    //img.onload = () => resolve(img);
    img.onload = () => { div.appendChild(img); resolve(img); }
    img.onerror = reject;
    img.src = src;
  })
}

async function decodeFile(filename) {
  const img = await loadImage(filename);
  const img_data = await loadImageData(img);
  console.log(img);
  console.log(img_data);

  encode(img_data, img.width, img.height);
}

