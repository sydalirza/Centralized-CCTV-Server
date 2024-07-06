#ifndef CAMERAWORKER_H
#define CAMERAWORKER_H

#include <QObject>
#include <QTimer>
#include <opencv2/opencv.hpp>

class CameraWorker : public QObject {
    Q_OBJECT

public:
    CameraWorker(const std::string& url, double scaleFactor, QObject* parent = nullptr);
    ~CameraWorker();

signals:
    void frameReady(const QImage& frame);

public slots:
    void start();
    void stop();
    void processFrame();
    void showPlaceholderImage();

private:
    std::string cameraUrl;
    double scaleFactor;
    cv::VideoCapture capture;
    QTimer *timer;
    bool running;

    int reconnectionAttempts;
    int maxReconnectionAttempts;
};

#endif // CAMERAWORKER_H
