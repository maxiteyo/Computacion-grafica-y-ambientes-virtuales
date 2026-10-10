#version 460 core
// ============================================================================
// cube.vs — Vertex shader del Práctico 05 (cámara en la escena).
//
// Recibe la tupla { position, normal, tex_coords } del VBO (ubicaciones 0, 1 y
// 2). La transformación llega ENTERA por uniforms, multiplicando columna-mayor
// como exige GLSL, y se elimina la "caja negra" (uAjuste, un escalado que solo
// corregía el aspecto y daba vuelta el eje z) en favor de una proyección de
// PERSPECTIVA real calculada por CameraSystem:
//   gl_Position = uProjection * uView * uModel * aPos
// El orden es P·V·M (el modelo se aplica primero). Si se invirtiera, cada
// vértice pasaría por las matrices en el orden equivocado y la escena se
// deformaría (ver preguntas de la consigna en el README).
// ============================================================================
layout (location = 0) in vec3 aPos;     // posición
layout (location = 1) in vec3 aNormal;  // normal (libre, sin mundo)
layout (location = 2) in vec2 aTexCoords;

out vec3 vNormal;

uniform mat4 uModel;      // modelo de la pieza (pose del avión * local)
uniform mat4 uView;       // cámara: mundo -> cámara (orbital)
uniform mat4 uProjection; // cámara -> clip (perspectiva con división por w)

void main()
{
    vNormal = aNormal;
    gl_Position = uProjection * uView * uModel * vec4(aPos, 1.0);
}