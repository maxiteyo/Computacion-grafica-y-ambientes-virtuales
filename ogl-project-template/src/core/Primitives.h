// ============================================================================
// Primitives.h — Generador de mallas de figuras primitivas (lado CPU, C++ puro).
//
// Cada función devuelve una MeshData (tuplas + índices). Hoy sólo existe el
// cubo; el próximo práctico se agregan cilindro y cono, y más adelante disco,
// quad, triángulo, etc.
//
// Decisiones:
//   - Funciones libres en un namespace (NO una clase): el generador no posee
//     ningún recurso, solo produce datos. "¿Qué posee este objeto?" → nada,
//     y por eso no es una clase.
//   - Los colores quedan fijos dentro del generador (uno plano por cara).
//   - El cubo queda CENTRADO en el origen (no apoyado sobre el plano y = 0).
//   - Cada cara se recorre en sentido antihorario (CCW) mirando la cara desde
//     fuera. De esto dependerá el descarte de caras traseras (back-face culling).
//   - Los factores de escala (scale_x/y/z) con valor por defecto 1 dan un cubo
//     de lado unitario; con otros valores dan placas / cajas rectas.
// ============================================================================
#pragma once

#include "MeshData.h"

namespace primitives {

// Cubo de lado unitario (por defecto), centrado en el origen.
// 24 vértices únicos + 36 índices, color plano distinto por cara.
// Los ejes X, Y, Z se escalan por los argumentos (cubo o placa).
MeshData cube(float scale_x = 1.0f, float scale_y = 1.0f, float scale_z = 1.0f);

}  // namespace primitives