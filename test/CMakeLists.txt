
file(GLOB TEST_DIRS CONFIGURE_DEPENDS RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}/test ${CMAKE_CURRENT_SOURCE_DIR}/test/*)

foreach(test_dir ${TEST_DIRS})
    # Ensure we are looking at a directory, not a random file
    if(IS_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/test/${test_dir})

        # 2. Define the executable name based on the folder name
        set(test_name "test_${test_dir}")

        # 3. Find all .cpp files within that specific test folder
        file(GLOB_RECURSE TEST_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/test/${test_dir}/*.cpp")

        if(TEST_SOURCES)
            add_executable(${test_name} ${TEST_SOURCES})

            # 4. Link your library (libvektor)
            target_link_libraries(${test_name} PRIVATE kestrel)

            add_sanitizers(${test_name})

            message(STATUS "Added test: ${test_name}")
        endif()
    endif()
endforeach()
