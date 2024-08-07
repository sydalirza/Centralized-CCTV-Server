#ifndef CAMERAHANDLER_H
#define CAMERAHANDLER_H


#include <opencv2/opencv.hpp>
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

#include <dlib/opencv.h>
#include <dlib/image_processing.h>
#include <dlib/image_processing/frontal_face_detector.h>
#include <dlib/image_processing/render_face_detections.h>
#include <dlib/image_processing.h>
#include <dlib/image_io.h>
#include <dlib/clustering.h>
#include <dlib/string.h>
#include <dlib/dnn.h>

class CameraHandler: public QObject
{
    Q_OBJECT

public:
    explicit CameraHandler(QObject* parent = nullptr);
    ~CameraHandler();

    void OpenCamera(const std::string &cameraUrl, const QString &cameraname);
    void OpenCamera_single(const std::string &cameraUrl, const QString &cameraname);
    void CloseCamera(const QString &cameraname);
    void closeAllCameras();
    void clearFrameBuffer(const QString &cameraname);
    void printConnectedCameras() const;

    const QImage& getLatestFrame(const QString &cameraname) const;
    int getNumberOfConnectedCameras() const;
    QString getCameraName(int index) const;
    std::string getCameraUrl(const QString& cameraname) const;
    bool getCameraError(const QString& cameraname) const;
    QVector<QPair<QDate, QPair<cv::Mat, QTime>>> getFrameBuffer(const QString& cameraname) const;
    void changeCamerastatus(const QString &cameraName);
    bool getArmedStatus(const QString &cameraName) const;
    double getScalefactor(const QString &cameraName);
    void changeScalefactor(double value, const QString &cameraName);

public slots:
    void add_new_face(dlib::matrix<float, 0, 1> face_encoding);
    void delete_face(int num);

signals:
    void frameUpdated(const QImage& frame, const QString& cameraname);
    void cameraOpeningFailed(const QString& cameraname);
    void cameraOpened(const QString& cameraname);
    void removeCamera(const QString& cameraname);

private slots:
    void updateFrames();
    void cleanupOldFrames();

private:

    //Rewinding stuff

    struct FrameInfo{
        QImage frame;
        double timestamp;
    };

    struct CameraInfo{
        cv::VideoCapture videoCapture;
        QString cameraname;
        QImage latestFrame;
        std::string cameraUrl;
        bool isError = false;
        bool isReconnecting = false;
        cv::VideoWriter videoWriter;
        QVector<QPair<QDate, QPair<cv::Mat, QTime>>> CameraRecording;
        QVector<QPair<cv::Mat, QTime>> frameBuffer;
        int currentBufferIndex;
        bool isRecording = false;
        int startFrameIndex;
        int endFrameIndex;
        bool persondetected = false;
        int cooldowntime = 0;
        bool armed = false;
        double scaleFactor = 0.3;
    };

    QTimer openTimer; //Camera Connection Timer
    QTimer *timer; //FPS Timer
    QTimer cleanupTimer;
    QVector<CameraInfo> cameras;
    QThreadPool threadPool;
    bool attemptReconnect(CameraInfo &camera);
    QImage matToImage(const cv::Mat &mat) const;
    void reconnectCamera(CameraInfo& camera);
    void processFrame(CameraInfo& camera);

    void queueSerializationTask(CameraInfo& camera);
    void serialize(const CameraInfo& camera);
    void deserialize(CameraInfo& camera);


    cv::Mat facedetection(cv::Mat frame, CameraInfo &camera);

    const QString videoFolder = "Recordings1";  // Added for video recording
    void initializeVideoWriter(const QString &cameraname);
    void closeVideoWriter(const QString &cameraname);


    cv::Mat detectFaces(const cv::Mat &frame);
    cv::CascadeClassifier faceCascade; // Declare a CascadeClassifier member
    // Load pre-trained face recognition model
    cv::Ptr<cv::face::LBPHFaceRecognizer> recognizer = cv::face::LBPHFaceRecognizer::create();

    QSqlDatabase db;

    bool cameras_armed = false;

    std::vector<dlib::matrix<float, 0, 1>>& encodings;

    dlib::frontal_face_detector detector = dlib::get_frontal_face_detector();

    void load_face_encodings(const std::string& folder_path);
};

#endif
