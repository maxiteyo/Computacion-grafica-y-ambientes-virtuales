// ============================================================================
// Mesh.cpp — Implementación del módulo Mesh (ownership de VAO/VBO/EBO).
// ============================================================================

#include "Mesh.h"

#include <cstddef>  // offsetof
#include <iostream>

// ---------------------------------------------------------------------------
// Destructor: libera lo que posee. El "borrado" se delega a clear(); al salir
// clear() los nombres quedan en 0 y eliminar 0 es no-op para OpenGL, así que es
// seguro aunque el objeto ya se haya limpiado o se haya movido.
// ---------------------------------------------------------------------------
Mesh::~Mesh()
{
    clear();
}

// ---------------------------------------------------------------------------
// Mover: se traspasa la propiedad (los names). El origen queda en cero para que
// su destructor (o clear()) no borre el objeto que ya no le pertenece.
// ---------------------------------------------------------------------------
Mesh::Mesh(Mesh&& other) noexcept
    : vao_(other.vao_), vbo_(other.vbo_), ebo_(other.ebo_), count_(other.count_)
{
    other.vao_ = other.vbo_ = other.ebo_ = 0U;
    other.count_ = 0;
}

Mesh& Mesh::operator=(Mesh&& other) noexcept
{
    if (this != &other) {
        clear();  // soltar lo que poseíamos antes de tomar lo nuevo
        vao_ = other.vao_;
        vbo_ = other.vbo_;
        ebo_ = other.ebo_;
        count_ = other.count_;
        other.vao_ = other.vbo_ = other.ebo_ = 0U;
        other.count_ = 0;
    }
    return *this;
}

// ---------------------------------------------------------------------------
// clear: libera los objetos que esta malla posee (si los tiene). Llamable dos
// veces sin romper: tras la primera los names vuelven a 0 y los glDelete son no-op.
// ---------------------------------------------------------------------------
void Mesh::clear(void)
{
    if (vao_ != 0U) {
        glDeleteVertexArrays(1, &vao_);
        vao_ = 0U;
    }
    if (vbo_ != 0U) {
        glDeleteBuffers(1, &vbo_);
        vbo_ = 0U;
    }
    if (ebo_ != 0U) {
        glDeleteBuffers(1, &ebo_);
        ebo_ = 0U;
    }
    count_ = 0;
}

// ---------------------------------------------------------------------------
// load: sube los datos a la GPU y arma el VAO con los atributos entrelazados.
// Dos buffers: VBO (vértices) y EBO (índices, conectado a la ranura dedicada).
// ---------------------------------------------------------------------------
void Mesh::load(const MeshData& data)
{
    // Si ya había algo subido, soltarlo antes de crear los objetos nuevos.
    clear();

    // Ancho de un vértice y desplazamientos de cada atributo, calculados (no
    // hardcodeados a 0 y 12) para no tener que tocarlos si la tupla cambia.
    const GLsizei stride =
        static_cast<GLsizei>(sizeof(Vertex));

    const GLint off_color =
        static_cast<GLint>(offsetof(Vertex, r));

    // VBO: subimos la lista de vértices completa (entrelazada).
    glCreateBuffers(1, &vbo_);
    glNamedBufferData(vbo_,
        static_cast<GLsizeiptr>(data.vertices.size() * sizeof(Vertex)),
        data.vertices.data(), GL_STATIC_DRAW);

    // EBO: subimos los índices si la malla es indexada.
    if (!data.indices.empty()) {
        glCreateBuffers(1, &ebo_);
        glNamedBufferData(ebo_,
            static_cast<GLsizeiptr>(data.indices.size() * sizeof(unsigned int)),
            data.indices.data(), GL_STATIC_DRAW);
    }

    // VAO: describe el formato de los atributos sobre el VBO y las ranuras.
    glCreateVertexArrays(1, &vao_);

    // El VBO se asocia a la ranura de atributos 0 (vía el binding 0).
    glVertexArrayVertexBuffer(vao_, 0, vbo_, 0, stride);

    // El EBO se conecta al VAO por su ranura dedicada (índice).
    if (ebo_ != 0U) {
        glVertexArrayElementBuffer(vao_, ebo_);
    }

    // Atributo 0: posición (3 floats).
    glVertexArrayAttribFormat(vao_, 0, 3, GL_FLOAT, GL_FALSE, 0);
    glVertexArrayAttribBinding(vao_, 0, 0);
    glEnableVertexArrayAttrib(vao_, 0);

    // Atributo 1: color (3 floats), a partir de off_color bytes.
    glVertexArrayAttribFormat(vao_, 1, 3, GL_FLOAT, GL_FALSE, off_color);
    glVertexArrayAttribBinding(vao_, 1, 0);
    glEnableVertexArrayAttrib(vao_, 1);

    // count_ guarda la cantidad de ÍNDICES (lo que necesita glDrawElements).
    count_ = static_cast<int>(data.indices.empty()
                                  ? data.vertices.size()
                                  : data.indices.size());
}