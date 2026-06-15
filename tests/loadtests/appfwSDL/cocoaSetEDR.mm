/* -*- tab-width: 4; -*- */
/* vi: set sw=2 ts=4 expandtab: */

/*
 * Copyright 2026 Mark Callow.
 * SPDX-License-Identifier: Apache-2.0
 */
#include <Cocoa/Cocoa.h>
#include <QuartzCore/QuartzCore.h>
#include <SDL3/SDL.h>

void
setWantsExtendedDynamicRangeContent(SDL_Window* window, bool enable)
{
    SDL_PropertiesID wprops = SDL_GetWindowProperties(window);
    NSInteger tag =
      SDL_GetNumberProperty(wprops, SDL_PROP_WINDOW_COCOA_METAL_VIEW_TAG_NUMBER, 0);
    NSWindow* nswindow =
      (NSWindow*)SDL_GetPointerProperty(wprops, SDL_PROP_WINDOW_COCOA_WINDOW_POINTER, nullptr);

    NSView* contentView = nswindow.contentView;
    NSArray* subviews = contentView.subviews;
    for (NSView* view in subviews) {
        if (view.tag == tag) {
            CALayer* layer = view.layer;
            if ([layer isKindOfClass:[CAMetalLayer class]]) {
                ((CAMetalLayer*)layer).wantsExtendedDynamicRangeContent = enable;
            }
            break;
        }
    }
}
