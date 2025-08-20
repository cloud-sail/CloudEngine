# Cloud Engine
A custom C++ game Engine.

## Core Features
- Renderer System
  - DirectX 11 Renderer (Bindful/Slot-based Rendering)
  - DirectX 12 Renderer (Bindless Rendering with HLSL Dynamic Resources)
  - DX Shader Compiler
  - Bitmap font
  - Sprite sheet and sprite animation
  - Debug Render help functions
- Asset Loading System
  - Images (stb_image)
  - XML (TinyXML2)
  - OBJ Loader
- Network System using Winsock2
- Hierarchical Scalable Clock System
- Input System (keyboard, mouse and xbox controller)
- Audio System using FMOD
- Event System
- Dev Console
- Dear ImGui
- 2D Physics
- Math
  - EulerAngles, Matrix, Quaternion
  - Geometric Primitives
  - Spline 2D/3D
  - Raycast 2D/3D


## Projects using this engine
- [ShaderTests](https://github.com/cloud-sail/ShaderTests) - Test experimental shaders and validate the renderer system
- [ChessDX](https://github.com/cloud-sail/ChessDX) - Chess Game with DirectX 12 Rendering, and Networked Multiplayer
- [Doomenstein](https://github.com/cloud-sail/Doomenstein) - Doom-like 3D FPS game with data-driven actors, weapons, and maps
- [Protogame3D](https://github.com/cloud-sail/Protogame3D) - 3D game template project
- [MathVisualTests](https://github.com/cloud-sail/MathVisualTests) - Visualizes and verifies engine math functions through interactive test scenes
- [MathUnitTests](https://github.com/cloud-sail/MathUnitTests) - Verifies engine math functions by unit tests
- [Libra](https://github.com/cloud-sail/Libra) - Top-down 2D tank game
- [Starship](https://github.com/cloud-sail/Starship) - Top-down 2D space shooter game

## How to use
### Build from template code
1. Clone Protegame3D
```bash
git clone --recurse-submodules https://github.com/cloud-sail/Protogame3D.git
```
2. Change Project Name
```
└───YOUR_PROJECT_NAME
    ├───Engine
    │   ├───Code
    │   └───Docs
    └───YOUR_PROJECT_NAME
        |   YOUR_PROJECT_NAME.sln
        ├───Code
        │   └───Game
        └───Run
            |   YOUR_PROJECT_NAME_Release_x64.exe
            └───...
```
3. Open `YOUR_PROJECT_NAME.sln`
- Rename Solution Name to `YOUR_PROJECT_NAME`
- In YOUR_PROJECT_NAME Property Pages 
  - Debugging->Command: `$(TargetFileName)`
  - Debugging->Working Directory: `$(SolutionDir)Run/`

## Important Notes
1. The only dependent file for Engine in Game Codes is `YOUR_PROJECT_NAME/YOUR_PROJECT_NAME/Code/Game/EngineBuildPreferences.hpp` 
```cpp
//-----------------------------------------------------------------------------------------------
// EngineBuildPreferences.hpp
//
// Defines build preferences that the Engine should use when building for this particular game.
//
// Note that this file is an exception to the rule "engine code shall not know about game code".
//	Purpose: Each game can now direct the engine via #defines to build differently for that game.
//	Downside: ALL games must now have this Code/Game/EngineBuildPreferences.hpp file.
//

//#define ENGINE_DISABLE_AUDIO	// (If uncommented) Disables AudioSystem code and fmod linkage.

#if defined(_DEBUG)
#define ENGINE_DEBUG_RENDER
#endif

//#define ENGINE_RENDER_D3D11
#define ENGINE_RENDER_D3D12
```
2. Excutable files, dll, and assets are in `YOUR_PROJECT_NAME/YOUR_PROJECT_NAME/Run` folder.
