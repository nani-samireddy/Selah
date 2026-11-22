#include "NativeMenuChecks.h"
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QGuiApplication>
#include <QJSValue>
#include <QJsonArray>
#include <QJsonDocument>
#include <QQmlApplicationEngine>
#include <QTimer>
#include <QWindow>
#import <AppKit/AppKit.h>

void runNativeMenuChecks(QQmlApplicationEngine &engine, const QString &folder) {
    QDir().mkpath(folder);
    [NSApp activateIgnoringOtherApps:YES];
    if (auto window = qobject_cast<QWindow *>(engine.rootObjects().value(0))) {
        window->raise();
        window->requestActivate();
    }
    QTimer::singleShot(1200, &engine, [&, folder] {
        QStringList titles;
        for (NSMenuItem *item in NSApp.mainMenu.itemArray)
            titles << QString::fromUtf8(item.title.UTF8String);
        QFile receipt(folder + "/native-menus.json");
        if (receipt.open(QIODevice::WriteOnly))
            receipt.write(QJsonDocument(QJsonArray::fromStringList(titles)).toJson());
        auto root = engine.rootObjects().value(0);
        auto bar = root->findChild<QObject *>("applicationMenuBar");
        if (!bar) {
            qApp->exit(250);
            return;
        }
        auto menuBar = engine.newQObject(bar);
        int index = 0;
        for (const auto &expected : { "File", "Edit", "View", "Presentation", "Help" }) {
            const auto menu = menuBar.property("menuAt").callWithInstance(menuBar, { index++ });
            if (menu.property("title").toString().remove('&') != QString::fromUtf8(expected)
                || ([NSApp isActive] && !titles.contains(QString::fromUtf8(expected)))) {
                qCritical() << "Menu missing:" << expected << titles;
                qApp->exit(250);
                return;
            }
        }
        if (![NSApp isActive])
            qInfo() << "AppKit system-bar verification deferred: window is not foreground; "
                       "QML menu definitions and actions are checked below.";
        auto libraries = root->findChild<QObject *>("menuLibraries");
        auto help = root->findChild<QObject *>("menuUserGuide");
        if (!libraries || !help || !root->findChild<QObject *>("menuImportProgram")
            || !root->findChild<QObject *>("menuExportProgram")) {
            qApp->exit(251);
            return;
        }
        auto action = engine.newQObject(libraries);
        action.property("trigger").callWithInstance(action);
        QTimer::singleShot(300, &engine, [root, help, &engine] {
            if (root->property("activeWorkspace") != "libraries") {
                qApp->exit(252);
                return;
            }
            auto action = engine.newQObject(help);
            action.property("trigger").callWithInstance(action);
            QTimer::singleShot(300, &engine, [root] {
                auto guide = root->findChild<QObject *>("userGuideDialog");
                if (!guide || !guide->property("opened").toBool()) {
                    qApp->exit(253);
                    return;
                }
                qInfo() << "Menu workflow checks passed: menu definitions, workspace action and user guide";
                qApp->exit(0);
            });
        });
    });
}
