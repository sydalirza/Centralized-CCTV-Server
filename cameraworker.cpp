#include "cameraworker.h"
#include <QImage>
#include <QDebug>
#include <QThread>

CameraWorker::CameraWorker(const std::string& url, double scaleFactor, QObject* parent)
    : QObject(parent), cameraUrl(url), scaleFactor(scaleFactor), running(false), reconnectionAttempts(0), maxReconnectionAttempts(5)
{
    capture.open(cameraUrl);
    timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &CameraWorker::processFrame);
}

CameraWorker::~CameraWorker()
{
    if (capture.isOpened()) {
        capture.release();
    }
}

void CameraWorker::start()
{
    if (capture.isOpened()) {
        running = true;
        reconnectionAttempts = 0;
        timer->start(30);  // Process frame every 30 ms (approx. 33 fps)
    } else {
        qDebug() << "Could not open Video Capture";
    }
}

void CameraWorker::stop()
{
    running = false;
    timer->stop();
}

void CameraWorker::processFrame()
{
    if (!running) return;

    cv::Mat frame;
    capture >> frame;
    if (frame.empty()) {
        qDebug() << "No frame captured from camera, attempting to reconnect...";

        // Stop the timer to prevent multiple reconnection attempts simultaneously
        timer->stop();

        if (reconnectionAttempts < maxReconnectionAttempts) {
            QThread::sleep(2);  // Wait for 2 seconds before trying to reconnect
            if (!capture.open(cameraUrl)) {
                qDebug() << "Reconnection attempt" << (reconnectionAttempts + 1) << "failed";
                reconnectionAttempts++;
            } else {
                qDebug() << "Reconnected successfully";
                reconnectionAttempts = 0;
            }
        } else {
            qDebug() << "Max reconnection attempts reached. Stopping camera worker.";
            running = false;
            showPlaceholderImage();
        }

        // Restart the timer
        if (running) {
            timer->start(30);
        }
        return;
    }

    cv::Mat mat;
    cv::resize(frame, mat, cv::Size(), scaleFactor, scaleFactor);

    if (mat.channels() == 3) {
        QImage image(mat.data, mat.cols, mat.rows, mat.step, QImage::Format_RGB888);
        emit frameReady(image.rgbSwapped());
    }
}

void CameraWorker::showPlaceholderImage()
{
    QImage placeholderImage("loading.png");
    if (placeholderImage.isNull()) {
        qDebug() << "Failed to load placeholder image.";
    } else {
        emit frameReady(placeholderImage);
    }
}
