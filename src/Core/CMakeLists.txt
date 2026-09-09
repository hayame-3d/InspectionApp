add_library(CoreModule STATIC
    Window.cpp
    Logger.cpp
)
target_include_directories(CoreModule PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/src/Utils
    ${CMAKE_SOURCE_DIR}/thirdparty/glad/include
    ${CMAKE_SOURCE_DIR}/thirdparty/glfw/include
)
target_link_libraries(CoreModule PUBLIC glad glfw)