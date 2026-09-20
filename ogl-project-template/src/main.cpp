// ============================================================================
// main.cpp — Práctico 04: aeronave compuesta (módulo Aircraft + RenderItem)
//
// La escena del Práctico 03 (grilla 2x2) se reemplaza por UNA sola aeronave
// (S-211 estilizado) compuesta por primitivas del P03 y agrupada en el módulo
// Aircraft. El main NO conoce las piezas del avión: por cuadro solo
//   1. avion.update(pos, angulos)  -> fija la pose de la aeronave (cabeceo)
//   2. avion.collect(items)        -> "suelta" los RenderItem listos (7)
//   3. por cada RenderItem          -> setea uModel/uColor y dibuja la malla
// El cabeceo se anima con el reloj (pitch = amplitud * sin(t)): es la rotación
// sobre el eje Y (paralelo al ala) que pasa por el punto de referencia de la
// aeronave (la punta de la nariz), así que el morro queda clavado y la cola
// balancea.
//
// El resto del esqueleto es idéntico al Práctico 03:
//   - ResourceManager : lee y cachea el código fuente de los shaders.
//   - Shader          : compila/linkea el programa y setea uniforms (DSA).
//   - Mesh            : sube cada malla a la GPU y arma VAO/VBO/EBO.
//   - uAjuste         : "caja negra" (ajuste por aspecto de la ventana de 800x600).
//   - prompt de normales: los fragmentos se pintan con uColor (vec4) plano.
//
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

static const char* kWindowTitle     = "OpenGL - Practico 04: aeronave (Aircraft + RenderItem)";
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

    // Configuración base de GLFW (callbacks, vsync).
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSwapInterval(1);

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
        const int locModel  = shader.loc("uModel");
        const int locColor  = shader.loc("uColor");
        const int locAjuste = shader.loc("uAjuste");

        // Matriz de ajuste de la "caja negra" (aspecto de la ventana 800x600).
        const glm::mat4 uAjuste =
            glm::scale(glm::mat4(1.0f), glm::vec3(
                static_cast<float>(kWindowHeight) / static_cast<float>(kWindowWidth),
                1.0f, -1.0f));

        shader.use();
        shader.set_uniform(locAjuste, uAjuste);  // se setea una sola vez

        // Habilitar el test de profundidad para que las piezas (y las caras de
        // cada pieza) se oculten entre sí de forma coherente (caja negra).
        glEnable(GL_DEPTH_TEST);

        // ----------------------------------------------------------------
        // ETAPA 3: bucle de renderizado
        //   En cada frame: avion.update() -> avion.collect(items) -> dibujar.
        //   items se recicla entre cuadros (collect() lo limpia y rellena).
        //   Nada de regenerar mallas ni de releer shaders dentro del bucle.
        // ----------------------------------------------------------------
        std::vector<RenderItem> items;
        items.reserve(8);

        while (!glfwWindowShouldClose(window)) {
            processInput(window);

            glClearColor(51.0f / 256.0f, 55.0f / 256.0f, 76.0f / 256.0f, 1.0f);
            // Borrar también el buffer de profundidad en cada frame.
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            // Animación del cabeceo alrededor de la punta (punto de referencia
            // de la aeronave): la rotación es sobre el eje Y del avión (paralelo
            // al ala). El avión se dibuja en vista TRES CUARTOS con el morro
            // "arriba" de la pantalla (base fija en Aircraft::update): el largo 1
            // del fuselaje ocupa y ∈ [-0.5, 0.5], las alas quedan horizontales
            // (izq-der) y la DERIVA se ve parada en la cola.
            const float t = static_cast<float>(glfwGetTime());
            const glm::vec3 posicion(0.0f, 0.5f, 0.0f);  // centra el largo 1
            const glm::vec3 angulos(0.0f, kPitchAmplitude * std::sin(t), 0.0f);

            avion.update(posicion, angulos);
            avion.collect(items);

            shader.use();
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
            //    std::ofstream out("/tmp/opencode/tp4.ppm", std::ios::binary);
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

void framebuffer_size_callback([[maybe_unused]] GLFWwindow* window,
                               int width, int height)
{
    glViewport(0, 0, width, height);
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