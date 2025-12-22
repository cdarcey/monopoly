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

// rendering functions
void render_game_ui(plAppData* ptAppData);


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
    //-------------------------------------------------------------------------
    // pilot light core
    //-------------------------------------------------------------------------
    plWindow* ptWindow;
    plDevice* ptDevice;

    // legacy 2d drawing (will eventually remove)
    plDrawList2D*  ptDrawlist;
    plDrawLayer2D* ptLayer;

    //-------------------------------------------------------------------------
    // command infrastructure
    //-------------------------------------------------------------------------
    plCommandPool*   ptCommandPool;
    plCommandBuffer* ptMainCommandBuffer; // TODO: do we need this here?

    //-------------------------------------------------------------------------
    // bind groups
    //-------------------------------------------------------------------------
    plBindGroupPool*        tBindGroupPoolTexAndSamp;
    plBindGroupLayoutHandle tTextureBindGroupLayout;     // handle
    plBindGroupLayoutDesc   tTextureBindGroupLayoutDesc; // description for shaders

    //-------------------------------------------------------------------------
    // samplers
    //-------------------------------------------------------------------------
    plSamplerHandle tLinearSampler;  // smooth filtering
    plSamplerHandle tNearestSampler; // sharp/pixelated filtering

    //-------------------------------------------------------------------------
    // swapchain
    //-------------------------------------------------------------------------
    plSwapchain*     ptSwapchain;
    plTextureHandle* atSwapchainImages;
    uint32_t         uSwapchainImageCount;

    //-------------------------------------------------------------------------
    // render passes
    //-------------------------------------------------------------------------
    plRenderPassLayoutHandle tMainPassLayout; // layout
    plRenderPassHandle       tRenderPass;  // instances (one per swapchain image)

    //-------------------------------------------------------------------------
    // shaders
    //-------------------------------------------------------------------------
    plShaderHandle tTexturedQuadShader;

    //-------------------------------------------------------------------------
    // geometry (shared unit quad for all textured quads)
    //-------------------------------------------------------------------------
    plBufferHandle           tQuadVertexBuffer;
    plBufferHandle           tQuadIndexBuffer;
    plDeviceMemoryAllocation tQuadVertexMemory;
    plDeviceMemoryAllocation tQuadIndexMemory;

    //-------------------------------------------------------------------------
    // textures
    //-------------------------------------------------------------------------

    // board
    plTextureHandle          tBoardTexture;
    plDeviceMemoryAllocation tBoardTextureMemory;
    plBindGroupHandle        tBoardBindGroup;
    bool                     bBoardTextureLoaded;

    // property cards (28 total)
    plTextureHandle          atPropertyTextures[28];
    plDeviceMemoryAllocation atPropertyMemory[28];
    plBindGroupHandle        atPropertyBindGroups[28];

    //-------------------------------------------------------------------------
    // monopoly game state
    //-------------------------------------------------------------------------
    mGameData* pGameData;
    mGameFlow  tGameFlow;

    //-------------------------------------------------------------------------
    // board layout
    //-------------------------------------------------------------------------
    float fBoardX;
    float fBoardY;
    float fBoardSize;
    float fSquareSize;

    //-------------------------------------------------------------------------
    // ui state
    //-------------------------------------------------------------------------
    bool          bShowPropertyPopup;
    mPropertyName ePropertyToShow;

    //-------------------------------------------------------------------------
    // viewport and scissor 
    //-------------------------------------------------------------------------
    plRenderViewport tViewport;
    plScissor        tScissor;

    //-------------------------------------------------------------------------
    // sync objects
    //-------------------------------------------------------------------------
    plTimelineSemaphore* ptSemaphore;

} plAppData;

// config struct
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
    // ++++++++++++++++++++++++++++++++++++++ APT loading ++++++++++++++++++++++++++++++++++++++ //
    // hot reload path
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

    // first load path
    ptAppData = malloc(sizeof(plAppData));
    memset(ptAppData, 0, sizeof(plAppData));

    // get extension registry
    const plExtensionRegistryI* ptExtensionRegistry = pl_get_api_latest(ptApiRegistry, plExtensionRegistryI);

    // load extensions
    ptExtensionRegistry->load("pl_unity_ext", NULL, NULL, true);
    ptExtensionRegistry->load("pl_platform_ext", NULL, NULL, false);

    // get APIs
    gptIO          = pl_get_api_latest(ptApiRegistry, plIOI);
    gptWindows     = pl_get_api_latest(ptApiRegistry, plWindowI);
    gptGfx         = pl_get_api_latest(ptApiRegistry, plGraphicsI);
    gptDraw        = pl_get_api_latest(ptApiRegistry, plDrawI);
    gptDrawBackend = pl_get_api_latest(ptApiRegistry, plDrawBackendI);
    gptShader      = pl_get_api_latest(ptApiRegistry, plShaderI);


    // ++++++++++++++++++++++++++++++++++++++ initialize ++++++++++++++++++++++++++++++++++++++ //

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

    // surface and device
    plSurface* ptSurface = gptGfx->create_surface(ptAppData->ptWindow);
    //     gptGfx->cleanup_surface() TODO: does this need to be called and if so when?

    const plDeviceInit tDeviceInit = {
        .uDeviceIdx               = 0,  // First GPU
        .szDynamicBufferBlockSize = 4194304,  // 4 MB
        .szDynamicDataMaxSize     = 4194304,
        .ptSurface                = ptSurface  // TODO: 
    };
    ptAppData->ptDevice = gptGfx->create_device(&tDeviceInit);

    // TODO: having trouble with relative paths currenly putting shaders in pilotlight out folder
    // init shaders
    static const plShaderOptions tDefaultShaderOptions = {
        .apcIncludeDirectories = {
            "C:/dev/monopoly/shaders/"
        },
        .apcDirectories = {
            "C:/dev/monopoly/shaders/"
        },
        .tFlags = PL_SHADER_FLAGS_AUTO_OUTPUT | PL_SHADER_FLAGS_NEVER_CACHE
    };
    gptShader->initialize(&tDefaultShaderOptions);

    // initialize draw list TODO: for ui 
    // ptAppData->ptDrawlist = gptDraw->request_2d_drawlist();
    // ptAppData->ptLayer    = gptDraw->request_2d_layer(ptAppData->ptDrawlist);


    // ++++++++++++++++++++++++++++++++++++++ swapchain ++++++++++++++++++++++++++++++++++++++ //

    plSwapchainInit tSwapchainInit = {
        .bVSync       = true,
        .tSampleCount = PL_SAMPLE_COUNT_1,
        .uWidth       = 1280,
        .uHeight      = 720
    };
    ptAppData->ptSwapchain = gptGfx->create_swapchain(ptAppData->ptDevice, ptSurface, &tSwapchainInit);

    
    uint32_t uImageCount = 0;
    ptAppData->atSwapchainImages = gptGfx->get_swapchain_images(ptAppData->ptSwapchain, &uImageCount);
    ptAppData->uSwapchainImageCount = uImageCount;

    // ++++++++++++++++++++++++++++++++++++++ command pool ++++++++++++++++++++++++++++++++++++++ //

    plCommandPoolDesc tPoolDesc = {0};
    ptAppData->ptCommandPool = gptGfx->create_command_pool(ptAppData->ptDevice, &tPoolDesc);

    // ++++++++++++++++++++++++++++++++++++++ samplers ++++++++++++++++++++++++++++++++++++++ //

    plSamplerDesc tLinearSamplerDesc = {
        .tMagFilter    = PL_FILTER_LINEAR,
        .tMinFilter    = PL_FILTER_LINEAR,
        .tMipmapMode   = PL_MIPMAP_MODE_LINEAR,
        .tUAddressMode = PL_ADDRESS_MODE_CLAMP_TO_EDGE,
        .tVAddressMode = PL_ADDRESS_MODE_CLAMP_TO_EDGE,
        .fMinMip       = 0.0f,
        .fMaxMip       = 1.0f,
        .pcDebugName   = "sampler_linear_clamp" 
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
        .pcDebugName   = "sampler_nearest_clamp"
    };
    ptAppData->tNearestSampler = gptGfx->create_sampler(ptAppData->ptDevice, &tNearSamplerDesc);

    // ++++++++++++++++++++++++++++++++++++++ bind group layouts/pools ++++++++++++++++++++++++++++++++++++++ //

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

    const plBindGroupPoolDesc tBindGroupPoolTexAndSampDesc = {
        .szSampledTextureBindings = 100,
        .szSamplerBindings        = 100
    };
    ptAppData->tBindGroupPoolTexAndSamp = gptGfx->create_bind_group_pool(ptAppData->ptDevice, &tBindGroupPoolTexAndSampDesc);

    // ++++++++++++++++++++++++++++++++++++++ load texture ++++++++++++++++++++++++++++++++++++++ //

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

    // ++++++++++++++++++++++++++++++++++++++ renderpass layout ++++++++++++++++++++++++++++++++++++++ //

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

    // ++++++++++++++++++++++++++++++++++++++ shaders ++++++++++++++++++++++++++++++++++++++ //

    // load vertex shader 
    plShaderModule tVertModule = gptShader->load_glsl(
        "textured_quad.vert",  // filename (looks in apcDirectories)
        "main",                 // entry function
        NULL,                   // file path (NULL = search directories)
        NULL                    // options (NULL = use default from initialize)
    );

    // load fragment shader
    plShaderModule tFragModule = gptShader->load_glsl(
        "textured_quad.frag",
        "main",
        NULL,
        NULL
    );

    // vertex layout
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

    // ++++++++++++++++++++++++++++++++++++++ renderpass instances ++++++++++++++++++++++++++++++++++++++ //

    // create render pass with swapchain images
    plRenderPassDesc tMainPassDesc = {
        .tLayout        = ptAppData->tMainPassLayout,
        .tDimensions    = {1280, 720},
        .atColorTargets = {
            {
                .tLoadOp       = PL_LOAD_OP_CLEAR,
                .tStoreOp      = PL_STORE_OP_STORE,
                .tCurrentUsage = PL_TEXTURE_USAGE_COLOR_ATTACHMENT,
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

    // createrender pass with attachments
    plRenderPassHandle tRenderPass = gptGfx->create_render_pass(ptAppData->ptDevice, &tMainPassDesc, atAttachments);

    free(atAttachments);

    // store handle 
    ptAppData->tRenderPass = tRenderPass;

    // ++++++++++++++++++++++++++++++++++++++ quad geometry ++++++++++++++++++++++++++++++++++++++ //

    plVec4 atVertices[] = {
        {-1.0f, -1.0f, 0.0f, 0.0f},  // bottom-left
        { 1.0f, -1.0f, 1.0f, 0.0f},  // bottom-right
        { 1.0f,  1.0f, 1.0f, 1.0f},  // top-right
        {-1.0f,  1.0f, 0.0f, 1.0f}   // top-left
    };
    uint16_t uIndices[] = {
        0, 1, 2,  // first triangle
        0, 2, 3   // second triangle
    };

    // create vertex buffer
    const plBufferDesc tVertexDesc = {
        .tUsage      = PL_BUFFER_USAGE_VERTEX,
        .szByteSize  = sizeof(atVertices),
        .pcDebugName = "quad vertices"
    };
    plBuffer* ptVertexBuffer = NULL;
    ptAppData->tQuadVertexBuffer = gptGfx->create_buffer(ptAppData->ptDevice, &tVertexDesc, &ptVertexBuffer);

    // allocate vertex memory
    ptAppData->tQuadVertexMemory = gptGfx->allocate_memory(ptAppData->ptDevice, ptVertexBuffer->tMemoryRequirements.ulSize, 
            PL_MEMORY_FLAGS_DEVICE_LOCAL, ptVertexBuffer->tMemoryRequirements.uMemoryTypeBits, "quad vertex memory");
    gptGfx->bind_buffer_to_memory(ptAppData->ptDevice, ptAppData->tQuadVertexBuffer, &ptAppData->tQuadVertexMemory);

    // create index buffer
    plBufferDesc tIndexDesc = {
        .tUsage      = PL_BUFFER_USAGE_INDEX,
        .szByteSize  = sizeof(uIndices),
        .pcDebugName = "quad indices"
    };
    plBuffer* ptIndexBuffer = NULL;
    ptAppData->tQuadIndexBuffer = gptGfx->create_buffer(ptAppData->ptDevice, &tIndexDesc, &ptIndexBuffer);

    // allocate index memory 
    ptAppData->tQuadIndexMemory = gptGfx->allocate_memory(ptAppData->ptDevice, ptIndexBuffer->tMemoryRequirements.ulSize, 
            PL_MEMORY_FLAGS_DEVICE_LOCAL, ptIndexBuffer->tMemoryRequirements.uMemoryTypeBits, "quad index memory");
    gptGfx->bind_buffer_to_memory(ptAppData->ptDevice, ptAppData->tQuadIndexBuffer, &ptAppData->tQuadIndexMemory);

    // upload data using staging buffer
    plBufferDesc tStagingDesc = {
        .tUsage      = PL_BUFFER_USAGE_STAGING,
        .szByteSize  = sizeof(atVertices) + sizeof(uIndices),
        .pcDebugName = "staging"
    };
    plBuffer* ptStaging = NULL;
    plBufferHandle tStagingHandle = gptGfx->create_buffer(ptAppData->ptDevice, &tStagingDesc, &ptStaging);

    plDeviceMemoryAllocation tStagingMem = gptGfx->allocate_memory(ptAppData->ptDevice, ptStaging->tMemoryRequirements.ulSize, 
            PL_MEMORY_FLAGS_HOST_VISIBLE | PL_MEMORY_FLAGS_HOST_COHERENT, ptStaging->tMemoryRequirements.uMemoryTypeBits, "staging memory");
    gptGfx->bind_buffer_to_memory(ptAppData->ptDevice, tStagingHandle, &tStagingMem);

    // copy to staging
    memcpy(tStagingMem.pHostMapped, atVertices, sizeof(atVertices));
    memcpy(tStagingMem.pHostMapped + sizeof(atVertices), uIndices, sizeof(uIndices));

    // upload to GPU
    plCommandBuffer* ptCmd = gptGfx->request_command_buffer(ptAppData->ptCommandPool, "upload geometry");
    gptGfx->begin_command_recording(ptCmd, NULL);

    plBlitEncoder* ptBlit = gptGfx->begin_blit_pass(ptCmd);
    gptGfx->copy_buffer(ptBlit, tStagingHandle, ptAppData->tQuadVertexBuffer, 0, 0, sizeof(atVertices));
    gptGfx->copy_buffer(ptBlit, tStagingHandle, ptAppData->tQuadIndexBuffer, sizeof(atVertices), 0, sizeof(uIndices));
    gptGfx->end_blit_pass(ptBlit);

    gptGfx->end_command_recording(ptCmd);
    gptGfx->submit_command_buffer(ptCmd, NULL);
    gptGfx->wait_on_command_buffer(ptCmd);

    // Cleanup staging
    gptGfx->destroy_buffer(ptAppData->ptDevice, tStagingHandle);
    gptGfx->free_memory(ptAppData->ptDevice, &tStagingMem);


    // ++++++++++++++++++++++++++++++++++++++ semaphores ++++++++++++++++++++++++++++++++++++++ //

    // create timeline semaphore  
    // ptAppData->ptSemaphore = gptGfx->create_semaphore(ptAppData->ptDevice, false); 



    // ++++++++++++++++++++++++++++++++++++++ Game init ++++++++++++++++++++++++++++++++++++++ //

    // TODO: get this working once we are rendering a board
    // // initialize monopoly game
    // mGameStartSettings tGameSettings = {
    //     .uStartingMoney       = 1500,
    //     .uStartingPlayerCount = 3,
    //     .uJailFine            = 50
    // };
    // ptAppData->pGameData = m_init_game(tGameSettings);

    // m_set_player_piece(ptAppData->pGameData, RACE_CAR, PLAYER_ONE);
    // m_set_player_piece(ptAppData->pGameData, TOP_HAT, PLAYER_TWO);
    // m_set_player_piece(ptAppData->pGameData, THIMBLE, PLAYER_THREE);

    // m_init_game_flow(&ptAppData->tGameFlow, ptAppData->pGameData, NULL);


    // ++++++++++++++++++++++++++++++++++++++ TODO: address this ++++++++++++++++++++++++++++++++++++++ //
    // setup board layout
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
    // flush any pending GPU operations
    gptGfx->flush_device(ptAppData->ptDevice);

    // cleanup textures
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

    // cleanup render passes
    gptGfx->destroy_render_pass(ptAppData->ptDevice, ptAppData->tRenderPass);

    // cleanup shaders
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

    // cleanup swapchain
    gptGfx->cleanup_swapchain(ptAppData->ptSwapchain);

    // cleanup device
    gptGfx->cleanup_device(ptAppData->ptDevice);

    // cleanup graphics system
    gptGfx->cleanup();

    // cleanup game data
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
    // TODO: handle resizing 
}

//-----------------------------------------------------------------------------
// [SECTION] pl_app_update
//-----------------------------------------------------------------------------

PL_EXPORT void
pl_app_update(plAppData* ptAppData)
{
    // ++++++++++++++++++++++++++++++++++++++ begin frame ++++++++++++++++++++++++++++++++++++++ //
    gptGfx->begin_frame(ptAppData->ptDevice);

    // get swapcahin image
    bool bSuccess = gptGfx->acquire_swapchain_image(ptAppData->ptSwapchain);
    if(!bSuccess)
    {
        // Swapchain needs recreation (minimized, resized, etc)
        return;
    }

    // get command buffer
    plCommandBuffer* ptCmd = gptGfx->request_command_buffer(ptAppData->ptCommandPool, "main"); 
    
    // track value
    // uint64_t uCurrentTimelineValue = 1;  
    
    // begin command recording with wait  
    // const plBeginCommandInfo tBeginInfo = {  
    //     .uWaitSemaphoreCount   = 0,  
    //     .atWaitSempahores      = {ptAppData->ptSemaphore},  
    //     .auWaitSemaphoreValues = {uCurrentTimelineValue}  
    // };  
    const plBeginCommandInfo tBeginInfo = {0};
    gptGfx->begin_command_recording(ptCmd, &tBeginInfo);

    // begin render pass (use same handle every frame)
    plRenderEncoder* ptRender = gptGfx->begin_render_pass(ptCmd, ptAppData->tRenderPass, NULL);

    // set viewport & scissor
    plRenderViewport tViewport = {
        .fWidth    = 1280, 
        .fHeight   = 720, 
        .fMaxDepth = 1.0f};
    gptGfx->set_viewport(ptRender, &tViewport);

    plScissor tScissor = {
        .uWidth  = 1280, 
        .uHeight = 720};
    gptGfx->set_scissor_region(ptRender, &tScissor);

    // Bind shader
    gptGfx->bind_shader(ptRender, ptAppData->tTexturedQuadShader);

    // bind vertex/index buffers
    gptGfx->bind_vertex_buffer(ptRender, ptAppData->tQuadVertexBuffer);
    
    // draw board (once texture is loaded)
    if(ptAppData->bBoardTextureLoaded) 
    {
        gptGfx->bind_graphics_bind_groups(ptRender, ptAppData->tTexturedQuadShader, 0, 1, &ptAppData->tBoardBindGroup, 0, NULL);

        plDrawIndex tDraw = {
            .uIndexCount    = 6,
            .tIndexBuffer   = ptAppData->tQuadIndexBuffer,
            .uInstanceCount = 1,
            .uIndexStart    = 0,
            .uInstance      = 0,
            .uVertexStart   = 0
        };
        gptGfx->draw_indexed(ptRender, 1, &tDraw);
    }

    // end renderpass and stop recording 
    gptGfx->end_render_pass(ptRender);
    gptGfx->end_command_recording(ptCmd);

    // present
    // const plSubmitInfo tSubmitInfo = {  
    //     .uSignalSemaphoreCount   = 1,  
    //     .atSignalSempahores      = {ptAppData->ptSemaphore},  
    //     .auSignalSemaphoreValues = {++uCurrentTimelineValue}  
    // };  
    const plSubmitInfo tSubmitInfo = {0};
    gptGfx->present(ptCmd, &tSubmitInfo, &ptAppData->ptSwapchain, 1);

    gptGfx->return_command_buffer(ptCmd);

}

//-----------------------------------------------------------------------------
// [SECTION] rendering functions
//-----------------------------------------------------------------------------

void
render_game_ui(plAppData* ptAppData)
{
    plIO* ptIO = gptIO->get_io();
    mGameData* pGame = ptAppData->pGameData;

    const float fPanelX      = 800.0f;
    const float fPanelY      = 20.0f;
    const float fPanelWidth  = 450.0f;
    const float fPanelHeight = 680.0f;

    const uint32_t uWhite        = PL_COLOR_32(1.0f, 1.0f, 1.0f, 1.0f);
    const uint32_t uDarkGray     = PL_COLOR_32(0.15f, 0.15f, 0.15f, 0.95f);
    const uint32_t uMediumGray   = PL_COLOR_32(0.2f, 0.2f, 0.2f, 1.0f);
    const uint32_t uLightGray    = PL_COLOR_32(0.3f, 0.3f, 0.5f, 1.0f);
    const uint32_t uVeryDarkGray = PL_COLOR_32(0.1f, 0.1f, 0.1f, 1.0f);

    // UI panel background
    gptDraw->add_rect_filled(ptAppData->ptLayer, (plVec2){fPanelX, fPanelY}, (plVec2){fPanelX + fPanelWidth, fPanelY + fPanelHeight}, 
            (plDrawSolidOptions){.uColor = uDarkGray});

    gptDraw->add_rect(ptAppData->ptLayer, (plVec2){fPanelX, fPanelY}, (plVec2){fPanelX + fPanelWidth, fPanelY + fPanelHeight}, 
            (plDrawLineOptions){.uColor = uWhite, .fThickness = 2.0f});

    // current player indicator
    mPlayer* pCurrentPlayer = pGame->mGamePlayers[pGame->uCurrentPlayer];
    float fInfoY = fPanelY + 20.0f;

    uint32_t uPlayerColor = get_player_color(pCurrentPlayer->ePiece);
    gptDraw->add_rect_filled(ptAppData->ptLayer, (plVec2){fPanelX + 20, fInfoY}, (plVec2){fPanelX + 50, fInfoY + 30}, (plDrawSolidOptions){.uColor = uPlayerColor});

    // menu area
    float fMenuY = fPanelY + 100.0f;

    if(m_is_waiting_input(&ptAppData->tGameFlow))
    {
        gptDraw->add_rect_filled(ptAppData->ptLayer, (plVec2){fPanelX + 10, fMenuY}, (plVec2){fPanelX + fPanelWidth - 10, fMenuY + 400}, 
                (plDrawSolidOptions){.uColor = uMediumGray});

        if(ptAppData->tGameFlow.pfCurrentPhase == m_phase_pre_roll)
        {
            for(uint32_t i = 0; i < 5; i++)
            {
                float fItemY = fMenuY + 20 + (i * 50.0f);

                gptDraw->add_rect_filled(ptAppData->ptLayer, (plVec2){fPanelX + 30, fItemY}, (plVec2){fPanelX + fPanelWidth - 30, fItemY + 40}, 
                        (plDrawSolidOptions){.uColor = uLightGray});
            }
        }
    }

    // log area
    float fLogY = fPanelY + fPanelHeight - 150;

    gptDraw->add_rect_filled(ptAppData->ptLayer, (plVec2){fPanelX + 10, fLogY}, (plVec2){fPanelX + fPanelWidth - 10, fPanelY + fPanelHeight - 10}, 
            (plDrawSolidOptions){.uColor = uVeryDarkGray});
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
        case PURSE:         return PL_COLOR_32(1.0f, 0.75f, 0.8f, 1.0f); // pink
        case ROCKING_HORSE: return PL_COLOR_32(0.6f, 0.3f,  0.0f, 1.0f);  // brown
        default:            return PL_COLOR_32(1.0f, 1.0f,  1.0f, 1.0f);  // white
    }
}

void
load_texture(plAppData* ptAppData, const plTextureLoadConfig* ptConfig)
{
    plDevice* ptDevice = ptAppData->ptDevice;

    // load image from disk
    int iWidth, iHeight, iChannels;
    unsigned char* pImageData = stbi_load(ptConfig->pcFilePath, &iWidth, &iHeight, &iChannels, 4);

    if(!pImageData)
    {
        printf("ERROR: Failed to load %s\n", ptConfig->pcFilePath);
        if(ptConfig->pbOutLoaded)
            *ptConfig->pbOutLoaded = false;
        return;
    }
    printf("Loaded texture: %s (%dx%d)\n", ptConfig->pcFilePath, iWidth, iHeight); // success

    // create gpu texture
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

    // allocate gpu memory
    *ptConfig->ptOutMemory = gptGfx->allocate_memory(ptDevice, ptTexture->tMemoryRequirements.ulSize, PL_MEMORY_FLAGS_DEVICE_LOCAL, 
            ptTexture->tMemoryRequirements.uMemoryTypeBits, "texture memory");
    gptGfx->bind_texture_to_memory(ptDevice, *ptConfig->ptOutTexture, ptConfig->ptOutMemory);

    // create staging buffer
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

    // copy image data to staging
    memcpy(tStagingMem.pHostMapped, pImageData, szImageSize);
    stbi_image_free(pImageData);

    // upload staging -> gpu texture
    plCommandBuffer* ptCmdbuff = gptGfx->request_command_buffer(ptAppData->ptCommandPool, "upload texture");
    gptGfx->begin_command_recording(ptCmdbuff, NULL);
    plBlitEncoder* ptBlit = gptGfx->begin_blit_pass(ptCmdbuff);

    // Transition to transfer destination  
    gptGfx->pipeline_barrier_blit(ptBlit, PL_PIPELINE_STAGE_VERTEX_SHADER | PL_PIPELINE_STAGE_TRANSFER, 
            PL_ACCESS_SHADER_READ | PL_ACCESS_TRANSFER_READ, PL_PIPELINE_STAGE_TRANSFER, PL_ACCESS_TRANSFER_WRITE);  
    gptGfx->set_texture_usage(ptBlit, *ptConfig->ptOutTexture, PL_TEXTURE_USAGE_SAMPLED, 0);  

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

    gptGfx->pipeline_barrier_blit(ptBlit, PL_PIPELINE_STAGE_TRANSFER, PL_ACCESS_TRANSFER_WRITE, PL_PIPELINE_STAGE_VERTEX_SHADER | 
        PL_PIPELINE_STAGE_TRANSFER, PL_ACCESS_SHADER_READ | PL_ACCESS_TRANSFER_READ);

    gptGfx->end_blit_pass(ptBlit);
    gptGfx->end_command_recording(ptCmdbuff);
    gptGfx->submit_command_buffer(ptCmdbuff, NULL);
    gptGfx->wait_on_command_buffer(ptCmdbuff);

    // cleanup staging
    gptGfx->destroy_buffer(ptDevice, tStagingHandle);
    gptGfx->free_memory(ptDevice, &tStagingMem);

    // create bind group
    plBindGroupDesc tBGDesc = {
        .tLayout     = ptAppData->tTextureBindGroupLayout,
        .ptPool      = ptAppData->tBindGroupPoolTexAndSamp,
        .pcDebugName = "texture bind group"
    };
    *ptConfig->ptOutBindGroup = gptGfx->create_bind_group(ptDevice, &tBGDesc);

    // update with actual texture + sampler
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