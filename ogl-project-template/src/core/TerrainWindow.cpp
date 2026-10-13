// ============================================================================
// TerrainWindow.cpp — Implementación de la ventana deslizante de terreno.
// Ver TerrainWindow.h para el diseño (reparto del recentrado entre cuadros).
// ============================================================================

#include "TerrainWindow.h"

#include <algorithm>  // std::min, std::max
#include <cstdlib>    // std::abs (int)
#include <limits>     // std::numeric_limits

#include <glm/gtc/matrix_transform.hpp>  // glm::translate

namespace {

// Tope de una fase: no pasarse del total (evita el desbordamiento del contador
// cuando la primera ventana se arma entera con budget "infinito").
std::size_t limit(std::size_t cursor, std::size_t presupuesto, std::size_t total)
{
    return (presupuesto >= total - cursor) ? total : cursor + presupuesto;
}

}  // namespace

void TerrainWindow::init(unsigned celdas, float paso, const terrain::HeightField& h,
                         float center_x, float center_z, unsigned presupuesto)
{
    celdas_ = celdas;
    n_ = celdas + 1u;
    paso_ = paso;
    h_ = &h;
    presupuesto_ = presupuesto;

    data_.vertices.resize(static_cast<std::size_t>(n_) * n_);
    data_.indices.resize(6u * celdas_ * celdas_);
    H_.resize(static_cast<std::size_t>(n_) * n_);

    // ---- Topología (índices): se arma UNA sola vez. Mismo orden que build_grid
    // ---- (CCW visto desde arriba): el culling dibuja la cara de arriba.
    std::size_t t = 0;
    const auto idx = [this](unsigned i, unsigned j) {
        return static_cast<unsigned>(i * n_ + j);
    };
    for (unsigned i = 0; i < celdas_; ++i) {
        for (unsigned j = 0; j < celdas_; ++j) {
            const unsigned a = idx(i, j);
            const unsigned b = idx(i, j + 1);
            const unsigned d = idx(i + 1, j);
            const unsigned c = idx(i + 1, j + 1);
            data_.indices[t++] = a; data_.indices[t++] = d; data_.indices[t++] = c;
            data_.indices[t++] = a; data_.indices[t++] = c; data_.indices[t++] = b;
        }
    }

    // ---- Primera ventana: sincrónica (una sola vez al arrancar) ----
    ox_ = terrain::window_origin(center_x, paso_, celdas_);
    oz_ = terrain::window_origin(center_z, paso_, celdas_);
    px_ = ox_;
    pz_ = oz_;
    phase_ = Phase::kHeights;
    cursor_ = 0;
    const std::size_t infinito = std::numeric_limits<std::size_t>::max();
    fill_heights(infinito);
    fill_vertices(infinito);

    // Rango de alturas de la ventana inicial.
    if (!H_.empty()) {
        stats_.min_h = stats_.max_h = H_[0];
        for (float y : H_) {
            stats_.min_h = std::min(stats_.min_h, y);
            stats_.max_h = std::max(stats_.max_h, y);
        }
    }

    mesh_.load(data_);
}

void TerrainWindow::fill_heights(std::size_t presupuesto)
{
    const std::size_t total = static_cast<std::size_t>(n_) * n_;
    const std::size_t fin = limit(cursor_, presupuesto, total);
    for (; cursor_ < fin; ++cursor_) {
        const unsigned i = static_cast<unsigned>(cursor_ / n_);
        const unsigned j = static_cast<unsigned>(cursor_ % n_);
        H_[cursor_] = (*h_)(px_ + static_cast<int>(j), pz_ + static_cast<int>(i));
    }
    if (cursor_ >= total) {
        phase_ = Phase::kVertices;
        cursor_ = 0;
    }
}

void TerrainWindow::fill_vertices(std::size_t presupuesto)
{
    const std::size_t total = static_cast<std::size_t>(n_) * n_;
    const std::size_t fin = limit(cursor_, presupuesto, total);
    const float inv_celdas = 1.0f / static_cast<float>(celdas_);
    for (; cursor_ < fin; ++cursor_) {
        const unsigned i = static_cast<unsigned>(cursor_ / n_);
        const unsigned j = static_cast<unsigned>(cursor_ % n_);

        Vertex v;
        v.position = glm::vec3(static_cast<float>(j) * paso_, H_[cursor_],
                               static_cast<float>(i) * paso_);
        v.normal = terrain::grid_normal(H_, celdas_, i, j, paso_);
        v.tex_coords = glm::vec2(static_cast<float>(j) * inv_celdas,
                                 static_cast<float>(i) * inv_celdas);
        data_.vertices[cursor_] = v;
    }
    if (cursor_ >= total) {
        phase_ = Phase::kIdle;
    }
}

bool TerrainWindow::update(float center_x, float center_z)
{
    // ¿Hay que empezar un recentrado? Solo si estamos quietos y el objetivo se
    // fue más allá del margen (en cualquier eje).
    if (phase_ == Phase::kIdle) {
        const int tx = terrain::window_origin(center_x, paso_, celdas_);
        const int tz = terrain::window_origin(center_z, paso_, celdas_);
        if (std::abs(tx - ox_) > kMargenCeldas ||
            std::abs(tz - oz_) > kMargenCeldas) {
            px_ = tx;
            pz_ = tz;
            cursor_ = 0;
            phase_ = Phase::kHeights;
        }
    }

    // Avanzar la fase en curso con el presupuesto del cuadro.
    if (phase_ == Phase::kHeights) fill_heights(presupuesto_);
    if (phase_ == Phase::kVertices) fill_vertices(presupuesto_);

    // Recién cuando la ventana nueva está COMPLETA se sube (un solo upload) y se
    // cambia el origen: nunca se dibuja una malla a medio armar (sin costuras).
    if (phase_ == Phase::kIdle && (ox_ != px_ || oz_ != pz_)) {
        mesh_.update_vertices(data_.vertices);
        ox_ = px_;
        oz_ = pz_;
        return true;
    }
    return false;
}

glm::mat4 TerrainWindow::model() const
{
    return glm::translate(glm::mat4(1.0f),
                          glm::vec3(static_cast<float>(ox_) * paso_, 0.0f,
                                    static_cast<float>(oz_) * paso_));
}
