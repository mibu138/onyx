#include "util.h"

void onyx_PrintFormatProperties(const Onyx_Instance* instance, VkFormat format)
{
    VkFormatProperties formatProps;
    vkGetPhysicalDeviceFormatProperties(onyx_GetPhysicalDevice(instance), format, &formatProps);
    hell_Print("VkFormat %d features:\n", format);
    hell_Print("linearTiling features: ");
    hell_BitPrint(&formatProps.linearTilingFeatures, 32);
    hell_Print("OptimalTiling features: ");
    hell_BitPrint(&formatProps.optimalTilingFeatures, 32);
    hell_Print("Buffer features: ");
    hell_BitPrint(&formatProps.bufferFeatures, 32);
}
