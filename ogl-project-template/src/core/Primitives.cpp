// ============================================================================
// Primitives.cpp — Implementación del generador de mallas paramétricas.
//
// Práctico 03: la tupla dejó de llevar color (pasa a uniform por pieza) y suma
// normal + tex_coords. Se agregan cilindro, cono y esfera junto al cubo.
//
// Convenciones (ver Primitives.h):
//   - Eje Y vertical; todas las primitivas quedan centradas en el origen.
//   - tex_coords: u = i/gajos (0..1, repite en la costura), v = anillo/anillos
//     con v creciendo en +Y: cilindro 0 (base) -> 1 (tapa), cono 0 (base) ->
//     1 (ápice), esfera 0 (polo sur) -> 1 (polo norte).
//   - Cada fila de anillo lleva N+1 vértices: el gajo final duplica el primero
//     (misma posición, u = 0 y u = 1) para "cerrar" la costura.
//   - Los cuadriláteros del lateral se emiten en CCW desde afuera partiendo el
//     cuadrilátero por la diagonal b-c (los triángulos a,b,c y b,c,d del doble
//     loop "naive" quedan INVERTIDOS y no lo muestra el culling, que está
//     deshabilitado; el test main_test lo valida con el producto vectorial).
//   - Zonas cerradas/costuras: ápice del cono y polos de la esfera son anillos
//     degenerados de radio 0 (N+1 vértices en un mismo punto, cada uno con su
//     normal propia); tapas y base NO comparten vértices con el lateral.
// ============================================================================

#include "Primitives.h"

#include <array>
#include <cmath>

#include <glm/glm.hpp>

namespace primitives {

namespace {

constexpr float kPi    = 3.14159265358979323846f;
constexpr float kTwoPi = 6.28318530717958647692f;

// Emite a `data` el fan de N triángulos de una tapa de radio `radio` en y = yc.
// dir = +1 para normal +Y, -1 para normal -Y (el giro del fan se invierte para
// que apunte a la normal saliente de la tapa). Las tex_coords proyectan el
// disco sobre [0,1]^2: uv = (0.5 + 0.5*x/radio, 0.5 + 0.5*z/radio).
void add_cap(MeshData& data, float yc, const glm::vec3& normal,
             float radio, unsigned gajos, int dir)
{
    const unsigned base = data.vertices.size();

    // Centro del disco.
    {
        Vertex v;
        v.position   = glm::vec3(0.0f, yc, 0.0f);
        v.normal     = normal;
        v.tex_coords = glm::vec2(0.5f, 0.5f);
        data.vertices.push_back(v);
    }

    // Borde del disco (N vértices, no comparte con el lateral).
    for (unsigned i = 0; i < gajos; ++i) {
        const float t  = kTwoPi * float(i) / float(gajos);
        const float c  = std::cos(t);
        const float s  = std::sin(t);
        Vertex v;
        v.position   = glm::vec3(radio * c, yc, radio * s);
        v.normal     = normal;
        v.tex_coords = glm::vec2(0.5f + 0.5f * c, 0.5f + 0.5f * s);
        data.vertices.push_back(v);
    }

    // Triángulos del fan: (centro, i, i+1) o al revés según la normal.
    for (unsigned i = 0; i < gajos; ++i) {
        const unsigned nxt = base + 1U + (i + 1U) % gajos;
        if (dir > 0) {
            data.indices.insert(data.indices.end(), {base, nxt, base + 1U + i});
        } else {
            data.indices.insert(data.indices.end(), {base, base + 1U + i, nxt});
        }
    }
}

}  // namespace

// ---------------------------------------------------------------------------
// Cubo
// ---------------------------------------------------------------------------
MeshData cube(float scale_x, float scale_y, float scale_z)
{
    MeshData data;
    data.vertices.reserve(24);
    data.indices.reserve(36);

    // Mitades del lado: [ -hx, hx ] x [ -hy, hy ] x [ -hz, hz ]
    const float hx = 0.5f * scale_x;
    const float hy = 0.5f * scale_y;
    const float hz = 0.5f * scale_z;

    // Cada cara: 4 vértices propios (CCW vistos desde fuera), todos con la
    // normal de la cara y con tex_coords (0,0),(1,0),(1,1),(0,1) en orden.
    auto push_face = [&](const glm::vec3& normal, const std::array<glm::vec3, 4>& p) {
        const float uv[4][2] = { {0.0f, 0.0f}, {1.0f, 0.0f}, {1.0f, 1.0f}, {0.0f, 1.0f} };
        const unsigned base = data.vertices.size();
        for (int k = 0; k < 4; ++k) {
            Vertex v;
            v.position   = p[k];
            v.normal     = normal;
            v.tex_coords = glm::vec2(uv[k][0], uv[k][1]);
            data.vertices.push_back(v);
        }
        data.indices.insert(data.indices.end(), {base, base+1, base+2,
                                                 base, base+2, base+3});
    };

    // Las 6 caras (orden CCW visto desde cada normal saliente).
    push_face({ 1.0f, 0.0f, 0.0f},
              std::array<glm::vec3, 4>{{ { hx, -hy,  hz}, { hx, -hy, -hz}, { hx,  hy, -hz}, { hx,  hy,  hz} }});
    push_face({-1.0f, 0.0f, 0.0f},
              std::array<glm::vec3, 4>{{ {-hx, -hy, -hz}, {-hx, -hy,  hz}, {-hx,  hy,  hz}, {-hx,  hy, -hz} }});
    push_face({0.0f, 1.0f, 0.0f},
              std::array<glm::vec3, 4>{{ {-hx,  hy, -hz}, {-hx,  hy,  hz}, { hx,  hy,  hz}, { hx,  hy, -hz} }});
    push_face({0.0f,-1.0f, 0.0f},
              std::array<glm::vec3, 4>{{ {-hx, -hy,  hz}, {-hx, -hy, -hz}, { hx, -hy, -hz}, { hx, -hy,  hz} }});
    push_face({0.0f, 0.0f, 1.0f},
              std::array<glm::vec3, 4>{{ {-hx, -hy,  hz}, { hx, -hy,  hz}, { hx,  hy,  hz}, {-hx,  hy,  hz} }});
    push_face({0.0f, 0.0f,-1.0f},
              std::array<glm::vec3, 4>{{ { hx, -hy, -hz}, {-hx, -hy, -hz}, {-hx,  hy, -hz}, { hx,  hy, -hz} }});

    return data;
}

// ---------------------------------------------------------------------------
// Cilindro con tapas
// ---------------------------------------------------------------------------
MeshData cylinder(float radio, float largo, unsigned gajos, unsigned anillos)
{
    MeshData data;
    const unsigned N = gajos;
    const unsigned M = anillos;
    const unsigned rows = M + 1U;
    const unsigned cols = N + 1U;

    data.vertices.reserve(rows * cols + 2U * (N + 1U));
    data.indices.reserve(6U * N * M + 6U * N);

    // Lateral: anillos de y = -largo/2 (v=0) a y = +largo/2 (v=1).
    for (unsigned r = 0; r < rows; ++r) {
        const float v   = float(r) / float(M);
        const float y   = -0.5f * largo + largo * v;
        for (unsigned i = 0; i < cols; ++i) {
            const float u = float(i) / float(N);
            const float t = kTwoPi * u;
            const float c = std::cos(t);
            const float s = std::sin(t);

            Vertex vert;
            vert.position   = glm::vec3(radio * c, y, radio * s);
            vert.normal     = glm::vec3(c, 0.0f, s);      // radial en XZ
            vert.tex_coords = glm::vec2(u, v);
            data.vertices.push_back(vert);
        }
    }

    // Índices del lateral. El cuadrilátero (a,c,b) y (b,c,d) (diagonal b-c)
    // queda en CCW visto desde la normal saliente; el doble loop "naive"
    // habría dado (a,b,c) y (b,d,c), que quedan invertidos.
    for (unsigned r = 0; r < M; ++r) {
        for (unsigned i = 0; i < N; ++i) {
            const unsigned a = r * cols + i;
            const unsigned b = a + 1U;
            const unsigned c = a + cols;
            const unsigned d = c + 1U;
            data.indices.insert(data.indices.end(), {a, c, b,
                                                     b, c, d});
        }
    }

    // Tapas: tapa superior +Y y tapa inferior -Y, sin compartir vértices.
    add_cap(data,  0.5f * largo, glm::vec3( 0.0f, 1.0f, 0.0f), radio, N, +1);
    add_cap(data, -0.5f * largo, glm::vec3( 0.0f,-1.0f, 0.0f), radio, N, -1);

    return data;
}

// ---------------------------------------------------------------------------
// Cono (base abajo, ápice arriba)
// ---------------------------------------------------------------------------
MeshData cone(float radio, float conicidad, unsigned gajos, unsigned anillos)
{
    MeshData data;
    const unsigned N = gajos;
    const unsigned M = anillos;
    const unsigned rows = M + 1U;
    const unsigned cols = N + 1U;

    // conicidad se interpreta en GRADOS: ángulo de apertura total en el ápice;
    // el lateral forma (conicidad/2) con el eje Y. Altura del cono resultante:
    // conforme el cono se abre (conicidad crece), la punta queda más lejos.
    const float semi    = 0.5f * glm::radians(conicidad);
    const float tanSemi = std::tan(semi);
    const float h       = radio / tanSemi;

    data.vertices.reserve(rows * cols + (N + 1U));
    data.indices.reserve(6U * N * M + 3U * N);

    // Lateral: filas de y = -h/2 (base, radio) a y = +h/2 (ápice, radio 0).
    // El último anillo es el ápice DEGENERADO: N+1 vértices en el mismo punto,
    // cada uno con la normal lateral de su gajo.
    for (unsigned r = 0; r < rows; ++r) {
        const float v   = float(r) / float(M);
        const float y   = -0.5f * h + h * v;
        const float rad = radio * (1.0f - v);
        for (unsigned i = 0; i < cols; ++i) {
            const float u = float(i) / float(N);
            const float t = kTwoPi * u;
            const float c = std::cos(t);
            const float s = std::sin(t);

            Vertex vert;
            vert.position   = glm::vec3(rad * c, y, rad * s);
            // Normal lateral saliente: inclinada por el semiángulo respecto del
            // eje, deducida del gradiente de la superficie rho = tan(semi)*(h/2-y).
            vert.normal     = glm::normalize(glm::vec3(c, tanSemi, s));
            vert.tex_coords = glm::vec2(u, v);
            data.vertices.push_back(vert);
        }
    }

    // Mismo patrón de índices y diagonales que el cilindro.
    for (unsigned r = 0; r < M; ++r) {
        for (unsigned i = 0; i < N; ++i) {
            const unsigned a = r * cols + i;
            const unsigned b = a + 1U;
            const unsigned c = a + cols;
            const unsigned d = c + 1U;
            data.indices.insert(data.indices.end(), {a, c, b,
                                                     b, c, d});
        }
    }

    // Base: tapa en y = -h/2 con normal -Y, sin compartir vértices.
    add_cap(data, -0.5f * h, glm::vec3(0.0f, -1.0f, 0.0f), radio, N, -1);

    return data;
}

// ---------------------------------------------------------------------------
// Esfera (polos en el eje Y)
// ---------------------------------------------------------------------------
MeshData sphere(float radio, unsigned gajos, unsigned anillos)
{
    MeshData data;
    const unsigned N = gajos;
    const unsigned M = anillos;
    const unsigned rows = M + 1U;
    const unsigned cols = N + 1U;

    data.vertices.reserve(rows * cols);
    data.indices.reserve(6U * N * M);

    // Filas de latitud: v = 0 en el polo sur (colatitud phi = pi) hasta v = 1
    // en el polo norte (phi = 0). Meridianos con costura en u = 1.
    for (unsigned r = 0; r < rows; ++r) {
        const float v    = float(r) / float(M);
        const float phi  = kPi * (1.0f - v);
        const float sp   = std::sin(phi);
        const float cp   = std::cos(phi);
        for (unsigned i = 0; i < cols; ++i) {
            const float u = float(i) / float(N);
            const float t = kTwoPi * u;
            const float c = std::cos(t);
            const float s = std::sin(t);

            Vertex vert;
            vert.position   = glm::vec3(radio * sp * c, radio * cp, radio * sp * s);
            vert.normal     = glm::normalize(vert.position);  // radial desde el origen
            vert.tex_coords = glm::vec2(u, v);
            data.vertices.push_back(vert);
        }
    }

    // Mismo patrón de índices que cilindro/cono. Los polos (primera y última
    // fila) son anillos degenerados: los triángulos de esas filas tienen área
    // nula y no pintan nada.
    for (unsigned r = 0; r < M; ++r) {
        for (unsigned i = 0; i < N; ++i) {
            const unsigned a = r * cols + i;
            const unsigned b = a + 1U;
            const unsigned c = a + cols;
            const unsigned d = c + 1U;
            data.indices.insert(data.indices.end(), {a, c, b,
                                                     b, c, d});
        }
    }

    return data;
}

}  // namespace primitives