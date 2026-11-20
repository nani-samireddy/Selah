#include "ModelChecks.h"
#include "ContentLibrary.h"
#include "EditorDocument.h"
#include "MediaLibrary.h"
#include "NdiWorker.h"
#include "OutputManager.h"
#include "ProgramStore.h"
#include "RemoteController.h"
#include "SlideFactory.h"
#include "TemplateStore.h"

int runModelChecks() {
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
