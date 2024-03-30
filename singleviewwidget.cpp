// singleviewwidget.cpp
#include "singleviewwidget.h"
#include <QVBoxLayout>
#include <QCloseEvent>

SingleViewWidget::SingleViewWidget(QWidget *parent, const QString& cameraName, const std::string& cameraUrl)
    : QWidget(parent), displayedCamera(cameraName)
{
    // Create the layout and QLabel for displaying the image
    QVBoxLayout *layout = new QVBoxLayout(this);
    imageLabel = new QLabel(this);
    layout->addWidget(imageLabel);

    // Move the widget to the cameraThread
    // this->moveToThread(&cameraThread);

    // Connect the camera signal to the slot for continuous frame updates
    connect(&cameraHandler, &CameraHandler::frameUpdated, this, &SingleViewWidget::updateImage);

    // Connect the widget signals to the cameraThread slots
    connect(this, &SingleViewWidget::openCameraSignal,
            [this](const string &cameraUrl, const QString &cameraName) {
                cameraHandler.OpenCamera_single(cameraUrl, cameraName);
            });

    connect(this, &SingleViewWidget::closeCameraSignal,
            [this]() {
                cameraHandler.CloseCamera(displayedCamera);
            });
    // Open the initial camera if a name is provided
    if (!cameraName.isEmpty()) {
        emit openCameraSignal(cameraUrl, cameraName);
    }

    if (!face_cascade.load(faceClassifier))
    {
        qDebug() << "Error: Could not load cascade classifier!";
    }
    else
    {
        detectfaces = true;
    }
}

SingleViewWidget::~SingleViewWidget()
{
    // Ensure the cameraThread is finished before destroying the widget
    cameraThread.quit();
    cameraThread.wait();
}

void SingleViewWidget::updateImage(const QImage &frame)
{
    if (detectfaces)
    {
        Mat newframe = ImagetoMat(frame);

        imshow("Live Face Detection", facedetection(newframe));

        QImage finalimage = matToImage(facedetection(newframe));

        // Update the QLabel with the new frame
        imageLabel->setPixmap(QPixmap::fromImage(frame));
        imageLabel->setScaledContents(true);
    }
    else
    {
        imageLabel->setPixmap(QPixmap::fromImage(frame));
        imageLabel->setScaledContents(true);
    }

}

void SingleViewWidget::openCamera(const std::string &cameraUrl, const QString &cameraName)
{
    // Close the current camera, if any
    closeCamera();

    // Emit signal to open the new camera in the cameraThread
    emit openCameraSignal(cameraUrl, cameraName);
    displayedCamera = cameraName;
}

void SingleViewWidget::closeCamera()
{
    // Emit signal to close the camera in the cameraThread
    emit closeCameraSignal();
}

void SingleViewWidget::closeEvent(QCloseEvent *event)
{
    // Close the camera when the tab is closed
    closeCamera();
    event->accept();
}

Mat SingleViewWidget::ImagetoMat(const QImage &image)
{
    Mat mat;
    mat = cv::Mat(image.height(), image.width(), CV_8UC1, const_cast<uchar*>(image.constBits()), image.bytesPerLine()).clone();
    cv::cvtColor(mat, mat, cv::COLOR_RGB2BGR);
    return mat;
}

Mat SingleViewWidget::facedetection(Mat &frame)
{
    Mat frame_gray;
    cvtColor(frame, frame_gray, COLOR_BGR2GRAY);

    vector<Rect> faces;

    face_cascade.detectMultiScale(frame_gray, faces);

    for (size_t i = 0; i < faces.size(); i++)
    {
        Point center(faces[i].x + faces[i].width / 2, faces[i].y + faces[i].height / 2);
        ellipse(frame, center, Size(faces[i].width / 2, faces[i].height / 2), 0, 0, 360, Scalar(0, 0, 255), 6);
        Mat faceROI = frame_gray(faces[i]);
    }

    return frame;
}

QImage SingleViewWidget::matToImage(const Mat &mat) const
{

    if (mat.channels() == 3)
    {
        QImage image(mat.data, mat.cols, mat.rows, mat.step, QImage::Format_RGB888);
        return image.rgbSwapped();
    }

    else if(mat.channels() == 1)
    {
        return QImage(mat.data, mat.cols, mat.rows, mat.step, QImage::Format_Grayscale8);
    }

    qDebug() << "Unsupported Image Format";
    return QImage();
}
