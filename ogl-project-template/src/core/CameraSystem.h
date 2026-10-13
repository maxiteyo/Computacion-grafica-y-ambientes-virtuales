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
//   distancia ∈ [30, 3000]  (no traspasar el objetivo ni perderlo de vista).
//   El alto de la ventana minimizada se guarda como >= 1: aspect nunca divide
//   por cero (¿qué pasa si height llega a 0? -> se responde en set_viewport()).
//
// Práctico 06: la escena pasa al mundo en METROS (el FDM suma 5000 m de
// altura y velocidades de ~150 m/s), por lo que la proyección y la órbita se
// ajustan a esa escala: near/far cubren el vuelo (el suelo queda a 5000 m bajo
// el avión) y la distancia inicial sigue al avión de cerca (60 m) para que la
// aeronave (~9.6 m con set_scale) se vea a una talla legible en la vista de
// "chase". El zoom del mouse es proporcional a la distancia (InputHandler).
//
// Práctico 07: entra el terreno procedural con una VENTANA deslizante de radio
// ~10 km (800 celdas × 25 m = 20×20 km). Para que NUNCA se vea el borde de la
// malla, el recorte lejano debe caer dentro de la ventana: se baja kFar a 6000 m
// (alcanza para ver el suelo 5000 m abajo y unos km de relieve) y kMaxDist a
// 2000 m, de modo que lo más lejano visible desde el centro de la ventana quede
// a far + distancia (8 km) < radio (10 km), aun con el margen de recentrado.
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

    // Distancia actual de la órbita [m]: la consume InputHandler para el zoom
    // proporcional (la ganancia se escala con la distancia).
    float distance() const { return distancia_; }

    // Reajusta la órbita de una vez (yaw/pitch en radianes, distancia en m),
    // aplicando los mismos clamps que update(). Sirve para SALTAR a una vista
    // predefinida sin esperar a que el usuario orbite con el mouse (Práctico 07:
    // la "vista de terreno" reapunta al origen con una pose que mira el relieve
    // desde arriba). No toca la proyección.
    void set_orbit(float yaw, float pitch, float dist);

private:
    void rebuild_projection();
    void rebuild_view(const glm::vec3& objetivo);

    // Configuración de la proyección (Unidad VII). Escala del mundo: METROS
    // (Práctico 06: el FDM trabaja en metros, altitud 5000 m).
    static constexpr float kFovYDeg  = 45.0f;  // ángulo de visión vertical θv
    static constexpr float kNear     = 5.0f;   // no "muy chico" (precisión de Z)
    // Práctico 07: el avión vuela a 5000 m, así que el suelo queda 5 km abajo y
    // solo entra en el cuadro mirando hacia abajo. Con far = 10 km el terreno
    // aparece como una franja en la parte baja (y el borde lejano del recorte
    // cae DENTRO de la ventana de 30 km: nunca se ve el borde de la malla).
    static constexpr float kFar      = 10000.0f;
    // Clamps de la órbita (preguntas de la consigna, ver header).
    static constexpr float kPitchLimit = 0.9f; // en múltiplos de π/2 (±81°)
    static constexpr float kMinDist = 30.0f;
    // Práctico 07: distancia máx. de órbita acotada para que lo más lejano
    // visible (far + corrimiento de la cámara) siga cayendo dentro de la ventana.
    static constexpr float kMaxDist = 2000.0f;

    // Órbita inicial: vista de "chase" 3/4 atrás-arriba, a 60 m del avión
    // (con el zoom proporcional del mouse se ajusta en ~30 px de arrastre).
    int   width_  = 0;
    int   height_ = 0;
    float yaw_        = glm::radians(90.0f);  // longitud inicial: costado 3/4
    float pitch_      = glm::radians(25.0f);  // latitud inicial: vista elevada
    float distancia_  = 60.0f;                // sigue al avión de cerca
    CameraData data_{};
};