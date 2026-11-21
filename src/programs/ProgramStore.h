#pragma once
#include <QObject>
#include <QSqlDatabase>
#include <QTimer>
#include <QUrl>
#include <QVariantList>
class OutputManager;

// Curated service snapshots. Mutations save locally; present() alone updates live content.
class ProgramStore final : public QObject {
    Q_OBJECT
    Q_PROPERTY(QVariantList programs READ programs NOTIFY changed)
    Q_PROPERTY(QString id READ id NOTIFY changed)
    Q_PROPERTY(QString title READ title NOTIFY changed)
    Q_PROPERTY(QString date READ date NOTIFY changed)
    Q_PROPERTY(QVariantList items READ items NOTIFY changed)
    Q_PROPERTY(int selected READ selected WRITE setSelected NOTIFY changed)
    Q_PROPERTY(QVariantMap selectedItem READ selectedItem NOTIFY changed)
    Q_PROPERTY(QVariantList editorSlides READ editorSlides NOTIFY changed)
    Q_PROPERTY(QString layoutKind READ layoutKind NOTIFY changed)
    Q_PROPERTY(QVariantList selectedSlides READ selectedSlides NOTIFY changed)
    Q_PROPERTY(QString status READ status NOTIFY changed)
    Q_PROPERTY(QStringList warnings READ warnings NOTIFY changed)
    Q_PROPERTY(bool dirty READ dirty NOTIFY changed)
    Q_PROPERTY(bool canUndo READ canUndo NOTIFY changed)
    Q_PROPERTY(bool canRedo READ canRedo NOTIFY changed)
    Q_PROPERTY(bool live READ live NOTIFY liveChanged)
    Q_PROPERTY(QString liveItemId READ liveItemId NOTIFY liveChanged)
public:
    explicit ProgramStore(
        OutputManager *outputs, QObject *parent = nullptr, const QString &directory = QString());
    ~ProgramStore() override;
    QVariantList programs() const;

    QString id() const { return m_program["id"].toString(); }

    QString title() const { return m_program["title"].toString(); }

    QString date() const { return m_program["date"].toString(); }

    QVariantList items() const { return m_program["items"].toList(); }

    int selected() const { return m_selected; }

    void setSelected(int index);
    QVariantMap selectedItem() const;
    QVariantList selectedSlides() const;
    QString layoutKind() const;
    Q_INVOKABLE QVariantMap previewLayout(const QVariantMap &definition) const;
    Q_INVOKABLE bool applyLayout(const QVariantMap &definition);
    Q_INVOKABLE bool clearLayout();

    QVariantList editorSlides() const {
        auto item = selectedItem();
        item["notes"] = "";
        return renderItem(item);
    }

    QString status() const { return m_status; }

    QStringList warnings() const;

    bool dirty() const { return m_dirty; }

    bool canUndo() const { return !m_undo.isEmpty(); }

    bool canRedo() const { return !m_redo.isEmpty(); }

    bool live() const;
    QString liveItemId() const;
    Q_INVOKABLE bool createProgram(const QString &title, const QString &date);
    Q_INVOKABLE bool openProgram(const QString &id);
    Q_INVOKABLE void updateDetails(const QString &title, const QString &date);
    Q_INVOKABLE bool save();
    Q_INVOKABLE bool addSong(const QVariantMap &song, int maxLines = 3,
        const QVariantMap &definition = {}, const QString &programId = QString());
    Q_INVOKABLE bool addScripture(const QVariantList &verses, int maxLines = 4,
        const QVariantMap &definition = {}, const QString &programId = QString());
    Q_INVOKABLE bool addPresentation(
        const QVariantList &slides, const QString &title, const QString &programId = QString());
    Q_INVOKABLE bool importPresentation(const QUrl &url);
    Q_INVOKABLE bool addMedia(const QVariantMap &asset, const QString &programId = QString());
    Q_INVOKABLE void moveItem(int delta);
    Q_INVOKABLE bool moveItemTo(int from, int to);
    Q_INVOKABLE void removeSelected();
    Q_INVOKABLE void setItemDetails(const QString &title, const QString &notes);
    Q_INVOKABLE bool setArrangement(int index);
    Q_INVOKABLE void setTemplate(const QString &name);
    Q_INVOKABLE bool updateSelectedSlides(const QVariantList &slides);
    Q_INVOKABLE void undo();
    Q_INVOKABLE void redo();
    Q_INVOKABLE bool present(int localSlide = 0);
    Q_INVOKABLE bool exportProgram(const QUrl &url);
    Q_INVOKABLE bool importProgram(const QUrl &url);
    static int selfTest();
signals:
    void changed();
    void liveChanged();

private:
    bool append(QVariantMap item, const QVariantList &slides, const QString &error = QString(),
        const QString &programId = QString());
    void replaceItem(const QVariantMap &item);
    void checkpoint();
    void restoreSelection(const QString &itemId);
    void commit();

    void statusMessage(const QString &message) {
        m_status = message;
        emit changed();
    }

    static QVariantList renderItem(const QVariantMap &item);
    static bool validate(QVariantMap &program, bool newIds);
    QVariantList flatten(int *selectedStart = nullptr) const;
    QSqlDatabase m_db;
    QVariantMap m_program;
    QList<QVariantMap> m_undo, m_redo;
    OutputManager *m_outputs;
    QTimer m_autosave;
    int m_selected = -1;
    bool m_dirty = false;
    QString m_status = "Create a program to prepare a service";
};
