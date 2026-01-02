/*
 * Copyright 2015-2020 MDN Contributors
 * Copyright 2024 Mark Callow
 * SPDX-License-Identifier: CC0-1.0
 */

/*
This code originated from sample 7 in the MDN WebGL examples
at https://github.com/mdn/webgl-examples which is licensed
under Creative Commons Zero v1.0 Universal. It has been
extensively modified to create a test for the JS wrapper
for libktx. Modifications are also licensed under CC0v1.
*/

var cubeRotation = 0.0;
var gl;

var astcSupported = false;
var etcSupported = false;
var dxtSupported = false;
var pvrtcSupported = false;

//
// Start here
//
const canvas = document.querySelector('#glcanvas');
gl = canvas.getContext('webgl2');

// If we don't have a GL context, give up now
if (!gl) {
  alert('Unable to initialize WebGL. Your browser or machine may not support it.');
} else {
  createKtxModule({preinitializedWebGLContext: gl}).then(instance => {
    window.ktx = instance;
    // Make existing WebGL context current for Emscripten OpenGL.
    ktx.GL.makeContextCurrent(
                ktx.GL.createContext(document.getElementById("glcanvas"),
                                        { majorVersion: 2.0 })
                );
    main()
  });
}

function main() {

  elem('runtests').disabled = false;

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

  function resizeCanvasToDisplaySize(canvas) {
    // Lookup the size the browser is displaying the canvas in CSS pixels.
    const displayWidth  = canvas.clientWidth;
    const displayHeight = canvas.clientHeight;

    // Check if the canvas is not the same size.
    const needResize = canvas.width  !== displayWidth ||
                       canvas.height !== displayHeight;

    if (needResize) {
      // Make the canvas the same size
      canvas.width  = displayWidth;
      canvas.height = displayHeight;
    }

      return needResize;
  }

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

    resizeCanvasToDisplaySize(gl.canvas);

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
      gl.bindTexture(texture.target, texture.object);

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
//
// Returns the created WebGL texture object and texture target.
function uploadTextureToGl(gl, ktexture) {
  const { transcode_fmt  } = ktx;
  var formatString;

  if (ktexture.needsTranscoding) {
    var format;
    if (astcSupported) {
      formatString = 'ASTC';
      format = transcode_fmt.ASTC_4x4_RGBA;
    } else if (dxtSupported) {
      formatString = ktexture.numComponents == 4 ? 'BC3' : 'BC1';
      format = transcode_fmt.BC1_OR_3;
    } else if (pvrtcSupported) {
      formatString = 'PVRTC1';
      format = transcode_fmt.PVRTC1_4_RGBA;
    } else if (etcSupported) {
      formatString = 'ETC';
      format = transcode_fmt.ETC;
    } else {
      formatString = 'RGBA4444';
      format = transcode_fmt.RGBA4444;
    }
    if (ktexture.transcodeBasis(format, 0) != ktx.error_code.SUCCESS) {
        alert('Texture transcode failed. See console for details.');
        return undefined;
    }
  }

  const result = ktexture.glUpload();
  if (result.error != gl.NO_ERROR) {
    alert('WebGL error when uploading texture, code = '
          + result.error.toString(16));
    return undefined;
  }
  if (result.object === undefined) {
    alert('Texture upload failed. See console for details.');
    return undefined;
  }
  if (result.target != gl.TEXTURE_2D) {
    alert('Loaded texture is not a TEXTURE2D.');
    return undefined;
  }

  return {
    target: result.target,
    object: result.object,
    format: formatString,
    uvMatrix: null
  }
}

function createPlaceholderTexture(gl, color)
{
//  // Must create texture via Emscripten so it knows of it.
//  var texName;
//  ktx.GL._glGenTextures(1, texName);
//  texture = ktx.GL.textures[texName];
  // Since it doesn't seem possible to get the above to work
  // use a placeholder WebGLTexture object to hold the temporary
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

  gl.texImage2D(gl.TEXTURE_2D, level, internalFormat,
                width, height, border, srcFormat, srcType,
                pixel);
  return {
    target: gl.TEXTURE_2D,
    object: placeholder,
    format: "",
    uvMatrix: mat3.create()
  };
}

async function setUVMatrix(texture, inMatrix, ktexture) {
  texture.uvMatrix = inMatrix;
  if (ktexture.orientation.x == ktx.OrientationX.LEFT) {
      mat3.translate(texture.uvMatrix, texture.uvMatrix, [1.0, 0.0]);
      mat3.scale(texture.uvMatrix, texture.uvMatrix, [-1.0, 1.0]);
  }
  if (ktexture.orientation.y == ktx.OrientationY.DOWN) {
      mat3.translate(texture.uvMatrix, texture.uvMatrix, [0.0, 1.0]);
      mat3.scale(texture.uvMatrix, texture.uvMatrix, [1.0, -1.0]);
  }
}

//
// Binds a texture and sets suitable texture parameters.
//
// The WebGL texture object is expected to have been created from the
// content of the ktxTexture object.
//
function setTexParameters(texture, ktexture) {
  gl.bindTexture(texture.target, texture.object);

  if (ktexture.numLevels > 1 || ktexture.generateMipmaps) {
     // Enable bilinear mipmapping.
     gl.texParameteri(texture.target,
                      gl.TEXTURE_MIN_FILTER, gl.LINEAR_MIPMAP_NEAREST);
  } else {
    gl.texParameteri(texture.target, gl.TEXTURE_MIN_FILTER, gl.LINEAR);
  }
  gl.texParameteri(texture.target, gl.TEXTURE_MAG_FILTER, gl.LINEAR);

  gl.bindTexture(texture.target, null);
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

async function updateItem(item, texture) {
  gl.deleteTexture(item.texture.object);
  item.texture = texture;
}

function arraysEqual(a, b) {
  if (a.length != b.length)
    return false;
  for (let i = 0; i < a.length; i++)
    if (a[i] != b[i]) return false;
  return true;
}

function showTestResult(id, pass) {
  element =elem(id);
  if (pass) {
      element.innerText = "PASSED";
      element.className = "pass";
  } else {
      element.innerText = "FAILED.";
      element.className = "fail";
  }
}

// Test creation of a ktxTexture2 from the provided imageData.
async function testCreate(imageData) {
  const createInfo = new ktx.textureCreateInfo();
  const colorSpace = imageData.colorSpace;

  createInfo.baseWidth = imageData.width;
  createInfo.baseHeight = imageData.height;
  createInfo.baseDepth = 1;
  createInfo.numDimensions = 2;
  createInfo.numLevels = 1;
  createInfo.numLayers = 1;
  createInfo.numFaces = 1;
  createInfo.isArray = false;
  createInfo.generateMipmaps = false;

  var displayP3;
  // Image data from 2d canvases is always 8-bit RGBA.
  // The only possible ImageData colorSpace choices are undefined, "srgb"
  // and "displayp3." All use the sRGB transfer function.
  createInfo.vkFormat = ktx.VkFormat.R8G8B8A8_SRGB;
  if ( imageData.colorSpace == "display-p3") {
    displayP3 = true;
  }

  const ktexture = new ktx.texture(createInfo,
                                   ktx.TextureCreateStorageEnum.ALLOC_STORAGE);
  showTestResult('create_result', ktexture != null);
  if (ktexture != null) {
    if (displayP3) {
        ktexture.primaries = ktx.khr_df_primaries.DISPLAYP3;
    }
    // Check DFD settings. Oetf should be SRGB due to SRGB vkFormat.
    // Primaries default to BT709 unless set to P3 above.
    const expectedPrimaries = displayP3 ? ktx.khr_df_primaries.DISPLAYP3
                                        : ktx.khr_df_primaries.BT709;
    const oetf = ktexture.oetf;
    const primaries = ktexture.primaries;
    // Do not know how to compare without using .value as all these
    // enumerators are objects.
    showTestResult('colorspace_set_result',
                   ( oetf.value == ktx.khr_df_transfer.SRGB.value
                    && primaries.value == expectedPrimaries.value));

    result = ktexture.setImageFromMemory(0, 0, 0, imageData.data);
    showTestResult('copy_image_result', result == ktx.error_code.SUCCESS);
  }
  return ktexture;
}

function testWriteReadMetadata(ktexture) {
  const writer = "libktx-js-test";
  const orientation = "rd";
  ktexture.addKVPairString(ktx.ORIENTATION_KEY, orientation);
  ktexture.addKVPairString(ktx.WRITER_KEY, writer);

  var textDecoder = new TextDecoder();
  var value = ktexture.findKeyValue(ktx.WRITER_KEY);
  // subarray to remove the terminating null we know is there.
  var string = textDecoder.decode(value.subarray(0,value.byteLength-1));
  var passed = true;
  //console.log(string);
  if (!writer.localeCompare(string)) {
    value = ktexture.findKeyValue(ktx.ORIENTATION_KEY);
    string = textDecoder.decode(value.subarray(0,value.byteLength-1));
    //console.log(string);
    if (orientation.localeCompare(string)) {
      passed = false;
    }
  } else {
    passed = false;
  }

  if (passed) {
    // Test passing of array-valued metadata between JS & C++.
    var animData = new Uint32Array(3);
    animData[0] = 20;     // duration
    animData[1] = 1000;   // timescale
    animData[2] = 10;     // loopCount
    // In real use the data should be endian-converted when on a
    // big-endian machine as the KTX v2 spec. requires these values
    // be little-endian. For the purpose of this test, it is not
    // necessary as the data is not used.
    ktexture.addKVPairByte(ktx.ANIMDATA_KEY, animData);
    value = ktexture.findKeyValue(ktx.ANIMDATA_KEY);
    if (value != null) {
      // In real use, data needs to be endian-converted when on a
      // big-endian machine.
      const retData = new Uint32Array(value.buffer, value.byteOffset,
                                      value.length / 4);
      if (retData[0] != 20 || retData[1] != 1000 || retData[2] != 10)
        passed = false;
    } else {
      passed = false;
    }
    // Since ktexture is not an array texture presence of AnimData
    // will cause the a load to fail So delete the AnimData.
    ktexture.deleteKVPair(ktx.ANIMDATA_KEY);
  }
  showTestResult('metadata_result', passed);
}

async function testGetImage(ktexture, imageData) {
  var passed = true;

  result = ktexture.getImage(0, 0, 0);
  if (result != null) {
    passed = arraysEqual(result, imageData.data);
  } else {
    passed = false;
  }
  showTestResult('get_image_result', passed);
}

async function testEncodeBasis(ktexture) {
  const basisu_options = new ktx.basisParams();

  basisu_options.uastc = false;
  basisu_options.noSSE = true;
  basisu_options.verbose = false;
  basisu_options.qualityLevel = 200;
  basisu_options.compressionLevel = ktx.ETC1S_DEFAULT_COMPRESSION_LEVEL;

  var result = ktexture.compressBasis(basisu_options);

  showTestResult('compress_basis_result', result == ktx.error_code.SUCCESS);
}

async function testEncodeAstc(ktexture) {
  const params = new ktx.astcParams();

  params.blockDimension = ktx.pack_astc_block_dimension.D8x8;
  params.mode = ktx.pack_astc_encoder_mode.DEFAULT;
  params.qualityLevel = ktx.pack_astc_quality_levels.FAST;
  params.normalMap = false;

  // Before we compress, test inputSwizzle setting
  params.inputSwizzle = 'rrrg';
  showTestResult('swizzle_set_result',
                  params.inputSwizzle.localeCompare('rrrg') == 0); 
  params.inputSwizzle = ''; // Reset to default.

  var result = ktexture.compressAstc(params);
  showTestResult('compress_astc_result', result == ktx.error_code.SUCCESS);
}

async function testCreateCopy(ktexture) {
  const copy = ktexture.createCopy();
  showTestResult('create_copy_result', copy != null);
  return copy;
}

async function testWriteToMemoryAndRead(ktexture) {
  // result is a KTX file in memory with the compressed image.
  result = ktexture.writeToMemory();
  // TODO: Check the first bytes are a KTX header.
  showTestResult('write_to_memory_result', result != null);

  readKTexture = new ktx.texture(result);
  showTestResult('read_from_memory_result', result != null);
  const readTexture = await uploadTextureToGl(gl, readKTexture);
  setUVMatrix(readTexture, mat3.create(), readKTexture);
  setTexParameters(readTexture, readKTexture);
  updateItem(items[writeReadTextureItem], readTexture);
}

async function loadImageData (img, flip = false) {
  const canvas    = document.createElement("canvas");
  const context   = canvas.getContext("2d");
  // These values draw the full image which gives a better starting
  // point for compression.
  //const width = img.naturalWidth;
  //const height = img.naturalHeight;
  // These values draw the image at the size displayed on the web
  // page which, per CSS, is currently 256px giving a faster result
  // which saves time for testing.
  const width = img.width;
  const height = img.height;
  canvas.width  = width;
  canvas.height = height;

  if (flip) {
    context.translate(0, height);
    context.scale(1, -1);
  }
  context.drawImage(img, 0, 0, width, height);

  const imageData = context.getImageData(0, 0, width, height);
  return imageData;
};

async function loadImage(src){
  return new Promise((resolve, reject) => {
    let img = new Image();
    div = items[origImageItem].element;
    img.onload = () => { div.appendChild(img); resolve(img); }
    img.onerror = reject;
    img.src = src;
  })
}

async function runTests(filename) {
    const img = await loadImage(filename);
    const imageData = await loadImageData(img);
    console.log(img);
    console.log(imageData);

    const ktexture = await testCreate(imageData);
    if (ktexture == null)
      return;

    testWriteReadMetadata(ktexture);

    const texture = await uploadTextureToGl(gl, ktexture);
    setUVMatrix(texture, mat3.create(), ktexture);
    setTexParameters(texture, ktexture);
    updateItem(items[uncompTextureItem], texture);

    await testGetImage(ktexture, imageData);

    const ktextureCopy = await testCreateCopy(ktexture);
    const textureCopy = uploadTextureToGl(gl, ktextureCopy);
    setUVMatrix(textureCopy, mat3.create(), ktextureCopy);
    updateItem(items[copyTextureItem], textureCopy);

    await testEncodeBasis(ktexture);
    textureComp = uploadTextureToGl(gl, ktexture);
    // upload transcodes the texture so ktexture is uncompresssed again.
    setUVMatrix(textureComp, mat3.create(), ktexture);
    setTexParameters(texture, ktexture);
    updateItem(items[basisCompTextureItem], textureComp);
    items[basisCompTextureItem].label.textContent +=
               ", transcoded to " + textureComp.format;

    await testWriteToMemoryAndRead(ktexture)

    await testEncodeAstc(ktextureCopy);
    if (!astcSupported) {
      var result = ktextureCopy.decodeAstc();
      showTestResult('compress_astc_result', result == ktx.error_code.SUCCESS);
      items[astcCompTextureItem].label.textContent +=
                 ". (Software decoded. This device does not support WEBGL_compressed_texture_astc)"
    }
    textureAstc = uploadTextureToGl(gl, ktextureCopy);
    setUVMatrix(textureAstc, mat3.create(), ktextureCopy);
    setTexParameters(texture, ktexture);
    updateItem(items[astcCompTextureItem], textureAstc);

    elem('runtests').disabled = true;

}

