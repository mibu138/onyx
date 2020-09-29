#include "def.h"
#include "v_video.h"
#include "v_memory.h"
#include "d_display.h"
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
VkQueue  graphicsQueues[G_QUEUE_COUNT];
VkQueue  presentQueue;

static VkSurfaceKHR      nativeSurface;
static VkSurfaceKHR*     pSurface;
VkSwapchainKHR   swapchain;

VkImage        swapchainImages[FRAME_COUNT];
const VkFormat swapFormat = VK_FORMAT_B8G8R8A8_SRGB;

VkSemaphore    imageAcquiredSemaphores[FRAME_COUNT];
uint64_t       frameCounter = 0;

static VkDebugUtilsMessengerEXT debugMessenger;
    
static VkPhysicalDeviceRayTracingPropertiesKHR rtProperties;

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
        VK_VALIDATION_FEATURE_ENABLE_BEST_PRACTICES_EXT,
        //VK_VALIDATION_FEATURE_ENABLE_DEBUG_PRINTF_EXT,
        VK_VALIDATION_FEATURE_ENABLE_GPU_ASSISTED_EXT
    };

    const VkValidationFeaturesEXT extraValidation = {
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

    const VkInstanceCreateInfo instanceInfo = {
        .enabledLayerCount = sizeof(enabledLayers) / sizeof(char*),
        .enabledExtensionCount = sizeof(enabledExtensions) / sizeof(char*),
        .ppEnabledExtensionNames = enabledExtensions,
        .ppEnabledLayerNames = enabledLayers,
        .pApplicationInfo = &appInfo,
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pNext = &extraValidation,
    };

    VkResult result = vkCreateInstance(&instanceInfo, NULL, &instance);
    assert(result == VK_SUCCESS);
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

    VkResult r = func(instance, 
            &ci, NULL, &debugMessenger);
    assert(r == VK_SUCCESS);
}

static VkPhysicalDevice retrievePhysicalDevice(void)
{
    uint32_t physdevcount;
    VkResult r = vkEnumeratePhysicalDevices(instance, &physdevcount, NULL);
    assert(r == VK_SUCCESS);
    VkPhysicalDevice devices[physdevcount];
    r = vkEnumeratePhysicalDevices(instance, &physdevcount, devices);
    assert(r == VK_SUCCESS);
    VkPhysicalDeviceProperties props[physdevcount];
    V1_PRINT("Physical device count: %d\n", physdevcount);
    V1_PRINT("Physical device names:\n");
    for (int i = 0; i < physdevcount; i++) 
    {
        vkGetPhysicalDeviceProperties(devices[i], &props[i]);
        V1_PRINT("%s\n", props[i].deviceName);
    }
    V1_PRINT("Selecting Device: %s\n", props[1].deviceName);
    return devices[1];
}

static void initDevice(void)
{
    physicalDevice = retrievePhysicalDevice();
    VkResult r;
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
    assert( G_QUEUE_COUNT < qfprops[graphicsQueueFamilyIndex].queueCount );

    const float priorities[G_QUEUE_COUNT] = {1.0, 1.0, 1.0, 1.0};

    const VkDeviceQueueCreateInfo qci[] = { 
        { 
            .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
            .queueFamilyIndex = graphicsQueueFamilyIndex,
            .queueCount = G_QUEUE_COUNT,
            .pQueuePriorities = priorities,
        }
    };

    uint32_t propCount;
    r = vkEnumerateDeviceExtensionProperties(physicalDevice, NULL, &propCount, NULL);
    assert(r == VK_SUCCESS);
    VkExtensionProperties properties[propCount];
    r = vkEnumerateDeviceExtensionProperties(physicalDevice, NULL, &propCount, properties);
    assert(r == VK_SUCCESS);

#if VERBOSE > 1
    V1_PRINT("Device Extensions available: \n");
    for (int i = 0; i < propCount; i++) 
    {
        V1_PRINT("Name: %s    Spec Version: %d\n", properties[i].extensionName, properties[i].specVersion);    
    }
#endif

#if RAY_TRACE 
    VkPhysicalDeviceRayTracingPropertiesKHR rayTracingProps = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PROPERTIES_KHR,
    };

    VkPhysicalDeviceProperties2 phsicalDeviceProperties2 = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2,
        .pNext = &rayTracingProps,
    };

    vkGetPhysicalDeviceProperties2(physicalDevice, &phsicalDeviceProperties2);
    rtProperties = rayTracingProps;
#endif

    const char* extensions[] = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,
#if RAY_TRACE
        VK_KHR_RAY_TRACING_EXTENSION_NAME,
        //VK_KHR_MAINTENANCE3_EXTENSION_NAME,
        VK_KHR_PIPELINE_LIBRARY_EXTENSION_NAME,
        VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME,
#endif
        //VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME
    };

    // Vulkan 1.0 features are specified in the 
    // features member of deviceFeatures. 
    // Newer features are enabled by chaining them 
    // on to the pNext member of deviceFeatures. 
    
#if RAY_TRACE
    VkPhysicalDeviceBufferDeviceAddressFeaturesEXT devAddressFeatures = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES_EXT,
        .pNext = NULL
    };

    VkPhysicalDeviceRayTracingFeaturesKHR rtFeatures = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_FEATURES_KHR,
        .pNext = &devAddressFeatures
    };

    VkPhysicalDeviceFeatures2 deviceFeatures = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2,
        .pNext = &rtFeatures
    };
#else 
    VkPhysicalDeviceFeatures2 deviceFeatures = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2,
        .pNext = NULL
    };
#endif

    vkGetPhysicalDeviceFeatures2(physicalDevice, &deviceFeatures);

    #if VERBOSE > 1
    {
        VkBool32* rtMembers = &rtFeatures.rayTracing;
        const char* memberNames[9] = {
            "rayTracing", 
            "rayTracingShaderGroupHandleCaptureReplay",
            "rayTracingShaderGroupHandleCaptureReplayMixed",
            "rayTracingAccelerationStructureCaptureReplay",
            "rayTracingIndirectTraceRays",
            "rayTracingIndirectAccelerationStructureBuild",
            "rayTracingHostAccelerationStructureCommands",
            "rayQuery",
            "rayTracingPrimitiveCulling"
        };
        for (int i = 0; i < 9; i++) 
        {
            printf("%s available: %s\n", memberNames[i], rtMembers[i] ? "TRUE" : "FALSE");
        }
    } 
    #endif

    assert( VK_TRUE == deviceFeatures.features.fillModeNonSolid );

    VkPhysicalDeviceFeatures enabledFeatures = {
        .fillModeNonSolid = VK_TRUE,
        .wideLines = VK_TRUE,
        .largePoints = VK_TRUE,
    };

    deviceFeatures.features = enabledFeatures; // only enable a subset of available features

    const VkDeviceCreateInfo dci = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .enabledLayerCount = 0,
        .pNext = &deviceFeatures,
        .ppEnabledLayerNames = NULL,
        .pEnabledFeatures = NULL, // not used in newer vulkan versions
        .pQueueCreateInfos = qci,
        .queueCreateInfoCount = ARRAY_SIZE(qci),
        .enabledExtensionCount = ARRAY_SIZE(extensions),
        .ppEnabledExtensionNames = extensions
    };

    r = vkCreateDevice(physicalDevice, &dci, NULL, &device);
    assert(r == VK_SUCCESS);
    V1_PRINT("Device created successfully.\n");
}

static void initQueues(void)
{
    for (int i = 0; i < G_QUEUE_COUNT; i++) 
    {
        vkGetDeviceQueue(device, graphicsQueueFamilyIndex, i, &graphicsQueues[i]);
    }
    presentQueue = graphicsQueues[0]; // use the first queue to present
}

static void initSurface(void)
{
    const VkXcbSurfaceCreateInfoKHR ci = {
        .sType = VK_STRUCTURE_TYPE_XCB_SURFACE_CREATE_INFO_KHR,
        .connection = d_XcbWindow.connection,
        .window = d_XcbWindow.window,
    };

    VkResult r = vkCreateXcbSurfaceKHR(instance, &ci, NULL, &nativeSurface);
    assert(r == VK_SUCCESS);
    V1_PRINT("Surface created successfully.\n");
}

static void initSwapchain(void)
{
    VkBool32 supported;
    vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, 0, *pSurface, &supported);

    assert(supported == VK_TRUE);

    VkSurfaceCapabilitiesKHR capabilities;
    VkResult r = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, *pSurface, &capabilities);
    assert(r == VK_SUCCESS);

    uint32_t formatsCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, *pSurface, &formatsCount, NULL);
    VkSurfaceFormatKHR surfaceFormats[formatsCount];
    vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, *pSurface, &formatsCount, surfaceFormats);

    V1_PRINT("Surface formats: \n");
    for (int i = 0; i < formatsCount; i++) {
        V1_PRINT("Format: %d   Colorspace: %d\n", surfaceFormats[i].format, surfaceFormats[i].colorSpace);
    }

    uint32_t presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, *pSurface, &presentModeCount, NULL);
    VkPresentModeKHR presentModes[presentModeCount];
    vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, *pSurface, &presentModeCount, presentModes);

    const VkPresentModeKHR presentMode = VK_PRESENT_MODE_FIFO_KHR; // i already know its supported 

    assert(capabilities.supportedUsageFlags & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT);
    V1_PRINT("Surface Capabilities: Min swapchain image count: %d\n", capabilities.minImageCount);

    const VkSwapchainCreateInfoKHR ci = {
        .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .surface = *pSurface,
        .minImageCount = 2,
        .imageFormat = swapFormat, //50
        .imageColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR,
        .imageExtent = capabilities.currentExtent,
        .imageArrayLayers = 1, // number of views in a multiview / stereo surface
        .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        .imageSharingMode = VK_SHARING_MODE_EXCLUSIVE, // queue sharing. see vkspec section 11.7. 
        .queueFamilyIndexCount = 0, // dont need with exclusive sharing
        .pQueueFamilyIndices = NULL, // ditto
        .preTransform = capabilities.currentTransform,
        .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR, //dunno. may affect blending
        .presentMode = presentMode,
        .clipped = VK_FALSE, // allows pixels convered by another window to be clipped. but will mess up saving the swap image.
        .oldSwapchain = VK_NULL_HANDLE
    };

    r = vkCreateSwapchainKHR(device, &ci, NULL, &swapchain);
    assert(VK_SUCCESS == r);

    uint32_t imageCount;
    r = vkGetSwapchainImagesKHR(device, swapchain, &imageCount, NULL);
    assert(VK_SUCCESS == r);
    assert(FRAME_COUNT == imageCount);
    r = vkGetSwapchainImagesKHR(device, swapchain, &imageCount, swapchainImages);
    assert(VK_SUCCESS == r);

    for (int i = 0; i < FRAME_COUNT; i++) 
    {
        VkSemaphoreCreateInfo semaCi = {.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
        r = vkCreateSemaphore(device, &semaCi, NULL, &imageAcquiredSemaphores[i]);
    }

    V1_PRINT("Swapchain created successfully.\n");
}

const VkInstance* v_Init(void)
{
    nativeSurface = VK_NULL_HANDLE;
    initVkInstance();
    initDebugMessenger();
    initDevice();
    v_LoadFunctions(&device);
    initQueues();
    v_InitMemory();
    return &instance;
}

void v_InitSwapchain(VkSurfaceKHR* psurface)
{
    if (psurface)
        pSurface = psurface;
    else 
    {
        d_Init();
        initSurface();
        pSurface = &nativeSurface;
    }
    initSwapchain();
}

void v_SubmitToQueue(const VkCommandBuffer* cmdBuf, const V_QueueType queueType, const uint32_t index)
{
    assert( V_QUEUE_TYPE_GRAPHICS == queueType );
    assert( G_QUEUE_COUNT > index );

    const VkSubmitInfo info = {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .commandBufferCount = 1,
        .signalSemaphoreCount = 0,
        .waitSemaphoreCount = 0,
        .pCommandBuffers = cmdBuf
    };

    VkResult r;
    r = vkQueueSubmit(graphicsQueues[index], 1, &info, VK_NULL_HANDLE);
    assert( VK_SUCCESS == r );
}

void v_SubmitToQueueWait(const VkCommandBuffer* buffer, const V_QueueType type, const uint32_t queueIndex)
{
    VkResult r;
    v_SubmitToQueue(buffer, type, queueIndex);
    r = vkQueueWaitIdle(graphicsQueues[queueIndex]);
    assert( VK_SUCCESS == r );
}

void v_CleanUp(void)
{
    PFN_vkDestroyDebugUtilsMessengerEXT vkDestroyDebugUtilsMessengerEXT = (PFN_vkDestroyDebugUtilsMessengerEXT)
        vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
        
    v_CleanUpMemory();
    for (int i = 0; i < FRAME_COUNT; i++) 
    {
        vkDestroySemaphore(device, imageAcquiredSemaphores[i], NULL);
    }
    vkDestroySwapchainKHR(device, swapchain, NULL);
    if (nativeSurface != VK_NULL_HANDLE)
        vkDestroySurfaceKHR(instance, nativeSurface, NULL);
    vkDestroyDevice(device, NULL);
    vkDestroyDebugUtilsMessengerEXT(instance, debugMessenger, NULL);
    vkDestroyInstance(instance, NULL);
    d_CleanUp();
}

VkPhysicalDeviceRayTracingPropertiesKHR v_GetPhysicalDeviceRayTracingProperties(void)
{
    return rtProperties;
}
