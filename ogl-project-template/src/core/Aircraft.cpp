// ============================================================================
// Aircraft.cpp — Implementación del módulo Aircraft (Práctico 04).
//
// Valores del despiece (largo total L = 1, origen en la punta, +X a la cola):
//   - Nariz  : cono de radio 1/6 y apertura total 90° -> h = r/tan(45°) = r,
//              cono local en el eje Y, girado Rz(+90°) para alinear su eje con
//              -X y trasladado para dejar el ápice en la punta (origen) y la
//              base en x = 1/6: local = T(1/12, 0, 0) · Rz(+90°).
//   - Fuselaje: cilindro de radio 1/6 (el MISMO que la base del cono) y largo
//              5/6; es la CONTINUACIÓN de la nariz: arranca en x = 1/6 (junta
//              con perfil continuo, mismo radio) y llega a x = 1. Con el cono
//              suma el largo total L = 1: local = T(7/12, 0, 0) · Rz(-90°).
//   - Alas (izq/derecha): placa cube(0.15, 7/15, 0.02), semienvergadura 7/15
//              centrada en su origen: local = T(0.45, ±7/30, 0) con y ∈ [0, ±7/15].
//   - Estabilizadores horizontales: cube(0.08, 0.25, 0.01):
//              local = T(0.94, ±1/8, 0.06).
//   - Deriva (timón vertical): cube(0.15, 0.018, 13/60):
//              local = T(0.95, 0, 0.275) (z arriba hasta ≈0.38).
// ============================================================================

#include "Aircraft.h"

#include <cstddef>

#include <glm/gtc/matrix_transform.hpp>  // translate, rotate, radians

#include "Primitives.h"

namespace {

// Punto de referencia de la aeronave = punta de la nariz (origen local).
constexpr glm::vec3 kRef{0.0f, 0.0f, 0.0f};

}  // namespace

void Aircraft::init(void)
{
    // -----------------------------------------------------------------------
    // 1) MALLAS ÚNICAS (5). Se generan en CPU y se suben a GPU en este orden:
    //    0 cono (nariz) | 1 cilindro (fuselaje) | 2 cubo ala | 3 cubo estab.
    //    horizontal | 4 cubo deriva.
    // -----------------------------------------------------------------------
    piezas_.resize(5U);

    // Nariz: radio 1/6, apertura TOTAL 90° (interfaz del cono en grados).
    piezas_[0].load(primitives::cone(1.0f / 6.0f, 90.0f, 24U));

    // Fuselaje: radio 1/6, largo 5/6 a lo largo de su eje local Y. Es la
    // CONTINUACIÓN del cono: arranca en x = 1/6 (base de la nariz, mismo radio
    // 1/6 -> perfil continuo) y llega hasta x = 1. Con el cono (1/6) suma L=1.
    piezas_[1].load(primitives::cylinder(1.0f / 6.0f, 5.0f / 6.0f, 24U));

    // Superficies: placas con escala no uniforme DENTRO del generador (regla
    // del enunciado: no se usa escala negativa para "espejar" piezas).
    piezas_[2].load(primitives::cube(0.15f, 7.0f / 15.0f, 0.02f));   // ala
    piezas_[3].load(primitives::cube(0.08f, 0.25f, 0.01f));          // estab.
    piezas_[4].load(primitives::cube(0.15f, 0.018f, 13.0f / 60.0f)); // deriva

    // -----------------------------------------------------------------------
    // 2) PIEZAS (7): local fijo de cada una (dentro del referencial del avión)
    //    + qué malla usa + color (uno distinto por pieza).
    // -----------------------------------------------------------------------
    const glm::mat4 I = glm::mat4(1.0f);
    const auto tr = [I](float x, float y, float z) {
        return glm::translate(I, glm::vec3(x, y, z));
    };
    const auto rot_z = [I](float grados) {
        return glm::rotate(I, glm::radians(grados), glm::vec3(0.0f, 0.0f, 1.0f));
    };

    locales_ = {
        // 0 nariz: Rz(+90°) alinea el eje local Y del cono con -X del modelo;
        //   T(1/12) deja el ápice en el origen y la base en x = 1/6 (donde
        //   arranca el fuselaje).
        tr(1.0f / 12.0f, 0.0f, 0.0f) * rot_z(90.0f),
        // 1 fuselaje: Rz(-90°) alinea el eje local Y con +X; cilindro de largo
        //   5/6 centrado y corrido a x = 7/12 -> cubre x en [1/6, 1]. El cono
        //   (x en [0, 1/6]) es su continuación: mismo radio en la junta.
        tr(7.0f / 12.0f, 0.0f, 0.0f) * rot_z(-90.0f),
        // 2-3 alas: semienvergadura 7/15 centrada -> y ∈ [0, ±7/15].
        tr(0.45f, 7.0f / 30.0f, 0.0f),
        tr(0.45f, -7.0f / 30.0f, 0.0f),
        // 4-5 estabilizadores horizontales (cola alta, z = 0.06).
        tr(0.94f, 1.0f / 8.0f, 0.06f),   // derecho (+Y)
        tr(0.94f, -1.0f / 8.0f, 0.06f),  // izquierdo (-Y)
        // 6 deriva (timón vertical que sube en +z).
        tr(0.95f, 0.0f, 0.275f),
    };

    pieza_a_malla_ = {0U, 1U, 2U, 2U, 3U, 3U, 4U};

    colores_ = {
        glm::vec4(0.30f, 0.35f, 0.45f, 1.0f),  // nariz (gris pizarra)
        glm::vec4(0.88f, 0.88f, 0.90f, 1.0f),  // fuselaje (plateado claro)
        glm::vec4(0.20f, 0.45f, 0.85f, 1.0f),  // ala derecha (azul; +Y)
        glm::vec4(0.20f, 0.70f, 0.35f, 1.0f),  // ala izquierda (verde; -Y)
        glm::vec4(0.95f, 0.55f, 0.15f, 1.0f),  // estabilizador derecho (naranja; +Y)
        glm::vec4(0.90f, 0.85f, 0.20f, 1.0f),  // estabilizador izquierdo (amarillo; -Y)
        glm::vec4(0.85f, 0.25f, 0.20f, 1.0f),  // deriva (rojo)
    };
}

void Aircraft::update(const glm::vec3& pos, const glm::vec3& angulos)
{
    // 1) Rotaciones de la aeronave en SU propio referencial (radianes).
    //    model = T(pos) · R(roll, pitch, yaw) · T(−ref)
    //    GLM columna-mayor: el último factor se aplica primero al vértice.
    const glm::mat4 R_avion =
        glm::rotate(glm::mat4(1.0f), angulos.x, glm::vec3(1.0f, 0.0f, 0.0f)) *
        glm::rotate(glm::mat4(1.0f), angulos.y, glm::vec3(0.0f, 1.0f, 0.0f)) *
        glm::rotate(glm::mat4(1.0f), angulos.z, glm::vec3(0.0f, 0.0f, 1.0f));

    // 2) SIN base de puesta en escena (Práctico 05): la antigua base existía
    //    para compensar la proyección 2D de la "caja negra" (uAjuste), que
    //    dibujaba solo el plano XY. Ahora la escena tiene una cámara orbital
    //    real con perspectiva (CameraSystem): el avión se muestra en su
    //    referencial NATURAL (morro hacia −X, +Y ala derecha, +Z arriba) y es
    //    la cámara la que elige el ángulo de vista al orbitar. R = R_avion.
    const glm::mat4 R = R_avion;

    pose_ = glm::translate(glm::mat4(1.0f), pos) * R
          * glm::translate(glm::mat4(1.0f), -kRef);
}

void Aircraft::collect(std::vector<RenderItem>& items) const
{
    // Borra y rellena: el llamador no tiene que limpiar items entre cuadros.
    items.clear();
    items.reserve(locales_.size());

    for (std::size_t i = 0; i < locales_.size(); ++i) {
        RenderItem item;
        item.mesh  = &piezas_[pieza_a_malla_[i]];
        item.model = pose_ * locales_[i];  // el local se aplica primero
        item.color = colores_[i];
        items.push_back(item);
    }
}

// ---------------------------------------------------------------------------
// Respuestas a las preguntas de la consigna (documentadas acá):
//
// 1. ¿collect() ANTES de init()?
//    No rompe: locales_ y colores_ están vacíos y la lista queda vacía (no se
//    itera nada, el puntero a malla nunca se desreferencia). El uso correcto
//    sigue siendo init() una vez, y por cuadro update() -> collect() -> dibujar.
//
// 2. ¿Y ANTES de update()?
//    Tampoco rompe: pose_ quedó en identidad (estado inicial), así que se
//    dibujaría la aeronave en el origen sin rotación. Los ítems se generan
//    igual porque collect() solo usa pose_ y los locales. El orden de la
//    consigna es update() ANTES de collect(): collect() arma cada modelo con
//    la pose del último update().
//
// 3. ¿Cuesta regenerar las mallas por frame?
//    En este diseño NO se regeneran: init() genera cada MeshData una vez (lado
//    CPU) y la sube a la GPU (Mesh::load). Regenerar por cuadro implicaría
//    recomputar vértices/índices en CPU Y re-llenar VBO/EBO con glBufferData
//    en cada frame (ancho de banda de CPU->GPU y trabajo inútil, porque la
//    geometría no cambia). Por cuadro solo se recalculan las transformaciones
//    (pose_), que es lo único que cambia (rango visible: iluminación/textura
//    seguirían sin re-subir mallas).
//
// 4. ¿Qué cambia si la aeronave viniera de un .obj en vez de primitivas?
//    - Habría UNA malla por objeto (o un solo mesh combinado) cargada del
//      archivo y subida una sola vez en init(): no se usarían cone/cylinder/
//      cube y el "despiece" en 7 piezas desaparecería del dibujo.
//    - Las posiciones de las piezas ya no saldrían de locales_: están
//      "horneadas" en los vértices del archivo (local = identidad). Los
//      colores vendrían de los materiales/grupos del .obj en vez de colores_.
//    - El despiece solo tendría sentido para animar/colorear partes (alerones,
//      estabilizadores), pero para DIBUJAR la aeronave entera bastaría un solo
//      RenderItem por objeto.
//    - La pose_ (update()/collect()) NO cambiaría: sigue siendo la forma de
//      colocar y rotar la aeronave entera en el mundo.
// ---------------------------------------------------------------------------