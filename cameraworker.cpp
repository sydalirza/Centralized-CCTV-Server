#include "cameraworker.h"
#include <QImage>
#include <QDebug>

CameraWorker::CameraWorker(const std::string& url, double scaleFactor, QObject* parent)
    : QObject(parent), cameraUrl(url), scaleFactor(scaleFactor), running(false) {
    capture.open(cameraUrl);
    timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &CameraWorker::processFrame);
}

CameraWorker::~CameraWorker() {
    if (capture.isOpened()) {
        capture.release();
    }
}

void CameraWorker::start() {
    if (capture.isOpened()) {
        running = true;
        timer->start(30);  // Process frame every 100 ms
    } else {
        qDebug() << "Could not open Video Capture";
    }
}

void CameraWorker::stop() {
    running = false;
    timer->stop();
}

void CameraWorker::processFrame() {
    if (!running) return;

    cv::Mat frame;
    capture >> frame;
    if (frame.empty()) {
        // qDebug() << "No frame captured from camera";
        return;
    }

    cv::Mat mat;
    cv::resize(frame, mat, cv::Size(), scaleFactor, scaleFactor);

    if (mat.channels() == 3)
    {
        QImage image(mat.data, mat.cols, mat.rows, mat.step, QImage::Format_RGB888);

        emit frameReady(image.rgbSwapped());
    }
}
