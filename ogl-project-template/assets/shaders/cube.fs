#version 460 core
// ============================================================================
// cube.fs — Fragment shader del cubo del Práctico 02.
// Pinta cada fragmento con el color por vértice (color por cara, entrelazado
// en el VBO por el módulo primitive).
// ============================================================================
in vec3 vColor;

out vec4 FragColor;

void main()
{
    FragColor = vec4(vColor, 1.0);
}