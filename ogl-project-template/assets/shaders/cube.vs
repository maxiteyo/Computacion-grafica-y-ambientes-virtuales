#version 460 core
// ============================================================================
// cube.vs — Vertex shader del cubo del Práctico 02.
//
// La matriz de transformación se escribe "a mano" dentro del shader (ver
// filminas). Es el producto (columna-mayor, como exige GLSL):
//
//   MVP = perspective(60°, aspect 4:3, near 0.1, far 100)
//         * translate(0, 0, -3)
//         * rotateY(-45°)
//         * rotateX(+35°)
//
// Sin esta transformación el cubo se vería como un cuadrado (proyección "de
// frente"); con ella se ve una perspectiva isométrica que muestra tres caras:
// +X (rojo), +Y (verde) y +Z (azul).
// ============================================================================
layout (location = 0) in vec3 aPos;    // posición
layout (location = 1) in vec3 aColor;  // color por vértice

out vec3 vColor;

void main()
{
    // Matriz MVP constante (columna-mayor; escrita a mano).
    mat4 MVP = mat4(
        vec4( 0.918559,  0.000000, -0.708522, -0.707107),
        vec4(-0.526864,  1.418813, -0.406392, -0.405580),
        vec4(-0.752439, -0.993464, -0.580388, -0.579228),
        vec4( 0.000000,  0.000000,  2.805806,  3.000000)
    );

    vColor = aColor;
    gl_Position = MVP * vec4(aPos, 1.0);
}