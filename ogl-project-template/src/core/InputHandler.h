// ============================================================================
// InputHandler.h — Traduce el hardware de entrada (mouse) a CameraCommand.
//
// Responsabilidad (Práctico 05): la cámara NO debe leer el mouse; este módulo
// sí. Una vez por cuadro (estrategia de polling, no eventos) convierte la
// POSICIÓN del cursor y los botones en los deltas de la órbita:
//   - Botón IZQUIERDO presionado (órbita): mover el mouse en X cambia el yaw
//     (azimut) y en Y cambia el pitch (elevación).
//   - Botón DERECHO presionado (zoom): el desplazamiento en Y cambia la
//     distancia al objetivo (tercer estado; el mouse es 2D y la órbita 3D).
//   - Sin botones: el comando queda vacío (se puede pasar el mouse por encima
//     de la ventana sin que la cámara se mueva).
//
// Qué NO hace: no conoce la cámara (no toca su estado), no decide qué dibujar.
// El desplazamiento se calcula como delta = actual - anterior; en el PRIMER
// cuadro no hay posición anterior: el delta se toma como 0 (no hay salto).
//
// Independencia de GLFW: poll() recibe valores crudos (posición y botones),
// por lo que el módulo es C++ puro y se puede testear sin ventana.
// ============================================================================
#pragma once

#include "CameraCommand.h"

class InputHandler {
public:
    InputHandler() = default;

    // Genera el comando de este cuadro a partir del estado crudo del mouse.
    //   cursor_x, cursor_y : posición del cursor (px, de glfwGetCursorPos).
    //   orbit_button        : botón izquierdo presionado (yaw/pitch).
    //   zoom_button         : botón derecho presionado (distancia).
    //   cmd                 : donde se escriben los deltas (se sobrescribe).
    void poll(double cursor_x, double cursor_y,
              bool orbit_button, bool zoom_button,
              CameraCommand& cmd);

    // Ganancia px -> rad para la órbita (0.005 rad/px ≈ 0.29°/px).
    static constexpr float kOrbitGain = 0.005f;
    // Ganancia px -> unidades de escena para el zoom (20 px ≈ 1 unidad).
    static constexpr float kZoomGain  = 0.02f;

private:
    bool   first_  = true;   // primer cuadro: sin posición anterior
    double last_x_ = 0.0;
    double last_y_ = 0.0;
};