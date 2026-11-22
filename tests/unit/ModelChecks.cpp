#include "ModelChecks.h"
#include "ContentLibrary.h"
#include "EditorDocument.h"
#include "FontLibrary.h"
#include "MediaLibrary.h"
#include "NdiWorker.h"
#include "OutputManager.h"
#include "PresentationLibrary.h"
#include "ProgramStore.h"
#include "RemoteController.h"
#include "SlideFactory.h"
#include "TemplatePresets.h"
#include "TemplateStore.h"
#include "TextFormatting.h"

int runModelChecks() {
    if (auto result = TemplatePresets::selfTest()) {
        return 260 + result;
    }
    if (auto result = PresentationLibrary::selfTest()) {
        return 240 + result;
    }
    if (auto result = TextFormatting::selfTest()) {
        return 220 + result;
    }
    if (auto result = FontLibrary::selfTest()) {
        return 200 + result;
    }
    if (auto result = RemoteController::selfTest()) {
        return 160 + result;
    }
    if (auto result = TemplateStore::selfTest()) {
        return 130 + result;
    }
    if (auto result = ProgramStore::selfTest()) {
        return 100 + result;
    }
    if (auto result = ContentLibrary::selfTest()) {
        return 60 + result;
    }
    if (auto result = SlideFactory::selfTest()) {
        return 70 + result;
    }
    if (auto result = EditorDocument::selfTest()) {
        return result;
    }
    if (auto result = MediaLibrary::selfTest()) {
        return 10 + result;
    }
    if (auto result = NdiWorker::selfTest()) {
        return 30 + result;
    }
    return OutputManager::selfTest();
}
