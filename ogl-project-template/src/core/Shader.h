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
//
// Uniforms (Práctico 03): se escriben con glProgramUniform* (DSA: no hace
// falta activar el programa con use() para setearlos). Estrategia de uso:
//   1. Consultar la ubicación una vez con loc(nombre) y cachearla en el
//      llamador (es un int).
//   2. Setear cada frame con set_uniform(ubicacion, valor).
//   loc() devuelve -1 si el uniform no existe (sin avisar); los setters por
//   nombre avisan por stderr cuando eso pasa y los setters por ubicación
//   IGNORAN en silencio el -1, así el llamador puede borrar/reordenar
//   uniforms sin tocar el código.
// ============================================================================
#pragma once

#include <string>

#include <glad/gl.h>

#include <glm/glm.hpp>

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

    // Ubicación de un uniform. Devuelve -1 si el uniform no existe en el
    // programa (sin avisar: se usa para consultar "está o no está").
    int loc(const std::string& name) const;

    // Setter por ubicación (ver estrategia de uso arriba). Ignora loc == -1.
    void set_uniform(int loc, const glm::mat4& value);
    void set_uniform(int loc, const glm::vec3& value);
    void set_uniform(int loc, const glm::vec4& value);  // p.ej. uColor RGBA

    // Conveniencia: consultan la ubicación y, si falta, avisan por stderr.
    void set_uniform(const std::string& name, const glm::mat4& value);
    void set_uniform(const std::string& name, const glm::vec3& value);
    void set_uniform(const std::string& name, const glm::vec4& value);

private:
    unsigned int id_ {0U};
};