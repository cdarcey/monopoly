/*
   app.c - Monopoly Game (Test Version)
        TODO:

        1. Chance & Community Chest Cards
   
            [x] Implement the card draw system 
            [x] Create the 16 Chance cards with their effects
            [x] Create the 16 Community Chest cards with their effects
            [x] Add card execution logic to POST_ROLL phase
            [ ] Handle "Get Out of Jail Free" cards being held/returned

        2. Property Management Phase

            [ ] Create Helper function to cleanup property arrays 
              i.e. put color groups together etc
            [x] Allow mortgaging/unmortgaging properties 
            [x] Show all owned properties with mortgage status
            [x] Calculate total asset value
            [x] This hooks into the "Manage Properties" button you already have

        3. Building Houses/Hotels

            [x] Add house/hotel counts to mProperty struct
            [x] Implement building rules (need monopoly, even building)
            [x] Calculate rent with houses/hotels 
            [x] Add UI to property management for building
            [x] Make property array index enums for readability
            [x] Make sure post roll phases handle rent calculations correctly now that houses can be added

        4. Trading Phase

            [x] Design trade offer structure (properties, money, jail cards)
            [x] Create UI for selecting trade items
            [x] Implement trade acceptance/rejection
            [x] Handle trade validation (can't trade mortgaged properties)

            Secondary Features
        5. Auction Phase

            [x] Implement when player passes on property purchase
            [x] Bidding system with all players
            [x] Award to highest bidder

        6. Bankruptcy Phase

            [x] Proper asset liquidation (mortgage everything possible)
            [x] Transfer assets to creditor
            [ ] Handle elimination properly

        7. Game End Conditions

            [ ] Show winner screen
            [ ] Track game statistics
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
// [SECTION] structs
//-----------------------------------------------------------------------------

typedef struct _mPropertyBounds
{
    plVec2 tMin;   // top-left corner
    plVec2 tMax;   // bottom-right corner
    plVec2 tCenter; // center point (for drawing tokens)
} mPropertyBounds;

typedef struct _plTextureLoadConfig
{
    const char*               pcFilePath;
    plSamplerHandle           tSampler;
    plTextureHandle*          ptOutTexture;
    plDeviceMemoryAllocation* ptOutMemory;
    plBindGroupHandle*        ptOutBindGroup;
    bool*                     pbOutLoaded;
} plTextureLoadConfig;

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
    plDrawList2D*   ptTokenDrawlist;
    plDrawLayer2D*  ptTokenLayer;
    mPropertyBounds atPropertyBounds[40]; 

    // monopoly game state
    mGameData* pGameData;
    mGameFlow  tGameFlow;

} plAppData;

//-----------------------------------------------------------------------------
// [SECTION] forward declarations
//-----------------------------------------------------------------------------

void   handle_keyboard_input(plAppData* ptAppData);
void   load_texture(plAppData* ptAppData, const plTextureLoadConfig* ptConfig);
plMat4 create_orthographic_projection(float fScreenWidth, float fScreenHeight);
void   show_player_status(mGameData* pGameData);
void   draw_dice_result(plAppData* ptAppData);
void   init_property_bounds(mPropertyBounds* atBounds);
void   draw_player_tokens(plAppData* ptAppData, plRenderEncoder* ptRender);
void   draw_preroll_menu(plAppData* ptAppData);
void   draw_postroll_menu(plAppData* ptAppData);
void   draw_jail_menu(plAppData* ptAppData);
void   draw_notification(plAppData* ptAppData);
void   draw_property_management_menu(plAppData* ptAppData);
void   draw_auction_menu(plAppData* ptAppData);
void   draw_trade_menu(plAppData* ptAppData);


//-----------------------------------------------------------------------------
// [SECTION] global api pointers
//-----------------------------------------------------------------------------

const plIOI*          gptIO          = NULL;
const plWindowI*      gptWindows     = NULL;
const plGraphicsI*    gptGfx         = NULL;
const plDrawI*        gptDraw        = NULL;
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
        .apcIncludeDirectories = { "../../monopoly/shaders/", "../shaders/"},
        .apcDirectories = { "../../monopoly/shaders/", "../shaders/"},
        .tFlags = PL_SHADER_FLAGS_NEVER_CACHE
    };
    gptShader->initialize(&tDefaultShaderOptions);
    
    // initialize draw backend (required for ui)
    const plDrawInit tDrawInit = {
        .ptDevice = ptAppData->ptDevice
    };
    gptDraw->initialize(&tDrawInit);

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
    gptDraw->build_font_atlas(ptCmdFont, gptDraw->get_current_font_atlas());
    gptGfx->return_command_buffer(ptCmdFont);
    
    // initialize ui (requires font atlas to be built first)
    gptUi->initialize();
    gptUi->set_default_font(ptDefaultFont);
    gptUi->set_dark_theme();

    // create sampler
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
        .pcFilePath     = "../../monopoly/assets/monopoly-board.png",
        .tSampler       = ptAppData->tLinearSampler,
        .ptOutTexture   = &ptAppData->tBoardTexture,
        .ptOutMemory    = &ptAppData->tBoardTextureMemory,
        .ptOutBindGroup = &ptAppData->tBoardBindGroup,
        .pbOutLoaded    = &ptAppData->bBoardTextureLoaded
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
        .tVertexShader            = tVertModule,
        .tFragmentShader          = tFragModule,
        .atVertexBufferLayouts[0] = tVertexLayout,
        .atBindGroupLayouts[0]    = ptAppData->tTextureBindGroupLayoutDesc,
        .tRenderPassLayout        = ptAppData->tMainPassLayout,
        .pcDebugName              = "textured quad shader"
    };
    ptAppData->tTexturedQuadShader = gptGfx->create_shader(ptAppData->ptDevice, &tShaderDesc);

    // create render pass
    plRenderPassDesc tMainPassDesc = {
        .tLayout        = ptAppData->tMainPassLayout,
        .tDimensions    = {SCREEN_WIDTH, SCREEN_HEIGHT},
        .atColorTargets = {
            {
                .tLoadOp       = PL_LOAD_OP_CLEAR,
                .tStoreOp      = PL_STORE_OP_STORE,
                .tCurrentUsage = PL_TEXTURE_USAGE_UNSPECIFIED,
                .tNextUsage    = PL_TEXTURE_USAGE_PRESENT,
                .tClearColor = {0.0f, 0.4f, 0.2f, 1.0f} // monopoly style green for background
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
        .tUsage      = PL_BUFFER_USAGE_VERTEX | PL_BUFFER_USAGE_TRANSFER_DESTINATION,
        .szByteSize  = sizeof(atVertices),
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
        .tUsage      = PL_BUFFER_USAGE_STAGING,
        .szByteSize  = 4096,
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

    // TODO: create menu system so player can adjust these before game starts
    // initialize monopoly game
    mGameSettings tSettings = {
        .uStartingMoney = 200,
        .uJailFine      = 50,
        .uPlayerCount   = 2
    };
    ptAppData->pGameData = m_init_game(tSettings);
    m_init_game_flow(&ptAppData->tGameFlow, ptAppData->pGameData, ptAppData->ptWindow);

    // create persistent drawlist and layer for player tokens
    ptAppData->ptTokenDrawlist = gptDraw->request_2d_drawlist();
    ptAppData->ptTokenLayer = gptDraw->request_2d_layer(ptAppData->ptTokenDrawlist);

    // init bounding boxes for properties 
    init_property_bounds(ptAppData->atPropertyBounds);

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
    gptDraw->cleanup_font_atlas(gptDraw->get_current_font_atlas());

    // cleanup draw system, backend, and UI
    gptDraw->cleanup();
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

    // cleanup shader, bind group pool, layout, and sampler
    gptGfx->destroy_shader(ptAppData->ptDevice, ptAppData->tTexturedQuadShader);
    gptGfx->cleanup_bind_group_pool(ptAppData->tBindGroupPoolTexAndSamp);
    gptGfx->destroy_bind_group_layout(ptAppData->ptDevice, ptAppData->tTextureBindGroupLayout);
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

    // process input events and start frame calls
    gptIO->new_frame();
    gptDraw->new_frame();
    gptUi->new_frame();
    handle_keyboard_input(ptAppData);

    // show ui windows
    show_player_status(ptAppData->pGameData);
    draw_dice_result(ptAppData);
        
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
    else if(ptAppData->tGameFlow.pfCurrentPhase == m_phase_jail)
    {
        draw_jail_menu(ptAppData);
    }
    else if(ptAppData->tGameFlow.pfCurrentPhase == m_phase_property_management)
    {
        draw_property_management_menu(ptAppData);
    }
    else if(ptAppData->tGameFlow.pfCurrentPhase == m_phase_auction)
    {
        draw_auction_menu(ptAppData);
    }
    else if(ptAppData->tGameFlow.pfCurrentPhase == m_phase_trade)
    {
        draw_trade_menu(ptAppData);
    }
        
    // show notification popup (on top of everything)
    draw_notification(ptAppData);

    // run game phase
    m_run_current_phase(&ptAppData->tGameFlow, 0.016f);
    if(m_check_game_over(ptAppData->pGameData))
    {
        //  TODO: add some shutdown screen
    }

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
        gptDraw->submit_2d_drawlist(ptDrawlist, ptRender, (float)SCREEN_WIDTH, (float)SCREEN_HEIGHT, 1);
        gptDraw->submit_2d_drawlist(gptUi->get_debug_draw_list(), ptRender, (float)SCREEN_WIDTH, (float)SCREEN_HEIGHT, 1);
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
init_property_bounds(mPropertyBounds* atBounds)
{
    // corners are larger than standard property pieces 
    // bottom right corner (GO) - position 0
    atBounds[0] = (mPropertyBounds){
        .tMin = {655.0f, 615.0f},
        .tMax = {750.0f, 710.0f},
        .tCenter = {702.5f, 662.5f}
    };
    
    // bottom row properties (positions 1-9)
    float fBottomY = 615.0f;
    float fBottomHeight = 95.0f; // 710 - 615
    float afBottomX[] = {145.0f, 201.67f, 258.33f, 315.0f, 371.67f, 428.33f, 485.0f, 541.67f, 598.33f, 655.0f};
    
    for(uint8_t i = 0; i < 9; i++)
    {
        atBounds[i + 1] = (mPropertyBounds)
        {
            .tMin = {afBottomX[i], fBottomY},
            .tMax = {afBottomX[i + 1], fBottomY + fBottomHeight},
            .tCenter = {(afBottomX[i] + afBottomX[i + 1]) / 2.0f, fBottomY + fBottomHeight / 2.0f}
        };
    }
    
    // bottom left corner (jail) - position 10
    atBounds[10] = (mPropertyBounds)
    {
        .tMin = {50.0f, 615.0f},
        .tMax = {145.0f, 710.0f},
        .tCenter = {97.5f, 662.5f}
    };
    
    // left column properties (positions 11-19)
    float fLeftX = 50.0f;
    float fLeftWidth = 95.0f; // 145 - 50
    float afLeftY[] = {615.0f, 558.33f, 501.67f, 445.0f, 388.33f, 331.67f, 275.0f, 218.33f, 161.67f, 105.0f};
    
    for(uint8_t i = 0; i < 9; i++)
    {
        atBounds[i + 11] = (mPropertyBounds){
            .tMin = {fLeftX, afLeftY[i + 1]},
            .tMax = {fLeftX + fLeftWidth, afLeftY[i]},
            .tCenter = {fLeftX + fLeftWidth / 2.0f, (afLeftY[i] + afLeftY[i + 1]) / 2.0f}
        };
    }
    
    // top left corner (free parking) - position 20
    atBounds[20] = (mPropertyBounds){
        .tMin = {50.0f, 10.0f},
        .tMax = {145.0f, 105.0f},
        .tCenter = {97.5f, 57.5f}
    };
    
    // top row properties (positions 21-29)
    float fTopY = 10.0f;
    float fTopHeight = 95.0f; // 105 - 10
    float afTopX[] = {145.0f, 201.67f, 258.33f, 315.0f, 371.67f, 428.33f, 485.0f, 541.67f, 598.33f, 655.0f};
    
    for(uint8_t i = 0; i < 9; i++)
    {
        atBounds[i + 21] = (mPropertyBounds){
            .tMin = {afTopX[i], fTopY},
            .tMax = {afTopX[i + 1], fTopY + fTopHeight},
            .tCenter = {(afTopX[i] + afTopX[i + 1]) / 2.0f, fTopY + fTopHeight / 2.0f}
        };
    }
    
    // top right corner (go to jail) - position 30
    atBounds[30] = (mPropertyBounds){
        .tMin = {655.0f, 10.0f},
        .tMax = {750.0f, 105.0f},
        .tCenter = {702.5f, 57.5f}
    };
    
    // right column properties (positions 31-39)
    float fRightX = 655.0f;
    float fRightWidth = 95.0f; // 750 - 655
    float afRightY[] = {105.0f, 161.67f, 218.33f, 275.0f, 331.67f, 388.33f, 445.0f, 501.67f, 558.33f, 615.0f};
    
    for(uint8_t i = 0; i < 9; i++)
    {
        atBounds[i + 31] = (mPropertyBounds){
            .tMin = {fRightX, afRightY[i]},
            .tMax = {fRightX + fRightWidth, afRightY[i + 1]},
            .tCenter = {fRightX + fRightWidth / 2.0f, (afRightY[i] + afRightY[i + 1]) / 2.0f}
        };
    }
}

void
draw_player_tokens(plAppData* ptAppData, plRenderEncoder* ptRender)
{
    plDrawList2D* ptDrawlist = ptAppData->ptTokenDrawlist;
    plDrawLayer2D* ptLayer = ptAppData->ptTokenLayer;
    
    const uint32_t atPlayerColors[6] = {
        {PL_COLOR_32_RED},
        {PL_COLOR_32_BLUE},
        {PL_COLOR_32_GREEN},
        {PL_COLOR_32_YELLOW},
        {PL_COLOR_32_MAGENTA},
        {PL_COLOR_32_ORANGE}
    };
    
    for(uint8_t i = 0; i < ptAppData->pGameData->uPlayerCount; i++)
    {
        // get player to draw this iteration/ skip bankrupts
        mPlayer* pPlayer = &ptAppData->pGameData->amPlayers[i];
        if(pPlayer->bIsBankrupt)
            continue;
        
        // count how many players are on this same space
        uint8_t uPlayersOnSpace = 0;
        for(uint8_t j = 0; j < ptAppData->pGameData->uPlayerCount; j++)
        {
            if(ptAppData->pGameData->amPlayers[j].uPosition == pPlayer->uPosition && !ptAppData->pGameData->amPlayers[j].bIsBankrupt)
            {
                uPlayersOnSpace++;
            }
        }
        
        // get bounds for this position
        mPropertyBounds* pBounds = &ptAppData->atPropertyBounds[pPlayer->uPosition];
        plVec2 tCenter = pBounds->tCenter;

        plVec2 tTokenPos;

        // if only one player, just center them
        if(uPlayersOnSpace == 1)
        {
            tTokenPos = tCenter;
        }
        else
        {
            // calculate radius based on property size (corners not same as props)
            float fWidth = pBounds->tMax.x - pBounds->tMin.x;
            float fHeight = pBounds->tMax.y - pBounds->tMin.y;
            float fSmallestDim = (fWidth < fHeight) ? fWidth : fHeight; // smallest to fit weather side prop or top/ bottom prop
            float fRadius = (fSmallestDim / 2.0f) * 0.6f; // 60% for some padding
            
            // figure out player 1, player 2, etc..
            uint8_t uPlayerSlot = 0;
            for(uint8_t j = 0; j < i; j++)
            {
                if(ptAppData->pGameData->amPlayers[j].uPosition == pPlayer->uPosition && !ptAppData->pGameData->amPlayers[j].bIsBankrupt)
                {
                    uPlayerSlot++;
                }
            }
            
            // calculate angle for this player (where to draw on the circle inside square)
            float fAnglePerPlayer = (2.0f * PL_PI) / (float)uPlayersOnSpace;
            float fAngle = fAnglePerPlayer * (float)uPlayerSlot;
            
            // calculate position using sin/cos
            tTokenPos.x = tCenter.x + fRadius * cosf(fAngle);
            tTokenPos.y = tCenter.y + fRadius * sinf(fAngle);
        }
        
        // draw circle for player token
        uint32_t uColor = atPlayerColors[i];
        plDrawSolidOptions tOptions = {.uColor = uColor};
        gptDraw->add_circle_filled(ptLayer, tTokenPos, 10.0f, 16, tOptions);
    }
    
    gptDraw->submit_2d_layer(ptLayer);
    gptDraw->submit_2d_drawlist(ptDrawlist, ptRender, (float)SCREEN_WIDTH, (float)SCREEN_HEIGHT, 1);
}

void
draw_preroll_menu(plAppData* ptAppData)
{
    mGameData* pGame = ptAppData->pGameData;
    
    // only show if menu flag is set and we're in pre-roll phase
    if(!pGame->bShowPrerollMenu || ptAppData->tGameFlow.pfCurrentPhase != m_phase_pre_roll)
        return;
    
    // position menu in top right
    gptUi->set_next_window_pos((plVec2){800.0f, 15.0f}, PL_UI_COND_ALWAYS);
    gptUi->set_next_window_size((plVec2){400.0f, 230.0f}, PL_UI_COND_ALWAYS);
    
    char acWindowTitle[64];
    snprintf(acWindowTitle, sizeof(acWindowTitle), "Player %d's Turn", pGame->uCurrentPlayerIndex + 1);
    
    if(!gptUi->begin_window(acWindowTitle, NULL, PL_UI_WINDOW_FLAGS_NO_RESIZE | PL_UI_WINDOW_FLAGS_NO_COLLAPSE | PL_UI_WINDOW_FLAGS_NO_MOVE | PL_UI_WINDOW_FLAGS_NO_SCROLLBAR))
        return;
    
    gptUi->layout_static(0.0f, 390, 1);
    gptUi->text("What would you like to do?");
    
    gptUi->vertical_spacing();
    
    // all buttons same size - 45px height, 360px width
    gptUi->layout_static(45.0f, 390, 1);
    
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
    mPostRollData* pPostRoll = (mPostRollData*)ptAppData->tGameFlow.pCurrentPhaseData;
    
    // check if we're on an unowned property and haven't handled landing yet
    bool bOnUnownedProperty = false;
    mProperty* pProp = NULL;
    uint8_t uPropIdx = BANK_PLAYER_INDEX;
    
    if(m_get_square_type(pPlayer->uPosition) == SQUARE_PROPERTY && !pPostRoll->bHandledLanding)
    {
        uPropIdx = m_get_property_at_position(pGame, pPlayer->uPosition);
        if(uPropIdx != BANK_PLAYER_INDEX)
        {
            pProp = &pGame->amProperties[uPropIdx];
            if(pProp->uOwnerIndex == BANK_PLAYER_INDEX)
            {
                bOnUnownedProperty = true;
            }
        }
    }

    // show property purchase menu if on unowned property
    if(bOnUnownedProperty && m_is_waiting_input(&ptAppData->tGameFlow))
    {
        gptUi->set_next_window_pos((plVec2){800.0f, 15.0f}, PL_UI_COND_ALWAYS);
        gptUi->set_next_window_size((plVec2){400.0f, 350.0f}, PL_UI_COND_ALWAYS);
        
        if(!gptUi->begin_window("Property Available", NULL, PL_UI_WINDOW_FLAGS_NO_RESIZE | PL_UI_WINDOW_FLAGS_NO_COLLAPSE | PL_UI_WINDOW_FLAGS_NO_MOVE | PL_UI_WINDOW_FLAGS_NO_SCROLLBAR))
            return;
        
        gptUi->layout_static(0.0f, 390, 1);
        gptUi->text("%s", pProp->cName);
        
        gptUi->vertical_spacing();
        
        gptUi->layout_static(0.0f, 390, 1);
        gptUi->text("Price: $%d", pProp->uPrice);
        gptUi->vertical_spacing();
        
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
        
        gptUi->layout_static(45.0f, 390, 1);
        
        if(bCanAfford && gptUi->button("Buy Property"))
        {
            m_set_input_int(&ptAppData->tGameFlow, 1);
        }
        
        if(gptUi->button("Pass (to Auction)"))
        {
            m_set_input_int(&ptAppData->tGameFlow, 2);
        }
        
        if(gptUi->button("Manage Properties"))
        {
            m_set_input_int(&ptAppData->tGameFlow, 3);
        }
        
        if(gptUi->button("Propose Trade"))
        {
            m_set_input_int(&ptAppData->tGameFlow, 4);
        }
        
        gptUi->end_window();
    }
    // show end-of-turn menu after forced actions
    else if(m_is_waiting_input(&ptAppData->tGameFlow))
    {
        gptUi->set_next_window_pos((plVec2){800.0f, 15.0f}, PL_UI_COND_ALWAYS);
        gptUi->set_next_window_size((plVec2){400.0f, 240.0f}, PL_UI_COND_ALWAYS);
        
        if(!gptUi->begin_window("Turn Options", NULL, PL_UI_WINDOW_FLAGS_NO_RESIZE | PL_UI_WINDOW_FLAGS_NO_COLLAPSE | PL_UI_WINDOW_FLAGS_NO_MOVE | PL_UI_WINDOW_FLAGS_NO_SCROLLBAR))
            return;
        
        gptUi->layout_static(0.0f, 390, 1);
        gptUi->text("Post roll actions");
        
        gptUi->vertical_spacing();
        
        gptUi->layout_static(45.0f, 390, 1);
        
        if(gptUi->button("Manage Properties"))
        {
            m_set_input_int(&ptAppData->tGameFlow, 1);
        }
        
        if(gptUi->button("Propose Trade"))
        {
            m_set_input_int(&ptAppData->tGameFlow, 2);
        }
        
        if(gptUi->button("End Turn"))
        {
            m_set_input_int(&ptAppData->tGameFlow, 3);
        }
        
        gptUi->end_window();
    }
}

void
draw_jail_menu(plAppData* ptAppData)
{
    mGameData* pGame = ptAppData->pGameData;
    mPlayer* pPlayer = &pGame->amPlayers[pGame->uCurrentPlayerIndex];

    // only show if jail menu flag is set
    if(!pGame->bShowJailMenu)
        return;
    
    // only show if in jail and waiting for input
    if(pPlayer->uJailTurns == 0 || !m_is_waiting_input(&ptAppData->tGameFlow))
        return;
    
    // position menu in top right
    gptUi->set_next_window_pos((plVec2){800.0f, 15.0f}, PL_UI_COND_ALWAYS);
    gptUi->set_next_window_size((plVec2){400.0f, 250.0f}, PL_UI_COND_ALWAYS);
    
    char acWindowTitle[64];
    snprintf(acWindowTitle, sizeof(acWindowTitle), "In Jail (Attempt %d/3)", pPlayer->uJailTurns);
    
    if(!gptUi->begin_window(acWindowTitle, NULL, PL_UI_WINDOW_FLAGS_NO_RESIZE | PL_UI_WINDOW_FLAGS_NO_COLLAPSE | PL_UI_WINDOW_FLAGS_NO_MOVE | PL_UI_WINDOW_FLAGS_NO_SCROLLBAR))
        return;
    
    gptUi->layout_static(0.0f, 390, 1);
    gptUi->text("Choose an option to get out:");
    
    gptUi->vertical_spacing();
    
    // buttons - 45px height
    gptUi->layout_static(45.0f, 390, 1);
    
    // pay fine button
    bool bCanAffordFine = m_can_afford(pPlayer, pGame->uJailFine);
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
    gptUi->set_next_window_pos((plVec2){300.0f, 15.0f}, PL_UI_COND_ALWAYS);
    gptUi->set_next_window_size((plVec2){680.0f, 80.0f}, PL_UI_COND_ALWAYS);
    
    if(!gptUi->begin_window("##notification", NULL, PL_UI_WINDOW_FLAGS_NO_RESIZE | PL_UI_WINDOW_FLAGS_NO_COLLAPSE | PL_UI_WINDOW_FLAGS_NO_MOVE | PL_UI_WINDOW_FLAGS_NO_SCROLLBAR | PL_UI_WINDOW_FLAGS_NO_TITLE_BAR))
        return;
    
    gptUi->layout_static(20.0f, 500.0f, 1);
    gptUi->text("%s", pGame->acNotification);
    
    gptUi->end_window();
}

void 
show_player_status(mGameData* pGameData)
{
    // render player status ui
    gptUi->set_next_window_pos((plVec2){800.0f, 460.0f}, PL_UI_COND_ALWAYS);
    gptUi->set_next_window_size((plVec2){400.0f, 250.0f}, PL_UI_COND_ALWAYS);

    // for code readability  
    mPlayer* pPlayer = &pGameData->amPlayers[pGameData->uCurrentPlayerIndex];

    char acWindowTitle[64];
    snprintf(acWindowTitle, sizeof(acWindowTitle), "Player %d Status", pGameData->uCurrentPlayerIndex + 1);

    if(!gptUi->begin_window(acWindowTitle, NULL, PL_UI_WINDOW_FLAGS_NO_RESIZE | PL_UI_WINDOW_FLAGS_NO_COLLAPSE | PL_UI_WINDOW_FLAGS_NO_MOVE))
        return;

    // show cash and net worth on same line
    gptUi->layout_static(0.0f, 160, 2);
    gptUi->text("Cash: $%d", pPlayer->uMoney);
    gptUi->text("Net Worth: $%d", m_calculate_net_worth(pGameData, pGameData->uCurrentPlayerIndex));
    
    gptUi->vertical_spacing();

    // show position on single line
    gptUi->layout_static(0.0f, 360, 1);
    gptUi->text("Position: %s", m_get_square_name(pGameData, pPlayer->uPosition));

    gptUi->vertical_spacing();

    gptUi->separator();
    gptUi->vertical_spacing();

    // properties owned (collapsible) 
    gptUi->layout_static(0.0f, 360, 1);
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
            
            if(pProp->bIsMortgaged)
            {
                gptUi->color_text((plVec4){1.0f, 0.5f, 0.5f, 1.0f}, "  %s [M]", pProp->cName);
            }
            else
            {
                gptUi->text("  %s", pProp->cName);
            }
        }
        if(!bHasProperties)
        {
            gptUi->text("  None");
        }
        gptUi->end_collapsing_header();
    }
    gptUi->end_window();
}

void 
draw_dice_result(plAppData* ptAppData)
{
    // only show if in post-roll phase
    if(ptAppData->tGameFlow.pfCurrentPhase != m_phase_post_roll)
        return;

    // position right above player status window
    gptUi->set_next_window_pos((plVec2){800.0f, 390.0f}, PL_UI_COND_ALWAYS);
    gptUi->set_next_window_size((plVec2){400.0f, 60.0f}, PL_UI_COND_ALWAYS);

    if(!gptUi->begin_window("Dice Result", NULL, PL_UI_WINDOW_FLAGS_NO_RESIZE | PL_UI_WINDOW_FLAGS_NO_COLLAPSE | PL_UI_WINDOW_FLAGS_NO_MOVE | PL_UI_WINDOW_FLAGS_NO_TITLE_BAR))
        return;

    gptUi->layout_static(0.0f, 360, 1);
    gptUi->text("Dice: %d + %d = %d", 
        ptAppData->pGameData->tDice.uDie1, 
        ptAppData->pGameData->tDice.uDie2, 
        ptAppData->pGameData->tDice.uDie1 + ptAppData->pGameData->tDice.uDie2);

    gptUi->end_window();
}
void
draw_property_management_menu(plAppData* ptAppData)
{
    mGameData* pGame = ptAppData->pGameData;
    mPlayer* pPlayer = &pGame->amPlayers[pGame->uCurrentPlayerIndex];
    
    if(!pGame->bShowPropertyMenu)
        return;
    
    gptUi->set_next_window_pos((plVec2){800.0f, 15.0f}, PL_UI_COND_ALWAYS);
    gptUi->set_next_window_size((plVec2){400.0f, 400.0f}, PL_UI_COND_ALWAYS);
    
    if(!gptUi->begin_window("Property Management", NULL, PL_UI_WINDOW_FLAGS_NO_RESIZE | PL_UI_WINDOW_FLAGS_NO_COLLAPSE | PL_UI_WINDOW_FLAGS_NO_MOVE))
        return;
    
    gptUi->layout_static(0.0f, 390, 1);
    gptUi->text("Cash: $%d", pPlayer->uMoney);
    
    gptUi->vertical_spacing();
    gptUi->separator();
    gptUi->vertical_spacing();
    
    if(pPlayer->uPropertyCount == 0)
    {
        gptUi->layout_static(0.0f, 390, 1);
        gptUi->text("No properties owned");
    }
    else
    {
        for(uint8_t uPlayerPropArrayIdx = 0; uPlayerPropArrayIdx < pPlayer->uPropertyCount; uPlayerPropArrayIdx++)
        {
            uint8_t uGlobalPropIdx = pPlayer->auPropertiesOwned[uPlayerPropArrayIdx];
            if(uGlobalPropIdx == BANK_PLAYER_INDEX)
                break;
            
            mProperty* pProp = &pGame->amProperties[uGlobalPropIdx];
            
            // property name with status
            gptUi->layout_static(0.0f, 390, 1);
            char acPropStatus[128];
            if(pProp->bHasHotel)
            {
                snprintf(acPropStatus, sizeof(acPropStatus), "%s [HOTEL]", pProp->cName);
                gptUi->color_text((plVec4){0.0f, 1.0f, 0.0f, 1.0f}, acPropStatus);
            }
            else if(pProp->uHouses > 0)
            {
                snprintf(acPropStatus, sizeof(acPropStatus), "%s [%d Houses]", pProp->cName, pProp->uHouses);
                gptUi->color_text((plVec4){0.0f, 0.8f, 1.0f, 1.0f}, acPropStatus);
            }
            else if(pProp->bIsMortgaged)
            {
                snprintf(acPropStatus, sizeof(acPropStatus), "%s [MORTGAGED]", pProp->cName);
                gptUi->color_text((plVec4){1.0f, 0.5f, 0.0f, 1.0f}, acPropStatus);
            }
            else
            {
                gptUi->text("%s", pProp->cName);
            }
            
            // only show building options for streets
            if(pProp->eType == PROPERTY_TYPE_STREET)
            {
                gptUi->layout_static(30.0f, 175, 2);
                
                // build house button
                bool bCanBuildHouse = m_can_build_house(pGame, uGlobalPropIdx, pGame->uCurrentPlayerIndex);
                char acHouseBtn[64];
                snprintf(acHouseBtn, sizeof(acHouseBtn), "Build House ($%d)", pProp->uHouseCost);
                
                if(bCanBuildHouse && gptUi->button(acHouseBtn))
                {
                    m_set_input_int(&ptAppData->tGameFlow, 100 + uPlayerPropArrayIdx);
                }
                else if(!bCanBuildHouse)
                {
                    gptUi->color_text((plVec4){0.5f, 0.5f, 0.5f, 1.0f}, acHouseBtn);
                }
                
                // sell house button
                bool bCanSellHouse = m_can_sell_house(pGame, uGlobalPropIdx, pGame->uCurrentPlayerIndex);
                char acSellHouseBtn[64];
                snprintf(acSellHouseBtn, sizeof(acSellHouseBtn), "Sell House (+$%d)", pProp->uHouseCost / 2);
                
                if(bCanSellHouse && gptUi->button(acSellHouseBtn))
                {
                    m_set_input_int(&ptAppData->tGameFlow, 300 + uPlayerPropArrayIdx);
                }
                else if(!bCanSellHouse)
                {
                    gptUi->color_text((plVec4){0.5f, 0.5f, 0.5f, 1.0f}, acSellHouseBtn);
                }
                
                gptUi->layout_static(30.0f, 175, 2);
                
                // build hotel button
                bool bCanBuildHotel = m_can_build_hotel(pGame, uGlobalPropIdx, pGame->uCurrentPlayerIndex);
                char acHotelBtn[64];
                snprintf(acHotelBtn, sizeof(acHotelBtn), "Build Hotel ($%d)", pProp->uHouseCost);
                
                if(bCanBuildHotel && gptUi->button(acHotelBtn))
                {
                    m_set_input_int(&ptAppData->tGameFlow, 200 + uPlayerPropArrayIdx);
                }
                else if(!bCanBuildHotel)
                {
                    gptUi->color_text((plVec4){0.5f, 0.5f, 0.5f, 1.0f}, acHotelBtn);
                }
                
                // sell hotel button
                bool bCanSellHotel = m_can_sell_hotel(pGame, uGlobalPropIdx, pGame->uCurrentPlayerIndex);
                char acSellHotelBtn[64];
                snprintf(acSellHotelBtn, sizeof(acSellHotelBtn), "Sell Hotel (+$%d)", pProp->uHouseCost / 2);
                
                if(bCanSellHotel && gptUi->button(acSellHotelBtn))
                {
                    m_set_input_int(&ptAppData->tGameFlow, 400 + uPlayerPropArrayIdx);
                }
                else if(!bCanSellHotel)
                {
                    gptUi->color_text((plVec4){0.5f, 0.5f, 0.5f, 1.0f}, acSellHotelBtn);
                }
            }
            
            // mortgage/unmortgage button (all property types)
            gptUi->layout_static(30.0f, 390, 1);
            if(pProp->bIsMortgaged)
            {
                uint32_t uCost = pProp->uMortgageValue + (pProp->uMortgageValue / 10);
                bool bCanAfford = pPlayer->uMoney >= uCost;
                char acUnmortgageBtn[64];
                snprintf(acUnmortgageBtn, sizeof(acUnmortgageBtn), "Unmortgage ($%d)", uCost);
                
                if(bCanAfford && gptUi->button(acUnmortgageBtn))
                {
                    m_set_input_int(&ptAppData->tGameFlow, uPlayerPropArrayIdx + 1);
                }
                else if(!bCanAfford)
                {
                    char acGrayText[64];
                    snprintf(acGrayText, sizeof(acGrayText), "Unmortgage ($%d) - Can't afford", uCost);
                    gptUi->color_text((plVec4){0.5f, 0.5f, 0.5f, 1.0f}, acGrayText);
                }
            }
            else
            {
                char acMortgageBtn[64];
                snprintf(acMortgageBtn, sizeof(acMortgageBtn), "Mortgage (Get $%d)", pProp->uMortgageValue);
                
                if(gptUi->button(acMortgageBtn))
                {
                    m_set_input_int(&ptAppData->tGameFlow, uPlayerPropArrayIdx + 1);
                }
            }
            
            gptUi->vertical_spacing();
            gptUi->separator();
            gptUi->vertical_spacing();
        }
    }
    
    // exit button
    gptUi->layout_static(45.0f, 390, 1);
    if(gptUi->button("Back to Turn Menu"))
    {
        m_set_input_int(&ptAppData->tGameFlow, 0);
    }
    
    gptUi->end_window();
}

void
draw_auction_menu(plAppData* ptAppData)
{
    mGameData* pGame = ptAppData->pGameData;
    
    if(!pGame->bShowAuctionMenu)
        return;
    
    // get auction data from current phase
    mAuctionData* pAuction = (mAuctionData*)ptAppData->tGameFlow.pCurrentPhaseData;
    mProperty* pProp = &pGame->amProperties[pAuction->ePropertyIndex];
    mPlayer* pCurrentBidder = &pGame->amPlayers[pAuction->uCurrentBidder];
    
    // position menu in center
    gptUi->set_next_window_pos((plVec2){300.0f, 50.0f}, PL_UI_COND_ALWAYS);
    gptUi->set_next_window_size((plVec2){680.0f, 500.0f}, PL_UI_COND_ALWAYS);
    
    if(!gptUi->begin_window("Auction", NULL, PL_UI_WINDOW_FLAGS_NO_RESIZE | PL_UI_WINDOW_FLAGS_NO_COLLAPSE | PL_UI_WINDOW_FLAGS_NO_MOVE))
        return;
    
    // property being auctioned
    gptUi->layout_static(0.0f, 660, 1);
    gptUi->text("Property: %s", pProp->cName);
    gptUi->text("List Price: $%d", pProp->uPrice);
    
    gptUi->vertical_spacing();
    gptUi->separator();
    gptUi->vertical_spacing();
    
    // current bid info
    gptUi->layout_static(0.0f, 660, 1);
    if(pAuction->uHighestBidder == BANK_PLAYER_INDEX)
    {
        gptUi->text("Current Bid: No bids yet");
    }
    else
    {
        gptUi->text("Current Bid: $%d by Player %d", pAuction->uHighestBid, pAuction->uHighestBidder + 1);
    }
    
    gptUi->vertical_spacing();
    gptUi->separator();
    gptUi->vertical_spacing();
    
    // player status
    gptUi->layout_static(0.0f, 660, 1);
    gptUi->text("Players:");
    
    for(uint8_t i = 0; i < pGame->uPlayerCount; i++)
    {
        if(pGame->amPlayers[i].bIsBankrupt)
            continue;
        
        char acPlayerStatus[128];
        if(pAuction->abPlayersPassed[i])
        {
            snprintf(acPlayerStatus, sizeof(acPlayerStatus), "  Player %d: $%d [PASSED]", 
                i + 1, pGame->amPlayers[i].uMoney);
            gptUi->color_text((plVec4){0.7f, 0.7f, 0.7f, 1.0f}, acPlayerStatus);
        }
        else if(i == pAuction->uCurrentBidder)
        {
            snprintf(acPlayerStatus, sizeof(acPlayerStatus), "  Player %d: $%d [BIDDING NOW]", 
                i + 1, pGame->amPlayers[i].uMoney);
            gptUi->color_text((plVec4){0.0f, 1.0f, 0.0f, 1.0f}, acPlayerStatus);
        }
        else
        {
            snprintf(acPlayerStatus, sizeof(acPlayerStatus), "  Player %d: $%d", 
                i + 1, pGame->amPlayers[i].uMoney);
            gptUi->text(acPlayerStatus);
        }
    }
    
    gptUi->vertical_spacing();
    gptUi->separator();
    gptUi->vertical_spacing();
    
    // bidding controls for current bidder
    gptUi->layout_static(0.0f, 660, 1);
    gptUi->text("Player %d - Your turn to bid:", pAuction->uCurrentBidder + 1);
    
    gptUi->vertical_spacing();
    
    uint32_t uMinBid = pAuction->uHighestBid + 1;
    
    // show minimum bid
    gptUi->layout_static(0.0f, 660, 1);
    gptUi->text("Minimum bid: $%d", uMinBid);
    
    gptUi->vertical_spacing();
    
    // text input for bid amount
    gptUi->layout_static(40.0f, 500, 1);
    static char acBidInput[32] = ""; // static to persit through frames without global variable
    gptUi->input_text("Bid Amount", acBidInput, 32, 0);

    gptUi->layout_static(45.0f, 320, 2);

    // submit bid button
    char acSubmitBtn[64];
    snprintf(acSubmitBtn, sizeof(acSubmitBtn), "Submit Bid");
    if(gptUi->button(acSubmitBtn))
    {
        int iBidAmount = atoi(acBidInput); // Convert string to int
        if(iBidAmount > 0)
        {
            m_set_input_int(&ptAppData->tGameFlow, iBidAmount);
            acBidInput[0] = '\0';
        }
    }

    // pass button
    if(gptUi->button("Pass (Don't Bid)"))
    {
        m_set_input_int(&ptAppData->tGameFlow, 0);
        acBidInput[0] = '\0';
    }
    
    gptUi->end_window();
}

void
draw_trade_menu(plAppData* ptAppData)
{
    mGameData* pGame = ptAppData->pGameData;
    
    if(!pGame->bShowTradeMenu)
        return;
    
    mTradeData* pTrade = (mTradeData*)ptAppData->tGameFlow.pCurrentPhaseData;
    mPlayer* pCurrentPlayer = &pGame->amPlayers[pGame->uCurrentPlayerIndex];
    
    // centered window
    gptUi->set_next_window_pos((plVec2){300.0f, 150.0f}, PL_UI_COND_ALWAYS);
    gptUi->set_next_window_size((plVec2){680.0f, 500.0f}, PL_UI_COND_ALWAYS);
    
    if(!gptUi->begin_window("Trade", NULL, PL_UI_WINDOW_FLAGS_NO_RESIZE | PL_UI_WINDOW_FLAGS_NO_COLLAPSE | PL_UI_WINDOW_FLAGS_NO_MOVE))
        return;
    
    switch(pTrade->eStep)
    {
        case TRADE_STEP_SELECT_PLAYER:
        {
            gptUi->layout_static(0.0f, 640, 1);
            gptUi->text("Select a player to trade with:");
            gptUi->vertical_spacing();
            
            gptUi->layout_static(45.0f, 640, 1);
            
            for(uint8_t i = 0; i < pGame->uPlayerCount; i++)
            {
                if(i == pGame->uCurrentPlayerIndex)
                    continue; // skip current player
                
                if(pGame->amPlayers[i].bIsBankrupt)
                    continue; // skip bankrupt players
                
                char acButtonLabel[64];
                snprintf(acButtonLabel, sizeof(acButtonLabel), "Player %d ($%d)", i + 1, pGame->amPlayers[i].uMoney);
                
                if(gptUi->button(acButtonLabel))
                {
                    m_set_input_int(&ptAppData->tGameFlow, i + 1);
                }
            }
            
            gptUi->vertical_spacing();
            
            if(gptUi->button("Cancel"))
            {
                m_set_input_int(&ptAppData->tGameFlow, 0);
            }
            
            break;
        }
        
        case TRADE_STEP_BUILD_OFFER:
        {
            mPlayer* pTargetPlayer = &pGame->amPlayers[pTrade->uTargetPlayer];
            
            gptUi->layout_static(0.0f, 640, 1);
            gptUi->text("Trading with Player %d", pTrade->uTargetPlayer + 1);
            gptUi->separator();
            gptUi->vertical_spacing();
            
            // two column layout: your offer | their properties (you want)
            gptUi->layout_static(0.0f, 310, 2);
            
            // LEFT COLUMN: Your offer
            gptUi->text("=== You Offer ===");
            
            // RIGHT COLUMN: What you want
            gptUi->text("=== You Request ===");
            
            gptUi->layout_static(0.0f, 310, 2);
            
            // LEFT: Money offer controls
            gptUi->text("Money: $%d", pTrade->uOfferedMoney);
            
            // RIGHT: Money request controls
            gptUi->text("Money: $%d", pTrade->uRequestedMoney);
            
            gptUi->layout_static(30.0f, 150, 4);
            
            // LEFT: Money offer buttons
            if(gptUi->button("-$100##offer"))
            {
                m_set_input_int(&ptAppData->tGameFlow, 300);
            }
            
            if(gptUi->button("+$100##offer"))
            {
                m_set_input_int(&ptAppData->tGameFlow, 400);
            }
            
            // RIGHT: Money request buttons
            if(gptUi->button("-$100##request"))
            {
                m_set_input_int(&ptAppData->tGameFlow, 500);
            }
            
            if(gptUi->button("+$100##request"))
            {
                m_set_input_int(&ptAppData->tGameFlow, 600);
            }
            
            gptUi->vertical_spacing();
            
            gptUi->layout_static(0.0f, 310, 2);
            
            // LEFT: Your properties
            gptUi->text("--- Your Properties ---");
            
            // RIGHT: Their properties
            gptUi->text("--- Their Properties ---");
            
            // Property lists
            gptUi->layout_static(30.0f, 310, 2);
            
            // LEFT COLUMN: Show current player's properties
            for(uint8_t i = 0; i < pCurrentPlayer->uPropertyCount; i++)
            {
                uint8_t uPropIdx = pCurrentPlayer->auPropertiesOwned[i];
                if(uPropIdx == BANK_PLAYER_INDEX)
                    break;
                
                mProperty* pProp = &pGame->amProperties[uPropIdx];
                
                // check if in offer
                bool bInOffer = false;
                for(uint8_t j = 0; j < pTrade->uOfferedPropertyCount; j++)
                {
                    if(pTrade->auOfferedProperties[j] == uPropIdx)
                    {
                        bInOffer = true;
                        break;
                    }
                }
                
                char acLabel[64];
                if(bInOffer)
                    snprintf(acLabel, sizeof(acLabel), "[X] %s", pProp->cName);
                else
                    snprintf(acLabel, sizeof(acLabel), "[ ] %s", pProp->cName);
                
                if(gptUi->button(acLabel))
                {
                    m_set_input_int(&ptAppData->tGameFlow, 100 + i);
                }
            }
            
            // RIGHT COLUMN: Show target player's properties
            for(uint8_t i = 0; i < pTargetPlayer->uPropertyCount; i++)
            {
                uint8_t uPropIdx = pTargetPlayer->auPropertiesOwned[i];
                if(uPropIdx == BANK_PLAYER_INDEX)
                    break;
                
                mProperty* pProp = &pGame->amProperties[uPropIdx];
                
                // check if in request
                bool bInRequest = false;
                for(uint8_t j = 0; j < pTrade->uRequestedPropertyCount; j++)
                {
                    if(pTrade->auRequestedProperties[j] == uPropIdx)
                    {
                        bInRequest = true;
                        break;
                    }
                }
                
                char acLabel[64];
                if(bInRequest)
                    snprintf(acLabel, sizeof(acLabel), "[X] %s##req", pProp->cName);
                else
                    snprintf(acLabel, sizeof(acLabel), "[ ] %s##req", pProp->cName);
                
                if(gptUi->button(acLabel))
                {
                    m_set_input_int(&ptAppData->tGameFlow, 200 + i);
                }
            }
            
            gptUi->vertical_spacing();
            
            gptUi->layout_static(45.0f, 310, 2);
            
            if(gptUi->button("Send Offer"))
            {
                m_set_input_int(&ptAppData->tGameFlow, 1);
            }
            
            if(gptUi->button("Back"))
            {
                m_set_input_int(&ptAppData->tGameFlow, 0);
            }
            
            break;
        }
        
        case TRADE_STEP_AWAITING_RESPONSE:
        {
            mPlayer* pTargetPlayer = &pGame->amPlayers[pTrade->uTargetPlayer];
            
            gptUi->layout_static(0.0f, 640, 1);
            gptUi->text("Trade Offer from Player %d to Player %d", 
                pGame->uCurrentPlayerIndex + 1, 
                pTrade->uTargetPlayer + 1);
            gptUi->separator();
            gptUi->vertical_spacing();
            
            // Show what Player 1 offers
            gptUi->layout_static(0.0f, 640, 1);
            gptUi->text("Player %d offers:", pGame->uCurrentPlayerIndex + 1);
            
            if(pTrade->uOfferedMoney > 0)
            {
                gptUi->text("  $%d", pTrade->uOfferedMoney);
            }
            
            for(uint8_t i = 0; i < pTrade->uOfferedPropertyCount; i++)
            {
                mProperty* pProp = &pGame->amProperties[pTrade->auOfferedProperties[i]];
                gptUi->text("  %s", pProp->cName);
            }
            
            if(pTrade->uOfferedPropertyCount == 0 && pTrade->uOfferedMoney == 0)
            {
                gptUi->text("  (Nothing)");
            }
            
            gptUi->vertical_spacing();
            
            // Show what Player 1 wants
            gptUi->layout_static(0.0f, 640, 1);
            gptUi->text("Player %d requests:", pGame->uCurrentPlayerIndex + 1);
            
            if(pTrade->uRequestedMoney > 0)
            {
                gptUi->text("  $%d", pTrade->uRequestedMoney);
            }
            
            for(uint8_t i = 0; i < pTrade->uRequestedPropertyCount; i++)
            {
                mProperty* pProp = &pGame->amProperties[pTrade->auRequestedProperties[i]];
                gptUi->text("  %s", pProp->cName);
            }
            
            if(pTrade->uRequestedPropertyCount == 0 && pTrade->uRequestedMoney == 0)
            {
                gptUi->text("  (Nothing)");
            }
            
            gptUi->vertical_spacing();
            gptUi->separator();
            gptUi->vertical_spacing();
            
            gptUi->layout_static(45.0f, 310, 2);
            
            if(gptUi->button("Accept Trade"))
            {
                m_set_input_int(&ptAppData->tGameFlow, 1);
            }
            
            if(gptUi->button("Reject Trade"))
            {
                m_set_input_int(&ptAppData->tGameFlow, 2);
            }
            
            break;
        }
    }
    
    gptUi->end_window();
}