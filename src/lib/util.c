#include "util.h"

void obdn_PrintFormatProperties(const Obdn_Instance* instance, VkFormat format)
{
    VkFormatProperties formatProps;
    vkGetPhysicalDeviceFormatProperties(obdn_GetPhysicalDevice(instance), format, &formatProps);
}
