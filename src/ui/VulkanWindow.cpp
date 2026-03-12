#include "VulkanWindow.h"
#include "VulkanRenderer.h"

#include <QVulkanDeviceFunctions>

namespace myvulkan 
{
//----------------------------------------------------------------------------------
QVulkanWindowRenderer* VulkanWindow::createRenderer() {
    return new VulkanRenderer(this);
}
}  // namespace myvulkan