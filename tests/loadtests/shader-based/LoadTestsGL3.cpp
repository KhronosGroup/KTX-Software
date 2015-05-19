/* -*- tab-width: 4; -*- */
/* vi: set sw=2 ts=4: */

/* $Id: ac63511da134f2c25a9e1da86a36bc27b6198ae3 $ */

/**
 * @internal
 * @file LoadTestsGL3.cpp
 * @~English
 *
 * @brief LoadTests app for OpenGL 3.3+ and OpenGL ES 3.x
 *
 * @author Mark Callow
 * @copyright (c) 2015, Mark Callow.
 */

/*
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

#include "LoadTestsGL3.h"

#if defined(_WIN32)
#define snprintf _snprintf
#endif

extern "C" {
/* ----------------------------------------------------------------------------- */
/* SAMPLE 01 */
void atInitialize_01_draw_texture(void** ppAppData, const char* const args);
void atRelease_01_draw_texture(void* pAppData);
void atResize_01_draw_texture(void* pAppData, int iWidth, int iHeight);
void atRun_01_draw_texture(void* pAppData, int iTimeMS); 

static const atSample sc_Sample01 = {
	atInitialize_01_draw_texture,
	atRelease_01_draw_texture,
	atResize_01_draw_texture,
	atRun_01_draw_texture,
};

/* ----------------------------------------------------------------------------- */
/* SAMPLE 02 */
void atInitialize_02_cube(void** ppAppData, const char* const args);
void atRelease_02_cube(void* pAppData);
void atResize_02_cube(void* pAppData, int iWidth, int iHeight);
void atRun_02_cube(void* pAppData, int iTimeMS); 

static const atSample sc_Sample02 = {
	atInitialize_02_cube,
	atRelease_02_cube,
	atResize_02_cube,
	atRun_02_cube,
};

}
/* ----------------------------------------------------------------------------- */

const LoadTestsGL3::sampleInvocation LoadTestsGL3::siSamples[] = {
	{ &sc_Sample01, "testimages/hi_mark.ktx", "RGB8 NPOT HI Logo" },
	{ &sc_Sample01, "testimages/luminance_unsized_reference.ktx", "Luminance (Unsized)" },
	{ &sc_Sample01, "testimages/luminance_sized_reference.ktx", "Luminance (Sized)" },
	{ &sc_Sample01, "testimages/up-reference.ktx", "RGB8" },
	{ &sc_Sample01, "testimages/down-reference.ktx", "RGB8 + KTXOrientation"},
	{ &sc_Sample01, "testimages/etc1.ktx", "ETC1 RGB8" },
	{ &sc_Sample01, "testimages/etc2-rgb.ktx", "ETC2 RGB8"},
	{ &sc_Sample01, "testimages/etc2-rgba1.ktx", "ETC2 RGB8A1"},
	{ &sc_Sample01, "testimages/etc2-rgba8.ktx", "ETC2 RGB8A8" },
	{ &sc_Sample01, "testimages/etc2-sRGB.ktx", "ETC2 sRGB8"},
	{ &sc_Sample01, "testimages/etc2-sRGBa1.ktx", "ETC2 sRGB8A1"},
	{ &sc_Sample01, "testimages/etc2-sRGBa8.ktx", "ETC2 sRGB8A8" },
	{ &sc_Sample01, "testimages/rgba-reference.ktx", "RGBA8"},
	{ &sc_Sample01, "testimages/rgb-reference.ktx", "RGB8" },
	{ &sc_Sample01, "testimages/conftestimage_R11_EAC.ktx", "ETC2 R11"},
	{ &sc_Sample01, "testimages/conftestimage_SIGNED_R11_EAC.ktx", "ETC2 Signed R11" },
	{ &sc_Sample01, "testimages/conftestimage_RG11_EAC.ktx", "ETC2 RG11" },
	{ &sc_Sample01, "testimages/conftestimage_SIGNED_RG11_EAC.ktx", "ETC2 Signed RG11" },
	{ &sc_Sample02, "testimages/rgb-amg-reference.ktx", "RGB8 + Auto Mipmap" },
	{ &sc_Sample02, "testimages/rgb-mipmap-reference.ktx", "Color/level mipmap" },
	{ &sc_Sample02, "testimages/hi_mark_sq.ktx", "RGB8 NPOT HI Logo" }
};

const int LoadTestsGL3::iNumSamples = sizeof(LoadTestsGL3::siSamples) / sizeof(sampleInvocation);

const char* const LoadTestsGL3::szName = "KTX Loader Tests";

AppBaseSDL* theApp = new LoadTestsGL3();


LoadTestsGL3::LoadTestsGL3()
{
    iCurSampleNum = 0;
    pCurSampleInv = &siSamples[0];
}

bool
LoadTestsGL3::initialize(int argc, char* argv[])
{
    int width, height;

    SDL_GL_SetAttribute( SDL_GL_CONTEXT_PROFILE_MASK, GL_CONTEXT_PROFILE);
    SDL_GL_SetAttribute( SDL_GL_CONTEXT_MAJOR_VERSION, GL_CONTEXT_MAJOR_VERSION);
    SDL_GL_SetAttribute( SDL_GL_CONTEXT_MINOR_VERSION, GL_CONTEXT_MINOR_VERSION);
    
    pswMainWindow = SDL_CreateWindow(
                        szName,
                        SDL_WINDOWPOS_UNDEFINED,
                        SDL_WINDOWPOS_UNDEFINED,
                        640, 480,
                        SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE
                    );

    if (pswMainWindow == NULL) {
        (void)SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, szName, SDL_GetError(), NULL);
        return false;
    }
    
    sgcGLContext = SDL_GL_CreateContext(pswMainWindow);
    if (sgcGLContext == NULL) {
        (void)SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, szName, SDL_GetError(), NULL);
        return false;
    }

    szBasePath = SDL_GetBasePath();
    if (szBasePath == NULL)
        szBasePath = SDL_strdup("./");
    
    // Not getting an initialize resize event, at least on Mac OS X.
    // Therefore use invokeSample which calls the sample's resize func.
    invokeSample(iCurSampleNum);
    
    return AppBaseSDL::initialize(argc, argv);
}


void
LoadTestsGL3::finalize()
{
    pCurSampleInv->sample->pfRelease(pCurSampleData);
}


int
LoadTestsGL3::doEvent(void* userdata, SDL_Event* event)
{
    switch (event->type) {
      case SDL_MOUSEBUTTONUP:
        switch (event->button.button) {
          case SDL_BUTTON_LEFT:
            pCurSampleInv->sample->pfRelease(pCurSampleData);
            if (++iCurSampleNum >= iNumSamples)
              iCurSampleNum = 0;
            pCurSampleInv = &siSamples[iCurSampleNum];
            invokeSample(iCurSampleNum);
            return 0;
        }
        break;
      case SDL_WINDOWEVENT:
        switch (event->window.event) {
          case SDL_WINDOWEVENT_RESIZED:
            resize(event->window.data1, event->window.data2);
            return 0;
        }
        break;
            
    }
    return AppBaseSDL::doEvent(userdata, event);
}


void
LoadTestsGL3::resize(int width, int height)
{
    pCurSampleInv->sample->pfResize(pCurSampleData, width, height);
}


void
LoadTestsGL3::update(void* userdata, int ticks)
{
    pCurSampleInv->sample->pfRun(pCurSampleData, ticks);
    SDL_GL_SwapWindow(pswMainWindow);
    AppBaseSDL::update(userdata, ticks);
}


void
LoadTestsGL3::invokeSample(int iSampleNum)
{
    int width, height;

    pCurSampleInv = &siSamples[iSampleNum];
    pCurSampleInv->sample->pfInitialize(
                                    &pCurSampleData,
                                    (szBasePath + pCurSampleInv->args).c_str()
                                       );
    
    SDL_GL_GetDrawableSize(pswMainWindow, &width, & height);
    setWindowTitle(pCurSampleInv->title);
    pCurSampleInv->sample->pfResize(pCurSampleData, width, height);
}


void
LoadTestsGL3::onFPSUpdate()
{
    // Using onFPSUpdate avoids rewriting the title every frame.
    setWindowTitle(pCurSampleInv->title);
}


void
LoadTestsGL3::setWindowTitle(const char* const szSampleName)
{
    char szTitle[100];
    
    snprintf(szTitle, sizeof(szTitle), "%#.2f fps. %s: %s", fFPS, szName, szSampleName);
    SDL_SetWindowTitle(pswMainWindow, szTitle);
}
