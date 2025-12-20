include(ExternalProject)


set(CMAKE_POSITION_INDEPENDENT_CODE ON)
add_subdirectory(fmt)

set(LIBDRM_SOURCE_DIR ${CMAKE_SOURCE_DIR}/vendor/libdrm-2.4.131)
set(LIBDRM_INSTALL_DIR ${CMAKE_BINARY_DIR}/vendor/install)

file(MAKE_DIRECTORY ${LIBDRM_INSTALL_DIR}/include)
file(MAKE_DIRECTORY ${LIBDRM_INSTALL_DIR}/include/libdrm)

find_program(MESON_EXECUTABLE NAMES meson)
if(NOT MESON_EXECUTABLE)
    message(FATAL_ERROR "Meson build system not found. Please install it (e.g., 'sudo apt install meson' or 'pip install meson').")
endif()

find_program(NINJA_EXECUTABLE NAMES ninja ninja-build)
if(NOT NINJA_EXECUTABLE)
    message(FATAL_ERROR "Ninja build tool not found. Please install it (e.g., 'sudo apt install ninja-build').")
endif()

# libdrm uses Meson. In a production CMake environment, we use
# ExternalProject to build it properly without messy manual header copies.
ExternalProject_Add(libdrm_project
    SOURCE_DIR ${LIBDRM_SOURCE_DIR}
    PREFIX ${CMAKE_BINARY_DIR}/vendor/libdrm
    CONFIGURE_COMMAND ${MESON_EXECUTABLE} setup <SOURCE_DIR> <BINARY_DIR>
                    --prefix=${LIBDRM_INSTALL_DIR}
                    --libdir=lib
                    -Dintel=disabled -Dnouveau=disabled -Dradeon=disabled -Damdgpu=enabled
    BUILD_COMMAND ${NINJA_EXECUTABLE} -C <BINARY_DIR>
    INSTALL_COMMAND ${NINJA_EXECUTABLE} -C <BINARY_DIR> install
    BUILD_BYPRODUCTS
        ${LIBDRM_INSTALL_DIR}/lib/libdrm.so
        ${LIBDRM_INSTALL_DIR}/lib/libdrm_amdgpu.so
)

add_library(libdrm::libdrm SHARED IMPORTED GLOBAL)
add_library(libdrm::amdgpu SHARED IMPORTED GLOBAL)

set_target_properties(libdrm::libdrm PROPERTIES
    IMPORTED_LOCATION "${LIBDRM_INSTALL_DIR}/lib/libdrm.so"
    INTERFACE_INCLUDE_DIRECTORIES "${LIBDRM_INSTALL_DIR}/include;${LIBDRM_INSTALL_DIR}/include/libdrm"
)

set_target_properties(libdrm::amdgpu PROPERTIES
    IMPORTED_LOCATION "${LIBDRM_INSTALL_DIR}/lib/libdrm_amdgpu.so"
    INTERFACE_INCLUDE_DIRECTORIES "${LIBDRM_INSTALL_DIR}/include;${LIBDRM_INSTALL_DIR}/include/libdrm"
)
add_dependencies(libdrm::libdrm libdrm_project)
add_dependencies(libdrm::amdgpu libdrm_project)
