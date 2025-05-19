#include "VulkanContext.h"

#include <vector>
#include <ranges>

#include <vulkan/vulkan.hpp>

VulkanContext::VulkanContext() {
  CheckRequestedValidationLayers({});
}

bool VulkanContext::CheckRequestedValidationLayers(const VulkanContextCreateInfo& options) const {
  std::vector<vk::LayerProperties> available_layers = vk::enumerateInstanceLayerProperties();

  for ( const char* layer_name : options.Layers ) {
    auto layer_it = std::ranges::find_if( 
      available_layers, 
      [layer_name] ( const VkLayerProperties& layer ) {
        return std::strcmp( layer.layerName, layer_name ) == 0;
      }
    );

    if ( layer_it == available_layers.end() ) {
      return false;
    }
  }

  return true;
}
