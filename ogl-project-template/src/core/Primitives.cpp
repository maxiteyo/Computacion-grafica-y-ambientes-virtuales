// ============================================================================
// Primitives.cpp — Implementación del generador de mallas (cubo).
//
// El cubo se define como un hexaedro de 6 caras; cada cara aporta 4 vértices
// únicos y 2 triángulos (6 índices). Total: 6 * 4 = 24 vértices y 6 * 6 = 36
// índices. Los 4 vértices de cada cara se recorren en sentido antihorario (CCW)
// mirando la cara desde fuera (normales salientes), condición que luego permite
// el descarte de caras traseras.
// ============================================================================

#include "Primitives.h"

namespace primitives {

MeshData cube(float scale_x, float scale_y, float scale_z)
{
    MeshData data;
    data.vertices.reserve(24);
    data.indices.reserve(36);

    // Mitades del lado del cubo unitario: [ -hx, hx ] x [ -hy, hy ] x [ -hz, hz ]
    const float hx = 0.5f * scale_x;
    const float hy = 0.5f * scale_y;
    const float hz = 0.5f * scale_z;

    // Cada cara: normal subyacente (para elegir los colores) y sus 4 vértices
    // en CCW mirando la cara desde fuera. Se usa un único array por cara para
    // mantener 24 vértices únicos.
    //
    // Nota sobre el orden: en OpenGL el frente (front face) por defecto es CCW
    // en NDC. Las coordenadas de los vértices se listan en CCW vista desde la
    // normal saliente; las dos siguientes caras quedan así. El color se elige
    // por cara (rojo=+X, verde=+Y, azul=+Z y sus negativos), de modo que la
    // vista isométrica del main muestra tres colores primarios.

    // Cara +X (normal (1,0,0)) -> rojo
    {
        const Vertex v[4] = {
            {  hx, -hy,  hz, 1.0f, 0.0f, 0.0f },
            {  hx, -hy, -hz, 1.0f, 0.0f, 0.0f },
            {  hx,  hy, -hz, 1.0f, 0.0f, 0.0f },
            {  hx,  hy,  hz, 1.0f, 0.0f, 0.0f },
        };
        const unsigned int base = static_cast<unsigned int>(data.vertices.size());
        data.vertices.insert(data.vertices.end(), v, v + 4);
        data.indices.insert(data.indices.end(), {base, base+1, base+2, base, base+2, base+3});
    }
    // Cara -X (normal (-1,0,0)) -> cyan
    {
        const Vertex v[4] = {
            { -hx, -hy, -hz, 0.0f, 1.0f, 1.0f },
            { -hx, -hy,  hz, 0.0f, 1.0f, 1.0f },
            { -hx,  hy,  hz, 0.0f, 1.0f, 1.0f },
            { -hx,  hy, -hz, 0.0f, 1.0f, 1.0f },
        };
        const unsigned int base = static_cast<unsigned int>(data.vertices.size());
        data.vertices.insert(data.vertices.end(), v, v + 4);
        data.indices.insert(data.indices.end(), {base, base+1, base+2, base, base+2, base+3});
    }
    // Cara +Y (normal (0,1,0)) -> verde
    {
        const Vertex v[4] = {
            { -hx,  hy, -hz, 0.0f, 1.0f, 0.0f },
            { -hx,  hy,  hz, 0.0f, 1.0f, 0.0f },
            {  hx,  hy,  hz, 0.0f, 1.0f, 0.0f },
            {  hx,  hy, -hz, 0.0f, 1.0f, 0.0f },
        };
        const unsigned int base = static_cast<unsigned int>(data.vertices.size());
        data.vertices.insert(data.vertices.end(), v, v + 4);
        data.indices.insert(data.indices.end(), {base, base+1, base+2, base, base+2, base+3});
    }
    // Cara -Y (normal (0,-1,0)) -> magenta
    {
        const Vertex v[4] = {
            { -hx, -hy,  hz, 1.0f, 0.0f, 1.0f },
            { -hx, -hy, -hz, 1.0f, 0.0f, 1.0f },
            {  hx, -hy, -hz, 1.0f, 0.0f, 1.0f },
            {  hx, -hy,  hz, 1.0f, 0.0f, 1.0f },
        };
        const unsigned int base = static_cast<unsigned int>(data.vertices.size());
        data.vertices.insert(data.vertices.end(), v, v + 4);
        data.indices.insert(data.indices.end(), {base, base+1, base+2, base, base+2, base+3});
    }
    // Cara +Z (normal (0,0,1)) -> azul
    {
        const Vertex v[4] = {
            { -hx, -hy,  hz, 0.0f, 0.0f, 1.0f },
            {  hx, -hy,  hz, 0.0f, 0.0f, 1.0f },
            {  hx,  hy,  hz, 0.0f, 0.0f, 1.0f },
            { -hx,  hy,  hz, 0.0f, 0.0f, 1.0f },
        };
        const unsigned int base = static_cast<unsigned int>(data.vertices.size());
        data.vertices.insert(data.vertices.end(), v, v + 4);
        data.indices.insert(data.indices.end(), {base, base+1, base+2, base, base+2, base+3});
    }
    // Cara -Z (normal (0,0,-1)) -> amarillo
    {
        const Vertex v[4] = {
            {  hx, -hy, -hz, 1.0f, 1.0f, 0.0f },
            { -hx, -hy, -hz, 1.0f, 1.0f, 0.0f },
            { -hx,  hy, -hz, 1.0f, 1.0f, 0.0f },
            {  hx,  hy, -hz, 1.0f, 1.0f, 0.0f },
        };
        const unsigned int base = static_cast<unsigned int>(data.vertices.size());
        data.vertices.insert(data.vertices.end(), v, v + 4);
        data.indices.insert(data.indices.end(), {base, base+1, base+2, base, base+2, base+3});
    }

    return data;
}

}  // namespace primitives