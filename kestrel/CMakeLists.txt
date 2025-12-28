file(GLOB_RECURSE KESTREL_RT_SOURCES CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/rt/*.cpp
)

add_library(kestrel STATIC
    ${KESTREL_RT_SOURCES}
)

target_compile_features(kestrel PRIVATE cxx_std_20)

set_target_properties(kestrel PROPERTIES
    CXX_VISIBILITY_PRESET hidden
    VISIBILITY_INLINES_HIDDEN ON
)

target_compile_options(kestrel PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:
        -fno-exceptions
        -fno-rtti
        -ffunction-sections
        -fdata-sections
    >
)

target_link_options(kestrel PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:
        -static-libstdc++
        -static-libgcc
        -Wl,--gc-sections
        -Wl,--exclude-libs,ALL
    >
)

target_include_directories(kestrel PUBLIC include)
target_include_directories(kestrel PRIVATE rt)

add_sanitizers(kestrel)
