cmake_minimum_required(VERSION 3.14)
project(knapsack-asm C ASM)

set(CMAKE_C_STANDARD 11)

add_executable(${PROJECT_NAME} knapsack.S main.c)

find_package(OpenMP REQUIRED)
target_compile_options(${PROJECT_NAME} PRIVATE -Wall ${OpenMP_C_FLAGS})
target_link_libraries(${PROJECT_NAME} ${OpenMP_C_LIBRARIES})

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/cmake-build-$<LOWER_CASE:$<CONFIG>>)
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different input ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        )