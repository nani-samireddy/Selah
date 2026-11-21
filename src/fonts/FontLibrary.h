#pragma once
#include <QHash>
#include <QNetworkAccessManager>
#include <QObject>
#include <QPointer>
#include <QVariantList>
class QNetworkReply;

// Full Google Fonts metadata with explicit, persistent, on-demand native font downloads.
class FontLibrary final : public QObject {
    Q_OBJECT
    Q_PROPERTY(int count READ count NOTIFY changed)
    Q_PROPERTY(bool busy READ busy NOTIFY stateChanged)
    Q_PROPERTY(QString status READ status NOTIFY stateChanged)
    Q_PROPERTY(QString activeFamily READ activeFamily NOTIFY stateChanged)
    Q_PROPERTY(int progress READ progress NOTIFY stateChanged)
public:
    explicit FontLibrary(QObject *parent = nullptr, const QString &directory = {});
    ~FontLibrary() override;

    int count() const { return m_records.size(); }

    bool busy() const { return m_busy; }

    QString status() const { return m_status; }

    QString activeFamily() const { return m_family; }

    int progress() const { return m_progress; }

    Q_INVOKABLE QVariantList search(
        const QString &query, const QString &scope = "google", const QString &script = "all") const;
    Q_INVOKABLE bool available(const QString &family) const;
    Q_INVOKABLE QString resolvedFamily(const QString &family) const;
    Q_INVOKABLE QString licenseText(const QString &family) const;
    Q_INVOKABLE void install(const QString &family);
    Q_INVOKABLE void cancel();
    static int selfTest();
    static int networkSelfTest(const QString &reportDirectory);
signals:
    void changed();
    void stateChanged();
    void familyReady(const QString &requested, const QString &resolved);
    void installFailed(const QString &message);

private:
    struct File {
        QUrl url;
        QString name, sha;
        bool font = false;
    };

    void loadCache();
    QString directoryFor(const QString &family) const;
    void next();
    void useGoogleCss();
    void finish();
    void fail(const QString &message);
    void saveIndex();
    static QString fontNotice(const QString &path);
    static QList<QUrl> cssFontUrls(const QByteArray &css);
    QVariantList m_records;
    QHash<QString, QVariantMap> m_byName, m_cache;
    QHash<QString, QString> m_resolved;
    QList<int> m_fontIds;
    QNetworkAccessManager m_network;
    QPointer<QNetworkReply> m_reply;
    QList<File> m_queue;
    QStringList m_downloadedFonts, m_downloadedLicenses;
    QString m_directory, m_commit, m_family, m_status = "Choose a font to preview and use";
    bool m_busy = false, m_css = false;
    int m_progress = 0, m_total = 0;
    quint64 m_generation = 0;
};
