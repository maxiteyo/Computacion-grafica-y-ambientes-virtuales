// ============================================================================
// Mesh.h — Administra el ownership del VAO, VBO y EBO de una malla (GPU).
//
// Un objeto de OpenGL se identifica por un entero (name); copiar ese entero no
// copia el objeto y perderlo lo deja huérfano en la GPU hasta cerrar el contexto.
// Por eso esta clase implementa ownership exclusivo tipo RAII:
//
// Decisiones:
//   - No copiable: copiar rompería el dueño único (double-free del name).
//   - Movible: el mover traspasa la propiedad y deja al origen en cero.
//   - El destructor libera todo (glDelete* se escriben SOLO acá).
//   - clear() permite liberar dos veces sin romper y sin banderas auxiliares:
//     los nombres vuelven a 0 y 0 es "no hay objeto" para OpenGL.
//   - count_ es la cantidad de ÍNDICES (lo que glDrawElements necesita).
// ============================================================================
#pragma once

#include "MeshData.h"

// Este módulo encapsula la API de OpenGL para subir/liberar la malla.
#include <glad/gl.h>

class Mesh {
public:
    Mesh() = default;
    ~Mesh();                              // libera lo que posee
    Mesh(const Mesh&) = delete;           // copiar rompe el dueño único
    Mesh& operator=(const Mesh&) = delete;
    Mesh(Mesh&& other) noexcept;          // mover: traspasa la propiedad
    Mesh& operator=(Mesh&& other) noexcept;

    // Sube los datos a la GPU y arma el VAO/VBO/EBO (debemos tener contexto GL).
    void load(const MeshData& data);

    // Libera los objetos de GPU. Llamable dos veces: no rompe nada.
    void clear(void);

    unsigned int vao(void) const { return vao_; }
    int count(void) const { return count_; }  // cantidad de ÍNDICES

private:
    unsigned int vao_ {0U}, vbo_ {0U}, ebo_ {0U};
    int count_ {0};
};