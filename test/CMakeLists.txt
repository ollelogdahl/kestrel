include(FetchContent)

FetchContent_Declare(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG v3.5.1
)
FetchContent_MakeAvailable(Catch2)

add_library(kestrel_test_common STATIC
    common/test_utils.cpp
)
target_link_libraries(kestrel_test_common PUBLIC kestrel Catch2::Catch2)
target_include_directories(kestrel_test_common PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/common)

file(GLOB UNIT_TEST_SOURCES CONFIGURE_DEPENDS "unit/*.cpp")

if(UNIT_TEST_SOURCES)
    add_executable(kestrel_unit_tests ${UNIT_TEST_SOURCES})
    target_link_libraries(kestrel_unit_tests PRIVATE kestrel_test_common Catch2::Catch2WithMain)
    add_sanitizers(kestrel_unit_tests)

    # Register with CTest
    add_test(NAME unit_tests COMMAND kestrel_unit_tests)

    # These tests need GPU access
    set_tests_properties(unit_tests PROPERTIES
        LABELS "gpu"
        TIMEOUT 300
    )
endif()

include(CTest)
include(Catch)

if(TARGET kestrel_unit_tests)
    catch_discover_tests(kestrel_unit_tests)
endif()

file(GLOB EXAMPLE_DIRS CONFIGURE_DEPENDS RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}/examples ${CMAKE_CURRENT_SOURCE_DIR}/examples/*)
foreach(example_dir ${EXAMPLE_DIRS})
    # Ensure we are looking at a directory, not a random file
    if(IS_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/examples/${example_dir})

        # 2. Define the executable name based on the folder name
        set(example_name "example_${example_dir}")

        # 3. Find all .cpp files within that specific folder
        file(GLOB_RECURSE EXAMPLE_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/examples/${example_dir}/*.cpp")

        if(EXAMPLE_SOURCES)
            add_executable(${example_name} ${EXAMPLE_SOURCES})

            target_link_libraries(${example_name} PRIVATE kestrel)

            add_sanitizers(${example_name})

            message(STATUS "Added example: ${example_name}")
        endif()
    endif()
endforeach()
