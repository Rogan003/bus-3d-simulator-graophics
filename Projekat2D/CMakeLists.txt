cmake_minimum_required(VERSION 3.20)
project(Projekat2D)

set(CMAKE_CXX_STANDARD 20)

set(CMAKE_PREFIX_PATH "/opt/homebrew")
set(CMAKE_INCLUDE_PATH "/opt/homebrew/include")
set(CMAKE_LIBRARY_PATH "/opt/homebrew/lib")

find_package(glfw3 REQUIRED)
find_package(GLEW REQUIRED)
find_package(OpenGL REQUIRED)
find_package(Freetype REQUIRED)

add_executable(Projekat2D
        Source/Main.cpp
        Source/Util.cpp
)

target_include_directories(Projekat2D PRIVATE
        /opt/homebrew/include
        ${FREETYPE_INCLUDE_DIRS}
)

target_link_directories(Projekat2D PRIVATE
        /opt/homebrew/lib
)

target_link_libraries(Projekat2D
        glfw
        GLEW::GLEW
        OpenGL::GL
        ${FREETYPE_LIBRARIES}
)