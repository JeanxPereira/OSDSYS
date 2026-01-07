#pragma once

// Forward declaration
union SDL_Event;
class Renderer;

// ============================================================================
// Scene - Base class for all scenes
// Each OSDSYS state is a different scene
// ============================================================================
class Scene {
public:
    virtual ~Scene() = default;

    // Lifecycle
    virtual void OnEnter() {}
    virtual void OnExit() {}

    // Input
    virtual void HandleInput(const SDL_Event& event) {}

    // Update (fixed timestep - 60 Hz)
    virtual void Update(double dt) = 0;

    // Render
    virtual void Render(Renderer& renderer) = 0;

protected:
    float time = 0.0f; // Scene internal time (in seconds)
    
    // Request state transition (checked by MainLoopController)
    // Set to valid state to request transition, or -1 for none
    int requestedNextState = -1;
    
    friend class MainLoopController; // Allow access to requestedNextState
};
