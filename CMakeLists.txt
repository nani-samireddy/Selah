cmake_minimum_required(VERSION 3.24)
project(ChurchDeck VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Qt6 6.8 REQUIRED COMPONENTS Quick QuickControls2 Multimedia Concurrent Sql Network GuiPrivate)
qt_standard_project_setup(REQUIRES 6.8)
include(CTest)
option(CHURCHDECK_ENABLE_DIAGNOSTICS "Build developer capture and verification commands" ON)

add_subdirectory(src)
qt_add_executable(church-deck src/app/main.cpp)
target_link_libraries(church-deck PRIVATE church-deck-core)
qt_add_resources(church-deck remote-assets PREFIX "/remote" BASE "${PROJECT_SOURCE_DIR}/remote" FILES remote/index.html remote/style.css remote/app.js)

# Keep resource URLs stable while the source tree is organized by feature.
file(GLOB_RECURSE CHURCHDECK_QML CONFIGURE_DEPENDS RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}" "qml/*.qml")
set(CHURCHDECK_QML_NAMES)
foreach(qml_file IN LISTS CHURCHDECK_QML)
    get_filename_component(qml_name "${qml_file}" NAME)
    if(qml_name IN_LIST CHURCHDECK_QML_NAMES)
        message(FATAL_ERROR "QML type names must be unique: ${qml_name}")
    endif()
    list(APPEND CHURCHDECK_QML_NAMES "${qml_name}")
    set_source_files_properties("${qml_file}" PROPERTIES QT_RESOURCE_ALIAS "qml/${qml_name}")
endforeach()
file(GLOB STUDIO_ICONS CONFIGURE_DEPENDS RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}" "assets/icons/*.svg")
file(GLOB CONTENT_ASSETS CONFIGURE_DEPENDS RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}" "assets/fonts/*" "assets/scriptures/*")
qt_add_qml_module(church-deck
    URI ChurchDeck
    VERSION 1.0
    QML_FILES ${CHURCHDECK_QML}
    RESOURCES assets/ambient.svg ${STUDIO_ICONS} ${CONTENT_ASSETS}
)

set_target_properties(church-deck PROPERTIES
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE_BUNDLE_NAME "Church Deck"
    MACOSX_BUNDLE_GUI_IDENTIFIER "app.churchdeck.editor"
)

if(BUILD_TESTING OR CHURCHDECK_ENABLE_DIAGNOSTICS)
    add_subdirectory(tests)
endif()
target_compile_definitions(church-deck PRIVATE
    CHURCHDECK_ENABLE_DIAGNOSTICS=$<BOOL:${CHURCHDECK_ENABLE_DIAGNOSTICS}>
)
if(CHURCHDECK_ENABLE_DIAGNOSTICS)
    target_link_libraries(church-deck PRIVATE church-deck-diagnostics)
endif()
install(TARGETS church-deck BUNDLE DESTINATION . RUNTIME DESTINATION bin)

find_package(Python3 QUIET COMPONENTS Interpreter)
if(Python3_Interpreter_FOUND)
    add_custom_target(format-check
        COMMAND "${Python3_EXECUTABLE}" "${PROJECT_SOURCE_DIR}/scripts/format.py" --check
        WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
        COMMENT "Checking C++ and QML formatting"
    )
endif()
