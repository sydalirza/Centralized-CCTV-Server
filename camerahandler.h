#ifndef CAMERAHANDLER_H
#define CAMERAHANDLER_H


#include <opencv2/opencv.hpp>
#include <QObject>
#include <QImage>
#include <QTimer>
#include <QThread>
#include <QRunnable>
#include <QThreadPool>

using namespace std;
using namespace cv;

class CameraHandler: public QObject
{
    Q_OBJECT

public:
    explicit CameraHandler(QObject* parent = nullptr);
    ~CameraHandler();

    void OpenCamera(const string &cameraUrl, const QString &cameraname);
    void OpenCamera_single(const string &cameraUrl, const QString &cameraname);
    void CloseCamera(const QString &cameraname);
    void closeAllCameras();
    void printConnectedCameras() const;

    const QImage& getLatestFrame(const QString &cameraname) const;
    int getNumberOfConnectedCameras() const;
    QString getCameraName(int index) const;
    string getCameraUrl(const QString& cameraname) const;
    bool getCameraError(const QString& cameraname) const;

signals:
    void frameUpdated(const QImage& frame, const QString& cameraname);
    void cameraOpeningFailed(const QString& cameraname);
    void cameraOpened(const QString& cameraname);
    void removeCamera(const QString& cameraname);

private slots:
    void updateFrames();

private:
    struct CameraInfo{
        VideoCapture videoCapture;
        QString cameraname;
        QImage latestFrame;
        string cameraUrl;
        bool isError = false;
        bool isReconnecting = false;
    };

    QTimer openTimer; //Camera Connection Timer
    QTimer *timer; //FPS Timer
    QVector<CameraInfo> cameras;
    QThreadPool threadPool;
    bool attemptReconnect(CameraInfo &camera);
    QImage matToImage(const Mat &mat) const;
    void reconnectCamera(CameraInfo& camera);
    void processFrame(CameraInfo& camera);

};

#endif
