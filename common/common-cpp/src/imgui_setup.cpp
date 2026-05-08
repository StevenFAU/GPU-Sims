#include <gpusims/imgui_setup.hpp>

#include <algorithm>
#include <chrono>
#include <deque>
#include <mutex>
#include <string>

#include <imgui.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_vulkan.h>

#include <gpusims/log.hpp>

namespace gpusims::ui {

namespace {
struct Toast {
    std::string                              text;
    bool                                     success;
    std::chrono::steady_clock::time_point    born;
    float                                    lifetime;
};
std::mutex          g_toast_mu;
std::deque<Toast>   g_toasts;
VkDescriptorPool    g_owned_pool = VK_NULL_HANDLE;
VkDevice            g_device     = VK_NULL_HANDLE;
}  // namespace

VkDescriptorPool createImGuiDescriptorPool(VkDevice device) {
    VkDescriptorPoolSize sizes[] = {
        { VK_DESCRIPTOR_TYPE_SAMPLER,                 1000 },
        { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,  1000 },
        { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,           1000 },
        { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,           1000 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER,    1000 },
        { VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER,    1000 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,          1000 },
        { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,          1000 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC,  1000 },
        { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC,  1000 },
        { VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT,        1000 },
    };
    VkDescriptorPoolCreateInfo pi{};
    pi.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    pi.flags         = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    pi.maxSets       = 1000;
    pi.poolSizeCount = static_cast<std::uint32_t>(std::size(sizes));
    pi.pPoolSizes    = sizes;
    VkDescriptorPool pool = VK_NULL_HANDLE;
    if (vkCreateDescriptorPool(device, &pi, nullptr, &pool) != VK_SUCCESS) {
        logError("imgui: vkCreateDescriptorPool failed");
        return VK_NULL_HANDLE;
    }
    return pool;
}

bool initImGui(const ImGuiInit& cfg) {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    ImGui::StyleColorsDark();
    if (!ImGui_ImplGlfw_InitForVulkan(cfg.glfw_window, true)) {
        logError("imgui: ImGui_ImplGlfw_InitForVulkan failed");
        return false;
    }
    VkDescriptorPool pool = cfg.descriptor_pool;
    if (pool == VK_NULL_HANDLE) {
        pool = createImGuiDescriptorPool(cfg.device);
        if (pool == VK_NULL_HANDLE) return false;
        g_owned_pool = pool;
    }
    g_device = cfg.device;
    VkFormat color_formats[] = { cfg.color_format };
    VkPipelineRenderingCreateInfoKHR rendering_info{};
    rendering_info.sType                   = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO_KHR;
    rendering_info.colorAttachmentCount    = 1;
    rendering_info.pColorAttachmentFormats = color_formats;
    ImGui_ImplVulkan_InitInfo init{};
    init.Instance       = cfg.instance;
    init.PhysicalDevice = cfg.physical_device;
    init.Device         = cfg.device;
    init.QueueFamily    = cfg.queue_family;
    init.Queue          = cfg.queue;
    init.DescriptorPool = pool;
    init.MinImageCount  = cfg.min_image_count;
    init.ImageCount     = cfg.image_count;
    init.MSAASamples    = cfg.samples;
    init.UseDynamicRendering = true;
    init.PipelineRenderingCreateInfo = rendering_info;
    init.Subpass        = 0;
    init.Allocator      = nullptr;
    init.CheckVkResultFn = [](VkResult r) {
        if (r != VK_SUCCESS) logError("imgui-vk: VkResult {}", static_cast<int>(r));
    };
    if (!ImGui_ImplVulkan_Init(&init)) {
        logError("imgui: ImGui_ImplVulkan_Init failed");
        return false;
    }
    return true;
}

void newImGuiFrame() {
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
}

void renderImGui(VkCommandBuffer cmd) {
    ImGui::Render();
    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmd);
}

void shutdownImGui() {
    if (g_device != VK_NULL_HANDLE) {
        ImGui_ImplVulkan_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
        if (g_owned_pool != VK_NULL_HANDLE) {
            vkDestroyDescriptorPool(g_device, g_owned_pool, nullptr);
            g_owned_pool = VK_NULL_HANDLE;
        }
        g_device = VK_NULL_HANDLE;
    }
    std::lock_guard<std::mutex> lock(g_toast_mu);
    g_toasts.clear();
}

void pushToast(const char* text, bool success, float lifetime_seconds) {
    Toast t;
    t.text     = text ? text : "";
    t.success  = success;
    t.born     = std::chrono::steady_clock::now();
    t.lifetime = lifetime_seconds;
    std::lock_guard<std::mutex> lock(g_toast_mu);
    g_toasts.push_back(std::move(t));
    if (g_toasts.size() > 16) g_toasts.pop_front();
}

void drawToasts() {
    std::vector<Toast> snapshot;
    {
        std::lock_guard<std::mutex> lock(g_toast_mu);
        const auto now = std::chrono::steady_clock::now();
        while (!g_toasts.empty()) {
            const auto& f = g_toasts.front();
            const float elapsed = std::chrono::duration<float>(now - f.born).count();
            if (elapsed > f.lifetime) g_toasts.pop_front(); else break;
        }
        snapshot.assign(g_toasts.begin(), g_toasts.end());
    }
    if (snapshot.empty()) return;
    const ImGuiViewport* vp = ImGui::GetMainViewport();
    const float pad = 10.0f;
    ImVec2 pos(vp->WorkPos.x + vp->WorkSize.x - pad,
               vp->WorkPos.y + pad);
    const ImVec2 pivot(1.0f, 0.0f);
    const auto now = std::chrono::steady_clock::now();
    for (std::size_t i = 0; i < snapshot.size(); ++i) {
        const auto& t = snapshot[i];
        const float elapsed = std::chrono::duration<float>(now - t.born).count();
        const float alpha = std::clamp(1.0f - (elapsed / t.lifetime), 0.0f, 1.0f);
        ImGui::SetNextWindowPos(pos, ImGuiCond_Always, pivot);
        ImGui::SetNextWindowBgAlpha(0.85f * alpha);
        ImGui::PushStyleColor(ImGuiCol_Text,
            t.success ? ImVec4(0.6f, 1.0f, 0.6f, alpha) : ImVec4(1.0f, 0.5f, 0.5f, alpha));
        char label[64];
        std::snprintf(label, sizeof(label), "##toast_%zu", i);
        const ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration |
                                       ImGuiWindowFlags_AlwaysAutoResize |
                                       ImGuiWindowFlags_NoSavedSettings |
                                       ImGuiWindowFlags_NoFocusOnAppearing |
                                       ImGuiWindowFlags_NoNav |
                                       ImGuiWindowFlags_NoMove;
        if (ImGui::Begin(label, nullptr, flags)) {
            ImGui::TextUnformatted(t.text.c_str());
        }
        const float h = ImGui::GetWindowHeight() + 4.0f;
        ImGui::End();
        ImGui::PopStyleColor();
        pos.y += h;
    }
}

}  // namespace gpusims::ui
