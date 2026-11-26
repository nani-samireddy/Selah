#include "AiTools.h"
#include <cmath>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QStringList>

namespace {
QJsonObject tool(const QString &name, const QString &description, const QJsonObject &properties,
    const QJsonArray &required) {
    return { { "type", "function" },
        { "function",
            QJsonObject { { "name", name }, { "description", description },
                { "parameters",
                    QJsonObject { { "type", "object" }, { "properties", properties },
                        { "required", required }, { "additionalProperties", false } } } } } };
}

QJsonObject text(const QString &description) {
    return { { "type", "string" }, { "description", description }, { "minLength", 1 },
        { "maxLength", 256 } };
}
}

QJsonArray AiTools::schemas(const QString &scope) {
    const QJsonArray tools {
        tool("search_songs",
            "Find saved songs by title, lyrics or tags. Does not add or present songs.",
            { { "query", text("Words to search for in the local song library") } }, { "query" }),
        tool("search_scripture",
            "Find a Bible passage by reference or words in the selected Bible.",
            { { "query", text("Bible reference such as John 3:16, or words to find") } },
            { "query" })
    };
    if (scope == "songs") {
        return { tools.first() };
    }
    if (scope == "scripture") {
        return { tools.last() };
    }
    return scope == "all" ? tools : QJsonArray();
}

AiTools::Proposal AiTools::decode(const QByteArray &response) {
    auto reject = [](const QString &message) { return Proposal { {}, message }; };
    if (response.isEmpty() || response.size() > 256 * 1024) {
        return reject("The assistant returned an empty or oversized response.");
    }
    QJsonParseError error;
    const auto document = QJsonDocument::fromJson(response, &error);
    if (error.error != QJsonParseError::NoError || !document.isObject()) {
        return reject("The assistant did not return a valid command proposal.");
    }
    const auto root = document.object();
    if (!root.value("success").isBool() || !root.value("success").toBool()
        || (!root.value("error").isNull() && !root.value("error").isUndefined())) {
        return reject("The assistant could not interpret this request.");
    }
    const auto confidence = root.value("confidence");
    if (!confidence.isDouble() || !std::isfinite(confidence.toDouble())
        || confidence.toDouble() < 0.1 || confidence.toDouble() > 1) {
        return reject("Please rephrase the request; the assistant is not confident enough.");
    }
    if (!root.value("function_calls").isArray()) {
        return reject("The assistant returned no search proposal.");
    }
    const auto calls = root.value("function_calls").toArray();
    if (calls.size() > 4) {
        return reject("Ask for no more than four searches at a time.");
    }
    QVariantList actions;
    for (const auto &value : calls) {
        if (!value.isObject()) {
            return reject("Invalid search proposal.");
        }
        const auto call = value.toObject();
        const auto name = call.value("name").toString();
        if (name != "search_songs" && name != "search_scripture") {
            return reject("This assistant can only search songs and scripture.");
        }
        if (!call.value("arguments").isObject()) {
            return reject("Invalid search arguments.");
        }
        const auto arguments = call.value("arguments").toObject();
        const QStringList expected { "query" };
        if (arguments.size() != expected.size()) {
            return reject("Unexpected search arguments.");
        }
        for (const auto &key : expected) {
            const auto argument = arguments.value(key);
            if (!argument.isString() || argument.toString().trimmed().isEmpty()
                || argument.toString().size() > 256 || argument.toString().contains(QChar::Null)) {
                return reject("A search argument is missing or invalid.");
            }
        }
        actions.append(QVariantMap { { "name", name }, { "arguments", arguments.toVariantMap() },
            { "confidence", confidence.toDouble() } });
    }
    return { actions, {} };
}
