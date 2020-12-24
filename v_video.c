#include "v_video.h"
#include "v_memory.h"
#include "d_display.h"
#include "t_def.h"
#include "v_loader.h"
#include <stdio.h>
#include <stdint.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>

#include <unistd.h>
#include <vulkan/vulkan_core.h>
#include <vulkan/vulkan_xcb.h>


static VkInstance instance;

VkDevice          device;
VkPhysicalDevice  physicalDevice;

uint32_t graphicsQueueFamilyIndex = UINT32_MAX; //hopefully this causes obvious errors
VkQueue  graphicsQueues[TANTO_G_QUEUE_COUNT];
VkQueue  presentQueue;

static VkSurfaceKHR      nativeSurface;
VkSurfaceKHR*    pSurface;

static VkDebugUtilsMessengerEXT debugMessenger;
    
static VkPhysicalDeviceRayTracingPipelinePropertiesKHR rtProperties;

VkPhysicalDeviceProperties deviceProperties;

VkBool32 debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
        VkDebugUtilsMessageTypeFlagsEXT messageTypes,
        const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
        void* pUserData)
{
    V1_PRINT("%s\n", pCallbackData->pMessage);
    return VK_FALSE; // application must return false;
}

static uint32_t getVkVersionAvailable(void)
{
    uint32_t v;
    vkEnumerateInstanceVersion(&v);
    uint32_t major = VK_VERSION_MAJOR(v);
    uint32_t minor = VK_VERSION_MINOR(v);
    uint32_t patch = VK_VERSION_PATCH(v);
    V1_PRINT("Vulkan Version available: %d.%d.%d\n", major, minor, patch);
    return v;
}

static void inspectAvailableLayers(void)
{
    uint32_t availableCount;
    vkEnumerateInstanceLayerProperties(&availableCount, NULL);
    VkLayerProperties propertiesAvailable[availableCount];
    vkEnumerateInstanceLayerProperties(&availableCount, propertiesAvailable);
    V1_PRINT("%s\n", "Vulkan Instance layers available:");
    const int padding = 90;
    for (int i = 0; i < padding; i++) {
        putchar('-');   
    }
    putchar('\n');
    for (int i = 0; i < availableCount; i++) {
        const char* name = propertiesAvailable[i].layerName;
        const char* desc = propertiesAvailable[i].description;
        const int pad = padding - strlen(name);
        V1_PRINT("%s%*s\n", name, pad, desc );
        for (int i = 0; i < padding; i++) {
            putchar('-');   
        }
        putchar('\n');
    }
    putchar('\n');
}

static void inspectAvailableExtensions(void)
{
    uint32_t availableCount;
    vkEnumerateInstanceExtensionProperties(NULL, &availableCount, NULL);
    VkExtensionProperties propertiesAvailable[availableCount];
    vkEnumerateInstanceExtensionProperties(NULL, &availableCount, propertiesAvailable);
    V1_PRINT("%s\n", "Vulkan Instance extensions available:");
    for (int i = 0; i < availableCount; i++) {
        V1_PRINT("%s\n", propertiesAvailable[i].extensionName);
    }
    putchar('\n');
}

static void initVkInstance(void)
{
    uint32_t vulkver = getVkVersionAvailable();
    //uint32_t vulkver = VK_MAKE_VERSION(1, 2, 0);
    //printf("Choosing vulkan version: 1.2.0\n");

    const char appName[] =    "Asteroids"; 
    const char engineName[] = "Sword";

    const VkApplicationInfo appInfo = {
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pApplicationName = appName,
        .applicationVersion = 1.0,
        .pEngineName = engineName,
        .apiVersion = vulkver,
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
        //VK_VALIDATION_FEATURE_ENABLE_DEBUG_PRINTF_EXT,
        VK_VALIDATION_FEATURE_ENABLE_GPU_ASSISTED_EXT
    };

    VkValidationFeaturesEXT extraValidation = {
        .sType = VK_STRUCTURE_TYPE_VALIDATION_FEATURES_EXT,
        .disabledValidationFeatureCount = 0,
        .enabledValidationFeatureCount = sizeof(valfeatures) / sizeof(VkValidationFeatureEnableEXT),
        .pEnabledValidationFeatures = valfeatures
    };

    const char* enabledLayers[] =     {
        "VK_LAYER_KHRONOS_validation",
        "VK_LAYER_LUNARG_monitor"
    };

    const char* enabledExtensions[] = {
        "VK_KHR_surface",
        "VK_KHR_xcb_surface",
        "VK_EXT_debug_report",
        "VK_EXT_debug_utils",
        VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME
    };

    VkInstanceCreateInfo instanceInfo = {
        .enabledLayerCount = sizeof(enabledLayers) / sizeof(char*),
        .enabledExtensionCount = sizeof(enabledExtensions) / sizeof(char*),
        .ppEnabledExtensionNames = enabledExtensions,
        .ppEnabledLayerNames = enabledLayers,
        .pApplicationInfo = &appInfo,
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pNext = &extraValidation,
    };

    if (!tanto_v_config.validationEnabled)
    {
        instanceInfo.enabledLayerCount = 0; // disables layers
    }

    V_ASSERT( vkCreateInstance(&instanceInfo, NULL, &instance) );
    V1_PRINT("Successfully initialized Vulkan instance.\n");
}

static void initDebugMessenger(void)
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
    fn = vkGetInstanceProcAddr(
            instance, 
            "vkCreateDebugUtilsMessengerEXT");

    assert(fn);

    PFN_vkCreateDebugUtilsMessengerEXT func = (PFN_vkCreateDebugUtilsMessengerEXT)fn;

    V_ASSERT( func(instance, 
            &ci, NULL, &debugMessenger) );
}

#define NVIDIA_ID 0x10DE
static VkPhysicalDevice retrievePhysicalDevice(void)
{
    uint32_t physdevcount;
    V_ASSERT( vkEnumeratePhysicalDevices(instance, &physdevcount, NULL) );
    VkPhysicalDevice devices[physdevcount];
    V_ASSERT( vkEnumeratePhysicalDevices(instance, &physdevcount, devices) );
    VkPhysicalDeviceProperties props[physdevcount];
    V1_PRINT("Physical device count: %d\n", physdevcount);
    V1_PRINT("Physical device names:\n");
    int nvidiaCardIndex = -1;
    for (int i = 0; i < physdevcount; i++) 
    {
        vkGetPhysicalDeviceProperties(devices[i], &props[i]);
        V1_PRINT("Device %d: name: %s\t vendorID: %d\n", i, props[i].deviceName, props[i].vendorID);
        if (props[i].vendorID == NVIDIA_ID) nvidiaCardIndex = i;
    }
    int selected = 0;
    if (nvidiaCardIndex != -1) selected = nvidiaCardIndex;
    V1_PRINT("Selecting Device: %s\n", props[selected].deviceName);
    deviceProperties = props[selected];
    return devices[selected];
}

static void initDevice(void)
{
    physicalDevice = retrievePhysicalDevice();
    uint32_t qfcount;

    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &qfcount, NULL);

    VkQueueFamilyProperties qfprops[qfcount];

    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &qfcount, qfprops);

    for (int i = 0; i < qfcount; i++) 
    {
        VkQueryControlFlags flags = qfprops[i].queueFlags;
        V1_PRINT("Queue Family %d: count: %d flags: ", i, qfprops[i].queueCount);
        if (flags & VK_QUEUE_GRAPHICS_BIT)  V1_PRINT(" Graphics ");
        if (flags & VK_QUEUE_COMPUTE_BIT)   V1_PRINT(" Compute ");
        if (flags & VK_QUEUE_TRANSFER_BIT)  V1_PRINT(" Tranfer ");
        V1_PRINT("\n");
    }

    graphicsQueueFamilyIndex = 0; // because we know this
    assert( TANTO_G_QUEUE_COUNT < qfprops[graphicsQueueFamilyIndex].queueCount );

    const float priorities[TANTO_G_QUEUE_COUNT] = {1.0, 1.0, 1.0, 1.0};

    const VkDeviceQueueCreateInfo qci[] = { 
        { 
            .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
            .queueFamilyIndex = graphicsQueueFamilyIndex,
            .queueCount = TANTO_G_QUEUE_COUNT,
            .pQueuePriorities = priorities,
        }
    };

    uint32_t propCount;
    V_ASSERT( vkEnumerateDeviceExtensionProperties(physicalDevice, NULL, &propCount, NULL) );
    VkExtensionProperties properties[propCount];
    V_ASSERT( vkEnumerateDeviceExtensionProperties(physicalDevice, NULL, &propCount, properties) );

    #if VERBOSE > 1
    V1_PRINT("Device Extensions available: \n");
    for (int i = 0; i < propCount; i++) 
    {
        V1_PRINT("Name: %s    Spec Version: %d\n", properties[i].extensionName, properties[i].specVersion);    
    }
    #endif

    VkPhysicalDeviceRayTracingPipelinePropertiesKHR rayTracingProps = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_PROPERTIES_KHR,
    };

    VkPhysicalDeviceProperties2 phsicalDeviceProperties2 = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2,
    };

    if (tanto_v_config.rayTraceEnabled)
        phsicalDeviceProperties2.pNext = &rayTracingProps;
    else
        phsicalDeviceProperties2.pNext = NULL;

    vkGetPhysicalDeviceProperties2(physicalDevice, &phsicalDeviceProperties2);

    if (tanto_v_config.rayTraceEnabled)
        rtProperties = rayTracingProps;

    const char* extensionsRT[] = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,
        VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME,
        VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME,
        VK_KHR_RAY_QUERY_EXTENSION_NAME,
        // Required by VK_KHR_acceleration_structure
        //VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME,
        VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME,
        //VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME,
        // Required for VK_KHR_ray_tracing_pipeline
        //VK_KHR_SPIRV_1_4_EXTENSION_NAME,
        // Required by VK_KHR_spirv_1_4
        //VK_KHR_SHADER_FLOAT_CONTROLS_EXTENSION_NAME,
        //VK_KHR_MAINTENANCE3_EXTENSION_NAME,
        // soft requirement for VK_KHR_ray_tracing_pipeline
        VK_KHR_PIPELINE_LIBRARY_EXTENSION_NAME
    };

    const char* extensionsReg[] = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };

    // Vulkan 1.0 features are specified in the 
    // features member of deviceFeatures. 
    // Newer features are enabled by chaining them 
    // on to the pNext member of deviceFeatures. 

    
    VkPhysicalDeviceBufferDeviceAddressFeaturesEXT devAddressFeatures = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES,
        .pNext = NULL
    };

    VkPhysicalDeviceAccelerationStructureFeaturesKHR accelFeatures = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR,
        .pNext = &devAddressFeatures
    };

    VkPhysicalDeviceRayTracingPipelineFeaturesKHR rtFeatures = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR,
        .pNext = &accelFeatures
    };

    // this allows us to use only a subset of a descriptor array's elements in a draw call
    // as well as unbounded descriptor arrays in shaders. 
    // this is particularly useful for arrays of textures.
    VkPhysicalDeviceDescriptorIndexingFeatures descIndexingFeatures = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES,
        .pNext = NULL
    };

    VkPhysicalDeviceFeatures2 deviceFeatures = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2,
        .pNext = &descIndexingFeatures
    };

    if (tanto_v_config.rayTraceEnabled)
        descIndexingFeatures.pNext = &rtFeatures;

    vkGetPhysicalDeviceFeatures2(physicalDevice, &deviceFeatures);

    #if VERBOSE > 0
    {
        VkBool32* iter = &rtFeatures.rayTracingPipeline;
        const char* rtMemberNames[] = {
            "rayTracingPipeline",
            "rayTracingPipelineShaderGroupHandleCaptureReplay",
            "rayTracingPipelineShaderGroupHandleCaptureReplayMixed",
            "rayTracingPipelineTraceRaysIndirect",
            "rayTraversalPrimitiveCulling",
        };
        int len = TANTO_ARRAY_SIZE(rtMemberNames);
        for (int i = 0; i < len; i++) 
        {
            printf("%s available: %s\n", rtMemberNames[i], iter[i] ? "TRUE" : "FALSE");
        }
        iter = &devAddressFeatures.bufferDeviceAddress;
        const char* daMemberNames[] = {
            "bufferDeviceAddress",
            "bufferDeviceAddressCaptureReplay",
            "bufferDeviceAddressMultiDevice",
        };
        len = TANTO_ARRAY_SIZE(daMemberNames);
        for (int i = 0; i < len; i++) 
        {
            printf("%s available: %s\n", daMemberNames[i], iter[i] ? "TRUE" : "FALSE");
        }
    } 
    #endif

    assert( VK_TRUE == deviceFeatures.features.fillModeNonSolid );

    VkPhysicalDeviceFeatures enabledFeatures = {
        .fillModeNonSolid = VK_TRUE,
        .wideLines = VK_TRUE,
        .largePoints = VK_TRUE,
        .sampleRateShading = VK_TRUE,
        .tessellationShader = VK_TRUE
    };

    deviceFeatures.features = enabledFeatures; // only enable a subset of available features

    VkDeviceCreateInfo dci = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .enabledLayerCount = 0,
        .pNext = &deviceFeatures,
        .ppEnabledLayerNames = NULL,
        .pEnabledFeatures = NULL, // not used in newer vulkan versions
        .pQueueCreateInfos = qci,
        .queueCreateInfoCount = TANTO_ARRAY_SIZE(qci),
    };

    if (tanto_v_config.rayTraceEnabled)
    {
        V1_PRINT("Ray tracing enabled...\n");
        dci.enabledExtensionCount = TANTO_ARRAY_SIZE(extensionsRT);
        dci.ppEnabledExtensionNames = extensionsRT;
    }
    else
    {
        dci.enabledExtensionCount = TANTO_ARRAY_SIZE(extensionsReg);
        dci.ppEnabledExtensionNames = extensionsReg;
    }

    V_ASSERT( vkCreateDevice(physicalDevice, &dci, NULL, &device) );
    V1_PRINT("Device created successfully.\n");
}

static void initQueues(void)
{
    for (int i = 0; i < TANTO_G_QUEUE_COUNT; i++) 
    {
        vkGetDeviceQueue(device, graphicsQueueFamilyIndex, i, &graphicsQueues[i]);
    }
    presentQueue = graphicsQueues[0]; // use the first queue to present
}

const VkInstance* tanto_v_Init(void)
{
    nativeSurface = VK_NULL_HANDLE;
    initVkInstance();
    if (tanto_v_config.validationEnabled)
        initDebugMessenger();
    initDevice();
    tanto_v_LoadFunctions(&device);
    initQueues();
    tanto_v_InitMemory();
    printf("Tanto Video initialized.\n");
    return &instance;
}

void tanto_v_InitSurfaceXcb(xcb_connection_t* connection, xcb_window_t window) 
{
    const VkXcbSurfaceCreateInfoKHR ci = {
        .sType = VK_STRUCTURE_TYPE_XCB_SURFACE_CREATE_INFO_KHR,
        .connection = connection,
        .window = window,
    };

    V_ASSERT( vkCreateXcbSurfaceKHR(instance, &ci, NULL, &nativeSurface) );
    V1_PRINT("Surface created successfully.\n");
    pSurface = &nativeSurface;
}

void tanto_v_SubmitToQueue(const VkCommandBuffer* cmdBuf, const Tanto_V_QueueType queueType, const uint32_t index)
{
    assert( TANTO_V_QUEUE_GRAPHICS_TYPE == queueType );
    assert( TANTO_G_QUEUE_COUNT > index );

    const VkSubmitInfo info = {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .commandBufferCount = 1,
        .signalSemaphoreCount = 0,
        .waitSemaphoreCount = 0,
        .pCommandBuffers = cmdBuf
    };

    V_ASSERT( vkQueueSubmit(graphicsQueues[index], 1, &info, VK_NULL_HANDLE) );
}

void tanto_v_SubmitToQueueWait(const VkCommandBuffer* buffer, const Tanto_V_QueueType type, const uint32_t queueIndex)
{
    tanto_v_SubmitToQueue(buffer, type, queueIndex);
    V_ASSERT( vkQueueWaitIdle(graphicsQueues[queueIndex]) );
}

void tanto_v_CleanUp(void)
{
    PFN_vkDestroyDebugUtilsMessengerEXT vkDestroyDebugUtilsMessengerEXT = (PFN_vkDestroyDebugUtilsMessengerEXT)
        vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
        
    tanto_v_CleanUpMemory();
    if (nativeSurface != VK_NULL_HANDLE)
        vkDestroySurfaceKHR(instance, nativeSurface, NULL);
    vkDestroyDevice(device, NULL);
    vkDestroyDebugUtilsMessengerEXT(instance, debugMessenger, NULL);
    vkDestroyInstance(instance, NULL);
}

VkPhysicalDeviceRayTracingPipelinePropertiesKHR tanto_v_GetPhysicalDeviceRayTracingProperties(void)
{
    return rtProperties;
}

uint32_t tanto_v_GetQueueFamilyIndex(Tanto_V_QueueType type)
{
    switch (type)
    {
        case TANTO_V_QUEUE_GRAPHICS_TYPE: return graphicsQueueFamilyIndex;
        case TANTO_V_QUEUE_COMPUTE_TYPE:  assert(0); return -1; //not supported yet
    }
    return -1;
}

VkDevice tanto_v_GetDevice(void)
{
    return device;
}
