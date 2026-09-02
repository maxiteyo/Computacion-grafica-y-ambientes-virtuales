// ============================================================================
// MeshData.h — Definición de los datos de malla (lado CPU) y de la tupla de
// atributos de cada vértice. Compartida por los módulos Mesh y primitives.
//
// Decisiones:
//   - Vertex: atributos entrelazados { px, py, pz, r, g, b }. Escribimos los
//     desplazamientos (offset) del tipo de datos (puede llevar más atributos a
//     futuro); el módulo Mesh los recalcula con offsetof en vez de hardcodear 0/12.
//   - MeshData: acumula tuplas y, opcionalmente, índices. Si indices está vacía
//     se considera malla no indexada.
// ============================================================================
#pragma once

#include <vector>
#include <cstddef>  // offsetof

// Tupla de atributos para un vértice.
struct Vertex {
    float px, py, pz;  // posición
    float r, g, b;     // color
};

// Datos de malla del lado del CPU.
struct MeshData {
    std::vector<Vertex> vertices;          // tuplas de cada vértice
    std::vector<unsigned int> indices;     // índices (vacío si no indexada)
};