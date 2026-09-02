// ============================================================================
// ResourceManager.h
// Módulo de gestión de recursos (C++ puro, sin dependencias de OpenGL).
//
// Funcionalidades:
//   - Lee archivos de shader desde disco y los almacena en memoria.
//   - Cachea recursos: si se pide dos veces la misma clave, no relee de disco.
//   - Advierte en stderr si algo falla (archivo no encontrado, etc.).
//
// Decisiones de diseño:
//   - Se indexa por una clave que elige el usuario (no por nombre de archivo).
//   - Si se pide una clave ya cargada con archivos distintos, se IGNORA la
//     nueva petición y se advierte en stderr.
//   - Los errores se notifican con std::cerr (sin excepciones).
//   - Se devuelve una referencia constante al recurso cacheado.
//   - Se usa un struct (ShaderSource) por tipo de recurso para facilitar
//     la ampliación futura (texturas, modelos, etc.).
// ============================================================================

#pragma once

#include <filesystem>
#include <string>
#include <unordered_map>

// ---------------------------------------------------------------------------
// ShaderSource: código fuente de un programa de shader.
// Contiene las fuentes del vertex, fragment y (opcionalmente) geometry shader.
// ---------------------------------------------------------------------------
struct ShaderSource {
	std::string vs;  // vertex shader
	std::string fs;  // fragment shader
	std::string gs;  // geometry shader (opcional, hoy no se usa)
};

// ---------------------------------------------------------------------------
// ResourceManager: administra la carga y cacheo de recursos del proyecto.
// Actualmente soporta shaders; está preparado para ampliarse a otros tipos.
// ---------------------------------------------------------------------------
class ResourceManager {
public:
	// Crea el manager con una raíz de assets (carpeta base para buscar archivos).
	explicit ResourceManager(const std::filesystem::path& assets_root);

	// Carga un shader desde disco y lo cachea bajo la clave `key`.
	// Si `key` ya existe, imprime aviso en stderr y NO relee de disco.
	// Si algún archivo no existe o no se puede leer, imprime aviso en stderr
	// y deja vacío el campo correspondiente en ShaderSource.
	// Retorna una referencia constante al ShaderSource cacheado.
	const ShaderSource& load_shader_source(const std::string& key,
	                                       const std::string& vs_file,
	                                       const std::string& fs_file,
	                                       const std::string& gs_file = "");

	// Retorna una referencia constante al ShaderSource cacheado bajo `key`.
	// Si `key` no existe, imprime aviso en stderr y retorna una referencia
	// a un ShaderSource vacío estático (no crash).
	const ShaderSource& get_shader_source(const std::string& key) const;

	// Limpia todos los recursos cacheados.
	void clear(void);

private:
	std::filesystem::path assets_root_;
	std::unordered_map<std::string, ShaderSource> shaders_sources_;

	// Lee un archivo de shader completo y retorna su contenido como string.
	// Si hay error, imprime en stderr y retorna string vacío.
	std::string read_shader_file(const std::string& shader_file,
	                             const std::string& shader_type);
};
