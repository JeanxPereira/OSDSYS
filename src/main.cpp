#define OSDSYS_USE_OPENGL
#include "Platform.h"  // MUST BE FIRST
#include <GL/glew.h>    // GLEW MUST come after Platform.h but before any OpenGL calls
#include "Core.h"
#include "Renderer.h"
#include "scenes/DebugVu1Scene.h"

// ============================================================================
// Main entry point
// Implements initialization and main loop similar to sub_209EB8
// ============================================================================
int main(int argc, char** argv) {
    printf("=======================================================\n");
    printf("  OSDSYS Remake - Clean Room Implementation\n");
    printf("  Based on reverse engineering of PS2 OSDSYS\n");
    printf("=======================================================\n\n");

    // SDL initialization
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER) < 0) {
        printf("[ERROR] SDL init failed: %s\n", SDL_GetError());
        return 1;
    }

    // OpenGL 3.3 Core
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    // SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_COMPATIBILITY);

    // Create window 
    SDL_Window* window = SDL_CreateWindow(
        "OSDSYS Remake",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        1280, 896, // (640x448 = PS2 native resolution)
        SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN
    );

    if (!window) {
        printf("[ERROR] Window creation failed: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    SDL_GLContext glContext = SDL_GL_CreateContext(window);
    if (!glContext) {
        printf("[ERROR] OpenGL context creation failed: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    SDL_GL_SetSwapInterval(1); // VSync enabled (~60 Hz)

    // Initialize GLEW (must be done after OpenGL context creation)
    glewExperimental = GL_TRUE;
    GLenum glewErr = glewInit();
    if (glewErr != GLEW_OK) {
        printf("[ERROR] GLEW initialization failed: %s\n", glewGetErrorString(glewErr));
        SDL_GL_DeleteContext(glContext);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    printf("\n[System Info]\n");
    printf("  GLEW Version: %s\n", glewGetString(GLEW_VERSION));
    printf("  OpenGL Version: %s\n", glGetString(GL_VERSION));
    printf("  Renderer: %s\n", glGetString(GL_RENDERER));
    printf("  Resolution: 640x448 (PS2 native)\n");
    printf("  VSync: Enabled (60 Hz target)\n\n");

    // Initialize systems
    MainLoopController mainLoop;
    Renderer renderer;
    
    if (!renderer.Init()) {
        printf("[ERROR] Renderer initialization failed!\n");
        SDL_GL_DeleteContext(glContext);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    // Start with SCELogo scene (pre-boot)
    // Note: MainLoopController already starts at SCELogo

    printf("\n[Controls]\n");
    printf("  F1  - Boot Scene (State 0)\n");
    printf("  F2  - Menu Scene (State 1)\n");
    printf("  F3  - Browser Scene (State 3)\n");
    printf("  F4  - SCE Logo (Pre-boot)\n");
    printf("  F5  - Debug Font\n");
    printf("  F6  - Debug Sound\n");
    printf("  ESC - Quit\n\n");
    printf("=======================================================\n");
    printf("[Main Loop] Starting infinite loop (sub_209EB8)...\n\n");

    // Main loop (infinite - matches sub_209EB8 structure)
    bool running = true;
    SDL_Event event;

    while (running) {
        // 1. Input processing
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = false;
            }
            if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE) {
                running = false;
            }
            
            mainLoop.HandleInput(event);
        }

        // 2. Update logic (fixed timestep - 60 Hz)
        mainLoop.UpdateLoop();

        // 3. Render frame
        // glClearColor(0.05f, 0.05f, 0.1f, 1.0f); // Dark blue (PS2 background)
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
        mainLoop.RenderFrame(renderer);
        
        SDL_GL_SwapWindow(window);
    }

    // Cleanup
    printf("\n[Main Loop] Exiting...\n");
    renderer.Shutdown();
    SDL_GL_DeleteContext(glContext);
    SDL_DestroyWindow(window);
    SDL_Quit();

    printf("[Main] Shutdown complete\n");
    return 0;
}
