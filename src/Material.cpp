#include "Material.h"

#include <memory>
#include <vector>

Material::Material(VkPhysicalDevice physicalDevice, VkDevice device, VkCommandBuffer copyCommandBuffer, const std::string& filename)
{
    m_albedoMap = std::make_unique<Texture>(physicalDevice, device, copyCommandBuffer, std::vector{ filename });

    VkDescriptorImageInfo imageInfo{};
    imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    imageInfo.imageView = m_albedoMap->m_imageView;
    imageInfo.sampler = m_albedoMap->m_sampler;

    VkWriteDescriptorSet textureDescriptorWrite{};
    textureDescriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    textureDescriptorWrite.dstSet = m_descriptorSet;
    textureDescriptorWrite.dstBinding = 1;
    textureDescriptorWrite.dstArrayElement = 0;
    textureDescriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    textureDescriptorWrite.descriptorCount = 1;
    textureDescriptorWrite.pImageInfo = &imageInfo;

    std::array<VkWriteDescriptorSet, 1> descriptorWrites{ textureDescriptorWrite };
    vkUpdateDescriptorSets(device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
}

void Material::bind(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayuout)
{

}