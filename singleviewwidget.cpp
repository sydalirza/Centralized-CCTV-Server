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
    this->moveToThread(&cameraThread);

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
}

SingleViewWidget::~SingleViewWidget()
{
    // Ensure the cameraThread is finished before destroying the widget
    cameraThread.quit();
    cameraThread.wait();
}

void SingleViewWidget::updateImage(const QImage &frame)
{
    // Update the QLabel with the new frame
    imageLabel->setPixmap(QPixmap::fromImage(frame));
    imageLabel->setScaledContents(true);
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
