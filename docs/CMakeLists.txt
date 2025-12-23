#find_program(SPHINX_EXECUTABLE sphinx-build REQUIRED)
#
#configure_file(
#    "${CMAKE_CURRENT_SOURCE_DIR}/conf.py.in"
#    "${CMAKE_CURRENT_BINARY_DIR}/conf.py"
#    @ONLY
#)
#
#add_custom_target(docs
#    COMMAND ${SPHINX_EXECUTABLE}
#            -W -n -b html
#            -c "${CMAKE_CURRENT_BINARY_DIR}"
#            "${CMAKE_CURRENT_SOURCE_DIR}"
#            "${CMAKE_CURRENT_BINARY_DIR}/html"
#    COMMENT "Building HTML documentation with Sphinx"
#    VERBATIM
#)

find_program(UV_EXECUTABLE uv REQUIRED)
find_package(Doxygen REQUIRED)

set(DOXYGEN_GENERATE_XML YES)
set(DOXYGEN_XML_OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/xml")

doxygen_add_docs(doxygen_xml
    "${PROJECT_SOURCE_DIR}/libvektor/include"
    COMMENT "Generating XML API data with Doxygen"
)

configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/conf.py.in"
    "${CMAKE_CURRENT_BINARY_DIR}/conf.py"
    @ONLY
)

add_custom_target(docs
    COMMAND ${UV_EXECUTABLE} run
            --with sphinx>=8.1.3,sphinx_rtd_theme,breathe
            --no-project
            sphinx-build
            -W -n -b html
            -c "${CMAKE_CURRENT_BINARY_DIR}"
            "${CMAKE_CURRENT_SOURCE_DIR}"
            "${CMAKE_CURRENT_BINARY_DIR}/html"
    WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
    COMMENT "Building HTML documentation with Sphinx"
    DEPENDS doxygen_xml
    VERBATIM
)

add_custom_target(docs-watch
    COMMAND ${UV_EXECUTABLE} run
            --with sphinx-autobuild>=8.1.3,sphinx_rtd_theme,breathe
            --no-project
            sphinx-autobuild
            -W -n -b html
            -c "${CMAKE_CURRENT_BINARY_DIR}"
            "${CMAKE_CURRENT_SOURCE_DIR}"
            "${CMAKE_CURRENT_BINARY_DIR}/html"
    WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
    COMMENT "Building HTML documentation with Sphinx"
    DEPENDS doxygen_xml
    VERBATIM
)
