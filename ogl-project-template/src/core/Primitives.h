// ============================================================================
// Primitives.h — Generador de mallas de figuras primitivas (lado CPU, C++ puro).
//
// Cada función devuelve una MeshData (tuplas + índices) usando la tupla
// { position, normal, tex_coords } del Práctico 03. El color NO se genera acá:
// es decisión del llamador (uniform del shader por pieza).
//
// Decisiones y convenciones comunes (documentar junto con cada primitiva):
//   - Funciones libres en un namespace (NO una clase): el generador no posee
//     ningún recurso, solo produce datos. "¿Qué posee este objeto?" → nada,
//     y por eso no es una clase.
//   - Sistema de referencia: eje Y vertical. El "largo" de cilindro y cono se
//     reparte simétricamente: y = -largo/2 .. +largo/2 (primitivas ya
//     centradas en el origen, como el cubo).
//   - Los triángulos se emiten en sentido antihorario (CCW) mirando la cara
//     desde FUERA (normales salientes). Dura de verificar sin culling: el
//     test main_test lo valida con el producto vectorial (B-A)x(C-A) paralelo
//     a la normal del primer vértice del triángulo.
//   - Cualquier punto donde la superficie se "cierra" o se "cose" genera
//     vértices duplicados con sus propias normales y tex_coords:
//       * costura del cilindro/cono/esfera (u = 0 y u = 1 en el mismo lugar);
//       * tapas del cilindro y base del cono (NUNCA comparten vértices con el
//         lateral);
//       * ápice del cono y polos de la esfera (anillos degenerados de radio 0:
//         N+1 vértices en el mismo punto, cada uno con la normal de su gajo).
//   - tex_coords de los laterales: u = i/gajos (0..1, repite en la costura),
//     v = anillo/anillos (0..1 a lo largo del eje). En las tapas/disco se
//     proyecta el círculo al cuadrado unitario:
//     uv = (0.5 + 0.5*(x/radio), 0.5 + 0.5*(z/radio)).
// ============================================================================
#pragma once

#include "MeshData.h"

namespace primitives {

// Cubo de lado unitario (por defecto), centrado en el origen.
// 24 vértices únicos + 36 índices. Cada cara tiene sus 4 vértices propios con
// la normal de la cara y tex_coords (0,0),(1,0),(1,1),(0,1) en CCW.
// Los ejes X, Y, Z se escalan por los argumentos (cubo o placa).
MeshData cube(float scale_x = 1.0f, float scale_y = 1.0f, float scale_z = 1.0f);

// Cilindro de revolución con costura, cerrado con dos tapas.
//   radio  : radio del lateral (y de las tapas).
//   largo  : extensión en Y (de -largo/2 a +largo/2).
//   gajos  : N sectores alrededor del eje (>= 3).
//   anillos: cant. de bandas a lo largo del eje (>= 1); controla la densidad
//            de vértices. Cada anillo lleva N+1 vértices (el gajo de la
//            costura duplica el vértice con u = 0 y u = 1).
// Conteos con anillos = 1: lateral 2*(N+1) vértices + 2 tapas de N+1 vértices;
// índices: 2N triángulos de lateral + N + N de tapas.
// Normales: lateral radial (cos t, 0, sin t); tapas axiales (0, +1, 0) y
// (0, -1, 0) (las tapas no comparten vértices con el lateral).
MeshData cylinder(float radio, float largo, unsigned gajos, unsigned anillos = 1U);

// Cono (base en y = -h/2, ápice en y = +h/2, eje Y).
//   radio     : radio de la base.
//   conicidad : ángulo de apertura TOTAL del cono, en GRADOS (ver abajo).
//   gajos     : N sectores alrededor del eje (>= 3).
//   anillos   : cant. de bandas a lo largo del eje (>= 1).
// Conicidad en grados: el lateral forma un ángulo (conicidad/2) con el eje Y.
// La altura resulta h = radio / tan(radianes(conicidad)/2): cuanto más se abre
// el cono, más lejos queda la punta. (Se elige grados en la interfaz por
// lectura natural; adentro se convierte con glm::radians.)
// Ápice: anillo degenerado (N+1 vértices en (0, +h/2, 0)), cada uno hereda la
// normal lateral de su gajo: normalize(cos t, tan(conicidad/2), sin t).
// Base: tapa en y = -h/2 con normal (0, -1, 0), sin compartir vértices.
MeshData cone(float radio, float conicidad, unsigned gajos, unsigned anillos = 1U);

// Esfera de radio dado, centrada en el origen, eje polar Y.
//   gajos  : N meridianos (columnas alrededor del eje, >= 3).
//   anillos: M paralelos de latitud (>= 2, sin contar los polos degenerados).
// Polos: anillos degenerados (N+1 vértices cada uno, normal = posición
// normalizada); costura de meridianos en u = 1. tex_coords: v = 0 en el polo
// SUR, v = 1 en el polo norte (convención clásica de mapas de globo).
// Conteos: (M+1)*(N+1) vértices, 2*M*N triángulos (6*M*N índices).
MeshData sphere(float radio, unsigned gajos, unsigned anillos);

}  // namespace primitives