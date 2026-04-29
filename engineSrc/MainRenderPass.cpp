#include "MainRenderPass.h"

#include "VulkanDevice.h"
#include "VulkanWindow.h"

#include <array>
#include <stdexcept>

void MainRenderPass::init( VulkanDevice& device, VulkanWindow& window )
{
    _device = &device;

    // --- Attachment descriptions -------------------------------------------

    // 0 — present output: imagen final que se envía a pantalla.
    VkAttachmentDescription presentAttachment{};
    presentAttachment.format         = window.getSwapChainFormat();
    presentAttachment.samples        = VK_SAMPLE_COUNT_1_BIT;
    presentAttachment.loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
    presentAttachment.storeOp        = VK_ATTACHMENT_STORE_OP_STORE;
    presentAttachment.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    presentAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    presentAttachment.initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
    presentAttachment.finalLayout    = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    // 1 — depth/stencil.
    VkAttachmentDescription depthAttachment{};
    depthAttachment.format           = device.findDepthFormat();
    depthAttachment.samples          = VK_SAMPLE_COUNT_1_BIT;
    depthAttachment.loadOp           = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthAttachment.storeOp          = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.stencilLoadOp    = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depthAttachment.stencilStoreOp   = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.initialLayout    = VK_IMAGE_LAYOUT_UNDEFINED;
    depthAttachment.finalLayout      = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    // 2 — GBuffer albedo/color (transient: solo vive dentro del tile en TBDR).
    VkAttachmentDescription colorAttachment{};
    colorAttachment.format           = window.getSwapChainFormat();
    colorAttachment.samples          = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp           = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp          = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp    = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp   = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout    = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout      = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    // 3 — GBuffer normales.
    VkAttachmentDescription normalAttachment{};
    normalAttachment.format          = VK_FORMAT_R16G16B16A16_SFLOAT;
    normalAttachment.samples         = VK_SAMPLE_COUNT_1_BIT;
    normalAttachment.loadOp          = VK_ATTACHMENT_LOAD_OP_CLEAR;
    normalAttachment.storeOp         = VK_ATTACHMENT_STORE_OP_STORE;
    normalAttachment.stencilLoadOp   = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    normalAttachment.stencilStoreOp  = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    normalAttachment.initialLayout   = VK_IMAGE_LAYOUT_UNDEFINED;
    normalAttachment.finalLayout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    // 4 — GBuffer posición mundo.
    VkAttachmentDescription posAttachment{};
    posAttachment.format             = VK_FORMAT_R16G16B16A16_SFLOAT;
    posAttachment.samples            = VK_SAMPLE_COUNT_1_BIT;
    posAttachment.loadOp             = VK_ATTACHMENT_LOAD_OP_CLEAR;
    posAttachment.storeOp            = VK_ATTACHMENT_STORE_OP_STORE;
    posAttachment.stencilLoadOp      = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    posAttachment.stencilStoreOp     = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    posAttachment.initialLayout      = VK_IMAGE_LAYOUT_UNDEFINED;
    posAttachment.finalLayout        = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    // --- Attachment references ---------------------------------------------

    // Subpass 0 outputs
    VkAttachmentReference depthRef{};
    depthRef.attachment     = 1;
    depthRef.layout         = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentReference colorRef{};
    colorRef.attachment     = 2;
    colorRef.layout         = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentReference normalRef{};
    normalRef.attachment    = 3;
    normalRef.layout        = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentReference posRef{};
    posRef.attachment       = 4;
    posRef.layout           = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    // Subpass 1 output
    VkAttachmentReference presentRef{};
    presentRef.attachment   = 0;
    presentRef.layout       = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    // Subpass 1 inputs (leen los outputs del subpass 0)
    VkAttachmentReference inputColor{};
    inputColor.attachment   = 2;
    inputColor.layout       = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    VkAttachmentReference inputNormal{};
    inputNormal.attachment  = 3;
    inputNormal.layout      = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    VkAttachmentReference inputPos{};
    inputPos.attachment     = 4;
    inputPos.layout         = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    // --- Subpasses ---------------------------------------------------------

    VkAttachmentReference geometryOutputs[] = { colorRef, normalRef, posRef };

    VkSubpassDescription geometrySubpass{};
    geometrySubpass.pipelineBindPoint       = VK_PIPELINE_BIND_POINT_GRAPHICS;
    geometrySubpass.colorAttachmentCount    = 3;
    geometrySubpass.pColorAttachments       = geometryOutputs;
    geometrySubpass.pDepthStencilAttachment = &depthRef;

    VkAttachmentReference lightingInputs[]  = { inputColor, inputNormal, inputPos };

    VkSubpassDescription lightingSubpass{};
    lightingSubpass.pipelineBindPoint       = VK_PIPELINE_BIND_POINT_GRAPHICS;
    lightingSubpass.colorAttachmentCount    = 1;
    lightingSubpass.pColorAttachments       = &presentRef;
    lightingSubpass.inputAttachmentCount    = 3;
    lightingSubpass.pInputAttachments       = lightingInputs;

    // --- Subpass dependencies ----------------------------------------------
    //
    // dep0: EXTERNAL → subpass 0
    //   Espera a que la imagen de swapchain esté disponible para escribir.
    VkSubpassDependency dep0{};
    dep0.srcSubpass      = VK_SUBPASS_EXTERNAL;
    dep0.dstSubpass      = 0;
    dep0.srcStageMask    = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
    dep0.dstStageMask    = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
                           VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dep0.srcAccessMask   = VK_ACCESS_MEMORY_READ_BIT;
    dep0.dstAccessMask   = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT  |
                           VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT |
                           VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

    // dep1: subpass 0 → subpass 1
    //   Garantiza que los GBuffer attachments estén escritos antes de que
    //   el lighting subpass los lea como input attachments.
    VkSubpassDependency dep1{};
    dep1.srcSubpass      = 0;
    dep1.dstSubpass      = 1;
    dep1.srcStageMask    = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dep1.dstStageMask    = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    dep1.srcAccessMask   = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    dep1.dstAccessMask   = VK_ACCESS_INPUT_ATTACHMENT_READ_BIT;
    dep1.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

    // dep2: subpass 0 → EXTERNAL
    //   Libera los attachments para lectura por otros stages.
    VkSubpassDependency dep2{};
    dep2.srcSubpass      = 0;
    dep2.dstSubpass      = VK_SUBPASS_EXTERNAL;
    dep2.srcStageMask    = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dep2.dstStageMask    = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
    dep2.srcAccessMask   = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT |
                           VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    dep2.dstAccessMask   = VK_ACCESS_MEMORY_READ_BIT;
    dep2.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

    // dep3: EXTERNAL → subpass 1
    //   El compute de light culling escribe en un storage buffer que el
    //   fragment shader del lighting subpass lee. La barrera de pipeline
    //   en recordCommandBuffer() sincroniza buffers; esta dependencia
    //   cubre la sincronización del subpass contra el compute externo.
    VkSubpassDependency dep3{};
    dep3.srcSubpass      = VK_SUBPASS_EXTERNAL;
    dep3.dstSubpass      = 1;
    dep3.srcStageMask    = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
    dep3.dstStageMask    = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    dep3.srcAccessMask   = VK_ACCESS_SHADER_WRITE_BIT;
    dep3.dstAccessMask   = VK_ACCESS_SHADER_READ_BIT;
    dep3.dependencyFlags = VK_DEPENDENCY_DEVICE_GROUP_BIT;

    // --- Render pass -------------------------------------------------------

    std::array<VkAttachmentDescription, 5> attachments = {
        presentAttachment,
        depthAttachment,
        colorAttachment,
        normalAttachment,
        posAttachment
    };

    VkSubpassDescription subpasses[] = { geometrySubpass, lightingSubpass };
    VkSubpassDependency  deps[]      = { dep0, dep1, dep2, dep3 };

    VkRenderPassCreateInfo info{};
    info.sType           = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    info.attachmentCount = static_cast<uint32_t>( attachments.size() );
    info.pAttachments    = attachments.data();
    info.subpassCount    = 2;
    info.pSubpasses      = subpasses;
    info.dependencyCount = 4;
    info.pDependencies   = deps;

    _renderPass = _device->createRenderPass( info );
}

void MainRenderPass::destroy()
{
    if ( _renderPass != VK_NULL_HANDLE ) {
        _device->destroyRenderPass( _renderPass );
        _renderPass = VK_NULL_HANDLE;
    }
}
