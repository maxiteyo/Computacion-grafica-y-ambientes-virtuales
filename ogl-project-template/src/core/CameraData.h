// ============================================================================
// CameraData.h — Datos de la cámara para completar el proceso de proyección.
//
// Estructura de datos COMPARTIDA entre módulos (ver arquitectura de la cátedra:
// "solo datos, sin lógica"): CameraSystem la rellena por cuadro y el Renderer /
// main la lee para setear los uniforms del vertex shader.
//   - view       : V (mundo -> cámara). La cámara siempre mira al objetivo.
//   - projection : P (cámara -> clip). Perspectiva: hay división por w.
//   gl_Position = projection * view * model * vec4(aPos, 1.0);
// ============================================================================
#pragma once

#include <glm/glm.hpp>

struct CameraData {
    glm::mat4 view       = glm::mat4(1.0f);
    glm::mat4 projection = glm::mat4(1.0f);
};