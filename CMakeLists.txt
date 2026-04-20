cmake_minimum_required(VERSION 3.20)
project(mcrc LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

set(MCRC_WARNINGS -Wall -Wextra -Wpedantic -Werror)

add_library(mcrc_lib
    src/clockface.cpp
    src/mirror.cpp
    src/pipeline.cpp
    src/reverse_cipher.cpp
)
target_include_directories(mcrc_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_compile_features(mcrc_lib PUBLIC cxx_std_20)
target_compile_options(mcrc_lib PRIVATE ${MCRC_WARNINGS})

add_executable(mcrc apps/cli.cpp)
target_link_libraries(mcrc PRIVATE mcrc_lib)
target_compile_options(mcrc PRIVATE ${MCRC_WARNINGS})

add_executable(mcrc_demo apps/demo.cpp)
target_link_libraries(mcrc_demo PRIVATE mcrc_lib)
target_compile_options(mcrc_demo PRIVATE ${MCRC_WARNINGS})

add_executable(mcrc_verify tests/verify.cpp)
target_link_libraries(mcrc_verify PRIVATE mcrc_lib)
target_compile_options(mcrc_verify PRIVATE ${MCRC_WARNINGS})
# Ensure assert() fires even if the build type set NDEBUG.
target_compile_definitions(mcrc_verify PRIVATE $<$<BOOL:${NDEBUG}>:>)

add_custom_target(verify
    COMMAND mcrc_verify
    DEPENDS mcrc_verify
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Running MCRC verification suite"
)
