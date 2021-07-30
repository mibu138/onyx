#ifndef OBDN_V_VULKAN_H
#define OBDN_V_VULKAN_H

#ifdef UNIX
#define VK_USE_PLATFORM_XCB_KHR
#else 
#define VK_USE_PLATFORM_WIN32_KHR
#endif
#include <vulkan/vulkan.h>
#include <assert.h>

#ifndef NDEBUG
#define V_ASSERT(expr) (assert( VK_SUCCESS == expr ) )
#else
#define V_ASSERT(expr) (expr)
#endif

#endif /* end of include guard: V_VULKAN_H */
