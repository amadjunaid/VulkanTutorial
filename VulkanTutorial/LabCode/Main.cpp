#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <iostream>
#include <stdexcept>
#include <functional>
#include <cstdlib>
#include <vector>
#include <algorithm>
#include <optional>
#include <set>
#include <cstdint> //Necessary for UINT32_MAX

const int WIDTH = 800;
const int HEIGHT = 600;

const std::vector<const char*> validationLayers = {
	"VK_LAYER_KHRONOS_validation"
};

const std::vector<const char*> deviceExtensions = {
	VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

#ifdef NDEBUG
const bool enableValidationLayers = false;
#else 
const bool enableValidationLayers = true;
#endif // NDEBUG


//Wrapper Function to create DebugUtilsMessenger
VkResult CreateDebugUtilsMessengerEXT(VkInstance instance,
	const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo_,
	const VkAllocationCallbacks* pAllocator_,
	VkDebugUtilsMessengerEXT* pDebugMessenger_)
{
	auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
	if (func != nullptr) {
		return func(instance, pCreateInfo_, pAllocator_, pDebugMessenger_);
	}
	else {
		return VK_ERROR_EXTENSION_NOT_PRESENT;
	}
}

void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator) {
	auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
	if (func != nullptr) {
		func(instance, debugMessenger, pAllocator);
	}
}
class HelloTriangleApplication {
public:
	void run() {
		initWindow();
		initVulkan();
		mainLoop();
		cleanup();
	}

private:
	void initWindow() {
		//InitGLFW
		glfwInit();

		//Disable initiliazing of OpenGL context
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

		//Disable window resizing for now
		glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

		//Create the GLFW Window		
		m_window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);

	}

	void initVulkan() {
		createInstance();
		setupDebugMessenger();
		createSurface();
		pickPhysicalDevice();
		createLogicalDevice();
	}


	void createInstance() {
		//Pre check for validation layers support
		if (enableValidationLayers && !checkValidationLayerSuport()) {
			throw std::runtime_error("validation layers requested, but not available! [::createInstance]");
		}

		//1.
		//Struct for application specific info
		VkApplicationInfo appInfo = {};
		appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		appInfo.pApplicationName = "Hello Triangle";
		appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
		appInfo.pEngineName = "No Engine";
		appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
		appInfo.apiVersion = VK_API_VERSION_1_0;

		//2.
		//Struct for vulkan instance info
		VkInstanceCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		createInfo.pApplicationInfo = &appInfo;

		//Get the required extensions
		auto extensions = getRequiredExtensions();

		//pass the GLFW extension to Vulkan create info struct
		createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
		createInfo.ppEnabledExtensionNames = extensions.data();

		//Debug messenger to debug insance creation and destruction
		VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo;

		//enable global validation layers =  0 for now
		if (enableValidationLayers) {
			createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
			createInfo.ppEnabledLayerNames = validationLayers.data();

			populateDebugMessengerCreateInfo(debugCreateInfo);
			createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugCreateInfo;
		}
		else
		{
			createInfo.enabledLayerCount = 0;
		}


		//3.
		//Create Vulkan instance based on application and instance info
		if (vkCreateInstance(&createInfo, nullptr, &m_vkInstance) != VK_SUCCESS) {
			throw std::runtime_error("failed to create Vulkan Instance! [::createInstance]");
		}
	}

	void setupDebugMessenger() {
		if (!enableValidationLayers) {
			return;
		}

		VkDebugUtilsMessengerCreateInfoEXT createInfo;

		populateDebugMessengerCreateInfo(createInfo);

		if (CreateDebugUtilsMessengerEXT(m_vkInstance, &createInfo, nullptr, &m_vkDebugMessenger) != VK_SUCCESS) {
			throw std::runtime_error("failed to set up debug messenger [::setupDebugMessenger]");
		}

	}

	void createSurface() {
		if (glfwCreateWindowSurface(m_vkInstance, m_window, nullptr, &m_vkSurface) != VK_SUCCESS) {
			throw std::runtime_error("failed to create window surface! [::createSurface]");
		}
	}

	void pickPhysicalDevice() {
		uint32_t deviceCount = 0;
		//1. 
		//Enumerate all devices and select one based on properties and features
		vkEnumeratePhysicalDevices(m_vkInstance, &deviceCount, nullptr);

		if (deviceCount == 0) {
			throw std::runtime_error("failed to find GPUs with Vulkan support!");
		}

		std::vector<VkPhysicalDevice> vkPhysicalDevices(deviceCount);
		vkEnumeratePhysicalDevices(m_vkInstance, &deviceCount, vkPhysicalDevices.data());

		for (const auto& device : vkPhysicalDevices)
		{
			if (isDeviceSuitable(device)) { // Can implement device selection criteria here
				m_vkPhysicalDevice = device;
				break;
			}
		}

		if (m_vkPhysicalDevice == VK_NULL_HANDLE) {
			throw std::runtime_error("failed to find a suitable GPU! [::pickPhysicalDevice]");
		}

	}

	void createLogicalDevice() {
		//1.
		//Get the required QueueFamily index from the physical device
		QueueFamilyIndices indices = findQueueFamilies(m_vkPhysicalDevice);
		std::set<uint32_t> uniqueQueueFamilies = { indices.graphicsFamily.value(), indices.presentFamily.value() };

		//2. 
		//Create struct for Queue creation for the logical device
		std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
		float queuePriority = 1.0f;
		for (uint32_t queueFamily : uniqueQueueFamilies) {
			VkDeviceQueueCreateInfo queueCreateInfo = {};
			queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
			queueCreateInfo.queueFamilyIndex = queueFamily;
			queueCreateInfo.queueCount = 1;
			queueCreateInfo.flags = 0;

			queueCreateInfo.pQueuePriorities = &queuePriority;
			queueCreateInfos.push_back(queueCreateInfo);
		}

		//3. 
		//Create struct for logical device features
		VkPhysicalDeviceFeatures deviceFeatures = {};

		//4. 
		//Create struct for device creation info
		VkDeviceCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		createInfo.pQueueCreateInfos = queueCreateInfos.data();
		createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
		createInfo.pEnabledFeatures = &deviceFeatures;

		createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
		createInfo.ppEnabledExtensionNames = deviceExtensions.data();
		if (enableValidationLayers) {
			createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
			createInfo.ppEnabledLayerNames = validationLayers.data();
		}
		else
		{
			createInfo.enabledLayerCount = 0;
		}

		//5. 
		//Create device and get the queue required from it
		if (vkCreateDevice(m_vkPhysicalDevice, &createInfo, nullptr, &m_vkLogicalDevice) != VK_SUCCESS) {
			throw std::runtime_error("failed to create logical device [::createLogicalDevice]");
		}

		vkGetDeviceQueue(m_vkLogicalDevice, indices.graphicsFamily.value(), 0, &m_graphicsQueue);
		vkGetDeviceQueue(m_vkLogicalDevice, indices.presentFamily.value(), 0, &m_presentQueue);
	}

	void mainLoop() {
		//Rendering loop, terminates if window is closed
		while (!glfwWindowShouldClose(m_window)) {

			//Poll input event
			glfwPollEvents();
		}
	}

	void cleanup() {
		vkDestroyDevice(m_vkLogicalDevice, nullptr);
		if (enableValidationLayers) {
			DestroyDebugUtilsMessengerEXT(m_vkInstance, m_vkDebugMessenger, nullptr);
		}

		//Cleanup GLFW window and deinitialization
		vkDestroySurfaceKHR(m_vkInstance, m_vkSurface, nullptr);
		vkDestroyInstance(m_vkInstance, nullptr);
		glfwDestroyWindow(m_window);
		glfwTerminate();
	}


	//////////////////////////////////////////////////////////////////////////////////////////////////////
	//////////////////////////////  Functions to Support instance creation : createInstance()
	bool ExistAllNeededExtensions(std::vector<const char*> requiredExtensions)
	{
		//Get vulkan extensions
		uint32_t vk_extensionCount = 0;
		vkEnumerateInstanceExtensionProperties(nullptr, &vk_extensionCount, nullptr);
		std::vector<VkExtensionProperties> vk_availableExtensions(vk_extensionCount);
		vkEnumerateInstanceExtensionProperties(nullptr, &vk_extensionCount, vk_availableExtensions.data());

		for (const auto& requiredExtension : requiredExtensions)
		{
			bool exists = false;
			for (const auto& vkExtension : vk_availableExtensions)
			{
				if (strcmp(vkExtension.extensionName, requiredExtension) == 0) {
					exists = true;
					break;
				}
			}
			if (!exists) {
				std::cout << "Required extension: '" << requiredExtension << "' not available [::CheckAllNeededExtensions]" << std::endl;
				return false;
			}
		}

		return true;
	}

	std::vector<const char*> getRequiredExtensions() {
		//Get the required extension for GLFW window system
		uint32_t glfwExtensionCount = 0;
		const char** glfwExtensions;
		glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

		//Add more extensions here
		std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

		if (enableValidationLayers) {
			extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
		}

		if (!ExistAllNeededExtensions(extensions)) {
			throw std::runtime_error("Extension not found [::createInstance]");
		}
		return extensions;
	}

	bool checkValidationLayerSuport() {
		uint32_t layerCount;
		vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

		std::vector<VkLayerProperties> availableLayers(layerCount);
		vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

		for (const char* layerName : validationLayers)
		{
			bool layerFound = false;

			for (const auto& layerProperties : availableLayers) {
				if (strcmp(layerName, layerProperties.layerName) == 0) {
					layerFound = true;
					break;
				}
			}

			if (!layerFound) {
				return false;
			}
		}

		return true;
	}


	//////////////////////////////////////////////////////////////////////////////////////////////////////
	//////////////////////////////  Functions to Support Debug Messenger Setup : setupDebugMessenger()
	void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo_) {
		createInfo_.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
		createInfo_.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT
			| VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT
			| VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
		createInfo_.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT
			| VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT
			| VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;

		createInfo_.pfnUserCallback = debugCallback;
		createInfo_.pNext = nullptr;
		createInfo_.flags = 0;
	}

	static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
		VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity_,
		VkDebugUtilsMessageTypeFlagsEXT messageType_,
		const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
		void* pUserData) {

		//Print debug message which caused the callback
		std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;

		//return false to not abort the vulkan call that triggered this callback
		return VK_FALSE;
		return VK_FALSE;
	}

	//////////////////////////////////////////////////////////////////////////////////////////////////////
	//////////////////////////////  Functions to Support Pyhsical Device Selection : pickPhysicalDevice()
	bool isDeviceSuitable(VkPhysicalDevice device_) {
		QueueFamilyIndices indices = findQueueFamilies(device_);

		bool extensionSupported = checkDeviceExtensionSupport(device_);

		bool swapChainAdequate = false;
		if (extensionSupported)	{
			SwapChainSupportDetails swapChainSupport = querySwapChainSupport(device_);
			swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.formats.empty();
		}

		return indices.isComplete() && extensionSupported && swapChainAdequate;
	}

	struct QueueFamilyIndices {
		std::optional<uint32_t> graphicsFamily;
		std::optional<uint32_t> presentFamily;

		bool isComplete() {
			return graphicsFamily.has_value() && presentFamily.has_value();
		}

	};

	QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device_) {
		QueueFamilyIndices indices;

		// Logic to find graphics queue family
		uint32_t queueFamilyCount = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(device_, &queueFamilyCount, nullptr);

		std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
		vkGetPhysicalDeviceQueueFamilyProperties(device_, &queueFamilyCount, queueFamilies.data());

		int i = 0;
		for (const auto& queueFamily : queueFamilies) {
			if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
				indices.graphicsFamily = i;
			}

			VkBool32 presentSupport = false;
			vkGetPhysicalDeviceSurfaceSupportKHR(device_, i, m_vkSurface, &presentSupport);
			if (presentSupport) {
				indices.presentFamily = i;
			}

			if (indices.isComplete()) {
				break;
			}

		}

		return indices;
	}

	//////////////////////////////////////////////////////////////////////////////////////////////////////
	//////////////////////////////  Functions to Support Presentation : createSurface()
	bool checkDeviceExtensionSupport(VkPhysicalDevice device_)
	{
		uint32_t extensionCount;
		vkEnumerateDeviceExtensionProperties(device_, nullptr, &extensionCount, nullptr);

		std::vector<VkExtensionProperties> availableExtensions(extensionCount);
		vkEnumerateDeviceExtensionProperties(device_, nullptr, &extensionCount, availableExtensions.data());

		std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());

		for (const auto& extensions : availableExtensions) {
			requiredExtensions.erase(extensions.extensionName);
		}

		return requiredExtensions.empty();
	}

	struct SwapChainSupportDetails {
		VkSurfaceCapabilitiesKHR capabilities;
		std::vector<VkSurfaceFormatKHR> formats;
		std::vector<VkPresentModeKHR> presentModes;
	};

	SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device_) {
		SwapChainSupportDetails details;
		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device_, m_vkSurface, &details.capabilities);

		uint32_t formatCount;
		vkGetPhysicalDeviceSurfaceFormatsKHR(device_, m_vkSurface, &formatCount, nullptr);

		if (formatCount != 0)
		{
			details.formats.resize(formatCount);
			vkGetPhysicalDeviceSurfaceFormatsKHR(device_, m_vkSurface, &formatCount, details.formats.data());
		}

		uint32_t presentModeCount;
		vkGetPhysicalDeviceSurfacePresentModesKHR(device_, m_vkSurface, &formatCount, nullptr);

		if (formatCount != 0) {
			details.presentModes.resize(presentModeCount);
			vkGetPhysicalDeviceSurfacePresentModesKHR(device_, m_vkSurface, &formatCount, details.presentModes.data());
		}

		return details;
	}

	VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) {
		for (const auto& availableFormat : availableFormats) {
			if (availableFormat.format == VK_FORMAT_B8G8R8_UNORM
				&& availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
				return availableFormat;
			}
		}

		return availableFormats[0];
	}

	VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes) {
		for (const auto& availableMode : availablePresentModes) {
			if (availableMode == VK_PRESENT_MODE_MAILBOX_KHR) {
				return availableMode;
			}
		}

		return VK_PRESENT_MODE_FIFO_KHR;
	}

	VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) {
		if (capabilities.currentExtent.width != UINT32_MAX) {
			return capabilities.currentExtent;
		}
		else{
			VkExtent2D actualExtent = { WIDTH, HEIGHT };

			actualExtent.width = std::max(capabilities.minImageExtent.width, std::min(capabilities.maxImageExtent.width, actualExtent.width));
			actualExtent.height = std::max(capabilities.minImageExtent.height, std::min(capabilities.maxImageExtent.height, actualExtent.height));

			return actualExtent;
		}
	}

	//Member Data
	GLFWwindow* m_window;

	VkInstance m_vkInstance;
	VkDebugUtilsMessengerEXT m_vkDebugMessenger;
	VkSurfaceKHR m_vkSurface;
	VkPhysicalDevice m_vkPhysicalDevice = VK_NULL_HANDLE;
	VkDevice m_vkLogicalDevice;

	VkQueue m_graphicsQueue;
	VkQueue m_presentQueue;
};

int main() {


	try {
		HelloTriangleApplication app;
		app.run();
	}
	catch (const std::exception& e) {
		std::cerr << e.what() << std::endl;
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}