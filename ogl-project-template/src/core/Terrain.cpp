// ============================================================================
// Terrain.cpp — Implementación de build_grid, grid_normal y window_origin.
// Ver Terrain.h para el diseño y las decisiones de topología.
// ============================================================================

#include "Terrain.h"

#include <algorithm>  // std::min, std::max
#include <cmath>      // std::lround

#include <glm/glm.hpp>

namespace terrain {

namespace {

// Índice de nodo del mundo clampeado a [0, celdas]: en el borde de la ventana el
// "vecino" es el propio borde (diferencia a un lado). Así la normal del borde
// está definida y nunca es nula (el término 2·paso > 0 se lo garantiza).
int clamp_node(int v, unsigned celdas)
{
    if (v < 0) return 0;
    if (v > static_cast<int>(celdas)) return static_cast<int>(celdas);
    return v;
}

}  // namespace

int window_origin(float x, float paso, unsigned celdas)
{
    const long centro = std::lround(x / paso);
    return static_cast<int>(centro - static_cast<long>(celdas / 2));
}

glm::vec3 grid_normal(const std::vector<float>& H, unsigned celdas,
                      unsigned i, unsigned j, float paso)
{
    const std::size_t fila = static_cast<std::size_t>(celdas) + 1u;
    const int ii = static_cast<int>(i);
    const int jj = static_cast<int>(j);

    // Normal por diferencias centradas. En x el vecino anterior es (j−1) y el
    // siguiente (j+1); en z, (i−1) y (i+1). El signo de cada componente deja la
    // normal apuntando HACIA ARRIBA (+Y):
    //   n ∝ ( h(i,j−1) − h(i,j+1),  2·paso,  h(i−1,j) − h(i+1,j) )
    const std::size_t row = static_cast<std::size_t>(ii) * fila;
    const float n_x = H[row + static_cast<std::size_t>(clamp_node(jj - 1, celdas))]
                    - H[row + static_cast<std::size_t>(clamp_node(jj + 1, celdas))];
    const float n_z = H[static_cast<std::size_t>(clamp_node(ii - 1, celdas)) * fila + j]
                    - H[static_cast<std::size_t>(clamp_node(ii + 1, celdas)) * fila + j];
    return glm::normalize(glm::vec3(n_x, 2.0f * paso, n_z));
}

MeshData build_grid(unsigned celdas, float paso, int ox, int oz,
                    const HeightField& h, Stats* stats)
{
    const unsigned fila = celdas + 1;           // vértices por lado
    const std::size_t n = static_cast<std::size_t>(fila);

    MeshData data;
    data.vertices.resize(n * n);
    data.indices.resize(6u * celdas * celdas);

    // ---- 1) Alturas: una lectura por nodo y se reutilizan para las normales ----
    // idx(i,j) = i*fila + j  (fila-mayor), igual que los vértices.
    std::vector<float> H(n * n, 0.0f);
    float hmin = 0.0f;
    float hmax = 0.0f;
    for (unsigned i = 0; i < fila; ++i) {
        for (unsigned j = 0; j < fila; ++j) {
            const std::size_t k = static_cast<std::size_t>(i) * n + j;
            const float y = h(ox + static_cast<int>(j), oz + static_cast<int>(i));
            H[k] = y;
            if (k == 0) {
                hmin = hmax = y;
            } else {
                hmin = std::min(hmin, y);
                hmax = std::max(hmax, y);
            }
        }
    }
    if (stats) {
        stats->min_h = hmin;
        stats->max_h = hmax;
    }

    // ---- 2) Vértices: posición LOCAL + normal + tex_coords ----
    const float inv_celdas = 1.0f / static_cast<float>(celdas);
    for (unsigned i = 0; i < fila; ++i) {
        for (unsigned j = 0; j < fila; ++j) {
            Vertex v;
            v.position = glm::vec3(static_cast<float>(j) * paso,
                                   H[static_cast<std::size_t>(i) * n + j],
                                   static_cast<float>(i) * paso);
            v.normal = grid_normal(H, celdas, i, j, paso);
            v.tex_coords = glm::vec2(static_cast<float>(j) * inv_celdas,
                                     static_cast<float>(i) * inv_celdas);
            data.vertices[static_cast<std::size_t>(i) * n + j] = v;
        }
    }

    // ---- 3) Índices: dos triángulos por celda, CCW vistos desde arriba ----
    // Para la celda (i,j) con a=(i,j), b=(i,j+1), d=(i+1,j), c=(i+1,j+1):
    //   triángulo 1 (a, d, c)   triángulo 2 (a, c, b)
    // El orden (mismo de la consigna) hace que (B−A)×(C−A) apunte a +Y en los
    // dos, así el culling (GL_CCW) mira la cara de arriba y descarta la de abajo.
    std::size_t t = 0;
    const auto idx = [fila](unsigned i, unsigned j) {
        return static_cast<unsigned>(i * fila + j);
    };
    for (unsigned i = 0; i < celdas; ++i) {
        for (unsigned j = 0; j < celdas; ++j) {
            const unsigned a = idx(i, j);
            const unsigned b = idx(i, j + 1);
            const unsigned d = idx(i + 1, j);
            const unsigned c = idx(i + 1, j + 1);
            data.indices[t++] = a; data.indices[t++] = d; data.indices[t++] = c;
            data.indices[t++] = a; data.indices[t++] = c; data.indices[t++] = b;
        }
    }

    return data;
}

}  // namespace terrain
