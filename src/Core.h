#pragma once
#include <memory>

// Forward declarations (avoid including SDL2 here)
union SDL_Event;
class Scene;
class Renderer;

// ============================================================================
// OSDSYS States (from analysis at 0x00203970)
// State values match the original OSDSYS state machine
// ============================================================================
enum class State {
    SCELogo = -1,  // SCE logo (pre-boot sequence)
    Boot = 0,      // Boot animation handler (sub_202AB0)
    Menu = 1,      // Main menu handler (sub_202AB0) 
    Config = 2,    // System configuration
    Browser = 3,   // Browser handler (sub_202E00)
    Version = 4,   // Version info
    DVD = 5,        // DVD player mode
    DebugVu1Scene = 6, // Debug VU1 Scene for development
    DebugFont = 7,  // Debug font rendering test
    DebugSound = 8, // Debug sound rendering test
    DebugTexture = 9  // Debug texture rendering test
};

// ============================================================================
// FixedTimeStep - 60 Hz timing (as in original OSDSYS main loop)
// Based on sub_209EB8 loop structure
// ============================================================================
class FixedTimeStep {
public:
    static constexpr double FIXED_DT = 1.0 / 60.0; // 16.67ms per frame

    void Update();
    bool ShouldUpdate();
    double GetTime() const { return totalTime; }
    static double GetDeltaTime() { return FIXED_DT; }

private:
    uint64_t lastTime = 0;
    double accumulator = 0.0;
    double totalTime = 0.0;
};

// ============================================================================
// ProcessTransition - State machine (sub_203970)
// Implements the state switching logic from OSDSYS
// Uses trigger mechanism identical to address 0x001F0014
// ============================================================================
class ProcessTransition {
public:
    void RequestStateChange(State newState);
    State GetCurrentState() const { return currentState; }
    bool HasPendingTransition() const { return trigger != -1; }
    void ProcessStateChange();

private:
    State currentState = State::Boot;
    int trigger = -1; // Matches behavior at 0x001F0014 (-1 = no transition)
};

// ============================================================================
// MainLoopController - Main application (sub_209EB8)
// Implements the infinite loop structure from original OSDSYS
// ============================================================================
class MainLoopController {
public:
    MainLoopController();
    ~MainLoopController();

    void HandleInput(const SDL_Event& event);
    void UpdateLoop();
    void RenderFrame(Renderer& renderer);
    void RequestStateChange(State newState);

private:
    FixedTimeStep timeStep;
    ProcessTransition stateMachine;
    std::unique_ptr<Scene> currentScene;

    void LoadSceneForState(State state);
};
