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

/* See Vulkan API for documentation. */
template<>
VKAPI_ATTR VkResult VKAPI_CALL layer_vkCreateRenderPass<user_tag>(VkDevice device,
                                                                  const VkRenderPassCreateInfo* pCreateInfo,
                                                                  const VkAllocationCallbacks* pAllocator,
                                                                  VkRenderPass* pRenderPass)
{
    LAYER_TRACE(__func__);

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
	
	LAYER_LOG("NORDEUS: Invoking layer_vkCreateRenderPass");

    // Retake the lock to access layer-wide global store
    /*lock.lock();
    auto& tracker = layer->getStateTracker();
    tracker.createRenderPass(*pRenderPass, *pCreateInfo);*/
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
	
	LAYER_LOG("NORDEUS: Invoking layer_vkCreateRenderPass2");

    // Retake the lock to access layer-wide global store
    /*lock.lock();
    auto& tracker = layer->getStateTracker();
    tracker.createRenderPass(*pRenderPass, *pCreateInfo);*/
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
	
	LAYER_LOG("NORDEUS: Invoking layer_vkCreateRenderPass2KHR");

    // Retake the lock to access layer-wide global store
    /*lock.lock();
    auto& tracker = layer->getStateTracker();
    tracker.createRenderPass(*pRenderPass, *pCreateInfo);*/
    return VK_SUCCESS;
}