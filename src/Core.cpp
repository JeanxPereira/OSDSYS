#include "Platform.h"  // MUST BE FIRST
#include "Core.h"
#include "scenes/Scene.h"
#include "scenes/SCELogoScene.h"
#include "scenes/BootScene.h"
#include "scenes/MenuScene.h"
#include "scenes/BrowserScene.h"
#include "scenes/DebugVu1Scene.h"
#include "scenes/DebugFontScene.h"
#include "scenes/DebugSoundScene.h"
#include "scenes/DebugTextureScene.h"

// ============================================================================
// FixedTimeStep Implementation
// ============================================================================
void FixedTimeStep::Update() {
    uint64_t now = SDL_GetPerformanceCounter();
    double frameTime = (now - lastTime) / (double)SDL_GetPerformanceFrequency();
    lastTime = now;

    // Clamp to prevent spiral of death
    if (frameTime > 0.25) {
        frameTime = 0.25;
    }

    accumulator += frameTime;
    totalTime += frameTime;
}

bool FixedTimeStep::ShouldUpdate() {
    if (accumulator >= FIXED_DT) {
        accumulator -= FIXED_DT;
        return true;
    }
    return false;
}

// ============================================================================
// ProcessTransition Implementation (sub_203970)
// State machine matching original OSDSYS behavior
// ============================================================================
void ProcessTransition::RequestStateChange(State newState) {
    // Only accept new transition if no pending transition
    // Matches behavior at 0x0020A984
    if (trigger == -1) {
        trigger = (int)newState;
        printf("[ProcessTransition] State change requested: %d -> %d\n", 
               (int)currentState, (int)newState);
    }
}

void ProcessTransition::ProcessStateChange() {
    // Process transition (matches sub_203970 logic)
    if (trigger != -1) {
        currentState = (State)trigger;
        trigger = -1; // Reset trigger (as in original at 0x00203970)
        printf("[ProcessTransition] State changed to: %d\n", (int)currentState);
    }
}

// ============================================================================
// MainLoopController Implementation (sub_209EB8)
// ============================================================================
MainLoopController::MainLoopController() {
    printf("[MainLoopController] Initialized (sub_209EB8)\n");
    // LoadSceneForState(State::SCELogo);
    LoadSceneForState(State::DebugTexture);
}

MainLoopController::~MainLoopController() {
    printf("[MainLoopController] Destroyed\n");
}

void MainLoopController::HandleInput(const SDL_Event& event) {
    // Pass input to current scene
    if (currentScene) {
        currentScene->HandleInput(event);
    }

    // Debug: State switching with F-keys
    if (event.type == SDL_KEYDOWN) {
        switch (event.key.keysym.sym) {
            case SDLK_F1:
                printf("[Input] F1 pressed - switching to Boot\n");
                RequestStateChange(State::Boot);
                break;
            case SDLK_F2:
                printf("[Input] F2 pressed - switching to Menu\n");
                RequestStateChange(State::Menu);
                break;
            case SDLK_F3:
                printf("[Input] F3 pressed - switching to Browser\n");
                RequestStateChange(State::Browser);
                break;
            case SDLK_F4:
                printf("[Input] F4 pressed - switching to SCE Logo\n");
                RequestStateChange(State::SCELogo);
                break;
            case SDLK_F5:
                printf("[Input] F5 pressed - switching to Debug Font\n");
                RequestStateChange(State::DebugFont);
                break;
            case SDLK_F6:
                printf("[Input] F6 pressed - switching to Debug Sound\n");
                RequestStateChange(State::DebugSound);
                break;
            case SDLK_F7:
                printf("[Input] F7 pressed - switching to Debug Texture\n");
                RequestStateChange(State::DebugTexture);
                break;
        }
    }
}

void MainLoopController::UpdateLoop() {
    // Update timing
    timeStep.Update();

    // Fixed timestep update (matches loop at 0x0020A980)
    while (timeStep.ShouldUpdate()) {
        // Process state transitions (0x0020A984 - state change check)
        if (stateMachine.HasPendingTransition()) {
            stateMachine.ProcessStateChange();
            LoadSceneForState(stateMachine.GetCurrentState());
        }

        // Update current scene
        if (currentScene) {
            currentScene->Update(FixedTimeStep::GetDeltaTime());
            
            // Check if scene requested a state transition
            if (currentScene->requestedNextState != -1) {
                State requestedState = (State)currentScene->requestedNextState;
                currentScene->requestedNextState = -1; // Reset
                RequestStateChange(requestedState);
            }
        }
    }
}

void MainLoopController::RenderFrame(Renderer& renderer) {
    if (currentScene) {
        currentScene->Render(renderer);
    }
}

void MainLoopController::RequestStateChange(State newState) {
    stateMachine.RequestStateChange(newState);
}

void MainLoopController::LoadSceneForState(State state) {
    // Exit current scene
    if (currentScene) {
        currentScene->OnExit();
    }

    // Load new scene based on state
    // Matches the switch statement at 0x00203970
    switch (state) {
        case State::DebugVu1Scene:
            printf("[LoadScene] Loading DebugVu1Scene...\n");
            currentScene = std::make_unique<DebugVu1Scene>();
            break;
        case State::DebugFont:
            printf("[LoadScene] Loading DebugFontScene...\n");
            currentScene = std::make_unique<DebugFontScene>();
            break;
        case State::DebugSound:
            printf("[LoadScene] Loading DebugSoundScene...\n");
            currentScene = std::make_unique<DebugSoundScene>();
            break;
        case State::DebugTexture:
            printf("[LoadScene] Loading DebugTextureScene...\n");
            currentScene = std::make_unique<DebugTextureScene>();
            break;
        case State::SCELogo:
            printf("[LoadScene] Loading SCELogoScene (pre-boot)...\n");
            currentScene = std::make_unique<SCELogoScene>();
            break;
         case State::Boot:
            printf("[LoadScene] Loading BootScene (handler: sub_202AB0)...\n");
            currentScene = std::make_unique<BootScene>();
            break;
        case State::Menu:
            printf("[LoadScene] Loading MenuScene (handler: sub_24F3E0)...\n");
            currentScene = std::make_unique<MenuScene>();
            break;
        case State::Config:
            printf("[LoadScene] Loading ConfigScene (not implemented)...\n");
            currentScene = std::make_unique<MenuScene>();  // TODO: ConfigScene
            break;
        case State::Browser:
            printf("[LoadScene] Loading BrowserScene (handler: sub_23FFA8)...\n");
            currentScene = std::make_unique<BrowserScene>();
            break;
        case State::Version:
            printf("[LoadScene] Loading VersionScene (not implemented)...\n");
            currentScene = std::make_unique<MenuScene>();  // TODO: VersionScene
            break;
        default:
            printf("[LoadScene] Unknown state: %d\n", (int)state);
            currentScene = nullptr;
            return;
    }

    // Enter new scene
    if (currentScene) {
        currentScene->OnEnter();
    }
}
