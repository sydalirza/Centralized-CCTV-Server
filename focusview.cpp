#include "focusview.h"
#include <QDateTime>

FocusView::FocusView(QWidget *parent, const QString& cameraName, const std::string& cameraUrl) : QWidget(nullptr), cameraUrl(QString::fromStdString(cameraUrl)) {
    this->moveToThread(&workerThread);

    workerThread.start();

    QHBoxLayout *layout = new QHBoxLayout(this);
    label = new QLabel(this);
    label->setScaledContents(true);
    layout->addWidget(label);

    capture.open(cameraUrl);
    displayedCamera = cameraName;

    if (!capture.isOpened()) {
        qDebug() << "Could not open Video Capture";
        QCoreApplication::exit(-1);
    }

    timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &FocusView::updateFrame);
    connect(this, &QObject::destroyed, this, &FocusView::stopTimer);
    timer->start(30); // Update every 30 milliseconds

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
    if (!capture.isOpened()) {
        qDebug() << "Video capture is not open, attempting to reopen...";
        capture.open(cameraUrl.toStdString());
        if (!capture.isOpened()) {
            qDebug() << "Failed to reopen video capture";
            return;
        }
    }

    Mat frame;
    capture >> frame;
    if (frame.empty()) {
        qDebug() << "No frame captured from camera";
        QImage placeholderImage("error.png");
        label->setPixmap(QPixmap::fromImage(placeholderImage).scaled(label->size(), Qt::KeepAspectRatio));
        return;
    }

    // Convert OpenCV Mat to QImage
    QImage qImage(frame.data, frame.cols, frame.rows, frame.step, QImage::Format_BGR888);

    // Scale the image to fit the label
    label->setPixmap(QPixmap::fromImage(qImage).scaled(label->size(), Qt::KeepAspectRatio));
}

