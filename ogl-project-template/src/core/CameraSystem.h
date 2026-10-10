// ============================================================================
// CameraSystem.h — Administrador del estado de la cámara de la escena (órbita).
//
// Responsabilidad (Práctico 05): calcular por cuadro el par de matrices
// (view + projection) para completar el proceso de proyección. El resto del
// programa NO sabe cómo mira la cámara (órbita, cabina o fija): solo pide
//   camara.update(objetivo, angulos_avion, cmd);   // por cuadro
//   ... = camara.data().view / .projection;        // a los uniforms
// y eso permite cambiar de tipo de vista sin tocar nada fuera de este módulo.
//
// Qué NO hace este módulo:
//   - No lee el mouse: recibe los cambios de la órbita YA interpretados en
//     CameraCommand (eso es trabajo de InputHandler).
//   - No dibuja: no usa nada de OpenGL (puro GLM, testeable sin GPU).
//   - No sabe cómo está compuesta la escena: solo recibe el punto objetivo.
//
// Órbita en coordenadas esféricas alrededor del objetivo:
//   eye = objetivo + ( x, y, z )
//   x = d · cos(pitch) · cos(yaw)      y = d · sin(pitch)
//   z = d · cos(pitch) · sin(yaw)
//   view = glm::lookAt(eye, objetivo, up)
// Estado acotado (para que lookAt no se indefina ni la cámara atraviese nada):
//   pitch     ∈ [±0.9·π/2]  (si llegara a ±π/2, up y la vista son colineales
//                            y el producto vectorial interno de lookAt se anula).
//   distancia ∈ [2, 50]     (no traspasar el objetivo ni perderlo de vista).
//   El alto de la ventana minimizada se guarda como >= 1: aspect nunca divide
//   por cero (¿qué pasa si height llega a 0? -> se responde en set_viewport()).
// ============================================================================
#pragma once

#include <glm/glm.hpp>

#include "CameraCommand.h"
#include "CameraData.h"

class CameraSystem {
public:
    CameraSystem() = default;
    // Crea el estado inicial y arma la PROYECCIÓN inicial (ver set_viewport).
    CameraSystem(int width, int height);

    // Acumula los deltas del comando sobre el estado de la órbita (con clamps)
    // y recalcula la matriz de vista mirando al objetivo.
    //   objetivo     : punto al que apunta la cámara (p.ej. la posición del avión).
    //   angulos_avion: (roll, pitch, yaw) de la aeronave. Por ahora se IGNORA:
    //                  la cámara orbital no depende de la actitud del avión.
    //                  Servirá para una vista de CABINA (la orientación de la
    //                  cámara = la del avión, dada en el referencial de este).
    //   cmd          : cambios de (yaw, pitch, distancia) del usuario.
    void update(const glm::vec3& objetivo,
                const glm::vec3& angulos_avion,
                const CameraCommand& cmd);

    // Al redimensionar el framebuffer: guarda el nuevo tamaño y recalcula la
    // proyección (aspect = width/height). La llamada a glViewport la hace el
    // callback de GLFW; esta es la otra mitad.
    void set_viewport(int width, int height);

    // Par de matrices listas para los uniforms uView / uProjection.
    const CameraData& data() const { return data_; }

private:
    void rebuild_projection();
    void rebuild_view(const glm::vec3& objetivo);

    // Configuración de la proyección (Unidad VII).
    static constexpr float kFovYDeg  = 45.0f;  // ángulo de visión vertical θv
    static constexpr float kNear     = 0.5f;   // no "muy chico" (precisión de Z)
    static constexpr float kFar      = 500.0f;
    // Clamps de la órbita (preguntas de la consigna, ver header).
    static constexpr float kPitchLimit = 0.9f; // en múltiplos de π/2 (±81°)
    static constexpr float kMinDist = 2.0f;
    static constexpr float kMaxDist = 50.0f;

    int   width_  = 0;
    int   height_ = 0;
    float yaw_        = glm::radians(90.0f);  // longitud inicial: costado 3/4
    float pitch_      = glm::radians(25.0f);  // latitud inicial: vista elevada
    float distancia_  = 10.0f;
    CameraData data_{};
};