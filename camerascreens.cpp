// camerascreens.cpp
#include "camerascreens.h"
#include "ui_camerascreens.h"
#include "customlabel.h"
#include <QGridLayout>
#include <QGroupBox>
#include <QPushButton>
#include <QFileDialog>
#include <QPixmap>
#include <QLabel>
#include <QTabWidget>
#include <QThread>
#include <QTimer>
#include "singleviewwidget.h"

CameraScreens::CameraScreens(QWidget *parent, QWidget *parentWidget)
    : QWidget(parent), ui(new Ui::CameraScreens), parentWidget(parentWidget)
{
    ui->setupUi(this);

    connect(ui->next_button, &QPushButton::clicked, this, &CameraScreens::onNextClicked);
    connect(ui->previous_button, &QPushButton::clicked, this, &CameraScreens::onPreviousClicked);

    updateCameraLayout(16, camerasPerWall); // Blank

    timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &CameraScreens::initialize);
    timer->start(1000);

    connect(ui->addcamerabutton, &QPushButton::clicked, this, [this](){
        // Replace "C:/Users/Yousuf Traders/Downloads/1.mp4" and "Camera 2" with appropriate values
        addCamera("C:/Users/Yousuf Traders/Downloads/2.mp4", "Camera 2");
    });

}

// Implementation of the initialization slot
void CameraScreens::initialize()
{
    qDebug() << "here";
    // Disconnect the signal to avoid repeated calls
    disconnect(timer, &QTimer::timeout, this, &CameraScreens::initialize);

    // Call the connectCameras function here
    connectCameras();

    // Continue with the rest of your initialization code
    int numberOfConnectedCameras = cameraHandler.getNumberOfConnectedCameras();
    qDebug() << "Number: " << numberOfConnectedCameras;
    showLayoutButtons(numberOfConnectedCameras);
    updateCameraLayout(numberOfConnectedCameras, camerasPerWall);

    // Connect CameraHandler's signal to handleFrameUpdate slot
    connect(&cameraHandler, &CameraHandler::frameUpdated, this, &CameraScreens::handleFrameUpdate);
}

CameraScreens::~CameraScreens()
{
    delete ui;
}

void CameraScreens::on_one_camera_clicked()
{
    updateCameraLayout(cameraHandler.getNumberOfConnectedCameras(), 1);
}

void CameraScreens::on_four_camera_clicked()
{
    updateCameraLayout(cameraHandler.getNumberOfConnectedCameras(), 4);
}

void CameraScreens::on_sixteen_camera_clicked()
{
    updateCameraLayout(cameraHandler.getNumberOfConnectedCameras(), 16);
}

void CameraScreens::onImageClicked()
{
    CustomLabel* clickedLabel = qobject_cast<CustomLabel*>(sender());

    if (clickedLabel && clickedLabel != lastClickedLabel) {
        if (lastClickedLabel) {
            lastClickedLabel->setStyleSheet("");
        }

        clickedLabel->setStyleSheet("border: 2px solid red;");
        lastClickedLabel = clickedLabel;
    }
}

void CameraScreens::onImageDoubleClicked()
{
    CustomLabel* doubleClickedLabel = qobject_cast<CustomLabel*>(sender());

    if (doubleClickedLabel && parentWidget && doubleClickedLabel == lastClickedLabel) {
        singleViewWidget = new SingleViewWidget(parentWidget);

        singleViewWidget->loadImage(doubleClickedLabel->pixmap());

        QTabWidget* tabWidget = parentWidget->findChild<QTabWidget*>("tabWidget");
        if (tabWidget) {
            int newIndex = tabWidget->addTab(singleViewWidget, "Single View");
            tabWidget->setCurrentIndex(newIndex);
        }
    }
}

void CameraScreens::onNextClicked()
{
    // Handle next button click
}

void CameraScreens::onPreviousClicked()
{
    // Handle previous button click
}

void CameraScreens::handleFrameUpdate(const QImage& frame, const QString& cameraName)
{
    // Update the label with the new frame or set it to a black pixmap if it's an error frame
    if (cameraLabelMap.contains(cameraName)) {
        CustomLabel* label = cameraLabelMap.value(cameraName);

        if (label) {
            // Check if the frame has an error
            if (cameraHandler.getCameraError(cameraName)) {
                // Create a black image
                QImage errorImage(frame.width(), frame.height(), QImage::Format_RGB32);
                errorImage.fill(Qt::red);

                // Set the label pixmap with the black image
                label->setPixmap(QPixmap::fromImage(errorImage));
            } else {
                // Set the label pixmap with the received frame
                label->setPixmap(QPixmap::fromImage(frame));
            }
        }
    }
}

void CameraScreens::addCameraLabel(const QString& cameraName, int total_screens, int i)
{

    // Determine the number of columns based on the total number of screens
    int columns = (total_screens > 4) ? 4 : 2;

    CustomLabel* imageLabel = new CustomLabel(this);
    cameraLabelMap.insert(cameraName, imageLabel);

    QPixmap imagePixmap("error.png");
    QPixmap blackPixmap(540, 330);  // Create a black pixmap with the desired size
    blackPixmap.fill(Qt::black);

    if (i < cameraHandler.getNumberOfConnectedCameras()) {
        // Connected camera: Set the pixmap
        imageLabel->setPixmap(imagePixmap.scaled(540, 330, Qt::KeepAspectRatioByExpanding));
    } else {
        // Not connected camera: Set a black background
        imageLabel->setPixmap(blackPixmap);
    }

    // Enable scaled contents to fill the available space
    imageLabel->setScaledContents(true);

    // Add the label to the layout
    ui->camera_viewer->addWidget(imageLabel, i/columns, i % columns);

    connect(imageLabel, &CustomLabel::clicked, this, &CameraScreens::onImageClicked);
    connect(imageLabel, &CustomLabel::doubleClicked, this, &CameraScreens::onImageDoubleClicked);

    cameraLabels.push_back(imageLabel);
}

void CameraScreens::updateCameraLayout(int numberOfConnectedCameras, int total_screens)
{
    // Clear existing widgets in the layout
    QLayoutItem* child;
    while ((child = ui->camera_viewer->takeAt(0)) != nullptr) {
        delete child->widget();
        delete child;
    }

    lastClickedLabel = nullptr;

    // Load QLabel widgets for connected cameras
    int boxNumber = 1;
    for (int i = 0; i < total_screens; ++i) {
        if (i < numberOfConnectedCameras) {
            const QString cameraName = cameraHandler.getCameraName(i);
            addCameraLabel(cameraName, total_screens, i);
            ++boxNumber;
        }
        else {
            // Handle the case when total_screens > numberOfConnectedCameras
            const QString cameraName = "Not Connected " + QString::number(i - numberOfConnectedCameras + 1);
            addCameraLabel(cameraName, total_screens, i);
            ++boxNumber;
        }
    }

    ui->previous_button->setVisible(total_screens < numberOfConnectedCameras);
    ui->next_button->setVisible(total_screens < numberOfConnectedCameras);
}

void CameraScreens::showLayoutButtons(int numberofConnectedCameras)
{
    if(numberofConnectedCameras > 1)
    {
        ui->one_camera->setEnabled(false);
    }
    else if(numberofConnectedCameras > 4)
    {
        ui->four_camera->setEnabled(false);
    }
}

void CameraScreens::connectCameras()
{
    qDebug() << "Connecting Cameras";

    // Connect signals and slots
    cameraHandler.OpenCamera("C:/Users/Yousuf Traders/Downloads/1.mp4", "Camera 1");

    connect(&cameraHandler, &CameraHandler::frameUpdated, this, &CameraScreens::handleFrameUpdate);
}

void CameraScreens::addCamera(const QString& cameraUrl, const QString& cameraName)
{
    qDebug() << "Adding Camera: " << cameraName;

    // Open the new camera
    cameraHandler.OpenCamera(cameraUrl.toStdString(), cameraName);

    // Update the UI with the new camera
    int numberOfConnectedCameras = cameraHandler.getNumberOfConnectedCameras();
    showLayoutButtons(numberOfConnectedCameras);
    updateCameraLayout(numberOfConnectedCameras, camerasPerWall);
}
