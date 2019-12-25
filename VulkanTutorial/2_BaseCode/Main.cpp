#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <iostream>
#include <stdexcept>
#include <functional>
#include <cstdlib>
#include <vector>
#include <algorithm>
#include <optional>

const int WIDTH = 800;
const int HEIGHT = 600;

const std::vector<const char*> validationLayers = {
	"VK_LAYER_KHRONOS_validation"
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
	auto func = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
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
		//pickPhysicalDevice();
		//createLogicalDevice();
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
			createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*) &debugCreateInfo;			
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

		//2. 
		//Create struct for Queue creation for the logical device
		VkDeviceQueueCreateInfo queueCreateInfo = {};
		queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueCreateInfo.queueFamilyIndex = indices.graphicsFamily.value();
		queueCreateInfo.queueCount = 1;
		queueCreateInfo.flags = 0;
		float queuePriority = 1.0f;
		queueCreateInfo.pQueuePriorities = &queuePriority;

		//3. 
		//Create struct for logical device features
		VkPhysicalDeviceFeatures deviceFeatures = {};

		//4. 
		//Create struct for device creation info
		VkDeviceCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		createInfo.pQueueCreateInfos = &queueCreateInfo;
		createInfo.queueCreateInfoCount = 1;
		createInfo.pEnabledFeatures = &deviceFeatures;
		
		createInfo.enabledExtensionCount = 0;
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
	}

	void mainLoop() {
		//Rendering loop, terminates if window is closed
		while (!glfwWindowShouldClose(m_window)) {
			
			//Poll input event
			glfwPollEvents();
		}
	}

	void cleanup() {
		//vkDestroyDevice(m_vkLogicalDevice, nullptr);
		if (enableValidationLayers) {
			DestroyDebugUtilsMessengerEXT(m_vkInstance, m_vkDebugMessenger, nullptr);
		}

		//Cleanup GLFW window and deinitialization
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
	bool isDeviceSuitable(VkPhysicalDevice device) {
		QueueFamilyIndices indices = findQueueFamilies(device);
			
		return indices.isComplete();
	}

	struct QueueFamilyIndices {
		std::optional<uint32_t> graphicsFamily;

		bool isComplete() {
			return graphicsFamily.has_value();
		}

	};

	QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device) {		
		QueueFamilyIndices indices;
		
		// Logic to find graphics queue family
		uint32_t queueFamilyCount = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

		std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());
		
		int i = 0;
		for (const auto& queueFamily : queueFamilies) {
			if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
				indices.graphicsFamily = i;
			}

			if (indices.isComplete()) {
				break;
			}

		}

		return indices;
	}

	//Member Data
	GLFWwindow* m_window;

	VkInstance m_vkInstance;
	VkDebugUtilsMessengerEXT m_vkDebugMessenger;
	VkPhysicalDevice m_vkPhysicalDevice = VK_NULL_HANDLE;
	VkDevice m_vkLogicalDevice;
	VkQueue m_graphicsQueue;
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