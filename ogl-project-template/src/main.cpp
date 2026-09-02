// ============================================================================
// main.cpp — Práctico 02: Del triángulo a la malla (cubo indexado)
//
// Dibuja un cubo indexado de 24 vértices y 36 índices, con un color plano por
// cara, usando los módulos del proyecto:
//   - ResourceManager : lee y cachea el código fuente de los shaders.
//   - Shader          : compila/linkea el programa (ownership del glDeleteProgram).
//   - primitives      : genera la MeshData del cubo (24 vértices / 36 índices).
//   - Mesh            : sube la malla a la GPU y arma VAO/VBO/EBO (ownership).
//
// El main NO posee llamadas a glDelete*: liberar los recursos es responsabilidad
// de los destructores de Shader y Mesh, que corren dentro del bloque { } con el
// contexto OpenGL todavía vivo (ver recomendación 2 de la consigna).
//
// Las tres primeras etapas (decidir datos, pasar a GPU, armar programa) las
// resuelven los módulos; en el main solo queda la etapa de dibujar con
// glDrawElements.
// ============================================================================

#include <cstdlib>          // EXIT_FAILURE, EXIT_SUCCESS
#include <iostream>
#include <string>

#include <glad/gl.h>
#include <GLFW/glfw3.h>

#include "core/ResourceManager.h"
#include "core/Shader.h"
#include "core/Mesh.h"
#include "core/Primitives.h"

static const char* kWindowTitle     = "OpenGL - Practico 02: cubo indexado";
static constexpr int kWindowWidth   = 800;
static constexpr int kWindowHeight  = 600;
static constexpr int kGLVerMajor    = 4;
static constexpr int kGLVerMinor    = 6;

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
    //   Shader y Mesh liberan sus recursos en sus destructores: este bloque
    //   garantiza que corran con el contexto OpenGL todavía vivo.
    // ========================================================================
    {
        // ----------------------------------------------------------------
        // ETAPA 1: decidir los datos (generar la malla del cubo, lado CPU)
        // ----------------------------------------------------------------
        const MeshData cube_mesh = primitives::cube();
        std::cout << "Cubo generado: " << cube_mesh.vertices.size()
                  << " vertices, " << cube_mesh.indices.size()
                  << " indices." << std::endl;

        // ----------------------------------------------------------------
        // ETAPA 2: pasar los datos a la GPU (módulo Mesh: VAO/VBO/EBO)
        // ----------------------------------------------------------------
        Mesh mesh;
        mesh.load(cube_mesh);

        // ----------------------------------------------------------------
        // ETAPA 3: armar el programa de shaders
        //   - ResourceManager lee las fuentes (cachea).
        //   - Shader compila y linkea.
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
            std::cout << "Fallo al compilar los shaders del cubo." << std::endl;
            return EXIT_FAILURE;  // los destructores liberan los recursos
        }

        // Habilitar el test de profundidad para que el orden de dibujo de los
        // triángulos genere una imagen coherente del cubo (caja negra).
        glEnable(GL_DEPTH_TEST);

        // ----------------------------------------------------------------
        // ETAPA 4: bucle de renderizado (dibujar con glDrawElements)
        // ----------------------------------------------------------------
        while (!glfwWindowShouldClose(window)) {
            processInput(window);

            glClearColor(51.0f / 256.0f, 55.0f / 256.0f, 76.0f / 256.0f, 1.0f);
            // Borrar también el buffer de profundidad en cada frame.
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            shader.use();
            glBindVertexArray(mesh.vao());
            glDrawElements(GL_TRIANGLES, mesh.count(), GL_UNSIGNED_INT, nullptr);

            glfwSwapBuffers(window);
            glfwPollEvents();
        }
    }  // <<-- acá corren los destructores de mesh y shader (contexto vivo)

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