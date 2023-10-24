#include "Mesh.h"

Material::Material()
{
    VkDescriptorImageInfo imageInfo{};
    imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    imageInfo.imageView = m_albedoMap->m_imageView;
    imageInfo.sampler = m_albedoMap->m_sampler;

    VkWriteDescriptorSet textureDescriptorWrite{};
    textureDescriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    textureDescriptorWrite.dstSet = m_descriptorSets[i];
    textureDescriptorWrite.dstBinding = 1;
    textureDescriptorWrite.dstArrayElement = 0;
    textureDescriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    textureDescriptorWrite.descriptorCount = 1;
    textureDescriptorWrite.pImageInfo = &imageInfo;

    std::array<VkWriteDescriptorSet, 1> descriptorWrites{ textureDescriptorWrite };
    vkUpdateDescriptorSets(m_device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
}