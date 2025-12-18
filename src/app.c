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
#include "pl_starter_ext.h"

// monopoly game logic
#include "m_init_game.h"

//-----------------------------------------------------------------------------
// [SECTION] helper macros
//-----------------------------------------------------------------------------

// convert rgba floats (0-1) to packed uint32_t color
#define PL_COLOR_32(r, g, b, a) ((uint32_t)((uint8_t)((a)*255.0f) << 24 | (uint8_t)((b)*255.0f) << 16 | (uint8_t)((g)*255.0f) << 8 | (uint8_t)((r)*255.0f)))

//-----------------------------------------------------------------------------
// [SECTION] forward declarations
//-----------------------------------------------------------------------------

// forward declare the app data struct
typedef struct _plAppData plAppData;

// rendering functions
void render_board_background(plAppData* ptAppData);
void render_board_squares(plAppData* ptAppData);
void render_player_pieces(plAppData* ptAppData);
void render_game_ui(plAppData* ptAppData);
void render_property_popup(plAppData* ptAppData, mPropertyName eProperty);

// input handling
void handle_keyboard_input(plAppData* ptAppData);

// helper functions
plVec2   board_position_to_screen(uint32_t uBoardPosition);
uint32_t get_player_color(mPlayerPiece ePiece);
void     load_board_textures(plAppData* ptAppData);

//-----------------------------------------------------------------------------
// [SECTION] structs
//-----------------------------------------------------------------------------

// main application data structure
typedef struct _plAppData
{
    // pilot light resources
    plWindow*      ptWindow;
    plDrawList2D*  ptDrawlist;
    plDrawLayer2D* ptLayer;

    // monopoly game state
    mGameData* pGameData;
    mGameFlow  tGameFlow;

    // board rendering data
    float fBoardX;
    float fBoardY;
    float fBoardSize;
    float fSquareSize;

    // property popup
    bool          bShowPropertyPopup;
    mPropertyName ePropertyToShow;

    // board texture
    plTextureHandle   tBoardTexture;
    plBindGroupHandle tBoardBindGroup;
    bool              bBoardTextureLoaded;
} plAppData;

//-----------------------------------------------------------------------------
// [SECTION] global api pointers
//-----------------------------------------------------------------------------

const plIOI*          gptIO          = NULL;
const plWindowI*      gptWindows     = NULL;
const plGraphicsI*    gptGfx         = NULL;
const plDrawI*        gptDraw        = NULL;
const plDrawBackendI* gptDrawBackend = NULL;
const plStarterI*     gptStarter     = NULL;

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
        gptStarter     = pl_get_api_latest(ptApiRegistry, plStarterI);
        return ptAppData;
    }

    // first load path
    ptAppData = malloc(sizeof(plAppData));
    memset(ptAppData, 0, sizeof(plAppData));

    // get extension registry
    const plExtensionRegistryI* ptExtensionRegistry = 
        pl_get_api_latest(ptApiRegistry, plExtensionRegistryI);

    // load extensions
    ptExtensionRegistry->load("pl_unity_ext", NULL, NULL, true);
    ptExtensionRegistry->load("pl_platform_ext", NULL, NULL, false);

    // get APIs
    gptIO          = pl_get_api_latest(ptApiRegistry, plIOI);
    gptWindows     = pl_get_api_latest(ptApiRegistry, plWindowI);
    gptGfx         = pl_get_api_latest(ptApiRegistry, plGraphicsI);
    gptDraw        = pl_get_api_latest(ptApiRegistry, plDrawI);
    gptDrawBackend = pl_get_api_latest(ptApiRegistry, plDrawBackendI);
    gptStarter     = pl_get_api_latest(ptApiRegistry, plStarterI);

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

    // initialize starter
    plStarterInit tStarterInit = {
        .tFlags   = PL_STARTER_FLAGS_ALL_EXTENSIONS,
        .ptWindow = ptAppData->ptWindow
    };
    gptStarter->initialize(tStarterInit);
    gptStarter->finalize();

    // initialize drawing
    ptAppData->ptDrawlist = gptDraw->request_2d_drawlist();
    ptAppData->ptLayer    = gptDraw->request_2d_layer(ptAppData->ptDrawlist);

    // initialize monopoly game
    mGameStartSettings tGameSettings = {
        .uStartingMoney       = 1500,
        .uStartingPlayerCount = 3,
        .uJailFine            = 50
    };
    ptAppData->pGameData = m_init_game(tGameSettings);

    m_set_player_piece(ptAppData->pGameData, RACE_CAR, PLAYER_ONE);
    m_set_player_piece(ptAppData->pGameData, TOP_HAT, PLAYER_TWO);
    m_set_player_piece(ptAppData->pGameData, THIMBLE, PLAYER_THREE);

    m_init_game_flow(&ptAppData->tGameFlow, ptAppData->pGameData, NULL);

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
    plDevice* ptDevice = gptStarter->get_device();
    gptGfx->flush_device(ptDevice);

    m_free_game_data(ptAppData->pGameData); // TODO: make sure this cleans all game data

    gptStarter->cleanup();
    gptWindows->destroy(ptAppData->ptWindow);

    free(ptAppData);
}

//-----------------------------------------------------------------------------
// [SECTION] pl_app_resize
//-----------------------------------------------------------------------------

PL_EXPORT void
pl_app_resize(plWindow* ptWindow, plAppData* ptAppData)
{
    gptStarter->resize();
}

//-----------------------------------------------------------------------------
// [SECTION] pl_app_update
//-----------------------------------------------------------------------------

PL_EXPORT void
pl_app_update(plAppData* ptAppData)
{
    if(!gptStarter->begin_frame()) // needs to be called at the start of every frame
        return;

    plIO* ptIO = gptIO->get_io();

    // handle input
    handle_keyboard_input(ptAppData);

    // update game
    m_run_current_phase(&ptAppData->tGameFlow, ptIO->fDeltaTime);
    m_game_over_check(ptAppData->pGameData);

    render_board_background(ptAppData);
    render_board_squares(ptAppData);
    render_player_pieces(ptAppData);
    render_game_ui(ptAppData);

    if(ptAppData->bShowPropertyPopup) // TODO: if prop clicked on popup card on screen to see all prop details easier 
    {
        render_property_popup(ptAppData, ptAppData->ePropertyToShow);
    }

    gptDraw->submit_2d_layer(ptAppData->ptLayer);

    // render to screen
    plRenderEncoder* ptEncoder = gptStarter->begin_main_pass();

    gptDrawBackend->submit_2d_drawlist(ptAppData->ptDrawlist, ptEncoder, ptIO->tMainViewportSize.x, ptIO->tMainViewportSize.y, 1);

    gptStarter->end_main_pass();
    gptStarter->end_frame();
}

//-----------------------------------------------------------------------------
// [SECTION] rendering functions
//-----------------------------------------------------------------------------

void
render_board_background(plAppData* ptAppData)
{
    plIO* ptIO = gptIO->get_io();

    // dark green background
    gptDraw->add_rect_filled(ptAppData->ptLayer, (plVec2){0, 0}, ptIO->tMainViewportSize, 
            (plDrawSolidOptions){.uColor = PL_COLOR_32(0.0f, 0.4f, 0.2f, 1.0f)});

    // board area
    plVec2 tBoardPos = {ptAppData->fBoardX, ptAppData->fBoardY};
    plVec2 tBoardSize = {ptAppData->fBoardSize, ptAppData->fBoardSize};

    gptDraw->add_rect_filled(ptAppData->ptLayer, tBoardPos, pl_add_vec2(tBoardPos, tBoardSize), 
            (plDrawSolidOptions){.uColor = PL_COLOR_32(0.5f, 0.7f, 0.5f, 1.0f)});
}

void
render_board_squares(plAppData* ptAppData)
{
    const float fSquareSize = ptAppData->fSquareSize;
    const float fBoardX     = ptAppData->fBoardX;
    const float fBoardY     = ptAppData->fBoardY;
    const float fBoardSize  = ptAppData->fBoardSize;

    const uint32_t uWhite = PL_COLOR_32(1.0f, 1.0f, 1.0f, 1.0f);
    const uint32_t uBlack = PL_COLOR_32(0.0f, 0.0f, 0.0f, 1.0f);

    // bottom row (0-10)`
    for(uint32_t i = 0; i <= 10; i++)
    {
        float fX = fBoardX + fBoardSize - (i * fSquareSize);
        float fY = fBoardY + fBoardSize - fSquareSize;

        plVec2 tPos  = {fX, fY};
        plVec2 tSize = {fSquareSize - 2, fSquareSize - 2};

        gptDraw->add_rect_filled(ptAppData->ptLayer, tPos, pl_add_vec2(tPos, tSize), (plDrawSolidOptions){.uColor = uWhite});
        gptDraw->add_rect(ptAppData->ptLayer, tPos, pl_add_vec2(tPos, tSize), (plDrawLineOptions){.uColor = uBlack, .fThickness = 2.0f});
    }

    // left column (11-19)
    for(uint32_t i = 1; i < 10; i++)
    {
        float fX = fBoardX;
        float fY = fBoardY + fBoardSize - fSquareSize - (i * fSquareSize);

        plVec2 tPos  = {fX, fY};
        plVec2 tSize = {fSquareSize - 2, fSquareSize - 2};

        gptDraw->add_rect_filled(ptAppData->ptLayer, tPos, pl_add_vec2(tPos, tSize), (plDrawSolidOptions){.uColor = uWhite});
        gptDraw->add_rect(ptAppData->ptLayer, tPos, pl_add_vec2(tPos, tSize), (plDrawLineOptions){.uColor = uBlack, .fThickness = 2.0f});
    }

    // top row (20-30)
    for(uint32_t i = 0; i <= 10; i++)
    {
        float fX = fBoardX + (i * fSquareSize);
        float fY = fBoardY;

        plVec2 tPos  = {fX, fY};
        plVec2 tSize = {fSquareSize - 2, fSquareSize - 2};

        gptDraw->add_rect_filled(ptAppData->ptLayer, tPos, pl_add_vec2(tPos, tSize), (plDrawSolidOptions){.uColor = uWhite});
        gptDraw->add_rect(ptAppData->ptLayer, tPos, pl_add_vec2(tPos, tSize), (plDrawLineOptions){.uColor = uBlack, .fThickness = 2.0f});
    }

    // right column (31-39)
    for(uint32_t i = 1; i < 10; i++)
    {
        float fX = fBoardX + fBoardSize - fSquareSize;
        float fY = fBoardY + (i * fSquareSize);

        plVec2 tPos  = {fX, fY};
        plVec2 tSize = {fSquareSize - 2, fSquareSize - 2};

        gptDraw->add_rect_filled(ptAppData->ptLayer, tPos, pl_add_vec2(tPos, tSize), (plDrawSolidOptions){.uColor = uWhite});
        gptDraw->add_rect(ptAppData->ptLayer, tPos, pl_add_vec2(tPos, tSize), (plDrawLineOptions){.uColor = uBlack, .fThickness = 2.0f});
    }

    // center area
    float fCenterX = fBoardX + fSquareSize;
    float fCenterY = fBoardY + fSquareSize;
    float fCenterSize = fBoardSize - (2 * fSquareSize);

    gptDraw->add_rect_filled(ptAppData->ptLayer, (plVec2){fCenterX, fCenterY}, (plVec2){fCenterX + fCenterSize, fCenterY + fCenterSize}, 
            (plDrawSolidOptions){.uColor = PL_COLOR_32(0.8f, 0.8f, 0.6f, 1.0f)});
}

void
render_player_pieces(plAppData* ptAppData)
{
    mGameData* pGame = ptAppData->pGameData;

    const uint32_t uBlack = PL_COLOR_32(0.0f, 0.0f, 0.0f, 1.0f);

    for(uint8_t i = 0; i < pGame->uStartingPlayerCount; i++)
    {
        mPlayer* pPlayer = pGame->mGamePlayers[i];

        if(pPlayer->bBankrupt)
            continue;

        plVec2 tScreenPos = board_position_to_screen(pPlayer->uPosition);

        float fOffsetX = (i % 2) * 15.0f;
        float fOffsetY = (i / 2) * 15.0f;
        tScreenPos.x += fOffsetX;
        tScreenPos.y += fOffsetY;

        uint32_t uColor = get_player_color(pPlayer->ePiece);

        gptDraw->add_circle_filled(ptAppData->ptLayer, tScreenPos, 12.0f, 16, (plDrawSolidOptions){.uColor = uColor});
        gptDraw->add_circle(ptAppData->ptLayer, tScreenPos, 12.0f, 16, (plDrawLineOptions){.uColor = uBlack, .fThickness = 2.0f});
    }
}

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

void
render_property_popup(plAppData* ptAppData, mPropertyName eProperty)
{
    plIO* ptIO           = gptIO->get_io();
    mGameData* pGame     = ptAppData->pGameData;
    mProperty* pProperty = &pGame->mGameProperties[eProperty];

    float fPopupWidth  = 400.0f;
    float fPopupHeight = 500.0f;
    float fPopupX      = (ptIO->tMainViewportSize.x - fPopupWidth) * 0.5f;
    float fPopupY      = (ptIO->tMainViewportSize.y - fPopupHeight) * 0.5f;

    const uint32_t uWhite   = PL_COLOR_32(1.0f, 1.0f, 1.0f, 1.0f);
    const uint32_t uBlack   = PL_COLOR_32(0.0f, 0.0f, 0.0f, 1.0f);
    const uint32_t uOverlay = PL_COLOR_32(0.0f, 0.0f, 0.0f, 0.5f);

    // overlay
    gptDraw->add_rect_filled(ptAppData->ptLayer, (plVec2){0, 0}, ptIO->tMainViewportSize, (plDrawSolidOptions){.uColor = uOverlay});

    // popup background
    gptDraw->add_rect_filled(ptAppData->ptLayer, (plVec2){fPopupX, fPopupY}, (plVec2){fPopupX + fPopupWidth, fPopupY + fPopupHeight}, 
            (plDrawSolidOptions){.uColor = uWhite});

    // popup border
    gptDraw->add_rect(ptAppData->ptLayer, (plVec2){fPopupX, fPopupY}, (plVec2){fPopupX + fPopupWidth, fPopupY + fPopupHeight}, 
            (plDrawLineOptions){.uColor = uBlack, .fThickness = 3.0f});
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

plVec2
board_position_to_screen(uint32_t uBoardPosition)
{
    // TODO: fix box alignment 
    const float fSquareSize = 60.0f;
    const float fBoardX     = 50.0f;
    const float fBoardY     = 10.0f;
    const float fBoardSize  = 700.0f;

    // bottom row (0-10)
    if(uBoardPosition <= 10)
    {
        float fX = fBoardX + fBoardSize - (uBoardPosition * fSquareSize) - fSquareSize/2;
        float fY = fBoardY + fBoardSize - fSquareSize/2;
        return (plVec2){fX, fY};
    }
    // left column (11-19)
    else if(uBoardPosition < 20)
    {
        float fX = fBoardX + fSquareSize/2;
        float fY = fBoardY + fBoardSize - fSquareSize - ((uBoardPosition - 10) * fSquareSize) - fSquareSize/2;
        return (plVec2){fX, fY};
    }
    // top row (20-30)
    else if(uBoardPosition <= 30)
    {
        float fX = fBoardX + ((uBoardPosition - 20) * fSquareSize) + fSquareSize/2;
        float fY = fBoardY + fSquareSize/2;
        return (plVec2){fX, fY};
    }
    // right column (31-39)
    else
    {
        float fX = fBoardX + fBoardSize - fSquareSize/2;
        float fY = fBoardY + ((uBoardPosition - 30) * fSquareSize) + fSquareSize/2;
        return (plVec2){fX, fY};
    }
}

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
load_board_textures(plAppData* ptAppData)
{
    // TODO: implement texture loading
}

