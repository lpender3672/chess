cmake_minimum_required(VERSION 3.16)
project(magic_bitboard C)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)

# --- Options ---
option(USE_BMI2 "Use PEXT/PDEP intrinsics (requires BMI2 — Haswell+)" OFF)
option(USE_POPCNT "Use hardware POPCNT intrinsic" ON)

# --- Compiler flags ---
add_compile_options(-O3 -Wall -Wextra)

if(USE_BMI2)
    add_compile_options(-mbmi2)
    add_compile_definitions(USE_BMI2)
endif()

if(USE_POPCNT)
    add_compile_options(-mpopcnt)
    add_compile_definitions(USE_POPCNT)
endif()

# --- Core library ---
set(LIB_SOURCES
    src/bitboard.c
    src/magic.c
    src/position.c
    src/movegen.c
    src/pgn.c
)

add_library(chess_engine STATIC ${LIB_SOURCES})
target_include_directories(chess_engine PUBLIC include src)

# --- Validator executable ---
add_executable(validate src/validate.c)
target_link_libraries(validate chess_engine)

# --- Perft test executable ---
add_executable(perft tests/perft.c)
target_link_libraries(perft chess_engine)
