# InspectionApp

> **Maqueta base histórica — versión 0.4.2 (Kimi 5.0).** Este código se
> conserva como el prototipo anterior a la línea actual de InspectionApp y no
> incluye las herramientas de reportes incorporadas en versiones posteriores.

Aplicación profesional de inspección de nubes de puntos inspirada en el flujo de trabajo de Pointools.
Desarrollada en C++20 con OpenGL 4.6 Core Profile.

Esta maqueta fue posible gracias a proyectos de código abierto, especialmente
[GLFW](https://www.glfw.org/), [GLAD](https://gen.glad.sh/),
[GLM](https://github.com/g-truc/glm) y
[Dear ImGui](https://github.com/ocornut/imgui). Sus licencias y versiones se
detallan en [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md).

## Requisitos

- Visual Studio 2022 Community (o cualquier IDE con soporte C++20)
- CMake >= 3.20
- Git

## Dependencias

- **GLFW3**: Manejo de ventana y entrada.
- **GLAD**: Loader de OpenGL 4.6 Core.
- **GLM**: Matemáticas de vectores y matrices.

### Instalación de dependencias

Clona los repositorios dentro de `thirdparty/`:

```bash
cd thirdparty
git clone https://github.com/glfw/glfw.git
git clone https://github.com/g-truc/glm.git
```

Para GLAD, genera el loader desde https://gen.glad.sh/ con:
- API: gl Version 4.6
- Profile: Core

Extrae el ZIP en `thirdparty/glad/` y crea `thirdparty/glad/CMakeLists.txt`:

```cmake
cmake_minimum_required(VERSION 3.20)
project(glad LANGUAGES C)
add_library(glad STATIC src/glad.c)
target_include_directories(glad PUBLIC include)
```

## Compilación

```bash
cd build
cmake ..
cmake --build . --config Release
```

O con Visual Studio:
```bash
cmake .. -G "Visual Studio 17 2022" -A x64
```
Luego abre `InspectionApp.sln`.

## Ejecución

```bash
.uild\src\Release\InspectionApp.exe
```

Los recursos (shaders) se copian automáticamente al directorio de salida.

## Controles

| Tecla | Acción |
|-------|--------|
| W/A/S/D | Pan (arriba/izq/abajo/der) |
| Q/E | Pan en Z |
| F1-F6 | Vistas ortogonales (Top/Bottom/Front/Back/Left/Right) |
| +/- | Zoom in/out |
| R | Rotar vista TOP en Z |
| ESC | Cerrar aplicación |

## Roadmap

- v0.1.0: Infraestructura, OpenGL, Logger, CMake
- v0.2.0: Cámara ortográfica con bloqueos y rotación
- v0.3.0: Importador PTS
- v0.4.0: Render de nube de puntos (VBO/VAO)
- v0.5.0: Octree
- v0.6.0: LOD
- v0.7.0: Picking
- v0.8.0: Rejilla configurable
- v0.9.0: Offsets visuales
- v1.0.0: Anotaciones, Section Box, Exportación

## Licencia

El código propio de esta maqueta se publica bajo la [licencia MIT](LICENSE).
Las dependencias incluidas o referenciadas conservan sus licencias originales,
detalladas en [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md).
