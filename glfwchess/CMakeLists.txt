cmake_minimum_required(VERSION 3.20)

project(glfwchess LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)

find_package(OpenGL REQUIRED)

FetchContent_Declare(
    glfw
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG 3.4
)
FetchContent_MakeAvailable(glfw)

FetchContent_Declare(
    stb
    GIT_REPOSITORY https://github.com/nothings/stb.git
    GIT_TAG master
)
FetchContent_MakeAvailable(stb)

add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/../magic_bitboard" "${CMAKE_BINARY_DIR}/magic_bitboard")

add_executable(glfwchess
    src/main.cpp
)

target_include_directories(glfwchess PRIVATE ${stb_SOURCE_DIR})

target_link_libraries(glfwchess PRIVATE
    chess_engine
    glfw
    OpenGL::GL
)

if(WIN32)
    target_link_libraries(glfwchess PRIVATE opengl32)
endif()

# Copy assets next to the executable at configure time
file(COPY
    "${CMAKE_CURRENT_LIST_DIR}/board.png"
    "${CMAKE_CURRENT_LIST_DIR}/pieces.png"
    DESTINATION "${CMAKE_BINARY_DIR}"
)

# Copy a system font for text rendering (try monospace candidates first)
if(WIN32)
    foreach(FONT "consola.ttf" "cour.ttf" "arial.ttf")
        if(EXISTS "C:/Windows/Fonts/${FONT}")
            file(COPY "C:/Windows/Fonts/${FONT}" DESTINATION "${CMAKE_BINARY_DIR}")
            set(GLFWCHESS_FONT "${FONT}")
            message(STATUS "Using font: ${FONT}")
            break()
        endif()
    endforeach()
endif()
