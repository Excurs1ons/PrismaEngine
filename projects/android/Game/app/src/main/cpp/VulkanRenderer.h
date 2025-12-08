#ifndef ANDROIDGLINVESTIGATIONS_VULKANRENDERER_H
#define ANDROIDGLINVESTIGATIONS_VULKANRENDERER_H

#include <vulkan/vulkan.h>
#include <vulkan/vulkan_android.h>
#include <android/native_window.h>
#include <memory>
#include <vector>
#include <optional>

struct android_app;

class VulkanRenderer {
public:
    explicit VulkanRenderer(android_app* app);
    ~VulkanRenderer();

    // called from main loop
    void handleInput();
    void render();

private:
    // init / cleanup
    void initRenderer();
    void cleanupRenderer();

    // vk setup
    void createInstance();
    void createSurface();
    void pickPhysicalDevice();
    void createLogicalDevice();
    void createSwapchain();
    void createImageViews();
    void createRenderPass();
    void createFramebuffers();
    void createCommandPool();
    void createCommandBuffers();
    void createSyncObjects();

    // swapchain management
    void cleanupSwapchain();
    void recreateSwapchain();
    void recordCommandBuffer(VkCommandBuffer cmdBuf, uint32_t imageIndex);

    // utility
    bool isDeviceSuitable(VkPhysicalDevice device);
    uint32_t findGraphicsQueueFamily(VkPhysicalDevice device);
    uint32_t findPresentQueueFamily(VkPhysicalDevice device);
    VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
    VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availableModes);
    VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);

    android_app* app_{nullptr};
    ANativeWindow* window_{nullptr};

    VkInstance instance_{VK_NULL_HANDLE};
    VkSurfaceKHR surface_{VK_NULL_HANDLE};
    VkPhysicalDevice physicalDevice_{VK_NULL_HANDLE};
    VkDevice device_{VK_NULL_HANDLE};

    VkQueue graphicsQueue_{VK_NULL_HANDLE};
    VkQueue presentQueue_{VK_NULL_HANDLE};

    VkSwapchainKHR swapchain_{VK_NULL_HANDLE};
    std::vector<VkImage> swapchainImages_;
    std::vector<VkImageView> swapchainImageViews_;
    VkFormat swapchainImageFormat_{VK_FORMAT_UNDEFINED};
    VkExtent2D swapchainExtent_{};

    VkRenderPass renderPass_{VK_NULL_HANDLE};
    std::vector<VkFramebuffer> swapchainFramebuffers_;

    VkCommandPool commandPool_{VK_NULL_HANDLE};
    std::vector<VkCommandBuffer> commandBuffers_;

    VkSemaphore imageAvailableSemaphore_{VK_NULL_HANDLE};
    VkSemaphore renderFinishedSemaphore_{VK_NULL_HANDLE};
    VkFence inFlightFence_{VK_NULL_HANDLE};

    // cached sizes to detect resize
    int width_{-1};
    int height_{-1};

    // sync/flags
    bool framebufferResized_{false};
};

#endif // ANDROIDGLINVESTIGATIONS_VULKANRENDERER_H
