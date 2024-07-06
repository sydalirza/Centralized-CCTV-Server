// camerascreens.cpp
#include "camerascreens.h"
#include "ui_camerascreens.h"
#include "customlabel.h"
#include "rewindui.h"
#include "focusview.h"
#include <QGridLayout>
#include <QGroupBox>
#include <QPushButton>
#include <QFileDialog>
#include <QPixmap>
#include <QLabel>
#include <QTabWidget>
#include <QThread>
#include <QTimer>
#include <QTabBar>

CameraScreens::CameraScreens(QWidget *parent, QWidget *parentWidget, const std::vector<std::pair<QString, QString>> &cameras)
    : QWidget(parent), ui(new Ui::CameraScreens), parentWidget(parentWidget), cameras(cameras)
{

    ui->setupUi(this);

    updateCameraLayout(16, camerasPerWall); // Blank

    timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &CameraScreens::initialize);
    timer->start(500);

    ui->camerastatusbutton->setVisible(false);

    ui->closecamerabutton->setEnabled(false);

    ui->rewind_button->setEnabled(false);

    ui->scale_factor_slider->setEnabled(false);

    ui->scale_factor_slider->setMinimum(1);

    tabWidget = parentWidget->findChild<QTabWidget*>("tabWidget");

    // Connect the tab close requested signal to the slot
    if (tabWidget) {
        connect(tabWidget->tabBar(), &QTabBar::tabBarDoubleClicked, this, &CameraScreens::handleTabCloseRequested);
    }
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
    updateCameraLayout(numberOfConnectedCameras, camerasPerWall);

    // Connect CameraHandler's signal to handleFrameUpdate slot
    connect(&cameraHandler, &CameraHandler::frameUpdated, this, &CameraScreens::handleFrameUpdate);
    connect(this, &CameraScreens::add_new_face, &cameraHandler, &CameraHandler::add_new_face);
    connect(this, &CameraScreens::delete_face, &cameraHandler, &CameraHandler::delete_face);
}

CameraScreens::~CameraScreens()
{
    qDebug() << "Closing Camera Screens";
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
            disconnect(ui->rewind_button, &QPushButton::clicked, nullptr, nullptr);
            disconnect(ui->camerastatusbutton, &QPushButton::clicked, nullptr, nullptr);
            disconnect(ui->scale_factor_slider, &QSlider::valueChanged, nullptr, nullptr);


            ui->closecamerabutton->setEnabled(false);
            ui->camerastatusbutton->setVisible(false);
            ui->camerastatusbutton->setText("Camera Status:");
            ui->rewind_button->setEnabled(false);
            ui->scale_factor_slider->setEnabled(false);
            }

        lastClickedLabel = clickedLabel;

        // Retrieve the camera name associated with the clicked label
        QString cameraName = cameraLabelMap.key(clickedLabel);


        // Print the name of the clicked camera
        qDebug() << "Clicked Camera: " << cameraName;


        if(cameraHandler.getCameraError(cameraName))
        {
            qDebug() << cameraName << " is disconnected";
            return;
        }
        else
        {
            clickedLabel->setStyleSheet("border: 2px solid red;");
            ui->closecamerabutton->setEnabled(true);
            ui->rewind_button->setEnabled(true);
            ui->camerastatusbutton->setVisible(true);
            ui->camerastatusbutton->setEnabled(true);
            ui->scale_factor_slider->setEnabled(true);

            // Get the scale factor for the clicked camera
            double scaleFactor = cameraHandler.getScalefactor(cameraName);

            // Calculate the position of the slider knob based on the scale factor value and the range of the slider
            int sliderValue = static_cast<int>(scaleFactor * ui->scale_factor_slider->maximum());

            // Set the slider value to the calculated position
            ui->scale_factor_slider->setValue(sliderValue);

            QString scaleFactorString = QString::number(scaleFactor*100, 'f', 0);
            ui->scale_factor_label->setText(scaleFactorString);

            if(cameraHandler.getArmedStatus(cameraName))
            {
                ui->camerastatusbutton->setText("Camera Status: Armed");
                ui->camerastatusbutton->setStyleSheet("background-color: #00ff00");
            }
            else
            {
                ui->camerastatusbutton->setText("Camera Status: Disarmed");
                ui->camerastatusbutton->setStyleSheet("background-color: #ff0000");
            }

            connect(ui->closecamerabutton, &QPushButton::clicked, this, [this, cameraName]() {
                removeCamera(cameraName);
            });

            connect(ui->camerastatusbutton, &QPushButton::clicked, this, [this, cameraName]() {
                changeCamerastatus(cameraName);
                if(cameraHandler.getArmedStatus(cameraName))
                {
                    ui->camerastatusbutton->setText("Camera Status: Armed");
                    ui->camerastatusbutton->setStyleSheet("background-color: #00ff00");
                }
                else
                {
                    ui->camerastatusbutton->setText("Camera Status: Disarmed");
                    ui->camerastatusbutton->setStyleSheet("background-color: #ff0000");
                }
            });

            connect(ui->rewind_button, &QPushButton::clicked, this, [this, cameraName]() {

                if (tabWidget) {
                    int newIndex = tabWidget->addTab(new RewindUi(cameraName, cameraHandler.getFrameBuffer(cameraName), parentWidget), cameraName);
                    tabWidget->setCurrentIndex(newIndex);
                    QTabBar* tabBar = tabWidget->findChild<QTabBar*>();
                    if (tabBar)
                    {
                        QWidget* closeButton = tabBar->tabButton(newIndex, QTabBar::RightSide);
                        if (closeButton)
                        {
                            closeButton->resize(0, 0);
                            closeButton->setVisible(false);
                        }
                    }
                }
            });

            // Connect slider value changed signal to update camera scale factor
            connect(ui->scale_factor_slider, &QSlider::valueChanged, this, [this, cameraName](int value) {
                on_scale_factor_slider_valueChanged(value, cameraName);
            });
        }
    }
}

void CameraScreens::onImageDoubleClicked() {
    CustomLabel* doubleClickedLabel = qobject_cast<CustomLabel*>(sender());

    if (doubleClickedLabel && parentWidget && doubleClickedLabel == lastClickedLabel) {
        QString cameraName = cameraLabelMap.key(doubleClickedLabel);
        std::string singlecameraUrl = cameraHandler.getCameraUrl(cameraName);

        if (cameraHandler.getCameraError(cameraName)) {
            qDebug() << cameraName << " is disconnected";
            return;
        }


        if (tabWidget) {
            int newIndex = tabWidget->addTab(new FocusView(tabWidget, cameraName, singlecameraUrl, cameraHandler.getScalefactor(cameraName)), cameraName);
            tabWidget->setCurrentIndex(newIndex);
            QTabBar* tabBar = tabWidget->findChild<QTabBar*>();
            if (tabBar)
            {
                QWidget* closeButton = tabBar->tabButton(newIndex, QTabBar::RightSide);
                if (closeButton)
                {
                    closeButton->resize(0, 0);
                    closeButton->setVisible(false);
                }
            }
        }
    }
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
                QImage errorImage("loading.png"); //Camera gets disconnected

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

    QPixmap defaultPixmap("loading.png"); // Default
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
    ui->rewind_button->setEnabled(false);
    ui->camerastatusbutton->setEnabled(false);


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

}


void CameraScreens::connectCameras()
{
    qDebug() << "Connecting Cameras";

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
    cameraHandler.CloseCamera(cameraName);

    if (cameraLabelMap.contains(cameraName)) {
        cameraLabelMap.remove(cameraName);
    }

    // Update the UI after removing the camera
    handleCameraClosed();
}

void CameraScreens::handleCameraOpened()
{
    int numberOfConnectedCameras = cameraHandler.getNumberOfConnectedCameras();
    updateCameraLayout(numberOfConnectedCameras, currentWall);
}


void CameraScreens::handleCameraClosed()
{
    int numberOfConnectedCameras = cameraHandler.getNumberOfConnectedCameras();
    updateCameraLayout(numberOfConnectedCameras, currentWall);
}

// Slot to handle camera opening failure
void CameraScreens::handleCameraOpeningFailed(const QString &cameraName)
{
    qDebug() << "Camera opening attempt failed for " << cameraName;
}

void CameraScreens::changeCamerastatus(const QString& cameraName)
{
    cameraHandler.changeCamerastatus(cameraName);
}

void CameraScreens::on_scale_factor_slider_valueChanged(int value, const QString &cameraName)
{
    double scaleFactor = static_cast<double>(value) / 100.0; // Convert the slider value to a double between 0.01 and 1
    // Convert scaleFactor to QString
    QString scaleFactorString = QString::number(value, 'f',0);
    ui->scale_factor_label->setText(scaleFactorString);
    cameraHandler.changeScalefactor(scaleFactor, cameraName);
}

void CameraScreens::handleTabCloseRequested(int index)
{
    if (index != 0) {
        if (tabWidget) {
            QWidget *widget = tabWidget->widget(index);
            if (widget) {
                // Check if the widget's objectName is "CameraSettings"
                if (widget->objectName() == "CameraSettings" || widget->objectName() == "faceshandler") {
                    tabWidget->removeTab(index); // Remove the tab first
                    return;
                }
                tabWidget->removeTab(index);
                delete widget; // Then delete the widget to free up memory
            }
        }
    } else {
        qDebug() << "Cannot close default tab";
    }
}
