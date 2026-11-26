#pragma once
#include "CactusBackend.h"
#include <QObject>
#include <QUrl>
#include <QVariantList>

class ContentLibrary;

// Optional read-only assistant. No editor, program, output or remote capability is injected.
class AiAssistant final : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool enabled READ enabled NOTIFY changed)
    Q_PROPERTY(bool configured READ configured NOTIFY changed)
    Q_PROPERTY(bool busy READ busy NOTIFY changed)
    Q_PROPERTY(QString runtimePath READ runtimePath NOTIFY changed)
    Q_PROPERTY(QString modelPath READ modelPath NOTIFY changed)
    Q_PROPERTY(QString status READ status NOTIFY changed)
    Q_PROPERTY(QVariantList proposals READ proposals NOTIFY changed)
    Q_PROPERTY(QVariantList results READ results NOTIFY changed)
public:
    explicit AiAssistant(
        const ContentLibrary *content, QObject *parent = nullptr, bool preferences = true);

    bool enabled() const { return m_enabled; }

    bool configured() const;

    bool busy() const { return m_backend.busy(); }

    QString runtimePath() const { return m_runtime; }

    QString modelPath() const { return m_model; }

    QString status() const { return m_status; }

    QVariantList proposals() const { return m_proposals; }

    QVariantList results() const { return m_results; }

    Q_INVOKABLE bool configure(const QUrl &runtime, const QUrl &model);
    Q_INVOKABLE void setEnabled(bool enabled);
    Q_INVOKABLE bool useManagedModel();
    Q_INVOKABLE void propose(const QString &prompt, const QString &translationId = QString(),
        const QString &scope = "all");
    Q_INVOKABLE bool runSearches();
    Q_INVOKABLE void cancel();
signals:
    void changed();

private:
    void persist();
    const ContentLibrary *m_content;
    CactusBackend m_backend;
    QString m_runtime, m_model, m_translation, m_scope;
    QString m_status = "Luma is off.";
    QVariantList m_proposals, m_results;
    bool m_enabled = false;
    bool m_preferences = true;
    bool m_discard = false;
};
