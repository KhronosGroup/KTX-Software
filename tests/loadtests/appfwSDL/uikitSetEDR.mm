/* -*- tab-width: 4; -*- */
/* vi: set sw=2 ts=4 expandtab: */

/*
 * Copyright 2026 Mark Callow.
 * SPDX-License-Identifier: Apache-2.0
 */
#include <UIKit/UIKit.h>
#include <QuartzCore/QuartzCore.h>
#include <SDL3/SDL.h>

void
setWantsExtendedDynamicRangeContent(SDL_Window* window, bool enable)
{
    SDL_PropertiesID wprops = SDL_GetWindowProperties(window);
    NSInteger tag =
      SDL_GetNumberProperty(wprops, SDL_PROP_WINDOW_UIKIT_METAL_VIEW_TAG_NUMBER, 0);
    UIWindow* uiwindow =
      (UIWindow*)SDL_GetPointerProperty(wprops, SDL_PROP_WINDOW_UIKIT_WINDOW_POINTER, nullptr);

    UIView *metalView = [uiwindow viewWithTag:tag];
    if (metalView != nil) {
        CALayer* layer = metalView.layer;
        if ([layer isKindOfClass:[CAMetalLayer class]]) {
            if (@available(iOS 16.0, *)) {
              ((CAMetalLayer*)layer).wantsExtendedDynamicRangeContent = enable;
            }
        }
    }
}
