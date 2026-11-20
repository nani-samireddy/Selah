#pragma once
#include <QMediaPlayer>
#include <QObject>
#include <QQueue>
#include <QSqlDatabase>
#include <QTimer>
#include <QVariantList>
#include <QVideoSink>

// Catalogs managed media independently of slide ownership and schedules thumbnails.
class MediaLibrary final : public QObject {
    Q_OBJECT
    Q_PROPERTY(QVariantList assets READ assets NOTIFY changed)
    Q_PROPERTY(QString error READ error NOTIFY changed)
public:
    explicit MediaLibrary(QObject *parent = nullptr);
    ~MediaLibrary() override;

    QVariantList assets() const { return m_assets; }

    QString error() const { return m_error; }

    Q_INVOKABLE void refresh();
    void registerAsset(const QVariantMap &element);
    static int selfTest();
signals:
    void changed();

private:
    void nextThumbnail();
    void finishThumbnail();
    QString thumbnailPath(const QString &source) const;
    QSqlDatabase m_db;
    QVariantList m_assets;
    QString m_error;
    QQueue<QVariantMap> m_queue;
    QVariantMap m_thumbnailAsset;
    QMediaPlayer m_player;
    QVideoSink m_sink;
    QTimer m_timeout;
    bool m_bulk = false;
};
