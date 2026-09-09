# Roadmap InspectionApp

## v0.1.0 - Infraestructura
- [x] CMake modular
- [x] Logger thread-safe
- [x] Ventana GLFW + OpenGL 4.6 Core
- [x] Shaders básicos (grid)
- [x] Estructura de carpetas profesional

## v0.2.0 - Cámara ortográfica
- [ ] Bloqueos de rotación y offset
- [ ] Rotación Z global desde vista TOP
- [ ] Navegación con ratón (pan/zoom)
- [ ] Configuración de velocidad

## v0.3.0 - Importador PTS
- [ ] Parser PTS (x y z i r g b)
- [ ] Barra de progreso en carga
- [ ] Metadatos básicos

## v0.4.0 - Render de nube
- [ ] VBO/VAO para nube de puntos
- [ ] Shader de puntos con tamaño adaptable
- [ ] Color por RGB o Intensidad

## v0.5.0 - Octree
- [ ] Construcción de Octree en memoria
- [ ] Serialización en formato CLOUD
- [ ] Apertura rápida vs PTS

## v0.6.0 - LOD
- [ ] Niveles de detalle por distancia
- [ ] Frame rate target

## v0.7.0 - Picking
- [ ] Selección de puntos en vista ortográfica
- [ ] Raycasting simplificado

## v0.8.0 - Rejilla
- [ ] Rejilla 3D fija (no rota con nube)
- [ ] Escala configurable en mm
- [ ] Colores adaptativos

## v0.9.0 - Offsets
- [ ] Offset X/Y/Z configurable
- [ ] Formato visual de coordenadas (escala)
- [ ] No modificar archivo original

## v1.0.0 - Inspección completa
- [ ] Anotaciones simples (TAG + XYZ)
- [ ] Section Box (activar/mover/redimensionar/guardar)
- [ ] Exportación de imágenes (snapshot)
- [ ] Formato PROJECT (serialización)
