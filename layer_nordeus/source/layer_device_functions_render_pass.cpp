/*
 * SPDX-License-Identifier: MIT
 * ----------------------------------------------------------------------------
 * Copyright (c) 2024-2025 Arm Limited
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 * ----------------------------------------------------------------------------
 */

#include "device.hpp"
#include "framework/device_dispatch_table.hpp"
#include "framework/utils.hpp"
#include "trackers/render_pass.hpp"

#include <mutex>

extern std::mutex g_vulkanLock;

void CheckRenderPassForMultisample(const VkAttachmentDescription* attachments, uint32_t count)
{
	LAYER_LOG("IGNACIO: Invoking CheckRenderPassForMultisample. Count: %d.", count);
	
    for (uint32_t i = 0; i < count; ++i)
	{
        if (attachments[i].samples != VK_SAMPLE_COUNT_1_BIT)
		{
            LAYER_LOG("IGNACIO: Render pass has multisample attachment [%d]: sample count = %d\n", i, attachments[i].samples);
        }
    }
}

void CheckRenderPass2ForMultisample(const VkAttachmentDescription2* attachments, uint32_t count)
{
	LAYER_LOG("IGNACIO: Invoking CheckRenderPass2ForMultisample. Count: %d.", count);
	
    for (uint32_t i = 0; i < count; ++i)
	{
        if (attachments[i].samples != VK_SAMPLE_COUNT_1_BIT)
		{
            LAYER_LOG("IGNACIO: Render pass2 has multisample attachment [%d]: sample count = %d\n", i, attachments[i].samples);
        }
    }
}

/* See Vulkan API for documentation. */
template<>
VKAPI_ATTR VkResult VKAPI_CALL layer_vkCreateRenderPass<user_tag>(VkDevice device,
                                                                  const VkRenderPassCreateInfo* pCreateInfo,
                                                                  const VkAllocationCallbacks* pAllocator,
                                                                  VkRenderPass* pRenderPass)
{
    LAYER_TRACE(__func__);
	
	LAYER_LOG("IGNACIO: Invoking layer_vkCreateRenderPass");
	
	// Count how many new resolve attachments we need
    uint32_t extraAttachmentCount = 0;
    for (uint32_t i = 0; i < pCreateInfo->attachmentCount; ++i) {
        if (pCreateInfo->pAttachments[i].samples != VK_SAMPLE_COUNT_1_BIT) {
            extraAttachmentCount++;
        }
    }

    if (extraAttachmentCount > 0)
	{
		// --- Allocate new attachment list ---
		const uint32_t newAttachmentCount = pCreateInfo->attachmentCount + extraAttachmentCount;
		std::vector<VkAttachmentDescription> newAttachments(newAttachmentCount);
		memcpy(newAttachments.data(), pCreateInfo->pAttachments, sizeof(VkAttachmentDescription) * pCreateInfo->attachmentCount);

		uint32_t resolveIndex = pCreateInfo->attachmentCount;

		// --- Create new resolve attachments ---
		std::unordered_map<uint32_t, uint32_t> resolveMap; // maps src attachment index → resolve index

		for (uint32_t i = 0; i < pCreateInfo->attachmentCount; ++i) {
			const auto& src = pCreateInfo->pAttachments[i];
			if (src.samples == VK_SAMPLE_COUNT_1_BIT)
				continue;

			VkAttachmentDescription resolveDesc = {};
			resolveDesc.format = src.format;
			resolveDesc.samples = VK_SAMPLE_COUNT_1_BIT;
			resolveDesc.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			resolveDesc.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
			resolveDesc.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			resolveDesc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			resolveDesc.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			resolveDesc.finalLayout = src.finalLayout;

			newAttachments[resolveIndex] = resolveDesc;
			resolveMap[i] = resolveIndex++;
		}

		// --- Patch subpasses ---
		std::vector<VkSubpassDescription> newSubpasses(pCreateInfo->subpassCount);
		std::vector<std::vector<VkAttachmentReference>> resolveRefs(pCreateInfo->subpassCount);

		for (uint32_t i = 0; i < pCreateInfo->subpassCount; ++i) {
			const auto& oldSubpass = pCreateInfo->pSubpasses[i];
			auto& newSubpass = newSubpasses[i] = oldSubpass;

			resolveRefs[i].resize(oldSubpass.colorAttachmentCount);

			newSubpass.pResolveAttachments = resolveRefs[i].data();

			for (uint32_t j = 0; j < oldSubpass.colorAttachmentCount; ++j) {
				const auto& colorRef = oldSubpass.pColorAttachments[j];

				if (resolveMap.count(colorRef.attachment)) {
					resolveRefs[i][j].attachment = resolveMap[colorRef.attachment];
					resolveRefs[i][j].layout = colorRef.layout;
				} else {
					resolveRefs[i][j].attachment = VK_ATTACHMENT_UNUSED;
				}
			}
		}

		// --- Final render pass create info ---
		VkRenderPassCreateInfo modifiedCreateInfo = *pCreateInfo;
		modifiedCreateInfo.attachmentCount = newAttachmentCount;
		modifiedCreateInfo.pAttachments = newAttachments.data();
		modifiedCreateInfo.pSubpasses = newSubpasses.data();
	}
    
    // Hold the lock to access layer-wide global store
    std::unique_lock<std::mutex> lock {g_vulkanLock};
    auto* layer = Device::retrieve(device);

    // Release the lock to call into the driver
    lock.unlock();
	VkResult ret = layer->driver.vkCreateRenderPass(device, pCreateInfo, pAllocator, pRenderPass);
    if (ret != VK_SUCCESS)
    {
        return ret;
    }

    return VK_SUCCESS;
}

/* See Vulkan API for documentation. */
template<>
VKAPI_ATTR VkResult VKAPI_CALL layer_vkCreateRenderPass2<user_tag>(VkDevice device,
                                                                   const VkRenderPassCreateInfo2* pCreateInfo,
                                                                   const VkAllocationCallbacks* pAllocator,
                                                                   VkRenderPass* pRenderPass)
{
    LAYER_TRACE(__func__);
	
	LAYER_LOG("IGNACIO: Invoking layer_vkCreateRenderPass2");
	
	CheckRenderPass2ForMultisample(pCreateInfo->pAttachments, pCreateInfo->attachmentCount);

    // Hold the lock to access layer-wide global store
    std::unique_lock<std::mutex> lock {g_vulkanLock};
    auto* layer = Device::retrieve(device);

    // Release the lock to call into the driver
    lock.unlock();
    VkResult ret = layer->driver.vkCreateRenderPass2(device, pCreateInfo, pAllocator, pRenderPass);
    if (ret != VK_SUCCESS)
    {
        return ret;
    }
	
    return VK_SUCCESS;
}

/* See Vulkan API for documentation. */
template<>
VKAPI_ATTR VkResult VKAPI_CALL layer_vkCreateRenderPass2KHR<user_tag>(VkDevice device,
                                                                      const VkRenderPassCreateInfo2* pCreateInfo,
                                                                      const VkAllocationCallbacks* pAllocator,
                                                                      VkRenderPass* pRenderPass)
{
    LAYER_TRACE(__func__);
	
	LAYER_LOG("IGNACIO: Invoking layer_vkCreateRenderPass2KHR");
	
	uint32_t extraAttachmentCount = 0;
    for (uint32_t i = 0; i < pCreateInfo->attachmentCount; ++i)
	{
        if (pCreateInfo->pAttachments[i].samples != VK_SAMPLE_COUNT_1_BIT)
		{
            extraAttachmentCount++;
        }
    }

    if (extraAttachmentCount > 0)
	{
		LAYER_LOG("IGNACIO: layer_vkCreateRenderPass2KHR. Extra attachments count: %d.", extraAttachmentCount);
		
		const uint32_t newAttachmentCount = pCreateInfo->attachmentCount + extraAttachmentCount;
		std::vector<VkAttachmentDescription2> newAttachments(newAttachmentCount);
		memcpy(newAttachments.data(), pCreateInfo->pAttachments, sizeof(VkAttachmentDescription2) * pCreateInfo->attachmentCount);

		uint32_t resolveIndex = pCreateInfo->attachmentCount;
		std::unordered_map<uint32_t, uint32_t> resolveMap;

		for (uint32_t i = 0; i < pCreateInfo->attachmentCount; ++i) {
			const auto& src = pCreateInfo->pAttachments[i];
			if (src.samples == VK_SAMPLE_COUNT_1_BIT)
				continue;
			
			LAYER_LOG("IGNACIO: In layer_vkCreateRenderPass2KHR. Creating new attachment description with resolve index %d.", resolveIndex);

			VkAttachmentDescription2 resolveDesc = {};
			resolveDesc.sType = VK_STRUCTURE_TYPE_ATTACHMENT_DESCRIPTION_2;
			resolveDesc.format = src.format;
			resolveDesc.samples = VK_SAMPLE_COUNT_1_BIT;
			resolveDesc.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			resolveDesc.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
			resolveDesc.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			resolveDesc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			resolveDesc.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			resolveDesc.finalLayout = src.finalLayout;

			newAttachments[resolveIndex] = resolveDesc;
			resolveMap[i] = resolveIndex++;
		}

		// Patch subpasses
		std::vector<VkSubpassDescription2> newSubpasses(pCreateInfo->subpassCount);
		std::vector<std::vector<VkAttachmentReference2>> resolveRefs(pCreateInfo->subpassCount);

		for (uint32_t i = 0; i < pCreateInfo->subpassCount; ++i) {
			const auto& oldSubpass = pCreateInfo->pSubpasses[i];
			auto& newSubpass = newSubpasses[i] = oldSubpass;

			resolveRefs[i].resize(oldSubpass.colorAttachmentCount);

			newSubpass.pResolveAttachments = resolveRefs[i].data();

			for (uint32_t j = 0; j < oldSubpass.colorAttachmentCount; ++j) {
				const auto& colorRef = oldSubpass.pColorAttachments[j];

				if (resolveMap.count(colorRef.attachment)) {
					resolveRefs[i][j] = {
						VK_STRUCTURE_TYPE_ATTACHMENT_REFERENCE_2,       // sType
						nullptr,                                        // pNext
						resolveMap[colorRef.attachment],               // attachment
						colorRef.layout,                               // layout
						0,                                             // aspectMask
					};
				} else {
					resolveRefs[i][j] = {
						VK_STRUCTURE_TYPE_ATTACHMENT_REFERENCE_2,
						nullptr,
						VK_ATTACHMENT_UNUSED,
						VK_IMAGE_LAYOUT_UNDEFINED,
						0,
					};
				}
			}
		}

		VkRenderPassCreateInfo2 modifiedInfo = *pCreateInfo;
		modifiedInfo.attachmentCount = newAttachmentCount;
		modifiedInfo.pAttachments = newAttachments.data();
		modifiedInfo.pSubpasses = newSubpasses.data();
	}
	
    // Hold the lock to access layer-wide global store
    std::unique_lock<std::mutex> lock {g_vulkanLock};
    auto* layer = Device::retrieve(device);

    // Release the lock to call into the driver
    lock.unlock();
    VkResult ret = layer->driver.vkCreateRenderPass2KHR(device, pCreateInfo, pAllocator, pRenderPass);
    if (ret != VK_SUCCESS)
    {
        return ret;
    }
	
    return VK_SUCCESS;
}


