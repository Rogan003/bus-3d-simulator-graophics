cmake_minimum_required(VERSION 3.20)
project(Projekat3D)

set(CMAKE_CXX_STANDARD 20)

set(CMAKE_PREFIX_PATH "/opt/homebrew")
set(CMAKE_INCLUDE_PATH "/opt/homebrew/include")
set(CMAKE_LIBRARY_PATH "/opt/homebrew/lib")

find_package(glfw3 REQUIRED)
find_package(GLEW REQUIRED)
find_package(OpenGL REQUIRED)
find_package(Freetype REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(assimp REQUIRED)

add_executable(Projekat3D
        Source/Main.cpp
        Source/Util.cpp
)

target_include_directories(Projekat3D PRIVATE
        ${FREETYPE_INCLUDE_DIRS}
)

target_link_directories(Projekat3D PRIVATE
        /opt/homebrew/lib
)

target_link_libraries(Projekat3D
        glfw
        GLEW::GLEW
        OpenGL::GL
        ${FREETYPE_LIBRARIES}
        glm::glm
        assimp::assimp
)