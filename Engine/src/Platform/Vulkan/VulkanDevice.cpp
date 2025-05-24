#include "VulkanDevice.h"

#include <format>
#include <set>
#include <stdexcept>

#include "Engine/Core/Common.h"

void VulkanDevice::Create(VulkanInstance* instance, const VulkanSurface& surface) {
  m_InstanceHandle = instance;
  m_SurfaceHandle = surface.Get();

  m_Features.Extensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

  PickPhysicalDevice();
  CreateLogicalDevice();
}

void VulkanDevice::PickPhysicalDevice() {
  uint32_t count = 0;
  vkEnumeratePhysicalDevices( m_InstanceHandle->Get(), &count, nullptr );

  if ( count == 0 ) {
    throw std::runtime_error( "failed to find GPUs with Vulkan support!" );
  }

  std::vector<VkPhysicalDevice> devices( count );
  vkEnumeratePhysicalDevices( m_InstanceHandle->Get(), &count, devices.data() );

  for ( const auto& device : devices ) {
    if ( true ) { // IsDeviceSuitable()
      m_PhysicalDevice = device;
      break;
    }
  }

  if ( m_PhysicalDevice == VK_NULL_HANDLE ) {
    throw std::runtime_error( "failed to find a suitable GPU!" );
  }
}

void VulkanDevice::CreateLogicalDevice() {
  auto FindQueueFamilies = [](VkPhysicalDevice physical_device, VkSurfaceKHR surface) {
    uint32_t count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties( physical_device, &count, nullptr );
    std::vector<VkQueueFamilyProperties> families( count );
    vkGetPhysicalDeviceQueueFamilyProperties( physical_device, &count, families.data() );

    QueueFamilyIndices indices;

    for (uint32_t i = 0; i < count; ++i) {
      if ( families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT ) {
        indices.GraphicsFamily = i;
      }

      VkBool32 present_support = false;
      vkGetPhysicalDeviceSurfaceSupportKHR( physical_device, i, surface, &present_support );

      if ( present_support ) {
        indices.PresentFamily = i;
      }

      if ( indices.IsComplete() ) {
        break;
      }
    }

    return indices;
  };

  QueueFamilyIndices indices = FindQueueFamilies( m_PhysicalDevice, m_SurfaceHandle );

  std::vector<VkDeviceQueueCreateInfo> queue_infos;
  std::set unique_families = {
    indices.GraphicsFamily.value(),
    indices.PresentFamily.value()
  };

  float priority = 1.0f;
  for ( uint32_t family : unique_families ) {
    VkDeviceQueueCreateInfo queue_info = {};
    queue_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queue_info.queueFamilyIndex = family;
    queue_info.queueCount = 1;
    queue_info.pQueuePriorities = &priority;
    queue_infos.push_back( queue_info );
  }

  VkPhysicalDeviceFeatures physical_features = {};
  physical_features.samplerAnisotropy = true;
  physical_features.sampleRateShading = true;

  VkDeviceCreateInfo device_info = {};
  device_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
  device_info.queueCreateInfoCount = STATIC_CAST( uint32_t, queue_infos.size() );
  device_info.pQueueCreateInfos = queue_infos.data();
  device_info.pEnabledFeatures = &physical_features;
  device_info.enabledExtensionCount = STATIC_CAST( uint32_t, m_Features.Extensions.size() );
  device_info.ppEnabledExtensionNames = m_Features.Extensions.data();

  if ( VkResult res = vkCreateDevice( m_PhysicalDevice, &device_info, nullptr, &m_LogicalDevice );
    res != VK_SUCCESS ) {
    throw std::runtime_error( 
      std::format( "failed to create logic device. error code: {}", res ) 
    );
  }

  vkGetDeviceQueue( m_LogicalDevice, indices.GraphicsFamily.value(), 0, &m_GraphicsQueue );
  vkGetDeviceQueue( m_LogicalDevice, indices.PresentFamily.value(), 0, &m_PresentQueue );
}

