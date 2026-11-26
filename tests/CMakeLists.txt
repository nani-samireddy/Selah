qt_add_executable(selah-ai-test-runtime fixtures/CactusFixture.cpp)
target_link_libraries(selah-ai-test-runtime PRIVATE Qt6::Core)

# Unit bodies are shared by the standalone runner and the optional --self-test command.
add_library(church-deck-test-cases STATIC
    unit/ModelChecks.cpp unit/ModelChecks.h
    unit/BookMatcherTests.cpp
    unit/ServiceToolsTests.cpp unit/BackupTests.cpp unit/ServiceCueTests.cpp
    unit/UpdateTests.cpp
    unit/AiAssistantTests.cpp
    unit/QuickAddTests.cpp
    unit/TextLayoutTests.cpp
    unit/ContentLibraryTests.cpp unit/DualScriptureTests.cpp
    unit/BibleXmlTests.cpp
    unit/EditorDocumentTests.cpp
    unit/MediaLibraryTests.cpp unit/AudioLibraryTests.cpp
    unit/TransitionTests.cpp
    unit/OutputManagerTests.cpp
    unit/DefaultImageTests.cpp
    unit/NdiWorkerTests.cpp
    unit/SlideFactoryTests.cpp
    unit/BundleExchangeChecks.cpp
    unit/ProgramSectionsTests.cpp
    unit/ProgramStoreTests.cpp
    unit/TemplateStoreTests.cpp
    unit/TemplatePresetTests.cpp
    unit/RemoteControllerTests.cpp
    unit/FontLibraryTests.cpp
    unit/PresentationLibraryTests.cpp
    unit/TextFormattingTests.cpp
)
target_compile_definitions(church-deck-test-cases PRIVATE
    SELAH_AI_TEST_RUNTIME_PATH="$<TARGET_FILE:selah-ai-test-runtime>")
add_dependencies(church-deck-test-cases selah-ai-test-runtime)
target_include_directories(church-deck-test-cases PUBLIC unit)
target_link_libraries(church-deck-test-cases PUBLIC church-deck-core)

qt_add_resources(church-deck-test-cases standby-fixture PREFIX "/test-fixtures"
    BASE "${PROJECT_SOURCE_DIR}/tests/fixtures" FILES "${PROJECT_SOURCE_DIR}/tests/fixtures/standby.mp4")

if(BUILD_TESTING)
    qt_add_executable(church-deck-tests unit/main.cpp)
    target_link_libraries(church-deck-tests PRIVATE church-deck-test-cases)
    qt_add_resources(church-deck-tests test-fixtures
        PREFIX "/qt/qml/ChurchDeck"
        BASE "${PROJECT_SOURCE_DIR}"
        FILES
            "${PROJECT_SOURCE_DIR}/assets/scriptures/content-seed.sqlite"
            "${PROJECT_SOURCE_DIR}/assets/scriptures/bible-repository.json"
            "${PROJECT_SOURCE_DIR}/assets/fonts/Inter.ttf"
            "${PROJECT_SOURCE_DIR}/assets/fonts/NotoSansTelugu.ttf"
            "${PROJECT_SOURCE_DIR}/assets/fonts/NotoSansDevanagari.ttf"
            "${PROJECT_SOURCE_DIR}/assets/fonts/cardo/Cardo-Regular.ttf"
            "${PROJECT_SOURCE_DIR}/assets/fonts/cardo/Cardo-Bold.ttf"
            "${PROJECT_SOURCE_DIR}/assets/fonts/prompt/Prompt-Bold.ttf"
            "${PROJECT_SOURCE_DIR}/assets/fonts/public-sans/PublicSans[wght].ttf"
            "${PROJECT_SOURCE_DIR}/assets/fonts/freckle-face/FreckleFace-Regular.ttf"
    )
    qt_add_resources(church-deck-tests remote-test-assets PREFIX "/remote" BASE "${PROJECT_SOURCE_DIR}/remote" FILES "${PROJECT_SOURCE_DIR}/remote/index.html" "${PROJECT_SOURCE_DIR}/remote/style.css" "${PROJECT_SOURCE_DIR}/remote/app.js")
    qt_add_resources(church-deck-tests google-font-fixtures PREFIX "/google-fonts" BASE "${PROJECT_SOURCE_DIR}/assets/google-fonts" FILES "${PROJECT_SOURCE_DIR}/assets/google-fonts/catalog.json" "${PROJECT_SOURCE_DIR}/assets/google-fonts/OFL.txt")
    add_test(NAME model-checks COMMAND church-deck-tests)
endif()

if(CHURCHDECK_ENABLE_DIAGNOSTICS)
    add_library(church-deck-diagnostics STATIC
        smoke/DiagnosticRunner.cpp smoke/DiagnosticRunner.h
        smoke/StandbyChecks.cpp smoke/StandbyChecks.h
        smoke/UpdateSetupChecks.cpp smoke/UpdateSetupChecks.h
        smoke/AiChecks.cpp smoke/AiChecks.h
        smoke/QuickAddChecks.cpp smoke/QuickAddChecks.h
        smoke/TextFitChecks.cpp smoke/TextFitChecks.h
        smoke/ContentChecks.cpp smoke/RoutingChecks.cpp smoke/MediaChecks.cpp
        smoke/ProgramChecks.cpp smoke/ProgramChecks.h
        smoke/TemplateChecks.cpp smoke/TemplateChecks.h
        smoke/TemplatePresetChecks.cpp smoke/TemplatePresetChecks.h
        smoke/PresentChecks.cpp smoke/PresentChecks.h
        smoke/TransitionChecks.cpp smoke/TransitionChecks.h
        smoke/FontPreviewChecks.cpp smoke/FontPreviewChecks.h
        smoke/ResizeChecks.cpp smoke/ResizeChecks.h
        smoke/BackgroundChecks.cpp smoke/BackgroundChecks.h
        smoke/BibleImportChecks.cpp smoke/BibleImportChecks.h
        smoke/DualScriptureChecks.cpp smoke/DualScriptureChecks.h
        smoke/ProgramBrowserChecks.cpp smoke/ProgramBrowserChecks.h
        smoke/OutputPanelChecks.cpp smoke/OutputPanelChecks.h
        smoke/PrototypeChecks.cpp smoke/PrototypeChecks.h
        smoke/RemoteChecks.cpp smoke/RemoteChecks.h
        smoke/WorkspaceChecks.cpp smoke/WorkspaceChecks.h
    )
    if(APPLE)
        target_sources(church-deck-diagnostics PRIVATE smoke/NativeMenuChecks.mm smoke/NativeMenuChecks.h)
    endif()
    target_include_directories(church-deck-diagnostics PUBLIC smoke)
    target_link_libraries(church-deck-diagnostics PUBLIC church-deck-test-cases)
endif()
