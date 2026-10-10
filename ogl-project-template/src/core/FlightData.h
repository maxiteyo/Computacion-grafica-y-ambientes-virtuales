// ============================================================================
// FlightData.h — Estructura de datos compartida entre el FDM y la escena.
//
// Responsabilidad (Práctico 06): es la "frontera" entre los dos mundos:
//   - El FDM (libs/dlfdm) trabaja en el sistema NED (North East Down):
//     x = norte, y = este, z = hacia abajo, y con la actitud (phi, theta,
//     psi) en el convenio aeroespacial.
//   - La escena (OpenGL) usa +X este, +Y arriba y +Z hacia el sur (−norte).
//
// to_world() es la ÚNICA transformación NED -> escena del proyecto: si en el
// futuro hiciera falta otra conversión se agrega acá, no se copian signos
// sueltos en cada módulo. Para que la terna siga siendo dextrógira se niegan
// DOS de los tres ejes (nunca uno solo, que produciría una reflexión y un
// mundo espejado):
//     x_gl = este          =  +ned.y
//     y_gl = arriba        = −ned.z   (z del NED es "down")
//     z_gl = sur (-norte)  = −ned.x
//   -> posición_gl = (ned.y, −ned.z, −ned.x).
//
// Actitud: to_world() devuelve la actitud NED TAL CUAL (phi, theta, psi).
// La adaptación al referencial del modelo de la escena (morro −X, +Y ala
// derecha, +Z arriba) NO se hace copiando signos acá: es una base constante
// que aplica Aircraft al componer la rotación (ver Aircraft.cpp, "base de
// referencial"). El resultado visual es equivalente al del sign-copy de la
// consigna, pero esa fórmula presupone un modelo con el morro en +X y el
// nuestro lo tiene en −X; con una base constante no hay que extraer ángulos
// equivalentes (que degeneraría en ciertas actitudes).
//
// forward_world() es una consulta auxiliar: dirección del morro en el mundo
// para el estado NED dado. Usa EXACTAMENTE la misma base que Aircraft::update
// (ver Aircraft.cpp), por lo que sirve para verificar en tests que el avión
// vuela "de morro" y, más adelante, para la vista de cabina.
//
// Qué NO hace: no conoce el FDM (solo usa dlfdm::AircraftState, una estructura
// de datos), no toca OpenGL ni la escena. Módulo C++ puro, testeable sin GPU.
// ============================================================================
#pragma once

#include <glm/glm.hpp>

#include <dlfdm/defines.h>  // dlfdm::AircraftState

namespace fp {  // fp = flight data (mundo de la escena)

// Datos de la aeronave expresados en el sistema de coordenadas de la escena.
struct FlightData {
    glm::vec3 position;  // [m] posición (punto de referencia del modelo)
    float phi   = 0.0f;  // [rad] roll  (NED, sin adaptar: la aplica la escena)
    float theta = 0.0f;  // [rad] pitch
    float psi   = 0.0f;  // [rad] yaw / rumbo
};

// Convierte el estado NED del FDM (posición y actitud) al mundo de la escena.
FlightData to_world(const dlfdm::AircraftState& ned);

// Matriz 4x4 (homogénea) del cambio de base NED -> mundo. Es la ÚNICA fuente
// de la conversión de ejes: la usan to_world()/forward_world() y Aircraft
// (que la multiplica por la actitud del cuerpo). Misma base, un solo lugar.
glm::mat4 to_world_basis();

// Dirección del morro (eje +X del cuerpo) en el mundo, para el estado dado.
// El cuerpo "mira" hacia donde vuela: con rumbo 0 (norte) apunta a −Z.
glm::vec3 forward_world(const dlfdm::AircraftState& ned);

}  // namespace fp