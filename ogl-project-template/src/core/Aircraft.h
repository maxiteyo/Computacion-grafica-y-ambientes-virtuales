// ============================================================================
// Aircraft.h — Módulo "aeronave" del Práctico 04: un S-211 estilizado compuesto
// por primitivas del Práctico 03, con su propia jerarquía de transformaciones.
//
// Idea de la consigna: el main NO debe saber cómo está armado el avión por
// dentro. Solo le pide a la aeronave que actualice su pose por cuadro y le
// "suelte" la lista de ítems listos para dibujar (RenderItem: malla +
// modelo->mundo + color), y se encarga de dibujarlos tal cual.
//
// Jerarquía de DOS niveles:
//   1. locales_: transformaciones FIJAS de cada pieza dentro del sistema de
//      referencia del avión (se cargan una sola vez en init()).
//   2. pose_: transformación de la AERONAVE COMPLETA, variable por cuadro
//      (update() la recalcula cada frame).
//      El modelo->mundo de cada pieza es modelo = pose_ * locales_[i]
//      (GLM columna-mayor: el local se aplica primero al vértice, después la
//      pose). La aeronave es rígida: ninguna pieza tiene animación propia;
//      todas comparten la misma pose.
//
// Sistema de referencia del avión (largo = 1):
//   - Origen = punta de la nariz = punto de referencia del cabeceo (ref).
//   - +X hacia la cola, +Y hacia el ala derecha (desde el piloto), +Z arriba.
//   - angulos de update() = (phi, theta, psi) = actitud en el convenio
//     aeroespacial (roll, pitch, yaw; ver FlightData.h). El módulo la adapta
//     al referencial del modelo con una base CONSTANTE (ver Aircraft.cpp):
//     la rotación final es  base_NED_gl · Rz(psi)·Ry(theta)·Rx(phi) ·
//     base_modelo, así el avión vuela "de morro" y con las convenciones
//     de signo físicas (rumbo 0 = −Z del mundo, pitch + = nariz arriba,
//     roll + = ala derecha abajo). El cabeceo es la rotación sobre el eje Y
//     (paralelo al ala) que pasa por el punto de referencia en el origen.
//   - pose_ = T(pos) · R_avion · S(escala) · T(−ref), con ref = (0,0,0) la
//     punta: la rotación deja clavado el morro y balancea la cola; la escala
//     (por defecto 1, ver set_scale) amplía el modelo alrededor del ref.
//   - Práctico 05: ya NO hay base de puesta en escena. La antigua base giraba el
//     modelo para compensar la proyección 2D ("caja negra" uAjuste) y dejarlo en
//     planta. Ahora la escena tiene una cámara orbital con perspectiva real
//     (CameraSystem): el avión se dibuja en su referencial NATURAL (morro hacia
//     −X, +Y ala derecha, +Z arriba) y el ángulo de vista lo elige la cámara al
//     orbitar alrededor de su posición.
//
// Piezas y mallas:
//   - 7 piezas (nariz, fuselaje, 2 alas, 2 estabilizadores horizontales,
//     deriva) pero 5 mallas ÚNICAS: las piezas simétricas (alas y
//     estabilizadores) COMPARTEN la misma malla de cubo y solo cambia su local
//     (traslación). Así no se duplica GPU innecesariamente.
//   - Las escalas NO uniformes van DENTRO del generador cube(sx, sy, sz):
//     nunca se usa escala negativa (regla del enunciado) para "espejar" piezas.
//
// Ciclo de vida / RAII: la clase posee las mallas de GPU (Mesh) con el mismo
// esquema que el resto del proyecto (no copiable, movible). El destructor
// libera dentro del bloque del main que mantiene el contexto OpenGL vivo.
// ============================================================================
#pragma once

#include <cstddef>
#include <vector>

#include <glm/glm.hpp>

#include "Mesh.h"
#include "RenderItem.h"

class Aircraft {
public:
    Aircraft() = default;
    ~Aircraft() = default;
    Aircraft(const Aircraft&) = delete;        // mallas de GPU: dueño único
    Aircraft& operator=(const Aircraft&) = delete;

    // Genera las 5 mallas y las 7 piezas (locales, mapeo malla y colores).
    // UNA sola vez, con el contexto OpenGL activo (Mesh::load sube a la GPU).
    void init(void);

    // Recalcula la pose de la aeronave completa para este cuadro.
    //   pos    : posición del punto de referencia (la punta) en el mundo [m].
    //   angulos: actitud (phi, theta, psi) en RADIANES, convenio aeroespacial
    //            (ver FlightData.h); la base del referencial la aplica el
    //            módulo internamente.
    void update(const glm::vec3& pos, const glm::vec3& angulos);

    // Escala visual uniforme del modelo alrededor del punto de referencia.
    // El modelo mide 1; en el mundo en metros (Práctico 06) se usa ~9.6 para
    // que el avión sea legible a la distancia de cámara de seguimiento.
    void set_scale(float scale);

    // Escribe en "items" (borra el contenido previo) los ítems listos para
    // dibujar: cada uno referencia la malla de la pieza, su modelo->mundo YA
    // compuesto (pose * local) y su color plano. El main no sabe cuántas
    // piezas son ni cómo se arma el avión.
    void collect(std::vector<RenderItem>& items) const;

private:
    std::vector<Mesh>           piezas_;        // 5 mallas únicas (una por forma)
    std::vector<unsigned int>   pieza_a_malla_; // por pieza: índice de malla
    std::vector<glm::mat4>      locales_;       // transformación fija de cada pieza
    std::vector<glm::vec4>      colores_;       // color plano de cada pieza
    glm::mat4                   pose_{1.0f};    // pose de la aeronave (por cuadro)
    float                       scale_ = 1.0f;  // escala visual (ver set_scale)
};