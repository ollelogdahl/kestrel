find_package(Git QUIET)

option(GIT_SUBMODULE "Check submodules during build" ON)

if(GIT_SUBMODULE AND NOT GIT_SUBMODULE_CHECKED)
    if(GIT_FOUND)
        message(STATUS "Git found: initializing/updating submodules...")
        execute_process(
            COMMAND ${GIT_EXECUTABLE} submodule update --init --recursive
            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
            RESULT_VARIABLE _git_submodule_result
            OUTPUT_VARIABLE git_submodule_output
            ERROR_VARIABLE git_submodule_error
        )
        if(_git_submodule_result EQUAL 0)
            message(STATUS "Submodules initialized successfully")
            set(GIT_SUBMODULE_CHECKED TRUE CACHE INTERNAL "Git submodules have been initialized")
        else()
            message(WARNING "git submodule update --init --recursive failed with exit code ${_git_submodule_result}")
            message(WARNING "Output: ${git_submodule_output}")
            message(WARNING "Error: ${git_submodule_error}")
        endif()
    else()
        message(STATUS "Git not found; skipping automatic git submodule initialization")
        message(WARNING "Please run 'git submodule update --init --recursive' manually.")
    endif()
endif()
