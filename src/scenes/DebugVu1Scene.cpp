#include "DebugVu1Scene.h"
#include "../Renderer.h"
#include <iostream>
#include <cmath>

DebugVu1Scene::DebugVu1Scene() {}
DebugVu1Scene::~DebugVu1Scene() {}

void DebugVu1Scene::OnEnter() {
    printf("[DebugVu1Scene] Iniciando Simulação de VU1...\n");
    
    // 1. Carrega os dados (Hardcoded do seu Hex Dump)
    CarregarDadosHardcoded();

    // 2. Executa a transformação (uma vez, ou a cada frame se quiser animar)
    ExecutarMicrocodeVU1();
}

void DebugVu1Scene::OnExit() {}

void DebugVu1Scene::Update(double dt) {
    // Apenas gira a câmera da engine para vermos o resultado 3D
    m_camRotY += (float)dt * 0.5f;
}

// =========================================================
// SIMULAÇÃO VU1 (Sua lógica aqui)
// =========================================================
void DebugVu1Scene::CarregarDadosHardcoded() {
    // 1. Matriz Decodificada do Hex (View Matrix do OSDSYS)
    m_viewMatrix.row[0] = {1.0f,  0.0f,  0.0f, -2.0f}; 
    m_viewMatrix.row[1] = {0.0f,  3.0f,  0.0f,  2.0f}; 
    m_viewMatrix.row[2] = {0.0f,  0.0f,  1.0f,  0.0f}; 
    m_viewMatrix.row[3] = {0.0f,  0.0f,  0.0f,  1.0f}; 

    // 2. Luzes
    m_lightColor = {12.306f, 12.306f, 12.306f, 128.0f}; 
    m_lightDir   = {15.383f, 15.383f, 15.383f, 128.0f};

    // 3. Geometria (Vamos criar um Cubo de teste já que o dump tinha só 1 vértice)
    m_rawVertices.clear();
    float s = 10.0f; // Tamanho
    // Frente
    m_rawVertices.push_back({-s, -s,  s, 1}); m_rawVertices.push_back({ s, -s,  s, 1});
    m_rawVertices.push_back({ s,  s,  s, 1}); m_rawVertices.push_back({-s,  s,  s, 1});
    // Trás (pra dar profundidade)
    m_rawVertices.push_back({-s, -s, -s, 1}); m_rawVertices.push_back({ s, -s, -s, 1});
    m_rawVertices.push_back({ s,  s, -s, 1}); m_rawVertices.push_back({-s,  s, -s, 1});
    
    // Conectar linhas (indices simples para wireframe)
    // (Num caso real, leríamos os vértices do vu1Memory.bin aqui)
}

void DebugVu1Scene::ExecutarMicrocodeVU1() {
    printf("[VU1] Executando Microcode...\n");
    m_processedVertices.clear();

    // Registradores VF virtuais
    VF_Register vf[32];

    // Carga DMA (Simulada)
    vf[9]  = m_viewMatrix.row[0];
    vf[10] = m_viewMatrix.row[1];
    vf[11] = m_viewMatrix.row[2];
    vf[12] = m_viewMatrix.row[3];
    vf[20] = m_lightColor;
    vf[21] = m_lightDir;

    // Loop de Processamento (XGKICK simulado)
    for (const auto& vin : m_rawVertices) {
        VF_Register acc; // Acumulador

        // Transformação Matricial (MULAx + MADDay + MADNaz + MADDw)
        // Basicamente: Resultado = Matriz * Vetor
        // Nota: O PS2 faz operações Row-Major nativamente em muitos casos
        
        acc.x = (vf[9].x  * vin.x) + (vf[10].x  * vin.y) + (vf[11].x  * vin.z) + (vf[12].x  * vin.w);
        acc.y = (vf[9].y  * vin.x) + (vf[10].y  * vin.y) + (vf[11].y  * vin.z) + (vf[12].y  * vin.w);
        acc.z = (vf[9].z  * vin.x) + (vf[10].z  * vin.y) + (vf[11].z  * vin.z) + (vf[12].z  * vin.w);
        acc.w = (vf[9].w  * vin.x) + (vf[10].w  * vin.y) + (vf[11].w  * vin.z) + (vf[12].w  * vin.w);

        // Armazena o resultado
        m_processedVertices.push_back(acc);
        
        // Debug Log
        // printf("V_IN(%.1f, %.1f, %.1f) -> V_OUT(%.1f, %.1f, %.1f)\n", 
        //       vin.x, vin.y, vin.z, acc.x, acc.y, acc.z);
    }
}

void DebugVu1Scene::Render(Renderer& renderer) {
    // Configura uma câmera na engine para visualizar o resultado 3D
    renderer.SetCamera(Vec3(0, 0, -100), Vec3(0, 0, 0));
    
    // Desenha eixos para referência
    renderer.DrawDebugAxis(50.0f);

    // Desenha os vértices PROCESSADOS pela sua "VU1"
    // Vamos desenhar linhas amarelas conectando os pontos
    Color c(1.0f, 1.0f, 0.0f, 1.0f);

    if (m_processedVertices.size() >= 2) {
        for (size_t i = 0; i < m_processedVertices.size(); i++) {
            VF_Register p = m_processedVertices[i];
            
            // Desenha um ponto onde o vértice está
            renderer.DrawSphere(Vec3(p.x, p.y, p.z), 1.0f, c, 4);
            
            // Desenha linha para o próximo (só para visualizar a forma)
            if (i < m_processedVertices.size() - 1) {
                VF_Register next = m_processedVertices[i+1];
                renderer.DrawLine(Vec3(p.x, p.y, p.z), Vec3(next.x, next.y, next.z), c);
            }
        }
    }
}

void DebugVu1Scene::HandleInput(const SDL_Event& event) {}