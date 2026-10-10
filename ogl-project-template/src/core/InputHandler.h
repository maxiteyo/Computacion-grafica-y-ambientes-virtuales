// ============================================================================
// InputHandler.h — Traduce el hardware de entrada (mouse + teclado) a los dos
// comandos que consume la escena: el de cámara (CameraCommand) y las
// posiciones de los mandos de la aeronave (dlfdm::ControlInputs).
//
// Responsabilidad (Práctico 06): el módulo lee UNA vez por cuadro (polling,
// no eventos) el estado de mouse y teclado y produce:
//   - camera_cmd() : deltas de la órbita (yaw/pitch/distancia), como en el
//                    Práctico 05: botón izquierdo = orbitar, derecho = zoom.
//   - controls()   : posiciones de los 4 mandos que consume el FDM:
//                    elevador, alerones, timón [rad] y potencia [0,1].
//
// Qué NO hace: no conoce la cámara ni el FDM (no integra: solo genera las
// posiciones de los mandos), no decide qué dibujar ni cuándo simular.
//
// Entradas de teclado — estrategia "estado" (polling):
//   Como el teclado es discreto y los mandos son proporcionales, la tecla fija
//   la VELOCIDAD de cambio del mando (rampa) y no el valor de la deflexión:
//     eje_ += signo * rate * dt     (eje_ normalizado; luego se mapea a
//     radianes con el tope de la superficie).
//   AL SOLTAR la tecla el mando VUELVE solo a la posición del trim ("resorte"):
//   las rampas se aflojan hacia el punto memorizado por set_controls() (si no
//   se llamó set_controls(), hacia el neutro 0). Así el avión NO queda girando
//   solo: al soltar, las superficies retornan a la condición de equilibrio y la
//   aeronave vuelve al vuelo del trim (un "tap" corto da un mando breve y
//   controlado, no una deflexión permanente).
//
// Convención registrada — "la tecla mueve la palanca/pedales" (emula la
// palanca FÍSICA: la flecha indica hacia dónde se empuja; el avión responde con
// el comando real, como al volar):
//     ↑ palanca adelante -> nariz ABAJO  -> elevador → +15° (max_elevator)
//     ↓ palanca atrás    -> nariz ARRIBA -> elevador → −25° (min_elevator)
//     ← bastón izq.   -> ala izq abajo -> alerones → −20° (min_aileron)
//     → bastón der.   -> ala der abajo -> alerones → +20° (max_aileron)
//     Q pedal izq.    -> nariz izq.    -> timón    → +30° (+max_rudder)
//     E pedal der.    -> nariz der.    -> timón    → −30° (−max_rudder)
//     W / S potencia: sube / baja (rampa en [0,1]; sin signo que analizar).
//   Los topes son los del modelo jettrainer (libs/dlfdm), por defecto; se
//   pueden inyectar con el constructor si se usara otra aeronave.
//
// Eventos (pausa, wireframe): NO van acá. Son acciones discretas (¿se acaba
// de apretar?) y se resuelven en el main con detección de borde (la consigna:
// estado vs evento).
//
// Testeabilidad: update(window, dt) es el ÚNICO punto que toca GLFW. La
// lógica de mandos (rampas + topes) vive en update_sticks(), que recibe el
// estado crudo de las teclas y es C++ puro; poll() (mouse) también es puro.
// ============================================================================
#pragma once

#include <glm/glm.hpp>

#include <dlfdm/defines.h>  // dlfdm::ControlInputs

#include "CameraCommand.h"

struct GLFWwindow;  // opaco: solo se usa como puntero (ver update)

class InputHandler {
public:
    InputHandler() = default;

    // ------------------------------------------------------------------
    // API del Práctico 06 (lo que consume el main y el FDM)
    // ------------------------------------------------------------------

    // Polling COMPLETO una vez por cuadro: lee mouse + teclado y actualiza
    // camera_cmd() y controls().  dt = duración del cuadro [s] (frame_dt) —
    // las rampas de los mandos se escalan con el tiempo real del cuadro.
    // camera_distance: distancia actual de la cámara [m] (para el zoom
    // proporcional; ver kZoomGain).
    void update(GLFWwindow* window, float dt, float camera_distance = 150.0f);

    // Posiciones actuales de los mandos (lo que consume el FDM).
    const dlfdm::ControlInputs& controls() const { return controls_; }

    // Fija las posiciones de los mandos (p. ej. la condición de equilibrio del
    // trim: ver jettrainer::get_trim_condition) e inicializa las rampas
    // internas para que coincidan (al empezar, sin tocar teclas, el avión
    // queda exactamente en el trim). Además memoriza esa posición como la
    // "casa" del resorte: al soltar cualquier tecla los mandos vuelven a ella.
    void set_controls(const dlfdm::ControlInputs& c);

    // Comando de cámara del cuadro actual (Práctico 05).
    CameraCommand camera_cmd() const { return camera_cmd_; }

    // ------------------------------------------------------------------
    // Lógica pura (sin GLFW), testeable en C++ puro
    // ------------------------------------------------------------------

    // Rampas de los mandos desde el estado crudo de 8 teclas (ver header para
    // la convención de signos). dt en [s].
    void update_sticks(bool elev_up, bool elev_down,
                       bool ail_left, bool ail_right,
                       bool rud_left, bool rud_right,
                       bool thr_up, bool thr_down,
                       float dt);

    // Mouse (Práctico 05): posición + botones -> deltas de órbita. update()
    // la usa internamente con los valores de glfwGetCursorPos. camera_distance
    // en [m] escala la ganancia de zoom (que es proporcional a la distancia).
    void poll(double cursor_x, double cursor_y,
              bool orbit_button, bool zoom_button,
              CameraCommand& cmd, float camera_distance = 150.0f);

    // Topes de las superficies de control (rad). Por defecto: jettrainer.
    struct Limits {
        float min_elevator = glm::radians(-25.0f);  // [rad] elevador (asimétrico)
        float max_elevator = glm::radians( 15.0f);  // [rad]
        float max_aileron  = glm::radians( 20.0f);  // [rad] ±
        float max_rudder   = glm::radians( 30.0f);  // [rad] ±
    };

    // Ganancias de las rampas (eje normalizado por segundo).
    static constexpr float kStickRate = 1.0f;  // [1/s] mandos (0->±1 en 1 s)
    static constexpr float kThrottleRate = 0.75f;  // [1/s] potencia (0->1 en 1,33 s)
    // Velocidad del "resorte": al soltar la tecla los mandos vuelven al trim
    // (más rápido que la rampa: recuperar el equilibrio no debe "dormirse").
    static constexpr float kReturnRate         = 2.5f;   // [1/s] mandos
    static constexpr float kThrottleReturnRate = 1.25f;  // [1/s] potencia

    // Ganancias px -> rad del mouse (Práctico 05). kZoomGain es FRACCIÓN de la
    // distancia por px (Práctico 06, mundo en METROS): el zoom se siente igual
    // de rápido a 60 m que a 2000 m (todo el rango [30, 3000] en ~150 px de
    // arrastre). Con la convención de "tomar la escena": arrastrar HACIA ARRIBA
    // ACERCA la cámara (dy < 0 -> dist_delta < 0) y hacia abajo la aleja.
    static constexpr float kOrbitGain = 0.005f;  // [rad/px]
    static constexpr float kZoomGain  = 0.03f;   // [1/px] (= 3 % de la distancia)

private:
    // Re-sincroniza controls_ desde los ejes normalizados y los topes.
    void sync_controls_from_sticks();

    dlfdm::ControlInputs controls_{};  // mandos de salida (lo que ve el FDM)
    CameraCommand        camera_cmd_{};
    Limits               limits_{};

    // Ejes normalizados internos: mandos en [-1, +1], potencia en [0, 1].
    float elev_ = 0.0f;
    float ail_  = 0.0f;
    float rud_  = 0.0f;
    float thr_  = 0.0f;
    // "Casas" del resorte: el punto al que VUELVEN los ejes al soltar la tecla.
    // set_controls() las fija en la posición del trim (sin trim: neutro 0).
    float elev_home_ = 0.0f;
    float ail_home_  = 0.0f;
    float rud_home_  = 0.0f;
    float thr_home_  = 0.0f;

    // Estado del mouse (Práctico 05): primer cuadro sin posición anterior.
    bool   first_ = true;
    double last_x_ = 0.0;
    double last_y_ = 0.0;
};