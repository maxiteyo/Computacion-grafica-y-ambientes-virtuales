// ============================================================================
// FlightData.cpp — Implementación de la frontera NED -> escena (Práctico 06).
//
// La base NED -> mundo (kT) es la UNICA transformación de posición del
// proyecto. La misma base la usa Aircraft.cpp para componer la actitud con el
// referencial del modelo (allí también se define la base del modelo, kA).
//
// Convenios que se verifican acá:
//   - Rumbo 0 (norte): el morro apunta a −Z del mundo.
//   - Rumbo 90° (este): el morro apunta a +X.
//   - Pitch + (nariz arriba): el morro sube en +Y.
//   - Roll + (ala derecha abajo): el ala derecha baja en −Y.
// (Estos invariantes los comprueban los tests: ver Test 16 en main_test.cpp.)
// ============================================================================

#include "FlightData.h"

#include <glm/gtc/matrix_transform.hpp>  // glm::eulerAngleXYZ

namespace fp {

namespace {

// Base NED -> mundo (matriz de cambio de base, column-major por columnas):
// las columnas son la IMAGEN de cada eje del NED en coordenadas de mundo:
//   norte (1,0,0) -> (0, 0, −1)   este (0,1,0) -> (1, 0, 0)
//   abajo (0,0,1) -> (0, −1, 0)   (arriba = −abajo = +Y del mundo)
const glm::mat3 kT(glm::vec3(0.0f, 0.0f, -1.0f),   // col 0: norte -> −Z
                   glm::vec3(1.0f, 0.0f, 0.0f),    // col 1: este  -> +X
                   glm::vec3(0.0f, -1.0f, 0.0f));  // col 2: abajo -> −Y

// Rotación cuerpo -> NED del convenio aeroespacial (Stevens-Lewis):
//   R = Rz(psi) · Ry(theta) · Rx(phi)   (GLM column-major: Rx se aplica
// primero al vector, luego pitch, luego yaw). Con ella el rumbo y el cabeceo
// definen la dirección del morro (independiente del alabeo) y el roll positivo
// baja el ala derecha.
glm::mat3 body_to_ned(float phi, float theta, float psi)
{
    const glm::mat4 Rz = glm::rotate(glm::mat4(1.0f), psi,   glm::vec3(0.0f, 0.0f, 1.0f));
    const glm::mat4 Ry = glm::rotate(glm::mat4(1.0f), theta, glm::vec3(0.0f, 1.0f, 0.0f));
    const glm::mat4 Rx = glm::rotate(glm::mat4(1.0f), phi,   glm::vec3(1.0f, 0.0f, 0.0f));
    return glm::mat3(Rz * Ry * Rx);
}

}  // namespace

FlightData to_world(const dlfdm::AircraftState& ned)
{
    FlightData fd;
    fd.position = glm::vec3(ned.inertial_position.y,     // x_gl = este
                            -ned.inertial_position.z,    // y_gl = −abajo = arriba
                            -ned.inertial_position.x);   // z_gl = −norte = sur
    fd.phi   = ned.phi;
    fd.theta = ned.theta;
    fd.psi   = ned.psi;
    return fd;
}

glm::mat4 to_world_basis()
{
    return glm::mat4(kT);
}

glm::vec3 forward_world(const dlfdm::AircraftState& ned)
{
    // Eje +X del cuerpo (morro) en el NED, y de ahí al mundo con la base kT.
    const glm::vec3 f_ned = body_to_ned(ned.phi, ned.theta, ned.psi)
                            * glm::vec3(1.0f, 0.0f, 0.0f);
    return kT * f_ned;
}

}  // namespace fp