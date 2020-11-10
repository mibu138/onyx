#ifndef TANTO_V_VULKAN_H
#define TANTO_V_VULKAN_H

#define VK_ENABLE_BETA_EXTENSIONS
#define VK_USE_PLATFORM_XCB_KHR
#include <vulkan/vulkan.h>
#include <assert.h>

#ifndef NDEBUG
#define V_ASSERT(expr) (assert( VK_SUCCESS == expr ) )
#else
#define V_ASSERT(expr) (expr)
#endif

extern uint32_t TANTO_WINDOW_WIDTH;
extern uint32_t TANTO_WINDOW_HEIGHT;

#endif /* end of include guard: V_VULKAN_H */
