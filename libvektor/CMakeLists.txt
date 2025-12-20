
file(GLOB_RECURSE VEKTOR_SOURCES CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp
)


add_library(vektor SHARED
    ${VEKTOR_SOURCES}
)

target_compile_features(vektor PRIVATE cxx_std_20)
set_target_properties(vektor PROPERTIES POSITION_INDEPENDENT_CODE ON)

target_include_directories(vektor PUBLIC
    include
)
target_include_directories(vektor PRIVATE
    src
)

target_link_libraries(vektor PRIVATE
    libdrm::libdrm
    libdrm::amdgpu
    fmt::fmt
)

add_executable(hello_compute test/01_hello_compute/hello_compute.cpp)
target_link_libraries(hello_compute PRIVATE vektor)
