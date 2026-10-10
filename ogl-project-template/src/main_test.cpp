// ============================================================================
// main_test.cpp — Tests de ResourceManager, primitives, CameraSystem e
//                 InputHandler (C++ puro, sin OpenGL).
//
// Compilación (desde la raíz del proyecto):
//   g++ -std=c++17 -Wall -Wextra src/core/Primitives.cpp
//       src/core/ResourceManager.cpp src/core/CameraSystem.cpp
//       src/core/InputHandler.cpp src/main_test.cpp -o test_rm
//
// Ejecución (desde la raíz del proyecto):
//   ./test_rm
//
// Práctico 03: los tests de primitives cubren la tupla nueva (sin color, con
// normal y tex_coords), los conteos de vértices/índices de cilindro, cono y
// esfera, las costuras y anillos degenerados, y la VERIFICACIÓN DEL ORDEN DE
// LOS TRIÁNGULOS con el producto vectorial (B-A)x(C-A): el resultado debe
// apuntar en la misma dirección que la normal del primer vértice del triángulo
// (CCW visto desde afuera; el culling está deshabilitado y no lo detecta).
//
// Práctico 05: CameraSystem no toca OpenGL ni el mouse (recibe CameraCommand),
// así que la proyección (perspectiva con división por w, aspect del viewport),
// la vista orbital (objetivo centrado, clamps de pitch/distancia) y
// InputHandler (primer cuadro sin salto, signos y ganancias px->rad) se
// testean sin GPU ni ventana.
// ============================================================================

#include <iostream>
#include <cassert>
#include <cmath>

#include <glm/glm.hpp>                     // glm::cross, dot, normalize, length
#include <glm/gtc/matrix_transform.hpp>    // glm::radians (tests de cámara)

#include "core/ResourceManager.h"
#include "core/Primitives.h"
#include "core/MeshData.h"
#include "core/CameraCommand.h"
#include "core/CameraSystem.h"
#include "core/InputHandler.h"

namespace {

// True si el triángulo (a,b,c) está en CCW visto desde su normal saliente:
// (B-A)x(C-A) debe quedar a < 90° de la normal del vértice A. Los triángulos
// degenerados (ápice del cono, polos de la esfera) se dan por buenos.
bool check_winding(const MeshData& m, unsigned a, unsigned b, unsigned c)
{
	const glm::vec3 A = m.vertices[a].position;
	const glm::vec3 B = m.vertices[b].position;
	const glm::vec3 C = m.vertices[c].position;
	const glm::vec3 g = glm::cross(B - A, C - A);
	if (glm::length(g) < 1e-6f) return true;
	return glm::dot(glm::normalize(g),
	                glm::normalize(m.vertices[a].normal)) > 0.5f;
}

bool all_winding_ok(const MeshData& m)
{
	for (std::size_t i = 0; i + 2 < m.indices.size(); i += 3) {
		if (!check_winding(m, m.indices[i], m.indices[i+1], m.indices[i+2]))
			return false;
	}
	return true;
}

// Conteo de vértices de posición (casi) idéntica => cicatriz/costura.
int count_near(const MeshData& m, const glm::vec3& p, float eps)
{
	int n = 0;
	for (const auto& v : m.vertices) {
		if (glm::length(v.position - p) < eps) ++n;
	}
	return n;
}

}  // namespace

int main()
{
	std::cout << "=== Test ResourceManager ===" << std::endl;
	std::cout << std::endl;

	// ---------------------------------------------------------------
	// Test 1: Cargar un shader existente
	// ---------------------------------------------------------------
	std::cout << "[Test 1] Cargar shader 'triangle'..." << std::endl;
	ResourceManager rm(".");
	const ShaderSource& tri = rm.load_shader_source(
	    "triangle",
	    "assets/shaders/triangle.vs",
	    "assets/shaders/triangle.fs"
	);

	assert(!tri.vs.empty() && "Vertex shader no debería estar vacío");
	assert(!tri.fs.empty() && "Fragment shader no debería estar vacío");
	assert(tri.gs.empty() && "Geometry shader debería estar vacío");
	std::cout << "  OK: shaders cargados correctamente." << std::endl;
	std::cout << "  VS (primeros 50 chars): " << tri.vs.substr(0, 50) << "..." << std::endl;
	std::cout << "  FS (primeros 50 chars): " << tri.fs.substr(0, 50) << "..." << std::endl;
	std::cout << std::endl;

	// ---------------------------------------------------------------
	// Test 2: Cacheo — pedir la misma clave dos veces
	// ---------------------------------------------------------------
	std::cout << "[Test 2] Pedir la misma clave dos veces (cacheo)..." << std::endl;
	const ShaderSource& tri2 = rm.get_shader_source("triangle");
	assert(&tri2 == &tri && "Debería retornar la misma referencia (cache)");
	std::cout << "  OK: la segunda petición retornó la misma referencia." << std::endl;
	std::cout << std::endl;

	// ---------------------------------------------------------------
	// Test 3: Clave duplicada con archivos distintos (debería ignorar)
	// ---------------------------------------------------------------
	std::cout << "[Test 3] Cargar misma clave con archivos distintos (debería ignorar)..." << std::endl;
	const ShaderSource& tri3 = rm.load_shader_source(
	    "triangle",
	    "assets/shaders/triangle.fs",  // Intercambiado: FS en vez de VS
	    "assets/shaders/triangle.vs"   // Intercambiado: VS en vez de FS
	);
	// Debería mantener los originales
	assert(&tri3 == &tri && "Debería retornar la misma referencia (ignora nueva carga)");
	std::cout << "  OK: se ignoró la segunda carga." << std::endl;
	std::cout << std::endl;

	// ---------------------------------------------------------------
	// Test 4: Clave inexistente
	// ---------------------------------------------------------------
	std::cout << "[Test 4] Buscar clave inexistente..." << std::endl;
	const ShaderSource& empty = rm.get_shader_source("nonexistent");
	assert(empty.vs.empty() && "Debería retornar ShaderSource vacío");
	assert(empty.fs.empty() && "Debería retornar ShaderSource vacío");
	std::cout << "  OK: se retornó ShaderSource vacío." << std::endl;
	std::cout << std::endl;

	// ---------------------------------------------------------------
	// Test 5: Archivo inexistente
	// ---------------------------------------------------------------
	std::cout << "[Test 5] Cargar archivo inexistente..." << std::endl;
	const ShaderSource& missing = rm.load_shader_source(
	    "missing",
	    "no_existe.vs",
	    "no_existe.fs"
	);
	assert(missing.vs.empty() && "VS debería estar vacío si el archivo no existe");
	assert(missing.fs.empty() && "FS debería estar vacío si el archivo no existe");
	std::cout << "  OK: se manejó el error sin crash." << std::endl;
	std::cout << std::endl;

	// ---------------------------------------------------------------
	// Test 6: clear()
	// ---------------------------------------------------------------
	std::cout << "[Test 6] clear()..." << std::endl;
	rm.clear();
	const ShaderSource& after_clear = rm.get_shader_source("triangle");
	assert(after_clear.vs.empty() && "Debería estar vacío tras clear()");
	std::cout << "  OK: el cache se limpió correctamente." << std::endl;
	std::cout << std::endl;

	// ---------------------------------------------------------------
	// Test 7: Recargar después de clear()
	// ---------------------------------------------------------------
	std::cout << "[Test 7] Recargar después de clear()..." << std::endl;
	const ShaderSource& reloaded = rm.load_shader_source(
	    "triangle",
	    "assets/shaders/triangle.vs",
	    "assets/shaders/triangle.fs"
	);
	assert(!reloaded.vs.empty() && "Debería poder recargar después de clear()");
	std::cout << "  OK: se recargó correctamente." << std::endl;
	std::cout << std::endl;

	// ---------------------------------------------------------------
	// Test 8: primitives::cube() genera 24 vértices y 36 índices
	// ---------------------------------------------------------------
	std::cout << "[Test 8] primitives::cube()..." << std::endl;
	const MeshData cube = primitives::cube();
	assert(cube.vertices.size() == 24 && "El cubo debe tener 24 vértices");
	assert(cube.indices.size() == 36 && "El cubo debe tener 36 índices");
	std::cout << "  OK: cubo con " << cube.vertices.size()
	          << " vertices y " << cube.indices.size()
	          << " indices." << std::endl;

	// El cubo por defecto tiene lado unitario (de -0.5 a 0.5), normales de
	// módulo 1 y tex_coords dentro de [0,1].
	for (const auto& v : cube.vertices) {
		assert(v.position.x >= -0.501f && v.position.x <= 0.501f);
		assert(v.position.y >= -0.501f && v.position.y <= 0.501f);
		assert(v.position.z >= -0.501f && v.position.z <= 0.501f);
		assert(std::abs(glm::length(v.normal) - 1.0f) < 1e-4f);
		assert(v.tex_coords.x >= -1e-4f && v.tex_coords.x <= 1.0f + 1e-4f);
		assert(v.tex_coords.y >= -1e-4f && v.tex_coords.y <= 1.0f + 1e-4f);
	}
	std::cout << "  OK: cubo de lado unitario (extremos -0.5..0.5), normales y UV." << std::endl;

	// Las normales de cada cara son vectores de eje puro (solo una componente
	// no nula): una cara no "mezcla" normales.
	for (const auto& v : cube.vertices) {
		const float abs_sum = std::abs(v.normal.x) + std::abs(v.normal.y)
		                    + std::abs(v.normal.z);
		assert(std::abs(abs_sum - 1.0f) < 1e-4f && "La normal del cubo es de eje puro");
	}
	std::cout << "  OK: normales por cara (vectores de eje puro)." << std::endl;

	// La escala escalable produce una "placa" (p. ej. escala Z > 1).
	const auto plate = primitives::cube(1.0f, 1.0f, 2.0f);
	assert(plate.indices.size() == 36 && "La placa mantiene 36 índices");
	std::cout << "  OK: escala z=2 produce una placa (misma topología)." << std::endl;
	std::cout << std::endl;

	// ---------------------------------------------------------------
	// Test 9: layout de la tupla Vertex (sin color, con normal y tex_coords)
	//   El módulo Mesh calcula los offsets con offsetof; acá se verifica que
	//   la tupla quede como se documenta: 32 bytes, offsets 0/12/24.
	// ---------------------------------------------------------------
	std::cout << "[Test 9] Layout de Vertex (Práctica 3)..." << std::endl;
	assert(sizeof(Vertex) == 32 && "sizeof(Vertex) debe ser 32 bytes");
	assert(offsetof(Vertex, position)   == 0  && "position debe arrancar en el byte 0");
	assert(offsetof(Vertex, normal)     == 12 && "normal debe arrancar en el byte 12");
	assert(offsetof(Vertex, tex_coords) == 24 && "tex_coords debe arrancar en el byte 24");
	std::cout << "  OK: sizeof=" << sizeof(Vertex)
	          << ", offsets position/normal/tex_coords = "
	          << offsetof(Vertex, position) << "/" << offsetof(Vertex, normal)
	          << "/" << offsetof(Vertex, tex_coords) << "." << std::endl;
	std::cout << std::endl;

	// ---------------------------------------------------------------
	// Test 10: cilindro (radio, largo, 12 gajos, 1 anillo) + winding
	// ---------------------------------------------------------------
	std::cout << "[Test 10] primitives::cylinder()..." << std::endl;
	const float cyl_radio = 0.5f, cyl_largo = 1.0f;
	const MeshData cyl = primitives::cylinder(cyl_radio, cyl_largo, 12U);
	// Lateral: 2*(12+1) = 26; 2 tapas de 13 => 52 vértices.
	assert(cyl.vertices.size() == 52 && "El cilindro debe tener 52 vértices");
	assert(cyl.indices.size() == 144 && "El cilindro debe tener 144 índices");
	std::cout << "  OK: " << cyl.vertices.size() << " vértices y "
	          << cyl.indices.size() << " índices." << std::endl;

	// Costura del lateral: por cada anillo hay dos vértices LATERALES en theta=0
	// (u = 0 y u = 1) con la misma posición (radio, y, 0). Nota: las tapas
	// aportan APARTE su propio rim en ese mismo punto (no comparten vértice),
	// por eso clasificamos por la normal y no por la posición.
	auto count_lateral_at = [&](const glm::vec3& probe) {
		int n = 0;
		for (const auto& v : cyl.vertices)
			if (glm::length(v.position - probe) < 1e-4f
			    && std::abs(v.normal.y) < 1e-5f)  // normal radial => lateral
				++n;
		return n;
	};
	assert(count_lateral_at(glm::vec3(cyl_radio, 0.5f * cyl_largo, 0.0f)) == 2
	       && "La costura superior duplica el vértice lateral (u=0 y u=1)");
	assert(count_lateral_at(glm::vec3(cyl_radio, -0.5f * cyl_largo, 0.0f)) == 2
	       && "La costura inferior duplica el vértice lateral (u=0 y u=1)");
	std::cout << "  OK: costura del lateral con vértice duplicado (u=0 y u=1)." << std::endl;

	// Tapas: normales axiales, nunca comparten vértice con el lateral. Las
	// clasificamos POR LA NORMAL (el y de los anillos superior/inferior del
	// lateral coincide con el de las tapas).
	int top_cap = 0, bot_cap = 0;
	for (const auto& v : cyl.vertices) {
		if (v.normal == glm::vec3(0, 1, 0)) {
			++top_cap;
			assert(std::abs(v.position.y - 0.5f * cyl_largo) < 1e-5f
			       && "Tapa superior en y = +largo/2");
		}
		if (v.normal == glm::vec3(0, -1, 0)) {
			++bot_cap;
			assert(std::abs(v.position.y + 0.5f * cyl_largo) < 1e-5f
			       && "Tapa inferior en y = -largo/2");
		}
	}
	assert(top_cap == 13 && "Tapa superior: centro + 12 rim");
	assert(bot_cap == 13 && "Tapa inferior: centro + 12 rim");
	std::cout << "  OK: tapas con normal axial (13 vértices c/u, separadas del lateral)." << std::endl;

	// Los vértices LATERALES (normal radial: y == 0 en la normal) quedan sobre
	// el manto de radio cyl_radio en TODOS los anillos (inclusive los de las
	// tapas), y cada normal es unitaria y radial.
	for (const auto& v : cyl.vertices) {
		assert(std::abs(glm::length(v.normal) - 1.0f) < 1e-4f);
		if (std::abs(v.normal.y) < 1e-5f) {  // lateral
			const float r2 = v.position.x * v.position.x + v.position.z * v.position.z;
			assert(std::abs(std::sqrt(r2) - cyl_radio) < 1e-4f
			       && "Lateral sobre el radio teórico (todos los anillos)");
		} else {  // tapa
			assert(v.normal == glm::vec3(0, 1, 0) || v.normal == glm::vec3(0, -1, 0));
		}
	}
	std::cout << "  OK: lateral sobre el radio teórico con normal radial." << std::endl;

	assert(all_winding_ok(cyl) && "Todos los triángulos del cilindro en CCW");
	std::cout << "  OK: winding CCW verificado por producto vectorial." << std::endl;
	std::cout << std::endl;

	// ---------------------------------------------------------------
	// Test 11: cono (radio, conicidad 60°, 12 gajos, 1 anillo) + winding
	// ---------------------------------------------------------------
	std::cout << "[Test 11] primitives::cone()..." << std::endl;
	const float cone_radio = 0.6f, cone_conicidad = 60.0f;
	const float semi  = 0.5f * cone_conicidad * 3.14159265f / 180.0f;
	const float h_cone = cone_radio / std::tan(semi);   // radio / tan(semiángulo)
	const MeshData cone = primitives::cone(cone_radio, cone_conicidad, 12U);
	// Lateral 2*(12+1) = 26 + base (12+1) = 13 => 39 vértices.
	assert(cone.vertices.size() == 39 && "El cono debe tener 39 vértices");
	assert(cone.indices.size() == 108 && "El cono debe tener 108 índices");
	std::cout << "  OK: " << cone.vertices.size() << " vértices y "
	          << cone.indices.size() << " índices." << std::endl;

	// Altura derivada de la conicidad: y_min = -h/2 (base), y_max = +h/2.
	float y_min = 1e30f, y_max = -1e30f;
	for (const auto& v : cone.vertices) {
		y_min = std::min(y_min, v.position.y);
		y_max = std::max(y_max, v.position.y);
	}
	assert(std::abs(y_min + 0.5f * h_cone) < 1e-4f
	       && "La base del cono cae en y = -h/2 (h = radio/tan(semi))");
	assert(std::abs(y_max - 0.5f * h_cone) < 1e-4f
	       && "El ápice del cono cae en y = +h/2 (h = radio/tan(semi))");
	std::cout << "  OK: altura h = " << h_cone
	          << " deducida de la conicidad (grados -> semiángulo)." << std::endl;

	// Ápice DEGENERADO: los últimos 13 vértices (anillo superior, radio 0)
	// comparten el punto (0, +h/2, 0), con 13 normales distintas (por gajo).
	assert(count_near(cone, glm::vec3(0, 0.5f * h_cone, 0), 1e-4f) == 13
	       && "El ápice degenerado debe tener 13 vértices en el mismo punto");
	std::cout << "  OK: ápice como anillo degenerado (13 vértices, uno por gajo)." << std::endl;

	// Base: tapa separada del lateral con normal -Y.
	//   (a) el centro del fan está en (0, -h/2, 0).
	assert(count_near(cone, glm::vec3(0.0f, -0.5f * h_cone, 0.0f), 1e-4f) == 1
	       && "El centro de la tapa de la base es el único vértice en (0,-h/2,0)");
	//   (b) vértices de la base por NORMAL: centro + 12 rim = 13, todos -Y.
	int base_verts = 0;
	for (const auto& v : cone.vertices) {
		if (v.normal == glm::vec3(0, -1, 0)) {
			++base_verts;
			assert(std::abs(v.position.y + 0.5f * h_cone) < 1e-5f
			       && "Vértices de la base en y = -h/2");
		}
	}
	assert(base_verts == 13 && "Fan de la base: 1 centro + 12 rim (todos -Y)");
	//   (c) el anillo lateral en y=-h/2 usa 13 vértices PROPIOS (12 + costura)
	//       con normal oblicua hacia arriba; el fan de -Y es aparte.
	int lateral_ring = 0;
	for (const auto& v : cone.vertices) {
		if (std::abs(v.position.y + 0.5f * h_cone) < 1e-5f && v.normal.y > 0.0f) {
			++lateral_ring;
		}
	}
	assert(lateral_ring == 13 && "Anillo lateral de la base: 12 + costura, separado del fan");
	std::cout << "  OK: base en y = -h/2 con fan (13) separado del anillo lateral (13)." << std::endl;

	assert(all_winding_ok(cone) && "Todos los triángulos del cono en CCW");
	std::cout << "  OK: winding CCW verificado por producto vectorial." << std::endl;
	std::cout << std::endl;

	// ---------------------------------------------------------------
	// Test 12: esfera (radio, 30 gajos, 18 anillos) + winding
	// ---------------------------------------------------------------
	std::cout << "[Test 12] primitives::sphere()..." << std::endl;
	const float sph_radio = 0.55f;
	const MeshData sph = primitives::sphere(sph_radio, 30U, 18U);
	assert(sph.vertices.size() == (18U + 1U) * (30U + 1U)
	       && "La esfera debe tener (M+1)*(N+1) vértices");
	assert(sph.indices.size() == 6U * 30U * 18U
	       && "La esfera debe tener 6*M*N índices");
	std::cout << "  OK: " << sph.vertices.size() << " vértices y "
	          << sph.indices.size() << " índices." << std::endl;

	// Todos los vértices sobre la esfera de radio sph_radio (excepto degenerados
	// que igual están sobre la misma).
	for (const auto& v : sph.vertices) {
		assert(std::abs(glm::length(v.position) - sph_radio) < 1e-4f
		       && "Todos los vértices sobre el radio teórico");
		assert(std::abs(glm::length(v.normal) - 1.0f) < 1e-4f
		       && "Normales unitarias (normal = posición normalizada)");
	}
	std::cout << "  OK: vértices y normales sobre el radio teórico." << std::endl;

	// Polos degenerados: primer anillo en (0,-r,0) y último en (0,+r,0), con
	// 31 vértices cada uno.
	assert(count_near(sph, glm::vec3(0.0f, -sph_radio, 0.0f), 1e-4f) == 31
	       && "Polo sur degenerado con N+1 vértices");
	assert(count_near(sph, glm::vec3(0.0f,  sph_radio, 0.0f), 1e-4f) == 31
	       && "Polo norte degenerado con N+1 vértices");
	std::cout << "  OK: polos como anillos degenerados (31 vértices c/u)." << std::endl;

	assert(all_winding_ok(sph) && "Todos los triángulos de la esfera en CCW");
	std::cout << "  OK: winding CCW verificado por producto vectorial." << std::endl;
	std::cout << std::endl;

	// ---------------------------------------------------------------
	// Test 13: CameraSystem::set_viewport() arma la proyección de
	//          PERSPECTIVA (división por w) con el aspect correcto.
	// ---------------------------------------------------------------
	std::cout << "[Test 13] CameraSystem::set_viewport() (perspectiva)..." << std::endl;
	CameraSystem cam(800, 600);  // aspect = 4/3
	const float inv_tan = 1.0f / std::tan(glm::radians(22.5f));
	{
		const glm::mat4& proj = cam.data().projection;

		// glm::perspective(45°): [1][1] = 1/tan(θv/2), [0][0] = [1][1]/aspect.
		assert(std::abs(proj[1][1] - inv_tan) < 1e-4f
		       && "proj[1][1] = 1/tan(fovy/2)");
		assert(std::abs(proj[0][0] - inv_tan / (800.0f / 600.0f)) < 1e-4f
		       && "proj[0][0] = proj[1][1] / aspect");
		// La "caja negra" tenía cuarta fila (0,0,0,1): la perspectiva NO.
		assert(std::abs(proj[2][3] + 1.0f) < 1e-6f
		       && "proj[2][3] = -1: hay división por w");
		assert(std::abs(proj[3][3]) < 1e-6f
		       && "proj[3][3] = 0: la cuarta fila ya no es (0,0,0,1)");
		std::cout << "  OK: perspectiva con aspect 4/3 y división por w." << std::endl;
	}
	// Al minimizar la ventana el alto llega a 0: set_viewport() lo corrige y
	// la proyección nunca queda con aspect indefinido (ni NaN ni inf).
	cam.set_viewport(800, 0);
	{
		const glm::mat4& proj = cam.data().projection;
		bool finito = true;
		for (int c = 0; c < 4 && finito; ++c)
			for (int f = 0; f < 4; ++f)
				finito = finito && !std::isnan(proj[c][f])
				              && !std::isinf(proj[c][f]);
		assert(finito && "proyección finita con la ventana minimizada (h=0)");
		std::cout << "  OK: proyección finita con alto 0 (minimizada)." << std::endl;
	}
	cam.set_viewport(800, 600);
	assert(std::abs(cam.data().projection[0][0]
	                - inv_tan / (800.0f / 600.0f)) < 1e-4f
	       && "Al restaurar el tamaño se restaura el aspect");
	std::cout << "  OK: se restaura el aspect al volver a 800x600." << std::endl;
	std::cout << std::endl;

	// ---------------------------------------------------------------
	// Test 14: CameraSystem::update() -> la vista SIEMPRE apunta al
	//          objetivo (el objetivo queda en (0,0,-d) de view space).
	// ---------------------------------------------------------------
	std::cout << "[Test 14] CameraSystem::update() (vista orbital)..." << std::endl;
	{
		const glm::vec3 objetivo(0.0f);
		CameraCommand cmd0;                        // deltas cero
		cam.update(objetivo, glm::vec3(0.0f), cmd0);
		glm::vec4 centered = cam.data().view * glm::vec4(objetivo, 1.0f);
		assert(std::abs(centered.x) < 1e-4f && std::abs(centered.y) < 1e-4f
		       && "El objetivo queda centrado en el view space");
		assert(std::abs(centered.z - (-10.0f)) < 1e-3f
		       && "El objetivo está a distancia_ = 10 delante de la cámara");

		// Comando extremo: la vista no se indefine (clamps de pitch y distancia).
		CameraCommand big;
		big.yaw_delta   = 5.0f;
		big.pitch_delta = 50.0f;   // muy por encima del límite ±0.9·π/2
		big.dist_delta  = -100.0f; // muy por debajo del mínimo
		cam.update(objetivo, glm::vec3(0.0f), big);
		const glm::mat4& view = cam.data().view;
		bool finito = true;
		for (int c = 0; c < 4 && finito; ++c)
			for (int f = 0; f < 4; ++f)
				finito = finito && !std::isnan(view[c][f])
				              && !std::isinf(view[c][f]);
		assert(finito && "El clamp de pitch (±0.9·π/2) evita el lookAt singular");

		centered = cam.data().view * glm::vec4(objetivo, 1.0f);
		assert(std::abs(centered.x) < 1e-4f && std::abs(centered.y) < 1e-4f
		       && "Con el comando extremo el objetivo sigue centrado");
		assert(std::abs(centered.z + 2.0f) < 1e-3f
		       && "La distancia se encoge al mínimo (2.0), no atraviesa el objetivo");
		std::cout << "  OK: vista apuntando al objetivo con clamps (pitch y dist)."
		          << std::endl;
	}
	std::cout << std::endl;

	// ---------------------------------------------------------------
	// Test 15: InputHandler: primer cuadro sin salto, signos y ganancias.
	// ---------------------------------------------------------------
	std::cout << "[Test 15] InputHandler::poll() (mouse -> CameraCommand)..." << std::endl;
	{
		InputHandler input;
		CameraCommand cmd;

		// Primer cuadro: no hay posición anterior -> comando vacío (sin salto).
		input.poll(100.0, 200.0, true, false, cmd);
		assert(cmd.yaw_delta == 0.0f && cmd.pitch_delta == 0.0f
		       && cmd.dist_delta == 0.0f
		       && "Primer cuadro: delta = 0");

		// Segundo cuadro, botón izquierdo: +100 px en X y -40 px en Y.
		input.poll(200.0, 160.0, true, false, cmd);
		assert(std::abs(cmd.yaw_delta - (-100.0f * InputHandler::kOrbitGain)) < 1e-6f
		       && "dx = +100 px -> yaw_delta = -dx * kOrbitGain");
		assert(std::abs(cmd.pitch_delta - (-(-40.0f) * InputHandler::kOrbitGain)) < 1e-6f
		       && "dy = -40 px -> pitch_delta = -dy * kOrbitGain (elevar)");
		assert(cmd.dist_delta == 0.0f && "Sin botón derecho no hay zoom");

		// Botón derecho: +20 px en Y -> se acerca (dist_delta negativo).
		input.poll(200.0, 180.0, false, true, cmd);
		assert(std::abs(cmd.dist_delta - (-20.0f * InputHandler::kZoomGain)) < 1e-6f
		       && "dy = +20 px -> dist_delta = -dy * kZoomGain (acerca)");
		assert(cmd.yaw_delta == 0.0f && cmd.pitch_delta == 0.0f
		       && "Sin botón izquierdo no hay órbita");

		// Sin botones: comando vacío (pasar el mouse no mueve la cámara).
		input.poll(300.0, 180.0, false, false, cmd);
		assert(cmd.yaw_delta == 0.0f && cmd.pitch_delta == 0.0f
		       && cmd.dist_delta == 0.0f && "Sin botones: nada se mueve");
		std::cout << "  OK: primer cuadro, signos, ganancias y botones." << std::endl;
	}
	std::cout << std::endl;

	std::cout << "=== Todos los tests pasaron ===" << std::endl;
	return 0;
}