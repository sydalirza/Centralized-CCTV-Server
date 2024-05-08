#ifndef CAMERAHANDLER_H
#define CAMERAHANDLER_H


#include <opencv2/opencv.hpp>
#include "facerecognition.h"
#include <QObject>
#include <QImage>
#include <QTimer>
#include <QThread>
#include <QRunnable>
#include <QThreadPool>
#include <QFile>
#include <QDataStream>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>

#include <opencv2/opencv.hpp>
#include <opencv2/face.hpp>

using namespace cv::face;
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
    void clearFrameBuffer(const QString &cameraname);
    void printConnectedCameras() const;

    const QImage& getLatestFrame(const QString &cameraname) const;
    int getNumberOfConnectedCameras() const;
    QString getCameraName(int index) const;
    string getCameraUrl(const QString& cameraname) const;
    bool getCameraError(const QString& cameraname) const;
    QVector<QPair<QDate, QPair<Mat, QTime>>> getFrameBuffer(const QString& cameraname) const;

signals:
    void frameUpdated(const QImage& frame, const QString& cameraname);
    void cameraOpeningFailed(const QString& cameraname);
    void cameraOpened(const QString& cameraname);
    void removeCamera(const QString& cameraname);

private slots:
    void updateFrames();

private:

    //Rewinding stuff

    struct FrameInfo{
        QImage frame;
        double timestamp;
    };

    struct CameraInfo{
        VideoCapture videoCapture;
        QString cameraname;
        QImage latestFrame;
        string cameraUrl;
        bool isError = false;
        bool isReconnecting = false;
        VideoWriter videoWriter;
        QVector<QPair<QDate, QPair<Mat, QTime>>> CameraRecording;
        QVector<QPair<Mat, QTime>> frameBuffer;
        int currentBufferIndex;
        bool isRecording = false;
        int startFrameIndex;
        int endFrameIndex;
        bool persondetected = false;
        int cooldowntime = 0;
    };

    QTimer openTimer; //Camera Connection Timer
    QTimer *timer; //FPS Timer
    QVector<CameraInfo> cameras;
    QThreadPool threadPool;
    bool attemptReconnect(CameraInfo &camera);
    QImage matToImage(const Mat &mat) const;
    void reconnectCamera(CameraInfo& camera);
    void processFrame(CameraInfo& camera);

    void queueSerializationTask(CameraInfo& camera);
    void serialize(const CameraInfo& camera);
    void deserialize(CameraInfo& camera);


    Mat facedetection(Mat frame, CameraInfo &camera);

    const QString videoFolder = "Recordings";  // Added for video recording
    void initializeVideoWriter(const QString &cameraname);
    void closeVideoWriter(const QString &cameraname);


    Mat detectFaces(const cv::Mat &frame);
    cv::CascadeClassifier faceCascade; // Declare a CascadeClassifier member
    // Load pre-trained face recognition model
    Ptr<LBPHFaceRecognizer> recognizer = LBPHFaceRecognizer::create();

    QSqlDatabase db;


};

#endif
