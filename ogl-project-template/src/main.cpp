// ============================================================================
// main.cpp — Práctico 06: game loop, input y FDM
//
// La escena sigue siendo la aeronave del Práctico 04 (Aircraft + RenderItem)
// con la cámara orbital del Práctico 05, pero ahora la aeronave se mueve según
// un MODELO DE DINÁMICA DE VUELO (FDM, libs/dlfdm) sincronizado con el pintado
// de cuadros.
//
// El loop usa la estrategia de PASO FIJO CON ACUMULADOR (consigna, Parte 1):
//   - El FDM se integra con un paso pequeño y CONSTANTE (dt = 1/120 s): el
//     error de cada paso no depende de la máquina.
//   - El tiempo real de cada cuadro (frame_dt) se acumula y se consume en
//     pasos discretos; el resto que queda NO se descarta (tiempo simulado +
//     resto = tiempo de reloj).
//   - frame_dt se topa en 0.25 s para que un cuadro lento (arrastrar la
//     ventana, pausa del depurador) no haga diverger el acumulador (la
//     aplicación simula como máximo 30 pasos y sigue viva, no se congela).
//   - La entrada del usuario se lee UNA vez por cuadro FUERA del while de
//     pasos fijos (es un estado: "¿está apretada ahora?").
//
// Módulos por responsabilidad (arquitectura de la cátedra):
//   - FlightData    : única transformación NED -> mundo (fp::to_world).
//   - dlfdm::FDMSolver: integra la dinámica; se actualiza con los mandos y
//                    entrega el estado (getState) una vez por cuadro.
//   - InputHandler  : lee mouse + teclado por polling y produce CameraCommand
//                    (cámara) y dlfdm::ControlInputs (mandos del avión).
//   - CameraSystem  : órbita de seguimiento del avión (objetivo = posición).
//   - Aircraft      : arma la pose del avión (actitud NED adaptada al modelo).
//
// Controles (ver InputHandler.h):
//   Mouse: botón IZQUIERDO orbita, botón DERECHO hace zoom (Práctico 05).
//   Flechas: palanca (↑↓ elevador: ↑ nariz abajo / ↓ nariz arriba; ←→
//   alerones); Q/E: pedales (timón); W/S: potencia. P: pausa (evento);
//   L: wireframe (evento); ESC: salir.
//
// El ejecutable de prueba standalone del FDM vive en libs/dlfdm (make + ./build/
// dlfdm-test): 60 s de simulación off-line para verificar el módulo aislado.
//
// El resto del esqueleto es idéntico a los prácticos anteriores (GLFW/GLAD,
// ResourceManager, Shader, Mesh, depth test y la herramienta PPM comentada).
// El main NO posee llamadas a glDelete*: liberar los recursos es
// responsabilidad de los destructores de Shader, Mesh y Aircraft, que corren
// dentro del bloque { } con el contexto OpenGL todavía vivo. El FDM vive
// FUERA de ese bloque (solo usa GLM), junto con cámara e input.
// ============================================================================

#include <algorithm>    // std::min
#include <cstdlib>      // EXIT_FAILURE, EXIT_SUCCESS
#include <fstream>      // captura temporal (PPM del back buffer)
#include <iostream>
#include <string>
#include <vector>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>  // glm::translate, scale, radians

#include <glad/gl.h>
#include <GLFW/glfw3.h>

#include <dlfdm/defines.h>
#include <dlfdm/fdmsolver.h>
#include <dlfdm/models/aircraft/jettrainer.h>

#include "core/ResourceManager.h"
#include "core/Shader.h"
#include "core/Aircraft.h"
#include "core/RenderItem.h"
#include "core/CameraCommand.h"
#include "core/CameraSystem.h"
#include "core/InputHandler.h"
#include "core/FlightData.h"

static const char* kWindowTitle     = "OpenGL - Practico 06: game loop, input y FDM";
static constexpr int kWindowWidth   = 800;
static constexpr int kWindowHeight  = 600;
static constexpr int kGLVerMajor    = 4;
static constexpr int kGLVerMinor    = 6;

// Paso fijo de integración del FDM [s]: el constructor de FDMSolver usa el
// mismo valor por defecto (1/120); acá queda explícito el del acumulador.
static constexpr double kFixedDt = 1.0 / 120.0;
// Tope del tiempo real acumulado por cuadro [s]: corta la divergencia del
// acumulador después de pausas o al arrastrar la ventana (consigna, Parte 1).
static constexpr double kMaxFrameDt = 0.25;
// Escala visual del modelo (mide 1) para el mundo en metros: el S-211 real
// mide ~9.6 m de largo -> legible a la distancia de cámara de seguimiento.
static constexpr float kAircraftScale = 9.6f;

static int glfw_error_code{};
static std::string glfw_error_str{};

static void error_callback(int error, const char *description);
static void framebuffer_size_callback(GLFWwindow* window, int width, int height);
static void print_gl_version(void);

int main()
{
    // ========================================================================
    // FASE 0: INICIALIZACIÓN DE GLFW (ventana + contexto OpenGL 4.6 core)
    // ========================================================================
    glfwSetErrorCallback(error_callback);

    if (!glfwInit()) {
        std::cout << "GLFW initialization failed!" << std::endl;
        return EXIT_FAILURE;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, kGLVerMajor);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, kGLVerMinor);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    // Asegurar un buffer de profundidad de 24 bits (el test de profundidad del
    // Práctico 02 no funciona sin él: las caras se superpondrían sin ocultarse).
    glfwWindowHint(GLFW_DEPTH_BITS, 24);
    glfwWindowHint(GLFW_STENCIL_BITS, 0);

    GLFWwindow* window = glfwCreateWindow(kWindowWidth, kWindowHeight,
                                          kWindowTitle, nullptr, nullptr);
    if (window == nullptr) {
        std::cout << "GLFW window creation failed!" << std::endl;
        glfwTerminate();
        return EXIT_FAILURE;
    }

    glfwMakeContextCurrent(window);

    // ========================================================================
    // FASE 1: CARGA DE PUNTEROS DE FUNCIONES OPENGL CON GLAD
    // ========================================================================
    if (!gladLoadGL(glfwGetProcAddress)) {
        std::cout << "GLAD initialization failed!" << std::endl;
        glfwDestroyWindow(window);
        glfwTerminate();
        return EXIT_FAILURE;
    }

    print_gl_version();

    // ========================================================================
    // FASE 2: ESTADO CPU (sin OpenGL): cámara, input y el FDM.
    //   El FDM solo usa GLM: se crea acá, antes del bloque de objetos de GPU.
    // ========================================================================
    CameraSystem camara(kWindowWidth, kWindowHeight);
    InputHandler input;

    // Modelo físico de la aeronave (S-211 "jet trainer" del repo dlfdm).
    const dlfdm::AircraftParameters params = dlfdm::jettrainer::load_model();
    // El paso de integración se fija ACÁ (por defecto vale dt = 1/120 s).
    dlfdm::FDMSolver fdm(params);

    // Condición inicial = equilibrio (trim) a 5000 m y 150 m/s (~540 km/h):
    // estado en el solver y posiciones de los mandos en el InputHandler.
    const dlfdm::TrimPoint trim = dlfdm::jettrainer::get_trim_condition(
        dlfdm::jettrainer::TrimCondition::kISA5000TAS150);
    fdm.setState(trim.state);
    input.set_controls(trim.controls);
    std::cout << "FDM listo: trim a 5000 [m] y 150 [m/s] TAS, paso "
              << kFixedDt << " [s]." << std::endl;

    // Configuración base de GLFW (callbacks, vsync).
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSwapInterval(1);

    // El callback de GLFW es una función, no un método: para que llegue a la
    // cámara se guarda el puntero al objeto como "user pointer" de la ventana
    // (glfwSetWindowUserPointer) y se recupera adentro del callback.
    glfwSetWindowUserPointer(window, &camara);

    // ========================================================================
    // COMIENZO DEL BLOQUE QUE POSEÉ LOS OBJETOS DE GPU (raspado en { })
    //   Shader, Mesh y Aircraft liberan sus recursos en sus destructores: este
    //   bloque garantiza que corran con el contexto OpenGL todavía vivo.
    // ========================================================================
    {
        // ----------------------------------------------------------------
        // ETAPA 1: crear la aeronave. init() genera sus 5 mallas y arma las
        //           7 piezas (locales y colores) UNA sola vez; el main solo
        //           sabe que "hay una aeronave". Escala visual para el mundo
        //           en metros (el modelo mide 1).
        // ----------------------------------------------------------------
        Aircraft avion;
        avion.init();
        avion.set_scale(kAircraftScale);
        std::cout << "Aeronave inicializada (mallas subidas a la GPU, escala "
                  << kAircraftScale << ")." << std::endl;

        // ----------------------------------------------------------------
        // ETAPA 2: armar el programa de shaders y consultar sus uniforms
        // ----------------------------------------------------------------
        ResourceManager rm(".");
        const ShaderSource& src = rm.load_shader_source(
            "cube",
            "assets/shaders/cube.vs",
            "assets/shaders/cube.fs"
        );
        Shader shader;
        const bool shader_ok = shader.compile_from_source(src.vs, src.fs);
        if (!shader_ok) {
            std::cout << "Fallo al compilar los shaders." << std::endl;
            return EXIT_FAILURE;  // los destructores liberan los recursos
        }

        // Ubicaciones consultadas UNA vez y luego reutilizadas en el bucle.
        const int locModel      = shader.loc("uModel");
        const int locView       = shader.loc("uView");
        const int locProjection = shader.loc("uProjection");
        const int locColor      = shader.loc("uColor");

        shader.use();

        // Habilitar el test de profundidad para que las piezas (y las caras de
        // cada pieza) se oculten entre sí de forma coherente.
        glEnable(GL_DEPTH_TEST);

        // ----------------------------------------------------------------
        // ETAPA 3: bucle de renderizado (paso fijo con acumulador, Parte 1)
        // ----------------------------------------------------------------
        std::vector<RenderItem> items;
        items.reserve(8);

        // Sincronización con el reloj: el acumulador junta el tiempo real de
        // cada cuadro y se consume en pasos fijos de kFixedDt. En el PRIMER
        // cuadro acc = 0 -> no corre ningún paso (el avión arranca en el trim).
        double antes = glfwGetTime();
        double acc = 0.0;

        // Eventos discretos (¿se acaba de apretar?): estado anterior por tecla
        // para detectar el borde y alternar UNA vez por pulsación (consigna).
        bool pause_on = false;   bool p_prev = false;
        bool wire_on  = false;   bool l_prev = false;

        while (!glfwWindowShouldClose(window)) {
            glfwPollEvents();

            const double ahora = glfwGetTime();
            const double frame_dt = ahora - antes;
            antes = ahora;

            // ---- Eventos discretos: pausa (P) y wireframe (L) ----
            const bool p_cur = glfwGetKey(window, GLFW_KEY_P) == GLFW_PRESS;
            if (p_cur && !p_prev) {
                pause_on = !pause_on;
                std::cout << (pause_on ? "[P] simulación PAUSADA."
                                       : "[P] simulación reanudada.")
                          << std::endl;
            }
            p_prev = p_cur;

            const bool l_cur = glfwGetKey(window, GLFW_KEY_L) == GLFW_PRESS;
            if (l_cur && !l_prev) {
                wire_on = !wire_on;
                glPolygonMode(GL_FRONT_AND_BACK,
                              wire_on ? GL_LINE : GL_FILL);
                std::cout << (wire_on ? "[L] modo WIREFRAME."
                                      : "[L] modo relleno.")
                          << std::endl;
            }
            l_prev = l_cur;

            if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
                glfwSetWindowShouldClose(window, true);
            }

            // ---- Input (Parte 2): polling una vez por cuadro, FUERA del
            //      while de pasos fijos (es un estado, no una acción). ----
            input.update(window, static_cast<float>(frame_dt),
                         camara.distance());

            // ---- Acumulador de paso fijo (Parte 1 + Parte 3) ----
            acc += std::min(frame_dt, kMaxFrameDt);
            if (pause_on) {
                // Congelado: se descarta el acumulador para que al reanudar no
                // "pegue" el salto de todos los cuadros acumulados.
                acc = 0.0;
            } else {
                // Un paso de simulación por cada dt acumulado. El FDM se
                // actualiza con las posiciones de los mandos del cuadro.
                while (acc >= kFixedDt) {
                    fdm.update(input.controls());
                    acc -= kFixedDt;
                }
            }

            // ---- Último estado integrado -> escena (UNA vez por cuadro) ----
            // getState() devuelve el estado YA integrado: se transforma NED ->
            // mundo y se lo pasan al avión y a la cámara (que lo sigue).
            const dlfdm::AircraftState& ned = fdm.getState();
            const fp::FlightData vuelo = fp::to_world(ned);
            const glm::vec3 angulos(vuelo.phi, vuelo.theta, vuelo.psi);

            glClearColor(51.0f / 256.0f, 55.0f / 256.0f, 76.0f / 256.0f, 1.0f);
            // Borrar también el buffer de profundidad en cada frame.
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            camara.update(vuelo.position, angulos, input.camera_cmd());
            avion.update(vuelo.position, angulos);
            avion.collect(items);

            shader.use();
            shader.set_uniform(locView, camara.data().view);
            shader.set_uniform(locProjection, camara.data().projection);
            for (const auto& item : items) {
                shader.set_uniform(locModel, item.model);
                shader.set_uniform(locColor, item.color);
                glBindVertexArray(item.mesh->vao());
                glDrawElements(GL_TRIANGLES, item.mesh->count(),
                               GL_UNSIGNED_INT, nullptr);
            }

            glfwSwapBuffers(window);

            // Herramienta de verificación (comentada): para volcar un frame a
            // un PPM usar glReadPixels(GL_BACK) ANTES del swap, sin depender
            // del compositor. La imagen sale invertida verticalmente (GL (0,0)
            // es abajo-izquierda): hay que voltearla al interpretarla.
            //static bool captured = false;
            //if (!captured) {
            //    std::vector<unsigned char> buf(4U * kWindowWidth * kWindowHeight);
            //    glReadBuffer(GL_BACK);
            //    glReadPixels(0, 0, kWindowWidth, kWindowHeight, GL_RGBA,
            //                 GL_UNSIGNED_BYTE, buf.data());
            //    std::vector<unsigned char> rgb(buf.size() / 4U * 3U);
            //    for (std::size_t i = 0, j = 0; i < buf.size(); i += 4, j += 3) {
            //        rgb[j] = buf[i]; rgb[j + 1] = buf[i + 1]; rgb[j + 2] = buf[i + 2];
            //    }
            //    std::ofstream out("/tmp/opencode/tp6.ppm", std::ios::binary);
            //    out << "P6\n" << kWindowWidth << " " << kWindowHeight << "\n255\n";
            //    out.write(reinterpret_cast<const char*>(rgb.data()),
            //              static_cast<std::streamsize>(rgb.size()));
            //    captured = true;
            //}
        }
    }  // <<-- acá corren los destructores de avion y shader (contexto vivo)

    // ========================================================================
    // FASE FINAL: cierre de GLFW
    // ========================================================================
    glfwDestroyWindow(window);
    glfwTerminate();

    return EXIT_SUCCESS;
}

void error_callback(int error, const char *description)
{
    glfw_error_code = error;
    glfw_error_str = std::string(description);
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    // El tamaño del framebuffer lo cambia el sistema de ventanas, no OpenGL:
    //   - glViewport define el rectángulo de píxeles donde se mapean las
    //     coordenadas normalizadas;
    //   - camara.set_viewport() recalcula la PROYECCIÓN para el nuevo aspect
    //     (la cámara llegó por el user pointer guardado en el main).
    glViewport(0, 0, width, height);
    if (auto* camara =
            static_cast<CameraSystem*>(glfwGetWindowUserPointer(window))) {
        camara->set_viewport(width, height);
    }
}

void print_gl_version(void)
{
    std::cout << " OpenGL Vendor: " << glGetString(GL_VENDOR) << std::endl;
    std::cout << " OpenGL Renderer: " << glGetString(GL_RENDERER) << std::endl;
    std::cout << " OpenGL Version: " << glGetString(GL_VERSION) << std::endl;
    std::cout << " GLSL Version: "
              << glGetString(GL_SHADING_LANGUAGE_VERSION) << std::endl;
}