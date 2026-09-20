// ============================================================================
// MeshData.h — Definición de los datos de malla (lado CPU) y de la tupla de
// atributos de cada vértice. Compartida por los módulos Mesh y primitives.
//
// Decisiones (Práctico 03):
//   - Vertex: atributos entrelazados { position, normal, tex_coords } con tipos
//     glm::vec3/vec2 (32 bytes en total; offets 0/12/24). El COLOR ya NO forma
//     parte de la tupla: pasa a ser dato de la pieza y se envía por uniform del
//     fragment shader (cada objeto de la escena puede tener su color propio).
//     La normal se usa para futura iluminación y para el debug de normales
//     (pintar cada fragmento según su vNormal); tex_coords quedan para futuras
//     texturas, pero hoy también sirven de "cicatriz" para detectar costuras.
//   - MeshData: acumula tuplas y, opcionalmente, índices. Si indices está vacía
//     se considera malla no indexada.
// ============================================================================
#pragma once

#include <vector>
#include <cstddef>  // offsetof

#include <glm/glm.hpp>

// Tupla de atributos para un vértice.
struct Vertex {
    glm::vec3 position;    // offset 0  (12 bytes)
    glm::vec3 normal;      // offset 12 (12 bytes)
    glm::vec2 tex_coords;  // offset 24 (8 bytes)  -> sizeof(Vertex) == 32
};

// Datos de malla del lado del CPU.
struct MeshData {
    std::vector<Vertex> vertices;          // tuplas de cada vértice
    std::vector<unsigned int> indices;     // índices (vacío si no indexada)
};