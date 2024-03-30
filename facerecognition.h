#ifndef FACERECOGNITION_H
#define FACERECOGNITION_H

#include <QObject>
#include <QImage>
#include <QDebug>

#include <opencv2/opencv.hpp>
#include <opencv2/core.hpp>
#include <opencv2/objdetect.hpp>
#include <opencv2/objdetect/face.hpp>

using namespace cv;

class FaceRecognition : public QObject
{
    Q_OBJECT

public:
    explicit FaceRecognition(QObject *parent = nullptr);
    void performFacialDetection(const Mat& inputFrame, Mat& outputFrame);
    ~FaceRecognition();

public slots:
    void processFrame(const QImage &frame, const QString &cameraName);

signals:
    void faceDetected(const QImage &frameWithFaces, const QString &cameraName);

private:
    CascadeClassifier faceCascade;

    void detectFaces(const cv::Mat &frame, cv::Mat &frameWithFaces);
    QImage MatToQImage(const cv::Mat &mat);
    cv::Mat QImageToMat(const QImage &image);
};

#endif // FACERECOGNITION_H
