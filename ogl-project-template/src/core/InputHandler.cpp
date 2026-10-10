// ============================================================================
// InputHandler.cpp — Implementación del traductor mouse -> CameraCommand.
// Convención de signos ("tomar la escena"): arrastrar el mouse hacia la
// derecha hace girar la órbita en un sentido y hacia arriba eleva la vista;
// arrastrar hacia arriba con el botón derecho acerca la cámara.
// ============================================================================

#include "InputHandler.h"

void InputHandler::poll(double cursor_x, double cursor_y,
                        bool orbit_button, bool zoom_button,
                        CameraCommand& cmd)
{
    cmd = CameraCommand{};  // por defecto: nada se mueve

    // Primer cuadro: no hay posición anterior -> delta = 0 (y se evita el
    // salto inicial de la cámara al primer frame).
    if (first_) {
        first_  = false;
        last_x_ = cursor_x;
        last_y_ = cursor_y;
        return;
    }

    const double dx = cursor_x - last_x_;
    const double dy = cursor_y - last_y_;
    last_x_ = cursor_x;
    last_y_ = cursor_y;

    if (orbit_button) {
        cmd.yaw_delta   = static_cast<float>(-dx) * kOrbitGain;
        cmd.pitch_delta = static_cast<float>(-dy) * kOrbitGain;
    }
    if (zoom_button) {
        cmd.dist_delta = static_cast<float>(-dy) * kZoomGain;
    }
}