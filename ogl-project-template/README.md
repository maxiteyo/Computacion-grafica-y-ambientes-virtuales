# Práctico 01, 02, 03, 04 y 05 — Intro a OpenGL, GLSL, la malla indexada y la cámara

**Materia:** Computación gráfica y ambientes virtuales (0494)  
**Institución:** Centro Regional Universitario Córdoba — IUA  
**Fecha:** 21 de agosto de 2026 (rev. Práctico 02: 28-ago-2026; rev. Práctico 03: 09-sep-2026; rev. Práctico 04: 14-sep-2026; rev. Práctico 05: 18-sep-2026)

---

## Descripción

Proyecto introductorio a OpenGL moderno (4.6 core profile) y GLSL, organizado en módulos:

- **Práctico 01 — Parte 1:** un triángulo de color sólido usando el pipeline gráfico.
- **Práctico 01 — Parte 2:** módulo `ResourceManager` (C++ puro, sin OpenGL) que lee
  shaders desde disco, los cachea y los deja disponibles.
- **Práctico 02:** un cubo indexado de 24 vértices / 36 índices con color plano por cara,
  encapsulando el acceso a OpenGL en módulos por funcionalidad y ownership.
- **Práctico 03:** primitivas paramétricas (cilindro, cono, esfera), tupla `Vertex` sin
  color (pasa a uniform), uniforms GLSL con `glProgramUniform*` (DSA) y una escena de
  cuatro piezas con matriz de modelo.
- **Práctico 04:** módulo `Aircraft` (`init`/`update`/`collect`) que compone una aeronave
  S-211 en 7 piezas que reutilizan 5 mallas del P03, `RenderItem` como ítem de dibujo,
  y una escena única con cabeceo animado alrededor de la punta (el `main` no sabe cómo
  está armada la aeronave).
- **Práctico 05:** cámara en la escena. Se elimina la "caja negra" (`uAjuste`) del vertex
  shader y se reemplaza por matrices de vista y proyección REALES (`uView`/`uProjection`):
  `CameraSystem` (órbita yaw/pitch/distancia + `perspective`) y `InputHandler` (mouse →
  `CameraCommand`), con la escena del P04 y controles con el mouse.

## Requisitos

- Linux (probado en Ubuntu/Arch) con Mesa/GL driver o Windows (MSYS2/MinGW)
- Compilador C++17 con soporte para `<filesystem>`
- Librerías: GLFW3, OpenGL, GLAD (incluido en `third_party/`), GLM (header-only, `/usr/include/glm`)
- Make

## Estructura del proyecto

```
ogl-project-template/
├── assets/
│   └── shaders/
│       ├── triangle.vs / triangle.fs   # Práctico 01 (triángulo)
│       └── cube.vs / cube.fs           # Prácticos 02-05 (uModel/uView/uProjection/uColor)
├── src/
│   ├── main.cpp                # Práctico 05: escena de la aeronave + cámara orbital
│   ├── main_test.cpp           # Tests de ResourceManager + primitives + cámara (C++ puro)
│   └── core/
│       ├── MeshData.h          # Vertex + MeshData (tupla y malla lado CPU)
│       ├── ResourceManager.h/.cpp   # Lectura/cacheo de fuentes de shaders
│       ├── Shader.h/.cpp            # Programa linkeado + set_uniform (DSA, mat4/vec3/vec4)
│       ├── Mesh.h/.cpp              # VAO/VBO/EBO (RAII, ownership)
│       ├── Primitives.h/.cpp        # Generador de mallas (cube/cylinder/cone/sphere)
│       ├── RenderItem.h              # Ítem listo para dibujar (mesh + model + color)
│       ├── Aircraft.h/.cpp           # Aeronave S-211 (init/update/collect)
│       ├── CameraData.h              # view + projection (estructura compartida)
│       ├── CameraCommand.h           # deltas de órbita (yaw/pitch/distancia)
│       ├── CameraSystem.h/.cpp       # Cámara orbital: estado + view + projection
│       └── InputHandler.h/.cpp       # Mouse (posición + botones) -> CameraCommand
├── third_party/
│   └── glad/                   # Loader de OpenGL (GLAD)
├── build/                      # Objetos compilados
├── bin/                        # Ejecutables
├── Makefile
└── Makefile.master
```

El `Makefile` ya tiene `./src/core` en `LIB_DIRS`: los nuevos `.cpp` se recolectan solos,
no hay que tocar la compilación al agregar módulos.

Nota: el Makefile no rastrea dependencias de headers; al cambiar un `.h` conviene
`touch` del `.cpp` asociado o `make clean` antes de recompilar.

## Compilación

### Programa principal (escena)

```bash
make
```

### Tests (C++ puro, sin OpenGL)

```bash
g++ -std=c++17 -Wall -Wextra src/core/Primitives.cpp src/core/ResourceManager.cpp \
    src/core/CameraSystem.cpp src/core/InputHandler.cpp src/main_test.cpp -o test_rm
./test_rm
```

## Ejecución

```bash
make run
```

Abre una ventana de 800×600 que muestra la aeronave S-211 del Práctico 04 (7 piezas,
7 colores, cabeceo `pitch = 0.45 · sin(t)` alrededor de la punta) pero vista por una
**cámara orbital en PROYECCIÓN DE PERSPECTIVA**: la "caja negra" de los prácticos
anteriores ya no existe, la cámara orbita el avión y el tamaño aparente depende de la
distancia. Controles con el mouse:

| Control | Acción |
|---------|--------|
| Arrastrar con botón **izquierdo** | Órbita: mover en X = acimut (yaw), en Y = elevación (pitch) |
| Arrastrar con botón **derecho** | Zoom: mover en Y = acercar/alejar la cámara |
| `Escape` | Cierra la ventana |

El pitch de la órbita está acotado a ±0.9·π/2 (no se "tumba" la cámara) y la distancia a
[2, 50] unidades (no atraviesa el avión ni lo pierde de vista). Al redimensionar la
ventana, la proyección se recalcula con el nuevo aspect (el círculo sigue siendo
círculo) y si se minimiza (alto 0) el aspect se se protege sin dividir por cero.

## Práctico 05 — Los cambios

### La cámara entra en la escena (adiós "caja negra")

Hasta el P04 el vertex shader usaba `uAjuste * uModel`: un escalado fijo
(`glm::scale(vec3(600.0f/800.0f, 1.0f, -1.0f))`) que corregía el aspect y daba vuelta
`z`, con la cuarta fila de la matriz en `(0,0,0,1)`. Al NO haber división por `w`, un
objeto que se aleja no se achica: no era una proyección real. En el P05:

```glsl
uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProjection;
gl_Position = uProjection * uView * uModel * vec4(aPos, 1.0);
```

`uProjection` sale de `glm::perspective(fovy, aspect, near, far)` con `fovy = 45°`,
`near = 0.5` (no "muy chico": precisión del z-buffer) y `far = 500`; `uView` sale de
`glm::lookAt(eye, objetivo, up)` con la cámara de órbita. El orden `P · V · M` es el
canónico (el modelo se aplica primero); invertirlo transforma los vértices en el orden
equivocado y deforma la escena (el compilador no avisa: son matrices del mismo tipo).

### `CameraData` y `CameraCommand` (estructuras compartidas, solo datos)

- `CameraData { view, projection }` : el par de matrices del cuadro.
- `CameraCommand { yaw_delta, pitch_delta, dist_delta }` : cambios de la órbita en
  esféricas, ya interpretados. Signos: `(+)` en `dist_delta` se aleja.

### `CameraSystem` (`CameraSystem.h/.cpp`)

Administra el estado de la cámara y construye ambas matrices por cuadro. **No lee el
mouse, no dibuja (cero OpenGL: testeable sin GPU) y no sabe cómo es la escena**: solo
recibe el punto objetivo y el comando.

- Estado: `yaw_` (longitud), `pitch_` (latitud), `distancia_` (radio). Con los deltas
  del comando se actualiza, se acota y se pasa esféricas → cartesianas:
  `eye = objetivo + d·(cosθ·cosψ, sinθ, cosθ·sinψ)`; `view = lookAt(eye, objetivo, up)`.
- **Clamps** (preguntas de la consigna):
  - `pitch ∈ ±0.9·π/2`: en ±π/2, `up` y la dirección de vista son colineales y el
    producto vectorial interno de `lookAt` se anula → matriz indefinida (la imagen
    "da un salto"). Con ±81° no se llega nunca a ese punto.
  - `distancia ∈ [2, 50]`: el mínimo evita traspasar el objetivo (y el z-buffer), el
    máximo evita perderlo de vista.
- `set_viewport(width, height)`: recalcula la proyección para el aspect nuevo.
  **Comprobación acá** (único consumidor del aspect): `width/height = max(1, valor)`,
  así la ventana minimizada (alto 0) nunca divide por cero y al restaurarse se vuelve
  al aspect correcto (el modelo no se deforma).
- `update()` recibe `angulos_avion` y NO lo usa todavía (la órbita no depende de la
  actitud). Servirá para una vista de **cabina** (Parte 3): ahí la orientación de la
  cámara es la orientación del avión, dada en su propio referencial.

### `InputHandler` (`InputHandler.h/.cpp`)

Traduce el mouse a `CameraCommand` (polling una vez por cuadro, no eventos):

```cpp
double x, y;
glfwGetCursorPos(window, &x, &y);          // POSICIÓN, no desplazamiento
// delta = actual - anterior; se convierte a radianes con una ganancia (px -> rad)
```

- Botón **izquierdo** (órbita): `dx → yaw_delta`, `dy → pitch_delta` (ganancia
  `0.005 rad/px`).
- Botón **derecho** (zoom): `dy → dist_delta` (ganancia `0.02 unidades/px`). El mouse
  es 2D y la órbita 3D: el tercer control entra con el segundo botón.
- **Primer cuadro**: no hay posición anterior → `delta = 0` (`first_ = true`), así la
  cámara no "salta" al empezar. Después `delta = actual - anterior` acumulado.

### `main.cpp`

- `CameraSystem camara(w, h)` e `InputHandler input` viven FUERA del bloque de objetos
  de GPU (son CPU puro) y `glfwSetWindowUserPointer(window, &camara)` guarda el puntero.
- El callback de GLFW es una **función, no un método**: `framebuffer_size_callback`
  recupera la cámara con `glfwGetWindowUserPointer` y llama en orden
  `glViewport(...)` (rectángulo de píxeles) y `camara.set_viewport(...)` (recalcular
  la proyección): son dos cosas distintas y hacen falta las dos.
- En el bucle: posición + botones → `input.poll(...)` → `camara.update(posicion,
  angulos, cmd)` → `shader.set_uniform(locView/locProjection, ...)` → ítems.
- `posicion = (-0.5, 0, 0)`: el avión (largo 1, punta en su origen) queda con su
  CENTRO en el origen del mundo, que es el objetivo de la órbita.
- **`Aircraft` sin base**: la antigua `base` compensaba la proyección 2D; con cámara
  real el avión se dibuja en su referencial natural (morro −X, +Y ala derecha, +Z
  arriba) y el ángulo de vista lo elige la cámara al orbitar.

### Verificación

- **Perspectiva**: con capturas PPM a distancia 10 y 30 (misma órbita) el avión mide
  `78×62` px y `25×21` px: se achica ~3× (10→30). Antes no pasaba (no había `w`).
- **Órbita sin deformación**: al arrastrar el mouse la escena queda quieta y solo se
  mueve el punto de vista; el modelo no cambia de tamaño ni se deforma.
- **Resize sin deformación**: redimensionar la esquina recalcula `perspective` con el
  aspect nuevo (`glViewport` + `set_viewport`).
- **Pitch acotado**: llevar el pitch hasta el tope no da saltos ni pantalla negra
  (clamp de `lookAt`).
- Tests (15): proyección con aspect y división por `w`, view centrando el objetivo con
  clamps de pitch/distancia (sin NaN), alto 0 en `set_viewport`, e `InputHandler`
  (primer cuadro, signos y ganancias).

## Práctico 04 — Los cambios

### `RenderItem` (`RenderItem.h`)

La única información que el `main` necesita por cuadro para dibujar una pieza:

```cpp
struct RenderItem {
    const Mesh* mesh  = nullptr;          // malla a dibujar (referencia: la posee Aircraft)
    glm::mat4   model = glm::mat4(1.0f);  // modelo->mundo YA compuesta (pose * local)
    glm::vec4   color = glm::vec4(1.0f);  // color plano del objeto (uniform)
};
```

El resto del programa no sabe cuántas piezas tiene el avión ni cómo se arma: `collect()`
entrega una lista `std::vector<RenderItem>` y el `main` solo setea `uModel`/`uColor` y
llama `glDrawElements` por cada uno.

### `Aircraft` (`Aircraft.h/.cpp`)

Es el módulo de la consigna: `init()` (una sola vez) + `update()` + `collect()` (por
cuadro). El `main` solo llama `avion.update(pos, angulos) → avion.collect(items) →
dibujar`, sin conocer el interior.

- **Jerarquía de dos niveles:** `locales_` fijos por pieza (se cargan en `init()`) y
  `pose_` de la aeronave completa por cuadro. `modelo = pose_ * locales_[i]` (GLM
  columna-mayor: el local se aplica primero).
- **Sistema de referencia (largo total 1):** origen = punta de la nariz (punto de
  referencia), `+X` hacia la cola, `+Y` hacia el ala derecha, `+Z` arriba. `angulos`
  de `update()` = `(roll, pitch, yaw)` sobre X, Y, Z; el cabeceo rota sobre el eje Y
  (paralelo al ala). `pose_ = T(pos) · base · R(angulos) · T(−ref)`, con `ref = (0,0,0)`,
  así que el morro queda clavado y la cola balancea.
- **Base `base` (orientación de puesta en escena):** la "caja negra" proyecta al plano
  XY de pantalla (el `+Z` del mundo solo participa del test de profundidad). Sin una
  base fija, el `+X` del avión iría al `+X` del mundo y el `+Y` (ala) quedaría
  **vertical** en pantalla (alas "arriba y abajo", el avión leído como rolado 90°).
  `base = Rx(-25°) · Ry(+50°) · Rz(-90°)`:
  - `Rz(-90°)` rota el modelo en planta clásica: morro "arriba" de la ventana, ala
    derecha a la derecha, alas y estabilizadores horizontales.
  - `Ry(+50°)` gira ese plano ~50° hacia el espectador (como abrir un libro): como solo
    se proyectan X e Y, este giro le da **ancho de pantalla** a la altura `+Z` del avión
    → la **deriva** deja de ser un filete vertical y se ve como una aleta roja parada
    en la cola (vista tres cuartos).
  - `Rx(-25°)` eleva levemente la vista: se ve el lomo del avión y el empenaje desde
    arriba ("más de costado"), sin mover el morro (punto de referencia).
  El morro (ref) no se mueve porque el pivote del cabeceo está en `ref` y `base` es
  constante por escena: la rotación de `angulos` se aplica SIEMPRE alrededor de la punta.
- **7 piezas, 5 mallas:** las piezas simétricas (alas y estabilizadores) comparten la
  misma malla de cubo y solo cambia su `local` (traslación); no se duplica GPU.
- **Escalas NO uniformes dentro del generador** `cube(sx, sy, sz)` (regla del
  enunciado): nunca se usa escala negativa para espejar piezas.

| Pieza | Malla | Local (en el referencial del avión) |
|-------|-------|-------------------------------------|
| Nariz | `cone(1/6, 90°, 24)` | `T(1/12,0,0) · Rz(+90°)` (ápice en el origen, base en x=1/6) |
| Fuselaje | `cylinder(1/6, 5/6, 24)` | `T(7/12,0,0) · Rz(−90°)` (continúa la nariz: x ∈ [1/6, 1], mismo radio) |
| Ala der / izq | `cube(0.15, 7/15, 0.02)` | `T(0.45, ±7/30, 0)` (+Y derecha, −Y izquierda) |
| Estab. horiz. der / izq | `cube(0.08, 0.25, 0.01)` | `T(0.94, ±1/8, 0.06)` (+Y derecha, −Y izquierda) |
| Deriva | `cube(0.15, 0.018, 13/60)` | `T(0.95, 0, 0.275)` |

El largo total es 1: la nariz (1/6) más el fuselaje (5/6) suman la eslora, y el cono
tiene el MISMO radio (1/6) que el cilindro en la junta → el perfil es continuo.

**Preguntas de la consigna (respondidas en el código, ver el final de `Aircraft.cpp`):**
- `collect()` antes de `init()` devuelve lista vacía (no rompe); antes de `update()`
  dibuja con `pose_` identidad (origen, sin rotación). El orden es `update()` →
  `collect()`.
- Regenerar las mallas por frame sería desperdicio: `init()` las genera y sube a la GPU
  una sola vez; por cuadro solo cambia `pose_`. Si la aeronave viniera de un `.obj`
  habría un mesh por objeto cargado en `init()`, locales = identidad y colores desde
  los materiales del archivo; `pose_` sería idéntica.

### Shader (`Shader.h/.cpp`) y `cube.fs`

`Shader` suma las sobrecargas `set_uniform(int|std::string, glm::vec4)` con
`glProgramUniform4fv` (DSA, igual estrategia que `mat4`/`vec3`). `cube.fs` pasa el color
como `uniform vec4 uColor` (RGBA) y pinta `FragColor = uColor;`; el debug de normales
sigue comentado.

### `main.cpp`

Remplaza la grilla 2×2 del P03 por la escena única:
`Aircraft avion; avion.init();` y, en el bucle, `avion.update(posicion, angulos)` y
`avion.collect(items)` con `items` reciclado (lo limpia `collect()`). El resto del
esqueleto (GLFW/GLAD, `uAjuste`, depth test, la herramienta PPM comentada para verificar)
no cambia.

### Verificación visual (cabeceo alrededor de la punta, vista tres cuartos)

En `main.cpp` hay una herramienta comentada con `glReadPixels` para volcar frames a PPM
(voltear verticalmente al interpretar). Con dos capturas (una con `pitch≈0` y otra cerca
del máximo, `t≈1.57 s`) se verificó el 14-sep-2026:

- El ápice de la nariz (gris pizarra) queda en la MISMA fila superior (`y ≈ 150`) en
  ambas capturas → la punta es el punto fijo del cabeceo.
- La **deriva (rojo)** deja de ser un filete de 6 px: pasa a `59×50` (pitch≈0) y `64×53`
  (máximo) px, parada sobre el empenaje (`x[388..446]`/`x[330..393]` en la cola) → se
  distingue la aleta vertical.
- Las alas salen **horizontales**: azul (derecha) arriba del plano del ala, verde
  (izquierda) debajo (asimetría esperada de la vista tres cuartos con elevación);
  su posición y tamaño cambian al cabecear (ej. azul `x[422..474]` → `x[393..453]`)
  → el avión rota sobre el eje del ala (Y del avión) alrededor del morro.
- Estabilizadores visibles en la cola: naranja (derecho) y amarillo (izquierdo), con la
  deriva roja entre/encima de ambos; el fuselaje plateado conecta la nariz con la cola.
- Las 7 piezas con sus 7 colores distintos se ven en las capturas.

## Práctico 03 — Los cambios

### Tupla `Vertex` (`MeshData.h`)

`Vertex` ya no lleva color: es `glm::vec3 position; glm::vec3 normal; glm::vec2
tex_coords` (32 bytes, offsets 0/12/24). El color pasa a un uniform por pieza
(`uColor`), lo que permite reutilizar la misma malla con colores distintos.

### Primitivas (`Primitives.h/.cpp`)

`primitives::cube(sx, sy, sz)` (24v/36i) y nuevas primitivas paramétricas. Convenciones
documentadas en el header:

| Primitiva | Vértices (N=12) | Índices (N=12) | Eje | UV |
|-----------|----------------|----------------|-----|----|
| `cylinder(radio, largo, gajos)` | 52 | 144 | longitudinal Y, `-largo/2..+largo/2` | `u = i/gajos`, `v = anillo/anillos` |
| `cone(radio, conicidad°, gajos)` | 39 | 108 | eje Y, ápice en `+h/2` con `h = radio/tan(semiángulo)` | ídem |
| `sphere(radio, gajos, anillos)` | (M+1)(N+1) | 6·MN | polar Y | `v=0` polo sur, `v=1` polo norte |

- **Conicidad en grados** en la interfaz pública = ángulo de apertura total; adentro se
  convierte con `glm::radians()`.
- **Vértices duplicados** en toda costura/cierre: costura `u=0`/`u=1`, tapas separadas
  del lateral, y ápice (cono) y polos (esfera) como **anillos degenerados** (un vértice
  por gajo, con la normal de su gajo) para mantener la misma topología de índices.
- Triángulos en **sentido antihorario mirando desde afuera** (CCW); con el culling
  deshabilitado se valida por producto vectorial `(B−A)×(C−A)` en los tests.

### Shader (`Shader.h/.cpp`)

Agrega `loc(name)` (devuelve −1 sin avisar) y `set_uniform(int|std::string, glm::mat4 |
glm::vec3)` que usan `glProgramUniformMatrix4fv`/`glProgramUniform3fv` (**DSA**, no
exige `use()` antes). Los setters por nombre avisan por `stderr` si falta el uniform.

### `main.cpp`

- `Piece { Mesh, glm::mat4 model, glm::vec3 color }`; las 4 mallas se generan **una sola
  vez** antes del bucle; en cada frame solo se seteán `uModel`/`uColor` por pieza y se
  llama `glDrawElements`. Las ubicaciones de uniforms se consultan también una vez.
- `uAjuste` = `glm::scale(vec3(alto/ancho, 1, -1))` es la matriz de aspecto de la "caja
  negra": con 800×600 el mundo visible es `x ∈ [-1.33, 1.33]`, `y ∈ [-1, 1]`. Se setea
  una sola vez; el shader compone `gl_Position = uAjuste * uModel * vec4(aPos, 1.0)`.
- Los shaders `cube.vs`/`cube.fs` usan `aPos`(0), `aNormal`(1), `aTexCoords`(2) con
  `offsetof` y un uniform `uColor`. Dejan preparada una línea de debug que pinta las
  normales como color (ver "Modo de depuración de normales" más abajo).

### Verificación visual

Para comprobar el layout sin depender del compositor se deja comentada en `main.cpp`
una herramienta que vuelca el primer frame con `glReadPixels(GL_BACK)` (ANTES del swap)
a un PPM. La imagen sale invertida verticalmente (en GL `(0,0)` es abajo-izquierda):
hay que voltearla con `Image.FLIP_TOP_BOTTOM` al interpretarla.

### Modo de depuración de normales

Las normales todavía no se usan (todavía no hay iluminación), de modo que una normal
mal calculada no se nota en pantalla. Para poder "verlas", la consigna propone
intercambiar las dos líneas del final de `cube.fs`:

```glsl
// FragColor = vec4(uColor, 1.0);
FragColor = vec4(vNormal * 0.5 + 0.5, 1.0);  // la normal, como color
```

Cada dirección de normal se ve de un color distinto. Qué esperar al correr (criterio
de la consigna "cómo se sabe que está bien", verificado el 09-sep-2026):

- **Cubo**: seis caras de **color plano** (la normal es constante dentro de cada cara).
- **Lateral de cilindro, cono y esfera**: un **degradé suave** alrededor del eje de
  simetría de revolución (la normal gira de vértice en vértice).

Recompilar con `make` y correr para inspeccionar; al terminar volver a activar la
línea `vec4(uColor, 1.0)` para dejar la escena final. El complemento numérico (sin
dibujar nada) es el producto vectorial `(B−A)×(C−A)` de los tests 10–12, que compara
el orden de los triángulos con la normal de sus vértices.

## Práctico 02 — Los módulos

### Mesh (`Mesh.h/.cpp`)
Posee el VAO, VBO y EBO (RAII). No copiable; movible; el destructor y `clear()` liberan
(todo `glDelete*` vive acá). `load()` sube VBO+EBO, arma el VAO con atributos posición,
normal y tex_coords entrelazados y conecta el EBO en su ranura dedicada. `count()`
responde la cantidad de **índices** (lo que necesita `glDrawElements`).

### Shader (`Shader.h/.cpp`)
Posee el programa linkeado. `compile_from_source()` compila VS+FS, linkea y libera los
objetos intermedios. No abre archivos (los lee `ResourceManager`). Si algo falla:
imprime el log del driver por `stderr` (largo consultado con `GL_INFO_LOG_LENGTH`) y el
objeto queda vacío.

### Primitives (`Primitives.h/.cpp`)
Generador de mallas de lado CPU, sin OpenGL. `primitives::cube(sx, sy, sz)` devuelve
24 vértices y 36 índices con normal por cara y tex_coords; las caras se recorren en
sentido antihorario mirando desde afuera (preparado para el back-face culling).

### depth buffer
La ventana se crea pidiendo `GLFW_DEPTH_BITS 24` y se habilita `glEnable(GL_DEPTH_TEST)`;
en cada frame se borra `GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT`. Sin depth buffer las
piezas (y sus caras) se superpondrían sin ocultarse.

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
9. Layout de `Vertex` sin color (sizeof 32, offsets 0/12/24)
10. `primitives::cylinder()`: 52v/144i, costura duplicada, tapas con normal axial, CCW
11. `primitives::cone()`: 39v/108i, altura deducida de la conicidad, ápice degenerado, CCW
12. `primitives::sphere()`: 589v/3240i, polos degenerados, CCW

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
- [GLM — OpenGL Mathematics](https://github.com/g-truc/glm)
- Filminas y Guías de aula A y B de la materia