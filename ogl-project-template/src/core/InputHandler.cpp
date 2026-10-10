// ============================================================================
// InputHandler.cpp — Implementación del traductor mouse + teclado a comandos.
//
// Convención de signos del mouse (Práctico 05, "tomar la escena"): arrastrar
// hacia la derecha hace girar la órbita en un sentido y hacia arriba eleva la
// vista; arrastrar HACIA ARRIBA con el botón derecho ACERCA la cámara (zoom
// proporcional a la distancia, Práctico 06 — el mundo es en metros).
//
// Convención de las teclas (Práctico 06, "la tecla mueve la palanca/pedales"):
// ver el header — la tecla fija la VELOCIDAD de cambio del mando (rampa sobre
// un eje normalizado, escalada con el dt del cuadro) y no el valor inmediato.
// Al soltar la tecla el mando VUELVE a la posición del trim (resorte): el
// avión no queda girando solo tras un "tap".
// ============================================================================

#include "InputHandler.h"

#include <algorithm>  // std::clamp, std::max, std::min

#include <GLFW/glfw3.h>  // glfwGetKey, glfwGetCursorPos, glfwGetMouseButton

namespace {
// Acerca la variable v hacia su "casa" (home) a velocidad rate sin pasarse:
// el resorte de los mandos al soltar la tecla.
void return_to_home(float& v, float home, float rate, float dt)
{
    if (v > home)      v = std::max(home, v - rate * dt);
    else if (v < home) v = std::min(home, v + rate * dt);
}
}  // namespace

void InputHandler::update(GLFWwindow* window, float dt, float camera_distance)
{
    // 1) Mouse -> comando de cámara (Práctico 05): posición + botones.
    double mouse_x = 0.0, mouse_y = 0.0;
    glfwGetCursorPos(window, &mouse_x, &mouse_y);
    const bool orbit_btn =
        glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;
    const bool zoom_btn =
        glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS;
    poll(mouse_x, mouse_y, orbit_btn, zoom_btn, camera_cmd_, camera_distance);

    // 2) Teclado -> mandos de la aeronave (rampas con el dt del cuadro).
    update_sticks(glfwGetKey(window, GLFW_KEY_UP)    == GLFW_PRESS,
                  glfwGetKey(window, GLFW_KEY_DOWN)  == GLFW_PRESS,
                  glfwGetKey(window, GLFW_KEY_LEFT)  == GLFW_PRESS,
                  glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS,
                  glfwGetKey(window, GLFW_KEY_Q)     == GLFW_PRESS,
                  glfwGetKey(window, GLFW_KEY_E)     == GLFW_PRESS,
                  glfwGetKey(window, GLFW_KEY_W)     == GLFW_PRESS,
                  glfwGetKey(window, GLFW_KEY_S)     == GLFW_PRESS,
                  dt);
}

void InputHandler::update_sticks(bool elev_up, bool elev_down,
                                 bool ail_left, bool ail_right,
                                 bool rud_left, bool rud_right,
                                 bool thr_up, bool thr_down,
                                 float dt)
{
    // Rampas sobre los ejes normalizados (velocidad * dt). El signo sigue la
    // convención del header: la tecla mueve la palanca/pedales y la deflexión
    // es la que produce la aeronave según la tabla de la consigna:
    //   δe>0 nariz abajo | δa>0 ala derecha abajo | δr<0 nariz a la derecha.
    // La flecha emula la PALANCA física: ↑ empuja la palanca hacia adelante
    // (nariz abajo) y ↓ la trae hacia atrás (nariz arriba).
    // Mientras NO se toca una tecla de un eje, el resorte devuelve ese eje a
    // su "casa" (el trim): el avión no queda girando solo tras soltar.
    if (elev_up)   elev_ += kStickRate * dt;  // palanca adelante -> nariz abajo
    if (elev_down) elev_ -= kStickRate * dt;  // palanca atrás    -> nariz arriba
    if (ail_left)  ail_  -= kStickRate * dt;  // bastón a la izquierda
    if (ail_right) ail_  += kStickRate * dt;  // bastón a la derecha
    if (rud_left)  rud_  += kStickRate * dt;  // pedal izquierdo
    if (rud_right) rud_  -= kStickRate * dt;  // pedal derecho
    if (thr_up)    thr_  += kThrottleRate * dt;
    if (thr_down)  thr_  -= kThrottleRate * dt;

    if (!elev_up && !elev_down) return_to_home(elev_, elev_home_, kReturnRate, dt);
    if (!ail_left && !ail_right) return_to_home(ail_, ail_home_, kReturnRate, dt);
    if (!rud_left && !rud_right) return_to_home(rud_, rud_home_, kReturnRate, dt);
    if (!thr_up && !thr_down) return_to_home(thr_, thr_home_, kThrottleReturnRate, dt);

    // Acotar los ejes (los mandos no pasan sus topes).
    elev_ = std::clamp(elev_, -1.0f, 1.0f);
    ail_  = std::clamp(ail_,  -1.0f, 1.0f);
    rud_  = std::clamp(rud_,  -1.0f, 1.0f);
    thr_  = std::clamp(thr_,   0.0f, 1.0f);

    sync_controls_from_sticks();
}

void InputHandler::set_controls(const dlfdm::ControlInputs& c)
{
    controls_ = c;

    // Inicializar los ejes internos con la posición dada (p. ej. el trim de la
    // Parte 3): así el valor de controls() NO cambia al arrancar, y al tocar
    // una tecla la rampa parte de la posición actual y no de cero.
    elev_ = c.elevator >= 0.0f
                ? c.elevator / limits_.max_elevator
                : c.elevator / -limits_.min_elevator;
    ail_  = c.aileron / limits_.max_aileron;
    rud_  = c.rudder  / limits_.max_rudder;
    thr_  = c.throttle;

    // Memorizar esa posición como la "casa" del resorte: al soltar las teclas
    // los mandos vuelven a ella (el avión retoma la condición de equilibrio).
    elev_home_ = elev_;
    ail_home_  = ail_;
    rud_home_  = rud_;
    thr_home_  = thr_;
}

void InputHandler::sync_controls_from_sticks()
{
    // Eje normalizado [-1, +1] -> deflexión [rad] con los topes del modelo:
    // con el bastón al NEUTRO (eje 0) la deflexión es 0 y cada rama llega al
    // tope correspondiente (el elevador es asimétrico: −25°..+15°; alerón y
    // timón son simétricos). La potencia ya viene normalizada [0, 1].
    controls_.elevator = elev_ >= 0.0f
                             ? elev_ * limits_.max_elevator
                             : elev_ * -limits_.min_elevator;
    controls_.aileron  = ail_ * limits_.max_aileron;
    controls_.rudder   = rud_ * limits_.max_rudder;
    controls_.throttle = thr_;
}

void InputHandler::poll(double cursor_x, double cursor_y,
                        bool orbit_button, bool zoom_button,
                        CameraCommand& cmd, float camera_distance)
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
        // Arrastrar hacia ARRIBA (dy < 0) ACERCA: dist_delta negativo -> la
        // cámara reduce distancia. La ganancia es proporcional a la distancia
        // actual (mundo en metros): el zoom se siente igual en todo el rango.
        cmd.dist_delta = static_cast<float>(dy) * kZoomGain
                       * static_cast<float>(camera_distance);
    }
}