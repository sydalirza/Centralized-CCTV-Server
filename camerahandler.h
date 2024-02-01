// CameraHandler.h
#ifndef CAMERAHANDLER_H
#define CAMERAHANDLER_H

#include <opencv2/opencv.hpp>
#include <opencv2/videoio.hpp>
#include <QObject>
#include <QTimer>


class CameraHandler : public QObject
{
    Q_OBJECT

public:
    explicit CameraHandler(QObject *parent = nullptr);
    ~CameraHandler();

    void openCamera(const std::string& cameraUrl, const QString& cameraName);
    void closeAllCameras();

signals:
    void frameUpdated(const cv::Mat& frame, const QString& cameraName);

private slots:
    void updateFrame();

private:
    struct CameraInfo
    {
        cv::VideoCapture videoCapture;
        QString cameraName;
    };

    QTimer *timer;
    QVector<CameraInfo> cameras;
};

#endif // CAMERAHANDLER_H
