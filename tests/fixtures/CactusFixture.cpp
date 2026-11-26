#include <QCoreApplication>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTextStream>
#include <QTimer>

// Protocol fixture only: no model or generated content is used in automated tests.
int main(int argc, char **argv) {
    QCoreApplication app(argc, argv);
    const auto args = app.arguments();
    auto option = [&](const QString &name) { return args.value(args.indexOf(name) + 1); };
    QFile tools(option("--tools"));
    if (!tools.open(QIODevice::ReadOnly)) {
        return 9;
    }
    const auto count = QJsonDocument::fromJson(tools.readAll()).array().size();
    if (count < 1 || count > 2) {
        return 9;
    }
    const auto prompt = option("--prompt");
    if (prompt == "stall") {
        QTimer::singleShot(60000, &app, &QCoreApplication::quit);
        return app.exec();
    }
    QTextStream output(stdout);
    if (prompt == "oversize") {
        output << QString(300000, 'x');
        output.flush();
        return 0;
    }
    if (prompt == "malformed") {
        output << "not JSON";
        return 0;
    }
    QJsonObject arguments { { "query", prompt } };
    QString name = "search_songs";
    if (prompt == "scripture") {
        name = "search_scripture";
        arguments = { { "query", "Genesis 1:1" } };
    }
    if (prompt == "blocked") {
        name = "stop_presentation";
    }
    output << QJsonDocument(
        QJsonObject { { "success", true }, { "confidence", 0.95 },
            { "function_calls",
                QJsonArray { QJsonObject { { "name", name }, { "arguments", arguments } } } } })
                  .toJson(QJsonDocument::Compact);
    output.flush();
    return 0;
}
