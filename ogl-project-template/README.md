# Práctico 01 y 02 — Intro a OpenGL, GLSL y la malla indexada

**Materia:** Computación gráfica y ambientes virtuales (0494)  
**Institución:** Centro Regional Universitario Córdoba — IUA  
**Fecha:** 21 de agosto de 2026 (rev. Práctico 02: 28-ago-2026)

---

## Descripción

Proyecto introductorio a OpenGL moderno (4.6 core profile) y GLSL, organizado en módulos:

- **Práctico 01 — Parte 1:** un triángulo de color sólido usando el pipeline gráfico.
- **Práctico 01 — Parte 2:** módulo `ResourceManager` (C++ puro, sin OpenGL) que lee
  shaders desde disco, los cachea y los deja disponibles.
- **Práctico 02:** un cubo indexado de 24 vértices / 36 índices con color plano por cara,
  encapsulando el acceso a OpenGL en módulos por funcionalidad y ownership.

## Requisitos

- Linux (probado en Ubuntu/Arch) con Mesa/GL driver o Windows (MSYS2/MinGW)
- Compilador C++17 con soporte para `<filesystem>`
- Librerías: GLFW3, OpenGL, GLAD (incluido en `third_party/`)
- Make

## Estructura del proyecto

```
ogl-project-template/
├── assets/
│   └── shaders/
│       ├── triangle.vs / triangle.fs   # Práctico 01 (triángulo)
│       └── cube.vs / cube.fs           # Práctico 02 (cubo, matriz escrita a mano)
├── src/
│   ├── main.cpp                # Práctico 02: cubo indexado (sin glDelete*)
│   ├── main_test.cpp           # Tests de ResourceManager + primitives (C++ puro)
│   └── core/
│       ├── MeshData.h          # Vertex + MeshData (tupla y malla lado CPU)
│       ├── ResourceManager.h/.cpp   # Lectura/cacheo de fuentes de shaders
│       ├── Shader.h/.cpp            # Programa linkeado (ownership, logs de error)
│       ├── Mesh.h/.cpp              # VAO/VBO/EBO (RAII, ownership)
│       └── Primitives.h/.cpp        # Generador de mallas (cube: 24v/36i)
├── third_party/
│   └── glad/                   # Loader de OpenGL (GLAD)
├── build/                      # Objetos compilados
├── bin/                        # Ejecutables
├── Makefile
└── Makefile.master
```

El `Makefile` ya tiene `./src/core` en `LIB_DIRS`: los nuevos `.cpp` se recolectan solos,
no hay que tocar la compilación al agregar módulos.

## Compilación

### Programa principal (cubo)

```bash
make
```

### Tests (C++ puro, sin OpenGL)

```bash
g++ -std=c++17 -Wall -Wextra src/core/Primitives.cpp src/core/ResourceManager.cpp src/main_test.cpp -o test_rm
./test_rm
```

## Ejecución

```bash
make run
```

Abre una ventana de 800×600 que muestra el cubo con tres caras visibles (rojo +X,
verde +Y, azul +Z) sobre fondo gris oscuro. Presionar `Escape` la cierra. En consola
se informa la cantidad de vértices e índices generados (24 y 36).

## Práctico 02 — Los módulos

### Mesh (`Mesh.h/.cpp`)
Posee el VAO, VBO y EBO (RAII). No copiable; movible; el destructor y `clear()` liberan
(todo `glDelete*` vive acá). `load()` sube VBO+EBO, arma el VAO con atributos posición
y color entrelazados y conecta el EBO en su ranura dedicada. `count()` responde la
cantidad de **índices** (lo que necesita `glDrawElements`).

### Shader (`Shader.h/.cpp`)
Posee el programa linkeado. `compile_from_source()` compila VS+FS, linkea y libera los
objetos intermedios. No abre archivos (los lee `ResourceManager`). Si algo falla:
imprime el log del driver por `stderr` (largo consultado con `GL_INFO_LOG_LENGTH`) y el
objeto queda vacío.

### Primitives (`Primitives.h/.cpp`)
Generador de mallas de lado CPU, sin OpenGL. `primitives::cube(sx, sy, sz)` devuelve
24 vértices y 36 índices con color plano por cara; las caras se recorren en sentido
antihorario mirando desde afuera (preparado para el back-face culling).

### depth buffer
La ventana se crea pidiendo `GLFW_DEPTH_BITS 24` y se habilita `glEnable(GL_DEPTH_TEST)`;
en cada frame se borra `GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT`. Sin depth buffer las
caras del cubo se superpondrían sin ocultarse.

## Práctico 01 — ResourceManager

### Funcionalidades

- **C++ puro**: sin llamadas a la API de OpenGL, solo biblioteca estándar.
- **Lectura de disco**: lee archivos `.vs`, `.fs` y opcionalmente `.gs`.
- **Cacheo**: si el mismo recurso se pide dos veces, la segunda no relee de disco.
- **Manejo de errores**: si algo falla (archivo no encontrado, etc.), imprime aviso en
  `std::cerr` sin romper el programa.

### Decisiones de diseño

| Decisión | Elección | Razón |
|----------|----------|-------|
| Índice del cache | Clave que elige el usuario | Más flexible que el nombre de archivo |
| Clave duplicada con archivos distintos | Ignorar la segunda carga | Evita sorpresas, predecible |
| Notificación de errores | `std::cerr` + retorno vacío | Simple, no requiere excepciones |
| Retorno de `get_shader_source` | Referencia const | Evita copias innecesarias |
| Struct por tipo de recurso | `ShaderSource` con `vs`, `fs`, `gs` | Preparado para ampliación (texturas, modelos) |

### Tests de ResourceManager

El archivo `src/main_test.cpp` verifica:

1. Carga correcta de shaders existentes
2. Cacheo (misma referencia en segunda petición)
3. Clave duplicada se ignora
4. Clave inexistente retorna ShaderSource vacío
5. Archivo inexistente se maneja sin crash
6. `clear()` limpia el cache
7. Recarga después de `clear()` funciona
8. `primitives::cube()` genera 24 vértices y 36 índices (y placa con escala)
9. Layout de `Vertex` (offset del color en el byte 12)

## Ejercicios de la clínica

- **Práctico 01:** sacar un punto y coma al vertex shader y recompilar; el log debe
  aparecer en consola (verificar el chequeo de `GL_COMPILE_STATUS`).
- **Práctico 02:** dibujar con `glDrawArrays(GL_TRIANGLES, 0, 36)` en vez de
  `glDrawElements(...)` y predecir por escrito qué se ve (sin índices el VAO recorre
  36 vértices consecutivos = cubo desordenado). Ninguna de estas fallas produce error
  de OpenGL: hay que predecirlas/visualizarlas.

## Compila sin warnings

```bash
g++ -std=c++17 -Wall -Wextra src/core/Primitives.cpp src/core/ResourceManager.cpp src/main_test.cpp -o test_rm
make
```

## Referencias

- [OpenGL Wiki](https://www.khronos.org/opengl/wiki/)
- [LearnOpenGL](https://learnopengl.com/)
- [GLFW Documentation](https://www.glfw.org/documentation.html)
- Filminas y Guías de aula A y B de la materia