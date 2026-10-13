// ============================================================================
// TerrainWindow.h — Ventana deslizante de terreno (mundo "ilimitado").
//
// El terreno es una VENTANA de (celdas+1)×(celdas+1) nodos que se desplaza por
// el heightmap infinito junto con el avión. Las alturas se leen de la fuente
// (HeightField) en NODOS enteros del mundo: por eso el origen de la ventana se
// redondea a la celda (window_origin). Al moverse a saltos de una celda, la
// superficie es SIEMPRE la misma (el terreno "pasa por abajo"); si el origen
// siguiera al avión de forma continua la malla "reptaría" (los vértices caerían
// entre nodos).
//
// EL PROBLEMA QUE RESUELVE ESTA CLASE (no obvio):
//   Recalcular toda la ventana de una vez cuesta ~78 ms para 800×800 (medido),
//   es decir, ~5 cuadros congelados cada vez que el avión cruza una celda. No se
//   puede. En cambio, la ventana solo necesita recentrarse cuando el avión se
//   aleja del centro MÁS DE UN MARGEN (kMargenCeldas), y ese recentrado se
//   REPARTE entre cuadros: en cada update() se avanza un presupuesto de muestras
//   (~1 ms por cuadro). Cuando la ventana nueva está completa se SUBCE el VBO de
//   una sola vez (un único glBufferSubData) y recién ahí se cambia el origen.
//   Hasta ese momento se sigue dibujando la ventana vieja → sin costura a medio
//   armar y sin titileo.
//
// Modelo de las dos fases (para no recalcular alturas dos veces):
//   kHeights : llena la grilla de alturas H_ (lo caro: 1 GetNoise por nodo).
//   kVertices: arma posición/normal/tex_coords desde H_ ya muestreada (barato).
//   kIdle    : sube la malla (si hacía falta) y espera.
//
// La clase posee el Mesh (VAO/VBO/EBO) y los buffers de CPU de staging. No
// conoce la cámara ni el FDM: solo recibe el punto (x,z) sobre el que centrarse.
// ============================================================================
#pragma once

#include <cstddef>
#include <vector>

#include <glm/glm.hpp>

#include "Mesh.h"
#include "MeshData.h"
#include "Terrain.h"

class TerrainWindow {
public:
    TerrainWindow() = default;
    ~TerrainWindow() = default;
    TerrainWindow(const TerrainWindow&) = delete;
    TerrainWindow& operator=(const TerrainWindow&) = delete;

    // Arma la topología (índices) UNA vez, genera la primera ventana centrada en
    // (center_x, center_z) de forma sincrónica y sube el VBO a la GPU.
    // Debe llamarse con el contexto OpenGL vivo.
    //   presupuesto: muestras de altura a procesar por cuadro en los recentrados
    //                posteriores (se reparten en varios update()).
    void init(unsigned celdas, float paso, const terrain::HeightField& h,
              float center_x, float center_z, unsigned presupuesto = 12000);

    // Avanza la construcción y recentra la ventana si el objetivo se alejó más
    // de kMargenCeldas del centro. No hace nada si el objetivo está cerca.
    // Devuelve true si la malla quedó actualizada (cambió el origen): el llamador
    // puede rearmar el RenderItem. Requiere contexto OpenGL vivo.
    bool update(float center_x, float center_z);

    // Malla a dibujar (la posee esta clase) y su modelo->mundo (traslación del
    // origen de la ventana). Los vértices están en coordenadas LOCALES.
    const Mesh* mesh() const { return &mesh_; }
    glm::mat4 model() const;

    int origin_x() const { return ox_; }
    int origin_z() const { return oz_; }
    bool rebuilding() const { return phase_ != Phase::kIdle; }

    // Rango de alturas de la ventana inicial (para reportar una vez en main).
    const terrain::Stats& stats() const { return stats_; }

private:
    enum class Phase { kIdle, kHeights, kVertices };

    void fill_heights(std::size_t presupuesto);
    void fill_vertices(std::size_t presupuesto);

    // Margen (en celdas) antes de recentrar: con paso 25 m son ~1 km. Como la
    // ventana tiene radio celdas/2 (~400 celdas = 10 km), mantener el margen
    // chico respecto del radio garantiza que el recorte lejano (far) siempre
    // caiga DENTRO de la ventana: nunca se ve el borde de la malla.
    static constexpr int kMargenCeldas = 40;

    unsigned celdas_ = 0;      // celdas por lado (vértices por lado = celdas_+1)
    unsigned n_ = 0;           // celdas_ + 1
    float paso_ = 1.0f;        // separación entre nodos [m]
    const terrain::HeightField* h_ = nullptr;
    unsigned presupuesto_ = 12000;

    Mesh mesh_;                    // VAO/VBO/EBO en la GPU
    MeshData data_;                // staging CPU de vértices + índices (topología)
    std::vector<float> H_;         // staging CPU de alturas (n_×n_)

    int ox_ = 0, oz_ = 0;          // origen de la ventana DIBUJADA
    int px_ = 0, pz_ = 0;          // origen de la ventana EN CONSTRUCCIÓN
    std::size_t cursor_ = 0;       // avance dentro de la fase actual
    Phase phase_ = Phase::kIdle;
    terrain::Stats stats_{};
};
