/**
 * This software is in the public domain. Where that dedication is not recognized,
 * you are granted a perpetual, irrevokable license to copy and modify this file
 * as you see fit.
 *
 * Requires SDL 2.0.4.
 * Devices that do not support Metal are not handled currently.
 **/

#import <UIKit/UIKit.h>
#import <Metal/Metal.h>
#import <QuartzCore/CAMetalLayer.h>

#import "SDL_uikitmetalview.h"

@implementation MetalView

+ (Class)layerClass
{
    return [CAMetalLayer class];
}

- (instancetype)initWithFrame:(CGRect)frame
{
    if ((self = [super initWithFrame:frame])) {
        /* Resize properly when rotated. */
        self.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight;

        /* Use the screen's native scale (retina resolution, when available.) */
        self.contentScaleFactor = [UIScreen mainScreen].nativeScale;

        _metalLayer = (CAMetalLayer *) self.layer;
        _metalLayer.opaque = YES;
        _metalLayer.device = MTLCreateSystemDefaultDevice();

        [self updateDrawableSize];
    }

    return self;
}

/* Set the size of the metal drawables when the view is resized. */
- (void)layoutSubviews
{
    [super layoutSubviews];
    [self updateDrawableSize];
}

- (void)updateDrawableSize
{
    CGSize size  = self.bounds.size;
    size.width  *= self.contentScaleFactor;
    size.height *= self.contentScaleFactor;

    _metalLayer.drawableSize = size;
}

@end

MetalView* SDL_AddMetalView(UIWindow* sdlwindow)
{
    UIView* sdlview = sdlwindow.rootViewController.view;
    MetalView *metalview = [[MetalView alloc] initWithFrame:sdlview.frame];
    [sdlview addSubview:metalview];
    return metalview;

    // XXX Somewhere during Window close we probably need something like this.
    //[metalview removeFromSuperview];
}
