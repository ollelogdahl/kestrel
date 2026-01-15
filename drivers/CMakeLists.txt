file(GLOB_RECURSE COMMON_DRIVER_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/common/*.cpp"
)
add_library(kes_driver_common STATIC ${COMMON_DRIVER_SOURCES})
target_include_directories(kes_driver_common PUBLIC
    common
)

function(add_kestrel_driver DRIVER_NAME)
    set(TARGET_NAME "kes_${DRIVER_NAME}")

    file(GLOB_RECURSE DRIVER_SOURCES CONFIGURE_DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/${DRIVER_NAME}/*.cpp"
    )

    add_library(${TARGET_NAME} SHARED ${DRIVER_SOURCES})

    target_compile_features(${TARGET_NAME} PRIVATE cxx_std_20)
    set_target_properties(${TARGET_NAME} PROPERTIES
        POSITION_INDEPENDENT_CODE ON
        CXX_VISIBILITY_PRESET hidden
        VISIBILITY_INLINES_HIDDEN ON
    )

    target_include_directories(${TARGET_NAME} PRIVATE
        ${PROJECT_SOURCE_DIR}/kestrel/include
        ${DRIVER_NAME}
    )

    target_compile_options(${TARGET_NAME} PRIVATE
        $<$<CXX_COMPILER_ID:GNU,Clang>:
            -fno-exceptions
            -fno-rtti
            -ffunction-sections
            -fdata-sections
        >
    )

    target_link_options(${TARGET_NAME} PRIVATE
        $<$<CXX_COMPILER_ID:GNU,Clang>:
            -static-libstdc++
            -static-libgcc
            -Wl,--gc-sections
            -Wl,--exclude-libs,ALL
            -Wl,--no-undefined
        >
    )

    target_link_libraries(${TARGET_NAME} PRIVATE
        kes_driver_common
        fmt::fmt
        libdrm::libdrm
        ${ARGN}
    )

    add_sanitizers(${TARGET_NAME})
endfunction()

add_kestrel_driver(amdgpu libdrm::amdgpu)
add_kestrel_driver(i915)
