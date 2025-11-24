# For more information about build system see
# https://docs.espressif.com/projects/esp-idf/en/latest/api-guides/build-system.html
# The following five lines of boilerplate have to be in your project's
# CMakeLists in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.5)

if (CMAKE_VERSION VERSION_GREATER_EQUAL "3.20.0" AND CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    set(CMAKE_C_USE_RESPONSE_FILE_FOR_INCLUDES 1)
    set(CMAKE_CXX_USE_RESPONSE_FILE_FOR_INCLUDES 1)
endif()

add_compile_options(-fdiagnostics-color=always)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
idf_build_set_property(DEPENDENCIES_LOCK dependencies.lock.${IDF_TARGET})

project(human_vital_signs)

