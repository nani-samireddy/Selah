# The shared runtime is independent of the application entry point and test cases.
qt_add_library(church-deck-core STATIC
    app/ApplicationSetup.cpp app/ApplicationSetup.h
    backup/AppBackup.cpp backup/AppBackup.h
    downloads/VerifiedDownload.cpp downloads/VerifiedDownload.h
    updates/UpdateManager.cpp updates/UpdateManager.h
    ai/AiRequest.cpp ai/AiRequest.h
    ai/ModelInstaller.cpp ai/ModelInstaller.h
    ai/AiAssistant.cpp ai/AiAssistant.h ai/AiTools.cpp ai/AiTools.h
    ai/CactusBackend.cpp ai/CactusBackend.h
    ai/SearchSpelling.cpp ai/SearchSpelling.h
    fonts/FontLibrary.cpp fonts/FontLibrary.h fonts/FontDownloads.cpp fonts/FontPreviews.cpp
    presentations/PresentationLibrary.cpp presentations/PresentationLibrary.h
    document/EditorDocument.cpp document/EditorDocument.h
    document/DocumentElements.cpp document/DocumentElements.h
    document/DocumentPersistence.cpp document/DocumentMedia.cpp document/DocumentBackground.cpp
    document/DocumentTransitions.cpp document/SlideTransition.h document/BackgroundFill.h
    songs/SongChords.cpp songs/SongChords.h
    content/BookMatcher.cpp content/BookMatcher.h
    content/ContentLibrary.cpp content/ContentLibrary.h content/ScripturePairing.cpp
    content/ScriptureImporter.cpp content/ScriptureImporter.h
    content/BibleXmlReader.cpp content/BibleXmlReader.h
    content/BibleRepository.cpp content/BibleRepository.h
    content/SlideFactory.cpp content/SlideFactory.h
    templates/TemplateStore.cpp templates/TemplateStore.h
    templates/TemplateDefinition.cpp templates/TemplateDefinition.h
    templates/TemplatePresets.cpp templates/TemplatePresets.h
    templates/TemplateTextLayout.cpp templates/TemplateTextLayout.h
    templates/TemplateComposer.cpp templates/TemplateComposer.h
    media/MediaLibrary.cpp media/MediaLibrary.h
    programs/ProgramStore.cpp programs/ProgramStore.h
    programs/ProgramSources.cpp programs/ProgramSources.h
    programs/ProgramTemplates.cpp
    programs/ProgramPersistence.cpp
    programs/PortableBundle.cpp programs/PortableBundle.h programs/ProgramBundles.cpp
    programs/ProgramSlides.cpp programs/ProgramSlides.h
    remote/RemoteController.cpp remote/RemoteController.h remote/RemoteApi.cpp
    remote/RemoteHttpServer.cpp remote/RemoteHttpServer.h
    remote/RemotePreview.cpp remote/RemotePreview.h
    ../third_party/qrcodegen/qrcodegen.cpp
    outputs/OutputManager.cpp outputs/OutputManager.h outputs/OutputTransitions.cpp
    outputs/DefaultImage.cpp outputs/DefaultImage.h
    outputs/LiveAudio.cpp outputs/LiveAudio.h
    outputs/OutputSession.cpp outputs/LiveSessionStore.cpp outputs/LiveSessionStore.h
    outputs/ServiceCues.cpp outputs/ServiceCues.h
    outputs/ServiceCueActions.cpp outputs/ServiceCueActions.h outputs/ServiceCueExecution.cpp
    outputs/QuickAddController.cpp outputs/QuickAddController.h
    broadcast/NdiController.cpp broadcast/NdiController.h
    broadcast/NdiWorker.cpp broadcast/NdiWorker.h
)
file(GLOB_RECURSE TEMPLATE_PRESET_ASSETS CONFIGURE_DEPENDS
    "${PROJECT_SOURCE_DIR}/assets/template-packs/selected/*.json"
    "${PROJECT_SOURCE_DIR}/assets/template-packs/selected/*.jpeg"
    "${PROJECT_SOURCE_DIR}/assets/template-packs/selected/*.jpg"
    "${PROJECT_SOURCE_DIR}/assets/template-packs/selected/*.png"
    "${PROJECT_SOURCE_DIR}/assets/template-packs/selected/*.svg"
)
qt_add_resources(church-deck-core template_presets PREFIX "/template-presets"
    BASE "${PROJECT_SOURCE_DIR}/assets/template-packs" FILES ${TEMPLATE_PRESET_ASSETS})
target_include_directories(church-deck-core PUBLIC
    presentations
    app document content media programs outputs broadcast templates remote fonts ai updates downloads songs backup
    "${PROJECT_SOURCE_DIR}/third_party/qrcodegen"
)
target_link_libraries(church-deck-core PUBLIC
    church-deck-formatting
    Qt6::Quick Qt6::QuickControls2 Qt6::Multimedia Qt6::Concurrent Qt6::Sql Qt6::Network
    PRIVATE Qt6::GuiPrivate
)

set(NDI_SDK_DIR "" CACHE PATH "Official NDI SDK root; runtime is loaded dynamically")
find_path(NDI_INCLUDE_DIR Processing.NDI.Lib.h
    HINTS "${NDI_SDK_DIR}" "$ENV{NDI_SDK_DIR}" "/Library/NDI SDK for Apple"
    PATH_SUFFIXES include Include
)
if(NDI_INCLUDE_DIR)
    target_include_directories(church-deck-core PRIVATE "${NDI_INCLUDE_DIR}")
    target_compile_definitions(church-deck-core PUBLIC CHURCHDECK_HAS_NDI_SDK=1)
else()
    target_compile_definitions(church-deck-core PUBLIC CHURCHDECK_HAS_NDI_SDK=0)
    message(STATUS "NDI SDK not found: capture diagnostics available; network sender disabled")
endif()

target_compile_definitions(church-deck-core PUBLIC SELAH_VERSION="${PROJECT_VERSION}")

qt_add_resources(church-deck-core book_names PREFIX "/book-names"
    BASE "${PROJECT_SOURCE_DIR}/assets/book-names"
    FILES "${PROJECT_SOURCE_DIR}/assets/book-names/catalog.json" "${PROJECT_SOURCE_DIR}/assets/book-names/indic-romanization.json")
