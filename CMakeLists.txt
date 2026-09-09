cmake_minimum_required(VERSION 3.20)
project(InspectionApp VERSION 0.4.2 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# === DEPENDENCIAS DEL SISTEMA ===
find_package(OpenGL REQUIRED)

# === THIRD PARTY ===
add_subdirectory(thirdparty/glad)
add_subdirectory(thirdparty/glfw)
add_subdirectory(thirdparty/glm)

# ── ImGui ──
set(IMGUI_DIR ${CMAKE_SOURCE_DIR}/thirdparty/imgui)
add_library(ImGui STATIC
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp
    ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp
)
target_include_directories(ImGui PUBLIC 
    ${IMGUI_DIR} 
    ${IMGUI_DIR}/backends
)
target_link_libraries(ImGui PUBLIC glfw glad)

# === MÓDULOS PROPIOS ===
add_subdirectory(src)

# === EJECUTABLE ===
add_executable(InspectionApp src/main.cpp)

target_include_directories(InspectionApp PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/App
    ${CMAKE_SOURCE_DIR}/src/Core
    ${CMAKE_SOURCE_DIR}/src/Render
    ${CMAKE_SOURCE_DIR}/src/Cloud
    ${CMAKE_SOURCE_DIR}/src/Utils
)

target_link_libraries(InspectionApp PRIVATE
    AppModule
    CoreModule
    RenderModule
    CloudModule
    UtilsModule
    ImGui
)

# === SHADERS TRACKEADOS COMO DEPENDENCIAS ===
set(SHADER_FILES
    ${CMAKE_SOURCE_DIR}/resources/shaders/grid.vert
    ${CMAKE_SOURCE_DIR}/resources/shaders/grid.frag
    ${CMAKE_SOURCE_DIR}/resources/shaders/point.vert
    ${CMAKE_SOURCE_DIR}/resources/shaders/point.frag
)
target_sources(InspectionApp PRIVATE ${SHADER_FILES})
set_source_files_properties(${SHADER_FILES} PROPERTIES HEADER_FILE_ONLY TRUE)

# === COPIAR RESOURCES ===
add_custom_target(CopyResources ALL
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/resources
        $<TARGET_FILE_DIR:InspectionApp>/resources
    COMMENT "Copying resources to output directory"
)
add_dependencies(InspectionApp CopyResources)