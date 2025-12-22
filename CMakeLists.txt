cmake_minimum_required(VERSION 3.16)
project(vektor VERSION 0.1.0 LANGUAGES C CXX)

LIST(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
LIST(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake/sanitizers")

find_package(Sanitizers)

add_subdirectory(vendor)
add_subdirectory(libvektor)
