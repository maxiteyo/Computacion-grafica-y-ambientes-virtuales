// ============================================================================
// Terrain.h — Generación de la malla de terreno (Práctico 07).
//
// Dos responsabilidades SEPARADAS (decisiones de la consigna):
//
//   1) La MALLA (topología + atributos) es una función libre del lado CPU que
//      solo depende de la interfaz HeightField: no sabe de OpenGL ni de ruido.
//      Devuelve un MeshData listo para subir con Mesh.
//
//   2) La ALTURA vive desacoplada de la malla: HeightField es una función
//      altura(i,j) evaluada en los NODOS del mundo (índices enteros). Cambiar la
//      fuente de alturas (ruido, archivo, una loma sintética) no toca la malla, y
//      viceversa. En los tests se inyecta un HeightField sintético y se verifica
//      la geometría sin ruido ni GPU.
//
// Convención de índices del mundo (la MISMA de la consigna, con x/z explícitos):
//   - La ventana es una grilla de (cells+1)×(cells+1) vértices.
//   - El vértice (i, j) (i = fila = índice z, j = columna = índice x) representa
//     el NODO del mundo (nx, nz) = (ox + j, oz + i).
//   - Su altura es SIEMPRE y = h(nx, nz); su posición LOCAL es (j·paso, y, i·paso)
//     y el origen (ox, oz) se aplica con la matriz de modelo (traslación). Así el
//     mundo es infinito: la malla es una VENTANA que se desliza sobre los índices
//     enteros y las alturas no dependen de dónde esté la ventana.
// ============================================================================
#pragma once

#include <vector>

#include "MeshData.h"

namespace terrain {

// ---------------------------------------------------------------------------
// Fuente de alturas, desacoplada de la malla. Se evalúa en NODOS del mundo:
//   x = nx · paso      z = nz · paso
// nx es el índice en x (columna) y nz el índice en z (fila); el nombre de los
// parámetros deja explícito el orden (no se puede "confundir" con (i,j)).
// ---------------------------------------------------------------------------
struct HeightField {
    virtual ~HeightField() = default;
    virtual float operator()(int nx, int nz) const = 0;
};

// Rango de alturas muestreadas en una ventana (mín/máx), para reportar una vez.
struct Stats {
    float min_h = 0.0f;
    float max_h = 0.0f;
};

// ---------------------------------------------------------------------------
// build_grid: genera la ventana de terreno.
//   celdas  : cantidad de celdas por lado (los vértices por lado son celdas+1).
//   paso    : separación entre nodos [m] (también lado de cada celda).
//   ox, oz  : índice del nodo del mundo en la esquina (fila 0, columna 0).
//   h       : fuente de alturas h(nx, nz).
//   stats   : si no es nullptr, se completa con el mínimo y el máximo muestreados.
//
// Vértices: (celdas+1)^2, en orden fila-mayor: idx(i,j) = i·(celdas+1) + j.
//   position   = (j·paso, h(ox+j, oz+i), i·paso)   [LOCAL; el origen va en el modelo]
//   normal     = normalizar( (H(i,j-1) − H(i,j+1), 2·paso, H(i-1,j) − H(i+1,j)) )
//                (diferencias CENTRADAS; en el borde se CLAMPea el índice del
//                 vecino: nunca queda una normal nula/indefinida)
//   tex_coords = (j/celdas, i/celdas)  [“cicatriz” para detectar costuras]
// Índices: 6·celdas^2 (dos triángulos por celda) y el orden elegido deja la
// normal geométrica (B−A)×(C−A) apuntando hacia +Y (arriba) en TODOS los
// triángulos, para que el back-face culling (CCW por defecto) oculte la cara de
// abajo del terreno sin tocar glFrontFace.
// ---------------------------------------------------------------------------
MeshData build_grid(unsigned celdas, float paso, int ox, int oz,
                    const HeightField& h, Stats* stats = nullptr);

// ---------------------------------------------------------------------------
// grid_normal: normal de la superficie en el nodo (i,j) a partir de una grilla
// de alturas H ya muestreada (fila-mayor, lado celdas+1, idx(i,j)=i·(celdas+1)+j).
// Diferencias CENTRADAS con los índices del borde clampeados a [0, celdas].
// Se expone para que la ventana deslizante (TerrainWindow) use EXACTAMENTE la
// misma fórmula que build_grid al recalcular una ventana de a poco.
// ---------------------------------------------------------------------------
glm::vec3 grid_normal(const std::vector<float>& H, unsigned celdas,
                      unsigned i, unsigned j, float paso);

// ---------------------------------------------------------------------------
// window_origin: índice de nodo de la esquina de la ventana para centrarla en
// (x, z) y que quede “encajada” en la grilla de nodos del mundo (múltiplos
// enteros de la celda):
//   ox = round(x / paso) − celdas/2
// El redondeo a entero es lo que evita el “repteo” (crawling): si el origen
// siguiera el avión de forma continua, los vértices caerían entre nodos y la
// superficie parecería arrastrarse/cambiar de forma. Moviéndola a saltos de una
// celda, la ventana siempre coincide con nodos reales del heightmap y la
// superficie es idéntica consigo misma (el terreno “pasa” por abajo).
// ---------------------------------------------------------------------------
int window_origin(float x, float paso, unsigned celdas);

}  // namespace terrain
