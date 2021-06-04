#include "v_video.h"
#include "v_private.h"
#include "common.h"
#include "dtags.h"
#include "v_command.h"
#include "v_loader.h"
#include "v_memory.h"
#include "v_vulkan.h"
#include <assert.h>
#include <hell/attributes.h>
#include <hell/common.h>
#include <hell/debug.h>
#include <hell/len.h>
#include <hell/platform.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <unistd.h>

static void inspectAvailableLayers(void) __attribute__((unused));
static void inspectAvailableExtensions(void) __attribute__((unused));

#define DPRINT_VK(fmt, ...)                                                    \
    hell_DebugPrint(OBDN_DEBUG_TAG_VK, fmt, ##__VA_ARGS__)

static VkBool32
debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT      messageSeverity,
              VkDebugUtilsMessageTypeFlagsEXT             messageTypes,
              const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
              void*                                       pUserData)
{
    DPRINT_VK("%s\n", pCallbackData->pMessage);
    return VK_FALSE; // application must return false;
}

static void
inspectAvailableLayers(void)
{
    uint32_t availableCount;
    vkEnumerateInstanceLayerProperties(&availableCount, NULL);
    VkLayerProperties propertiesAvailable[availableCount];
    vkEnumerateInstanceLayerProperties(&availableCount, propertiesAvailable);
    obdn_Announce("%s\n", "Vulkan Instance layers available:");
    const int padding = 90;
    for (int i = 0; i < padding; i++)
    {
        hell_Print("-");
    }
    hell_Print("\n");
    for (int i = 0; i < availableCount; i++)
    {
        const char*      name   = propertiesAvailable[i].layerName;
        const char* desc UNUSED = propertiesAvailable[i].description;
        const int pad    UNUSED = padding - strlen(name);
        hell_Print("%s%*s\n", name, pad, desc);
        for (int i = 0; i < padding; i++)
        {
            hell_Print("-");
        }
        hell_Print("\n");
    }
    hell_Print("\n");
}

static void
inspectAvailableExtensions(void)
{
    uint32_t availableCount;
    vkEnumerateInstanceExtensionProperties(NULL, &availableCount, NULL);
    VkExtensionProperties propertiesAvailable[availableCount];
    vkEnumerateInstanceExtensionProperties(NULL, &availableCount,
                                           propertiesAvailable);
    obdn_Announce("%s\n", "Vulkan Instance extensions available:");
    for (int i = 0; i < availableCount; i++)
    {
        hell_Print("%s\n", propertiesAvailable[i].extensionName);
    }
    hell_Print("\n");
}

static uint32_t
getVkVersionAvailable(void)
{
    uint32_t v;
    vkEnumerateInstanceVersion(&v);
    uint32_t major UNUSED = VK_VERSION_MAJOR(v);
    uint32_t minor UNUSED = VK_VERSION_MINOR(v);
    uint32_t patch UNUSED = VK_VERSION_PATCH(v);
    obdn_Announce("Vulkan Version available: %d.%d.%d\n", major, minor, patch);
    return v;
}

static void
initVkInstance(const bool enableValidation, VkInstance* instance)
{
    uint32_t vulkver = getVkVersionAvailable();
    // uint32_t vulkver = VK_MAKE_VERSION(1, 2, 0);
    // printf("Choosing vulkan version: 1.2.0\n");

    const char appName[]    = "Hell";
    const char engineName[] = "Obsidian";

    const VkApplicationInfo appInfo = {
        .sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pApplicationName   = appName,
        .applicationVersion = 1.0,
        .pEngineName        = engineName,
        .apiVersion         = vulkver,
    };

#if VERBOSE > 1
    inspectAvailableLayers();
    inspectAvailableExtensions();
#endif

    // one for best practices
    // second one is interesting, sounds like it allows
    // V1_PRINT to be called from shaders.
    const VkValidationFeatureEnableEXT valfeatures[] = {
#ifndef NO_BEST_PRACTICE
        VK_VALIDATION_FEATURE_ENABLE_BEST_PRACTICES_EXT,
#endif
        // VK_VALIDATION_FEATURE_ENABLE_DEBUG_PRINTF_EXT,
        VK_VALIDATION_FEATURE_ENABLE_GPU_ASSISTED_EXT};

    VkValidationFeaturesEXT extraValidation = {
        .sType = VK_STRUCTURE_TYPE_VALIDATION_FEATURES_EXT,
        .disabledValidationFeatureCount = 0,
        .enabledValidationFeatureCount =
            sizeof(valfeatures) / sizeof(VkValidationFeatureEnableEXT),
        .pEnabledValidationFeatures = valfeatures};

    const char* enabledLayers[] = {"VK_LAYER_KHRONOS_validation",
                                   "VK_LAYER_LUNARG_monitor"};

    const char* enabledExtensions[] = {
        "VK_KHR_surface",
#ifdef UNIX
        "VK_KHR_xcb_surface",
#elif defined(WINDOWS)
        "VK_KHR_win32_surface",
#else
#error Unsupported platform
#endif
        "VK_EXT_debug_report", "VK_EXT_debug_utils",
        VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME};

    VkInstanceCreateInfo instanceInfo = {
        .enabledLayerCount       = sizeof(enabledLayers) / sizeof(char*),
        .enabledExtensionCount   = sizeof(enabledExtensions) / sizeof(char*),
        .ppEnabledExtensionNames = enabledExtensions,
        .ppEnabledLayerNames     = enabledLayers,
        .pApplicationInfo        = &appInfo,
        .sType                   = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pNext                   = &extraValidation,
    };

    if (!enableValidation)
    {
        instanceInfo.enabledLayerCount = 0; // disables layers
    }

    V_ASSERT(vkCreateInstance(&instanceInfo, NULL, instance));
    obdn_Announce("Vulkan Instance initilized.\n");
}

static void
initDebugMessenger(const VkInstance          instance,
                   VkDebugUtilsMessengerEXT* debugMessenger)
{
    const VkDebugUtilsMessengerCreateInfoEXT ci = {
        .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
        .messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT |
                           VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT |
#if VERBOSE > 1
                           VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
#endif
                           VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT,
        .messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                       VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT |
                       VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT,
        .pfnUserCallback = debugCallback,
    };

    PFN_vkVoidFunction fn;
    fn = vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");

    assert(fn);

    PFN_vkCreateDebugUtilsMessengerEXT func =
        (PFN_vkCreateDebugUtilsMessengerEXT)fn;

    V_ASSERT(func(instance, &ci, NULL, debugMessenger));
}

#define NVIDIA_ID 0x10DE
static VkPhysicalDevice
retrievePhysicalDevice(const VkInstance            instance,
                       VkPhysicalDeviceProperties* deviceProperties)
{
    uint32_t physdevcount;
    V_ASSERT(vkEnumeratePhysicalDevices(instance, &physdevcount, NULL));
    VkPhysicalDevice devices[physdevcount];
    V_ASSERT(vkEnumeratePhysicalDevices(instance, &physdevcount, devices));
    VkPhysicalDeviceProperties props[physdevcount];
    DPRINT_VK("Physical device count: %d\n", physdevcount);
    DPRINT_VK("Physical device names:\n");
    int nvidiaCardIndex = -1;
    for (int i = 0; i < physdevcount; i++)
    {
        vkGetPhysicalDeviceProperties(devices[i], &props[i]);
        DPRINT_VK("Device %d: name: %s\t vendorID: %d\n", i,
                  props[i].deviceName, props[i].vendorID);
        if (props[i].vendorID == NVIDIA_ID)
            nvidiaCardIndex = i;
    }
    int selected = 0;
    if (nvidiaCardIndex != -1)
        selected = nvidiaCardIndex;
    obdn_Announce("Selecting Device: %s\n", props[selected].deviceName);
    *deviceProperties = props[selected];
    return devices[selected];
}

static void
initDevice(
    const bool enableRayTracing, const uint32_t userExtCount,
    const char*            userExtensions[userExtCount],
    const VkPhysicalDevice physicalDevice, QueueFamily* graphicsQueueFamily,
    QueueFamily* computeQueueFamily, QueueFamily* transferQueueFamily,
    VkPhysicalDeviceRayTracingPipelinePropertiesKHR*    rtProperties,
    VkPhysicalDeviceAccelerationStructurePropertiesKHR* accelStructProperties,
    VkDevice*                                           device)
{
    graphicsQueueFamily->queueCount = UINT32_MAX;
    transferQueueFamily->queueCount = UINT32_MAX;
    computeQueueFamily->queueCount  = UINT32_MAX;
    uint32_t qfcount;

    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &qfcount, NULL);

    VkQueueFamilyProperties qfprops[qfcount];

    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &qfcount, qfprops);

    int8_t transferQueueTicker = 2, computeQueueTicker = 2;
    for (int i = 0; i < qfcount; i++)
    {
        VkQueryControlFlags flags = qfprops[i].queueFlags;
        obdn_Announce("Queue Family %d: count: %d flags: ", i,
                  qfprops[i].queueCount);
        if (flags & VK_QUEUE_GRAPHICS_BIT)
            hell_Print(" Graphics ");
        if (flags & VK_QUEUE_COMPUTE_BIT)
            hell_Print(" Compute ");
        if (flags & VK_QUEUE_TRANSFER_BIT)
            hell_Print(" Tranfer ");
        hell_Print("\n");
        if (flags & VK_QUEUE_GRAPHICS_BIT &&
            graphicsQueueFamily->index == 0) // first graphics queue
        {
            graphicsQueueFamily->queueCount = qfprops[i].queueCount;
            graphicsQueueFamily->index      = i;
        }
        if (flags & VK_QUEUE_TRANSFER_BIT &&
            transferQueueTicker >
                0) // TODO: this a blind heuristic. get more info.
        {
            transferQueueFamily->queueCount = qfprops[i].queueCount;
            transferQueueFamily->index      = i;
            transferQueueTicker--;
        }
        if (flags & VK_QUEUE_COMPUTE_BIT &&
            computeQueueTicker >
                0) // TODO: this is a blind heuristic. get more info.
        {
            computeQueueFamily->queueCount = qfprops[i].queueCount;
            computeQueueFamily->index      = i;
            computeQueueTicker--;
        }
    }

    DPRINT_VK("Graphics Queue family index: %d\n", graphicsQueueFamily->index);
    DPRINT_VK("Transfer Queue family index: %d\n", transferQueueFamily->index);
    DPRINT_VK("Compute  Queue family index: %d\n", computeQueueFamily->index);

    assert(graphicsQueueFamily->queueCount < MAX_QUEUES);
    assert(transferQueueFamily->queueCount < MAX_QUEUES);
    assert(computeQueueFamily->queueCount < MAX_QUEUES);

    float graphicsPriorities[MAX_QUEUES];
    float transferPriorities[MAX_QUEUES];
    float computePriorities[MAX_QUEUES];

    for (int i = 0; i < graphicsQueueFamily->queueCount; i++)
    {
        graphicsPriorities[i] = 1.0;
    }
    for (int i = 0; i < transferQueueFamily->queueCount; i++)
    {
        transferPriorities[i] = 1.0;
    }
    for (int i = 0; i < computeQueueFamily->queueCount; i++)
    {
        computePriorities[i] = 1.0;
    }

    // TODO: test what would happen if families alias eachother.
    const VkDeviceQueueCreateInfo qci[] = {
        {
            .sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
            .queueFamilyIndex = graphicsQueueFamily->index,
            .queueCount       = graphicsQueueFamily->queueCount,
            .pQueuePriorities = graphicsPriorities,
        },
        {
            .sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
            .queueFamilyIndex = transferQueueFamily->index,
            .queueCount       = transferQueueFamily->queueCount,
            .pQueuePriorities = transferPriorities,
        },
        {
            .sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
            .queueFamilyIndex = computeQueueFamily->index,
            .queueCount       = computeQueueFamily->queueCount,
            .pQueuePriorities = computePriorities,
        }};

    uint32_t propCount;
    V_ASSERT(vkEnumerateDeviceExtensionProperties(physicalDevice, NULL,
                                                  &propCount, NULL));
    VkExtensionProperties properties[propCount];
    V_ASSERT(vkEnumerateDeviceExtensionProperties(physicalDevice, NULL,
                                                  &propCount, properties));

#if VERBOSE > 1
    V1_PRINT("Device Extensions available: \n");
    for (int i = 0; i < propCount; i++)
    {
        DPRINT_VK("Name: %s    Spec Version: %d\n", properties[i].extensionName,
                  properties[i].specVersion);
    }
#endif

    VkPhysicalDeviceAccelerationStructurePropertiesKHR asProps = {
        .sType =
            VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_PROPERTIES_KHR,
    };

    VkPhysicalDeviceRayTracingPipelinePropertiesKHR rayTracingProps = {
        .sType =
            VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_PROPERTIES_KHR,
        .pNext = &asProps};

    VkPhysicalDeviceProperties2 phsicalDeviceProperties2 = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2,
    };

    if (enableRayTracing)
        phsicalDeviceProperties2.pNext = &rayTracingProps;
    else
        phsicalDeviceProperties2.pNext = NULL;

    vkGetPhysicalDeviceProperties2(physicalDevice, &phsicalDeviceProperties2);

    if (enableRayTracing)
    {
        *rtProperties          = rayTracingProps;
        *accelStructProperties = asProps;
    }

    const char* extensionsRT[] = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,
        VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME,
        VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME,
        VK_KHR_RAY_QUERY_EXTENSION_NAME,
        // Required by VK_KHR_acceleration_structure
        // VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME,
        VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME,
        // VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME,
        // Required for VK_KHR_ray_tracing_pipeline
        // VK_KHR_SPIRV_1_4_EXTENSION_NAME,
        // Required by VK_KHR_spirv_1_4
        // VK_KHR_SHADER_FLOAT_CONTROLS_EXTENSION_NAME,
        // VK_KHR_MAINTENANCE3_EXTENSION_NAME,
        // soft requirement for VK_KHR_ray_tracing_pipeline
        VK_KHR_PIPELINE_LIBRARY_EXTENSION_NAME};

    const char* extensionsReg[] = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};

    // Vulkan 1.0 features are specified in the
    // features member of deviceFeatures.
    // Newer features are enabled by chaining them
    // on to the pNext member of deviceFeatures.

    VkPhysicalDeviceScalarBlockLayoutFeatures scalarBlockLayoutFeatures = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SCALAR_BLOCK_LAYOUT_FEATURES,
        .pNext = NULL};

    VkPhysicalDeviceAccelerationStructureFeaturesKHR accelFeatures = {
        .sType =
            VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR,
        .pNext = &scalarBlockLayoutFeatures};

    VkPhysicalDeviceRayTracingPipelineFeaturesKHR rtFeatures = {
        .sType =
            VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR,
        .pNext = &accelFeatures};

    VkPhysicalDeviceBufferDeviceAddressFeaturesEXT devAddressFeatures = {
        .sType =
            VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES,
        .pNext = NULL};

    VkPhysicalDeviceDescriptorIndexingFeatures descIndexingFeatures = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES,
        .pNext = &devAddressFeatures,
    };

    VkPhysicalDeviceFeatures2 deviceFeatures = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2,
        .pNext = &descIndexingFeatures};

    if (enableRayTracing)
        devAddressFeatures.pNext = &rtFeatures;

    vkGetPhysicalDeviceFeatures2(physicalDevice, &deviceFeatures);

#if VERBOSE > 0
    {
        VkBool32*   iter            = &rtFeatures.rayTracingPipeline;
        const char* rtMemberNames[] = {
            "rayTracingPipeline",
            "rayTracingPipelineShaderGroupHandleCaptureReplay",
            "rayTracingPipelineShaderGroupHandleCaptureReplayMixed",
            "rayTracingPipelineTraceRaysIndirect",
            "rayTraversalPrimitiveCulling",
        };
        int len = LEN(rtMemberNames);
        for (int i = 0; i < len; i++)
        {
            DPRINT_VK("%s available: %s\n", rtMemberNames[i],
                      iter[i] ? "TRUE" : "FALSE");
        }
        iter                        = &devAddressFeatures.bufferDeviceAddress;
        const char* daMemberNames[] = {
            "bufferDeviceAddress",
            "bufferDeviceAddressCaptureReplay",
            "bufferDeviceAddressMultiDevice",
        };
        len = LEN(daMemberNames);
        for (int i = 0; i < len; i++)
        {
            DPRINT_VK("%s available: %s\n", daMemberNames[i],
                      iter[i] ? "TRUE" : "FALSE");
        }
    }
#endif

    assert(VK_TRUE == deviceFeatures.features.fillModeNonSolid);

    VkPhysicalDeviceFeatures enabledFeatures = {
        .fillModeNonSolid   = VK_TRUE,
        .wideLines          = VK_TRUE,
        .largePoints        = VK_TRUE,
        .sampleRateShading  = VK_TRUE,
        .tessellationShader = VK_TRUE,
        .samplerAnisotropy  = VK_TRUE,
    };

    deviceFeatures.features =
        enabledFeatures; // only enable a subset of available features

    int          defExtCount = 0;
    const char** defaultExtNames;
    if (enableRayTracing)
    {
        defExtCount += LEN(extensionsRT);
        defaultExtNames = extensionsRT;
    }
    else
    {
        defExtCount += LEN(extensionsReg);
        defaultExtNames = extensionsReg;
    }

    int  extCount = userExtCount + defExtCount;
    char extNamesData[extCount][VK_MAX_EXTENSION_NAME_SIZE];

    for (int i = 0; i < defExtCount; i++)
        strcpy(extNamesData[i], defaultExtNames[i]);
    for (int i = 0; i < userExtCount; i++)
        strcpy(extNamesData[i + defExtCount], userExtensions[i]);

    const char* extNames[extCount];
    for (int i = 0; i < extCount; i++)
    {
        extNames[i] = extNamesData[i];
    }

    DPRINT_VK("Requesting extensions...\n");
    for (int i = 0; i < extCount; i++)
    {
        DPRINT_VK("%s\n", extNames[i]);
    }

    VkDeviceCreateInfo dci = {
        .sType                   = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .enabledLayerCount       = 0,
        .enabledExtensionCount   = extCount,
        .ppEnabledExtensionNames = extNames,
        .pNext                   = &deviceFeatures,
        .ppEnabledLayerNames     = NULL,
        .pEnabledFeatures        = NULL, // not used in newer vulkan versions
        .pQueueCreateInfos       = qci,
        .queueCreateInfoCount    = LEN(qci),
    };

    V_ASSERT(vkCreateDevice(physicalDevice, &dci, NULL, device));
    obdn_Announce("Vulkan Device created successfully.\n");
}

static void
initQueues(const VkDevice device, QueueFamily* graphicsQueueFamily,
           QueueFamily* computeQueueFamily, QueueFamily* transferQueueFamily,
           VkQueue* presentQueue)
{
    for (int i = 0; i < graphicsQueueFamily->queueCount; i++)
    {
        vkGetDeviceQueue(device, graphicsQueueFamily->index, i,
                         &graphicsQueueFamily->queues[i]);
    }
    for (int i = 0; i < transferQueueFamily->queueCount; i++)
    {
        vkGetDeviceQueue(device, transferQueueFamily->index, i,
                         &transferQueueFamily->queues[i]);
    }
    for (int i = 0; i < computeQueueFamily->queueCount; i++)
    {
        vkGetDeviceQueue(device, computeQueueFamily->index, i,
                         &computeQueueFamily->queues[i]);
    }
    *presentQueue =
        graphicsQueueFamily->queues[0]; // use the first queue to present
    obdn_Announce("Obsidian: Queues Initialized\n");
}

void
obdn_CreateInstance(bool enableValidation, bool enableRayTracing, const int extcount,
                  const char* extensions[], Obdn_Instance* instance)
{
    memset(instance, 0, sizeof(Obdn_Instance));
    initVkInstance(enableValidation, &instance->vkinstance);
    if (enableValidation)
        initDebugMessenger(instance->vkinstance, &instance->debugMessenger);
    instance->physicalDevice = retrievePhysicalDevice(
        instance->vkinstance, &instance->deviceProperties);
    initDevice(enableRayTracing, extcount, extensions, instance->physicalDevice,
               &instance->graphicsQueueFamily, &instance->computeQueueFamily,
               &instance->transferQueueFamily, &instance->rtProperties,
               &instance->accelStructProperties, &instance->device);
    if (enableRayTracing) // TODO not all functions have to do with
        obdn_v_LoadFunctions(instance->device);
    initQueues(instance->device, &instance->graphicsQueueFamily,
               &instance->computeQueueFamily, &instance->transferQueueFamily,
               &instance->presentQueue);
    obdn_Announce("Initialized Obsidian Instance.\n");
}

void
obdn_SubmitToQueue(const Obdn_Instance* instance, const VkCommandBuffer* cmdBuf,
                   const Obdn_V_QueueType queueType, const uint32_t index)
{
    assert(OBDN_V_QUEUE_GRAPHICS_TYPE == queueType);
    assert(instance->graphicsQueueFamily.queueCount > index);

    const VkSubmitInfo info = {.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
                               .commandBufferCount   = 1,
                               .signalSemaphoreCount = 0,
                               .waitSemaphoreCount   = 0,
                               .pCommandBuffers      = cmdBuf};

    V_ASSERT(vkQueueSubmit(instance->graphicsQueueFamily.queues[index], 1,
                           &info, VK_NULL_HANDLE));
}

void
obdn_SubmitToQueueWait(const Obdn_Instance*   instance,
                       const VkCommandBuffer* buffer,
                       const Obdn_V_QueueType type, const uint32_t queueIndex)
{
    obdn_SubmitToQueue(instance, buffer, type, queueIndex);
    V_ASSERT(vkQueueWaitIdle(instance->graphicsQueueFamily.queues[queueIndex]));
}

void
obdn_DestroyInstance(Obdn_Instance* instance)
{
    PFN_vkDestroyDebugUtilsMessengerEXT vkDestroyDebugUtilsMessengerEXT =
        (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
            instance->vkinstance, "vkDestroyDebugUtilsMessengerEXT");

    vkDestroyDevice(instance->device, NULL);
    if (instance->debugMessenger != VK_NULL_HANDLE)
        vkDestroyDebugUtilsMessengerEXT(instance->vkinstance,
                                        instance->debugMessenger, NULL);
    vkDestroyInstance(instance->vkinstance, NULL);
    obdn_Announce("Cleaned up.\n");
}

VkPhysicalDeviceRayTracingPipelinePropertiesKHR
obdn_GetPhysicalDeviceRayTracingProperties(const Obdn_Instance* instance)
{
    return instance->rtProperties;
}

uint32_t
obdn_GetQueueFamilyIndex(const Obdn_Instance* instance, Obdn_V_QueueType type)
{
    switch (type)
    {
    case OBDN_V_QUEUE_GRAPHICS_TYPE:
        return instance->graphicsQueueFamily.index;
    case OBDN_V_QUEUE_TRANSFER_TYPE:
        return instance->transferQueueFamily.index;
    case OBDN_V_QUEUE_COMPUTE_TYPE:
        assert(0);
        return -1; // not supported yet
    }
    return -1;
}

VkDevice
obdn_GetDevice(const Obdn_Instance* instance)
{
    return instance->device;
}

VkQueue
obdn_GetPresentQueue(const Obdn_Instance* instance)
{
    return instance->presentQueue;
}

void
obdn_SubmitGraphicsCommands(const Obdn_Instance* instance,
                            const uint32_t       queueIndex,
                            const uint32_t       submitInfoCount,
                            const VkSubmitInfo* submitInfos, VkFence fence)
{
    V_ASSERT(vkQueueSubmit(instance->graphicsQueueFamily.queues[queueIndex],
                           submitInfoCount, submitInfos, fence));
}

void
obdn_SubmitGraphicsCommand(const Obdn_Instance*       instance,
                           const uint32_t             queueIndex,
                           const VkPipelineStageFlags waitDstStageMask,
                           uint32_t                   waitCount,
                           VkSemaphore                waitSemephores[waitCount],
                           uint32_t                   signalCount,
                           VkSemaphore signalSemphores[signalCount],
                           VkFence fence, const VkCommandBuffer cmdBuf)
{
    VkSubmitInfo si = {.sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO,
                       .pWaitDstStageMask    = &waitDstStageMask,
                       .waitSemaphoreCount   = waitCount,
                       .pWaitSemaphores      = waitSemephores,
                       .signalSemaphoreCount = signalCount,
                       .pSignalSemaphores    = signalSemphores,
                       .commandBufferCount   = 1,
                       .pCommandBuffers      = &cmdBuf};

    V_ASSERT(vkQueueSubmit(instance->graphicsQueueFamily.queues[queueIndex], 1,
                           &si, fence));
}

void
obdn_SubmitTransferCommand(const Obdn_Instance*       instance,
                           const uint32_t             queueIndex,
                           const VkPipelineStageFlags waitDstStageMask,
                           const VkSemaphore* pWaitSemephore, VkFence fence,
                           const Obdn_V_Command* cmd)
{
    VkSubmitInfo si = {
        .sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .pWaitDstStageMask    = &waitDstStageMask,
        .waitSemaphoreCount   = pWaitSemephore == NULL ? 0 : 1,
        .pWaitSemaphores      = pWaitSemephore,
        .signalSemaphoreCount = 1,
        .pSignalSemaphores    = &cmd->semaphore,
        .commandBufferCount   = 1,
        .pCommandBuffers      = &cmd->buffer,
    };

    V_ASSERT(vkQueueSubmit(instance->transferQueueFamily.queues[queueIndex], 1,
                           &si, fence));
}

VkPhysicalDevice
obdn_GetPhysicalDevice(const Obdn_Instance* instance)
{
    return instance->physicalDevice;
}

const VkPhysicalDeviceProperties*
obdn_GetPhysicalDeviceProperties(const Obdn_Instance* instance)
{
    return &instance->deviceProperties;
}

VkPhysicalDeviceAccelerationStructurePropertiesKHR
obdn_GetPhysicalDeviceAccelerationStructureProperties(
    const Obdn_Instance* instance)
{
    return instance->accelStructProperties;
}


const VkInstance*
obdn_GetVkInstance(const Obdn_Instance* instance)
{
    return &instance->vkinstance;
}

void
obdn_PresentQueueWaitIdle(const Obdn_Instance* instance)
{
    vkQueueWaitIdle(instance->presentQueue);
}

void
obdn_DeviceWaitIdle(const Obdn_Instance* instance)
{
    vkDeviceWaitIdle(instance->device);
}

uint64_t 
obdn_SizeOfInstance(void)
{
    return sizeof(Obdn_Instance);
}

Obdn_Instance* obdn_AllocInstance(void)
{
    return hell_Malloc(sizeof(Obdn_Instance));
}
