// ============================================================================
// ResourceManager.cpp — Implementación del módulo de gestión de recursos.
// ============================================================================

#include "ResourceManager.h"

#include <fstream>   // std::ifstream
#include <sstream>   // std::ostringstream
#include <iostream>  // std::cerr

// ---------------------------------------------------------------------------
// Constructor
// ---------------------------------------------------------------------------
ResourceManager::ResourceManager(const std::filesystem::path& assets_root)
    : assets_root_(assets_root)
{
}

// ---------------------------------------------------------------------------
// read_shader_file: lee un archivo completo y retorna su contenido.
// ---------------------------------------------------------------------------
std::string ResourceManager::read_shader_file(const std::string& shader_file,
                                              const std::string& shader_type)
{
	// Construir la ruta completa: assets_root_ / shader_file
	std::filesystem::path file_path = assets_root_ / shader_file;

	// Verificar que el archivo existe
	if (!std::filesystem::exists(file_path)) {
		std::cerr << "[ResourceManager] Error: " << shader_type
		          << " file not found: " << file_path << std::endl;
		return "";
	}

	// Abrir el archivo
	std::ifstream file(file_path);
	if (!file.is_open()) {
		std::cerr << "[ResourceManager] Error: could not open " << shader_type
		          << " file: " << file_path << std::endl;
		return "";
	}

	// Leer todo el contenido en un string usando un buffer
	std::ostringstream oss;
	oss << file.rdbuf();

	return oss.str();
}

// ---------------------------------------------------------------------------
// load_shader_source: carga VS/FS/GS desde disco y los cachea.
// ---------------------------------------------------------------------------
const ShaderSource& ResourceManager::load_shader_source(
    const std::string& key,
    const std::string& vs_file,
    const std::string& fs_file,
    const std::string& gs_file)
{
	// Si la clave ya existe, ignorar y advertir
	if (shaders_sources_.find(key) != shaders_sources_.end()) {
		std::cerr << "[ResourceManager] Warning: shader key '" << key
		          << "' already loaded. Ignoring new load request." << std::endl;
		return shaders_sources_.at(key);
	}

	// Crear entrada en el mapa
	ShaderSource& source = shaders_sources_[key];

	// Leer cada archivo de shader
	source.vs = read_shader_file(vs_file, "vertex shader");
	source.fs = read_shader_file(fs_file, "fragment shader");

	if (!gs_file.empty()) {
		source.gs = read_shader_file(gs_file, "geometry shader");
	}

	return source;
}

// ---------------------------------------------------------------------------
// get_shader_source: retorna un ShaderSource cacheado por su clave.
// ---------------------------------------------------------------------------
const ShaderSource& ResourceManager::get_shader_source(const std::string& key) const
{
	auto it = shaders_sources_.find(key);
	if (it == shaders_sources_.end()) {
		std::cerr << "[ResourceManager] Warning: shader key '" << key
		          << "' not found. Returning empty ShaderSource." << std::endl;
		// Retornar un ShaderSource vacío estático (evita dangling reference)
		static const ShaderSource empty{};
		return empty;
	}
	return it->second;
}

// ---------------------------------------------------------------------------
// clear: elimina todos los recursos cacheados.
// ---------------------------------------------------------------------------
void ResourceManager::clear(void)
{
	shaders_sources_.clear();
}
