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
#include <QDebug>
int runProgramSectionChecks();
int runAudioLibraryChecks();
int runServiceCueChecks();
int runBackupChecks();
int runServiceToolsChecks();
int runDefaultImageChecks();
int runAiAssistantChecks();
int runUpdateChecks();
int runBookMatcherChecks();
int runQuickAddChecks();
int runTextLayoutChecks();
int runTransitionChecks();
int runDualScriptureChecks();
int runBibleXmlChecks(const QString &realFile = QString());

int runModelChecks() {
    if (auto result = runProgramSectionChecks()) {
        qCritical() << "Program sections failed:" << result;
        return 900 + result;
    }
    if (auto result = runAudioLibraryChecks()) {
        qCritical() << "Audio checks failed:" << result;
        return 850 + result;
    }
    if (auto result = runServiceCueChecks()) {
        qCritical() << "Custom cue checks failed:" << result;
        return 800 + result;
    }
    if (auto result = runBackupChecks()) {
        qCritical() << "Backup checks failed:" << result;
        return 750 + result;
    }
    if (auto result = runServiceToolsChecks()) {
        qCritical() << "Service tools failed:" << result;
        return 700 + result;
    }
    if (auto result = runBookMatcherChecks()) {
        qCritical() << "Multilingual book matching failed:" << result;
        return 600 + result;
    }
    if (auto result = runUpdateChecks()) {
        qCritical() << "Update checks failed:" << result;
        return 550 + result;
    }
    if (auto result = runTextLayoutChecks()) {
        qCritical() << "Text fitting and logical split check failed:" << result;
        return 490 + result;
    }
    if (auto result = runQuickAddChecks()) {
        qCritical() << "Quick Add check failed:" << result;
        return 450 + result;
    }
    if (auto result = runAiAssistantChecks()) {
        qCritical() << "AI foundation check failed:" << result;
        return 400 + result;
    }
    if (auto result = runTransitionChecks()) {
        qCritical() << "Transition check failed:" << result;
        return 360 + result;
    }
    if (auto result = runDualScriptureChecks()) {
        qCritical() << "Dual scripture check failed:" << result;
        return 330 + result;
    }
    if (auto result = runBibleXmlChecks()) {
        return 300 + result;
    }
    if (auto result = runDefaultImageChecks()) {
        qCritical() << "Standby media checks failed:" << result;
        return 280 + result;
    }
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
