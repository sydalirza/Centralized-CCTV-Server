#include "facerecognition.h"
#include <opencv2/face.hpp>


using namespace cv;
using namespace cv::face;
using namespace std;

FaceRecognition::FaceRecognition(QObject *parent) : QObject(parent) {
    // Load the pre-trained face cascade classifier
    QString cascadePath = "C:/Users/Yousuf Traders/Downloads/opencv/sources/data/haarcascades/haarcascade_frontalface_default.xml";  // Change the path accordingly
    if (!faceCascade.load(cascadePath.toStdString())) {
        qWarning() << "Error loading face cascade.";
    }
}

FaceRecognition::~FaceRecognition() {
    // Destructor implementation, if needed
}

void FaceRecognition::processFrame(const QImage &frame, const QString &cameraName) {
    // Convert QImage to cv::Mat
    Mat cvFrame = QImageToMat(frame);

    // Detect faces
    Mat frameWithFaces;
    detectFaces(cvFrame, frameWithFaces);

    // Convert cv::Mat back to QImage
    QImage frameWithFacesImage = MatToQImage(frameWithFaces);


    // Emit signal with the frame containing faces
    emit faceDetected(frameWithFacesImage, cameraName);
}

void FaceRecognition::detectFaces(const cv::Mat &frame, cv::Mat &frameWithFaces) {
    Mat grayFrame;
    cvtColor(frame, grayFrame, COLOR_BGR2GRAY);
    equalizeHist(grayFrame, grayFrame);

    vector<Rect> faces;
    faceCascade.detectMultiScale(grayFrame, faces, 1.1, 2, 0 | cv::CASCADE_SCALE_IMAGE, cv::Size(30, 30));

    frameWithFaces = frame.clone();

    if (!faces.empty()) {
        qDebug() << "Faces detected!";
    } else {
        qDebug() << "No faces detected.";
    }

    for (const auto &face : faces) {
        rectangle(frameWithFaces, face, Scalar(0, 255, 0), 2);
    }
}

QImage FaceRecognition::MatToQImage(const cv::Mat &mat) {
    // Convert the OpenCV Mat to QImage
    QImage image(mat.data, mat.cols, mat.rows, mat.step, QImage::Format_RGB888);
    return image.rgbSwapped();
}

Mat FaceRecognition::QImageToMat(const QImage &image) {
    // Convert the QImage to OpenCV Mat
    Mat mat(image.height(), image.width(), CV_8UC4, const_cast<uchar *>(image.bits()), image.bytesPerLine());
    return mat.clone();
}

void FaceRecognition::performFacialDetection(const Mat& inputFrame, Mat& outputFrame)
{
    detectFaces(inputFrame, outputFrame);
}
