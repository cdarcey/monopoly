# Monopoly Game

A learning project implementing Monopoly using [Pilot Light](https://github.com/PilotLightTech/pilotlight) as the graphics engine.

## Project Goals

This project serves as a hands-on learning experience for:
- **C programming fundamentals** - memory management, structs, pointers, and proper C idioms
- **Graphics programming** - working with a graphics engine, rendering pipelines, and 2D drawing
- **Game architecture** - state machines, game loops, and event-driven programming
- **Software engineering practices** - code organization, documentation, and version control

## Features

- Full monopoly board with property management
- 2-6 player support
- Property trading system
- Auction system for unpurchased properties
- Mortgage/unmortgage mechanics
- House and hotel building
- Chance and Community Chest cards
- Jail mechanics with multiple escape options

## Build Instructions

### Prerequisites
First, clone and build **Pilot Light** (assumed to be adjacent to this repo).

### Windows
```bash
# clone & build pilot light
git clone https://github.com/PilotLightTech/pilotlight
cd pilotlight/src
build_win32.bat

# clone & build monopoly
cd ..
git clone https://github.com/cdarcey/monopoly
cd monopoly/scripts
python setup.py
python gen_build.py
cd ../src
build.bat
```

### Linux
```bash
# clone & build pilot light
git clone https://github.com/PilotLightTech/pilotlight
cd pilotlight/src
chmod +x build_linux.sh
./build_linux.sh

# clone & build monopoly
cd ..
git clone https://github.com/cdarcey/monopoly
cd monopoly/scripts
python3 setup.py
python3 gen_build.py
cd ../src
chmod +x build.sh
./build.sh
```

### MacOS
```bash
# clone & build pilot light
git clone https://github.com/PilotLightTech/pilotlight
cd pilotlight/src
chmod +x build_linux.sh
./build_linux.sh

# clone & build monopoly
cd ..
git clone https://github.com/cdarcey/monopoly
cd monopoly/scripts
python3 setup.py
python3 gen_build.py
cd ../src
chmod +x build.sh
./build.sh
```

Binaries will be in _pilotlight/out/_.

## Running

```bash
# windows
pilot_light.exe -a monopoly_app

# linux/macos
./pilot_light -a monopoly_app
```

Or press F5 if using VSCode.

## Technical Implementation

### Architecture
- **Game state management** - custom state machine handling turn phases (pre-roll, post-roll, jail, trading, auction)
- **UI system** - immediate mode GUI for game menus and overlays
- **Rendering** - 2D sprite rendering with custom shaders and texture management
- **Data structures** - efficient property ownership tracking and player state management

### Key Components
- `monopoly.h` - core game logic and data structures
- `monopoly_init.h` - board initialization and property definitions
- `app.c` - main application, rendering, and UI integration

### Graphics Pipeline
- Vulkan-backed rendering via Pilot Light
- Custom texture loading and bind group management
- 2D draw list system for dynamic UI elements

## What I Learned

- **Memory management** - proper allocation/deallocation patterns in C, avoiding leaks
- **Low-level graphics** - working with GPU buffers, shaders, and render passes
- **Complex state management** - handling turn-based game flow with multiple interactive phases
- **UI/UX design** - creating intuitive menus for complex game mechanics
- **Debugging** - using tools and techniques to track down issues in C code

## Todo

### Current Priorities
- [ ] Handle "Get Out of Jail Free" card storage/return
- [ ] Property array cleanup utilities
- [ ] Player elimination handling
- [ ] Winner screen and game statistics

### Future Enhancements (maybe?? not sure howfar this will go)
- [ ] AI opponents with difficulty levels
- [ ] Network multiplayer support
- [ ] Save/load game state
- [ ] Animated dice rolls and token movement
- [ ] Sound effects and background music
