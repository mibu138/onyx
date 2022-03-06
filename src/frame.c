#include "frame.h"

void onyx_CreateFrames(Onyx_Memory* memory, uint8_t count, uint8_t aovCount, const Onyx_AovInfo* aov_infos, 
        uint32_t width, uint32_t height, Onyx_Frame* frames)
{
    for (uint32_t i = 0; i < count; i++) 
    {
        Onyx_Frame frame = {
            .aovCount = aovCount,
            .dirty = true,
            .width = width,
            .height = height,
            .index = i};
        for (uint8_t aov = 0; aov < aovCount; aov++) 
        {
            Onyx_Image image = onyx_CreateImage(memory, width, height,
                    aov_infos[aov].format, aov_infos[aov].usageFlags, aov_infos[aov].aspectFlags,
                    VK_SAMPLE_COUNT_1_BIT, 1, ONYX_MEMORY_DEVICE_TYPE);
            frame.aovs[aov] = image;
        }
        frames[i] = frame;
    }
}
