add_library(RenderModule STATIC
    Renderer.cpp
    Camera.cpp
    Shader.cpp
)
target_include_directories(RenderModule PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/src/Utils
    ${CMAKE_SOURCE_DIR}/src/Core
    ${CMAKE_SOURCE_DIR}/thirdparty/glad/include
    ${CMAKE_SOURCE_DIR}/thirdparty/glm
)
target_link_libraries(RenderModule PUBLIC CoreModule glad glm)