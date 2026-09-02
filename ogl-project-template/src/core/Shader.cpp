// ============================================================================
// Shader.cpp — Implementación del módulo Shader (ownership del programa).
// ============================================================================

#include "Shader.h"

#include <iostream>  // std::clog
#include <string>

namespace {

// Compila un shader individual y retorna su name; 0 si falla (imprime log).
unsigned int compile_shader(GLenum type, const std::string& src)
{
    const unsigned int shader = glCreateShader(type);
    const char* src_cstr = src.c_str();
    glShaderSource(shader, 1, &src_cstr, nullptr);
    glCompileShader(shader);

    GLint ok = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        // Largo obtenido del driver (no asumimos un buffer de tamaño fijo).
        GLint log_len = 0;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &log_len);
        std::string log(static_cast<std::size_t>(log_len), '\0');
        glGetShaderInfoLog(shader, log_len, nullptr, log.data());
        std::clog << "[Shader] Compile error:\n" << log << std::endl;
        glDeleteShader(shader);
        return 0U;
    }
    return shader;
}

}  // namespace

// ---------------------------------------------------------------------------
// Destructor
// ---------------------------------------------------------------------------
Shader::~Shader()
{
    clear();
}

// ---------------------------------------------------------------------------
// Mover: traslada la propiedad. El origen queda en cero.
// ---------------------------------------------------------------------------
Shader::Shader(Shader&& other) noexcept
    : id_(other.id_)
{
    other.id_ = 0U;
}

Shader& Shader::operator=(Shader&& other) noexcept
{
    if (this != &other) {
        clear();
        id_ = other.id_;
        other.id_ = 0U;
    }
    return *this;
}

// ---------------------------------------------------------------------------
// clear: libera el programa si lo tiene. Llamable dos veces sin romper.
// ---------------------------------------------------------------------------
void Shader::clear(void)
{
    if (id_ != 0U) {
        glDeleteProgram(id_);
        id_ = 0U;
    }
}

// ---------------------------------------------------------------------------
// use: activa el programa en el contexto actual.
// ---------------------------------------------------------------------------
void Shader::use(void) const
{
    glUseProgram(id_);
}

// ---------------------------------------------------------------------------
// compile_from_source: compila VS+FS, linkea y libera los objetos intermedios.
// Deja el objeto vacío (id == 0) si algo falla y retorna false. El objeto debe
// estar limpio al entrar; los objetos intermedios se borran en todos los caminos.
// ---------------------------------------------------------------------------
bool Shader::compile_from_source(const std::string& vs, const std::string& fs)
{
    clear();

    const unsigned int vs_id = compile_shader(GL_VERTEX_SHADER, vs);
    const unsigned int fs_id = compile_shader(GL_FRAGMENT_SHADER, fs);

    // Si alguna pieza no compiló, limpiar lo que sí y fallar.
    if (vs_id == 0U || fs_id == 0U) {
        if (vs_id != 0U) glDeleteShader(vs_id);
        if (fs_id != 0U) glDeleteShader(fs_id);
        return false;
    }

    const unsigned int program = glCreateProgram();
    glAttachShader(program, vs_id);
    glAttachShader(program, fs_id);
    glLinkProgram(program);

    // Los objetos intermedios ya no hacen falta en ningún caso.
    glDeleteShader(vs_id);
    glDeleteShader(fs_id);

    GLint ok = 0;
    glGetProgramiv(program, GL_LINK_STATUS, &ok);
    if (!ok) {
        GLint log_len = 0;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &log_len);
        std::string log(static_cast<std::size_t>(log_len), '\0');
        glGetProgramInfoLog(program, log_len, nullptr, log.data());
        std::clog << "[Shader] Link error:\n" << log << std::endl;
        glDeleteProgram(program);
        return false;
    }

    id_ = program;
    return true;
}