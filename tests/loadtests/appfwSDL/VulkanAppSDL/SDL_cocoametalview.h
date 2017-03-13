
#import <Cocoa/Cocoa.h>
#import <Metal/Metal.h>
#import <QuartzCore/CAMetalLayer.h>

@interface MetalView : NSView

@property (retain, nonatomic) CAMetalLayer *metalLayer;

@end
