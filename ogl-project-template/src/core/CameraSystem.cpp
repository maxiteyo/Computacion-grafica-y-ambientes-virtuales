// ============================================================================
// CameraSystem.cpp — Implementación de la cámara orbital (Práctico 05).
// Ver CameraSystem.h para el diseño y las decisiones de acotamiento.
// ============================================================================

#include <algorithm>  // std::clamp
#include <cmath>      // std::cos, std::sin

#include <glm/gtc/matrix_transform.hpp>  // glm::lookAt, glm::perspective, radians

#include "CameraSystem.h"

CameraSystem::CameraSystem(int width, int height)
{
    set_viewport(width, height);
}

void CameraSystem::update(const glm::vec3& objetivo,
                          const glm::vec3& /*angulos_avion*/,
                          const CameraCommand& cmd)
{
    // 1) Acumular los deltas del usuario sobre el estado de la órbita.
    yaw_   += cmd.yaw_delta;
    pitch_ += cmd.pitch_delta;
    distancia_ += cmd.dist_delta;

    // 2) Acotar el estado (ver fundamentos en el header).
    pitch_ = std::clamp(pitch_, -glm::radians(0.9f * 90.0f),
                        glm::radians(0.9f * 90.0f));
    distancia_ = std::clamp(distancia_, kMinDist, kMaxDist);

    // 3) Esféricas -> cartesianas y matriz de vista.
    rebuild_view(objetivo);
}

void CameraSystem::set_orbit(float yaw, float pitch, float dist)
{
    // Mismos clamps que update(): el estado nunca sale del rango válido. No se
    // rearma la vista acá porque no se conoce el objetivo; la próxima llamada a
    // update() (en el mismo cuadro) la reconstruye con el objetivo correcto.
    yaw_ = yaw;
    pitch_ = std::clamp(pitch, -glm::radians(0.9f * 90.0f),
                        glm::radians(0.9f * 90.0f));
    distancia_ = std::clamp(dist, kMinDist, kMaxDist);
}

void CameraSystem::set_viewport(int width, int height)
{
    // Ventana minimizada: GLFW puede reportar 0 (o incluso negativo) de alto.
    // aspect = width/height quedaría indefinido (división por cero): la
    // comprobación conviene ponerse ACÁ, en el único punto que consume la
    // relación de aspecto, y no en cada llamador del callback.
    width_  = std::max(width, 1);
    height_ = std::max(height, 1);
    rebuild_projection();
}

void CameraSystem::rebuild_projection()
{
    const float aspect = static_cast<float>(width_) / static_cast<float>(height_);
    data_.projection = glm::perspective(glm::radians(kFovYDeg), aspect,
                                        kNear, kFar);
}

void CameraSystem::rebuild_view(const glm::vec3& objetivo)
{
    const float cp = std::cos(pitch_);
    const glm::vec3 eye = objetivo
        + distancia_ * glm::vec3(cp * std::cos(yaw_),
                                 std::sin(pitch_),
                                 cp * std::sin(yaw_));
    data_.view = glm::lookAt(eye, objetivo, glm::vec3(0.0f, 1.0f, 0.0f));
}