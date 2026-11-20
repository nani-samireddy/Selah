#include "NdiWorker.h"
#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <QLibrary>

int NdiWorker::selfTest() {
    NdiWorker worker("Queue test", "", 30, true, 50);
    worker.start();
    for (int i = 0; i < 100; i++) {
        QImage frame(32, 18, QImage::Format_RGBA8888);
        frame.fill(Qt::red);
        worker.submit(frame);
    }
    QThread::msleep(130);
    worker.requestStop();
    worker.wait();
    if (worker.dropped() < 90 || worker.processed() > 3) {
        return 1;
    }
    qInfo() << "Sender queue check passed: bounded pending frames, superseded frames dropped, "
               "clean stop.";
    return 0;
}
