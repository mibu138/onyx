#include "t_viewer.h"

int main(int argc, char *argv[])
{
    t_InitVulkan();
    t_InitVulkanSwapchain(NULL);
    t_StartViewer();
    return 0;
}
