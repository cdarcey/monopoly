/*
   app.c - Monopoly Game (Minimal Test Version)
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
#include "pl_ui_ext.h"
#include "pl_shader_ext.h"

// monopoly game logic
#include "monopoly.h"
#include "monopoly_init.h"

// libraries
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define SCREEN_WIDTH 1280
#define SCREEN_HEIGHT 720

//-----------------------------------------------------------------------------
// [SECTION] helper macros
//-----------------------------------------------------------------------------

#define PL_COLOR_32(r, g, b, a) ((uint32_t)((uint8_t)((a)*255.0f) << 24 | (uint8_t)((b)*255.0f) << 16 | (uint8_t)((g)*255.0f) << 8 | (uint8_t)((r)*255.0f)))

//-----------------------------------------------------------------------------
// [SECTION] forward declarations
//-----------------------------------------------------------------------------

typedef struct _plAppData plAppData;
typedef struct _plTextureLoadConfig plTextureLoadConfig;

void   handle_keyboard_input(plAppData* ptAppData);
void   load_texture(plAppData* ptAppData, const plTextureLoadConfig* ptConfig);
plMat4 create_orthographic_projection(float fScreenWidth, float fScreenHeight);
void   show_player_status(mGameData* pGameData);
void   draw_player_tokens(plAppData* ptAppData, plRenderEncoder* ptRender);
void   draw_preroll_menu(plAppData* ptAppData);
void   draw_postroll_menu(plAppData* ptAppData);
void   draw_jail_menu(plAppData* ptAppData);
void   draw_notification(plAppData* ptAppData);

//-----------------------------------------------------------------------------
// [SECTION] structs
//-----------------------------------------------------------------------------

typedef struct _plAppData
{
    // window & device
    plWindow*  ptWindow;
    plDevice*  ptDevice;
    plSurface* ptSurface;

    // command infrastructure
    plCommandPool* ptCommandPool;

    // bind groups
    plBindGroupPool*        tBindGroupPoolTexAndSamp;
    plBindGroupLayoutHandle tTextureBindGroupLayout;
    plBindGroupLayoutDesc   tTextureBindGroupLayoutDesc;

    // samplers
    plSamplerHandle tLinearSampler;

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

    // player token drawing
    plDrawList2D*  ptTokenDrawlist;
    plDrawLayer2D* ptTokenLayer;

    // monopoly game state
    mGameData* pGameData;
    mGameFlow  tGameFlow;

    // frame counter for buffer warmup
    uint32_t uFrameCount;

} plAppData;

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
const plUiI*          gptUi          = NULL;
const plShaderI*      gptShader      = NULL;

//-----------------------------------------------------------------------------
// [SECTION] pl_app_load
//-----------------------------------------------------------------------------

PL_EXPORT void*
pl_app_load(plApiRegistryI* ptApiRegistry, plAppData* ptAppData)
{
    // hot reload path
    if(ptAppData)
    {
        gptIO          = pl_get_api_latest(ptApiRegistry, plIOI);
        gptWindows     = pl_get_api_latest(ptApiRegistry, plWindowI);
        gptGfx         = pl_get_api_latest(ptApiRegistry, plGraphicsI);
        gptDraw        = pl_get_api_latest(ptApiRegistry, plDrawI);
        gptDrawBackend = pl_get_api_latest(ptApiRegistry, plDrawBackendI);
        gptUi          = pl_get_api_latest(ptApiRegistry, plUiI);
        gptShader      = pl_get_api_latest(ptApiRegistry, plShaderI);
        return ptAppData;
    }

    // first load
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
    gptUi          = pl_get_api_latest(ptApiRegistry, plUiI);
    gptShader      = pl_get_api_latest(ptApiRegistry, plShaderI);

    // initialize graphics
    plGraphicsInit tGraphicsInit = {
        .uFramesInFlight = 2,
        .tFlags = PL_GRAPHICS_INIT_FLAGS_SWAPCHAIN_ENABLED | PL_GRAPHICS_INIT_FLAGS_VALIDATION_ENABLED
    };
    gptGfx->initialize(&tGraphicsInit);

    // create window
    plWindowDesc tWindowDesc = {
        .pcTitle = "Monopoly Test",
        .iXPos   = 100,
        .iYPos   = 100,
        .uWidth  = SCREEN_WIDTH,
        .uHeight = SCREEN_HEIGHT,
    };
    gptWindows->create(tWindowDesc, &ptAppData->ptWindow);
    gptWindows->show(ptAppData->ptWindow);

    // create surface and device
    plSurface* ptSurface = gptGfx->create_surface(ptAppData->ptWindow);
    ptAppData->ptSurface = ptSurface;
    const plDeviceInit tDeviceInit = {
        .uDeviceIdx = 0,
        .szDynamicBufferBlockSize = 65536,
        .szDynamicDataMaxSize = 65536,
        .ptSurface = ptSurface
    };
    ptAppData->ptDevice = gptGfx->create_device(&tDeviceInit);

    // initialize shader system
    static const plShaderOptions tDefaultShaderOptions = {
        .apcIncludeDirectories = { "../../monopoly/shaders/" },
        .apcDirectories = { "../../monopoly/shaders/" },
        .tFlags = PL_SHADER_FLAGS_NEVER_CACHE
    };
    gptShader->initialize(&tDefaultShaderOptions);

    // initialize draw system (required before draw backend and ui)
    gptDraw->initialize(NULL);
    
    // initialize draw backend (required for ui)
    gptDrawBackend->initialize(ptAppData->ptDevice);

    // create font atlas (will be built after command pool is created)
    plFontAtlas* ptAtlas = gptDraw->create_font_atlas();
    gptDraw->set_font_atlas(ptAtlas);

    // add default font to atlas
    plFont* ptDefaultFont = gptDraw->add_default_font(ptAtlas);

    // create swapchain
    plSwapchainInit tSwapchainInit = {
        .bVSync = true,
        .tSampleCount = PL_SAMPLE_COUNT_1,
        .uWidth = SCREEN_WIDTH,
        .uHeight = SCREEN_HEIGHT
    };
    ptAppData->ptSwapchain = gptGfx->create_swapchain(ptAppData->ptDevice, ptSurface, &tSwapchainInit);

    uint32_t uImageCount = 0;
    ptAppData->atSwapchainImages = gptGfx->get_swapchain_images(ptAppData->ptSwapchain, &uImageCount);
    ptAppData->uSwapchainImageCount = uImageCount;

    // create command pool
    plCommandPoolDesc tPoolDesc = {0};
    ptAppData->ptCommandPool = gptGfx->create_command_pool(ptAppData->ptDevice, &tPoolDesc);

    // build font atlas on gpu (now that command pool exists)
    plCommandBuffer* ptCmdFont = gptGfx->request_command_buffer(ptAppData->ptCommandPool, "font atlas");
    gptDrawBackend->build_font_atlas(ptCmdFont, gptDraw->get_current_font_atlas());
    gptGfx->return_command_buffer(ptCmdFont);
    
    // initialize ui (requires font atlas to be built first)
    gptUi->initialize();
    gptUi->set_default_font(ptDefaultFont);
    gptUi->set_dark_theme();

    // create sampler
    plSamplerDesc tLinearSamplerDesc = {
        .tMagFilter = PL_FILTER_LINEAR,
        .tMinFilter = PL_FILTER_LINEAR,
        .tMipmapMode = PL_MIPMAP_MODE_LINEAR,
        .tUAddressMode = PL_ADDRESS_MODE_CLAMP_TO_EDGE,
        .tVAddressMode = PL_ADDRESS_MODE_CLAMP_TO_EDGE,
        .fMinMip = 0.0f,
        .fMaxMip = 1.0f,
        .pcDebugName = "sampler_linear" 
    };
    ptAppData->tLinearSampler = gptGfx->create_sampler(ptAppData->ptDevice, &tLinearSamplerDesc);

    // create bind group layout
    const plBindGroupLayoutDesc tBindGroupLayoutTexAndSamp = {
        .atTextureBindings = {
            { .uSlot = 0, .tType = PL_TEXTURE_BINDING_TYPE_SAMPLED, .tStages = PL_SHADER_STAGE_FRAGMENT }
        },
        .atSamplerBindings = {
            { .uSlot = 1, .tStages = PL_SHADER_STAGE_FRAGMENT }
        },
        .pcDebugName = "texture + sampler layout"
    };
    ptAppData->tTextureBindGroupLayoutDesc = tBindGroupLayoutTexAndSamp;
    ptAppData->tTextureBindGroupLayout = gptGfx->create_bind_group_layout(ptAppData->ptDevice, &tBindGroupLayoutTexAndSamp);

    // create bind group pool
    const plBindGroupPoolDesc tBindGroupPoolTexAndSampDesc = {
        .szSampledTextureBindings = 100,
        .szSamplerBindings = 100
    };
    ptAppData->tBindGroupPoolTexAndSamp = gptGfx->create_bind_group_pool(ptAppData->ptDevice, &tBindGroupPoolTexAndSampDesc);

    // load board texture
    plTextureLoadConfig tBoardConfig = {
        .pcFilePath = "../../monopoly/assets/monopoly-board.png",
        .tSampler = ptAppData->tLinearSampler,
        .ptOutTexture = &ptAppData->tBoardTexture,
        .ptOutMemory = &ptAppData->tBoardTextureMemory,
        .ptOutBindGroup = &ptAppData->tBoardBindGroup,
        .pbOutLoaded = &ptAppData->bBoardTextureLoaded
    };
    load_texture(ptAppData, &tBoardConfig);

    // create render pass layout
    const plRenderPassLayoutDesc tMainRenderPassLayoutDesc = {
        .atRenderTargets = {
            { .tFormat = PL_FORMAT_R8G8B8A8_UNORM, .tSamples = PL_SAMPLE_COUNT_1 }
        },
        .atSubpasses = {
            { .uRenderTargetCount = 1, .auRenderTargets = {0} }
        },
        .pcDebugName = "main pass layout"
    };
    ptAppData->tMainPassLayout = gptGfx->create_render_pass_layout(ptAppData->ptDevice, &tMainRenderPassLayoutDesc);

    // load shaders
    plShaderModule tVertModule = gptShader->load_glsl("textured_quad.vert", "main", NULL, NULL);
    plShaderModule tFragModule = gptShader->load_glsl("textured_quad.frag", "main", NULL, NULL);

    plVertexBufferLayout tVertexLayout = {
        .uByteStride = 0,
        .atAttributes = {
            { .tFormat = PL_VERTEX_FORMAT_FLOAT2 },
            { .tFormat = PL_VERTEX_FORMAT_FLOAT2 }
        }
    };

    plShaderDesc tShaderDesc = {
        .tVertexShader = tVertModule,
        .tFragmentShader = tFragModule,
        .atVertexBufferLayouts[0] = tVertexLayout,
        .atBindGroupLayouts[0] = ptAppData->tTextureBindGroupLayoutDesc,
        .tRenderPassLayout = ptAppData->tMainPassLayout,
        .pcDebugName = "textured quad shader"
    };
    ptAppData->tTexturedQuadShader = gptGfx->create_shader(ptAppData->ptDevice, &tShaderDesc);

    // create render pass
    plRenderPassDesc tMainPassDesc = {
        .tLayout = ptAppData->tMainPassLayout,
        .tDimensions = {SCREEN_WIDTH, SCREEN_HEIGHT},
        .atColorTargets = {
            {
                .tLoadOp = PL_LOAD_OP_CLEAR,
                .tStoreOp = PL_STORE_OP_STORE,
                .tCurrentUsage = PL_TEXTURE_USAGE_UNSPECIFIED,
                .tNextUsage = PL_TEXTURE_USAGE_PRESENT,
                .tClearColor = {0.1f, 0.1f, 0.1f, 1.0f}
            }
        },
        .ptSwapchain = ptAppData->ptSwapchain,
        .pcDebugName = "main render pass"
    };

    plRenderPassAttachments* atAttachments = malloc(sizeof(plRenderPassAttachments) * uImageCount);
    for(uint32_t i = 0; i < uImageCount; i++)
    {
        atAttachments[i].atViewAttachments[0] = ptAppData->atSwapchainImages[i];
    }

    ptAppData->tRenderPass = gptGfx->create_render_pass(ptAppData->ptDevice, &tMainPassDesc, atAttachments);
    free(atAttachments);

    // create quad geometry
    const float atVertices[] = {
        0.0f,   0.0f,   0.0f, 0.0f,
        700.0f, 0.0f,   1.0f, 0.0f,
        700.0f, 700.0f, 1.0f, 1.0f,
        0.0f,   700.0f, 0.0f, 1.0f
    };

    const plBufferDesc tVertexDesc = {
        .tUsage = PL_BUFFER_USAGE_VERTEX | PL_BUFFER_USAGE_TRANSFER_DESTINATION,
        .szByteSize = sizeof(atVertices),
        .pcDebugName = "quad vertices"
    };
    ptAppData->tQuadVertexBuffer = gptGfx->create_buffer(ptAppData->ptDevice, &tVertexDesc, NULL);
    plBuffer* ptVertexBuffer = gptGfx->get_buffer(ptAppData->ptDevice, ptAppData->tQuadVertexBuffer);

    ptAppData->tQuadVertexMemory = gptGfx->allocate_memory(ptAppData->ptDevice, ptVertexBuffer->tMemoryRequirements.ulSize, 
        PL_MEMORY_FLAGS_DEVICE_LOCAL, ptVertexBuffer->tMemoryRequirements.uMemoryTypeBits, "vertex buffer memory");
    gptGfx->bind_buffer_to_memory(ptAppData->ptDevice, ptAppData->tQuadVertexBuffer, &ptAppData->tQuadVertexMemory);

    uint32_t auIndices[] = { 0, 1, 2, 0, 2, 3 };

    plBufferDesc tIndexDesc = {
        .tUsage = PL_BUFFER_USAGE_INDEX | PL_BUFFER_USAGE_TRANSFER_DESTINATION,
        .szByteSize = sizeof(auIndices),
        .pcDebugName = "quad indices"
    };
    ptAppData->tQuadIndexBuffer = gptGfx->create_buffer(ptAppData->ptDevice, &tIndexDesc, NULL);
    plBuffer* ptIndexBuffer = gptGfx->get_buffer(ptAppData->ptDevice, ptAppData->tQuadIndexBuffer);

    ptAppData->tQuadIndexMemory = gptGfx->allocate_memory(ptAppData->ptDevice, ptIndexBuffer->tMemoryRequirements.ulSize, 
        PL_MEMORY_FLAGS_DEVICE_LOCAL, ptIndexBuffer->tMemoryRequirements.uMemoryTypeBits, "index buffer memory");
    gptGfx->bind_buffer_to_memory(ptAppData->ptDevice, ptAppData->tQuadIndexBuffer, &ptAppData->tQuadIndexMemory);

    // upload geometry
    plBufferDesc tStagingDesc = {
        .tUsage = PL_BUFFER_USAGE_STAGING,
        .szByteSize = 4096,
        .pcDebugName = "staging buffer"
    };
    plBufferHandle tStagingHandle = gptGfx->create_buffer(ptAppData->ptDevice, &tStagingDesc, NULL);
    plBuffer* ptStagingBuffer = gptGfx->get_buffer(ptAppData->ptDevice, tStagingHandle);

    plDeviceMemoryAllocation tStagingMem = gptGfx->allocate_memory(ptAppData->ptDevice, ptStagingBuffer->tMemoryRequirements.ulSize, 
        PL_MEMORY_FLAGS_HOST_VISIBLE | PL_MEMORY_FLAGS_HOST_COHERENT, ptStagingBuffer->tMemoryRequirements.uMemoryTypeBits, "staging memory");
    gptGfx->bind_buffer_to_memory(ptAppData->ptDevice, tStagingHandle, &tStagingMem);

    memcpy(ptStagingBuffer->tMemoryAllocation.pHostMapped, atVertices, sizeof(atVertices));
    memcpy(&ptStagingBuffer->tMemoryAllocation.pHostMapped[1024], auIndices, sizeof(auIndices));

    plCommandBuffer* ptCmd = gptGfx->request_command_buffer(ptAppData->ptCommandPool, "upload geometry");
    gptGfx->begin_command_recording(ptCmd, NULL);

    plBlitEncoder* ptBlit = gptGfx->begin_blit_pass(ptCmd);
    gptGfx->copy_buffer(ptBlit, tStagingHandle, ptAppData->tQuadVertexBuffer, 0, 0, sizeof(atVertices));
    gptGfx->copy_buffer(ptBlit, tStagingHandle, ptAppData->tQuadIndexBuffer, 1024, 0, sizeof(auIndices));
    gptGfx->end_blit_pass(ptBlit);

    gptGfx->end_command_recording(ptCmd);
    gptGfx->submit_command_buffer(ptCmd, NULL);
    gptGfx->wait_on_command_buffer(ptCmd);
    gptGfx->return_command_buffer(ptCmd);

    gptGfx->destroy_buffer(ptAppData->ptDevice, tStagingHandle);

    // initialize monopoly game
    mGameSettings tSettings = {
        .uStartingMoney = 1500,
        .uJailFine = 50,
        .uPlayerCount = 2
    };
    ptAppData->pGameData = m_init_game(tSettings);
    m_init_game_flow(&ptAppData->tGameFlow, ptAppData->pGameData, ptAppData->ptWindow);

    // create persistent drawlist and layer for player tokens
    ptAppData->ptTokenDrawlist = gptDraw->request_2d_drawlist();
    ptAppData->ptTokenLayer = gptDraw->request_2d_layer(ptAppData->ptTokenDrawlist);

    // initialize frame counter for buffer warmup
    ptAppData->uFrameCount = 0;

    printf("=== Monopoly Game Started ===\n");
    printf("2 players, $1500 starting money\n\n");

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

    // return persistent drawing resources 
    if(ptAppData->ptTokenLayer)
        gptDraw->return_2d_layer(ptAppData->ptTokenLayer);
    if(ptAppData->ptTokenDrawlist)
        gptDraw->return_2d_drawlist(ptAppData->ptTokenDrawlist);

    // cleanup font atlas 
    gptDrawBackend->cleanup_font_atlas(gptDraw->get_current_font_atlas());

    // cleanup draw system, backend, and UI
    gptDraw->cleanup();
    gptDrawBackend->cleanup();
    gptUi->cleanup();

    // cleanup textures (NOT swapchain textures)
    if(ptAppData->bBoardTextureLoaded)
    {
        gptGfx->destroy_texture(ptAppData->ptDevice, ptAppData->tBoardTexture);
        gptGfx->destroy_bind_group(ptAppData->ptDevice, ptAppData->tBoardBindGroup);
    }

    // cleanup geometry buffers
    gptGfx->destroy_buffer(ptAppData->ptDevice, ptAppData->tQuadVertexBuffer);
    gptGfx->destroy_buffer(ptAppData->ptDevice, ptAppData->tQuadIndexBuffer);

    // cleanup render shader
    gptGfx->destroy_shader(ptAppData->ptDevice, ptAppData->tTexturedQuadShader);

    // cleanup bind group pool and layout
    gptGfx->cleanup_bind_group_pool(ptAppData->tBindGroupPoolTexAndSamp);
    gptGfx->destroy_bind_group_layout(ptAppData->ptDevice, ptAppData->tTextureBindGroupLayout);

    // cleanup samplers
    gptGfx->destroy_sampler(ptAppData->ptDevice, ptAppData->tLinearSampler);

    // cleanup command pool
    gptGfx->cleanup_command_pool(ptAppData->ptCommandPool);

    // cleanup swapchain (this handles swapchain texture cleanup internally)
    gptGfx->cleanup_swapchain(ptAppData->ptSwapchain);

    // cleanup device 
    gptGfx->cleanup_device(ptAppData->ptDevice);

    // cleanup surface 
    if(ptAppData->ptSurface)
    {
        gptGfx->cleanup_surface(ptAppData->ptSurface);
    }

    // cleanup graphics system
    gptGfx->cleanup();

    // cleanup game data
    if(ptAppData->pGameData)
        m_free_game(ptAppData->pGameData);

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
    // handle window resizing if needed
}

//-----------------------------------------------------------------------------
// [SECTION] pl_app_update
//-----------------------------------------------------------------------------

PL_EXPORT void
pl_app_update(plAppData* ptAppData)
{
    // increment frame counter
    ptAppData->uFrameCount++;

    // process input events and start frame calls
    gptIO->new_frame();
    gptDraw->new_frame();
    gptDrawBackend->new_frame();
    gptUi->new_frame();
    handle_keyboard_input(ptAppData);

    // TODO: (this is not fixing errors) skip ui rendering for first 5 frames to let buffers grow without errors
    if(ptAppData->uFrameCount > 5)
    {
        // show ui windows
        show_player_status(ptAppData->pGameData);
        
        // show phase-specific menus
        if(ptAppData->tGameFlow.pfCurrentPhase == m_phase_pre_roll)
        {
            draw_preroll_menu(ptAppData);
        }
        else if(ptAppData->tGameFlow.pfCurrentPhase == m_phase_post_roll)
        {
            draw_postroll_menu(ptAppData);
        }
        else if(ptAppData->tGameFlow.pfCurrentPhase == m_phase_jail)
        {
            draw_jail_menu(ptAppData);
        }
        
        // show notification popup (on top of everything)
        draw_notification(ptAppData);
    }

    // run game phase
    m_run_current_phase(&ptAppData->tGameFlow, 0.016f);

    // end ui frame
    gptUi->end_frame();

    // begin frame
    gptGfx->begin_frame(ptAppData->ptDevice);

    if(!gptGfx->acquire_swapchain_image(ptAppData->ptSwapchain))
        return;

    plCommandBuffer* ptCmd = gptGfx->request_command_buffer(ptAppData->ptCommandPool, "main");
    const plBeginCommandInfo tBeginInfo = { .uWaitSemaphoreCount = 0 };
    gptGfx->begin_command_recording(ptCmd, &tBeginInfo);

    plRenderEncoder* ptRender = gptGfx->begin_render_pass(ptCmd, ptAppData->tRenderPass, NULL);

    plRenderViewport tViewport = {.fWidth = 1280, .fHeight = 720, .fMaxDepth = 1.0f};
    gptGfx->set_viewport(ptRender, &tViewport);

    plScissor tScissor = {.uWidth = 1280, .uHeight = 720};
    gptGfx->set_scissor_region(ptRender, &tScissor);

    gptGfx->bind_shader(ptRender, ptAppData->tTexturedQuadShader);
    gptGfx->bind_vertex_buffer(ptRender, ptAppData->tQuadVertexBuffer);

    plDynamicDataBlock tBlock = gptGfx->allocate_dynamic_data_block(ptAppData->ptDevice);
    plDynamicBinding tBinding = pl_allocate_dynamic_data(gptGfx, ptAppData->ptDevice, &tBlock);

    plMat4* pMVP = (plMat4*)tBinding.pcData;
    plMat4 m4Projection = create_orthographic_projection((float)SCREEN_WIDTH, (float)SCREEN_HEIGHT);
    plMat4 m4Translate = pl_mat4_translate_xyz(50.0f, 10.0f, 0.0f);
    *pMVP = pl_mul_mat4(&m4Projection, &m4Translate);

    gptGfx->bind_graphics_bind_groups(ptRender, ptAppData->tTexturedQuadShader, 0, 1, &ptAppData->tBoardBindGroup, 1, &tBinding);

    plDrawIndex tDraw = {
        .uIndexCount = 6,
        .tIndexBuffer = ptAppData->tQuadIndexBuffer,
        .uInstanceCount = 1
    };
    gptGfx->draw_indexed(ptRender, 1, &tDraw);

    // draw player tokens on the board
    draw_player_tokens(ptAppData, ptRender);

    // submit ui drawlist
    plDrawList2D* ptDrawlist = gptUi->get_draw_list();
    if(ptDrawlist)
    {
        gptDraw->prepare_2d_drawlist(ptDrawlist);
        gptDrawBackend->submit_2d_drawlist(ptDrawlist, ptRender, (float)SCREEN_WIDTH, (float)SCREEN_HEIGHT, 1);
    }

    gptGfx->end_render_pass(ptRender);
    gptGfx->end_command_recording(ptCmd);
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
    
    // handle property buy menu (1 = buy, 2 = pass)
    if(gptIO->is_key_pressed(PL_KEY_1, false))
        m_set_input_int(&ptAppData->tGameFlow, 1);
    else if(gptIO->is_key_pressed(PL_KEY_2, false))
        m_set_input_int(&ptAppData->tGameFlow, 2);
}

//-----------------------------------------------------------------------------
// [SECTION] helper functions
//-----------------------------------------------------------------------------

void
load_texture(plAppData* ptAppData, const plTextureLoadConfig* ptConfig)
{
    plDevice* ptDevice = ptAppData->ptDevice;

    int iWidth, iHeight, iChannels;
    unsigned char* pImageData = stbi_load(ptConfig->pcFilePath, &iWidth, &iHeight, &iChannels, 4);

    if(!pImageData)
    {
        printf("ERROR: Failed to load %s\n", ptConfig->pcFilePath);
        if(ptConfig->pbOutLoaded)
            *ptConfig->pbOutLoaded = false;
        return;
    }

    plTextureDesc tTexDesc = {
        .tDimensions = {(float)iWidth, (float)iHeight, 1.0f},
        .tFormat = PL_FORMAT_R8G8B8A8_UNORM,
        .uLayers = 1,
        .uMips = 1,
        .tType = PL_TEXTURE_TYPE_2D,
        .tUsage = PL_TEXTURE_USAGE_SAMPLED,
        .pcDebugName = ptConfig->pcFilePath
    };

    plTexture* ptTexture = NULL;
    *ptConfig->ptOutTexture = gptGfx->create_texture(ptDevice, &tTexDesc, &ptTexture);
    *ptConfig->ptOutMemory = gptGfx->allocate_memory(ptDevice, ptTexture->tMemoryRequirements.ulSize, PL_MEMORY_FLAGS_DEVICE_LOCAL, 
        ptTexture->tMemoryRequirements.uMemoryTypeBits, "texture memory");
    gptGfx->bind_texture_to_memory(ptDevice, *ptConfig->ptOutTexture, ptConfig->ptOutMemory);

    size_t szImageSize = iWidth * iHeight * 4;
    plBufferDesc tStagingDesc = {
        .tUsage = PL_BUFFER_USAGE_STAGING,
        .szByteSize = szImageSize,
        .pcDebugName = "texture staging"
    };

    plBuffer* ptStaging = NULL;
    plBufferHandle tStagingHandle = gptGfx->create_buffer(ptDevice, &tStagingDesc, &ptStaging);
    plDeviceMemoryAllocation tStagingMem = gptGfx->allocate_memory(ptDevice, ptStaging->tMemoryRequirements.ulSize, 
        PL_MEMORY_FLAGS_HOST_VISIBLE | PL_MEMORY_FLAGS_HOST_COHERENT, ptStaging->tMemoryRequirements.uMemoryTypeBits, "staging memory");
    gptGfx->bind_buffer_to_memory(ptDevice, tStagingHandle, &tStagingMem);

    memcpy(tStagingMem.pHostMapped, pImageData, szImageSize);
    stbi_image_free(pImageData);

    plCommandBuffer* ptCmdbuff = gptGfx->request_command_buffer(ptAppData->ptCommandPool, "upload texture");
    gptGfx->begin_command_recording(ptCmdbuff, NULL);
    plBlitEncoder* ptBlit = gptGfx->begin_blit_pass(ptCmdbuff);

    gptGfx->pipeline_barrier_blit(ptBlit, 
        PL_PIPELINE_STAGE_VERTEX_SHADER | PL_PIPELINE_STAGE_TRANSFER, 
        PL_ACCESS_SHADER_READ | PL_ACCESS_TRANSFER_READ, 
        PL_PIPELINE_STAGE_TRANSFER, 
        PL_ACCESS_TRANSFER_WRITE);

    gptGfx->set_texture_usage(ptBlit, *ptConfig->ptOutTexture, PL_TEXTURE_USAGE_SAMPLED, 0);

    plBufferImageCopy tCopy = {
        .szBufferOffset = 0,
        .uImageWidth = iWidth,
        .uImageHeight = iHeight,
        .uImageDepth = 1,
        .uMipLevel = 0,
        .uBaseArrayLayer = 0,
        .uLayerCount = 1,
        .tCurrentImageUsage = PL_TEXTURE_USAGE_SAMPLED
    };
    gptGfx->copy_buffer_to_texture(ptBlit, tStagingHandle, *ptConfig->ptOutTexture, 1, &tCopy);

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

    gptGfx->destroy_buffer(ptDevice, tStagingHandle);

    plBindGroupDesc tBGDesc = {
        .tLayout = ptAppData->tTextureBindGroupLayout,
        .ptPool = ptAppData->tBindGroupPoolTexAndSamp,
        .pcDebugName = "texture bind group"
    };
    *ptConfig->ptOutBindGroup = gptGfx->create_bind_group(ptDevice, &tBGDesc);

    plBindGroupUpdateTextureData tTexUpdate = {
        .tTexture = *ptConfig->ptOutTexture,
        .uSlot = 0,
        .tType = PL_TEXTURE_BINDING_TYPE_SAMPLED,
        .tCurrentUsage = PL_TEXTURE_USAGE_SAMPLED
    };
    plBindGroupUpdateSamplerData tSamplerUpdate = {
        .tSampler = ptConfig->tSampler,
        .uSlot = 1
    };
    plBindGroupUpdateData tUpdateData = {
        .uTextureCount = 1,
        .atTextureBindings = &tTexUpdate,
        .uSamplerCount = 1,
        .atSamplerBindings = &tSamplerUpdate
    };

    gptGfx->update_bind_group(ptDevice, *ptConfig->ptOutBindGroup, &tUpdateData);

    if(ptConfig->pbOutLoaded)
        *ptConfig->pbOutLoaded = true;
}

plMat4 
create_orthographic_projection(float fScreenWidth, float fScreenHeight)
{
    plMat4 result = {0};

    // maps (0, fScreenWidth) to (-1, 1) for x
    result.col[0].x = 2.0f / fScreenWidth;
    result.col[3].x = -1.0f;

    // maps (0, fScreenHeight) to (1, -1) for y (flipped)
    result.col[1].y = -2.0f / fScreenHeight;
    result.col[3].y = 1.0f;

    // z and w
    result.col[2].z = -1.0f;
    result.col[3].w = 1.0f;

    return result;
}

void
draw_player_tokens(plAppData* ptAppData, plRenderEncoder* ptRender)
{
    // board offset and size (matching the textured quad)
    const float fBoardX = 50.0f;
    const float fBoardY = 10.0f;
    const float fBoardSize = 700.0f;
    
    // TODO: calculate instead of hard code???
    static const plVec2 atBoardPositions[40] = {
        // bottom row (GO to Jail visiting)
        {0.90f, 0.90f},  // 0 - GO
        {0.82f, 0.90f},  // 1
        {0.74f, 0.90f},  // 2
        {0.66f, 0.90f},  // 3
        {0.58f, 0.90f},  // 4
        {0.50f, 0.90f},  // 5
        {0.42f, 0.90f},  // 6
        {0.34f, 0.90f},  // 7
        {0.26f, 0.90f},  // 8
        {0.18f, 0.90f},  // 9
        {0.10f, 0.90f},  // 10 - Jail (just visiting)
        
        // left column (jail to free parking)
        {0.10f, 0.82f},  // 11
        {0.10f, 0.74f},  // 12
        {0.10f, 0.66f},  // 13
        {0.10f, 0.58f},  // 14
        {0.10f, 0.50f},  // 15
        {0.10f, 0.42f},  // 16
        {0.10f, 0.34f},  // 17
        {0.10f, 0.26f},  // 18
        {0.10f, 0.18f},  // 19
        {0.10f, 0.10f},  // 20 - Free Parking
        
        // top row (free parking to go to jail)
        {0.18f, 0.10f},  // 21
        {0.26f, 0.10f},  // 22
        {0.34f, 0.10f},  // 23
        {0.42f, 0.10f},  // 24
        {0.50f, 0.10f},  // 25
        {0.58f, 0.10f},  // 26
        {0.66f, 0.10f},  // 27
        {0.74f, 0.10f},  // 28
        {0.82f, 0.10f},  // 29
        {0.90f, 0.10f},  // 30 - Go To Jail
        
        // right column (go to jail to go)
        {0.90f, 0.18f},  // 31
        {0.90f, 0.26f},  // 32
        {0.90f, 0.34f},  // 33
        {0.90f, 0.42f},  // 34
        {0.90f, 0.50f},  // 35
        {0.90f, 0.58f},  // 36
        {0.90f, 0.66f},  // 37
        {0.90f, 0.74f},  // 38
        {0.90f, 0.82f}   // 39
    };
    
    // player colors
    const plVec4 atPlayerColors[6] = {
        {1.0f, 0.0f, 0.0f, 1.0f},  // red
        {0.0f, 0.0f, 1.0f, 1.0f},  // blue
        {0.0f, 1.0f, 0.0f, 1.0f},  // green
        {1.0f, 1.0f, 0.0f, 1.0f},  // yellow
        {1.0f, 0.0f, 1.0f, 1.0f},  // magenta
        {0.0f, 1.0f, 1.0f, 1.0f}   // cyan
    };
    
    // use persistent drawlist and layer
    plDrawList2D* ptDrawlist = ptAppData->ptTokenDrawlist;
    plDrawLayer2D* ptLayer = ptAppData->ptTokenLayer;
    
    // draw each active player
    for(uint8_t i = 0; i < ptAppData->pGameData->uPlayerCount; i++)
    {
        mPlayer* pPlayer = &ptAppData->pGameData->amPlayers[i];
        if(pPlayer->bIsBankrupt)
            continue;
        
        // get board position
        plVec2 tNormPos = atBoardPositions[pPlayer->uPosition];
        
        // convert to screen space
        float fX = fBoardX + (tNormPos.x * fBoardSize);
        float fY = fBoardY + (tNormPos.y * fBoardSize);
        
        // offset multiple players on same space
        float fOffset = (float)i * 15.0f;
        fX += fOffset;
        
        // draw circle for player token
        plVec2 tCenter = {fX, fY};
        uint32_t uColor = PL_COLOR_32(atPlayerColors[i].x, atPlayerColors[i].y, atPlayerColors[i].z, atPlayerColors[i].w);
        
        plDrawSolidOptions tOptions = {.uColor = uColor};
        gptDraw->add_circle_filled(ptLayer, tCenter, 10.0f, 16, tOptions);
    }
    
    // submit and prepare the layer
    gptDraw->submit_2d_layer(ptLayer);
    gptDraw->prepare_2d_drawlist(ptDrawlist);
    
    // submit to backend for rendering
    gptDrawBackend->submit_2d_drawlist(ptDrawlist, ptRender, (float)SCREEN_WIDTH, (float)SCREEN_HEIGHT, 1);
}

void
draw_preroll_menu(plAppData* ptAppData)
{
    mGameData* pGame = ptAppData->pGameData;
    
    // only show if menu flag is set
    if(!pGame->bShowPrerollMenu)
        return;
    
    // position menu in top right
    gptUi->set_next_window_pos((plVec2){880.0f, 20.0f}, PL_UI_COND_ALWAYS);
    gptUi->set_next_window_size((plVec2){380.0f, 200.0f}, PL_UI_COND_ALWAYS);
    
    char acWindowTitle[64];
    snprintf(acWindowTitle, sizeof(acWindowTitle), "Player %d's Turn", pGame->uCurrentPlayerIndex + 1);
    
    if(!gptUi->begin_window(acWindowTitle, NULL, PL_UI_WINDOW_FLAGS_NO_RESIZE | PL_UI_WINDOW_FLAGS_NO_COLLAPSE | PL_UI_WINDOW_FLAGS_NO_MOVE | PL_UI_WINDOW_FLAGS_NO_SCROLLBAR))
        return;
    
    gptUi->layout_static(0.0f, 360, 1);
    gptUi->text("What would you like to do?");
    
    gptUi->vertical_spacing();
    
    // all buttons same size - 45px height, 360px width
    gptUi->layout_static(45.0f, 360, 1);
    
    if(gptUi->button("Manage Properties"))
    {
        m_set_input_int(&ptAppData->tGameFlow, 1);
    }
    
    if(gptUi->button("Propose Trade"))
    {
        m_set_input_int(&ptAppData->tGameFlow, 2);
    }
    
    if(gptUi->button("ROLL DICE"))
    {
        m_set_input_int(&ptAppData->tGameFlow, 3);
    }
    
    gptUi->end_window();
}

void
draw_postroll_menu(plAppData* ptAppData)
{
    mGameData* pGame = ptAppData->pGameData;
    mPlayer* pPlayer = &pGame->amPlayers[pGame->uCurrentPlayerIndex];
    
    // only show if waiting for input on property decision
    if(!m_is_waiting_input(&ptAppData->tGameFlow))
        return;
    
    // get property info from game state
    uint8_t uPropIdx = m_get_property_at_position(pGame, pPlayer->uPosition);
    if(uPropIdx == BANK_PLAYER_INDEX)
        return; // not on a property
    
    mProperty* pProp = &pGame->amProperties[uPropIdx];
    
    // only show menu if property is unowned
    if(pProp->uOwnerIndex != BANK_PLAYER_INDEX)
        return;
    
    // position menu in top right
    gptUi->set_next_window_pos((plVec2){880.0f, 20.0f}, PL_UI_COND_ALWAYS);
    gptUi->set_next_window_size((plVec2){380.0f, 220.0f}, PL_UI_COND_ALWAYS);
    
    char acWindowTitle[64];
    snprintf(acWindowTitle, sizeof(acWindowTitle), "Property Available");
    
    if(!gptUi->begin_window(acWindowTitle, NULL, PL_UI_WINDOW_FLAGS_NO_RESIZE | PL_UI_WINDOW_FLAGS_NO_COLLAPSE | PL_UI_WINDOW_FLAGS_NO_MOVE | PL_UI_WINDOW_FLAGS_NO_SCROLLBAR))
        return;
    
    gptUi->layout_static(0.0f, 360, 1);
    gptUi->text("%s", pProp->cName);
    
    gptUi->vertical_spacing();
    
    gptUi->layout_static(0.0f, 360, 1);
    gptUi->text("Price: $%d", pProp->uPrice);
    
    // show if player can afford it
    bool bCanAfford = m_can_afford(pPlayer, pProp->uPrice);
    if(bCanAfford)
    {
        gptUi->text("You have: $%d", pPlayer->uMoney);
    }
    else
    {
        gptUi->color_text((plVec4){1.0f, 0.3f, 0.3f, 1.0f}, "You have: $%d (Cannot afford!)", pPlayer->uMoney);
    }
    
    gptUi->vertical_spacing();
    
    // buttons - 45px height
    gptUi->layout_static(45.0f, 360, 1);
    
    if(bCanAfford && gptUi->button("Buy Property"))
    {
        m_set_input_int(&ptAppData->tGameFlow, 1);
    }
    
    if(gptUi->button("Pass (to Auction)"))
    {
        m_set_input_int(&ptAppData->tGameFlow, 2);
    }
    
    gptUi->end_window();
}

void
draw_jail_menu(plAppData* ptAppData)
{
    mGameData* pGame = ptAppData->pGameData;
    mPlayer* pPlayer = &pGame->amPlayers[pGame->uCurrentPlayerIndex];
    
    // only show if in jail and waiting for input
    if(pPlayer->uJailTurns == 0 || !m_is_waiting_input(&ptAppData->tGameFlow))
        return;
    
    // position menu in top right
    gptUi->set_next_window_pos((plVec2){880.0f, 20.0f}, PL_UI_COND_ALWAYS);
    gptUi->set_next_window_size((plVec2){380.0f, 260.0f}, PL_UI_COND_ALWAYS);
    
    char acWindowTitle[64];
    snprintf(acWindowTitle, sizeof(acWindowTitle), "In Jail (Attempt %d/3)", pPlayer->uJailTurns);
    
    if(!gptUi->begin_window(acWindowTitle, NULL, PL_UI_WINDOW_FLAGS_NO_RESIZE | PL_UI_WINDOW_FLAGS_NO_COLLAPSE | PL_UI_WINDOW_FLAGS_NO_MOVE | PL_UI_WINDOW_FLAGS_NO_SCROLLBAR))
        return;
    
    gptUi->layout_static(0.0f, 360, 1);
    gptUi->text("Choose an option to get out:");
    
    gptUi->vertical_spacing();
    
    // buttons - 45px height
    gptUi->layout_static(45.0f, 360, 1);
    
    // pay fine button
    bool bCanAffordFine = pPlayer->uMoney >= pGame->uJailFine;
    if(bCanAffordFine && gptUi->button("Pay $50 Fine"))
    {
        m_set_input_int(&ptAppData->tGameFlow, 1);
    }
    else if(!bCanAffordFine)
    {
        gptUi->color_text((plVec4){0.5f, 0.5f, 0.5f, 1.0f}, "Pay $50 Fine (Can't afford)");
    }
    
    // use card button
    if(pPlayer->bHasJailFreeCard && gptUi->button("Use Get Out of Jail Free Card"))
    {
        m_set_input_int(&ptAppData->tGameFlow, 2);
    }
    else if(!pPlayer->bHasJailFreeCard)
    {
        gptUi->color_text((plVec4){0.5f, 0.5f, 0.5f, 1.0f}, "Use Card (Don't have one)");
    }
    
    // roll for doubles button
    if(gptUi->button("Roll for Doubles"))
    {
        m_set_input_int(&ptAppData->tGameFlow, 3);
    }
    
    gptUi->end_window();
}

void
draw_notification(plAppData* ptAppData)
{
    mGameData* pGame = ptAppData->pGameData;
    
    // only show if notification flag is set
    if(!pGame->bShowNotification)
        return;
    
    // tick down timer (assuming ~60fps, so ~0.016s per frame)
    pGame->fNotificationTimer -= 0.016f;
    if(pGame->fNotificationTimer <= 0.0f)
    {
        m_clear_notification(pGame);
        return;
    }
    
    // banner at top center
    gptUi->set_next_window_pos((plVec2){300.0f, 20.0f}, PL_UI_COND_ALWAYS);
    gptUi->set_next_window_size((plVec2){680.0f, 80.0f}, PL_UI_COND_ALWAYS);
    
    if(!gptUi->begin_window("##notification", NULL, PL_UI_WINDOW_FLAGS_NO_RESIZE | PL_UI_WINDOW_FLAGS_NO_COLLAPSE | PL_UI_WINDOW_FLAGS_NO_MOVE | PL_UI_WINDOW_FLAGS_NO_SCROLLBAR | PL_UI_WINDOW_FLAGS_NO_TITLE_BAR))
        return;
    
    gptUi->layout_static(0.0f, 660, 1);
    gptUi->text("%s", pGame->acNotification);
    
    gptUi->end_window();
}

void 
show_player_status(mGameData* pGameData)
{
    // render player status ui
    gptUi->set_next_window_pos((plVec2){880.0f, 400.0f}, PL_UI_COND_ALWAYS);
    gptUi->set_next_window_size((plVec2){350.0f, 300.0f}, PL_UI_COND_ALWAYS);

    // use the actual current player from game state
    uint8_t uCurrentPlayer = pGameData->uCurrentPlayerIndex;
    mPlayer* pPlayer = &pGameData->amPlayers[uCurrentPlayer];

    char acWindowTitle[64];
    snprintf(acWindowTitle, sizeof(acWindowTitle), "Player %d Status", uCurrentPlayer + 1);

    if(!gptUi->begin_window(acWindowTitle, NULL, PL_UI_WINDOW_FLAGS_NO_RESIZE | PL_UI_WINDOW_FLAGS_NO_COLLAPSE | PL_UI_WINDOW_FLAGS_NO_MOVE))
        return;

    // show money
    gptUi->layout_static(0.0f, 320, 1);
    gptUi->text("Cash: $%d", pPlayer->uMoney);

    gptUi->vertical_spacing();

    // show position
    gptUi->layout_static(0.0f, 320, 1);
    gptUi->text("Position: %d", pPlayer->uPosition);

    gptUi->vertical_spacing();

    gptUi->separator();
    gptUi->vertical_spacing();

    // properties owned (collapsible)
    if(gptUi->begin_collapsing_header("Properties Owned", 0))
    {
        gptUi->layout_static(0.0f, 310, 1);
        bool bHasProperties = false;
        for(uint8_t i = 0; i < PROPERTY_ARRAY_SIZE; i++)
        {
            uint8_t uPropIdx = pPlayer->auPropertiesOwned[i];
            if(uPropIdx == BANK_PLAYER_INDEX)
                break;

            bHasProperties = true;
            mProperty* pProp = &pGameData->amProperties[uPropIdx];
            gptUi->text("  %s", pProp->cName);
        }
        if(!bHasProperties)
        {
            gptUi->text("  None");
        }
        gptUi->end_collapsing_header();
    }
    gptUi->end_window();
}
