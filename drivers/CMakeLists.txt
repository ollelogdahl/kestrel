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

    set(DRIVER_LIBS
        kes_driver_common
        fmt::fmt
        libdrm::libdrm
        ${ARGN}
    )

    file(GLOB_RECURSE DRIVER_ALL_SOURCES CONFIGURE_DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/${DRIVER_NAME}/*.cpp"
    )

    set(DRIVER_SOURCES ${DRIVER_ALL_SOURCES})
    set(TEST_SOURCES "")
    set(FUZZ_SOURCES "")

    foreach(FILE ${DRIVER_ALL_SOURCES})
        if(FILE MATCHES ".*_test\\.cpp$")
            list(APPEND TEST_SOURCES ${FILE})
            list(REMOVE_ITEM DRIVER_SOURCES ${FILE})
        endif()
        if(FILE MATCHES ".*_fuzz\\.cpp$")
          list(APPEND FUZZ_SOURCES ${FILE})
          list(REMOVE_ITEM DRIVER_SOURCES ${FILE})
        endif()
    endforeach()

    add_library(${TARGET_NAME}_obj OBJECT ${DRIVER_SOURCES})
    target_compile_features(${TARGET_NAME}_obj PRIVATE cxx_std_20)

    set_target_properties(${TARGET_NAME}_obj PROPERTIES
        POSITION_INDEPENDENT_CODE ON
        CXX_VISIBILITY_PRESET hidden
        VISIBILITY_INLINES_HIDDEN ON
    )

    target_include_directories(${TARGET_NAME}_obj PRIVATE
        ${PROJECT_SOURCE_DIR}/kestrel/include
        ${DRIVER_NAME}
    )

    target_compile_options(${TARGET_NAME}_obj PRIVATE
        $<$<CXX_COMPILER_ID:GNU,Clang>:
            -fno-exceptions
            -fno-rtti
            -ffunction-sections
            -fdata-sections
        >
    )

    target_link_libraries(${TARGET_NAME}_obj PRIVATE ${DRIVER_LIBS})

    target_link_options(${TARGET_NAME}_obj PRIVATE
        $<$<CXX_COMPILER_ID:GNU,Clang>:
            -static-libstdc++
            -static-libgcc
            -Wl,--gc-sections
            -Wl,--exclude-libs,ALL
            -Wl,--no-undefined
        >
    )

    add_library(${TARGET_NAME} SHARED
        $<TARGET_OBJECTS:${TARGET_NAME}_obj>
    )

    if(TEST_SOURCES)
        set(TEST_EXE_NAME "test_${TARGET_NAME}")

        add_executable(${TEST_EXE_NAME}
            $<TARGET_OBJECTS:${TARGET_NAME}_obj>
            ${TEST_SOURCES}
        )

        set_target_properties(${TEST_EXE_NAME} PROPERTIES
            POSITION_INDEPENDENT_CODE ON
            CXX_VISIBILITY_PRESET hidden
            VISIBILITY_INLINES_HIDDEN ON
        )

        target_include_directories(${TEST_EXE_NAME} PRIVATE
            ${PROJECT_SOURCE_DIR}/kestrel/include
            ${DRIVER_NAME}
        )

        target_compile_options(${TEST_EXE_NAME} PRIVATE
            $<$<CXX_COMPILER_ID:GNU,Clang>:
                -fno-exceptions
                -fno-rtti
                -ffunction-sections
                -fdata-sections
            >
        )

        target_link_options(${TEST_EXE_NAME} PRIVATE
            $<$<CXX_COMPILER_ID:GNU,Clang>:
                -static-libstdc++
                -static-libgcc
                -Wl,--gc-sections
                -Wl,--exclude-libs,ALL
                -Wl,--no-undefined
            >
        )

        target_link_libraries(${TEST_EXE_NAME} PRIVATE
            ${DRIVER_LIBS}
            Catch2::Catch2WithMain
        )

        add_test(${DRIVER_NAME} ${TEST_EXE_NAME})
    endif()

    if(FUZZ_SOURCES)
        foreach(FUZZ_FILE ${FUZZ_SOURCES})
            # Extract file name without extension to create a clean individual target name
            get_filename_component(FUZZ_BASE_NAME ${FUZZ_FILE} NAME_WE)
            set(FUZZ_EXE_NAME "fuzz_${TARGET_NAME}_${FUZZ_BASE_NAME}")

            add_executable(${FUZZ_EXE_NAME}
                $<TARGET_OBJECTS:${TARGET_NAME}_obj>
                ${FUZZ_FILE}
            )

            set_target_properties(${FUZZ_EXE_NAME} PROPERTIES
                POSITION_INDEPENDENT_CODE ON
                CXX_VISIBILITY_PRESET hidden
                VISIBILITY_INLINES_HIDDEN ON
            )

            target_include_directories(${FUZZ_EXE_NAME} PRIVATE
                ${PROJECT_SOURCE_DIR}/kestrel/include
                ${DRIVER_NAME}
            )

            target_compile_options(${FUZZ_EXE_NAME} PRIVATE
                $<$<CXX_COMPILER_ID:Clang>:
                    -fsanitize=fuzzer,address,undefined
                >
            )

            target_link_options(${FUZZ_EXE_NAME} PRIVATE
                $<$<CXX_COMPILER_ID:Clang>:
                    -fsanitize=fuzzer,address,undefined
                >
            )

            target_link_libraries(${FUZZ_EXE_NAME} PRIVATE
                ${DRIVER_LIBS}
            )
        endforeach()
    endif()

    add_sanitizers(${TARGET_NAME})
endfunction()

add_kestrel_driver(amdgpu libdrm::amdgpu)
add_kestrel_driver(i915)
