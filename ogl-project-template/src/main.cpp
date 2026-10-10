// ============================================================================
// main.cpp — Práctico 05: cámara en la escena (CameraSystem + InputHandler)
//
// La escena es la aeronave del Práctico 04 (Aircraft + RenderItem), pero la
// visualización deja de usar la "caja negra" (uniform uAjuste, un escalado que
// corregía el aspecto y daba vuelta el eje z SIN perspectiva) y pasa a una
// PROYECCIÓN DE PERSPECTIVA real armada por CameraSystem:
//   gl_Position = uProjection * uView * uModel * vec4(aPos, 1.0)
//
// Módulos por responsabilidad (arquitectura de la cátedra):
//   - CameraSystem : estado de la órbita (yaw/pitch/distancia) + view +
//                    projection. No lee el mouse ni sabe cómo es la escena:
//                    solo recibe el punto objetivo y un CameraCommand.
//   - InputHandler : lee el mouse (posición + botones) una vez por cuadro
//                    (polling) y lo convierte a CameraCommand.
//   - Aircraft     : arma la pose del avión por cuadro (sin base: la cámara
//                    elige el ángulo de vista, el modelo está en su
//                    referencial natural: morro −X, +Y ala derecha, +Z arriba).
//   - main         : orquesta: input -> cámara -> avión -> uniforms -> draw.
//
// Controles (ver InputHandler.h):
//   - Arrastrar con botón IZQUIERDO : orbita (mover X = acimut, Y = elevación).
//   - Arrastrar con botón DERECHO   : zoom (mover Y = acercar/alejar).
//   - Escape                        : cierra la ventana.
//
// El resto del esqueleto es idéntico a los prácticos anteriores (GLFW/GLAD,
// ResourceManager, Shader, Mesh, depth test y la herramienta PPM comentada).
// El main NO posee llamadas a glDelete*: liberar los recursos es
// responsabilidad de los destructores de Shader, Mesh y Aircraft, que corren
// dentro del bloque { } con el contexto OpenGL todavía vivo.
// ============================================================================

#include <cmath>            // std::sin
#include <cstdlib>          // EXIT_FAILURE, EXIT_SUCCESS
#include <fstream>          // captura temporal (PPM del back buffer)
#include <iostream>
#include <string>
#include <vector>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>  // glm::translate, scale, radians

#include <glad/gl.h>
#include <GLFW/glfw3.h>

#include "core/ResourceManager.h"
#include "core/Shader.h"
#include "core/Aircraft.h"
#include "core/RenderItem.h"
#include "core/CameraCommand.h"
#include "core/CameraSystem.h"
#include "core/InputHandler.h"

static const char* kWindowTitle     = "OpenGL - Practico 05: camara en la escena (orbita)";
static constexpr int kWindowWidth   = 800;
static constexpr int kWindowHeight  = 600;
static constexpr int kGLVerMajor    = 4;
static constexpr int kGLVerMinor    = 6;

// Amplitud del cabeceo animado en radianes (≈25.8°).
static constexpr float kPitchAmplitude = 0.45f;

static int glfw_error_code{};
static std::string glfw_error_str{};

static void error_callback(int error, const char *description);
static void framebuffer_size_callback(GLFWwindow* window, int width, int height);
static void processInput(GLFWwindow *window);
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

    // Colegir la cámara (CPU, sin OpenGL) ANTES del bloque de objetos de GPU:
    // vive toda la vida de la ventana y el callback de resize la necesita.
    CameraSystem camara(kWindowWidth, kWindowHeight);
    InputHandler input;

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
        //           sabe que "hay una aeronave".
        // ----------------------------------------------------------------
        Aircraft avion;
        avion.init();
        std::cout << "Aeronave inicializada (mallas subidas a la GPU)."
                  << std::endl;

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
        // ETAPA 3: bucle de renderizado
        //   En cada frame: leer mouse -> camara.update() -> avion.update()
        //   -> avion.collect(items) -> setear uniforms -> dibujar.
        //   items se recicla entre cuadros (collect() lo limpia y rellena).
        // ----------------------------------------------------------------
        std::vector<RenderItem> items;
        items.reserve(8);

        while (!glfwWindowShouldClose(window)) {
            processInput(window);

            glClearColor(51.0f / 256.0f, 55.0f / 256.0f, 76.0f / 256.0f, 1.0f);
            // Borrar también el buffer de profundidad en cada frame.
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            // ---- Input: mouse (posición, no desplazamiento) -> cámara ----
            double mouse_x = 0.0, mouse_y = 0.0;
            glfwGetCursorPos(window, &mouse_x, &mouse_y);
            const bool orbit_btn =
                glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;
            const bool zoom_btn =
                glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS;
            CameraCommand cmd;
            input.poll(mouse_x, mouse_y, orbit_btn, zoom_btn, cmd);

            // ---- Escena: cabeceo animado alrededor de la punta ----
            const float t = static_cast<float>(glfwGetTime());
            // El avión (largo 1, punta en su origen) se traslada para que su
            // CENTRO quede en el origen del mundo: la cámara orbita ese punto.
            const glm::vec3 posicion(-0.5f, 0.0f, 0.0f);
            const glm::vec3 angulos(0.0f, kPitchAmplitude * std::sin(t), 0.0f);

            // Posición (objetivo) y actitud (aún sin usar) de la aeronave.
            camara.update(posicion, angulos, cmd);
            avion.update(posicion, angulos);
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
            glfwPollEvents();

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
            //    std::ofstream out("/tmp/opencode/tp5.ppm", std::ios::binary);
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

void processInput(GLFWwindow *window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, true);
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