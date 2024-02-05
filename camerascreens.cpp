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
#include "singleviewwidget.h"

CameraScreens::CameraScreens(QWidget *parent, QWidget *parentWidget)
    : QWidget(parent), ui(new Ui::CameraScreens), parentWidget(parentWidget)
{
    ui->setupUi(this);

    connect(ui->next_button, &QPushButton::clicked, this, &CameraScreens::onNextClicked);
    connect(ui->previous_button, &QPushButton::clicked, this, &CameraScreens::onPreviousClicked);

    int numberOfConnectedCameras = cameraHandler.getNumberOfConnectedCameras();
    qDebug() << "Number: " << numberOfConnectedCameras;
    updateCameraLayout(numberOfConnectedCameras, camerasPerWall);

    // Connect CameraHandler's signal to handleFrameUpdate slot
    connect(&cameraHandler, &CameraHandler::frameUpdated, this, &CameraScreens::handleFrameUpdate);
}

CameraScreens::~CameraScreens()
{
    delete ui;
}

// void CameraScreens::openImage(int boxNumber)
// {
//     // Open an image using QFileDialog
//     QString imagePath = QFileDialog::getOpenFileName(this, "Open Image", QDir::homePath(), "Images (*.png *.jpg *.bmp)");

//     if (!imagePath.isEmpty()) {
//         // You can do something with the image path, for example, display it in a QLabel
//         // For simplicity, let's show the image path in a message box
//         QMessageBox::information(this, "Image Path", "Image path for Box " + QString::number(boxNumber) + ": " + imagePath);
//     }
// }

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
    // Update the label with the new frame
    if (cameraLabelMap.contains(cameraName)) {
        CustomLabel* label = cameraLabelMap.value(cameraName);
        label->setPixmap(QPixmap::fromImage(frame));
    }
}

void CameraScreens::addCameraLabel(const QString& cameraName, int total_screens, int i)
{

    // Determine the number of columns based on the total number of screens
    int columns = (total_screens > 4) ? 4 : 2;

    CustomLabel* imageLabel = new CustomLabel(this);
    cameraLabelMap.insert(cameraName, imageLabel);

    QPixmap imagePixmap("error.png");

    // Set the initial pixmap (you can set a placeholder here)
    imageLabel->setPixmap(imagePixmap.scaled(540, 330, Qt::KeepAspectRatioByExpanding));

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

