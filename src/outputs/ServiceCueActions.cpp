#include "ServiceCueActions.h"
#include "SlideTransition.h"
#include <cmath>
#include <QDataStream>
#include <QIODevice>
#include <QJsonDocument>
#include <QRegularExpression>
#include <QUrl>

QVariantList ServiceCueActions::catalog() {
    QVariantList result;
    const QList<QPair<QString, QString>> types { { "stage", "Stage view" },
        { "audience", "Audience content" }, { "audio", "Presentation audio" },
        { "media", "Media playback" }, { "timer", "Countdown" }, { "message", "Stage message" },
        { "navigate", "Go to slide" }, { "item", "Go to service item" },
        { "resume", "Resume program" }, { "transition", "Slide transition" },
        { "overlay", "Show image overlay" }, { "clearOverlay", "Clear overlay" },
        { "clip", "Play audio clip" }, { "stopClip", "Stop audio clip" },
        { "output", "Start / stop output" }, { "delay", "Wait" }, { "http", "HTTP request" },
        { "osc", "OSC command" } };
    for (const auto &[type, label] : types) {
        QVariantList fields;
        auto choice = [&](const QString &key, const QString &name, const QStringList &values) {
            fields.append(QVariantMap {
                { "key", key }, { "label", name }, { "kind", "choice" }, { "options", values } });
        };
        auto field = [&](const QString &key, const QString &name, const QString &kind, int low = 0,
                         int high = 1000) {
            fields.append(QVariantMap { { "key", key }, { "label", name }, { "kind", kind },
                { "min", low }, { "max", high } });
        };
        if (type == "stage") {
            choice("value", "View", { "speaker", "musician" });
        }
        if (type == "audience") {
            choice("value", "Content", { "lyrics", "blackout", "default", "clear", "restore" });
        }
        if (type == "audio") {
            choice("value", "Audio", { "on", "off" });
        }
        if (type == "media") {
            choice("value", "Playback", { "play", "pause" });
        }
        if (type == "timer") {
            choice("value", "Timer", { "start", "pause" });
            field("minutes", "Minutes", "number", 1, 180);
        }
        if (type == "message") {
            field("text", "Message", "text");
        }
        if (type == "navigate") {
            field("slide", "Slide number", "number", 1, 5000);
        }
        if (type == "item") {
            field("itemId", "Loaded service item", "item");
        }
        if (type == "transition") {
            choice(
                "effect", "Effect", QStringList { "authored", "cut" } + SlideTransition::effects());
            field("durationMs", "Duration (ms)", "number", 50, 5000);
        }
        if (type == "overlay") {
            field("source", "Image", "image");
        }
        if (type == "clip") {
            field("source", "Audio file", "file");
        }
        if (type == "output") {
            choice("role", "Output", { "audience", "stage" });
            choice("value", "Action", { "start", "preview", "stop" });
        }
        if (type == "delay") {
            field("durationMs", "Wait (ms)", "number", 1, 60000);
        }
        if (type == "http") {
            field("url", "URL", "text", 0, 4096);
            choice("method", "Method", { "GET", "POST" });
            field("body", "JSON body", "text", 0, 10000);
        }
        if (type == "osc") {
            field("host", "Destination IP", "text");
            field("port", "Port", "number", 1, 65535);
            field("address", "OSC address", "text");
            field("arguments", "Arguments (JSON array)", "text", 0, 4096);
        }
        result.append(QVariantMap { { "type", type }, { "label", label }, { "fields", fields } });
    }
    return result;
}

QVariantMap ServiceCueActions::defaults(const QString &type) {
    QVariantMap action { { "type", type } };
    if (type == "stage") {
        action["value"] = "speaker";
    } else if (type == "audience") {
        action["value"] = "lyrics";
    } else if (type == "audio") {
        action["value"] = "off";
    } else if (type == "media" || type == "timer") {
        action["value"] = "pause";
    }
    if (type == "timer") {
        action["minutes"] = 20;
    }
    if (type == "message") {
        action["text"] = "";
    }
    if (type == "navigate") {
        action["slide"] = 1;
    }
    if (type == "item") {
        action["itemId"] = "";
    }
    if (type == "transition") {
        action["effect"] = "fade";
        action["durationMs"] = 500;
    }
    if (type == "overlay" || type == "clip") {
        action["source"] = "";
    }
    if (type == "output") {
        action["role"] = "audience";
        action["value"] = "stop";
    }
    if (type == "delay") {
        action["durationMs"] = 1000;
    }
    if (type == "http") {
        action["url"] = "";
        action["method"] = "POST";
        action["body"] = "{}";
    }
    if (type == "osc") {
        action["host"] = "127.0.0.1";
        action["port"] = 8000;
        action["address"] = "/cue/go";
        action["arguments"] = "[]";
    }
    return action;
}

QVariantMap ServiceCueActions::normalize(const QVariantMap &cue) {
    if (cue.contains("actions")) {
        return cue;
    }
    QVariantList actions;
    for (const auto &type : { "stage", "audience", "audio", "media", "timer" }) {
        const auto value = cue.value(type, "keep").toString();
        if (value != "keep") {
            auto action = defaults(type);
            action["value"] = value;
            if (QString(type) == "timer") {
                action["minutes"] = cue.value("minutes", 20);
            }
            actions.append(action);
        }
    }
    actions.append(QVariantMap { { "type", "message" }, { "text", cue.value("message", "") } });
    return { { "version", 2 }, { "name", cue["name"] }, { "actions", actions } };
}

namespace {
bool integer(const QVariant &value, int low, int high) {
    bool ok = false;
    const double number = value.toDouble(&ok);
    return ok && std::isfinite(number) && number == std::floor(number) && number >= low
        && number <= high;
}

QByteArray oscString(const QString &text) {
    auto bytes = text.toUtf8();
    bytes.append('\0');
    while (bytes.size() % 4) {
        bytes.append('\0');
    }
    return bytes;
}
}

QByteArray ServiceCueActions::oscPacket(const QVariantMap &action) {
    const auto doc = QJsonDocument::fromJson(action["arguments"].toString().toUtf8());
    if (!doc.isArray()) {
        return {};
    }
    const auto arguments = doc.toVariant().toList();
    if (arguments.size() > 32) {
        return {};
    }
    QString tags = ",";
    QByteArray payload;
    QDataStream stream(&payload, QIODevice::WriteOnly);
    stream.setByteOrder(QDataStream::BigEndian);
    stream.setFloatingPointPrecision(QDataStream::SinglePrecision);
    for (const auto &value : arguments) {
        if (value.metaType().id() == QMetaType::Bool) {
            tags += value.toBool() ? 'T' : 'F';
        } else if (value.metaType().id() == QMetaType::QString) {
            const auto text = value.toString();
            if (text.size() > 256 || text.contains(QChar(0))) {
                return {};
            }
            tags += 's';
            const auto bytes = oscString(text);
            stream.writeRawData(bytes.constData(), bytes.size());
        } else if (value.metaType().id() == QMetaType::Double
            || value.metaType().id() == QMetaType::LongLong) {
            const double number = value.toDouble();
            if (!std::isfinite(number) || std::abs(number) > 3.4e38) {
                return {};
            }
            if (number == std::floor(number) && number >= -2147483648. && number <= 2147483647.) {
                tags += 'i';
                stream << qint32(number);
            } else {
                tags += 'f';
                stream << float(number);
            }
        } else {
            return {};
        }
    }
    return oscString(action["address"].toString()) + oscString(tags) + payload;
}

bool ServiceCueActions::valid(const QVariantMap &input, QString *error) {
    auto fail = [&](const QString &message) {
        if (error) {
            *error = message;
        }
        return false;
    };
    const auto cue = normalize(input);
    const auto actions = cue["actions"].toList();
    if (cue["name"].toString().trimmed().isEmpty() || cue["name"].toString().size() > 60
        || cue["actions"].metaType().id() != QMetaType::QVariantList || actions.isEmpty()
        || actions.size() > 32) {
        return fail("Give the cue a name and 1–32 actions");
    }
    int totalDelay = 0;
    for (int index = 0; index < actions.size(); ++index) {
        const auto action = actions[index].toMap();
        const auto type = action["type"].toString();
        bool ok = false;
        if (type == "stage") {
            ok = QStringList { "speaker", "musician" }.contains(action["value"].toString());
        } else if (type == "audience") {
            ok = QStringList { "lyrics", "blackout", "default", "clear", "restore" }.contains(
                action["value"].toString());
        } else if (type == "audio") {
            ok = QStringList { "on", "off" }.contains(action["value"].toString());
        } else if (type == "media") {
            ok = QStringList { "play", "pause" }.contains(action["value"].toString());
        } else if (type == "timer") {
            ok = QStringList { "start", "pause" }.contains(action["value"].toString())
                && integer(action["minutes"], 1, 180);
        } else if (type == "message") {
            ok = action["text"].toString().size() <= 1000;
        } else if (type == "navigate") {
            ok = integer(action["slide"], 1, 5000);
        } else if (type == "item") {
            ok = !action["itemId"].toString().isEmpty()
                && action["itemId"].toString().size() <= 200;
        } else if (type == "resume" || type == "clearOverlay" || type == "stopClip") {
            ok = true;
        } else if (type == "transition") {
            ok = action["effect"] == "authored"
                || SlideTransition::valid(QVariantMap {
                    { "type", action["effect"] }, { "durationMs", action["durationMs"] } });
        } else if (type == "overlay" || type == "clip") {
            ok = QUrl(action["source"].toString()).isLocalFile()
                && action["source"].toString().size() <= 4096;
        } else if (type == "output") {
            ok = QStringList { "audience", "stage" }.contains(action["role"].toString())
                && QStringList { "start", "preview", "stop" }.contains(action["value"].toString());
        } else if (type == "delay") {
            ok = integer(action["durationMs"], 1, 60000);
            totalDelay += action["durationMs"].toInt();
        } else if (type == "http") {
            const QUrl url(action["url"].toString());
            ok = url.isValid() && QStringList { "http", "https" }.contains(url.scheme())
                && !url.host().isEmpty() && url.userInfo().isEmpty()
                && url.toString().size() <= 4096
                && QStringList { "GET", "POST" }.contains(action["method"].toString())
                && action["body"].toString().toUtf8().size() <= 10000;
        } else if (type == "osc") {
            static const QRegularExpression address("^/[A-Za-z0-9_/.-]{1,127}$");
            ok = !action["host"].toString().isEmpty() && integer(action["port"], 1, 65535)
                && address.match(action["address"].toString()).hasMatch()
                && action["arguments"].toString().size() <= 4096 && !oscPacket(action).isEmpty();
        }
        if (!ok) {
            return fail(QString("Check the settings for action %1 (%2)").arg(index + 1).arg(type));
        }
    }
    return totalDelay <= 120000 || fail("Total waits must be no more than two minutes");
}
