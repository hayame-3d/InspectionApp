# Arquitectura InspectionApp

## Principios

- **MVC desde el inicio**: Separación clara de Modelo (Cloud/Project), Vista (Render/UI) y Controlador (App).
- **Una responsabilidad por clase**: Ninguna clase hace dos cosas.
- **No tocar el original**: El archivo PTS/CLOUD original nunca se modifica.
- **Performance primero**: Cada decisión prioriza la velocidad de inspección.

## Módulos

```
App/
  Application.cpp         -> Loop principal, orquestación

Core/
  Window.cpp              -> GLFW, contexto OpenGL 4.6 Core
  Logger.cpp              -> Logging thread-safe a archivo y consola

Render/
  Renderer.cpp            -> OpenGL moderno: VBO, VAO, GLSL
  Shader.cpp              -> Compilación y binding de shaders
  Camera.cpp              -> Cámara ortográfica + bloqueos + rotación Z

Cloud/                    -> (v0.3.0+) Importador PTS, formato CLOUD, Octree
UI/                       -> (v0.9.0+) Interfaz minimalista (Dear ImGui probable)
Inspection/               -> (v1.0.0+) Anotaciones, mediciones, Section Box
Project/                  -> (v1.0.0+) Serialización PROJECT sin tocar CLOUD
Utils/
  Types.h                 -> Alias de tipos (u32, f32, Vec3, etc.)
```

## Flujo de datos

```
PTS Import -> Conversión -> CLOUD (Octree + índices) -> Streaming -> Render
                                    ^
                                    |
                              PROJECT (offsets, anotaciones, section boxes)
```

## Convenciones

- C++20 obligatorio.
- Sin raw pointers; usar std::unique_ptr / std::shared_ptr.
- Sin OpenGL Legacy: solo VBO/VAO/GLSL.
- Namespace raíz: InspectionApp.
