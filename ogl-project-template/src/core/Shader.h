// ============================================================================
// Shader.h — Administra el ownership del programa de shader linkeado (GPU).
//
// Misma estrategia RAII que Mesh, aplicada al programa de shader (un único
// entero en vez de tres). Decisiones:
//   - No copiable / movible: id_ es un name de OpenGL con dueño único.
//   - No abre archivos: recibe el código fuente ya leído (lo provee
//     ResourceManager del Práctico 01).
//   - Errores: imprime el log del driver por stderr y deja el objeto vacío
//     (id == 0). Coherente con la decisión del ResourceManager (stderr, sin
//     excepciones). El "log" NO se asume de largo fijo: se consulta con
//     glGetShaderiv/glGetProgramiv(GL_INFO_LOG_LENGTH).
//   - clear() y el destructor liberan; llamables repetidamente sin romper.
// ============================================================================
#pragma once

#include <string>

#include <glad/gl.h>

class Shader {
public:
    Shader() = default;
    ~Shader();                            // libera el programa
    Shader(const Shader&) = delete;       // las MISMAS tres decisiones
    Shader& operator=(const Shader&) = delete;
    Shader(Shader&& other) noexcept;
    Shader& operator=(Shader&& other) noexcept;

    // Compila VS + FS, linkea y libera los objetos intermedios.
    // Retorna true si compiló y linkeó OK; false si falló (objeto queda vacío).
    bool compile_from_source(const std::string& vs, const std::string& fs);

    void use(void) const;
    void clear(void);
    unsigned int id(void) const { return id_; }

private:
    unsigned int id_ {0U};
};