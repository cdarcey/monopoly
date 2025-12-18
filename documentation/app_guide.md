# Monopoly app.c - Complete Guide

## Table of Contents
1. [High-Level Overview](#high-level-overview)
2. [File Structure](#file-structure)
3. [The Lifecycle](#the-lifecycle)
4. [Data Structures](#data-structures)
5. [Rendering Pipeline](#rendering-pipeline)
6. [Input System](#input-system)
7. [Helper Functions](#helper-functions)
8. [Common Patterns](#common-patterns)

---

## High-Level Overview

This file is the **bridge** between Monopoly game logic and Pilot Light's rendering system. Think of it as:
- Monopoly game = the brain (rules, turn logic, properties)
- app.c = the eyes and hands (display, input)
- Pilot Light = the body (window, graphics, framework)

**Key Concept:** The game runs in a **non-blocking loop** at a target 60 FPS. Every frame:
1. Handle input (check keyboard)
2. Update game state (run current phase)
3. Render everything (board, pieces, UI)
4. Present to screen

---

## File Structure

### [SECTION] includes
```c
#include "pl.h"          // Pilot Light core
#include "pl_draw_ext.h" // 2D drawing (shapes, text)
#include "m_init_game.h" // Your Monopoly game
```
**What it does:** Pulls in all the APIs you need. Pilot Light uses an extension system, so each feature is a separate header.

---

### [SECTION] helper macros

```c
#define PL_COLOR_32(r, g, b, a) (...)
```
**What it does:** Converts RGBA floats (0.0-1.0) to a packed 32-bit integer color.
- Example: `PL_COLOR_32(1.0f, 0.0f, 0.0f, 1.0f)` = red
- Format: RGBA packed as bytes (0-255 each)

**Why:** Pilot Light's drawing API expects colors as `uint32_t`, not separate float channels.

---

### [SECTION] forward declarations

```c
typedef struct _plAppData plAppData;
void render_board_background(plAppData* ptAppData);
// ... etc
```
**What it does:** Declares functions before they're implemented so they can call each other.

**Why:** C requires you to declare functions before using them. Forward declarations let you organize code logically without worrying about order.

---

### [SECTION] structs

```c
typedef struct _plAppData
{
    // Pilot Light resources
    plWindow*      ptWindow;    // The window
    plDrawList2D*  ptDrawlist;  // Container for all draw commands
    plDrawLayer2D* ptLayer;     // Organizes draw commands

    // Monopoly game state
    mGameData* pGameData;       // Properties, players, board state
    mGameFlow  tGameFlow;       // Phase system (turn management)

    // Board rendering data
    float fBoardX;              // Board position on screen
    float fBoardY;
    float fBoardSize;           // 700x700 pixels
    float fSquareSize;          // 60x60 per square

    // ... etc
} plAppData;
```

**What it does:** Holds ALL application state in one struct.

**Why this pattern:**
- Easy to pass around (just one pointer)
- Survives hot reloads (code changes without restarting)
- Clean separation: rendering data + game data

**Key members:**
- `ptDrawlist/ptLayer`: Where you submit drawing commands
- `pGameData`: Your entire Monopoly game state
- `tGameFlow`: Manages turn phases (pre-roll, post-roll, jail, etc.)

---

### [SECTION] global api pointers

```c
const plIOI*     gptIO     = NULL;  // Input/output
const plDrawI*   gptDraw   = NULL;  // Drawing functions
const plStarterI* gptStarter = NULL; // Device/swapchain
// ... etc
```

**What it does:** Function pointer tables that give you access to Pilot Light APIs.

**Why globals:** Pilot Light uses a plugin system. These APIs come from DLLs that can be hot-reloaded. Globals make them accessible everywhere without passing them around.

**Usage example:**
```c
gptDraw->add_rect_filled(...);  // Call a drawing function
gptIO->is_key_pressed(...);     // Check keyboard input
```

---

## The Lifecycle

### 1. pl_app_load() - Initialization

```c
PL_EXPORT void*
pl_app_load(plApiRegistryI* ptApiRegistry, plAppData* ptAppData)
```

**When called:**
- Once at startup
- Every time code is hot-reloaded (DLL recompiled)

**Two paths:**

#### Hot Reload Path (ptAppData != NULL)
```c
if(ptAppData)  // We already have app data from before
{
    // Just re-fetch API pointers (they moved in memory)
    gptIO = pl_get_api_latest(ptApiRegistry, plIOI);
    return ptAppData;  // Keep existing game state
}
```
**Why:** When you recompile, the DLL moves in memory. APIs need to be re-fetched, but game state stays intact.

#### First Load Path (ptAppData == NULL)
```c
// 1. Allocate app data
ptAppData = malloc(sizeof(plAppData));

// 2. Load extensions
ptExtensionRegistry->load("pl_platform_ext", ...);

// 3. Get APIs
gptIO = pl_get_api_latest(ptApiRegistry, plIOI);

// 4. Create window
gptWindows->create(tWindowDesc, &ptAppData->ptWindow);

// 5. Initialize starter (device, swapchain)
gptStarter->initialize(tStarterInit);

// 6. Initialize drawing
ptAppData->ptDrawlist = gptDraw->request_2d_drawlist();
ptAppData->ptLayer = gptDraw->request_2d_layer(ptAppData->ptDrawlist);

// 7. Initialize Monopoly game
ptAppData->pGameData = m_init_game(tGameSettings);

// 8. Setup board layout
ptAppData->fBoardSize = 700.0f;

return ptAppData;
```

**Key concept:** You return `ptAppData` and Pilot Light passes it back to you every frame.

---

### 2. pl_app_update() - Main Loop

```c
PL_EXPORT void
pl_app_update(plAppData* ptAppData)
```

**When called:** Every frame (~60 FPS)

**Flow:**
```c
// 1. Begin frame (handle window events, prepare for rendering)
if(!gptStarter->begin_frame())
    return;  // Skip this frame (e.g., window minimized)

// 2. Get IO (input, time, viewport size)
plIO* ptIO = gptIO->get_io();

// 3. Handle input
handle_keyboard_input(ptAppData);

// 4. Update game logic (NON-BLOCKING)
m_run_current_phase(&ptAppData->tGameFlow, ptIO->fDeltaTime);

// 5. Render everything
render_board_background(ptAppData);
render_board_squares(ptAppData);
render_player_pieces(ptAppData);
render_game_ui(ptAppData);

// 6. Submit draw commands
gptDraw->submit_2d_layer(ptAppData->ptLayer);

// 7. Render to screen
plRenderEncoder* ptEncoder = gptStarter->begin_main_pass();
gptDrawBackend->submit_2d_drawlist(...);
gptStarter->end_main_pass();

// 8. End frame (present to screen)
gptStarter->end_frame();
```

**Critical:** This runs 60 times per second. Nothing can block here (no scanf, no infinite loops).

---

### 3. pl_app_shutdown() - Cleanup

```c
PL_EXPORT void
pl_app_shutdown(plAppData* ptAppData)
```

**When called:** Once when app closes

**Flow:**
```c
// 1. Flush GPU (wait for all rendering to finish)
gptGfx->flush_device(ptDevice);

// 2. Free game data
m_free_game_data(ptAppData->pGameData);

// 3. Cleanup Pilot Light
gptStarter->cleanup();
gptWindows->destroy(ptAppData->ptWindow);

// 4. Free app data
free(ptAppData);
```

---

### 4. pl_app_resize() - Window Resize

```c
PL_EXPORT void
pl_app_resize(plWindow* ptWindow, plAppData* ptAppData)
```

**When called:** Whenever window is resized

**What it does:** Tells starter extension to recreate the swapchain for new size.

---

## Data Structures

### plAppData - The Master Struct

**Pilot Light Resources:**
- `ptWindow`: The OS window
- `ptDrawlist`: Accumulates all drawing commands for this frame
- `ptLayer`: Organizes draw commands (you can have multiple layers for depth sorting)

**Monopoly Game State:**
- `pGameData`: Everything about the game (properties, players, dice, cards)
- `tGameFlow`: The phase system (tracks current turn phase, handles input)

**Rendering Constants:**
- `fBoardX, fBoardY`: Top-left corner of board (pixels)
- `fBoardSize`: 700x700 pixels
- `fSquareSize`: Each property is 60x60 pixels

---

## Rendering Pipeline

### How Drawing Works

1. **Request resources** (once in pl_app_load):
```c
ptAppData->ptDrawlist = gptDraw->request_2d_drawlist();
ptAppData->ptLayer = gptDraw->request_2d_layer(ptAppData->ptDrawlist);
```

2. **Draw to layer** (every frame):
```c
gptDraw->add_rect_filled(ptAppData->ptLayer, pos, size, options);
gptDraw->add_circle_filled(ptAppData->ptLayer, center, radius, segments, options);
```

3. **Submit layer** (batches draw commands):
```c
gptDraw->submit_2d_layer(ptAppData->ptLayer);
```

4. **Render to GPU**:
```c
gptDrawBackend->submit_2d_drawlist(ptAppData->ptDrawlist, ...);
```

**Analogy:** 
- Layer = canvas
- Drawing functions = paint on canvas
- Submit = send canvas to printer
- Backend = printer converts to GPU commands

---

### render_board_background()

```c
void render_board_background(plAppData* ptAppData)
{
    // Full screen dark green
    gptDraw->add_rect_filled(ptAppData->ptLayer, 
        (plVec2){0, 0}, 
        ptIO->tMainViewportSize, 
        (plDrawSolidOptions){.uColor = PL_COLOR_32(0.0f, 0.4f, 0.2f, 1.0f)});
    
    // Board area light green
    gptDraw->add_rect_filled(ptAppData->ptLayer, 
        tBoardPos, 
        tBoardSize, 
        (plDrawSolidOptions){.uColor = PL_COLOR_32(0.5f, 0.7f, 0.5f, 1.0f)});
}
```

**What it does:** Draws two rectangles (background color, board area)

**Key concepts:**
- `plVec2`: 2D vector (x, y)
- `plDrawSolidOptions`: Options struct with color
- Rectangles are drawn from top-left to bottom-right corners

---

### render_board_squares()

```c
void render_board_squares(plAppData* ptAppData)
{
    // Bottom row (GO to Just Visiting)
    for(uint32_t i = 0; i <= 10; i++)
    {
        // Position square on bottom edge, right to left
        float fX = fBoardX + fBoardSize - (i * fSquareSize);
        float fY = fBoardY + fBoardSize - fSquareSize;
        
        // Draw white square
        gptDraw->add_rect_filled(...);
        
        // Draw black border
        gptDraw->add_rect(...);
    }
    
    // Repeat for left column, top row, right column
}
```

**Board layout:**
```
20---------30 (top)
|           |
|  CENTER   |
|           |
10---------0  (bottom)
```

**Numbering:**
- 0: GO (bottom-right)
- 10: Just Visiting (bottom-left)
- 20: Free Parking (top-left)
- 30: Go To Jail (top-right)

**Why the math:**
- Bottom row: `fBoardSize - (i * fSquareSize)` = start from right, move left
- Left column: `fBoardSize - fSquareSize - (i * fSquareSize)` = start from bottom, move up
- Top row: `i * fSquareSize` = start from left, move right
- Right column: `i * fSquareSize` = start from top, move down

---

### render_player_pieces()

```c
void render_player_pieces(plAppData* ptAppData)
{
    for(uint8_t i = 0; i < pGame->uStartingPlayerCount; i++)
    {
        mPlayer* pPlayer = pGame->mGamePlayers[i];
        
        if(pPlayer->bBankrupt)
            continue;  // Don't draw bankrupt players
        
        // Convert board position (0-39) to screen coordinates
        plVec2 tScreenPos = board_position_to_screen(pPlayer->uPosition);
        
        // Offset multiple players on same square
        float fOffsetX = (i % 2) * 15.0f;  // 0 or 15 pixels
        float fOffsetY = (i / 2) * 15.0f;  // 0, 15, or 30 pixels
        
        // Draw colored circle
        gptDraw->add_circle_filled(ptAppData->ptLayer, 
            tScreenPos, 
            12.0f,  // radius
            16,     // segments (higher = smoother)
            (plDrawSolidOptions){.uColor = uColor});
        
        // Draw black outline
        gptDraw->add_circle(ptAppData->ptLayer, ...);
    }
}
```

**Offset logic:** Arranges players in a 2x2 grid on same square:
```
Player 0: (0, 0)    Player 1: (15, 0)
Player 2: (0, 15)   Player 3: (15, 15)
```

---

### render_game_ui()

**Structure:**
```c
// 1. UI panel background (dark gray rectangle)
gptDraw->add_rect_filled(...);

// 2. Panel border (white outline)
gptDraw->add_rect(...);

// 3. Current player indicator (colored square)
gptDraw->add_rect_filled(...);

// 4. Menu area (if waiting for input)
if(m_is_waiting_input(&ptAppData->tGameFlow))
{
    // Draw menu background
    gptDraw->add_rect_filled(...);
    
    // Draw menu items (5 rectangles)
    for(uint32_t i = 0; i < 5; i++)
    {
        gptDraw->add_rect_filled(...);
    }
}

// 5. Log area (bottom of panel)
gptDraw->add_rect_filled(...);
```

**Layout:**
```
┌─────────────────────┐
│ [■] Current Player  │ ← Player indicator
│                     │
│ ┌─────────────────┐ │
│ │  1. View Status │ │ ← Menu items
│ │  2. View Props  │ │
│ │  3. Management  │ │
│ │  4. Trade       │ │
│ │  5. Roll Dice   │ │
│ └─────────────────┘ │
│                     │
│ ┌─────────────────┐ │
│ │   Game Log      │ │ ← Recent actions
│ └─────────────────┘ │
└─────────────────────┘
```

---

### render_property_popup()

```c
void render_property_popup(plAppData* ptAppData, mPropertyName eProperty)
{
    // 1. Semi-transparent overlay (dims background)
    gptDraw->add_rect_filled(..., uOverlay);
    
    // 2. White popup window (centered)
    gptDraw->add_rect_filled(..., uWhite);
    
    // 3. Black border
    gptDraw->add_rect(..., uBlack);
    
    // TODO: Add property details (name, price, rent, etc.)
}
```

**Centering math:**
```c
float fPopupX = (screenWidth - popupWidth) * 0.5f;
float fPopupY = (screenHeight - popupHeight) * 0.5f;
```

---

## Input System

### handle_keyboard_input()

```c
void handle_keyboard_input(plAppData* ptAppData)
{
    // Only check input if game is waiting for it
    if(!m_is_waiting_input(&ptAppData->tGameFlow))
        return;
    
    // Check each number key
    if(gptIO->is_key_pressed(PL_KEY_1, false))
        m_set_input_int(&ptAppData->tGameFlow, 1);
    
    // ... keys 2-9
    
    // Escape closes popup
    if(gptIO->is_key_pressed(PL_KEY_ESCAPE, false))
        ptAppData->bShowPropertyPopup = false;
}
```

**Key concepts:**
- `is_key_pressed(key, repeat)`: Returns true if key was just pressed
  - `repeat = false`: Only trigger once per press
  - `repeat = true`: Trigger continuously while held
- `m_set_input_int()`: Sends input to game phase system
- Input is POLLED (checked every frame), not EVENT-DRIVEN

**Flow:**
1. User presses '5'
2. `is_key_pressed(PL_KEY_5, false)` returns true
3. Call `m_set_input_int(&tGameFlow, 5)`
4. Phase system receives input, processes choice
5. Next frame: Phase has completed or moved to next state

---

## Helper Functions

### board_position_to_screen()

```c
plVec2 board_position_to_screen(uint32_t uBoardPosition)
{
    // Convert Monopoly board position (0-39) to pixel coordinates
    
    if(uBoardPosition <= 10)        // Bottom row
    {
        float fX = fBoardX + fBoardSize - (uBoardPosition * fSquareSize) - fSquareSize/2;
        float fY = fBoardY + fBoardSize - fSquareSize/2;
        return (plVec2){fX, fY};
    }
    else if(uBoardPosition < 20)    // Left column
    {
        // ... similar math
    }
    else if(uBoardPosition <= 30)   // Top row
    {
        // ... similar math
    }
    else                            // Right column
    {
        // ... similar math
    }
}
```

**Math breakdown for bottom row:**
- Start at right: `fBoardX + fBoardSize`
- Move left: `- (uBoardPosition * fSquareSize)`
- Center on square: `- fSquareSize/2`

**Example:**
- Position 0 (GO): Right-most square
- Position 5: 5 squares to the left
- Position 10: Left-most square (Just Visiting)

---

### get_player_color()

```c
uint32_t get_player_color(mPlayerPiece ePiece)
{
    switch(ePiece)
    {
        case RACE_CAR:  return PL_COLOR_32(1.0f, 0.0f, 0.0f, 1.0f);  // Red
        case TOP_HAT:   return PL_COLOR_32(0.0f, 0.0f, 1.0f, 1.0f);  // Blue
        case THIMBLE:   return PL_COLOR_32(0.0f, 1.0f, 0.0f, 1.0f);  // Green
        // ... etc
        default:        return PL_COLOR_32(1.0f, 1.0f, 1.0f, 1.0f);  // White
    }
}
```

**Simple lookup:** Maps piece type to color for rendering.

---

## Common Patterns

### Pattern 1: Option Structs

Pilot Light uses option structs instead of long parameter lists:

```c
// OLD style (many parameters):
add_rect(layer, pos, size, color, thickness, segments);

// NEW style (option struct):
add_rect(layer, pos, size, (plDrawLineOptions){
    .uColor = uBlack,
    .fThickness = 2.0f
});
```

**Benefits:**
- Named parameters (clear what each value means)
- Defaults (unspecified fields = 0)
- Extensible (add new options without breaking old code)

---

### Pattern 2: Const Colors

Define commonly-used colors once:

```c
const uint32_t uWhite = PL_COLOR_32(1.0f, 1.0f, 1.0f, 1.0f);
const uint32_t uBlack = PL_COLOR_32(0.0f, 0.0f, 0.0f, 1.0f);

// Use throughout function
gptDraw->add_rect_filled(..., (plDrawSolidOptions){.uColor = uWhite});
```

**Why:** Saves typing, easy to change, more readable.

---

### Pattern 3: Guard Clauses

Exit early from functions when conditions aren't met:

```c
void handle_keyboard_input(plAppData* ptAppData)
{
    // Guard clause - exit if not waiting for input
    if(!m_is_waiting_input(&ptAppData->tGameFlow))
        return;
    
    // Main logic only runs if we get past guard
    if(gptIO->is_key_pressed(PL_KEY_1, false))
        m_set_input_int(&ptAppData->tGameFlow, 1);
}
```

**Why:** Reduces nesting, makes flow clearer.

---

### Pattern 4: Hot Reload Support

Always check if ptAppData exists:

```c
PL_EXPORT void* pl_app_load(plApiRegistryI* ptApiRegistry, plAppData* ptAppData)
{
    if(ptAppData)  // HOT RELOAD
    {
        // Re-fetch APIs only
        return ptAppData;
    }
    
    // FIRST LOAD
    // Full initialization
}
```

**Why:** Lets you change code and recompile without restarting the game.

---

## Quick Reference

### Drawing Functions
```c
// Filled shapes
add_rect_filled(layer, topLeft, bottomRight, options)
add_circle_filled(layer, center, radius, segments, options)
add_triangle_filled(layer, p1, p2, p3, options)

// Outlines
add_rect(layer, topLeft, bottomRight, options)
add_circle(layer, center, radius, segments, options)
add_line(layer, p1, p2, options)

// Text (requires font)
add_text(layer, pos, text, options)
```

### Input Functions
```c
is_key_pressed(key, repeat)     // Just pressed?
is_key_down(key)                // Currently held?
is_key_released(key)            // Just released?
```

### Game Flow Functions
```c
m_run_current_phase(&tGameFlow, fDeltaTime)  // Run current turn phase
m_is_waiting_input(&tGameFlow)               // Waiting for user input?
m_set_input_int(&tGameFlow, value)           // Send integer input
m_set_input_string(&tGameFlow, text)         // Send string input
```

---

## Summary

**app.c is the glue layer:**
- Takes input from keyboard → sends to game logic
- Gets game state → renders to screen
- Runs at 60 FPS, never blocks
- Survives hot reloads (code changes without restart)

**Key mental model:**
```
┌──────────────────────────────────────┐
│         Every Frame (16ms)           │
├──────────────────────────────────────┤
│ 1. Check keyboard                    │
│ 2. Update game (run current phase)   │
│ 3. Draw board                         │
│ 4. Draw pieces                        │
│ 5. Draw UI                            │
│ 6. Submit to GPU                      │
│ 7. Present to screen                  │
└──────────────────────────────────────┘
```

**Most important functions:**
- `pl_app_load()`: Initialize once
- `pl_app_update()`: Run every frame
- `render_*()`: Draw parts of the game
- `handle_keyboard_input()`: Route input to game

That's it! You now have a complete, real-time, graphical Monopoly game running in a professional rendering framework.