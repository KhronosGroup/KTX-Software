#include <string.h>

#include <SDL_config.h>
/* The following is necessary when compiling this file outside of SDL
 * due to the linux build using a generated SDL_config.h which is not
 * visible outside SDL. The other platforms use the public SDL_config.h.
 */
#if __LINUX__
#define SDL_VIDEO_DRIVER_WAYLAND 1
#define SDL_VIDEO_DRIVER_X11 1
#endif

#if SDL_VIDEO_DRIVER_ANDROID
  #define VK_USE_PLATFORM_ANDROID_KHR
#elif SDL_VIDEO_DRIVER_COCOA
  #define VK_USE_PLATFORM_MACOS_MVK
  typedef struct _MetalView MetalView;
  typedef struct _NSWindow NSWindow;
  MetalView* SDL_AddMetalView(NSWindow* sdlWindow);
#elif SDL_VIDEO_DRIVER_UIKIT
  #define VK_USE_PLATFORM_IOS_MVK
  typedef struct _MetalView MetalView;
  typedef struct _UIWindow UIWindow;
  MetalView* SDL_AddMetalView(UIWindow* sdlWindow);
#elif SDL_VIDEO_DRIVER_WINDOWS
  #define VK_USE_PLATFORM_WIN32_KHR
#else
  #if SDL_VIDEO_DRIVER_WAYLAND
    #define VK_USE_PLATFORM_WAYLAND_KHR
  #endif
  #if SDL_VIDEO_DRIVER_X11
    #define VK_USE_PLATFORM_XCB_KHR
    #include <X11/Xlib-xcb.h>
  #endif
#endif
#include <SDL_vulkan.h>

#include <SDL_syswm.h>

static SDL_bool SetNames(unsigned* count, const char** names, unsigned inCount, const char* const* inNames) {
    unsigned capacity = *count;
    *count = inCount;
    if (names) {
        if (capacity < inCount) {
            SDL_SetError("Insufficient capacity for extension names: %u < %u", capacity, inCount);
            return SDL_FALSE;
        }
        for (unsigned i = 0; i < inCount; ++i)
            names[i] = inNames[i];
    }
    return SDL_TRUE;
}

SDL_bool SDL_GetVulkanInstanceExtensions(unsigned* count, const char** names) {
    const char *driver = SDL_GetCurrentVideoDriver();
    if (!driver) {
        SDL_SetError("No video driver - has SDL_Init(SDL_INIT_VIDEO) been called?");
        return SDL_FALSE;
    }
    if (!count) {
        SDL_SetError("'count' is null");
        return SDL_FALSE;
    }
#if SDL_VIDEO_DRIVER_ANDROID
    if (!strcmp(driver, "android")) {
        const char* ext[] = { VK_KHR_ANDROID_SURFACE_EXTENSION_NAME };
        return SetNames(count, names, 1, ext);
    }
#endif
#if SDL_VIDEO_DRIVER_COCOA
    if (!strcmp(driver, "cocoa")) {
        const char* ext[] = { VK_MVK_MACOS_SURFACE_EXTENSION_NAME };
        return SetNames(count, names, 1, ext);
    }
#endif
#if SDL_VIDEO_DRIVER_UIKIT
    if (!strcmp(driver, "uikit")) {
        const char* ext[] = { VK_MVK_IOS_SURFACE_EXTENSION_NAME };
        return SetNames(count, names, 1, ext);
    }
#endif
#if SDL_VIDEO_DRIVER_WAYLAND
    if (!strcmp(driver, "wayland")) {
        const char* ext[] = { VK_KHR_WAYLAND_SURFACE_EXTENSION_NAME };
        return SetNames(count, names, 1, ext);
    }
#endif
#if SDL_VIDEO_DRIVER_WINDOWS
    if (!strcmp(driver, "windows")) {
        const char* ext[] = { VK_KHR_WIN32_SURFACE_EXTENSION_NAME };
        return SetNames(count, names, 1, ext);
    }
#endif
#if SDL_VIDEO_DRIVER_X11
    if (!strcmp(driver, "x11")) {
        const char* ext[] = { VK_KHR_XCB_SURFACE_EXTENSION_NAME };
        return SetNames(count, names, 1, ext);
    }
#endif

    (void)SetNames;
    (void)names;

    SDL_SetError("Unsupported video driver '%s'", driver);
    return SDL_FALSE;
}

SDL_bool SDL_CreateVulkanSurface(SDL_Window* window, VkInstance instance, VkSurfaceKHR* surface) {
    if (!window) {
        SDL_SetError("'window' is null");
        return SDL_FALSE;
    }
    if (instance == VK_NULL_HANDLE) {
        SDL_SetError("'instance' is null");
        return SDL_FALSE;
    }

    SDL_SysWMinfo wminfo;
    SDL_VERSION(&wminfo.version);
    if (!SDL_GetWindowWMInfo(window, &wminfo))
        return SDL_FALSE;

    switch (wminfo.subsystem) {
#if SDL_VIDEO_DRIVER_ANDROID
    case SDL_SYSWM_ANDROID:
    {
        VkAndroidSurfaceCreateInfoKHR createInfo;
        createInfo.sType = VK_STRUCTURE_TYPE_ANDROID_SURFACE_CREATE_INFO_KHR;
        createInfo.pNext = NULL;
        createInfo.flags = 0;
        createInfo.window = wminfo.info.android.window;

        VkResult r =
            vkCreateAndroidSurfaceKHR(instance, &createInfo, NULL, surface);
        if (r != VK_SUCCESS) {
            SDL_SetError("vkCreateAndroidSurfaceKHR failed: %i", (int)r);
            return SDL_FALSE;
        }
        return SDL_TRUE;
   }
#endif
#if SDL_VIDEO_DRIVER_UIKIT
    case SDL_SYSWM_UIKIT:
    {
        VkIOSSurfaceCreateInfoMVK createInfo;
        createInfo.sType = VK_STRUCTURE_TYPE_IOS_SURFACE_CREATE_INFO_MVK;
        createInfo.pNext = NULL;
        createInfo.flags = 0;
        // Must be a reference to a UIView object and the UIView must be
        // backed by a CALayer instance of type CAMetalLayer.
        createInfo.pView = SDL_AddMetalView(wminfo.info.uikit.window);
      
        VkResult r =
        vkCreateIOSSurfaceMVK(instance, &createInfo, NULL, surface);
        if (r != VK_SUCCESS) {
            SDL_SetError("vkCreateIOSSurfaceMVK failed: %i", (int)r);
            return SDL_FALSE;
        }
        return SDL_TRUE;
    }
#endif
#if SDL_VIDEO_DRIVER_COCOA
    case SDL_SYSWM_COCOA:
    {
        VkMacOSSurfaceCreateInfoMVK createInfo;
        createInfo.sType = VK_STRUCTURE_TYPE_MACOS_SURFACE_CREATE_INFO_MVK;
        createInfo.pNext = NULL;
        createInfo.flags = 0;
        // Must be a reference to an NSView object and the NSView must be
        // backed by a CALayer instance of type CAMetalLayer.
        createInfo.pView = SDL_AddMetalView(wminfo.info.cocoa.window);
        
        VkResult r =
        vkCreateMacOSSurfaceMVK(instance, &createInfo, NULL, surface);
        if (r != VK_SUCCESS) {
            SDL_SetError("vkCreateOSXSurfaceMVK failed: %i", (int)r);
            return SDL_FALSE;
        }
        return SDL_TRUE;
    }
#endif
#if SDL_VIDEO_DRIVER_WINDOWS
    case SDL_SYSWM_WINDOWS:
    {
        VkWin32SurfaceCreateInfoKHR createInfo;
        createInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
        createInfo.pNext = NULL;
        createInfo.flags = 0;
        createInfo.hinstance = wminfo.info.win.hdc; // XXX ???
        createInfo.hwnd = wminfo.info.win.window;

        VkResult r =
            vkCreateWin32SurfaceKHR(instance, &createInfo, NULL, surface);
        if (r != VK_SUCCESS) {
            SDL_SetError("vkCreateAndroidSurfaceKHR failed: %i", (int)r);
            return SDL_FALSE;
        }
        return SDL_TRUE;
    }
#endif
#if SDL_VIDEO_DRIVER_X11
    case SDL_SYSWM_X11:
    {
        VkXcbSurfaceCreateInfoKHR createInfo;
        createInfo.sType = VK_STRUCTURE_TYPE_XCB_SURFACE_CREATE_INFO_KHR;
        createInfo.pNext = NULL;
        createInfo.flags = 0;
        createInfo.connection = XGetXCBConnection(wminfo.info.x11.display);
        createInfo.window = wminfo.info.x11.window;

        VkResult r = vkCreateXcbSurfaceKHR(instance, &createInfo, NULL, surface);
        if (r != VK_SUCCESS) {
            SDL_SetError("vkCreateXcbSurfaceKHR failed: %i", (int)r);
            return SDL_FALSE;
        }
        return SDL_TRUE;
    }
#endif
    default:
        (void)surface;
        SDL_SetError("Unsupported subsystem %i", (int)wminfo.subsystem);
        return SDL_FALSE;
    }
}
