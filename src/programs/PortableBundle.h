#pragma once
#include <atomic>
#include <memory>
#include <QVariantMap>

struct BundleProgress {
    std::atomic_bool cancelled { false };
    std::atomic_int percent { 0 };
};

struct BundleResult {
    QVariantMap program;
    QString directory, error;
};

namespace PortableBundle {
BundleResult write(const QVariantMap &program, const QString &path,
    const std::shared_ptr<BundleProgress> &progress);
BundleResult read(
    const QString &path, const QString &directory, const std::shared_ptr<BundleProgress> &progress);
}
