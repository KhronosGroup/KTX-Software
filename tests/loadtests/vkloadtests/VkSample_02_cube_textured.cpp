/* -*- tab-width: 4; -*- */
/* vi: set sw=2 ts=4 expandtab: */

/* $Id$ */

/**
 * @file	sample_02_cube_textured.c
 * @brief	Draw a textured cube.
 */

/*
 * Copyright (c) 2008 HI Corporation.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and/or associated documentation files (the
 * "Materials"), to deal in the Materials without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Materials, and to
 * permit persons to whom the Materials are furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * unaltered in all copies or substantial portions of the Materials.
 * Any additions, deletions, or changes to the original source files
 * must be clearly indicated in accompanying documentation.
 *
 * If only executable code is distributed, then the accompanying
 * documentation must state that "this software is based in part on the
 * work of HI Corporation."
 *
 * THE MATERIALS ARE PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * MATERIALS OR THE USE OR OTHER DEALINGS IN THE MATERIALS.
 */

#if defined(_WIN32)
  #if _MSC_VER < 1900
	#define snprintf _snprintf
  #endif
  #define _CRT_SECURE_NO_WARNINGS
#endif

#include <assert.h>
#include <ktx.h>
#include <math.h>
#include <vulkan/vulkan.h>

#include "VkSample_02_cube_textured.h"

VkSample*
VkSample_02_cube_textured::create(const VkCommandPool& commandPool,
                                  const VkDevice& device,
                                  const VkRenderPass& renderPass,
                                  VkAppSDL::Swapchain& swapchain,
                                  const char* const szArgs,
                                  const char* const szBasePath)
{
    VkSample_02_cube_textured* result =
            new VkSample_02_cube_textured(commandPool, device, renderPass,
                                          swapchain);
    result->initialize(szArgs, szBasePath);
    return result;
}


VkSample_02_cube_textured::~VkSample_02_cube_textured()
{

}


#if 0
#include "mygl.h"
#include "at.h"
#include "cube.h"

/* ------------------------------------------------------------------------- */

GLboolean makeShader(GLenum type, const GLchar* const source, GLuint* shader);
GLboolean makeProgram(GLuint vs, GLuint fs, GLuint* program);

extern const GLchar* pszVs;
extern const GLchar* pszDecalFs;
#endif
/* ------------------------------------------------------------------------- */

typedef struct CubeTextured_def {
        ktx_uint32_t dummy;
#if 0
	GLuint gnTexture;
	GLuint gnTexProg;

	GLuint gnVao;
	GLuint gnVbo;

	GLsizeiptr iIndicesOffset;

	GLint gulMvMatrixLocTP;
	GLint gulPMatrixLocTP;
	GLint gulSamplerLocTP;

	GLboolean bInitialized;
#endif
} CubeTextured;

/* ------------------------------------------------------------------------- */

void
VkSample_02_cube_textured::initialize(const char* const szArgs,
                                      const char* const szBasePath)
{
    for (int i = 0; i < swapchain.imageCount; i++) {
        buildCommandBuffer(i);
    }


#if 0
    const char* filename;
	GLuint texture = 0;
	GLenum target;
	GLenum glerror;
	GLboolean isMipmapped;
	GLuint gnDecalFs, gnVs;
	GLsizeiptr offset;
	KTX_error_code ktxerror;

	CubeTextured* pData = (CubeTextured*)atMalloc(sizeof(CubeTextured), 0);

	atAssert(pData);
	atAssert(ppAppData);

	*ppAppData = pData;

	pData->bInitialized = GL_FALSE;
	pData->gnTexture = 0;

    filename = atStrCat(szBasePath, szArgs);
    
    if (filename != NULL) {
        ktxerror = ktxLoadTextureN(filename, &texture, &target,
                                   NULL, &isMipmapped, &glerror,
                                   0, NULL);

        if (KTX_SUCCESS == ktxerror) {
            if (target != GL_TEXTURE_2D) {
                /* Can only draw 2D textures */
                glDeleteTextures(1, &texture);
                return;
            }

            if (isMipmapped) 
                /* Enable bilinear mipmapping */
                /* TO DO: application can consider inserting a key,value pair in the KTX
                 * file that indicates what type of filtering to use.
                 */
                glTexParameteri(target, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
            else
                glTexParameteri(target, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(target, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

            atAssert(GL_NO_ERROR == glGetError());
        } else {
            char message[1024];
            int maxchars = sizeof(message)/sizeof(char);
            int nchars;

			nchars = snprintf(message, maxchars, "Load of texture \"%s\" failed: ",
				              filename);
			maxchars -= nchars;
			if (ktxerror == KTX_GL_ERROR) {
				nchars += snprintf(&message[nchars], maxchars, "GL error %#x occurred.", glerror);
			} else {
				nchars += snprintf(&message[nchars], maxchars, "%s.", ktxErrorString(ktxerror));
			}
			atMessageBox(message, "Texture load failed", AT_MB_OK | AT_MB_ICONERROR);
        }
        
        atFree((void*)filename, NULL);
    } /* else
       Out of memory. In which case, a message box is unlikely to work. */


	/* By default dithering is enabled. Dithering does not provide visual improvement
	 * in this sample so disable it to improve performance. 
	 */
	glDisable(GL_DITHER);

	glEnable(GL_CULL_FACE);
	glClearColor(0.2f,0.3f,0.4f,1.0f);

	// Create a VAO and bind it.
	glGenVertexArrays(1, &pData->gnVao);
	glBindVertexArray(pData->gnVao);

	// Must have vertex data in buffer objects to use VAO's on ES3/GL Core
	glGenBuffers(1, &pData->gnVbo);
	glBindBuffer(GL_ARRAY_BUFFER, pData->gnVbo);
	// Must be done after the VAO is bound
	// Use the same buffer for vertex attributes and element indices.
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, pData->gnVbo);

	// Create the buffer data store. 
	glBufferData(GL_ARRAY_BUFFER,
				 sizeof(cube_face) + sizeof(cube_color) + sizeof(cube_texture)
				 + sizeof(cube_normal) + sizeof(cube_index_buffer),
				 NULL, GL_STATIC_DRAW);

	// Interleave data copying and attrib pointer setup so offset is only computed once.
	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);
	glEnableVertexAttribArray(2);
	glEnableVertexAttribArray(3);
	offset = 0;
	glBufferSubData(GL_ARRAY_BUFFER, offset, sizeof(cube_face), cube_face);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (GLvoid*)offset);
	offset += sizeof(cube_face);
	glBufferSubData(GL_ARRAY_BUFFER, offset, sizeof(cube_color), cube_color);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, (GLvoid*)offset);
	offset += sizeof(cube_color);
	glBufferSubData(GL_ARRAY_BUFFER, offset, sizeof(cube_texture), cube_texture);
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 0, (GLvoid*)offset);
	offset += sizeof(cube_texture);
	glBufferSubData(GL_ARRAY_BUFFER, offset, sizeof(cube_normal), cube_normal);
	glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, 0, (GLvoid*)offset);
	offset += sizeof(cube_normal);
	pData->iIndicesOffset = offset;
	// Either of the following can be used to buffer the data.
	glBufferSubData(GL_ARRAY_BUFFER, offset, sizeof(cube_index_buffer), cube_index_buffer);
	//glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, offset, sizeof(cube_index_buffer), cube_index_buffer);

	if (makeShader(GL_VERTEX_SHADER, pszVs, &gnVs)) {
		if (makeShader(GL_FRAGMENT_SHADER, pszDecalFs, &gnDecalFs)) {
			if (makeProgram(gnVs, gnDecalFs, &pData->gnTexProg)) {
				pData->gulMvMatrixLocTP = glGetUniformLocation(pData->gnTexProg, "mvmatrix");
				pData->gulPMatrixLocTP = glGetUniformLocation(pData->gnTexProg, "pmatrix");
				pData->gulSamplerLocTP = glGetUniformLocation(pData->gnTexProg, "sampler");
				glUseProgram(pData->gnTexProg);
				// We're using the default texture unit 0
				glUniform1i(pData->gulSamplerLocTP, 0);
			}
		}
		glDeleteShader(gnVs);
		glDeleteShader(gnDecalFs);
	}

	atAssert(GL_NO_ERROR == glGetError());
	pData->bInitialized = GL_TRUE;
#endif
}

void
VkSample_02_cube_textured::finalize()
{
    for (int i = 0; i < swapchain.imageCount; i++) {
      vkFreeCommandBuffers(device, commandPool, 1, &swapchain.buffers[i].cmd);
    }
#if 0
	CubeTextured* pData = (CubeTextured*)pAppData;
	atAssert(pData);

	glEnable(GL_DITHER);
    glEnable(GL_CULL_FACE);
	if (pData->bInitialized) {
		glUseProgram(0);
		glDeleteTextures(1, &pData->gnTexture);
		glDeleteProgram(pData->gnTexProg);
		glDeleteBuffers(1, &pData->gnVbo);
		glDeleteVertexArrays(1, &pData->gnVao);
	}
	atAssert(GL_NO_ERROR == glGetError());
	atFree(pData, 0);
#endif
}


void
VkSample_02_cube_textured::resize(int iWidth, int iHeight)
{
    for (int i = 0; i < swapchain.imageCount; i++) {
        buildCommandBuffer(i);
    }
#if 0
	GLfloat matProj[16];
	CubeTextured* pData = (CubeTextured*)pAppData;
	atAssert(pData);

	glViewport(0, 0, iWidth, iHeight);

	atSetProjectionMatrix(matProj, 45.f, iWidth / (GLfloat)iHeight, 1.0f, 100.f);
	glUniformMatrix4fv(pData->gulPMatrixLocTP, 1, GL_FALSE, matProj);
#endif
}

void
VkSample_02_cube_textured::run(int iTimeMS)
{
#if 0
	/* Draw */
	CubeTextured* pData = (CubeTextured*)pAppData;
	/* Setup the view matrix : just turn around the cube. */
	float matView[16];
	const float fDistance = 50.0f;

	atAssert(pData);

	atSetViewMatrix(matView, 
		(float)cos( iTimeMS*0.001f ) * fDistance, (float)sin( iTimeMS*0.0007f ) * fDistance, (float)sin( iTimeMS*0.001f ) * fDistance, 
		0.0f, 0.0f, 0.0f);

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glUniformMatrix4fv(pData->gulMvMatrixLocTP, 1, GL_FALSE, matView);

	glDrawElements(GL_TRIANGLES, sizeof(cube_index_buffer), GL_UNSIGNED_BYTE, (GLvoid*)(pData->iIndicesOffset));

	atAssert(GL_NO_ERROR == glGetError());
#endif
}

/* ------------------------------------------------------------------------- */

void
VkSample_02_cube_textured::buildCommandBuffer(int bufferIndex)
{
    VkResult U_ASSERT_ONLY err;

    const VkCommandBufferBeginInfo cmd_buf_info = {
        VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO, NULL, 0, NULL
    };

    VkClearValue clear_values[2] = {
       { bufferIndex * 1.f, 0.2f, 0.2f, 1.0f },
       { 0.0f, 0 }
    };

    const VkRenderPassBeginInfo rp_begin = {
        VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        NULL,
        renderPass,
        swapchain.buffers[bufferIndex].fb,
        { 0, 0, swapchain.extent.width, swapchain.extent.height },
        2,
        clear_values,
    };

    err = vkBeginCommandBuffer(swapchain.buffers[bufferIndex].cmd,
                               &cmd_buf_info);
    assert(!err);

    // We can use LAYOUT_UNDEFINED as a wildcard here because we don't care what
    // happens to the previous contents of the image
    VkImageMemoryBarrier image_memory_barrier = {
        VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        NULL,
        0,
        VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
        VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        VK_QUEUE_FAMILY_IGNORED,
        VK_QUEUE_FAMILY_IGNORED,
        swapchain.buffers[bufferIndex].image,
        {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1},
    };
    vkCmdPipelineBarrier(swapchain.buffers[bufferIndex].cmd,
                         VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                         VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
                         0, 0, NULL, 0,
                         NULL, 1, &image_memory_barrier);

    vkCmdBeginRenderPass(swapchain.buffers[bufferIndex].cmd, &rp_begin,
                         VK_SUBPASS_CONTENTS_INLINE);
    vkCmdEndRenderPass(swapchain.buffers[bufferIndex].cmd);

    VkImageMemoryBarrier present_barrier = {
        VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        NULL,
        VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
        VK_ACCESS_MEMORY_READ_BIT,
        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
        VK_QUEUE_FAMILY_IGNORED,
        VK_QUEUE_FAMILY_IGNORED,
        swapchain.buffers[bufferIndex].image,
        { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 },
    };
    present_barrier.image = swapchain.buffers[bufferIndex].image;

    vkCmdPipelineBarrier(
        swapchain.buffers[bufferIndex].cmd,
        VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
        VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
        0, 0, NULL, 0, NULL, 1, &present_barrier);

    err = vkEndCommandBuffer(swapchain.buffers[bufferIndex].cmd);
    assert(!err);
}


void
VkSample_02_cube_textured::prepareCubeDataBuffer()
{

}


