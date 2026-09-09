add_library(CloudModule STATIC
    PTSImporter.cpp
    CloudSerializer.cpp
)
target_include_directories(CloudModule PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/Utils
    ${CMAKE_SOURCE_DIR}/src/Core
    ${CMAKE_SOURCE_DIR}/thirdparty/glm
)
target_link_libraries(CloudModule PUBLIC CoreModule glm)