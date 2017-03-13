#include <UIKit/UIKit.h>
#include <Metal/Metal.h>
#include <QuartzCore/CAMetalLayer.h>

@interface MetalView : UIView

@property (assign, nonatomic) CAMetalLayer *metalLayer;

@end

MetalView* SDL_AddMetalView(UIWindow* sdlWindow);
