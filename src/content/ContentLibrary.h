#pragma once
#include <QObject>
#include <QSqlDatabase>
#include <QUrl>
#include <QVariantList>

// Local song catalog and indexed Bible queries. Imports use separate worker connections.
class ContentLibrary final : public QObject {
    Q_OBJECT
    Q_PROPERTY(QVariantList songs READ songs NOTIFY changed)
    Q_PROPERTY(QStringList songTags READ songTags NOTIFY changed)
    Q_PROPERTY(QVariantList translations READ translations NOTIFY changed)
    Q_PROPERTY(QString status READ status NOTIFY changed)
    Q_PROPERTY(bool busy READ busy NOTIFY changed)
    Q_PROPERTY(QVariantList repositoryBibles READ repositoryBibles CONSTANT)
    Q_PROPERTY(QString bibleImportPhase READ bibleImportPhase NOTIFY bibleImportProgressChanged)
    Q_PROPERTY(
        qint64 bibleDownloadedBytes READ bibleDownloadedBytes NOTIFY bibleImportProgressChanged)
    Q_PROPERTY(qint64 bibleDownloadTotal READ bibleDownloadTotal NOTIFY bibleImportProgressChanged)
public:
    explicit ContentLibrary(QObject *parent = nullptr, const QString &directory = QString());
    ~ContentLibrary() override;
    QVariantList songs() const;
    QVariantList translations() const;
    QVariantList repositoryBibles() const;

    QString bibleImportPhase() const { return m_bibleImportPhase; }

    qint64 bibleDownloadedBytes() const { return m_bibleDownloadedBytes; }

    qint64 bibleDownloadTotal() const { return m_bibleDownloadTotal; }

    QStringList songTags() const;
    Q_INVOKABLE QStringList parseSongTags(const QVariant &value) const;

    QString status() const { return m_status; }

    bool busy() const { return m_busy; }

    Q_INVOKABLE QVariantList searchSongs(
        const QString &query, const QString &tag = QString()) const;
    Q_INVOKABLE QVariantMap song(const QString &id) const;
    Q_INVOKABLE QString saveSong(const QVariantMap &song);
    Q_INVOKABLE QString createQuickSong(const QString &title, const QString &lyrics,
        const QVariant &tags, const QString &language = QStringLiteral("English"));
    Q_INVOKABLE bool importSong(const QUrl &url);
    Q_INVOKABLE bool exportSong(const QString &id, const QUrl &url);
    Q_INVOKABLE QVariantList books(const QString &translation) const;
    Q_INVOKABLE QVariantList chapters(const QString &translation, const QString &book) const;
    Q_INVOKABLE QVariantList passage(
        const QString &translation, const QString &book, int chapter, int first, int last) const;
    Q_INVOKABLE QVariantMap pairScripture(
        const QVariantList &verses, const QString &secondaryTranslation) const;
    Q_INVOKABLE QVariantList searchVerses(const QString &translation, const QString &query) const;
    Q_INVOKABLE void importBible(const QUrl &url);
    static int selfTest();
signals:
    void changed();
    void bibleImported(const QString &translationId);
    void bibleImportProgressChanged();

private:
    void finishBibleImport(const QUrl &source, const QByteArray &data = QByteArray());

    void setStatus(const QString &value) {
        m_status = value;
        emit changed();
    }

    QSqlDatabase m_db;
    QString m_path, m_status = "Library ready";
    bool m_busy = false;
    QString m_bibleImportPhase;
    qint64 m_bibleDownloadedBytes = 0, m_bibleDownloadTotal = -1;
};
