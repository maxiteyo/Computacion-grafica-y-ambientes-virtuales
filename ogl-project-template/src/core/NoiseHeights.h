// ============================================================================
// NoiseHeights.h — HeightField basado en ruido procedural (FastNoiseLite).
//
// Es una de las posibles fuentes de altura del Terrain: implementa la interfaz
// HeightField evaluando el ruido en los nodos del mundo. La malla (Terrain) NO
// conoce esta clase: solo ve la interfaz. Cambiar el ruido por un archivo o por
// una función analítica no toca la generación de la malla.
//
// Decisiones:
//   - Ruido Perlin con FBm de 5 octavas: el ruido "de una sola octava" es
//     demasiado suave; sumando octavas (cada una al doble de frecuencia y a la
//     mitad de amplitud) el relieve tiene rugosidad a varias escalas, como un
//     terreno real. FastNoiseLite normaliza la FBm a ~[-1, 1].
//   - GetNoise(x, y) toma x = índice de columna (nx) e y = índice de fila (nz):
//     la frecuencia se expresa en "ciclos por nodo" del heightmap. Una frecuencia
//     de 0.01 hace que una "loma" ocupe ~100 celdas (con paso 25 m, ~2.5 km).
//   - Semilla fija: el terreno es DETERMINISTA (misma semilla -> mismas alturas),
//     lo que hace el ruido testeable (dos instancias iguales dan lo mismo).
// ============================================================================
#pragma once

#include "Terrain.h"

#include "FastNoiseLite.h"  // header-only (third_party/FastNoiseLite)

namespace terrain {

class NoiseHeights : public HeightField {
public:
    // seed      : semilla del ruido (determinista).
    // frequency : frecuencia base de la primera octava (ciclos por nodo).
    // amplitude : tope nominal de |altura| [m]. La FBm normaliza a ~[-1,1], pero
    //             una ventana concreta no llega al extremo (con 800 m se midió
    //             ~[-600, 520] m).
    // octaves   : cantidad de octavas de la FBm.
    // base      : ELEVACIÓN MEDIA del terreno [m] (nivel del "suelo"). Se suma al
    //             relieve: y = base + amplitud · ruido. No afecta las normales (la
    //             base cancela en las diferencias centradas) ni la continuidad de
    //             la ventana. Se usa para colocar el mundo a la altura del vuelo
    //             (el avión arranca en el trim de 5000 m): con base = 4200 m las
    //             cumbres quedan a ~300 m del avión, en vez de a ~4.5 km.
    NoiseHeights(int seed, float frequency, float amplitude, int octaves = 5,
                 float base = 0.0f);

    // Altura [m] en el nodo del mundo (nx, nz): base + relieve.
    float operator()(int nx, int nz) const override;

private:
    FastNoiseLite ruido_;
    float amplitud_ = 1.0f;
    float base_ = 0.0f;   // elevación media del terreno [m]
};

}  // namespace terrain
