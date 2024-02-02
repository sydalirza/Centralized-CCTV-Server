#ifndef CAMERAHANDLER_H
#define CAMERAHANDLER_H


#include <opencv2/opencv.hpp>
#include <QObject>
#include <QImage>
#include <QTimer>

using namespace std;
using namespace cv;

class CameraHandler: public QObject
{
    Q_OBJECT

public:
    explicit CameraHandler(QObject* parent = nullptr);
    ~CameraHandler();

    void OpenCamera(const string &cameraUrl, const QString &cameraname);
    void CloseCamera(const QString &cameraname);
    void closeAllCameras();

    const QImage& getLatestFrame(const QString &cameraname) const;

signals:
    void frameupdated(const QImage& frame, const QString& cameraname);

private slots:
    void updateFrames();

private:
    struct CameraInfo{
        VideoCapture videoCapture;
        QString cameraname;
        QImage latestFrame;
    };

    QTimer* timer;
    QVector<CameraInfo> cameras;


    QImage matToImage(const Mat &mat) const;
};

#endif
