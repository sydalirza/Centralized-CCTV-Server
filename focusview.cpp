#include "focusview.h"
#include <QDateTime>

FocusView::FocusView(QWidget *parent, const QString& cameraName, const std::string& cameraUrl) : QWidget(nullptr) {
    this->moveToThread(&workerThread);

    workerThread.start();

    QHBoxLayout *layout = new QHBoxLayout(this);
    label = new QLabel(this);
    label->setScaledContents(true);
    layout->addWidget(label);



    string faceClassifier = "C:/Users/Yousuf Traders/Downloads/opencv/sources/data/haarcascades/haarcascade_frontalface_alt2.xml";

    if (!face_cascade.load(faceClassifier)) {
        qDebug() << "Could not load the classifier";
        QCoreApplication::exit(-1);
    }

    qDebug() << "Classifier Loaded!";

    capture.open(cameraUrl);
    displayedCamera = cameraName;

    if (!capture.isOpened()) {
        qDebug() << "Could not open Video Capture";
        QCoreApplication::exit(-1);
    }

    timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &FocusView::updateFrame);
    connect(this, &QObject::destroyed, this, &FocusView::stopTimer);
    timer->start(66); // Update every 30 milliseconds (adjust as needed)

    if (parent)
        setParent(parent);
}

FocusView::~FocusView() {
    if (capture.isOpened()) {
        capture.release();
    }

    stopTimer();
    workerThread.quit();
    workerThread.wait();

    delete this;
    qDebug() << "Focus View deleted";

}

void FocusView::stopTimer() {
    if (timer->isActive()) {
        timer->stop();
    }

}

void FocusView::updateFrame() {
    Mat frame;
    capture >> frame;

    // if (!videoWriter.isOpened()) {
    //     videoWriter.open("output.avi", VideoWriter::fourcc('X','V','I','D'), 10, Size(frame.cols, frame.rows));
    // }

    if (frame.empty()) {
        qDebug() << "No frame captured from camera";
        QImage placeholderImage("E:/ImageViewer/error.png");
        label->setPixmap(QPixmap::fromImage(placeholderImage).scaled(label->size(), Qt::KeepAspectRatio));

        return;
    }

    facedetection(frame);

    // Write frame to VideoWriter
    // if (videoWriter.isOpened()) {
    //     videoWriter.write(frame);
    // }
}

Mat FocusView::facedetection(Mat frame) {
    Mat frame_gray;
    cvtColor(frame, frame_gray, COLOR_BGR2GRAY);

    vector<Rect> faces;

    face_cascade.detectMultiScale(frame_gray, faces);

    if (faces.empty()) {
        // No faces detected, reset persondetected
        persondetected = false;
    }

    for (size_t i = 0; i < faces.size(); i++) {
        Point center(faces[i].x + faces[i].width / 2, faces[i].y + faces[i].height / 2);
        ellipse(frame, center, Size(faces[i].width / 2, faces[i].height / 2), 0, 0, 360, Scalar(0, 0, 255), 1);
        Mat faceROI = frame_gray(faces[i]);
        if(!persondetected)
        {
            QDateTime currentDateTime = QDateTime::currentDateTime();
            QString formattedDateTime = currentDateTime.toString("yyyy-MM-dd hh:mm:ss.zzz");
            qDebug() << "Person detected in the " << displayedCamera << " camera at " << formattedDateTime;
            persondetected = true;
        }
    }

    // Convert OpenCV Mat to QImage
    QImage qImg(frame.data, frame.cols, frame.rows, frame.step, QImage::Format_RGB888);
    qImg = qImg.rgbSwapped(); // BGR to RGB

    // Display the QImage in the QLabel
    label->setPixmap(QPixmap::fromImage(qImg).scaled(label->size(), Qt::KeepAspectRatio));

    return frame;
}
