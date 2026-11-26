#pragma once
#include <QObject>
#include <QUrl>
#include <QVariantList>

class ContentLibrary;
class PresentationLibrary;
class MediaLibrary;
class OutputManager;

// Stages spontaneous content independently of the editor and saved program.
class QuickAddController final : public QObject {
    Q_OBJECT
    Q_PROPERTY(QVariantList slides READ slides NOTIFY changed)
    Q_PROPERTY(QString title READ title NOTIFY changed)
    Q_PROPERTY(QString status READ status NOTIFY changed)
    Q_PROPERTY(QVariantMap draft READ draft WRITE setDraft NOTIFY draftChanged)
    Q_PROPERTY(bool canSaveDraft READ canSaveDraft NOTIFY changed)
public:
    QuickAddController(ContentLibrary *content, PresentationLibrary *presentations,
        MediaLibrary *media, OutputManager *outputs, QObject *parent = nullptr,
        bool preferences = true);

    QVariantList slides() const { return m_slides; }

    QString title() const { return m_title; }

    QString status() const { return m_status; }

    QVariantMap draft() const { return m_draft; }

    bool canSaveDraft() const { return m_draftPreview; }

    void setDraft(const QVariantMap &draft);
    Q_INVOKABLE QVariantList searchSongs(const QString &query) const;
    Q_INVOKABLE bool previewSong(const QString &id);
    Q_INVOKABLE bool previewBible(
        const QString &translation, const QString &query, const QString &secondary = QString());
    Q_INVOKABLE bool previewScripture(
        const QVariantList &verses, const QVariantMap &definition, int lines = 4);
    Q_INVOKABLE bool presentScripture();
    Q_INVOKABLE bool advanceScripture(int delta);
    Q_INVOKABLE bool previewDraft(bool song);
    Q_INVOKABLE bool previewMedia(const QVariantMap &asset);
    Q_INVOKABLE bool importFile(const QUrl &source, bool presentation);
    Q_INVOKABLE bool previewPresentation(const QString &id);
    Q_INVOKABLE bool add(bool now);
    Q_INVOKABLE bool saveDraft();
    Q_INVOKABLE void clearPreview();
signals:
    void changed();
    void draftChanged();

private:
    bool stage(const QVariantList &slides, const QString &title, const QString &error = QString());
    ContentLibrary *m_content;
    PresentationLibrary *m_presentations;
    MediaLibrary *m_media;
    OutputManager *m_outputs;
    QVariantMap m_draft;
    QVariantList m_slides;
    QString m_scriptureKey, m_presentedScriptureKey;
    QString m_title, m_status = "Choose content to preview. The current slide stays live.";
    bool m_preferences;
    bool m_draftPreview = false, m_songDraft = false;
};
