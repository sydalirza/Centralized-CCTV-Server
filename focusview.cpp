#include "focusview.h"
#include <QDebug>

FocusView::FocusView(QWidget *parent, const QString& cameraName, const std::string& cameraUrl, double scaleFactor)
    : QWidget(parent), cameraWorker(new CameraWorker(cameraUrl, scaleFactor)) {

    QHBoxLayout *layout = new QHBoxLayout(this);
    label = new QLabel(this);
    label->setScaledContents(true);
    layout->addWidget(label);

    // Move the worker to a separate thread
    cameraWorker->moveToThread(&workerThread);

    // Connect signals and slots
    connect(&workerThread, &QThread::finished, cameraWorker, &QObject::deleteLater);
    connect(cameraWorker, &CameraWorker::frameReady, this, &FocusView::onFrameReady);
    connect(this, &FocusView::stopWorkerSignal, cameraWorker, &CameraWorker::stop);

    // Start the worker thread and processing
    workerThread.start();
    QMetaObject::invokeMethod(cameraWorker, "start", Qt::QueuedConnection);
}

FocusView::~FocusView() {
    // Ensure the worker is stopped and thread is terminated
    onDestroyed();
    qDebug() << "Focus View deleted";
}

void FocusView::stopWorker() {
    if (cameraWorker) {
        emit stopWorkerSignal();
    }
}

void FocusView::onDestroyed() {
    stopWorker();
    if (cameraWorker) {
        disconnect(cameraWorker, &CameraWorker::frameReady, this, &FocusView::onFrameReady);
    }
    workerThread.quit();
    workerThread.wait();
    qDebug() << "Focus View is being destroyed, signals disconnected and worker stopped";
}

void FocusView::onFrameReady(const QImage& frame) {
    label->setPixmap(QPixmap::fromImage(frame).scaled(label->size()));
}
