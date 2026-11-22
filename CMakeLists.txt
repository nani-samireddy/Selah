cmake_minimum_required(VERSION 3.24)
project(ChurchDeck VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Qt6 6.9 REQUIRED COMPONENTS Quick QuickControls2 Multimedia Concurrent Sql Network GuiPrivate)
qt_standard_project_setup(REQUIRES 6.9)
include(CTest)
option(CHURCHDECK_ENABLE_DIAGNOSTICS "Build developer capture and verification commands" ON)

qt_add_library(church-deck-formatting STATIC)
qt_add_qml_module(church-deck-formatting
    URI ChurchDeck.Formatting
    VERSION 1.0
    CLASS_NAME ChurchDeckFormattingPlugin
    OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/ChurchDeck/Formatting"
    RESOURCE_PREFIX "/qt/qml"
    SOURCES src/document/TextFormatting.cpp src/document/TextFormatting.h src/document/TextLayoutMetrics.h
)
target_include_directories(church-deck-formatting PUBLIC src/document)
target_link_libraries(church-deck-formatting PUBLIC Qt6::Quick)

add_subdirectory(src)
qt_add_executable(church-deck src/app/main.cpp)
target_link_libraries(church-deck PRIVATE church-deck-core church-deck-formattingplugin)
if(APPLE)
    enable_language(OBJCXX)
    target_sources(church-deck PRIVATE src/app/WindowChrome.mm src/app/WindowChrome.h)
    target_link_libraries(church-deck PRIVATE "-framework AppKit")
    set_source_files_properties(assets/branding/church-deck.icns PROPERTIES MACOSX_PACKAGE_LOCATION Resources)
    target_sources(church-deck PRIVATE assets/branding/church-deck.icns)
    set_target_properties(church-deck PROPERTIES MACOSX_BUNDLE_ICON_FILE church-deck.icns)
elseif(WIN32)
    target_sources(church-deck PRIVATE assets/branding/church-deck.rc)
    target_include_directories(church-deck PRIVATE "${PROJECT_SOURCE_DIR}/assets/branding")
endif()
qt_add_resources(church-deck google-font-catalog PREFIX "/google-fonts" BASE "${PROJECT_SOURCE_DIR}/assets/google-fonts" FILES assets/google-fonts/catalog.json assets/google-fonts/OFL.txt)
qt_add_resources(church-deck remote-assets PREFIX "/remote" BASE "${PROJECT_SOURCE_DIR}/remote" FILES remote/index.html remote/style.css remote/app.js)

qt_add_resources(church-deck feedback-sounds PREFIX "/sounds" BASE "${PROJECT_SOURCE_DIR}/assets/sounds" FILES assets/sounds/click.wav assets/sounds/select.wav)

set_source_files_properties(qml/controls/LibraryStyle.qml qml/controls/InteractionFeedback.qml PROPERTIES QT_QML_SINGLETON_TYPE TRUE)

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
file(GLOB BRAND_ASSETS CONFIGURE_DEPENDS RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}" "assets/branding/*.svg" "assets/branding/*.png")
file(GLOB_RECURSE CONTENT_ASSETS CONFIGURE_DEPENDS RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}" "assets/fonts/*" "assets/scriptures/*")
qt_add_qml_module(church-deck
    URI ChurchDeck
    VERSION 1.0
    QML_FILES ${CHURCHDECK_QML}
    RESOURCES assets/ambient.svg ${BRAND_ASSETS} ${STUDIO_ICONS} ${CONTENT_ASSETS}
)

set_target_properties(church-deck PROPERTIES
    OUTPUT_NAME "Selah"
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE_BUNDLE_NAME "Selah"
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
