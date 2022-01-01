#include "frame.h"

void obdn_CreateFrames(Obdn_Memory* memory, uint8_t count, uint8_t aovCount, const Obdn_AovInfo* aov_infos, 
        uint32_t width, uint32_t height, Obdn_Frame* frames)
{
    for (uint32_t i = 0; i < count; i++) 
    {
        Obdn_Frame frame = {
            .aovCount = aovCount,
            .dirty = true,
            .width = width,
            .height = height,
            .index = i};
        for (uint8_t aov = 0; aov < aovCount; aov++) 
        {
            Obdn_Image image = obdn_CreateImage(memory, width, height,
                    aov_infos[aov].format, aov_infos[aov].usageFlags, aov_infos[aov].aspectFlags,
                    VK_SAMPLE_COUNT_1_BIT, 1, OBDN_MEMORY_DEVICE_TYPE);
            frame.aovs[aov] = image;
        }
        frames[i] = frame;
    }
}
