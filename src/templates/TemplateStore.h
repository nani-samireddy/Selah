#pragma once
#include <QObject>
#include <QSqlDatabase>
#include <QUrl>
#include <QVariantList>

// A catalog of reusable definitions. Programs retain their own copies when applying a template.
class TemplateStore final : public QObject {
    Q_OBJECT
    Q_PROPERTY(QVariantList layouts READ layouts NOTIFY changed)
    Q_PROPERTY(QString status READ status NOTIFY changed)
public:
    explicit TemplateStore(QObject *parent = nullptr, const QString &directory = QString());
    ~TemplateStore() override;
    QVariantList layouts() const;

    QString status() const { return m_status; }

    Q_INVOKABLE QVariantMap layout(const QString &id) const;
    Q_INVOKABLE QString saveTemplate(const QString &name, const QString &kind,
        const QVariantMap &slide, const QString &id = QString());
    Q_INVOKABLE QString createTemplate(const QString &name, const QString &kind);
    Q_INVOKABLE QVariantMap preview(const QString &id) const;
    Q_INVOKABLE bool importTemplate(const QUrl &url);
    Q_INVOKABLE bool exportTemplate(const QString &id, const QUrl &url);
    // Bundled layouts have stable identities. Startup must never overwrite a user's edits.
    bool installPreset(const QVariantMap &definition);
    // Retire the old stock pack without deleting saved edits or its durable media.
    bool retireLegacyPresets();
    static int selfTest();
signals:
    void changed();

private:
    QString persist(QVariantMap definition);

    void setStatus(const QString &message) {
        m_status = message;
        emit changed();
    }

    QSqlDatabase m_db;
    QString m_status = "Templates ready";
};
