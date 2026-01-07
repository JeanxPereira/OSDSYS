#pragma once
#include "Scene.h"
#include <vector>
#include "../MathTypes.h" // Para usar Vec3 se precisar

// Simula um registrador de 128 bits da Vector Unit
struct VF_Register {
    float x, y, z, w;

    VF_Register() : x(0), y(0), z(0), w(1) {}
    VF_Register(float _x, float _y, float _z, float _w) : x(_x), y(_y), z(_z), w(_w) {}
    
    // Operações vetoriais básicas
    VF_Register operator+(const VF_Register& o) const { return {x+o.x, y+o.y, z+o.z, w+o.w}; }
    VF_Register operator*(float s) const { return {x*s, y*s, z*s, w*s}; }
};

struct Matrix4x4 {
    VF_Register row[4];
};

class DebugVu1Scene : public Scene {
public:
    DebugVu1Scene();
    virtual ~DebugVu1Scene();

    virtual void OnEnter() override;
    virtual void OnExit() override;
    virtual void Update(double dt) override;
    virtual void Render(Renderer& renderer) override;
    virtual void HandleInput(const SDL_Event& event) override;

private:
    // Dados Brutos (Simulando o VIF Packet)
    Matrix4x4 m_viewMatrix;
    VF_Register m_lightColor;
    VF_Register m_lightDir;
    std::vector<VF_Register> m_rawVertices; // Vértices de entrada (Object Space)

    // Dados Processados (Saída da VU1)
    std::vector<VF_Register> m_processedVertices; // Vértices transformados (Clip Space)

    // Funções de Simulação
    void CarregarDadosHardcoded(); // Sua função "CarregarDadosDoDump"
    void ExecutarMicrocodeVU1();   // Sua função de simulação
    
    // Controle de câmera para visualização
    float m_camRotY = 0.0f;
};