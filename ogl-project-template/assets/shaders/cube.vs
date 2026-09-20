#version 460 core
// ============================================================================
// cube.vs — Vertex shader del Práctico 03.
//
// Recibe la tupla { position, normal, tex_coords } del VBO (ubicaciones 0, 1 y
// 2). La transformación ya NO se escribe a mano en el shader (como en el
// Práctico 02): llega por uniforms, multiplicando columna-mayor como exige
// GLSL (uAjuste reemplaza a la "perspectiva caja negra", uModel coloca y
// dimensiona la pieza en la escena para que varíe su posición/tamaño/rotación).
// ============================================================================
layout (location = 0) in vec3 aPos;     // posición
layout (location = 1) in vec3 aNormal;  // normal (libre, sin mundo)
layout (location = 2) in vec2 aTexCoords;

out vec3 vNormal;

uniform mat4 uModel;    // modelo de la pieza (traslación * rotación * escala)
uniform mat4 uAjuste;   // ajuste de la "caja negra" (alto/ancho y -1 en Z)

void main()
{
    vNormal = aNormal;
    gl_Position = uAjuste * uModel * vec4(aPos, 1.0);
}