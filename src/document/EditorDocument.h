#pragma once
#include <QJsonObject>
#include <QObject>
#include <QTimer>
#include <QUrl>
#include <QVariantList>

// Editable presentation draft. History and autosave belong here, never in output windows.
class EditorDocument final : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString backgroundColor READ background NOTIFY changed)
    Q_PROPERTY(double ambientOpacity READ ambientOpacity NOTIFY changed)
    Q_PROPERTY(QVariantList elements READ elements NOTIFY changed)
    Q_PROPERTY(QVariantList slides READ slides NOTIFY changed)
    Q_PROPERTY(int currentSlide READ currentSlide WRITE setCurrentSlide NOTIFY changed)
    Q_PROPERTY(int selected READ selected WRITE setSelected NOTIFY selectionChanged)
    Q_PROPERTY(QVariantMap selectedElement READ selectedElement NOTIFY selectionChanged)
    Q_PROPERTY(bool canUndo READ canUndo NOTIFY changed)
    Q_PROPERTY(bool canRedo READ canRedo NOTIFY changed)
    Q_PROPERTY(QString status READ status NOTIFY statusChanged)
    Q_PROPERTY(QString notes READ notes WRITE setNotes NOTIFY changed)
    Q_PROPERTY(bool importing READ importing NOTIFY importingChanged)
public:
    explicit EditorDocument(QObject *parent = nullptr, bool recover = true);
    QVariantList elements() const;
    QVariantList slides() const;

    int currentSlide() const { return m_current; }

    void setCurrentSlide(int index);

    int selected() const { return m_selected; }

    void setSelected(int index);
    QVariantMap selectedElement() const;

    bool canUndo() const { return !m_undo.empty(); }

    bool canRedo() const { return !m_redo.empty(); }

    QString status() const { return m_status; }

    QString notes() const;
    void setNotes(const QString &notes);
    Q_INVOKABLE void updateElement(int index, const QVariantMap &patch);
    Q_INVOKABLE void formatText(const QVariantMap &patch, int start = -1, int end = -1);
    Q_INVOKABLE void addText();
    Q_INVOKABLE bool addMedia(const QUrl &url);
    Q_INVOKABLE void importMedia(const QUrl &url, bool insertOnSlide = true);
    Q_INVOKABLE bool useMedia(const QVariantMap &asset);

    bool importing() const { return m_importing; }

    Q_INVOKABLE void moveSelected(int delta);
    Q_INVOKABLE bool moveElementTo(int from, int to);
    Q_INVOKABLE void duplicateSelected();
    Q_INVOKABLE void deleteSelected();
    Q_INVOKABLE void undo();
    Q_INVOKABLE void redo();
    Q_INVOKABLE void addSlide();
    Q_INVOKABLE bool addSongSlides(
        const QVariantMap &song, int maxLines = 3, const QVariantMap &definition = {});
    Q_INVOKABLE bool addScriptureSlides(
        const QVariantList &verses, int maxLines = 4, const QVariantMap &definition = {});
    Q_INVOKABLE void setBackground(const QString &color);
    Q_INVOKABLE QString background() const;

    double ambientOpacity() const {
        return m_slides[m_current]
            .toMap()
            .value("ambientOpacity", m_current == 0 ? 0.85 : 0.5)
            .toDouble();
    }

    Q_INVOKABLE bool saveProject(const QUrl &url = QUrl());
    Q_INVOKABLE bool openProject(const QUrl &url);
    Q_INVOKABLE bool replaceSlides(const QVariantList &slides);
    static QVariantList validatedSlides(const QVariantList &slides);
    static int selfTest();
signals:
    void changed();
    void selectionChanged();
    void statusChanged();
    void importingChanged();
    void importFailed(const QString &message);
    void mediaImported(const QVariantMap &element);

private:
    void checkpoint();
    void commit();
    QJsonObject snapshot() const;
    bool restore(const QJsonObject &obj);
    QString recoveryPath() const;
    void setStatus(const QString &text);
    static QVariantMap prepareMedia(const QUrl &url);
    bool insertMedia(const QVariantMap &result, int slideIndex);
    bool appendGenerated(const QVariantList &slides, const QString &error);
    QVariantList m_slides;
    int m_current = 0;
    int m_selected = 1;
    QList<QJsonObject> m_undo, m_redo;
    QTimer m_autosave;
    QString m_status = "All changes saved";
    bool m_importing = false;
    int m_generation = 0;
};
