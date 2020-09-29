#ifndef V_VULKAN_H
#define V_VULKAN_H

#define VK_ENABLE_BETA_EXTENSIONS
#define VK_USE_PLATFORM_XCB_KHR
#include <vulkan/vulkan.h>
#include <assert.h>

#define V_ASSERT(expr) (assert( VK_SUCCESS == expr ) )

#define WINDOW_WIDTH  1000
#define WINDOW_HEIGHT 1000

#endif /* end of include guard: V_VULKAN_H */
