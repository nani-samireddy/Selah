#include "ProgramStore.h"
#include <QDir>
#include <QFileInfo>
#include <QtConcurrentRun>

bool ProgramStore::exportBundle(const QUrl &url) {
    return startBundle(url, false);
}

bool ProgramStore::importBundle(const QUrl &url) {
    return startBundle(url, true);
}

bool ProgramStore::startBundle(const QUrl &url, bool importing) {
    if (!url.isLocalFile() || bundleBusy() || (!importing && id().isEmpty())) {
        return false;
    }
    if (m_dirty && !save()) {
        return false;
    }
    m_bundleProgress = std::make_shared<BundleProgress>();
    const auto progress = m_bundleProgress;
    const auto path = url.toLocalFile();
    const auto directory = QFileInfo(m_db.databaseName()).absolutePath();
    const auto program = m_program;
    m_status = importing ? "Importing portable program…" : "Packing program and media…";
    disconnect(&m_bundleWatcher, nullptr, this, nullptr);
    connect(&m_bundleWatcher, &QFutureWatcher<BundleResult>::finished, this, [this, importing] {
        m_bundleTimer.stop();
        const auto result = m_bundleWatcher.result();
        if (!result.error.isEmpty()) {
            statusMessage(result.error);
            return;
        }
        if (!importing) {
            statusMessage("Portable program exported · media and template snapshots included. "
                          "Matching fonts are needed on the other computer.");
            return;
        }
        auto candidate = result.program;
        auto cleanup = [&] {
            if (!result.directory.isEmpty()) {
                QDir(result.directory).removeRecursively();
            }
        };
        if (!validate(candidate, true)) {
            cleanup();
            statusMessage("Invalid bundled program; current service kept");
            return;
        }
        const auto previous = m_program;
        // A bundle is a new local program; the loaded live snapshot remains untouched.
        m_program = candidate;
        m_dirty = true;
        if (!save()) {
            m_program = previous;
            m_dirty = false;
            cleanup();
            emit changed();
            return;
        }
        m_undo.clear();
        m_redo.clear();
        m_selected = items().isEmpty() ? -1 : 0;
        statusMessage(
            "Portable program imported · available offline. Check fonts before presenting.");
        emit liveChanged();
    });
    disconnect(&m_bundleTimer, nullptr, this, nullptr);
    m_bundleTimer.setInterval(100);
    connect(&m_bundleTimer, &QTimer::timeout, this, &ProgramStore::changed);
    m_bundleWatcher.setFuture(QtConcurrent::run([program, path, directory, progress, importing] {
        return importing ? PortableBundle::read(path, directory, progress)
                         : PortableBundle::write(program, path, progress);
    }));
    m_bundleTimer.start();
    emit changed();
    return true;
}
