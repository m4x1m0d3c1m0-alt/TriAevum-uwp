#ifndef VK_USE_PLATFORM_WIN32_KHR
#define VK_USE_PLATFORM_WIN32_KHR
#endif
#include <windows.h>
#include <winrt/base.h>
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.ApplicationModel.Core.h>
#include <winrt/Windows.UI.Core.h>
#include <winrt/Windows.Storage.h>
#include <vulkan/vulkan.h>
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <vector>
namespace core=winrt::Windows::ApplicationModel::Core;
namespace ui=winrt::Windows::UI::Core;
static HANDLE logFile=INVALID_HANDLE_VALUE;
static ui::CoreWindow window{nullptr};
static bool closed=false;
static HMODULE driver=nullptr;
static VkInstance instance=VK_NULL_HANDLE;
static VkSurfaceKHR surface=VK_NULL_HANDLE;
static VkDevice device=VK_NULL_HANDLE;
static VkSwapchainKHR swapchain=VK_NULL_HANDLE;
static void Log(const char* s) {
 OutputDebugStringA(s); OutputDebugStringA("\r\n");
 if(logFile!=INVALID_HANDLE_VALUE){DWORD n=0;WriteFile(logFile,s,(DWORD)strlen(s),&n,nullptr);WriteFile(logFile,"\r\n",2,&n,nullptr);FlushFileBuffers(logFile);}
}
static void OpenLog(){
 try{auto f=winrt::Windows::Storage::ApplicationData::Current().LocalFolder();std::wstring p=f.Path().c_str();p+=L"\\TriAevumVulkanProbe.log";logFile=CreateFile2FromAppW(p.c_str(),FILE_APPEND_DATA,FILE_SHARE_READ,OPEN_ALWAYS,nullptr);}catch(...){}
 Log("=== TriAevum Dozen Vulkan UWP probe ===");
}
extern "C" __declspec(dllexport) unsigned int uwp_get_width(){return window?(unsigned int)(std::max)(1.0f,window.Bounds().Width):1280;}
extern "C" __declspec(dllexport) unsigned int uwp_get_height(){return window?(unsigned int)(std::max)(1.0f,window.Bounds().Height):720;}
static PFN_vkGetInstanceProcAddr gipa=nullptr;
#define GLOBAL(var,fn) var=reinterpret_cast<PFN_##fn>(gipa(VK_NULL_HANDLE,#fn));if(!var){Log("Missing global " #fn);return false;}
#define INST(var,fn) var=reinterpret_cast<PFN_##fn>(gipa(instance,#fn));if(!var){Log("Missing instance " #fn);return false;}
#define DEV(var,fn) var=reinterpret_cast<PFN_##fn>(gdpa(device,#fn));if(!var){Log("Missing device " #fn);return false;}
static PFN_vkCreateInstance pCreateInstance;
static PFN_vkEnumerateInstanceExtensionProperties pEnumInstExt;
static PFN_vkDestroyInstance pDestroyInstance;
static PFN_vkEnumeratePhysicalDevices pEnumPhysical;
static PFN_vkGetPhysicalDeviceProperties pGetProps;
static PFN_vkGetPhysicalDeviceFeatures2 pGetFeatures2;
static PFN_vkGetPhysicalDeviceProperties2 pGetProperties2;
static PFN_vkEnumerateDeviceExtensionProperties pEnumDevExt;
static PFN_vkGetPhysicalDeviceQueueFamilyProperties pGetQueues;
static PFN_vkCreateWin32SurfaceKHR pCreateSurface;
static PFN_vkDestroySurfaceKHR pDestroySurface;
static PFN_vkGetPhysicalDeviceSurfaceSupportKHR pSurfaceSupport;
static PFN_vkGetPhysicalDeviceSurfaceCapabilitiesKHR pSurfaceCaps;
static PFN_vkGetPhysicalDeviceSurfaceFormatsKHR pSurfaceFormats;
static PFN_vkCreateDevice pCreateDevice;
static PFN_vkGetDeviceProcAddr gdpa;
static PFN_vkCreateSwapchainKHR pCreateSwapchain;
static PFN_vkDestroySwapchainKHR pDestroySwapchain;
static PFN_vkDestroyDevice pDestroyDevice;
static PFN_vkGetDeviceQueue pGetDeviceQueue;
static PFN_vkGetSwapchainImagesKHR pGetSwapchainImages;
static PFN_vkAcquireNextImageKHR pAcquireNextImage;
static PFN_vkQueuePresentKHR pQueuePresent;
static PFN_vkQueueSubmit pQueueSubmit;
static PFN_vkQueueWaitIdle pQueueWaitIdle;
static PFN_vkCreateCommandPool pCreateCommandPool;
static PFN_vkDestroyCommandPool pDestroyCommandPool;
static PFN_vkAllocateCommandBuffers pAllocateCommandBuffers;
static PFN_vkBeginCommandBuffer pBeginCommandBuffer;
static PFN_vkEndCommandBuffer pEndCommandBuffer;
static PFN_vkCmdPipelineBarrier pCmdPipelineBarrier;
static PFN_vkCmdClearColorImage pCmdClearColorImage;
static PFN_vkCreateSemaphore pCreateSemaphore;
static PFN_vkDestroySemaphore pDestroySemaphore;
static void Result(const char* label,VkResult r){char b[160];sprintf_s(b,"%s => VkResult %d",label,(int)r);Log(b);}
static bool Probe(){
 Log("Loading packaged vulkan_dzn.dll");
 driver=LoadPackagedLibrary(L"vulkan_dzn.dll",0);
 if(!driver){char b[128];sprintf_s(b,"LoadPackagedLibrary failed: %lu",GetLastError());Log(b);return false;}
 gipa=(PFN_vkGetInstanceProcAddr)GetProcAddress(driver,"vk_icdGetInstanceProcAddr");
 if(!gipa){Log("vk_icdGetInstanceProcAddr export missing");return false;}
 GLOBAL(pCreateInstance,vkCreateInstance);
 GLOBAL(pEnumInstExt,vkEnumerateInstanceExtensionProperties);
 uint32_t n=0;VkResult r=pEnumInstExt(nullptr,&n,nullptr);Result("Enumerate instance extensions",r);if(r!=VK_SUCCESS)return false;
 std::vector<VkExtensionProperties> ie(n);r=pEnumInstExt(nullptr,&n,ie.data());if(r!=VK_SUCCESS)return false;
 bool surfaceExt=false,win32Ext=false;for(auto&e:ie){Log(e.extensionName);surfaceExt|=!strcmp(e.extensionName,VK_KHR_SURFACE_EXTENSION_NAME);win32Ext|=!strcmp(e.extensionName,VK_KHR_WIN32_SURFACE_EXTENSION_NAME);}
 if(!surfaceExt||!win32Ext){Log("Missing VK_KHR_surface or VK_KHR_win32_surface");return false;}
 const char* enabled[]={VK_KHR_SURFACE_EXTENSION_NAME,VK_KHR_WIN32_SURFACE_EXTENSION_NAME};
 VkApplicationInfo ai{VK_STRUCTURE_TYPE_APPLICATION_INFO};ai.pApplicationName="TriAevum Vulkan Probe";ai.apiVersion=VK_API_VERSION_1_2;
 VkInstanceCreateInfo ci{VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};ci.pApplicationInfo=&ai;ci.enabledExtensionCount=2;ci.ppEnabledExtensionNames=enabled;
 r=pCreateInstance(&ci,nullptr,&instance);Result("vkCreateInstance",r);if(r!=VK_SUCCESS)return false;
 INST(pDestroyInstance,vkDestroyInstance);INST(pEnumPhysical,vkEnumeratePhysicalDevices);INST(pGetProps,vkGetPhysicalDeviceProperties);
 INST(pGetFeatures2,vkGetPhysicalDeviceFeatures2);INST(pGetProperties2,vkGetPhysicalDeviceProperties2);
 INST(pEnumDevExt,vkEnumerateDeviceExtensionProperties);INST(pGetQueues,vkGetPhysicalDeviceQueueFamilyProperties);
 INST(pCreateSurface,vkCreateWin32SurfaceKHR);INST(pDestroySurface,vkDestroySurfaceKHR);
 INST(pSurfaceSupport,vkGetPhysicalDeviceSurfaceSupportKHR);INST(pSurfaceCaps,vkGetPhysicalDeviceSurfaceCapabilitiesKHR);
 INST(pSurfaceFormats,vkGetPhysicalDeviceSurfaceFormatsKHR);INST(pCreateDevice,vkCreateDevice);INST(gdpa,vkGetDeviceProcAddr);
 uint32_t pc=0;r=pEnumPhysical(instance,&pc,nullptr);Result("Enumerate physical devices",r);if(r!=VK_SUCCESS||!pc)return false;
 std::vector<VkPhysicalDevice> phys(pc);r=pEnumPhysical(instance,&pc,phys.data());if(r!=VK_SUCCESS)return false;
 VkPhysicalDevice gpu=phys[0];VkPhysicalDeviceProperties props{};pGetProps(gpu,&props);Log(props.deviceName);
 char inf[256];sprintf_s(inf,"Vulkan %u.%u.%u, vendor %04x device %04x, driverVersion %u",VK_VERSION_MAJOR(props.apiVersion),VK_VERSION_MINOR(props.apiVersion),VK_VERSION_PATCH(props.apiVersion),props.vendorID,props.deviceID,props.driverVersion);Log(inf);
 // Capture the actual feature/limit profile of the Xbox adapter. This is the
 // compatibility baseline for TriAevum; do not infer support from API version.
 VkPhysicalDeviceVulkan12Features f12{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES};
 VkPhysicalDeviceVulkan11Features f11{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES};
 VkPhysicalDeviceSynchronization2FeaturesKHR sync2{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SYNCHRONIZATION_2_FEATURES_KHR};
 VkPhysicalDeviceDynamicRenderingFeaturesKHR dynamicRendering{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES_KHR};
 f11.pNext=&f12;f12.pNext=&sync2;sync2.pNext=&dynamicRendering;
 VkPhysicalDeviceFeatures2 f2{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2};f2.pNext=&f11;
 pGetFeatures2(gpu,&f2);const auto& cf=f2.features;
 #define FEATURE(name) do{sprintf_s(inf,"FEATURE " #name "=%u",(unsigned)cf.name);Log(inf);}while(0)
 FEATURE(robustBufferAccess);FEATURE(fullDrawIndexUint32);FEATURE(imageCubeArray);FEATURE(independentBlend);FEATURE(geometryShader);FEATURE(tessellationShader);FEATURE(sampleRateShading);FEATURE(dualSrcBlend);FEATURE(logicOp);FEATURE(multiDrawIndirect);FEATURE(drawIndirectFirstInstance);FEATURE(depthClamp);FEATURE(depthBiasClamp);FEATURE(fillModeNonSolid);FEATURE(depthBounds);FEATURE(wideLines);FEATURE(largePoints);FEATURE(alphaToOne);FEATURE(multiViewport);FEATURE(samplerAnisotropy);FEATURE(textureCompressionBC);FEATURE(occlusionQueryPrecise);FEATURE(pipelineStatisticsQuery);FEATURE(vertexPipelineStoresAndAtomics);FEATURE(fragmentStoresAndAtomics);FEATURE(shaderTessellationAndGeometryPointSize);FEATURE(shaderImageGatherExtended);FEATURE(shaderStorageImageExtendedFormats);FEATURE(shaderStorageImageReadWithoutFormat);FEATURE(shaderStorageImageWriteWithoutFormat);FEATURE(shaderUniformBufferArrayDynamicIndexing);FEATURE(shaderSampledImageArrayDynamicIndexing);FEATURE(shaderStorageBufferArrayDynamicIndexing);FEATURE(shaderStorageImageArrayDynamicIndexing);FEATURE(shaderClipDistance);FEATURE(shaderCullDistance);FEATURE(shaderFloat64);FEATURE(shaderInt64);FEATURE(shaderInt16);FEATURE(shaderResourceResidency);FEATURE(shaderResourceMinLod);FEATURE(sparseBinding);FEATURE(sparseResidencyBuffer);FEATURE(sparseResidencyImage2D);
 #undef FEATURE
 #define FEATURE12(name) do{sprintf_s(inf,"FEATURE12 " #name "=%u",(unsigned)f12.name);Log(inf);}while(0)
 FEATURE12(samplerMirrorClampToEdge);FEATURE12(drawIndirectCount);FEATURE12(storageBuffer8BitAccess);FEATURE12(uniformAndStorageBuffer8BitAccess);FEATURE12(shaderBufferInt64Atomics);FEATURE12(shaderSharedInt64Atomics);FEATURE12(shaderFloat16);FEATURE12(shaderInt8);FEATURE12(descriptorIndexing);FEATURE12(descriptorBindingUniformBufferUpdateAfterBind);FEATURE12(descriptorBindingSampledImageUpdateAfterBind);FEATURE12(descriptorBindingStorageImageUpdateAfterBind);FEATURE12(descriptorBindingStorageBufferUpdateAfterBind);FEATURE12(descriptorBindingUniformTexelBufferUpdateAfterBind);FEATURE12(descriptorBindingStorageTexelBufferUpdateAfterBind);FEATURE12(descriptorBindingUpdateUnusedWhilePending);FEATURE12(shaderInputAttachmentArrayDynamicIndexing);FEATURE12(shaderUniformTexelBufferArrayDynamicIndexing);FEATURE12(shaderStorageTexelBufferArrayDynamicIndexing);FEATURE12(shaderUniformBufferArrayNonUniformIndexing);FEATURE12(shaderSampledImageArrayNonUniformIndexing);FEATURE12(shaderStorageBufferArrayNonUniformIndexing);FEATURE12(shaderStorageImageArrayNonUniformIndexing);FEATURE12(shaderInputAttachmentArrayNonUniformIndexing);FEATURE12(shaderUniformTexelBufferArrayNonUniformIndexing);FEATURE12(shaderStorageTexelBufferArrayNonUniformIndexing);FEATURE12(descriptorBindingPartiallyBound);FEATURE12(descriptorBindingVariableDescriptorCount);FEATURE12(runtimeDescriptorArray);FEATURE12(samplerFilterMinmax);FEATURE12(scalarBlockLayout);FEATURE12(imagelessFramebuffer);FEATURE12(uniformBufferStandardLayout);FEATURE12(shaderSubgroupExtendedTypes);FEATURE12(separateDepthStencilLayouts);FEATURE12(hostQueryReset);FEATURE12(timelineSemaphore);FEATURE12(bufferDeviceAddress);FEATURE12(bufferDeviceAddressCaptureReplay);FEATURE12(bufferDeviceAddressMultiDevice);FEATURE12(vulkanMemoryModel);FEATURE12(vulkanMemoryModelDeviceScope);FEATURE12(vulkanMemoryModelAvailabilityVisibilityChains);FEATURE12(shaderOutputViewportIndex);FEATURE12(shaderOutputLayer);FEATURE12(subgroupBroadcastDynamicId);
 #undef FEATURE12
 sprintf_s(inf,"FEATURE_EXT synchronization2=%u dynamicRendering=%u",(unsigned)sync2.synchronization2,(unsigned)dynamicRendering.dynamicRendering);Log(inf);
 sprintf_s(inf,"LIMITS pushConstants=%u maxStorageBufferRange=%u maxUniformBufferRange=%u maxBoundDescriptorSets=%u maxPerStageSampledImages=%u maxPerStageStorageBuffers=%u maxPerStageDescriptorSamplers=%u maxColorAttachments=%u maxImageDimension2D=%u maxFramebufferWidth=%u maxFramebufferHeight=%u",props.limits.maxPushConstantsSize,props.limits.maxStorageBufferRange,props.limits.maxUniformBufferRange,props.limits.maxBoundDescriptorSets,props.limits.maxPerStageDescriptorSampledImages,props.limits.maxPerStageDescriptorStorageBuffers,props.limits.maxPerStageDescriptorSamplers,props.limits.maxColorAttachments,props.limits.maxImageDimension2D,props.limits.maxFramebufferWidth,props.limits.maxFramebufferHeight);Log(inf);
 sprintf_s(inf,"ALIGNMENTS minUniformBufferOffsetAlignment=%llu minStorageBufferOffsetAlignment=%llu minTexelBufferOffsetAlignment=%llu minMemoryMapAlignment=%llu",(unsigned long long)props.limits.minUniformBufferOffsetAlignment,(unsigned long long)props.limits.minStorageBufferOffsetAlignment,(unsigned long long)props.limits.minTexelBufferOffsetAlignment,(unsigned long long)props.limits.minMemoryMapAlignment);Log(inf);
 VkWin32SurfaceCreateInfoKHR sci{VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR};sci.hinstance=GetModuleHandleW(nullptr);
 // The Mesa UWP patch interprets hwnd as the CoreWindow IUnknown.
 sci.hwnd=reinterpret_cast<HWND>(winrt::get_unknown(window));
 r=pCreateSurface(instance,&sci,nullptr,&surface);Result("vkCreateWin32SurfaceKHR(CoreWindow)",r);if(r!=VK_SUCCESS)return false;
 uint32_t qn=0;pGetQueues(gpu,&qn,nullptr);std::vector<VkQueueFamilyProperties> q(qn);pGetQueues(gpu,&qn,q.data());
 uint32_t family=UINT32_MAX;for(uint32_t i=0;i<qn;i++){VkBool32 ok=VK_FALSE;r=pSurfaceSupport(gpu,i,surface,&ok);if(r==VK_SUCCESS&&ok&&(q[i].queueFlags&VK_QUEUE_GRAPHICS_BIT)){family=i;break;}}
 if(family==UINT32_MAX){Log("No graphics queue supports CoreWindow");return false;}Log("CoreWindow surface supported by graphics queue");
 uint32_t en=0;r=pEnumDevExt(gpu,nullptr,&en,nullptr);if(r!=VK_SUCCESS)return false;std::vector<VkExtensionProperties> de(en);r=pEnumDevExt(gpu,nullptr,&en,de.data());if(r!=VK_SUCCESS)return false;
 bool swapExt=false;for(auto&e:de){Log(e.extensionName);swapExt|=!strcmp(e.extensionName,VK_KHR_SWAPCHAIN_EXTENSION_NAME);}if(!swapExt){Log("VK_KHR_swapchain missing");return false;}
 float priority=1.f;VkDeviceQueueCreateInfo qi{VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO};qi.queueFamilyIndex=family;qi.queueCount=1;qi.pQueuePriorities=&priority;
 const char* dex[]={VK_KHR_SWAPCHAIN_EXTENSION_NAME};VkDeviceCreateInfo di{VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};di.queueCreateInfoCount=1;di.pQueueCreateInfos=&qi;di.enabledExtensionCount=1;di.ppEnabledExtensionNames=dex;
 r=pCreateDevice(gpu,&di,nullptr,&device);Result("vkCreateDevice",r);if(r!=VK_SUCCESS)return false;
 DEV(pCreateSwapchain,vkCreateSwapchainKHR);DEV(pDestroySwapchain,vkDestroySwapchainKHR);DEV(pDestroyDevice,vkDestroyDevice);
 DEV(pGetDeviceQueue,vkGetDeviceQueue);DEV(pGetSwapchainImages,vkGetSwapchainImagesKHR);
 DEV(pAcquireNextImage,vkAcquireNextImageKHR);DEV(pQueuePresent,vkQueuePresentKHR);
 DEV(pQueueSubmit,vkQueueSubmit);DEV(pQueueWaitIdle,vkQueueWaitIdle);
 DEV(pCreateCommandPool,vkCreateCommandPool);DEV(pDestroyCommandPool,vkDestroyCommandPool);
 DEV(pAllocateCommandBuffers,vkAllocateCommandBuffers);DEV(pBeginCommandBuffer,vkBeginCommandBuffer);
 DEV(pEndCommandBuffer,vkEndCommandBuffer);DEV(pCmdPipelineBarrier,vkCmdPipelineBarrier);
 DEV(pCmdClearColorImage,vkCmdClearColorImage);DEV(pCreateSemaphore,vkCreateSemaphore);
 DEV(pDestroySemaphore,vkDestroySemaphore);
 VkSurfaceCapabilitiesKHR caps{};r=pSurfaceCaps(gpu,surface,&caps);Result("Surface capabilities",r);if(r!=VK_SUCCESS)return false;
 sprintf_s(inf,"SURFACE minImages=%u maxImages=%u currentExtent=%ux%u minExtent=%ux%u maxExtent=%ux%u usage=0x%08X transforms=0x%08X compositeAlpha=0x%08X",caps.minImageCount,caps.maxImageCount,caps.currentExtent.width,caps.currentExtent.height,caps.minImageExtent.width,caps.minImageExtent.height,caps.maxImageExtent.width,caps.maxImageExtent.height,caps.supportedUsageFlags,caps.supportedTransforms,caps.supportedCompositeAlpha);Log(inf);
 if(!(caps.supportedUsageFlags&VK_IMAGE_USAGE_TRANSFER_DST_BIT)){Log("Surface does not support VK_IMAGE_USAGE_TRANSFER_DST_BIT; cannot run clear/present test");return false;}
 uint32_t fn=0;r=pSurfaceFormats(gpu,surface,&fn,nullptr);Result("Surface formats count",r);if(r!=VK_SUCCESS||!fn)return false;
 std::vector<VkSurfaceFormatKHR> formats(fn);r=pSurfaceFormats(gpu,surface,&fn,formats.data());if(r!=VK_SUCCESS)return false;
 for(uint32_t i=0;i<fn;i++){sprintf_s(inf,"SURFACE_FORMAT[%u] format=%d colorSpace=%d",i,(int)formats[i].format,(int)formats[i].colorSpace);Log(inf);}
 VkExtent2D extent=caps.currentExtent;if(extent.width==UINT32_MAX){extent.width=uwp_get_width();extent.height=uwp_get_height();}
 uint32_t count=(std::max)(2u,caps.minImageCount);if(caps.maxImageCount&&count>caps.maxImageCount)count=caps.maxImageCount;
 VkSwapchainCreateInfoKHR si{VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR};si.surface=surface;si.minImageCount=count;si.imageFormat=formats[0].format;si.imageColorSpace=formats[0].colorSpace;si.imageExtent=extent;si.imageArrayLayers=1;si.imageUsage=VK_IMAGE_USAGE_TRANSFER_DST_BIT;si.imageSharingMode=VK_SHARING_MODE_EXCLUSIVE;si.preTransform=caps.currentTransform;si.compositeAlpha=VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;si.presentMode=VK_PRESENT_MODE_FIFO_KHR;si.clipped=VK_TRUE;
 r=pCreateSwapchain(device,&si,nullptr,&swapchain);Result("vkCreateSwapchainKHR(CoreWindow)",r);if(r!=VK_SUCCESS)return false;
 VkQueue queue=VK_NULL_HANDLE;pGetDeviceQueue(device,family,0,&queue);if(!queue){Log("vkGetDeviceQueue returned VK_NULL_HANDLE");return false;}
 uint32_t imageCount=0;r=pGetSwapchainImages(device,swapchain,&imageCount,nullptr);Result("vkGetSwapchainImagesKHR(count)",r);if(r!=VK_SUCCESS||!imageCount)return false;
 std::vector<VkImage> images(imageCount);r=pGetSwapchainImages(device,swapchain,&imageCount,images.data());Result("vkGetSwapchainImagesKHR(data)",r);if(r!=VK_SUCCESS||!imageCount)return false;
 VkSemaphoreCreateInfo semInfo{VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};VkSemaphore acquired=VK_NULL_HANDLE,rendered=VK_NULL_HANDLE;
 r=pCreateSemaphore(device,&semInfo,nullptr,&acquired);Result("vkCreateSemaphore(acquire)",r);if(r!=VK_SUCCESS)return false;
 r=pCreateSemaphore(device,&semInfo,nullptr,&rendered);Result("vkCreateSemaphore(render)",r);if(r!=VK_SUCCESS){pDestroySemaphore(device,acquired,nullptr);return false;}
 VkCommandPoolCreateInfo poolInfo{VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};poolInfo.queueFamilyIndex=family;
 VkCommandPool pool=VK_NULL_HANDLE;r=pCreateCommandPool(device,&poolInfo,nullptr,&pool);Result("vkCreateCommandPool",r);if(r!=VK_SUCCESS){pDestroySemaphore(device,rendered,nullptr);pDestroySemaphore(device,acquired,nullptr);return false;}
 uint32_t imageIndex=0;r=pAcquireNextImage(device,swapchain,UINT64_MAX,acquired,VK_NULL_HANDLE,&imageIndex);Result("vkAcquireNextImageKHR",r);if(r!=VK_SUCCESS&&r!=VK_SUBOPTIMAL_KHR){pDestroyCommandPool(device,pool,nullptr);pDestroySemaphore(device,rendered,nullptr);pDestroySemaphore(device,acquired,nullptr);return false;}
 VkCommandBufferAllocateInfo alloc{VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};alloc.commandPool=pool;alloc.level=VK_COMMAND_BUFFER_LEVEL_PRIMARY;alloc.commandBufferCount=1;
 VkCommandBuffer cmd=VK_NULL_HANDLE;r=pAllocateCommandBuffers(device,&alloc,&cmd);Result("vkAllocateCommandBuffers",r);if(r!=VK_SUCCESS){pDestroyCommandPool(device,pool,nullptr);pDestroySemaphore(device,rendered,nullptr);pDestroySemaphore(device,acquired,nullptr);return false;}
 VkCommandBufferBeginInfo begin{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};begin.flags=VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
 r=pBeginCommandBuffer(cmd,&begin);Result("vkBeginCommandBuffer",r);
 if(r==VK_SUCCESS){
  VkImageMemoryBarrier toClear{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};toClear.srcAccessMask=0;toClear.dstAccessMask=VK_ACCESS_TRANSFER_WRITE_BIT;toClear.oldLayout=VK_IMAGE_LAYOUT_UNDEFINED;toClear.newLayout=VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;toClear.srcQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED;toClear.dstQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED;toClear.image=images[imageIndex];toClear.subresourceRange={VK_IMAGE_ASPECT_COLOR_BIT,0,1,0,1};
  pCmdPipelineBarrier(cmd,VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,VK_PIPELINE_STAGE_TRANSFER_BIT,0,0,nullptr,0,nullptr,1,&toClear);
  VkClearColorValue color{{0.035f,0.12f,0.28f,1.0f}};VkImageSubresourceRange range{VK_IMAGE_ASPECT_COLOR_BIT,0,1,0,1};
  pCmdClearColorImage(cmd,images[imageIndex],VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,&color,1,&range);
  VkImageMemoryBarrier toPresent{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};toPresent.srcAccessMask=VK_ACCESS_TRANSFER_WRITE_BIT;toPresent.dstAccessMask=0;toPresent.oldLayout=VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;toPresent.newLayout=VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;toPresent.srcQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED;toPresent.dstQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED;toPresent.image=images[imageIndex];toPresent.subresourceRange=range;
  pCmdPipelineBarrier(cmd,VK_PIPELINE_STAGE_TRANSFER_BIT,VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,0,0,nullptr,0,nullptr,1,&toPresent);
  r=pEndCommandBuffer(cmd);Result("vkEndCommandBuffer",r);
 }
 if(r==VK_SUCCESS){
  VkPipelineStageFlags waitStage=VK_PIPELINE_STAGE_TRANSFER_BIT;VkSubmitInfo submit{VK_STRUCTURE_TYPE_SUBMIT_INFO};submit.waitSemaphoreCount=1;submit.pWaitSemaphores=&acquired;submit.pWaitDstStageMask=&waitStage;submit.commandBufferCount=1;submit.pCommandBuffers=&cmd;submit.signalSemaphoreCount=1;submit.pSignalSemaphores=&rendered;
  r=pQueueSubmit(queue,1,&submit,VK_NULL_HANDLE);Result("vkQueueSubmit(clear)",r);
 }
 if(r==VK_SUCCESS){
  VkPresentInfoKHR present{VK_STRUCTURE_TYPE_PRESENT_INFO_KHR};present.waitSemaphoreCount=1;present.pWaitSemaphores=&rendered;present.swapchainCount=1;present.pSwapchains=&swapchain;present.pImageIndices=&imageIndex;
  r=pQueuePresent(queue,&present);Result("vkQueuePresentKHR(clear frame)",r);
  if(r==VK_SUCCESS||r==VK_SUBOPTIMAL_KHR){VkResult idle=pQueueWaitIdle(queue);Result("vkQueueWaitIdle(after present)",idle);if(idle==VK_SUCCESS)Log("PASS: presented one Vulkan clear-color frame to CoreWindow");else r=idle;}
 }
 pDestroyCommandPool(device,pool,nullptr);pDestroySemaphore(device,rendered,nullptr);pDestroySemaphore(device,acquired,nullptr);
 if(r!=VK_SUCCESS&&r!=VK_SUBOPTIMAL_KHR)return false;
 sprintf_s(inf,"PROBE FRAME: %ux%u",extent.width,extent.height);Log(inf);return true;
}
struct ProbeView:winrt::implements<ProbeView,core::IFrameworkView>{
 void Initialize(core::CoreApplicationView const&){OpenLog();}
 void SetWindow(ui::CoreWindow const&w){window=w;window.Closed([](auto const&,auto const&){closed=true;});}
 void Load(winrt::hstring const&){}
 void Run(){try{window.Activate();Log("CoreWindow activated");const bool ok=Probe();Log(ok?"PROBE RESULT: PASS":"PROBE RESULT: FAIL");}catch(winrt::hresult_error const&e){char b[128];sprintf_s(b,"WinRT exception 0x%08lx",(unsigned long)e.code().value);Log(b);}catch(...){Log("Unhandled probe exception");}
  while(!closed)window.Dispatcher().ProcessEvents(ui::CoreProcessEventsOption::ProcessOneAndAllPending);
  if(swapchain&&pDestroySwapchain)pDestroySwapchain(device,swapchain,nullptr);if(device&&pDestroyDevice)pDestroyDevice(device,nullptr);
  if(surface&&pDestroySurface)pDestroySurface(instance,surface,nullptr);if(instance&&pDestroyInstance)pDestroyInstance(instance,nullptr);
  if(driver)FreeLibrary(driver);if(logFile!=INVALID_HANDLE_VALUE)CloseHandle(logFile);
 }
 void Uninitialize(){}
};
struct ProbeSource:winrt::implements<ProbeSource,core::IFrameworkViewSource>{core::IFrameworkView CreateView(){return winrt::make<ProbeView>();}};
int __stdcall wWinMain(HINSTANCE,HINSTANCE,PWSTR,int){try{OutputDebugStringA("TriAevum Vulkan Probe: entering CoreApplication::Run\r\n");core::CoreApplication::Run(winrt::make<ProbeSource>());}catch(...){Log("CoreApplication::Run failed");}return 0;}
