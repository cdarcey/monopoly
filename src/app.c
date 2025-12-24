/*
   app.c - Monopoly Game
*/

//-----------------------------------------------------------------------------
// [SECTION] includes
//-----------------------------------------------------------------------------

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// pilot light core
#include "pl.h"
#include "pl_ds.h"
#include "pl_memory.h"
#define PL_MATH_INCLUDE_FUNCTIONS
#include "pl_math.h"

// pilot light extensions
#include "pl_graphics_ext.h"
#include "pl_draw_ext.h"
#include "pl_draw_backend_ext.h"
#include "pl_shader_ext.h"

// monopoly game logic
#include "m_init_game.h"

// libraries
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

//-----------------------------------------------------------------------------
// [SECTION] helper macros
//-----------------------------------------------------------------------------

// convert rgba floats (0-1) to packed uint32_t color
#define PL_COLOR_32(r, g, b, a) ((uint32_t)((uint8_t)((a)*255.0f) << 24 | (uint8_t)((b)*255.0f) << 16 | (uint8_t)((g)*255.0f) << 8 | (uint8_t)((r)*255.0f)))

//-----------------------------------------------------------------------------
// [SECTION] forward declarations
//-----------------------------------------------------------------------------

// forward declare structs
typedef struct _plAppData plAppData;
typedef struct _plTextureLoadConfig plTextureLoadConfig;


// input handling
void handle_keyboard_input(plAppData* ptAppData);

// helper functions
plVec2   board_position_to_screen(uint32_t uBoardPosition);
uint32_t get_player_color(mPlayerPiece ePiece);
void     load_board_textures(plAppData* ptAppData);
void     load_texture(plAppData* ptAppData, const plTextureLoadConfig* ptConfig);

//-----------------------------------------------------------------------------
// [SECTION] structs
//-----------------------------------------------------------------------------

typedef struct _plAppData
{
    // window & device
    plWindow* ptWindow;
    plDevice* ptDevice;

    // command infrastructure
    plCommandPool* ptCommandPool;

    // bind groups
    plBindGroupPool*        tBindGroupPoolTexAndSamp;
    plBindGroupLayoutHandle tTextureBindGroupLayout;
    plBindGroupLayoutDesc   tTextureBindGroupLayoutDesc;

    // samplers
    plSamplerHandle tLinearSampler;
    plSamplerHandle tNearestSampler;

    // swapchain
    plSwapchain*     ptSwapchain;
    plTextureHandle* atSwapchainImages;
    uint32_t         uSwapchainImageCount;

    // render passes
    plRenderPassLayoutHandle tMainPassLayout;
    plRenderPassHandle       tRenderPass;

    // shaders
    plShaderHandle tTexturedQuadShader;

    // geometry
    plBufferHandle           tQuadVertexBuffer;
    plBufferHandle           tQuadIndexBuffer;
    plDeviceMemoryAllocation tQuadVertexMemory;
    plDeviceMemoryAllocation tQuadIndexMemory;

    // textures
    plTextureHandle          tBoardTexture;
    plDeviceMemoryAllocation tBoardTextureMemory;
    plBindGroupHandle        tBoardBindGroup;
    bool                     bBoardTextureLoaded;

    // property cards (28 total)
    plTextureHandle          atPropertyTextures[28];
    plDeviceMemoryAllocation atPropertyMemory[28];
    plBindGroupHandle        atPropertyBindGroups[28];

    // monopoly game state
    mGameData* pGameData;
    mGameFlow  tGameFlow;

    // board layout
    float fBoardX;
    float fBoardY;
    float fBoardSize;
    float fSquareSize;

    // ui state
    bool          bShowPropertyPopup;
    mPropertyName ePropertyToShow;

} plAppData;

// texture loading configuration
typedef struct _plTextureLoadConfig
{
    const char*               pcFilePath;
    plSamplerHandle           tSampler;
    plTextureHandle*          ptOutTexture;
    plDeviceMemoryAllocation* ptOutMemory;
    plBindGroupHandle*        ptOutBindGroup;
    bool*                     pbOutLoaded;
} plTextureLoadConfig;

//-----------------------------------------------------------------------------
// [SECTION] global api pointers
//-----------------------------------------------------------------------------

const plIOI*          gptIO          = NULL;
const plWindowI*      gptWindows     = NULL;
const plGraphicsI*    gptGfx         = NULL;
const plDrawI*        gptDraw        = NULL;
const plDrawBackendI* gptDrawBackend = NULL;
const plShaderI*      gptShader      = NULL;

//-----------------------------------------------------------------------------
// [SECTION] pl_app_load
//-----------------------------------------------------------------------------

PL_EXPORT void*
pl_app_load(plApiRegistryI* ptApiRegistry, plAppData* ptAppData)
{
    // hot reload path - re-retrieve APIs
    if(ptAppData)
    {
        gptIO          = pl_get_api_latest(ptApiRegistry, plIOI);
        gptWindows     = pl_get_api_latest(ptApiRegistry, plWindowI);
        gptGfx         = pl_get_api_latest(ptApiRegistry, plGraphicsI);
        gptDraw        = pl_get_api_latest(ptApiRegistry, plDrawI);
        gptDrawBackend = pl_get_api_latest(ptApiRegistry, plDrawBackendI);
        gptShader      = pl_get_api_latest(ptApiRegistry, plShaderI);
        return ptAppData;
    }

    // first load - allocate app memory
    ptAppData = malloc(sizeof(plAppData));
    memset(ptAppData, 0, sizeof(plAppData));

    // load extensions
    const plExtensionRegistryI* ptExtensionRegistry = pl_get_api_latest(ptApiRegistry, plExtensionRegistryI);
    ptExtensionRegistry->load("pl_unity_ext", NULL, NULL, true);
    ptExtensionRegistry->load("pl_platform_ext", NULL, NULL, false);

    // get APIs
    gptIO          = pl_get_api_latest(ptApiRegistry, plIOI);
    gptWindows     = pl_get_api_latest(ptApiRegistry, plWindowI);
    gptGfx         = pl_get_api_latest(ptApiRegistry, plGraphicsI);
    gptDraw        = pl_get_api_latest(ptApiRegistry, plDrawI);
    gptDrawBackend = pl_get_api_latest(ptApiRegistry, plDrawBackendI);
    gptShader      = pl_get_api_latest(ptApiRegistry, plShaderI);

    // initialize graphics with validation enabled
    plGraphicsInit tGraphicsInit = {
        .uFramesInFlight = 2,
        .tFlags          = PL_GRAPHICS_INIT_FLAGS_SWAPCHAIN_ENABLED | PL_GRAPHICS_INIT_FLAGS_VALIDATION_ENABLED
    };
    gptGfx->initialize(&tGraphicsInit);

    // create window
    plWindowDesc tWindowDesc = {
        .pcTitle = "Monopoly",
        .iXPos   = 100,
        .iYPos   = 100,
        .uWidth  = 1280,
        .uHeight = 720,
    };
    gptWindows->create(tWindowDesc, &ptAppData->ptWindow);
    gptWindows->show(ptAppData->ptWindow);

    // create surface and device
    plSurface* ptSurface = gptGfx->create_surface(ptAppData->ptWindow);

    const plDeviceInit tDeviceInit = {
        .uDeviceIdx               = 0,
        .szDynamicBufferBlockSize = 65536,
        .szDynamicDataMaxSize     = 65536,
        .ptSurface                = ptSurface
    };
    ptAppData->ptDevice = gptGfx->create_device(&tDeviceInit);

    // initialize shader system
    static const plShaderOptions tDefaultShaderOptions = {
        .apcIncludeDirectories = {
            "../../monopoly/shaders/"
        },
        .apcDirectories = {
            "../../monopoly/shaders/"
        },
        .tFlags = PL_SHADER_FLAGS_AUTO_OUTPUT | PL_SHADER_FLAGS_NEVER_CACHE
    };
    gptShader->initialize(&tDefaultShaderOptions);

    // create swapchain
    plSwapchainInit tSwapchainInit = {
        .bVSync       = true,
        .tSampleCount = PL_SAMPLE_COUNT_1,
        .uWidth       = 1280,
        .uHeight      = 720
    };
    ptAppData->ptSwapchain = gptGfx->create_swapchain(ptAppData->ptDevice, ptSurface, &tSwapchainInit);

    // get swapchain images
    uint32_t uImageCount = 0;
    ptAppData->atSwapchainImages = gptGfx->get_swapchain_images(ptAppData->ptSwapchain, &uImageCount);
    ptAppData->uSwapchainImageCount = uImageCount;

    // create command pool
    plCommandPoolDesc tPoolDesc = {0};
    ptAppData->ptCommandPool = gptGfx->create_command_pool(ptAppData->ptDevice, &tPoolDesc);

    // create samplers
    plSamplerDesc tLinearSamplerDesc = {
        .tMagFilter    = PL_FILTER_LINEAR,
        .tMinFilter    = PL_FILTER_LINEAR,
        .tMipmapMode   = PL_MIPMAP_MODE_LINEAR,
        .tUAddressMode = PL_ADDRESS_MODE_CLAMP_TO_EDGE,
        .tVAddressMode = PL_ADDRESS_MODE_CLAMP_TO_EDGE,
        .fMinMip       = 0.0f,
        .fMaxMip       = 1.0f,
        .pcDebugName   = "sampler_linear" 
    };
    ptAppData->tLinearSampler = gptGfx->create_sampler(ptAppData->ptDevice, &tLinearSamplerDesc);

    plSamplerDesc tNearSamplerDesc = {
        .tMagFilter    = PL_FILTER_NEAREST,
        .tMinFilter    = PL_FILTER_NEAREST,
        .tMipmapMode   = PL_MIPMAP_MODE_NEAREST,
        .tUAddressMode = PL_ADDRESS_MODE_CLAMP_TO_EDGE,
        .tVAddressMode = PL_ADDRESS_MODE_CLAMP_TO_EDGE,
        .fMinMip       = 0.0f,
        .fMaxMip       = 1.0f,
        .pcDebugName   = "sampler_nearest"
    };
    ptAppData->tNearestSampler = gptGfx->create_sampler(ptAppData->ptDevice, &tNearSamplerDesc);

    // create bind group layout
    const plBindGroupLayoutDesc tBindGroupLayoutTexAndSamp = {
        .atTextureBindings = {
            {
                .uSlot   = 0,
                .tType   = PL_TEXTURE_BINDING_TYPE_SAMPLED,
                .tStages = PL_SHADER_STAGE_FRAGMENT
            }
        },
        .atSamplerBindings = {
            {
                .uSlot   = 1,
                .tStages = PL_SHADER_STAGE_FRAGMENT
            }
        },
        .pcDebugName = "texture + sampler layout"
    };
    ptAppData->tTextureBindGroupLayoutDesc = tBindGroupLayoutTexAndSamp;
    ptAppData->tTextureBindGroupLayout = gptGfx->create_bind_group_layout(ptAppData->ptDevice, &tBindGroupLayoutTexAndSamp);

    // create bind group pool
    const plBindGroupPoolDesc tBindGroupPoolTexAndSampDesc = {
        .szSampledTextureBindings = 100,
        .szSamplerBindings        = 100
    };
    ptAppData->tBindGroupPoolTexAndSamp = gptGfx->create_bind_group_pool(ptAppData->ptDevice, &tBindGroupPoolTexAndSampDesc);

    // load board texture
    plTextureLoadConfig tBoardConfig = {
        .pcFilePath      = "D:/Dev/monopoly/assets/monopoly-board.png",
        .tSampler        = ptAppData->tLinearSampler,
        .ptOutTexture    = &ptAppData->tBoardTexture,
        .ptOutMemory     = &ptAppData->tBoardTextureMemory,
        .ptOutBindGroup  = &ptAppData->tBoardBindGroup,
        .pbOutLoaded     = &ptAppData->bBoardTextureLoaded
    };
    load_texture(ptAppData, &tBoardConfig);

    // create render pass layout
    const plRenderPassLayoutDesc tMainRenderPassLayoutDesc = {
        .atRenderTargets = {
            {
                .tFormat  = PL_FORMAT_R8G8B8A8_UNORM,
                .tSamples = PL_SAMPLE_COUNT_1,
                .bResolve = false,
                .bDepth   = false
            }
        },
        .atSubpasses = {
            {
                .uRenderTargetCount = 1,
                .auRenderTargets    = {0}
            }
        },
        .pcDebugName = "main pass layout"
    };
    ptAppData->tMainPassLayout = gptGfx->create_render_pass_layout(ptAppData->ptDevice, &tMainRenderPassLayoutDesc);

    // load shaders
    plShaderModule tVertModule = gptShader->load_glsl("textured_quad.vert", "main", NULL, NULL);
    plShaderModule tFragModule = gptShader->load_glsl("textured_quad.frag", "main", NULL, NULL);

    // define vertex layout
    plVertexBufferLayout tVertexLayout = {
        .uByteStride = 0,
        .atAttributes = {
            { .tFormat = PL_VERTEX_FORMAT_FLOAT2 },  // position
            { .tFormat = PL_VERTEX_FORMAT_FLOAT2 }   // uv
        }
    };

    // create shader
    plShaderDesc tShaderDesc = {
        .tVertexShader            = tVertModule,
        .tFragmentShader          = tFragModule,
        .atVertexBufferLayouts[0] = tVertexLayout,
        .atBindGroupLayouts[0]    = ptAppData->tTextureBindGroupLayoutDesc,
        .tRenderPassLayout        = ptAppData->tMainPassLayout,
        .pcDebugName              = "textured quad shader"
    };
    ptAppData->tTexturedQuadShader = gptGfx->create_shader(ptAppData->ptDevice, &tShaderDesc);

    // create render pass with swapchain attachments
    plRenderPassDesc tMainPassDesc = {
        .tLayout        = ptAppData->tMainPassLayout,
        .tDimensions    = {1280, 720},
        .atColorTargets = {
            {
                .tLoadOp       = PL_LOAD_OP_CLEAR,
                .tStoreOp      = PL_STORE_OP_STORE,
                .tCurrentUsage = PL_TEXTURE_USAGE_UNSPECIFIED,  // TODO:
                .tNextUsage    = PL_TEXTURE_USAGE_PRESENT,
                .tClearColor   = {0.1f, 0.1f, 0.1f, 1.0f}
            }
        },
        .ptSwapchain = ptAppData->ptSwapchain,
        .pcDebugName = "main render pass"
    };

    // create array of attachments (one per swapchain image)
    plRenderPassAttachments* atAttachments = malloc(sizeof(plRenderPassAttachments) * uImageCount);
    for(uint32_t i = 0; i < uImageCount; i++)
    {
        atAttachments[i].atViewAttachments[0] = ptAppData->atSwapchainImages[i];
    }

    // create render pass
    plRenderPassHandle tRenderPass = gptGfx->create_render_pass(ptAppData->ptDevice, &tMainPassDesc, atAttachments);
    free(atAttachments);
    ptAppData->tRenderPass = tRenderPass;

    // define quad vertices -> neg Y is up.....I think
    const float atVertices[] = {
        -0.5f, -0.5f, 0.0f, 0.0f,
        -0.5f,  0.5f, 0.0f, 1.0f, 
         0.5f,  0.5f, 1.0f, 1.0f,
         0.5f, -0.5f, 1.0f, 0.0f  
    };

    // create vertex buffer
    const plBufferDesc tVertexDesc = {
        .tUsage      = PL_BUFFER_USAGE_VERTEX,
        .szByteSize  = sizeof(float) * PL_ARRAYSIZE(atVertices),
        .pcDebugName = "quad vertices"
    };
    ptAppData->tQuadVertexBuffer = gptGfx->create_buffer(ptAppData->ptDevice, &tVertexDesc, NULL);
    plBuffer* ptVertexBuffer = gptGfx->get_buffer(ptAppData->ptDevice, ptAppData->tQuadVertexBuffer);

    // allocate vertex memory
    const plDeviceMemoryAllocation tVertexBufferAllocation = gptGfx->allocate_memory(ptAppData->ptDevice, ptVertexBuffer->tMemoryRequirements.ulSize, 
            PL_MEMORY_FLAGS_DEVICE_LOCAL, ptVertexBuffer->tMemoryRequirements.uMemoryTypeBits, "vertex buffer memory");
    gptGfx->bind_buffer_to_memory(ptAppData->ptDevice, ptAppData->tQuadVertexBuffer, &tVertexBufferAllocation);

    // create index buffer 
    uint32_t uIndices[] = {
        0, 1, 2,  // first triangle
        0, 2, 3   // second triangle
    };

    // create index buffer
    plBufferDesc tIndexDesc = {
        .tUsage      = PL_BUFFER_USAGE_INDEX,
        .szByteSize  = sizeof(uint32_t) * PL_ARRAYSIZE(uIndices),
        .pcDebugName = "quad indices"
    };
    ptAppData->tQuadIndexBuffer = gptGfx->create_buffer(ptAppData->ptDevice, &tIndexDesc, NULL);
    plBuffer* ptIndexBuffer = gptGfx->get_buffer(ptAppData->ptDevice, ptAppData->tQuadIndexBuffer);

    // allocate index memory
    plDeviceMemoryAllocation tIndexBufferAllocation = gptGfx->allocate_memory(ptAppData->ptDevice, ptIndexBuffer->tMemoryRequirements.ulSize, 
            PL_MEMORY_FLAGS_DEVICE_LOCAL, ptIndexBuffer->tMemoryRequirements.uMemoryTypeBits, "index buffer memory");
    gptGfx->bind_buffer_to_memory(ptAppData->ptDevice, ptAppData->tQuadIndexBuffer, &tIndexBufferAllocation);

    // create staging buffer for uploads
    plBufferDesc tStagingDesc = {
        .tUsage      = PL_BUFFER_USAGE_STAGING,
        .szByteSize  = 4096,
        .pcDebugName = "staging buffer"
    };
    plBufferHandle tStagingHandle = gptGfx->create_buffer(ptAppData->ptDevice, &tStagingDesc, NULL);
    plBuffer* ptStagingBuffer = gptGfx->get_buffer(ptAppData->ptDevice, tStagingHandle); 

    // allocate memory
    plDeviceMemoryAllocation tStagingMem = gptGfx->allocate_memory(ptAppData->ptDevice, ptStagingBuffer->tMemoryRequirements.ulSize, 
            PL_MEMORY_FLAGS_HOST_VISIBLE | PL_MEMORY_FLAGS_HOST_COHERENT, ptStagingBuffer->tMemoryRequirements.uMemoryTypeBits, "staging memory");
    gptGfx->bind_buffer_to_memory(ptAppData->ptDevice, tStagingHandle, &tStagingMem);

    // copy vertex/index data to staging buffer
    memcpy(ptStagingBuffer->tMemoryAllocation.pHostMapped, atVertices, sizeof(float) * PL_ARRAYSIZE(atVertices));
    memcpy(&ptStagingBuffer->tMemoryAllocation.pHostMapped[1024], uIndices, sizeof(uint32_t) * PL_ARRAYSIZE(uIndices));

    // upload to GPU
    plCommandBuffer* ptCmd = gptGfx->request_command_buffer(ptAppData->ptCommandPool, "upload geometry");
    gptGfx->begin_command_recording(ptCmd, NULL);

    plBlitEncoder* ptBlit = gptGfx->begin_blit_pass(ptCmd);
    gptGfx->copy_buffer(ptBlit, tStagingHandle, ptAppData->tQuadVertexBuffer, 0, 0, sizeof(float) * PL_ARRAYSIZE(atVertices));
    gptGfx->copy_buffer(ptBlit, tStagingHandle, ptAppData->tQuadIndexBuffer, 1024, 0, sizeof(uint32_t) * PL_ARRAYSIZE(uIndices));
    gptGfx->end_blit_pass(ptBlit);

    gptGfx->end_command_recording(ptCmd);
    gptGfx->submit_command_buffer(ptCmd, NULL);
    gptGfx->wait_on_command_buffer(ptCmd);
    gptGfx->return_command_buffer(ptCmd);

    // cleanup staging buffer
    gptGfx->destroy_buffer(ptAppData->ptDevice, tStagingHandle);
    gptGfx->free_memory(ptAppData->ptDevice, &tStagingMem);

    // initialize board layout values
    ptAppData->fBoardSize   = 700.0f;
    ptAppData->fSquareSize  = 60.0f;
    ptAppData->fBoardX      = 50.0f;
    ptAppData->fBoardY      = 10.0f;
    ptAppData->bShowPropertyPopup = false;

    return ptAppData;
}

//-----------------------------------------------------------------------------
// [SECTION] pl_app_shutdown
//-----------------------------------------------------------------------------

PL_EXPORT void
pl_app_shutdown(plAppData* ptAppData)
{
    // wait for GPU to finish
    gptGfx->flush_device(ptAppData->ptDevice);

    // cleanup textures (NOT swapchain textures)
    if(ptAppData->bBoardTextureLoaded)
    {
        gptGfx->destroy_texture(ptAppData->ptDevice, ptAppData->tBoardTexture);
        gptGfx->free_memory(ptAppData->ptDevice, &ptAppData->tBoardTextureMemory);
        gptGfx->destroy_bind_group(ptAppData->ptDevice, ptAppData->tBoardBindGroup);
    }

    // cleanup geometry buffers
    gptGfx->destroy_buffer(ptAppData->ptDevice, ptAppData->tQuadVertexBuffer);
    gptGfx->destroy_buffer(ptAppData->ptDevice, ptAppData->tQuadIndexBuffer);
    gptGfx->free_memory(ptAppData->ptDevice, &ptAppData->tQuadVertexMemory);
    gptGfx->free_memory(ptAppData->ptDevice, &ptAppData->tQuadIndexMemory);

    // cleanup render pass
    gptGfx->destroy_render_pass(ptAppData->ptDevice, ptAppData->tRenderPass);

    // cleanup shader
    gptGfx->destroy_shader(ptAppData->ptDevice, ptAppData->tTexturedQuadShader);

    // cleanup render pass layout
    gptGfx->destroy_render_pass_layout(ptAppData->ptDevice, ptAppData->tMainPassLayout);

    // cleanup bind group pool and layout
    gptGfx->cleanup_bind_group_pool(ptAppData->tBindGroupPoolTexAndSamp);
    gptGfx->destroy_bind_group_layout(ptAppData->ptDevice, ptAppData->tTextureBindGroupLayout);

    // cleanup samplers
    gptGfx->destroy_sampler(ptAppData->ptDevice, ptAppData->tLinearSampler);
    gptGfx->destroy_sampler(ptAppData->ptDevice, ptAppData->tNearestSampler);

    // cleanup command pool
    gptGfx->cleanup_command_pool(ptAppData->ptCommandPool);

    // cleanup swapchain (this handles swapchain texture cleanup internally)
    gptGfx->cleanup_swapchain(ptAppData->ptSwapchain);

    // cleanup device
    gptGfx->cleanup_device(ptAppData->ptDevice);

    // cleanup graphics system
    gptGfx->cleanup();

    // cleanup game data
    if(ptAppData->pGameData)  // Add null check
        m_free_game_data(ptAppData->pGameData);

    // cleanup window
    gptWindows->destroy(ptAppData->ptWindow);

    // free app data
    free(ptAppData);
}

//-----------------------------------------------------------------------------
// [SECTION] pl_app_resize
//-----------------------------------------------------------------------------

PL_EXPORT void
pl_app_resize(plWindow* ptWindow, plAppData* ptAppData)
{
    // TODO: handle window resizing
}

//-----------------------------------------------------------------------------
// [SECTION] pl_app_update
//-----------------------------------------------------------------------------

PL_EXPORT void
pl_app_update(plAppData* ptAppData)
{
    // begin frame (waits on fence internally)
    gptGfx->begin_frame(ptAppData->ptDevice);

    // acquire next swapchain image
    bool bSuccess = gptGfx->acquire_swapchain_image(ptAppData->ptSwapchain);
    if(!bSuccess)
        return;

    // get command buffer from pool
    plCommandBuffer* ptCmd = gptGfx->request_command_buffer(ptAppData->ptCommandPool, "main");
    const plBeginCommandInfo tBeginInfo = {
        .uWaitSemaphoreCount = 0  // explicitly set to 0, not UINT32_MAX
    };
    gptGfx->begin_command_recording(ptCmd, &tBeginInfo);

    // begin render pass
    plRenderEncoder* ptRender = gptGfx->begin_render_pass(ptCmd, ptAppData->tRenderPass, NULL);

    // set viewport and scissor
    plRenderViewport tViewport = {.fWidth = 1280, .fHeight = 720, .fMaxDepth = 1.0f};
    gptGfx->set_viewport(ptRender, &tViewport);

    plScissor tScissor = {.uWidth = 1280, .uHeight = 720};
    gptGfx->set_scissor_region(ptRender, &tScissor);

    // bind shader
    gptGfx->bind_shader(ptRender, ptAppData->tTexturedQuadShader);

    // bind vertex buffer
    gptGfx->bind_vertex_buffer(ptRender, ptAppData->tQuadVertexBuffer);

    plDynamicDataBlock tCurrentDynamicBufferBlock = gptGfx->allocate_dynamic_data_block(ptAppData->ptDevice);
    plDynamicBinding tDynamicBinding = pl_allocate_dynamic_data(gptGfx, ptAppData->ptDevice, &tCurrentDynamicBufferBlock);
    plVec4* tTintColor = (plVec4*)tDynamicBinding.pcData;
    tTintColor->r = 1.0f;
    tTintColor->g = 1.0f;
    tTintColor->b = 1.0f;
    tTintColor->a = 1.0f;

    gptGfx->bind_graphics_bind_groups(ptRender, ptAppData->tTexturedQuadShader, 0, 1, &ptAppData->tBoardBindGroup, 0, NULL);
    
    plDrawIndex tDraw = {
        .uIndexCount = 6,
        .tIndexBuffer = ptAppData->tQuadIndexBuffer,
        .uInstanceCount = 1
    };
    gptGfx->draw_indexed(ptRender, 1, &tDraw);

    // // draw
    // gptGfx->bind_graphics_bind_groups(ptRender, ptAppData->tTexturedQuadShader, 0, 1, &ptAppData->tBoardBindGroup, 0, NULL);

    // plDraw tDraw = {
    //     .uVertexCount = 6,
    //     .uInstanceCount = 1
    // };
    // gptGfx->draw(ptRender, 1, &tDraw);
    
    

    // end render pass
    gptGfx->end_render_pass(ptRender);
    gptGfx->end_command_recording(ptCmd);

    // present and return command buffer
    gptGfx->present(ptCmd, NULL, &ptAppData->ptSwapchain, 1);
    gptGfx->return_command_buffer(ptCmd);
}

//-----------------------------------------------------------------------------
// [SECTION] input handling
//-----------------------------------------------------------------------------

void
handle_keyboard_input(plAppData* ptAppData)
{
    if(!m_is_waiting_input(&ptAppData->tGameFlow))
        return;

    if(gptIO->is_key_pressed(PL_KEY_1, false))
        m_set_input_int(&ptAppData->tGameFlow, 1);
    else if(gptIO->is_key_pressed(PL_KEY_2, false))
        m_set_input_int(&ptAppData->tGameFlow, 2);
    else if(gptIO->is_key_pressed(PL_KEY_3, false))
        m_set_input_int(&ptAppData->tGameFlow, 3);
    else if(gptIO->is_key_pressed(PL_KEY_4, false))
        m_set_input_int(&ptAppData->tGameFlow, 4);
    else if(gptIO->is_key_pressed(PL_KEY_5, false))
        m_set_input_int(&ptAppData->tGameFlow, 5);
    else if(gptIO->is_key_pressed(PL_KEY_6, false))
        m_set_input_int(&ptAppData->tGameFlow, 6);
    else if(gptIO->is_key_pressed(PL_KEY_7, false))
        m_set_input_int(&ptAppData->tGameFlow, 7);
    else if(gptIO->is_key_pressed(PL_KEY_8, false))
        m_set_input_int(&ptAppData->tGameFlow, 8);
    else if(gptIO->is_key_pressed(PL_KEY_9, false))
        m_set_input_int(&ptAppData->tGameFlow, 9);

    if(gptIO->is_key_pressed(PL_KEY_ESCAPE, false))
        ptAppData->bShowPropertyPopup = false;
}

//-----------------------------------------------------------------------------
// [SECTION] helper functions
//-----------------------------------------------------------------------------

uint32_t
get_player_color(mPlayerPiece ePiece)
{
    switch(ePiece)
    {
        case RACE_CAR:      return PL_COLOR_32(1.0f, 0.0f,  0.0f, 1.0f);  // red
        case TOP_HAT:       return PL_COLOR_32(0.0f, 0.0f,  1.0f, 1.0f);  // blue
        case THIMBLE:       return PL_COLOR_32(0.0f, 1.0f,  0.0f, 1.0f);  // green
        case BOOT:          return PL_COLOR_32(1.0f, 1.0f,  0.0f, 1.0f);  // yellow
        case BATTLESHIP:    return PL_COLOR_32(0.0f, 1.0f,  1.0f, 1.0f);  // cyan
        case IRON:          return PL_COLOR_32(1.0f, 0.0f,  1.0f, 1.0f);  // magenta
        case CANNON:        return PL_COLOR_32(1.0f, 0.5f,  0.0f, 1.0f);  // orange
        case LANTERN:       return PL_COLOR_32(0.5f, 0.0f,  0.5f, 1.0f);  // purple
        case PURSE:         return PL_COLOR_32(1.0f, 0.75f, 0.8f, 1.0f);  // pink
        case ROCKING_HORSE: return PL_COLOR_32(0.6f, 0.3f,  0.0f, 1.0f);  // brown
        default:            return PL_COLOR_32(1.0f, 1.0f,  1.0f, 1.0f);  // white
    }
}

void
load_texture(plAppData* ptAppData, const plTextureLoadConfig* ptConfig)
{
    plDevice* ptDevice = ptAppData->ptDevice;

    // load image from disk using stb_image
    int iWidth, iHeight, iChannels;
    unsigned char* pImageData = stbi_load(ptConfig->pcFilePath, &iWidth, &iHeight, &iChannels, 4);

    if(!pImageData)
    {
        printf("ERROR: Failed to load %s\n", ptConfig->pcFilePath);
        if(ptConfig->pbOutLoaded)
            *ptConfig->pbOutLoaded = false;
        return;
    }
    printf("Loaded texture: %s (%dx%d)\n", ptConfig->pcFilePath, iWidth, iHeight);

    // create GPU texture
    plTextureDesc tTexDesc = {
        .tDimensions = {(float)iWidth, (float)iHeight, 1.0f},
        .tFormat     = PL_FORMAT_R8G8B8A8_UNORM,
        .uLayers     = 1,
        .uMips       = 1,
        .tType       = PL_TEXTURE_TYPE_2D,
        .tUsage      = PL_TEXTURE_USAGE_SAMPLED,
        .pcDebugName = ptConfig->pcFilePath
    };

    plTexture* ptTexture = NULL;
    *ptConfig->ptOutTexture = gptGfx->create_texture(ptDevice, &tTexDesc, &ptTexture);

    // allocate GPU memory for texture
    *ptConfig->ptOutMemory = gptGfx->allocate_memory(ptDevice, ptTexture->tMemoryRequirements.ulSize, PL_MEMORY_FLAGS_DEVICE_LOCAL, 
            ptTexture->tMemoryRequirements.uMemoryTypeBits, "texture memory");
    gptGfx->bind_texture_to_memory(ptDevice, *ptConfig->ptOutTexture, ptConfig->ptOutMemory);

    // create staging buffer for upload
    size_t szImageSize = iWidth * iHeight * 4;

    plBufferDesc tStagingDesc = {
        .tUsage      = PL_BUFFER_USAGE_STAGING,
        .szByteSize  = szImageSize,
        .pcDebugName = "texture staging"
    };

    plBuffer* ptStaging = NULL;
    plBufferHandle tStagingHandle = gptGfx->create_buffer(ptDevice, &tStagingDesc, &ptStaging);

    plDeviceMemoryAllocation tStagingMem = gptGfx->allocate_memory(ptDevice, ptStaging->tMemoryRequirements.ulSize, 
            PL_MEMORY_FLAGS_HOST_VISIBLE | PL_MEMORY_FLAGS_HOST_COHERENT, ptStaging->tMemoryRequirements.uMemoryTypeBits, "staging memory");
    gptGfx->bind_buffer_to_memory(ptDevice, tStagingHandle, &tStagingMem);

    // copy image data to staging buffer
    memcpy(tStagingMem.pHostMapped, pImageData, szImageSize);
    stbi_image_free(pImageData);

    // upload staging buffer to GPU texture
    plCommandBuffer* ptCmdbuff = gptGfx->request_command_buffer(ptAppData->ptCommandPool, "upload texture");
    gptGfx->begin_command_recording(ptCmdbuff, NULL);
    plBlitEncoder* ptBlit = gptGfx->begin_blit_pass(ptCmdbuff);

    // transition image to transfer destination layout
    gptGfx->pipeline_barrier_blit(ptBlit, 
        PL_PIPELINE_STAGE_VERTEX_SHADER | PL_PIPELINE_STAGE_TRANSFER, 
        PL_ACCESS_SHADER_READ | PL_ACCESS_TRANSFER_READ, 
        PL_PIPELINE_STAGE_TRANSFER, 
        PL_ACCESS_TRANSFER_WRITE);
    
    gptGfx->set_texture_usage(ptBlit, *ptConfig->ptOutTexture, PL_TEXTURE_USAGE_SAMPLED, 0);

    // copy buffer to texture
    plBufferImageCopy tCopy = {
        .szBufferOffset     = 0,
        .uImageWidth        = iWidth,
        .uImageHeight       = iHeight,
        .uImageDepth        = 1,
        .uMipLevel          = 0,
        .uBaseArrayLayer    = 0,
        .uLayerCount        = 1,
        .tCurrentImageUsage = PL_TEXTURE_USAGE_SAMPLED
    };
    gptGfx->copy_buffer_to_texture(ptBlit, tStagingHandle, *ptConfig->ptOutTexture, 1, &tCopy);

    // transition image to shader read layout
    gptGfx->pipeline_barrier_blit(ptBlit, 
        PL_PIPELINE_STAGE_TRANSFER, 
        PL_ACCESS_TRANSFER_WRITE, 
        PL_PIPELINE_STAGE_VERTEX_SHADER | PL_PIPELINE_STAGE_TRANSFER, 
        PL_ACCESS_SHADER_READ | PL_ACCESS_TRANSFER_READ);

    gptGfx->end_blit_pass(ptBlit);
    gptGfx->end_command_recording(ptCmdbuff);
    gptGfx->submit_command_buffer(ptCmdbuff, NULL);
    gptGfx->wait_on_command_buffer(ptCmdbuff);
    gptGfx->return_command_buffer(ptCmdbuff);

    // cleanup staging buffer
    gptGfx->destroy_buffer(ptDevice, tStagingHandle);
    gptGfx->free_memory(ptDevice, &tStagingMem);

    // create bind group for this texture
    plBindGroupDesc tBGDesc = {
        .tLayout     = ptAppData->tTextureBindGroupLayout,
        .ptPool      = ptAppData->tBindGroupPoolTexAndSamp,
        .pcDebugName = "texture bind group"
    };
    *ptConfig->ptOutBindGroup = gptGfx->create_bind_group(ptDevice, &tBGDesc);

    // update bind group with texture and sampler
    plBindGroupUpdateTextureData tTexUpdate = {
        .tTexture      = *ptConfig->ptOutTexture,
        .uSlot         = 0,
        .tType         = PL_TEXTURE_BINDING_TYPE_SAMPLED,
        .tCurrentUsage = PL_TEXTURE_USAGE_SAMPLED
    };
    plBindGroupUpdateSamplerData tSamplerUpdate = {
        .tSampler = ptConfig->tSampler,
        .uSlot    = 1
    };
    plBindGroupUpdateData tUpdateData = {
        .uTextureCount     = 1,
        .atTextureBindings = &tTexUpdate,
        .uSamplerCount     = 1,
        .atSamplerBindings = &tSamplerUpdate
    };

    gptGfx->update_bind_group(ptDevice, *ptConfig->ptOutBindGroup, &tUpdateData);

    if(ptConfig->pbOutLoaded)
        *ptConfig->pbOutLoaded = true;
    printf("Texture uploaded to GPU\n");
}