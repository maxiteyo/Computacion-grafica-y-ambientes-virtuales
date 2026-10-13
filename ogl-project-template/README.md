# Práctico 01, 02, 03, 04, 05, 06 y 07 — OpenGL, GLSL, la malla indexada, la cámara, la simulación de vuelo (FDM) y el terreno procedural

**Materia:** Computación gráfica y ambientes virtuales (0494)  
**Institución:** Centro Regional Universitario Córdoba — IUA  
**Fecha:** 21 de agosto de 2026 (rev. Práctico 02: 28-ago-2026; rev. Práctico 03: 09-sep-2026; rev. Práctico 04: 14-sep-2026; rev. Práctico 05: 18-sep-2026; rev. Práctico 06: 10-oct-2026; rev. Práctico 07: 12-oct-2026)

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
- **Práctico 06:** game loop con acumulador de paso fijo + un MODELO DE DINÁMICA DE VUELO
  (FDM). La aeronave deja de animarse con `sin(t)`: se integra con el módulo `libs/dlfdm`
  (S-211 jet trainer) a `dt = 1/120 s`, arrancando en la condición de equilibrio (trim) a
  5000 m y 150 m/s; el teclado pasa por rampas a las posiciones de los mandos
  (`InputHandler: update/controls/set_controls`), y el estado NED del FDM se transforma al
  mundo de la escena con `FlightData::to_world()` (única conversión de referenciales).
- **Práctico 07:** TERRENO procedural. Una malla N×N se genera por programa con
  `terrain::build_grid()` (función libre, alturas desacopladas en un `HeightField`), las
  alturas salen de ruido Perlin/FBm (`NoiseHeights`, semilla fija → determinista), se
  calculan NORMALES por diferencias centradas, se habilita el back-face culling (el winding
  se corrige en el generador, NO con `glFrontFace`) y el terreno es ILIMITADO: una VENTANA
  deslizante (`TerrainWindow`) sigue al avión por el heightmap, moviéndose a saltos de una
  celda (sin "reptar") y repartiendo el recálculo entre cuadros (sin tirones).

## Requisitos

- Linux (probado en Ubuntu/Arch) con Mesa/GL driver o Windows (MSYS2/MinGW)
- Compilador C++17 con soporte para `<filesystem>`
- Librerías: GLFW3, OpenGL, GLAD (incluido en `third_party/`), GLM (header-only, `/usr/include/glm`)
  y FastNoiseLite (header-only, incluido en `third_party/`)
- Make

## Estructura del proyecto

```
ogl-project-template/
├── assets/
│   └── shaders/
│       ├── triangle.vs / triangle.fs   # Práctico 01 (triángulo)
│       └── cube.vs / cube.fs           # Prácticos 02-06 (uModel/uView/uProjection/uColor)
├── src/
│   ├── main.cpp                # Práctico 07: game loop de paso fijo + FDM + cámara orbital + terreno
│   ├── main_test.cpp           # Tests de RM + primitives + cámara + FDM/input + terreno (C++ puro)
│   └── core/
│       ├── MeshData.h          # Vertex + MeshData (tupla y malla lado CPU)
│       ├── ResourceManager.h/.cpp   # Lectura/cacheo de fuentes de shaders
│       ├── Shader.h/.cpp            # Programa linkeado + set_uniform (DSA, mat4/vec3/vec4)
│       ├── Mesh.h/.cpp              # VAO/VBO/EBO (RAII); update_vertices (P07, VBO sin recrear)
│       ├── Primitives.h/.cpp        # Generador de mallas (cube/cylinder/cone/sphere)
│       ├── RenderItem.h              # Ítem listo para dibujar (mesh + model + color)
│       ├── Aircraft.h/.cpp           # Aeronave S-211 (init/update/collect + set_scale)
│       ├── CameraData.h              # view + projection (estructura compartida)
│       ├── CameraCommand.h           # deltas de órbita (yaw/pitch/distancia)
│       ├── CameraSystem.h/.cpp       # Cámara orbital: estado + view + projection (+set_orbit P07)
│       ├── InputHandler.h/.cpp       # Mouse + teclado -> CameraCommand y mandos del FDM
│       ├── FlightData.h/.cpp         # NED -> mundo (to_world: única transformación)
│       ├── Terrain.h/.cpp            # P07: build_grid + grid_normal + window_origin (C++ puro)
│       ├── NoiseHeights.h/.cpp       # P07: HeightField de ruido (Perlin/FBm, semilla fija)
│       └── TerrainWindow.h/.cpp      # P07: ventana deslizante + reconstrucción progresiva
├── third_party/
│   ├── glad/                   # Loader de OpenGL (GLAD)
│   └── FastNoiseLite/          # P07: ruido procedural, header-only (dependencia del proyecto)
├── libs/
│   └── dlfdm/                  # FDM del Práctico 06 (Makefile.master propio; ver abajo)
├── build/                      # Objetos compilados
├── bin/                        # Ejecutables
├── Makefile
└── Makefile.master
```

El `Makefile` ya tiene `./src/core` en `LIB_DIRS`: los nuevos `.cpp` se recolectan solos,
no hay que tocar la compilación al agregar módulos.

A partir del Práctico 06 el `Makefile` también incluye el FDM del repo con
`DLFDM_DIR = ./libs/dlfdm` + `include $(DLFDM_DIR)/dlfdm.mk` (aporta `DLFDM_INC_DIRS` y
`DLFDM_LIB_DIRS`): las 7 fuentes del módulo se compilan al proyecto con los mismos
`-Wall -Wextra`, sin warnings. El módulo tiene su propio `Makefile.master` si se quiere
compilar aislado (genera `build/dlfdm-test`: 60 s de vuelo off-line con `.csv`).

Nota: el Makefile no rastrea dependencias de headers; al cambiar un `.h` conviene
`touch` del `.cpp` asociado o `make clean` antes de recompilar.

## Compilación

### Programa principal (escena)

```bash
make
```

### Tests (C++ puro, sin OpenGL)

```bash
g++ -std=c++17 -Wall -Wextra -I./libs/dlfdm/include \
    -I./third_party/FastNoiseLite \
    libs/dlfdm/src/dlfdm/fdmsolver.cpp \
    libs/dlfdm/src/dlfdm/aerodynamicsmodel.cpp \
    libs/dlfdm/src/dlfdm/aircraftdynamics.cpp \
    libs/dlfdm/src/dlfdm/propulsionmodel.cpp \
    libs/dlfdm/src/dlfdm/models/aircraft/jettrainer.cpp \
    libs/dlfdm/src/dlfdm/models/propulsion/turbofan.cpp \
    libs/dlfdm/src/dlfdm/models/propulsion/turbojet.cpp \
    src/core/Primitives.cpp src/core/ResourceManager.cpp \
    src/core/CameraSystem.cpp src/core/InputHandler.cpp \
    src/core/FlightData.cpp src/core/Terrain.cpp src/core/NoiseHeights.cpp \
    src/main_test.cpp -o /tmp/opencode/test_rm \
    -lglfw -lpthread
/tmp/opencode/test_rm
```

(`InputHandler.cpp` lee el teclado por GLFW en `update()`, por eso `-lglfw` en el link;
los 22 tests corren sin ventana ni GPU. `TerrainWindow.cpp` NO se compila en los tests
porque usa OpenGL: el terreno se prueba con las funciones puras de `Terrain.cpp`
(`build_grid`/`grid_normal`/`window_origin`) y `NoiseHeights.cpp`.)

## Ejecución

```bash
make run
```

Abre una ventana de 800×600 que muestra la aeronave S-211 del Práctico 04 (7 piezas,
7 colores) volada por el **FDM en la condición de equilibrio (trim)** a 5000 m de altura y
~150 m/s TAS (~540 km/h), **sobre un terreno procedural** (Práctico 07), con la cámara
orbital del Práctico 05 siguiéndola en la escala del mundo en METROS (`CameraSystem` usa
`near=5`/`far=10000` y distancia de órbita [30, 2000]; el modelo del avión, que mide 1, se
escala visualmente con `set_scale(9.6)`). El terreno está **elevado** a una base de ~4200 m
(`kTerrainBase`), así que el avión (trim a 5000 m) vuela a **~800 m sobre el suelo** y el
terreno **llena casi todo el cuadro** en la vista de seguimiento (con la tecla **V** la cámara
salta a una vista de terreno mirando el origen desde arriba, a la altura del terreno).
La vista inicial es de "chase" 3/4 a **60 m** (el avión se ve a una talla legible) y el
zoom del mouse es **proporcional a la distancia** (mismo "paso" de zoom en 60 m que en
2000 m; todo el rango de órbita se recorre en ~150 px de arrastre).
Controles:

| Control | Acción |
|---------|--------|
| Arrastrar con botón **izquierdo** | Órbita: mover en X = acimut (yaw), en Y = elevación (pitch) |
| Arrastrar con botón **derecho** | Zoom: arrastrar hacia **arriba** acerca, hacia abajo aleja (proporcional a la distancia) |
| **↑ / ↓** | Palanca adelante/atrás → elevador (nariz abajo/arriba) |
| **← / →** | Bastón izquierda/derecha → alerones (ala izquierda/derecha abajo) |
| **Q / E** | Pedal izquierdo/derecho → timón (nariz izquierda/derecha) |
| **W / S** | Subir / bajar potencia (al soltar, vuelve a la del trim) |
| **P** | Pausa la simulación (la cámara sigue orbitando) |
| **L** | Alterna wireframe / relleno (con el terreno en wireframe se ve la grilla N×N) |
| **V** | Vista de terreno: reapunta la cámara al **origen del mundo** (a la altura del terreno, mirando hacia abajo) / vuelve a la de chase |
| `Escape` | Cierra la ventana |

Al **soltar** cualquiera de las teclas de mandos (↑ ↓ ← → Q E W S) el mando **vuelve solo
al trim** ("resorte"): la tecla mueve el mando mientras esté apretada y al soltarla la
rama se afloja hacia la posición de equilibrio (`set_controls(trim.controls)`); el avión
no queda girando solo con la deflexión retenida.

El pitch de la órbita está acotado a ±0.9·π/2 (no se "tumba" la cámara) y la distancia a
[30, 2000] metros (no atraviesa el avión ni lo pierde de vista, y el recorte lejano cae
siempre dentro de la ventana de terreno, ver Práctico 07). Al redimensionar la
ventana, la proyección se recalcula con el nuevo aspect (el círculo sigue siendo
círculo) y si se minimiza (alto 0) el aspect se protege sin dividir por cero.

## Práctico 06 — Los cambios

### El game loop: paso fijo con acumulador (Parte 1 de la consigna)

El bucle de `main.cpp` ya no avanza el mundo con el tiempo del cuadro: se integra con un
paso CONSTANTE y PEQUEÑO, y el tiempo real de cada cuadro se acumula hasta completar
pasos discretos:

```cpp
const double dt = 1.0 / 120.0;       // paso de integración del FDM
double acc = 0.0;
while (...) {
    glfwPollEvents();
    double ahora = glfwGetTime();
    double frame_dt = ahora - antes;  antes = ahora;

    input.update(window, frame_dt, camara.distance());  // 1) entrada UNA vez por cuadro

    acc += std::min(frame_dt, 0.25);  // 2) acumular (tope: 0.25 s)
    while (acc >= dt) {               // 3) consumir pasos fijos
        fdm.update(input.controls()); //    (el FDM ya integra con su dt)
        acc -= dt;
    }
    // 4) getState() UNA vez por cuadro -> escena
    const auto& ned = fdm.getState();
    ...
}
```

- **Errores acotados y deterministas**: el error de cada paso no depende de qué tan
  rápido corra la máquina (misma velocidad con o sin vsync: solo cambia la cantidad de
  pasos por cuadro, nunca el paso en sí).
- **El acumulador no se descarta**: lo que sobra tras un paso (resto < dt) se conserva
  para el cuadro siguiente (tiempo simulado + resto = tiempo de reloj).
- **Tope de 0.25 s por cuadro**: si un cuadro tarda mucho (arrastrar la ventana, pausa
  del depurador con el proceso congelado), se simulan como máximo ~30 pasos y la app
  sigue viva: al volver el cuadro no hay "explosión" de pasos acumulados.
- **La entrada está FUERA del `while` de pasos**: el teclado/mouse es un ESTADO ("¿está
  apretada ahora?"), no una acción; el FDM recibe las mismas posiciones de mandos en
  todos los pasos del cuadro, sin depender del número de pasos.
- **`getState()` se consulta UNA vez por cuadro** (Parte 3 de la consigna): el estado ya
  integrado se transforma a la escena y se reparte a cámara y avión.

### El FDM entra al proyecto (`libs/dlfdm`)

- El módulo del repositorio (S-211 jet trainer, modelado con la tabla de estabilidad del
  Aermacchi S-211 real) vive en `libs/dlfdm` y se integra con
  `DLFDM_DIR = ./libs/dlfdm` + `include $(DLFDM_DIR)/dlfdm.mk` en el `Makefile`: sus
  7 fuentes se compilan al proyecto con `-Wall -Wextra` (cero warnings) y sin colisiones
  de objetos (`Makefile.master` genera objetos `notdir` en `build/`).
- **Condición inicial = equilibrio (trim)** (Parte 3): se pide con
  `jettrainer::get_trim_condition(kISA5000TAS150)` — el estado que hace que la aeronave
  vuele en línea recta nivelada a 5000 m / 150 m/s TAS — y se aplica con
  `fdm.setState(trim.state)`; las posiciones de los mandos del trim (elevador −5.37°,
  potencia ~0.50) se fijan en el `InputHandler` con `set_controls(trim.controls)`.
- **Interfaz usada por el proyecto**: `FDMSolver::update(ControlInputs)` (nada de
  métodos físicos internos) y `FDMSolver::getState()` devuelve un `AircraftState` NED
  (x=norte, y=este, z=abajo) con la actitud aeroespacial `(phi, theta, psi)`.

### `FlightData` — la ÚNICA transformación NED → mundo

`FlightData.h/.cpp` es la frontera entre los dos mundos: entrada `dlfdm::AircraftState`,
salida lo que consume la escena.

- **Posición** — exactamente la fórmula de la consigna, negando DOS ejes (nunca uno, que
  daría una reflexión y un mundo espejado):
  `position = (ned.y, −ned.z, −ned.x)` → x=este, y=arriba, z=−norte.
- **Actitud** — decisión documentada: `to_world()` devuelve `(phi, theta, psi)` del NED
  TAL CUAL. El "sign-copy" de la consigna (`−φ, θ, −ψ`) presupone un modelo de escena
  con el morro en **+X**; el de este proyecto (Práctico 04) tiene el morro en **−X**.
  En vez de copiar signos (que con este modelo no representan ningún ángulo válido) se
  aplica una base CONSTANTE al componer la rotación en `Aircraft` (ver abajo): el
  resultado visual es el mismo que busca la consigna (rumbo 0 = morro a −Z, pitch + =
  nariz arriba, roll + = ala derecha abajo), sin extraer ángulos equivalentes (que
  degenera en ciertas actitudes).
- `to_world_basis()` es la fuente ÚNICA de esa base (la usan `to_world`,
  `forward_world` y `Aircraft`); `forward_world()` es una consulta auxiliar (dirección
  del morro, verificada en los tests y útil para la futura vista de cabina).

### `Aircraft` — escala en metros y la base del referencial

- `set_scale(float)`: el modelo mide 1; en el mundo en metros se usa `set_scale(9.6)`.
  La pose pasa a `T(pos) · R_avion · S(escala) · T(−ref)`.
- `update()` compone la rotación como
  `R_avion = fp::to_world_basis() · Rz(psi)·Ry(theta)·Rx(phi) · kBaseModelo`, donde
  `Rz·Ry·Rx` es la rotación cuerpo→NED del convenio aeroespacial (Stevens-Lewis: el
  rumbo/cabeceo definen la dirección del morro, el roll positivo baja el ala derecha) y
  `kBaseModelo = diag(−1,+1,−1)` adapta el referencial del cuerpo (+X morro, +Z abajo)
  al del modelo geométrico (morro −X, +Z arriba). El avión vuela "de morro": sin base de
  puesta en escena 2D del P04 — esta base es la conversión de referenciales, constante.

### `InputHandler` — el teclado se vuelve mandos de la aeronave

Se conserva el mouse → `CameraCommand` del P05 y se agrega:

- `update(GLFWwindow*, float dt, float camera_distance = 150)`: polling completo una vez
  por cuadro (mouse + teclado). El tercer parámetro es la distancia actual de la cámara:
  el zoom del mouse es **proporcional** (`dist_delta = dy · kZoomGain · distancia`, con
  `kZoomGain = 0.03` = 3 % de la distancia por px) porque la escena está en metros
  (con la ganancia absoluta del P05, acercarse del todo exigía arrastrar miles de px).
  Convención: arrastrar hacia **arriba** acerca (dy < 0 → dist_delta < 0).
- `controls()` → `dlfdm::ControlInputs` (elevador, alerón, timón en rad y potencia en
  [0,1]): lo único que consume el FDM.
- `set_controls(c)`: fija la posición inicial de los mandos (el trim de la Parte 3),
  inicializa las rampas internas para que partan de ahí y memoriza ese punto como la
  "casa" del resorte.
- `update_sticks(...)`: la lógica pura de mandos (C++ puro, testeable): cada tecla mueve
  la palanca/pedales y fija la VELOCIDAD de cambio de un eje normalizado
  (`eje_ ±= rate·dt`, `rate = 1.0 /s`; potencia `0.75 /s`), que luego se mapea al tope de
  la superficie. **Al soltar la tecla el mando VUELVE a la posición del trim** (resorte,
  `rate = 2.5 /s`; potencia `1.25 /s`): las rampas se aflojan hacia el punto memorizado
  por `set_controls()` (sin trim, hacia el neutro 0). Con esto el avión no queda girando
  solo tras un "tap" corto — antes de ajustarlo, la tecla soltada MANTENÍA la deflexión y
  un toque de 0,3 s dejaba ~60 % del mando retenido para siempre (el FDM entraba en una
  espiral cada vez más rápida).

| Tecla | Eje | Deflexión que produce (jettrainer) |
|-------|-----|--------------------------------------|
| ↑ / ↓ | elevador | +15° (nariz abajo) / −25° (nariz arriba) |
| ← / → | alerones | −20° / +20° (ala izq./der. abajo) |
| Q / E | timón | +30° / −30° (nariz izq./der.) |
| W / S | potencia | sube / baja en [0, 1] |

Con el bastón en el neutro (eje 0) la deflexión es 0; el trim del FDM queda como una
deflexión previa (elevador −5.37°, potencia ~0.50; alerón y timón en 0): desde ahí
parte la rampa al tocar una tecla y a AHÍ vuelve el mando al soltarla.

### Eventos discretos (P y L)

El teclado vuelve a ser "estado o evento": la posición de un mando es estado (queda
apretada), pero pausar / cambiar el modo de dibujo es un EVENTO (una acción que solo
debe pasar UNA vez por pulsación). En `main.cpp` se detecta el borde con el estado de la
tecla del cuadro anterior:

```cpp
bool p_prev = false;
// en el bucle:
bool p_cur = glfwGetKey(window, GLFW_KEY_P) == GLFW_PRESS;
if (p_cur && !p_prev) pause_on = !pause_on;   // flanco: alterna UNA vez
p_prev = p_cur;
```

- **P → pausa**: congela la integración (`acc = 0` para que al reanudar no "pegue"
  todos los pasos acumulados) pero la cámara sigue orbitando (sigue a un avión inmóvil).
- **L → wireframe**: alterna `glPolygonMode(GL_FRONT_AND_BACK, GL_LINE/GL_FILL)`.

### Verificación

- **Tests (18, C++ puro)**: los 15 del P05 más:
  - **16** `FlightData`: `to_world` (posición x=este/y=arriba/z=−norte) y
    `forward_world` (morro alineado con el vuelo: rumbo 0 → −Z, rumbo 90 → +X, el
    alabeo no cambia la dirección del morro).
  - **17** FDM real: integrando 60 s desde el trim sin tocar mandos, la altura queda en
    5000 ± 25 m y la TAS en ~150 ± 2 m/s (la aeronave mantiene el equilibrio).
  - **18** `InputHandler`: rampas por velocidad, topes de cada superficie (jettrainer),
    potencia acotada a [0,1], **soltar la tecla devuelve el mando al trim** (resorte,
    sin pasarse) y `set_controls()` arranca desde el trim y fija la "casa" del resorte.
- **Misma velocidad con o sin vsync**: `glfwSwapInterval(1)` en `make run`; cambiar a 0
  en `main.cpp` y recompilar → la posición/actitud del avión en un momento dado es la
  misma (el paso fijo no depende del rate de cuadros).
- **Sin teclas, mantiene altura y velocidad**: Test 17 (numérico) + la app arranca en
  el trim y no se toca nada → el avión sigue recto y nivelado (el morro no cambia).
- **Deflexión sostenida** (tecla sostenida): el avión rota en el eje y sentido correctos
  (↓ → palanca atrás → nariz arriba con elevador negativo; ↑ → palanca adelante → nariz
  abajo con elevador positivo; roll con alerones; giro con el timón). Al **soltar**, los
  mandos vuelven al trim (Test 18) y el FDM deja de recibir
  deflexión: verificado también numéricamente contra el comportamiento anterior — con la
  deflexión retenida (antes) un alerón sostenido llegaba a φ ≈ −9356° en 30 s (espiral
  divergente, giraba cada vez más rápido); con el resorte, tras un toque de 0,3 s el
  cabeceo vuelve solo al nivel y el alabeo queda acotado y sin acelerar (espiral neutra
  propia del modelo, igual que en un avión real sin piloto automático).
- **Tamaño del modelo en cámara**: con la vista inicial de 60 m una captura PPM da el
  avión en 118×87 px (a los 150 m de antes quedaba en 44×32 px) y el zoom proporcional
  del mouse recorre todo el rango [30, 3000] m en ~150 px de arrastre.
- **Arrastrar la ventana / pausa del depurador**: el tope de 0.25 s evita la divergencia
  del acumulador; al soltar la ventana la simulación continúa sin saltos.
- **Comando discreto una vez por pulsación**: la detección de borde (P, L) alterna una
  sola vez aunque se sostenga la tecla.

## Práctico 07 — Los cambios

### Objetivo

Sobre la escena del P06 (FDM + cámara orbital) se agrega un **terreno procedural**: una
malla poligonal N×N generada por programa a partir de una **función altura** (ruido), con
**normales** calculadas, **back-face culling** y comportamiento de terreno **ilimitado**
mediante una **ventana deslizante**. Se documenta también el tamaño del mundo elegido y el
comportamiento del borde.

### La malla es una función libre (`terrain::build_grid`)

`src/core/Terrain.h/.cpp` — C++ puro, **sin OpenGL ni ruido**: testeable sin GPU.

- **Topología**: para `celdas = N`, `(N+1)²` vértices en orden **fila-mayor**
  (`idx(i,j) = i·(N+1) + j`, con `i` = fila = z y `j` = columna = x) y `6·N²` índices
  (dos triángulos por celda: `(a,d,c)` y `(a,c,b)`, con
  `a=(i,j), b=(i,j+1), d=(i+1,j), c=(i+1,j+1)`). El bucle de vértices recorre `fila = N+1`
  con `<= N` y el de celdas recorre `< N` (por eso los vértices por lado son `N+1`).
- **Posición LOCAL**: `(j·paso, h(ox+j, oz+i), i·paso)`. El origen `(ox, oz)` NO entra en los
  vértices: se aplica con la **matriz de modelo** (traslación). Así el mundo es infinito y
  la malla es una **ventana** sobre los índices enteros del heightmap.
- **Alturas desacopladas de la malla**: `build_grid` recibe un `HeightField` (interfaz
  `float operator()(int nx, int nz)`) y lo evalúa en los **NODOS** del mundo. Ni la malla
  sabe de dónde salen las alturas, ni la fuente sabe de la malla: cambiar el ruido por un
  archivo o una función analítica no toca `build_grid`, y viceversa.
- **`tex_coords`** normalizadas `(j/N, i/N)` (dejadas para una futura textura; hoy el color
  es un **uniform plano**, sin iluminación).

### La fuente de alturas (`terrain::NoiseHeights`)

`FastNoiseLite` (header-only, en `third_party/FastNoiseLite`) implementa `HeightField`:

- Ruido **Perlin** con **FBm de 5 octavas** (gain 0.5, lacunarity 2.0): una sola octava es
  demasiado lisa; sumando octavas el relieve tiene rugosidad a varias escalas.
  `h = amplitud · GetNoise(nx, nz)`: la amplitud es el tope NOMINAL (`|h| ≤ amplitud`), pero
  una ventana concreta no llega al extremo (con amplitud 800 m el rango medido fue
  ~[-600, 520] m).
- **Semilla fija** (`kTerrainSeed = 1337`): el terreno es **determinista** (misma semilla →
  mismas alturas), lo que lo hace reproducible y testeable.
- La **frecuencia** se expresa en "ciclos por nodo": `0.01` → una loma ocupa ~100 celdas
  (~3 km con paso 30 m).
- **Elevación base** (`base`, parámetro del constructor, default `0`): `y = base + amplitud ·
  GetNoise(nx, nz)`. Es un simple **desplazamiento vertical** del mundo — no cambia el relieve
  ni las normales (la base se cancela en las diferencias centradas) ni la continuidad entre
  ventanas. Se usa para **acercar el terreno al avión sin tocar el FDM**: el FDM vuela en el
  **trim exacto de 5000 m** (bajar el avión rompería el equilibrio), así que en vez de eso se
  sube el terreno con `kTerrainBase = 4200 m`; el avión queda a ~800 m del suelo medio y a
  ~300 m de las cumbres. Es **tuneable**: con `kTerrainBase` más alto el suelo queda aún más
  cerca; con uno más bajo (p. ej. 3000 m) reaparece algo de **cielo** en la parte alta del
  cuadro. Con 4200 m el terreno cubre casi todo el cuadro.

### Las normales (`terrain::grid_normal`)

Por **diferencias centradas** sobre la grilla de alturas ya muestreada:

```
n = normalize( vec3( h(i,j−1) − h(i,j+1),  2·paso,  h(i−1,j) − h(i+1,j) ) )
```

- El término `2·paso > 0` en `y` garantiza que la normal apunta **hacia arriba**.
- En el **borde** de la ventana el índice del vecino se **clampea** a `[0, N]`: la diferencia
  es a un solo lado y la normal queda **definida y nunca nula** (no hace falta "vecino
  fantasma").
- Todavía **no hay iluminación**: las normales no se ven en la escena (el color es plano).
  El complemento visual es el **modo de depuración de normales** de `cube.fs` (ver P03) y el
  numérico, los Tests 20 y 22.
- `grid_normal` se expone aparte porque la ventana deslizante recalcula los vértices de a
  poco y debe usar EXACTAMENTE la misma fórmula que `build_grid`.

### Culling: `glEnable(GL_CULL_FACE)` y el WINDING en el generador

`main.cpp` habilita `glEnable(GL_DEPTH_TEST)` (ya estaba) y `glEnable(GL_CULL_FACE)`. Con los
valores por defecto (`glCullFace(GL_BACK)` + `glFrontFace(GL_CCW)`) se descartan los
triángulos vistos por su cara de atrás: **desde abajo del terreno no se dibuja nada** (se ve
el color de fondo).

El orden de los índices se eligió para que la normal geométrica `(B−A)×(C−A)` apunte a **+Y
en todos los triángulos** (Test 19), así el culling dibuja la cara de **arriba**. Este es el
punto de la consigna: se corrigió el **winding en el generador de la malla**, NO se usó
`glFrontFace` para "parchear" el pipeline.

### Tamaño del mundo y comportamiento del borde (decisión)

- La malla es una ventana de `kTerrainCells = 1000` celdas de `kTerrainSpacing = 30 m` →
  **30×30 km** (radio 15 km), en **metros** (la unidad del P06):
  `(N+1)² = 1001² = 1.002.001` vértices y `6·N² = 6.000.000` índices (~32 MB de VBO).
  El relieve medido con la semilla 1337 es de unos **[-553, 485] m**; con la **elevación base**
  `kTerrainBase = 4200 m` (`y = base + relieve`) el mundo queda en **~[3650, 4685] m**, esto es,
  a ~300 m de las cumbres del avión que vuela en el trim de 5000 m (ver `NoiseHeights`).
- **El borde NO se ve: no hay regla de borde porque el terreno es infinito de verdad.** La
  ventana se desliza sobre el heightmap y el recorte de la proyección corta antes que el
  borde de la malla: con `far = 10000 m` y la órbita acotada a `kMaxDist = 2000 m`, lo más
  lejano visible (≤ ~9 km horizontales) cae cómodo dentro del radio de 15 km. Por eso la
  proyección se ajustó a `far = 10000` (ver `CameraSystem`).

### Terreno ilimitado: la ventana deslizante (`TerrainWindow`)

`src/core/TerrainWindow.h/.cpp` posee el `Mesh` (más los buffers de staging `MeshData` y una
grilla de alturas `H_`). No conoce cámara ni FDM: solo recibe el punto `(x,z)` sobre el que
centrarse.

- **Origen "encajado" a la celda** (`terrain::window_origin`):
  `ox = round(x/paso) − N/2`. Redondear a entero evita el **"repteo" (crawling)**: si el
  origen siguiera a la cámara de forma continua, los vértices caerían entre nodos y la
  superficie parecería arrastrarse/cambiar de forma. Moviéndose a **saltos de una celda**, la
  ventana siempre coincide con nodos reales y el terreno "pasa por abajo" idéntico a sí mismo.
- **Posiciones locales + `model = translate(ox·paso, 0, oz·paso)`**: al recentrar solo cambian
  las ALTURAS/NORMALES del VBO, nunca la topología (los índices se arman UNA vez, igual que
  en `build_grid`).
- **El problema (no obvio)**: recalcular la ventana entera de una vez cuesta ~78 ms (medido,
  para 800²) → varios cuadros congelados cada vez que el avión cruza una celda. Inaceptable.
- **La solución — reconstrucción progresiva**: la ventana solo se recentra cuando el objetivo
  se aleja más de `kMargenCeldas = 40` celdas (~1,2 km) del centro, y el recálculo se
  **reparte entre cuadros** con un presupuesto (`presupuesto_ = 12000` muestras/cuadro,
  ~1 ms):
  - fase `kHeights`: llena `H_` (lo caro: 1 `GetNoise` por nodo);
  - fase `kVertices`: arma posición/normal/tex del `MeshData` desde `H_` (barato);
  - fase `kIdle`: no hace nada.
  Mientras se reconstruye se sigue dibujando la ventana VIEJA; recién cuando la nueva está
  **completa** se sube con **UN** `glNamedBufferSubData` (`Mesh::update_vertices`, que
  reescribe el VBO sin recrear el VAO/EBO) y se cambia el origen → **sin costura a medio
  armar, sin titileo y sin "reptar"**. Un recentrado completo tarda
  ~`1.002.001 / 12000` ≈ 84 cuadros (~1,4 s a 60 fps); el avión tarda ~8 s en recorrer
  1,2 km, así que sobra tiempo. (Se descartaron: recalcular todo en el cruce, `memmove` 2D
  de la grilla y doble buffer; ver el header de `TerrainWindow`.)
- La ventana **sigue al objetivo de la cámara**: en chase, la posición del avión; con V, el
  origen del mundo.

### `main.cpp` y `CameraSystem`

- Init (con el contexto GL vivo): `terrain::NoiseHeights alturas(kTerrainSeed, kTerrainFreq,
  kTerrainAmplitud, 5, kTerrainBase)` + `TerrainWindow terreno; terreno.init(kTerrainCells,
  kTerrainSpacing, alturas, pos0.x, pos0.z)` — arma la topología UNA vez, genera la primera
  ventana de forma sincrónica, la sube e imprime el rango de alturas muestreado.
- Por cuadro: `terreno.update(objetivo.x, objetivo.z)` con `objetivo` = posición del avión (o
  el origen si `terreno_view`); el retorno indica si la malla cambió de origen (se loguea
  `[terreno] ventana recentrada a (ox, oz)`).
- El terreno entra como **una pieza más**: `items.push_back({terreno.mesh(), terreno.model(),
  kTerrainColor})` **después** de `avion.collect(items)` (porque `collect()` limpia el
  vector) y se dibuja con el mismo `glDrawElements`.
- Tecla **V** (evento, por flanco): alterna la "vista de terreno" (reapunta al **origen a la
  altura del terreno** — `(0, alturas(0,0), 0)`, no `y = 0`, porque el mundo está elevado) y
  salta a una pose de órbita útil con `CameraSystem::set_orbit(...)` (2000 m mirando hacia
  abajo, pitch 40°), volviendo a la de chase (60 m, 25°) al salir.

### Verificación (los cinco chequeos de "¿cómo sé que está bien?")

1. **La malla es N×N**: `(N+1)²` vértices y `6·N²` índices, con posiciones locales
   `j·paso`/`i·paso` y `tex_coords` normalizadas → **Test 19**. En la app, **L** (wireframe)
   muestra la grilla de triángulos del terreno.
2. **Las normales están bien**: en un plano inclinado la normal (diferencias centradas)
   coincide con la analítica, y en el borde sigue siendo **unitaria y hacia arriba** (nunca
   nula) → **Tests 20 y 22**. Visualmente, el **modo de depuración de normales** de `cube.fs`
   pinta el relieve con un degradé suave (dirección de la normal ↔ color).
3. **El culling descarta la cara de abajo**: `(B−A)×(C−A).y > 0` en TODOS los triángulos
   → **Test 19**; desde abajo del terreno no se ve la superficie (verificado con capturas:
   sin culling el 100 % del cuadro es terreno desde abajo, con culling queda un residuo de
   ~0,4 % de triángulos muy parados — se descarta el **99,6 %**).
4. **El terreno es continuo (ilimitado)**: un mismo nodo del mundo da la MISMA altura y la
   MISMA posición mundial en dos ventanas corridas una celda (no hay costura ni "escalón"
   entre ventanas) → **Test 22**.
5. **La ventana se desliza sin reptar ni tirones**: `window_origin` no se mueve dentro de una
   celda y salta 1 nodo al cruzarla → **Test 21**; en la app, el log de recentrado aparece
   cada ~1,2 km (~8 s) y el relieve es determinista y acotado (misma semilla → mismo terreno).

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
- Botón **derecho** (zoom): `dy → dist_delta` proporcional a la distancia actual
  (`kZoomGain = 0.03` = 3 %/px) — en el mundo METROS del P06 una ganancia absoluta haría
  el zoom imperceptible (para acercarse del todo habría que arrastrar miles de px).
  Arrastrar hacia arriba (dy < 0) acerca. El mouse
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
g++ -std=c++17 -Wall -Wextra -I./libs/dlfdm/include \
    -I./third_party/FastNoiseLite \
    libs/dlfdm/src/dlfdm/fdmsolver.cpp \
    libs/dlfdm/src/dlfdm/aerodynamicsmodel.cpp \
    libs/dlfdm/src/dlfdm/aircraftdynamics.cpp \
    libs/dlfdm/src/dlfdm/propulsionmodel.cpp \
    libs/dlfdm/src/dlfdm/models/aircraft/jettrainer.cpp \
    libs/dlfdm/src/dlfdm/models/propulsion/turbofan.cpp \
    libs/dlfdm/src/dlfdm/models/propulsion/turbojet.cpp \
    src/core/Primitives.cpp src/core/ResourceManager.cpp \
    src/core/CameraSystem.cpp src/core/InputHandler.cpp \
    src/core/FlightData.cpp src/core/Terrain.cpp src/core/NoiseHeights.cpp \
    src/main_test.cpp -o /tmp/opencode/test_rm \
    -lglfw -lpthread
/tmp/opencode/test_rm        # 22 tests
make
```

## Referencias

- [OpenGL Wiki](https://www.khronos.org/opengl/wiki/)
- [LearnOpenGL](https://learnopengl.com/)
- [GLFW Documentation](https://www.glfw.org/documentation.html)
- [GLM — OpenGL Mathematics](https://github.com/g-truc/glm)
- Filminas y Guías de aula A y B de la materia