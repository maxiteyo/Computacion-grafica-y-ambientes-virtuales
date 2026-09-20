// ============================================================================
// RenderItem.h — Item listo para dibujar (Práctico 04).
//
// Es la única información que el main necesita por cuadro para dibujar una
// pieza: la malla referenciada (NO es propia: la posee Aircraft), la
// transformación modelo->mundo YA compuesta (pose * local) y su color plano.
// El resto del programa no sabe cuántas piezas tiene el avión ni cómo se arma.
// ============================================================================
#pragma once

#include <glm/glm.hpp>

#include "Mesh.h"

struct RenderItem {
    const Mesh* mesh  = nullptr;          // malla a dibujar (referencia, no es propia)
    glm::mat4   model = glm::mat4(1.0f);  // modelo -> mundo, YA compuesta
    glm::vec4   color = glm::vec4(1.0f);  // color plano del objeto (uniform)
};