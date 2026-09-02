// ============================================================================
// main_test.cpp — Test del módulo ResourceManager (C++ puro, sin OpenGL).
//
// Compilación:
//   g++ -std=c++17 -Wall -Wextra ResourceManager.cpp main_test.cpp -o test_rm
//
// Ejecución (desde la raíz del proyecto):
//   ./test_rm
// ============================================================================

#include <iostream>
#include <cassert>

#include "core/ResourceManager.h"
#include "core/Primitives.h"
#include "core/MeshData.h"

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

	// El cubo por defecto tiene lado unitario (de -0.5 a 0.5).
	for (const auto& v : cube.vertices) {
		assert(v.px >= -0.501f && v.px <= 0.501f);
		assert(v.py >= -0.501f && v.py <= 0.501f);
		assert(v.pz >= -0.501f && v.pz <= 0.501f);
	}
	std::cout << "  OK: cubo de lado unitario (extremos -0.5..0.5)." << std::endl;

	// La escala escalable produce una "placa" (p. ej. escala Z > 1).
	const auto plate = primitives::cube(1.0f, 1.0f, 2.0f);
	assert(plate.indices.size() == 36 && "La placa mantiene 36 índices");
	std::cout << "  OK: escala z=2 produce una placa (misma topología)." << std::endl;
	std::cout << std::endl;

	// ---------------------------------------------------------------
	// Test 9: el offset del atributo color está en la posición esperada (12)
	//   (el módulo Mesh lo calcula con offsetof; acá se verifica la tupla).
	// ---------------------------------------------------------------
	std::cout << "[Test 9] Layout de Vertex (offset de color)..." << std::endl;
	Vertex v;
	const auto off_color = reinterpret_cast<const char*>(&v.r) -
	                       reinterpret_cast<const char*>(&v);
	assert(off_color == 12 && "Color debe empezar en el byte 12");
	std::cout << "  OK: offset de color = " << off_color << " bytes." << std::endl;
	std::cout << std::endl;

	std::cout << "=== Todos los tests pasaron ===" << std::endl;
	return 0;
}
