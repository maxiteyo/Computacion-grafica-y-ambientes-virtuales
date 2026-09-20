#version 460 core
// ============================================================================
// cube.fs — Fragment shader común (Prácticos 03 y 04).
// Pinta cada fragmento con un color PLANO por pieza, recibido por uniform
// (el color ya no viaja en el vértice). uColor es vec4 (RGB + alfa) desde el
// Práctico 04 porque RenderItem transporta el color del objeto como vec4.
// vNormal queda preparado para el debug de normales: descomentar la línea de
// abajo pinta cada normal como un color.
// ============================================================================
in vec3 vNormal;

uniform vec4 uColor;

out vec4 FragColor;

void main()
{
    // Debug de normales: descomentar para ver vNormal como color.
    // FragColor = vec4(vNormal * 0.5 + 0.5, 1.0);

    FragColor = uColor;
}