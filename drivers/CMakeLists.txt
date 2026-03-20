file(GLOB_RECURSE COMMON_DRIVER_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/common/*.cpp"
)
add_library(kes_driver_common STATIC ${COMMON_DRIVER_SOURCES})
target_include_directories(kes_driver_common PUBLIC
    ${PROJECT_SOURCE_DIR}/kestrel/include
    common
)

function(add_kestrel_driver DRIVER_NAME)
    set(TARGET_NAME "kes_${DRIVER_NAME}")

    file(GLOB_RECURSE DRIVER_ALL_SOURCES CONFIGURE_DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/${DRIVER_NAME}/*.cpp"
    )

    set(DRIVER_SOURCES ${DRIVER_ALL_SOURCES})
    set(TEST_SOURCES "")

    foreach(FILE ${DRIVER_ALL_SOURCES})
        if(FILE MATCHES ".*_test\\.cpp$")
            list(APPEND TEST_SOURCES ${FILE})
            list(REMOVE_ITEM DRIVER_SOURCES ${FILE})
        endif()
    endforeach()

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

    if(TEST_SOURCES)
        set(TEST_EXE_NAME "test_${TARGET_NAME}")

        add_executable(${TEST_EXE_NAME} ${TEST_SOURCES})

        # Link the driver itself so tests can access its internal logic
        # Note: If you need to test private symbols, you might need a OBJECT library approach
        target_link_libraries(${TEST_EXE_NAME} PRIVATE
            ${TARGET_NAME}
            Catch2::Catch2WithMain
        )

        add_test(${DRIVER_NAME} ${TEST_EXE_NAME})
    endif()

    add_sanitizers(${TARGET_NAME})
endfunction()

add_kestrel_driver(amdgpu libdrm::amdgpu)
add_kestrel_driver(i915)
