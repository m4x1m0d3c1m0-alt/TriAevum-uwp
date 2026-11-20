#define VK_USE_PLATFORM_WIN32_KHR
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
 INST(pEnumDevExt,vkEnumerateDeviceExtensionProperties);INST(pGetQueues,vkGetPhysicalDeviceQueueFamilyProperties);
 INST(pCreateSurface,vkCreateWin32SurfaceKHR);INST(pDestroySurface,vkDestroySurfaceKHR);
 INST(pSurfaceSupport,vkGetPhysicalDeviceSurfaceSupportKHR);INST(pSurfaceCaps,vkGetPhysicalDeviceSurfaceCapabilitiesKHR);
 INST(pSurfaceFormats,vkGetPhysicalDeviceSurfaceFormatsKHR);INST(pCreateDevice,vkCreateDevice);INST(gdpa,vkGetDeviceProcAddr);
 uint32_t pc=0;r=pEnumPhysical(instance,&pc,nullptr);Result("Enumerate physical devices",r);if(r!=VK_SUCCESS||!pc)return false;
 std::vector<VkPhysicalDevice> phys(pc);r=pEnumPhysical(instance,&pc,phys.data());if(r!=VK_SUCCESS)return false;
 VkPhysicalDevice gpu=phys[0];VkPhysicalDeviceProperties props{};pGetProps(gpu,&props);Log(props.deviceName);
 char inf[128];sprintf_s(inf,"Vulkan %u.%u.%u, vendor %04x device %04x",VK_VERSION_MAJOR(props.apiVersion),VK_VERSION_MINOR(props.apiVersion),VK_VERSION_PATCH(props.apiVersion),props.vendorID,props.deviceID);Log(inf);
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
 VkSurfaceCapabilitiesKHR caps{};r=pSurfaceCaps(gpu,surface,&caps);Result("Surface capabilities",r);if(r!=VK_SUCCESS)return false;
 uint32_t fn=0;r=pSurfaceFormats(gpu,surface,&fn,nullptr);Result("Surface formats count",r);if(r!=VK_SUCCESS||!fn)return false;
 std::vector<VkSurfaceFormatKHR> formats(fn);r=pSurfaceFormats(gpu,surface,&fn,formats.data());if(r!=VK_SUCCESS)return false;
 VkExtent2D extent=caps.currentExtent;if(extent.width==UINT32_MAX){extent.width=uwp_get_width();extent.height=uwp_get_height();}
 uint32_t count=(std::max)(2u,caps.minImageCount);if(caps.maxImageCount&&count>caps.maxImageCount)count=caps.maxImageCount;
 VkSwapchainCreateInfoKHR si{VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR};si.surface=surface;si.minImageCount=count;si.imageFormat=formats[0].format;si.imageColorSpace=formats[0].colorSpace;si.imageExtent=extent;si.imageArrayLayers=1;si.imageUsage=VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;si.imageSharingMode=VK_SHARING_MODE_EXCLUSIVE;si.preTransform=caps.currentTransform;si.compositeAlpha=VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;si.presentMode=VK_PRESENT_MODE_FIFO_KHR;si.clipped=VK_TRUE;
 r=pCreateSwapchain(device,&si,nullptr,&swapchain);Result("vkCreateSwapchainKHR(CoreWindow)",r);if(r!=VK_SUCCESS)return false;
 sprintf_s(inf,"PASS: Vulkan device + CoreWindow swapchain %ux%u",extent.width,extent.height);Log(inf);return true;
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
int __stdcall wWinMain(HINSTANCE,HINSTANCE,PWSTR,int){try{winrt::init_apartment(winrt::apartment_type::single_threaded);core::CoreApplication::Run(winrt::make<ProbeSource>());}catch(...){Log("CoreApplication::Run failed");}return 0;}
