#ifndef OBDN_V_VULKAN_H
#define OBDN_V_VULKAN_H

#ifdef UNIX
#define VK_USE_PLATFORM_XCB_KHR
#else 
#define VK_USE_PLATFORM_WIN32_KHR
#endif
#include <vulkan/vulkan.h>
#include <assert.h>
#include <hell/debug.h>
#include <hell/hell.h>

static inline void obdn_VulkanErrorMessage(VkResult result)
{
    if (result != VK_SUCCESS)
    {
        switch (result)
        {
#define CASE_PRINT(r) case r: hell_DPrint(#r); hell_Print("\n"); break
            CASE_PRINT(VK_NOT_READY);
            CASE_PRINT(VK_TIMEOUT);
            CASE_PRINT(VK_EVENT_SET);
            CASE_PRINT(VK_EVENT_RESET);
            CASE_PRINT(VK_INCOMPLETE);
            CASE_PRINT(VK_ERROR_OUT_OF_HOST_MEMORY);
            CASE_PRINT(VK_ERROR_OUT_OF_DEVICE_MEMORY);
            CASE_PRINT(VK_ERROR_INITIALIZATION_FAILED);
            CASE_PRINT(VK_ERROR_DEVICE_LOST);
            CASE_PRINT(VK_ERROR_MEMORY_MAP_FAILED);
            CASE_PRINT(VK_ERROR_LAYER_NOT_PRESENT);
            CASE_PRINT(VK_ERROR_EXTENSION_NOT_PRESENT);
            CASE_PRINT(VK_ERROR_FEATURE_NOT_PRESENT);
            CASE_PRINT(VK_ERROR_INCOMPATIBLE_DRIVER);
            CASE_PRINT(VK_ERROR_TOO_MANY_OBJECTS);
            CASE_PRINT(VK_ERROR_FORMAT_NOT_SUPPORTED);
            CASE_PRINT(VK_ERROR_FRAGMENTED_POOL);
            CASE_PRINT(VK_ERROR_UNKNOWN);
            CASE_PRINT(VK_ERROR_OUT_OF_POOL_MEMORY);
            CASE_PRINT(VK_ERROR_INVALID_EXTERNAL_HANDLE);
            CASE_PRINT(VK_ERROR_FRAGMENTATION);
            CASE_PRINT(VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS);
            CASE_PRINT(VK_ERROR_SURFACE_LOST_KHR);
            CASE_PRINT(VK_ERROR_NATIVE_WINDOW_IN_USE_KHR);
            CASE_PRINT(VK_SUBOPTIMAL_KHR);
            CASE_PRINT(VK_ERROR_OUT_OF_DATE_KHR);
            CASE_PRINT(VK_ERROR_INCOMPATIBLE_DISPLAY_KHR);
            CASE_PRINT(VK_ERROR_VALIDATION_FAILED_EXT);
            CASE_PRINT(VK_ERROR_INVALID_SHADER_NV);
            CASE_PRINT(VK_ERROR_INVALID_DRM_FORMAT_MODIFIER_PLANE_LAYOUT_EXT);
            CASE_PRINT(VK_ERROR_NOT_PERMITTED_EXT);
            CASE_PRINT(VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT);
            CASE_PRINT(VK_THREAD_IDLE_KHR);
            CASE_PRINT(VK_THREAD_DONE_KHR);
            CASE_PRINT(VK_OPERATION_DEFERRED_KHR);
            CASE_PRINT(VK_OPERATION_NOT_DEFERRED_KHR);
            CASE_PRINT(VK_PIPELINE_COMPILE_REQUIRED_EXT);
#undef CASE_PRINT
        default:
            hell_DPrint("UNKNOWN ERROR");
        }
    }
    if (result < 0)
        assert(0);
}

// note VkResults >= 0 are considered success codes.
#ifndef NDEBUG
#define V_ASSERT(expr) ( obdn_VulkanErrorMessage(expr) )
#else
#define V_ASSERT(expr) (expr)
#endif

#endif /* end of include guard: V_VULKAN_H */
