/*
 * Copyright (C) 2011 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <android/log.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <media/NdkImageReader.h>
#include <vulkan/vulkan.h>

#define LOGI(...) \
    __android_log_print(ANDROID_LOG_INFO, "libvulkan_test", __VA_ARGS__)
#define LOGE(...) \
    __android_log_print(ANDROID_LOG_ERROR, "libvulkan_test", __VA_ARGS__)

#define VK_CHECK(result) ASSERT_EQ(VK_SUCCESS, result)

namespace android {

class AImageReaderVulkanSwapchainTest : public ::testing::Test {
   public:
    AImageReaderVulkanSwapchainTest() {}

    AImageReader* mReader = nullptr;
    ANativeWindow* mWindow = nullptr;
    VkInstance mVkInstance = VK_NULL_HANDLE;
    VkPhysicalDevice mPhysicalDev = VK_NULL_HANDLE;
    VkDevice mDevice = VK_NULL_HANDLE;
    VkSurfaceKHR mSurface = VK_NULL_HANDLE;
    VkQueue mPresentQueue = VK_NULL_HANDLE;
    uint32_t mPresentQueueFamily = UINT32_MAX;
    VkSwapchainKHR mSwapchain = VK_NULL_HANDLE;

    void SetUp() override {}

    void TearDown() override {}

    // ------------------------------------------------------
    // Helper methods
    // ------------------------------------------------------

    void createVulkanInstance(std::vector<const char*>& layers) {
        const char* extensions[] = {
            VK_KHR_SURFACE_EXTENSION_NAME,
            VK_KHR_ANDROID_SURFACE_EXTENSION_NAME,
            VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME,
        };

        VkApplicationInfo appInfo{};
        appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        appInfo.pApplicationName = "AImageReader Vulkan Swapchain Test";
        appInfo.applicationVersion = 1;
        appInfo.pEngineName = "TestEngine";
        appInfo.engineVersion = 1;
        appInfo.apiVersion = VK_API_VERSION_1_0;

        VkInstanceCreateInfo instInfo{};
        instInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        instInfo.pApplicationInfo = &appInfo;
        instInfo.enabledExtensionCount =
            sizeof(extensions) / sizeof(extensions[0]);
        instInfo.ppEnabledExtensionNames = extensions;
        instInfo.enabledLayerCount = layers.size();
        instInfo.ppEnabledLayerNames = layers.data();
        VkResult res = vkCreateInstance(&instInfo, nullptr, &mVkInstance);
        VK_CHECK(res);
        LOGE("Vulkan instance created");
    }

    void createAImageReader(int width, int height, int format, int maxImages) {
        media_status_t status =
            AImageReader_new(width, height, format, maxImages, &mReader);
        ASSERT_EQ(AMEDIA_OK, status) << "Failed to create AImageReader";
        ASSERT_NE(nullptr, mReader) << "AImageReader is null";

        // Optionally set a listener
        AImageReader_ImageListener listener{};
        listener.context = this;
        listener.onImageAvailable =
            &AImageReaderVulkanSwapchainTest::onImageAvailable;
        AImageReader_setImageListener(mReader, &listener);

        LOGI("AImageReader created with %dx%d, format=%d", width, height,
             format);
    }

    void getANativeWindowFromReader() {
        ASSERT_NE(nullptr, mReader);

        media_status_t status = AImageReader_getWindow(mReader, &mWindow);
        ASSERT_EQ(AMEDIA_OK, status)
            << "Failed to get ANativeWindow from AImageReader";
        ASSERT_NE(nullptr, mWindow) << "ANativeWindow is null";
        LOGI("ANativeWindow obtained from AImageReader");
    }

    void createVulkanSurface() {
        ASSERT_NE((VkInstance)VK_NULL_HANDLE, mVkInstance);
        ASSERT_NE((ANativeWindow*)nullptr, mWindow);

        VkAndroidSurfaceCreateInfoKHR surfaceCreateInfo{};
        surfaceCreateInfo.sType =
            VK_STRUCTURE_TYPE_ANDROID_SURFACE_CREATE_INFO_KHR;
        surfaceCreateInfo.window = mWindow;

        VkResult res = vkCreateAndroidSurfaceKHR(
            mVkInstance, &surfaceCreateInfo, nullptr, &mSurface);
        VK_CHECK(res);
        LOGI("Vulkan surface created from ANativeWindow");
    }

    void pickPhysicalDeviceAndQueueFamily() {
        ASSERT_NE((VkInstance)VK_NULL_HANDLE, mVkInstance);

        uint32_t deviceCount = 0;
        vkEnumeratePhysicalDevices(mVkInstance, &deviceCount, nullptr);
        ASSERT_GT(deviceCount, 0U) << "No Vulkan physical devices found!";

        std::vector<VkPhysicalDevice> devices(deviceCount);
        vkEnumeratePhysicalDevices(mVkInstance, &deviceCount, devices.data());

        for (auto& dev : devices) {
            uint32_t queueFamilyCount = 0;
            vkGetPhysicalDeviceQueueFamilyProperties(dev, &queueFamilyCount,
                                                     nullptr);
            std::vector<VkQueueFamilyProperties> queueProps(queueFamilyCount);
            vkGetPhysicalDeviceQueueFamilyProperties(dev, &queueFamilyCount,
                                                     queueProps.data());

            for (uint32_t i = 0; i < queueFamilyCount; i++) {
                VkBool32 support = VK_FALSE;
                vkGetPhysicalDeviceSurfaceSupportKHR(dev, i, mSurface,
                                                     &support);
                if (support == VK_TRUE) {
                    // Found a queue family that can present
                    mPhysicalDev = dev;
                    mPresentQueueFamily = i;

                    LOGI(
                        "Physical device found with queue family %u supporting "
                        "present",
                        i);
                    return;
                }
            }
        }

        FAIL()
            << "No physical device found that supports present to the surface!";
    }

    void createDeviceAndGetQueue(std::vector<const char*>& layers) {
        ASSERT_NE((void*)VK_NULL_HANDLE, mPhysicalDev);
        ASSERT_NE(UINT32_MAX, mPresentQueueFamily);

        float queuePriority = 1.0f;
        VkDeviceQueueCreateInfo queueInfo{};
        queueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueInfo.queueFamilyIndex = mPresentQueueFamily;
        queueInfo.queueCount = 1;
        queueInfo.pQueuePriorities = &queuePriority;

        VkDeviceCreateInfo deviceInfo{};
        deviceInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        deviceInfo.queueCreateInfoCount = 1;
        deviceInfo.pQueueCreateInfos = &queueInfo;
        deviceInfo.enabledLayerCount = layers.size();
        deviceInfo.ppEnabledLayerNames = layers.data();

        const char* extensions[] = {
            VK_KHR_SWAPCHAIN_EXTENSION_NAME,
        };
        deviceInfo.enabledExtensionCount =
            sizeof(extensions) / sizeof(extensions[0]);
        deviceInfo.ppEnabledExtensionNames = extensions;

        VkResult res =
            vkCreateDevice(mPhysicalDev, &deviceInfo, nullptr, &mDevice);
        VK_CHECK(res);
        LOGI("Logical device created");

        vkGetDeviceQueue(mDevice, mPresentQueueFamily, 0, &mPresentQueue);
        ASSERT_NE((VkQueue)VK_NULL_HANDLE, mPresentQueue);
        LOGI("Acquired present-capable queue");
    }

    void createSwapchain() {
        ASSERT_NE((VkDevice)VK_NULL_HANDLE, mDevice);
        ASSERT_NE((VkSurfaceKHR)VK_NULL_HANDLE, mSurface);

        VkSurfaceCapabilitiesKHR surfaceCaps{};
        VK_CHECK(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(
            mPhysicalDev, mSurface, &surfaceCaps));

        uint32_t formatCount = 0;
        vkGetPhysicalDeviceSurfaceFormatsKHR(mPhysicalDev, mSurface,
                                             &formatCount, nullptr);
        ASSERT_GT(formatCount, 0U);
        std::vector<VkSurfaceFormatKHR> formats(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(mPhysicalDev, mSurface,
                                             &formatCount, formats.data());

        VkSurfaceFormatKHR chosenFormat = formats[0];
        LOGI("Chosen surface format: %d", chosenFormat.format);

        uint32_t presentModeCount = 0;
        vkGetPhysicalDeviceSurfacePresentModesKHR(mPhysicalDev, mSurface,
                                                  &presentModeCount, nullptr);
        ASSERT_GT(presentModeCount, 0U);
        std::vector<VkPresentModeKHR> presentModes(presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(
            mPhysicalDev, mSurface, &presentModeCount, presentModes.data());

        VkPresentModeKHR chosenPresentMode = VK_PRESENT_MODE_FIFO_KHR;
        for (auto mode : presentModes) {
            if (mode == VK_PRESENT_MODE_FIFO_KHR) {
                chosenPresentMode = mode;
                break;
            }
        }
        LOGI("Chosen present mode: %d", chosenPresentMode);

        VkExtent2D swapchainExtent{};
        if (surfaceCaps.currentExtent.width == 0xFFFFFFFF) {
            swapchainExtent.width = 640;   // fallback
            swapchainExtent.height = 480;  // fallback
        } else {
            swapchainExtent = surfaceCaps.currentExtent;
        }
        LOGI("Swapchain extent: %d x %d", swapchainExtent.width,
             swapchainExtent.height);

        uint32_t desiredImageCount = surfaceCaps.minImageCount + 1;
        if (surfaceCaps.maxImageCount > 0 &&
            desiredImageCount > surfaceCaps.maxImageCount) {
            desiredImageCount = surfaceCaps.maxImageCount;
        }

        VkSwapchainCreateInfoKHR swapchainInfo{};
        swapchainInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        swapchainInfo.surface = mSurface;
        swapchainInfo.minImageCount = desiredImageCount;
        swapchainInfo.imageFormat = chosenFormat.format;
        swapchainInfo.imageColorSpace = chosenFormat.colorSpace;
        swapchainInfo.imageExtent = swapchainExtent;
        swapchainInfo.imageArrayLayers = 1;
        swapchainInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
                                   VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
        swapchainInfo.preTransform = surfaceCaps.currentTransform;
        swapchainInfo.compositeAlpha = VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR;
        swapchainInfo.presentMode = chosenPresentMode;
        swapchainInfo.clipped = VK_TRUE;
        swapchainInfo.oldSwapchain = VK_NULL_HANDLE;

        uint32_t queueFamilyIndices[] = {mPresentQueueFamily};
        swapchainInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        swapchainInfo.queueFamilyIndexCount = 1;
        swapchainInfo.pQueueFamilyIndices = queueFamilyIndices;

        VkResult res =
            vkCreateSwapchainKHR(mDevice, &swapchainInfo, nullptr, &mSwapchain);
        VK_CHECK(res);
        LOGI("Swapchain created successfully");

        uint32_t swapchainImageCount = 0;
        vkGetSwapchainImagesKHR(mDevice, mSwapchain, &swapchainImageCount,
                                nullptr);
        std::vector<VkImage> swapchainImages(swapchainImageCount);
        vkGetSwapchainImagesKHR(mDevice, mSwapchain, &swapchainImageCount,
                                swapchainImages.data());

        LOGI("Swapchain has %u images", swapchainImageCount);
    }

    // Image available callback (AImageReader)
    static void onImageAvailable(void*, AImageReader* reader) {
        LOGI("onImageAvailable callback triggered");
        AImage* image = nullptr;
        media_status_t status = AImageReader_acquireLatestImage(reader, &image);
        if (status != AMEDIA_OK || !image) {
            LOGE("Failed to acquire latest image");
            return;
        }
        AImage_delete(image);
        LOGI("Released acquired image");
    }

    void cleanUpSwapchainForTest() {
        if (mSwapchain != VK_NULL_HANDLE) {
            vkDestroySwapchainKHR(mDevice, mSwapchain, nullptr);
            mSwapchain = VK_NULL_HANDLE;
        }
        if (mDevice != VK_NULL_HANDLE) {
            vkDestroyDevice(mDevice, nullptr);
            mDevice = VK_NULL_HANDLE;
        }
        if (mSurface != VK_NULL_HANDLE) {
            vkDestroySurfaceKHR(mVkInstance, mSurface, nullptr);
            mSurface = VK_NULL_HANDLE;
        }
        if (mVkInstance != VK_NULL_HANDLE) {
            vkDestroyInstance(mVkInstance, nullptr);
            mVkInstance = VK_NULL_HANDLE;
        }
        if (mReader) {
            // AImageReader_delete(mReader);
            mReader = nullptr;
        }
        // Note: The ANativeWindow from AImageReader is implicitly
        // managed by the reader, so we don't explicitly delete it.
        mWindow = nullptr;
    }

    void buildSwapchianForTest(std::vector<const char*>& instanceLayers,
                               std::vector<const char*>& deviceLayers) {
        createVulkanInstance(instanceLayers);

        // the "atest libvulkan_test" command will execute this test as a binary
        // (not apk) on the device. Consequently we can't render to the screen
        // and need to work around this by using AImageReader*
        createAImageReader(640, 480, AIMAGE_FORMAT_PRIVATE, 3);
        getANativeWindowFromReader();
        createVulkanSurface();
        pickPhysicalDeviceAndQueueFamily();

        createDeviceAndGetQueue(deviceLayers);
        createSwapchain();
    }
};

TEST_F(AImageReaderVulkanSwapchainTest, TestHelperMethods) {
    // Verify that the basic plumbing/helper functions of these tests is
    // working. This doesn't directly test any of the layer code. It only
    // verifies that we can successfully create a swapchain with an AImageReader

    std::vector<const char*> instanceLayers;
    std::vector<const char*> deviceLayers;
    buildSwapchianForTest(deviceLayers, instanceLayers);

    ASSERT_NE(mVkInstance, (VkInstance)VK_NULL_HANDLE);
    ASSERT_NE(mPhysicalDev, (VkPhysicalDevice)VK_NULL_HANDLE);
    ASSERT_NE(mDevice, (VkDevice)VK_NULL_HANDLE);
    ASSERT_NE(mSurface, (VkSurfaceKHR)VK_NULL_HANDLE);
    ASSERT_NE(mSwapchain, (VkSwapchainKHR)VK_NULL_HANDLE);
    cleanUpSwapchainForTest();
}

}  // namespace android
