#include "VulkanDevice.h"

#include <format>
#include <mutex>
#include <set>
#include <stdexcept>

#include "Engine/Core/Common.h"
#include "VulkanSwapchain.h"

void VulkanDevice::Create( VulkanInstance* instance, VulkanSurface* surface ) {
  InstanceHandle = instance;
  SurfaceHandle = surface;

  Features.Extensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

  PickPhysical();
  CreateLogical();
}

QueueFamilyIndices VulkanDevice::FindQueueFamilies( VkPhysicalDevice physical_device,
    VkSurfaceKHR surface ) {
  uint32_t count = 0;
  vkGetPhysicalDeviceQueueFamilyProperties( physical_device, &count, nullptr );
  std::vector<VkQueueFamilyProperties> families( count );
  vkGetPhysicalDeviceQueueFamilyProperties( physical_device, &count, families.data() );

  QueueFamilyIndices indices;

  for ( uint32_t i = 0; i < count; ++i ) {
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
}


bool VulkanDevice::IsDeviceSuitable( VulkanDevice* device, VulkanSurface* surface ) {
  QueueFamilyIndices indices = FindQueueFamilies( device->Physical(), surface->Get() );
  bool extensions_supported = CheckDeviceExtensionSupport( device );

  bool swapchain_adequate = false;
  if ( extensions_supported ) {
    VulkanSwapchain::Features features  = VulkanSwapchain::QuerySwapChainSupport( device, surface );
    swapchain_adequate = !features.Formats.empty() && !features.PresentModes.empty();
  }

  VkPhysicalDeviceFeatures features;
  vkGetPhysicalDeviceFeatures( device->Physical(), &features );

  return indices.IsComplete() && extensions_supported && swapchain_adequate && 
    features.samplerAnisotropy;
}

bool VulkanDevice::CheckDeviceExtensionSupport(VulkanDevice* device) {
  uint32_t count = 0;
  vkEnumerateDeviceExtensionProperties( device->Physical(), nullptr, &count, nullptr );
  std::vector<VkExtensionProperties> available( count );
  vkEnumerateDeviceExtensionProperties( device->Physical(), nullptr, &count, available.data() );

  const auto& features = device->GetFeatures();
  std::set required( features.Extensions.begin(), features.Extensions.end() );
  for ( const auto& extension : available ) {
    required.erase( extension.extensionName );
  }

  return required.empty();
}

void VulkanDevice::PickPhysical() {
  uint32_t count = 0;
  vkEnumeratePhysicalDevices( InstanceHandle->Get(), &count, nullptr );

  if ( count == 0 ) {
    throw std::runtime_error( "failed to find GPUs with Vulkan support!" );
  }

  std::vector<VkPhysicalDevice> devices( count );
  vkEnumeratePhysicalDevices( InstanceHandle->Get(), &count, devices.data() );

  for ( const auto& device : devices ) {
    if ( IsDeviceSuitable( this, SurfaceHandle ) ) { // ???
      PhysicalDevice = device;
      break;
    }
  }

  if ( PhysicalDevice == VK_NULL_HANDLE ) {
    throw std::runtime_error( "failed to find a suitable GPU!" );
  }
}

void VulkanDevice::CreateLogical() {
  QueueFamilyIndices indices = FindQueueFamilies( PhysicalDevice, SurfaceHandle->Get() );

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
  device_info.enabledExtensionCount = STATIC_CAST( uint32_t, Features.Extensions.size() );
  device_info.ppEnabledExtensionNames = Features.Extensions.data();

  if ( VkResult res = vkCreateDevice( PhysicalDevice, &device_info, nullptr, &LogicalDevice );
    res != VK_SUCCESS ) {
      throw std::runtime_error( "failed to create logic device. error code" );
  }

  vkGetDeviceQueue( LogicalDevice, indices.GraphicsFamily.value(), 0, &GraphicsQueue );
  vkGetDeviceQueue( LogicalDevice, indices.PresentFamily.value(), 0, &PresentQueue );
}

