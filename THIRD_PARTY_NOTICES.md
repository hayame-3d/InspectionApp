# Dependencias de código abierto

InspectionApp 0.4.2 utiliza los siguientes proyectos de código abierto. Cada
dependencia conserva sus avisos y condiciones originales.

| Proyecto | Uso | Licencia / aviso |
|---|---|---|
| [GLFW](https://github.com/glfw/glfw) | Ventanas, contexto OpenGL y entrada | Licencia zlib/libpng; texto en `thirdparty/glfw/LICENSE.md` |
| [GLAD](https://github.com/Dav1dde/glad) | Cargador de OpenGL 4.6 generado con GLAD 2.0.8 | `(WTFPL OR CC0-1.0) AND Apache-2.0`, según el encabezado generado |
| [GLM](https://github.com/g-truc/glm) | Matemáticas de vectores y matrices | Texto en `thirdparty/glm/copying.txt` |
| [Dear ImGui](https://github.com/ocornut/imgui) | Interfaz gráfica inmediata | Licencia MIT; texto en `thirdparty/imgui/LICENSE.txt` |

Los directorios GLFW, GLM y Dear ImGui se registran como submódulos, fijados a
las revisiones empleadas por esta maqueta. El código generado de GLAD se incluye
directamente en el repositorio y mantiene su identificador SPDX original.
