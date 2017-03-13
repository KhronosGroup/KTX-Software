#import "SDL_cocoametalview.h"

@implementation MetalView

+ (Class)layerClass
{
  return [CAMetalLayer class];
}

- (instancetype)initWithFrame:(CGRect)frame;
{
  if ((self = [super initWithFrame:frame])) {
    
    /* Resize properly when rotated. */
    self.autoresizingMask = NSViewWidthSizable | NSViewHeightSizable;
    
    _metalLayer = [CAMetalLayer layer];
    _metalLayer.framebufferOnly = YES;
    _metalLayer.opaque = YES;
    _metalLayer.device = MTLCreateSystemDefaultDevice();
    //metalLayer.drawableSize = (CGSize) [nsview convertRectToBacking:[nsview bounds]].size;

    //[self updateDrawableSize];
  }
  
  return self;
}

@end

MetalView* SDL_AddMetalView(NSWindow* sdlwindow)
{
    NSView *nsview = [sdlwindow contentView];
    MetalView *metalview = [[MetalView alloc] initWithFrame:nsview.frame];
    [nsview addSubview:metalview];
  
    return metalview;
}

