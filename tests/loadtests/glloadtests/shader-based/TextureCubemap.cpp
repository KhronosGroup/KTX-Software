/* -*- tab-width: 4; -*- */
/* vi: set sw=2 ts=4 expandtab: */

/*
 * Copyright 2020 Mark Callow.
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @internal
 * @file TextureCubemap.cpp
 * @~English
 *
 * @brief Test loading of 2D texture arrays.
 *
 * @author Mark Callow, www.edgewise-consulting.com.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <algorithm>
#include <time.h>
#include <sstream>
#include <vector>
#include <ktx.h>

#include "argparser.h"
#include "TextureCubemap.h"
#include "GLTextureTranscoder.hpp"
#include "ltexceptions.h"

#define member_size(type, member) sizeof(((type *)0)->member)

const GLchar* pszReflectFs =
    "precision highp float;"
    "uniform UBO\n"
    "{\n"
    "  mat4 projection;\n"
    "  mat4 modelView;\n"
    "  mat4 skyboxView;\n"
    "  mat4 invModelView;\n"
    "  mat4 uvwTransform;\n"
    "  float lodBias;\n"
    "} ubo;\n\n"

    "uniform samplerCube uSamplerColor;\n\n"

    "in vec3 vPos;\n"
    "in vec3 vNormal;\n"
    "in float vLodBias;\n"
    "in vec3 vViewVec;\n"
    "in vec3 vLightVec;\n\n"

    "layout (location = 0) out vec4 outFragColor;\n\n"

    "void main()\n"
    "{\n"
    "  vec3 cI = normalize (vPos);\n"
    "  vec3 cR = reflect (cI, normalize(vNormal));\n\n"

    "  cR = vec3(ubo.uvwTransform * ubo.invModelView * vec4(cR, 0.0));\n\n"

    "  vec4 color = texture(uSamplerColor, cR, vLodBias);\n\n"

    "  vec3 N = normalize(vNormal);\n"
    "  vec3 L = normalize(vLightVec);\n"
    "  vec3 V = normalize(vViewVec);\n"
    "  vec3 R = reflect(-L, N);\n"
    "  vec3 ambient = vec3(0.5) * color.rgb;\n"
    "  vec3 diffuse = max(dot(N, L), 0.0) * vec3(1.0);\n"
    "  vec3 specular = pow(max(dot(R, V), 0.0), 16.0) * vec3(0.5);\n"
    "  outFragColor = vec4(ambient + diffuse * color.rgb + specular, 1.0);\n"
    "}\n";

const GLchar* pszReflectSrgbEncodeFs =
    "precision highp float;"
    "uniform UBO\n"
    "{\n"
    "  mat4 projection;\n"
    "  mat4 modelView;\n"
    "  mat4 skyboxView;\n"
    "  mat4 invModelView;\n"
    "  mat4 uvwTransform;\n"
    "  float lodBias;\n"
    "} ubo;\n\n"

    "uniform samplerCube uSamplerColor;\n\n"

    "in vec3 vPos;\n"
    "in vec3 vNormal;\n"
    "in float vLodBias;\n"
    "in vec3 vViewVec;\n"
    "in vec3 vLightVec;\n\n"

    "layout (location = 0) out vec4 outFragColor;\n\n"

    "vec3 srgb_encode(vec3 color) {\n"
    "   float r = color.r < 0.0031308 ? 12.92 * color.r : 1.055 * pow(color.r, 1.0/2.4) - 0.055;\n"
    "   float g = color.g < 0.0031308 ? 12.92 * color.g : 1.055 * pow(color.g, 1.0/2.4) - 0.055;\n"
    "   float b = color.b < 0.0031308 ? 12.92 * color.b : 1.055 * pow(color.b, 1.0/2.4) - 0.055;\n"
    "   return vec3(r, g, b);\n"
    "}\n\n"

    "void main()\n"
    "{\n"
    "  vec3 cI = normalize (vPos);\n"
    "  vec3 cR = reflect (cI, normalize(vNormal));\n\n"

    "  cR = vec3(ubo.uvwTransform * ubo.invModelView * vec4(cR, 0.0));\n\n"

    "  vec4 color = texture(uSamplerColor, cR, vLodBias);\n\n"

    "  vec3 N = normalize(vNormal);\n"
    "  vec3 L = normalize(vLightVec);\n"
    "  vec3 V = normalize(vViewVec);\n"
    "  vec3 R = reflect(-L, N);\n"
    "  vec3 ambient = vec3(0.5) * color.rgb;\n"
    "  vec3 diffuse = max(dot(N, L), 0.0) * vec3(1.0);\n"
    "  vec3 specular = pow(max(dot(R, V), 0.0), 16.0) * vec3(0.5);\n"
    "  color.rgb = srgb_encode(ambient + diffuse * color.rgb + specular);\n"
    "  outFragColor = vec4(color.rgb, 1.0);\n"
    "}\n";

const GLchar* pszReflectVs =
    "precision highp float;"
    "layout (location = 0) in vec3 inPos;\n"
    "layout (location = 1) in vec3 inNormal;\n\n"

    "uniform UBO\n"
    "{\n"
    "  mat4 projection;\n"
    "  mat4 modelView;\n"
    "  mat4 skyboxView;\n"
    "  mat4 invModelView;\n"
    "  mat4 uvwTransform;\n"
    "  float lodBias;\n"
    "} ubo;\n"
    "\n"
    "out vec3 vPos;\n"
    "out vec3 vNormal;\n"
    "out float vLodBias;\n"
    "out vec3 vViewVec;\n"
    "out vec3 vLightVec;\n\n"

    "void main()\n"
    "{\n"
    "  gl_Position = ubo.projection * ubo.modelView * vec4(inPos, 1.0);\n\n"

    "  vPos = vec3(ubo.modelView * vec4(inPos, 1.0));\n"
    "  vNormal = mat3(ubo.modelView) * inNormal;\n"
    "  vLodBias = ubo.lodBias;\n\n"

    "  vec3 lightPos = vec3(0.0f, -5.0f, 5.0f);\n"
    "  vLightVec = lightPos.xyz - vPos.xyz;\n"
    "  vViewVec = -vPos.xyz;\n"
    "}\n";

const GLchar* pszSkyboxFs =
    "precision highp float;"
    "uniform samplerCube uSamplerColor;\n\n"

    "in vec3 vUVW;\n\n"

    "layout (location = 0) out vec4 outFragColor;\n\n"

    "void main()\n"
    "{\n"
      "  outFragColor = texture(uSamplerColor, vUVW);\n"
    "}\n";

const GLchar* pszSkyboxSrgbEncodeFs =
    "precision highp float;"
    "uniform samplerCube uSamplerColor;\n\n"

    "in vec3 vUVW;\n\n"

    "layout (location = 0) out vec4 outFragColor;\n\n"

    "vec3 srgb_encode(vec3 color) {\n"
    "   float r = color.r < 0.0031308 ? 12.92 * color.r : 1.055 * pow(color.r, 1.0/2.4) - 0.055;\n"
    "   float g = color.g < 0.0031308 ? 12.92 * color.g : 1.055 * pow(color.g, 1.0/2.4) - 0.055;\n"
    "   float b = color.b < 0.0031308 ? 12.92 * color.b : 1.055 * pow(color.b, 1.0/2.4) - 0.055;\n"
    "   return vec3(r, g, b);\n"
    "}\n\n"

    "void main()\n"
    "{\n"
    "  vec4 color = texture(uSamplerColor, vUVW);\n"
    "  outFragColor.rgb = srgb_encode(color.rgb);\n"
    "  outFragColor.a = color.a;\n"
    "}\n";

const GLchar* pszSkyboxVs =
    "precision highp float;"
    "layout (location = 0) in vec3 inPos;\n\n"

    "uniform UBO\n"
    "{\n"
    "  mat4 projection;\n"
    "  mat4 modelView;\n"
    "  mat4 skyboxView;\n"
    "  mat4 invModelView;\n"
    "  mat4 uvwTransform;\n"
    "} ubo;\n\n"

    "out vec3 vUVW;\n\n"

    "void main()\n"
    "{\n"
    "  vUVW = (ubo.uvwTransform * vec4(inPos.xyz, 1.0)).xyz;\n"
    "  //vUVW = inPos.xyz;\n"
    "  gl_Position = (ubo.projection * ubo.skyboxView * vec4(inPos.xyz, 1.0)).xyww;\n"
    "}\n";

/* ------------------------------------------------------------------------- */

// Vertex layout for this example
std::vector<glMeshLoader::VertexLayout> vertexLayout =
{
    glMeshLoader::VERTEX_LAYOUT_POSITION,
    glMeshLoader::VERTEX_LAYOUT_NORMAL,
    glMeshLoader::VERTEX_LAYOUT_UV
};

LoadTestSample*
TextureCubemap::create(uint32_t width, uint32_t height,
                     const char* const szArgs, const std::string sBasePath)
{
    return new TextureCubemap(width, height, szArgs, sBasePath);
}

/**
 * @internal
 * @class TextureCubemap
 * @~English
 *
 * @brief Test loading of 2D texture arrays.
 */
TextureCubemap::TextureCubemap(uint32_t width, uint32_t height,
                           const char* const szArgs,
                           const std::string sBasePath)
        : GL3LoadTestSample(width, height, szArgs, sBasePath),
          cubemapTexUnit(GL_TEXTURE0), uniformBufferBindId(0),
          bInitialized(false)
{
    zoom = -4.0f;
    rotationSpeed = 0.25f;
    rotation = { -7.25f, 120.0f, 0.0f };
    gnCubemapTexture = 0;
    // Ensure we're using the desired unit
    glActiveTexture(cubemapTexUnit);

    processArgs(szArgs);

    KTX_error_code ktxresult;
    ktxTexture* kTexture;
    GLenum glerror;
    std::string ktxfilepath = externalFile ? ktxfilename
                                           : getAssetPath() + ktxfilename;
    ktxresult = ktxTexture_CreateFromNamedFile(ktxfilepath.c_str(),
                                               KTX_TEXTURE_CREATE_NO_FLAGS,
                                               &kTexture);
    if (KTX_SUCCESS != ktxresult) {
        std::stringstream message;
        
        message << "Creation of ktxTexture from \"" << ktxfilepath
                << "\" failed: " << ktxErrorString(ktxresult);
        throw std::runtime_error(message.str());
    }

    if (ktxTexture_NeedsTranscoding(kTexture)) {
        TextureTranscoder tc;
        tc.transcode((ktxTexture2*)kTexture);
        //transcoded = true;
    }

    ktxresult = ktxTexture_GLUpload(kTexture,
                                    &gnCubemapTexture, &cubemapTexTarget,
                                    &glerror);
    
    if (KTX_SUCCESS != ktxresult) {
        std::stringstream message;
        
        message << "ktxTexture_GLUpload failed: ";
        if (ktxresult != KTX_GL_ERROR) {
             message << ktxErrorString(ktxresult);
             throw std::runtime_error(message.str());
        } else if (kTexture->isCompressed && glerror == GL_INVALID_ENUM) {
             throw unsupported_ctype();
        } else {
             message << std::showbase << "GL error " << std::hex << glerror
                    << " occurred.";
             throw std::runtime_error(message.str());
        }
    }
    if (cubemapTexTarget != GL_TEXTURE_CUBE_MAP) {
        std::stringstream message;
        
        message << "Loaded texture is not a cubemap texture.";
        throw std::runtime_error(message.str());
    }
    
    numLayers = kTexture->numLayers;
    if (numLayers > 1 || kTexture->generateMipmaps)
        // GLUpload will have generated the mipmaps already.
        bIsMipmapped = true;
    else
        bIsMipmapped = false;

    levelCount = kTexture->numLevels;

    if (kTexture->orientation.y == KTX_ORIENT_Y_DOWN) {
        // Assume a KTX-compliant cube map. That means the faces are in a
        // LH coord system with +y up, +z forward and +x on the right.
        // Scale the skybox cube's z by -1 to convert it to LH coords to
        // match the cube map while placing the +z face in the -z direction
        // so it will be in front of the view. Alternatively we could multiply
        // the cube's x by -1 which will place the +z face in the +z direction
        // placing it behind the viewer.
        ubo.uvwTransform = glm::scale(glm::mat4(1.0f), glm::vec3(1, 1, -1));
    } else {
        std::stringstream message;

        message << "Cubemap faces have unsupported KTXorientation value.";
        throw std::runtime_error(message.str());
    }

    ktxTexture_Destroy(kTexture);

    try {
        prepare();
    } catch (std::exception& e) {
        (void)e; // To quiet unused variable warnings from some compilers.
        cleanup();
        throw;
    }
    bInitialized = true;
}

TextureCubemap::~TextureCubemap()
{
    cleanup();
}

void
TextureCubemap::resize(uint32_t width, uint32_t height)
{
    this->w_width = width;
    this->w_height = height;
    glViewport(0, 0, width, height);
    updateUniformBuffers();
}

void
TextureCubemap::run(uint32_t /*msTicks*/)
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


    // Draw object.
    glFrontFace(GL_CW); // Why is everything CW?
    glCullFace(GL_BACK);
    glUseProgram(gnReflectProg);
    meshes.objects[meshes.objectIndex].Draw();
    if (bDisplaySkybox) {
        // Change so depth test passes when values are equal to the
        // depth buffer's content. This works in conjunction with the
        // gl_Position = inPos.xyww trick in the shader.
        glDepthFunc(GL_LEQUAL);
        // The cube is a regular mesh with the front faces on the outside.
        // We're inside the cube so want to see the back faces.
        glCullFace(GL_FRONT);
        glUseProgram(gnSkyboxProg);
        meshes.skybox.Draw();
        // Revert to defaults for 3D object.
        glDepthFunc(GL_LESS);
    }
    assert(GL_NO_ERROR == glGetError());
}

//===================================================================

void
TextureCubemap::processArgs(std::string sArgs)
{
    // Options descriptor
    struct argparser::option longopts[] = {
      {"external",      argparser::option::no_argument, &externalFile, 1},
      {NULL,            argparser::option::no_argument, NULL,          0}
    };

    argvector argv(sArgs);
    argparser ap(argv);

    int ch;
    while ((ch = ap.getopt(nullptr, longopts, nullptr)) != -1) {
        switch (ch) {
            case 0: break;
            default: assert(false); // Error in args in sample table.
        }
    }
    assert(ap.optind < argv.size());
    ktxfilename = argv[ap.optind];
}

/* ------------------------------------------------------------------------- */

void
TextureCubemap::cleanup()
{
    glEnable(GL_DITHER);
    glDisable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);
    glDisable(GL_DEPTH_TEST);
    if (bInitialized) {
        glUseProgram(0);
        glDeleteTextures(1, &gnCubemapTexture);
        glDeleteProgram(gnReflectProg);
        glDeleteProgram(gnSkyboxProg);
        meshes.skybox.FreeGLResources();
        for (int i = 0; i < 3; i++) {
            meshes.objects[i].FreeGLResources();
        }
    }
    assert(GL_NO_ERROR == glGetError());
}

void
TextureCubemap::loadMeshes()
{
    std::string filepath = getAssetPath();

    // Skybox
    loadMesh(filepath + "cube.obj", meshes.skybox, vertexLayout, 0.05f);

    // Objects
    meshes.objects.resize(3);
    loadMesh(filepath + "sphere.obj", meshes.objects[0], vertexLayout, 0.05f);
    loadMesh(filepath + "teapot.dae", meshes.objects[1], vertexLayout, 0.05f);
    loadMesh(filepath + "torusknot.obj", meshes.objects[2], vertexLayout, 0.05f);
}

void
TextureCubemap::prepareUniformBuffers()
{
    uReflectProgramUniforms = glGetUniformBlockIndex(gnReflectProg, "UBO");
    if (uReflectProgramUniforms == -1) {
        std::stringstream message;
        
        message << "prepareUniformBuffers: UBO not found in reflect program";
        throw std::runtime_error(message.str());
    }

    uSkyboxProgramUniforms = glGetUniformBlockIndex(gnSkyboxProg, "UBO");
    if (uSkyboxProgramUniforms == -1) {
       std::stringstream message;

       message << "prepareUniformBuffers: UBO not found in skybox program";
       throw std::runtime_error(message.str());
    }

    glGenBuffers(1, &gnUbo);

    glBindBuffer(GL_UNIFORM_BUFFER, gnUbo);
    // Create the data store.
    glBufferData(GL_UNIFORM_BUFFER, sizeof(ubo), 0, GL_DYNAMIC_DRAW);

    glBindBufferBase(GL_UNIFORM_BUFFER, uniformBufferBindId, gnUbo);
    glUseProgram(gnReflectProg);
    glUniformBlockBinding(gnReflectProg, uReflectProgramUniforms,
                          uniformBufferBindId);

    glUseProgram(gnSkyboxProg);
    glUniformBlockBinding(gnSkyboxProg, uSkyboxProgramUniforms,
                          uniformBufferBindId);

    glBindBuffer(GL_UNIFORM_BUFFER, 0);

    updateUniformBuffers();
    glUseProgram(0);
    
    assert(glGetError() == GL_NO_ERROR);
}

void
TextureCubemap::updateUniformBuffers()
{
    // Reflect / 3D object
    glm::mat4 viewMatrix = glm::mat4(1.0f);
    ubo.projection = glm::perspective(glm::radians(60.0f),
                                      (float)w_width / (float)w_height,
                                      0.001f, 256.0f);
    viewMatrix = glm::translate(viewMatrix, glm::vec3(0.0f, 0.0f, zoom));

    // I do not understand why this is necessary. Assimp is supposed to
    // put models in the GL coordinate system by default but the teapot is
    // upside down. Since the other objects are symmetrical it is not possible
    // to say if they are upside down.
    glm::mat4 object;
    object = glm::rotate(object, glm::radians(180.0f),
                         glm::vec3(1.0f, 0.0f, 0.0f));
    ubo.modelView = viewMatrix * glm::translate(glm::mat4(), cameraPos);
    ubo.modelView = glm::rotate(ubo.modelView, glm::radians(rotation.x),
                                glm::vec3(1.0f, 0.0f, 0.0f));
    ubo.modelView = glm::rotate(ubo.modelView, glm::radians(rotation.y),
                                glm::vec3(0.0f, 1.0f, 0.0f));
    ubo.modelView = glm::rotate(ubo.modelView, glm::radians(rotation.z),
                                glm::vec3(0.0f, 0.0f, 1.0f));
    // Remove translation from modelView so the skybox doesn't move.
    ubo.skyboxView = glm::mat4(glm::mat3(ubo.modelView));
    // Do the inverse here because doing it in every fragment is a bit much.
    ubo.invModelView = glm::inverse(ubo.modelView);
    // Now add the object rotation.
    ubo.modelView = ubo.modelView * object;


    glBindBuffer(GL_UNIFORM_BUFFER, gnUbo);
#if !defined(EMSCRIPTEN)
    uint8_t* pData = (uint8_t*)glMapBufferRange(GL_UNIFORM_BUFFER,
                                                0, sizeof(ubo),
                                                GL_MAP_WRITE_BIT);
    memcpy(pData, &ubo, sizeof(ubo));
    glUnmapBuffer(GL_UNIFORM_BUFFER);
#else
    glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(ubo), &ubo);
#endif
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
}

void
TextureCubemap::prepareSampler()
{
    glBindTexture(cubemapTexTarget, gnCubemapTexture);
    if (bIsMipmapped)
        glTexParameteri(cubemapTexTarget,
                        GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
    else
        glTexParameteri(cubemapTexTarget, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(cubemapTexTarget, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glUseProgram(gnReflectProg);
    if ((uReflectCubemap = glGetUniformLocation(gnReflectProg,
                                                "uSamplerColor")) == -1) {
        std::stringstream message;
        
        message << "prepareSampler: uSamplerColor not found in reflect program";
        throw std::runtime_error(message.str());
    }
    glUniform1i(uReflectCubemap, cubemapTexUnit - GL_TEXTURE0);

    glUseProgram(gnSkyboxProg);
    if ((uSkyboxCubemap = glGetUniformLocation(gnSkyboxProg,
                                              "uSamplerColor")) == -1) {
        std::stringstream message;

        message << "prepareSampler: uSamplerColor not found in skybox program";
        throw std::runtime_error(message.str());
    }
    glUniform1i(uSkyboxCubemap, cubemapTexUnit - GL_TEXTURE0);

    glUseProgram(0);
}

void
TextureCubemap::preparePrograms()
{
    GLuint gnReflectFs, gnReflectVs, gnSkyboxFs, gnSkyboxVs;
    const GLchar* actualReflectFs;
    const GLchar* actualSkyboxFs;

    if (framebufferColorEncoding() == GL_LINEAR) {
        actualReflectFs = pszReflectSrgbEncodeFs;
        actualSkyboxFs = pszSkyboxSrgbEncodeFs;
    } else {
        actualReflectFs = pszReflectFs;
        actualSkyboxFs = pszSkyboxFs;
    }
    try {
        makeShader(GL_VERTEX_SHADER, pszReflectVs, &gnReflectVs);
        makeShader(GL_FRAGMENT_SHADER, actualReflectFs, &gnReflectFs);
        makeProgram(gnReflectVs, gnReflectFs, &gnReflectProg);
        makeShader(GL_VERTEX_SHADER, pszSkyboxVs, &gnSkyboxVs);
        makeShader(GL_FRAGMENT_SHADER, actualSkyboxFs, &gnSkyboxFs);
        makeProgram(gnSkyboxVs, gnSkyboxFs, &gnSkyboxProg);
    } catch (std::exception& e) {
        (void)e; // To quiet unused variable warnings from some compilers.
        throw;
    }
    glDeleteShader(gnReflectVs);
    glDeleteShader(gnReflectFs);
    glDeleteShader(gnSkyboxVs);
    glDeleteShader(gnSkyboxFs);
}

void
TextureCubemap::prepare()
{
    // By default dithering is enabled. Dithering does not provide visual
    // improvement in this sample so disable it to improve performance.
    glDisable(GL_DITHER);

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glClearColor(0.2f,0.3f,0.4f,1.0f);

    loadMeshes();

    preparePrograms();
    prepareUniformBuffers();
    prepareSampler();
}

void
TextureCubemap::toggleSkyBox()
{
    bDisplaySkybox = !bDisplaySkybox;
}

void
TextureCubemap::toggleObject()
{
    meshes.objectIndex++;
    if (meshes.objectIndex >= static_cast<uint32_t>(meshes.objects.size()))
    {
        meshes.objectIndex = 0;
    }
}

void
TextureCubemap::changeLodBias(float delta)
{
    ubo.lodBias += delta;
    if (ubo.lodBias < 0.0f)
    {
        ubo.lodBias = 0.0f;
    }
    if (ubo.lodBias > levelCount)
    {
        ubo.lodBias = (float)levelCount;
    }
    updateUniformBuffers();
}

void
TextureCubemap::keyPressed(uint32_t keyCode)
{
    switch (keyCode)
    {
    case 's':
        toggleSkyBox();
        break;
    case ' ':
        toggleObject();
        break;
    case SDLK_KP_PLUS:
        changeLodBias(0.1f);
        break;
    case SDLK_KP_MINUS:
        changeLodBias(-0.1f);
        break;
    }
}



