// ============================================================================
// NoiseHeights.cpp — Implementación del HeightField de ruido (FastNoiseLite).
// ============================================================================

#include "NoiseHeights.h"

namespace terrain {

NoiseHeights::NoiseHeights(int seed, float frequency, float amplitude, int octaves,
                           float base)
    : amplitud_(amplitude), base_(base)
{
    ruido_.SetSeed(seed);
    ruido_.SetNoiseType(FastNoiseLite::NoiseType_Perlin);
    ruido_.SetFractalType(FastNoiseLite::FractalType_FBm);
    ruido_.SetFractalOctaves(octaves);
    // La "ganancia"/"lagunaridad" por defecto (0.5 / 2.0) da la progresión clásica
    // de octavas; se dejan explícitas para documentar que son las que se esperan.
    ruido_.SetFractalGain(0.5f);
    ruido_.SetFractalLacunarity(2.0f);
    ruido_.SetFrequency(frequency);
}

float NoiseHeights::operator()(int nx, int nz) const
{
    // x = índice de columna (nx), y = índice de fila (nz). GetNoise devuelve
    // ~[-1, 1]; la amplitud es el tope nominal y el relieve de una ventana queda
    // por debajo de ese tope. La base lo desplaza al nivel del vuelo (no cambia
    // el relieve, solo la elevación media).
    return base_ + amplitud_ * ruido_.GetNoise(static_cast<float>(nx),
                                               static_cast<float>(nz));
}

}  // namespace terrain
