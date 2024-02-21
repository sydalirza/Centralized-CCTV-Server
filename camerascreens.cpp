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
    timer->start(500);

    connect(ui->addcamerabutton, &QPushButton::clicked, this, [this](){
        // Replace "C:/Users/Yousuf Traders/Downloads/1.mp4" and "Camera 2" with appropriate values
        addCamera("rtsp://admin:admin@192.168.1.6:8080/h264.sdp", "Camera 8");
    });

    ui->closecamerabutton->setEnabled(false);
}

// Implementation of the initialization slot
void CameraScreens::initialize()
{
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
    currentWall = 1;
    updateCameraLayout(cameraHandler.getNumberOfConnectedCameras(), 1);
}

void CameraScreens::on_four_camera_clicked()
{
    currentWall = 4;
    updateCameraLayout(cameraHandler.getNumberOfConnectedCameras(), 4);
}

void CameraScreens::on_sixteen_camera_clicked()
{
    currentWall = 16;
    updateCameraLayout(cameraHandler.getNumberOfConnectedCameras(), 16);
}


void CameraScreens::onImageClicked()
{

    CustomLabel* clickedLabel = qobject_cast<CustomLabel*>(sender());

    if (clickedLabel && clickedLabel != lastClickedLabel) {
        if (lastClickedLabel) {
            lastClickedLabel->setStyleSheet("");
            disconnect(ui->closecamerabutton, &QPushButton::clicked, nullptr, nullptr);
        }

        lastClickedLabel = clickedLabel;

        // Retrieve the camera name associated with the clicked label
        QString cameraName = cameraLabelMap.key(clickedLabel);


        if(cameraHandler.getCameraError(cameraName))
        {
            qDebug() << cameraName << " is disconnected";
            return;
        }
        else
        {
            clickedLabel->setStyleSheet("border: 2px solid red;");
            ui->closecamerabutton->setEnabled(true);
            connect(ui->closecamerabutton, &QPushButton::clicked, this, [this, cameraName]() {
                removeCamera(cameraName);
            });
        }

        // Print the name of the clicked camera
        qDebug() << "Clicked Camera: " << cameraName;

    }
}

void CameraScreens::onImageDoubleClicked()
{
    CustomLabel* doubleClickedLabel = qobject_cast<CustomLabel*>(sender());

    if (doubleClickedLabel && parentWidget && doubleClickedLabel == lastClickedLabel) {
        // Retrieve the camera name associated with the double-clicked label
        QString cameraName = cameraLabelMap.key(doubleClickedLabel);
        string singlecameraUrl = cameraHandler.getCameraUrl(cameraName);

        if(cameraHandler.getCameraError(cameraName))
        {
            qDebug() << cameraName << " is disconnected";
            return;
        }

        // Create a new singleViewWidget for the camera
        singleViewWidget = new SingleViewWidget(parentWidget, cameraName, singlecameraUrl);

        QTabWidget* tabWidget = parentWidget->findChild<QTabWidget*>("tabWidget");
        if (tabWidget) {
            int newIndex = tabWidget->addTab(singleViewWidget, cameraName);
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
                QImage errorImage("E:/ImageViewer/reconnecting.png"); //Camera gets disconnected

                // Set the label pixmap with the black image
                label->setPixmap(QPixmap::fromImage(errorImage));

            } else {
                // Set the label pixmap with the received frame
                label->setPixmap(QPixmap::fromImage(frame));
            }
        }
    }
    else
    {
        qDebug() << "Camera does not exist: " << cameraName;
    }
}

void CameraScreens::addCameraLabel(const QString& cameraName, int total_screens, int i)
{

    // Determine the number of columns based on the total number of screens
    int columns = (total_screens > 4) ? 4 : 2;

    CustomLabel* imageLabel = new CustomLabel(this);
    cameraLabelMap.insert(cameraName, imageLabel);

    QPixmap defaultPixmap("E:/FYP/image1.png"); // Default
    QPixmap blackPixmap(540, 330);  // Create a black pixmap with the desired size
    blackPixmap.fill(Qt::black);


    if (i < cameraHandler.getNumberOfConnectedCameras()) {
        // Connected camera: Set the pixmap
        imageLabel->setPixmap(defaultPixmap.scaled(540, 330, Qt::IgnoreAspectRatio));
    }
    else {
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
    ui->closecamerabutton->setEnabled(false);
    ui->addcamerabutton->setEnabled(false);


    // Clear existing widgets in the layout
    QLayoutItem* child;
    while ((child = ui->camera_viewer->takeAt(0)) != nullptr) {
        delete child->widget();
        delete child;
    }

    lastClickedLabel = nullptr;

    if (total_screens < numberOfConnectedCameras)
    {
        if(total_screens == 1 && numberOfConnectedCameras <= 4 && numberOfConnectedCameras > 1)
        {
            total_screens = 4;
        }
        else if(total_screens == 4 && numberOfConnectedCameras <= 16 && numberOfConnectedCameras > 4)
        {
            total_screens = 16;
        }
    }

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

    ui->addcamerabutton->setEnabled(true);

}

void CameraScreens::showLayoutButtons(int numberofConnectedCameras)
{
    if(numberofConnectedCameras > 4 || numberofConnectedCameras > 1)
    {
        if(numberofConnectedCameras <= 4 && numberofConnectedCameras > 1)
        {
            ui->one_camera->setEnabled(false);
        }
        else
        {
            ui->one_camera->setEnabled(false);
            ui->four_camera->setEnabled(false);
            ui->sixteen_camera->setEnabled(false);
        }
    }
    else if(numberofConnectedCameras == 1 && !(ui->one_camera->isEnabled()))
    {
        ui->one_camera->setEnabled(true);

        ui->sixteen_camera->setEnabled(true);
    }
    else if(numberofConnectedCameras < 4 && !(ui->four_camera->isEnabled()))
    {
        ui->one_camera->setEnabled(true);

        ui->sixteen_camera->setEnabled(true);
    }
}

void CameraScreens::connectCameras()
{
    qDebug() << "Connecting Cameras";

    // Array of camera details (URL and name)
    const vector<std::pair<QString, QString>> cameras = {
                                                         {"C:/Users/Yousuf Traders/Downloads/1.mp4", "Dining Room"}
                                                         };


    connect(&cameraHandler, &CameraHandler::cameraOpened, this, &CameraScreens::handleCameraOpened);
    connect(&cameraHandler, &CameraHandler::cameraOpeningFailed, this, &CameraScreens::handleCameraOpeningFailed);
    connect(&cameraHandler, &CameraHandler::frameUpdated, this, &CameraScreens::handleFrameUpdate);
    connect(&cameraHandler, &CameraHandler::removeCamera, this, &CameraScreens::removeCamera);


    // Add cameras using a loop
    for (const auto &camera : cameras)
    {
        addCamera(camera.first, camera.second);
    }

    cameraHandler.printConnectedCameras();
}

void CameraScreens::addCamera(const QString& cameraUrl, const QString& cameraName)
{
    qDebug() << "Adding Camera: " << cameraName;
    // Open the new camera
    cameraHandler.OpenCamera(cameraUrl.toStdString(), cameraName);
    handleCameraOpened();
}


void CameraScreens::removeCamera(const QString& cameraName)
{

    qDebug() << "Removing Camera: " << cameraName;

    // Close the camera
    cameraLabelMap.remove(cameraName);
    cameraHandler.CloseCamera(cameraName);

    // Update the UI after removing the camera
    handleCameraClosed();
}

void CameraScreens::handleCameraOpened()
{
    int numberOfConnectedCameras = cameraHandler.getNumberOfConnectedCameras();
    showLayoutButtons(numberOfConnectedCameras);
    updateCameraLayout(numberOfConnectedCameras, currentWall);
}


void CameraScreens::handleCameraClosed()
{
    int numberOfConnectedCameras = cameraHandler.getNumberOfConnectedCameras();
    showLayoutButtons(numberOfConnectedCameras);
    updateCameraLayout(numberOfConnectedCameras, currentWall);
}

// Slot to handle camera opening failure
void CameraScreens::handleCameraOpeningFailed(const QString &cameraName)
{
    qDebug() << "Camera opening attempt failed for " << cameraName;
}

