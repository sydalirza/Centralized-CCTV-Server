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

    // Create a layout for the camera_viewer
    // QGridLayout *cameraLayout = ui->camera_viewer;

    connect(ui->next_button, &QPushButton::clicked, this, &CameraScreens::onNextClicked);
    connect(ui->previous_button, &QPushButton::clicked, this, &CameraScreens::onPreviousClicked);

    int numberOfConnectedCameras = getNumberOfConnectedCameras();
    // Initially load the camera layout with placeholders
    if (numberOfConnectedCameras <= 1)
    {
        updateCameraLayout(numberOfConnectedCameras, 1);
    }
    else if ( numberOfConnectedCameras > 1 && numberOfConnectedCameras <= 4)
    {
        updateCameraLayout(numberOfConnectedCameras, 4);
    }
    else
    {
        updateCameraLayout(numberOfConnectedCameras, 16);
    }


}

CameraScreens::~CameraScreens()
{
    delete ui;
}

void CameraScreens::openImage(int boxNumber)
{
    // Open an image using QFileDialog
    QString imagePath = QFileDialog::getOpenFileName(this, "Open Image", QDir::homePath(), "Images (*.png *.jpg *.bmp)");

    if (!imagePath.isEmpty()) {
        // You can do something with the image path, for example, display it in a QLabel
        // For simplicity, let's show the image path in a message box
        QMessageBox::information(this, "Image Path", "Image path for Box " + QString::number(boxNumber) + ": " + imagePath);
    }
}

int CameraScreens::getNumberOfConnectedCameras()
{
    return 0;
}

void CameraScreens::updateCameraLayout(int numberOfConnectedCameras, int total_screens)
{
    // Clear existing widgets in the layout
    QLayoutItem *child;
    while ((child = ui->camera_viewer->takeAt(0)) != nullptr) {
        delete child->widget();
        delete child;
    }
    lastClickedLabel = nullptr;
    // Determine the number of columns based on the total number of screens
    int columns = (total_screens > 4) ? 4 : 2;

    // Load QLabel widgets for connected cameras
    for (int i = 0; i < total_screens; ++i) {
        CustomLabel *imageLabel = new CustomLabel(this);
        QPixmap imagePixmap("E:/FYP/image1.png"); // Placeholder for connected cameras

        // Set the pixmap to the label and scale it if needed
        imageLabel->setPixmap(imagePixmap.scaled(540, 330, Qt::KeepAspectRatioByExpanding));

        // Enable scaled contents to fill the available space
        imageLabel->setScaledContents(true);

        // Add the label to the layout
        ui->camera_viewer->addWidget(imageLabel, i / columns, i % columns);

        connect(imageLabel, &CustomLabel::clicked, this, &CameraScreens::onImageClicked);

        connect(imageLabel, &CustomLabel::doubleClicked, this, &CameraScreens::onImageDoubleClicked);
    }

    // Load QLabel widgets for cameras that are not connected
    for (int i = numberOfConnectedCameras; i < total_screens; ++i) {
        CustomLabel *imageLabel = new CustomLabel(this);
        QPixmap imagePixmap("E:/FYP/image.png"); // Placeholder for not connected cameras

        // Set the pixmap to the label and scale it if needed
        imageLabel->setPixmap(imagePixmap.scaled(540, 330, Qt::KeepAspectRatioByExpanding));

        // Enable scaled contents to fill the available space
        imageLabel->setScaledContents(true);

        // Add the label to the layout
        ui->camera_viewer->addWidget(imageLabel, i / columns, i % columns);
        connect(imageLabel, &CustomLabel::clicked, this, &CameraScreens::onImageClicked);

        connect(imageLabel, &CustomLabel::doubleClicked, this, &CameraScreens::onImageDoubleClicked);
    }


    ui->previous_button->setVisible(total_screens < numberOfConnectedCameras);
    ui->next_button->setVisible(total_screens < numberOfConnectedCameras);


}

// Example usage in one of your slots
void CameraScreens::on_one_camera_clicked()
{
    // Assume you have a function to get the number of connected cameras
    int numberOfConnectedCameras = getNumberOfConnectedCameras();

    // Update the camera layout based on the number of connected cameras
    updateCameraLayout(numberOfConnectedCameras, 1);
}

void CameraScreens::on_four_camera_clicked()
{
    // Assume you have a function to get the number of connected cameras
    int numberOfConnectedCameras = getNumberOfConnectedCameras();

    // Update the camera layout based on the number of connected cameras
    updateCameraLayout(numberOfConnectedCameras, 4);
}

// Similarly, implement functions for eight, sixteen, and thirtytwo cameras

void CameraScreens::on_sixteen_camera_clicked()
{
    // Assume you have a function to get the number of connected cameras
    int numberOfConnectedCameras = getNumberOfConnectedCameras();

    // Update the camera layout based on the number of connected cameras
    updateCameraLayout(numberOfConnectedCameras, 16);
}

void CameraScreens::onImageClicked()
{

    CustomLabel *clickedLabel = qobject_cast<CustomLabel*>(sender());

    if (clickedLabel && clickedLabel != lastClickedLabel) {
        // Remove border from the last clicked label
        if (lastClickedLabel) {
            lastClickedLabel->setStyleSheet("");
        }

        // Add border to the clicked label
        clickedLabel->setStyleSheet("border: 2px solid red;");

        // Store the last clicked label
        lastClickedLabel = clickedLabel;
    }
}

void CameraScreens::onImageDoubleClicked()
{
    CustomLabel *doubleClickedLabel = qobject_cast<CustomLabel*>(sender());

    if (doubleClickedLabel && parentWidget && doubleClickedLabel == lastClickedLabel) {
        // Create a new instance of SingleViewWidget
        singleViewWidget = new SingleViewWidget(parentWidget);


        // Load the image into SingleViewWidget
        singleViewWidget->loadImage(doubleClickedLabel->pixmap());

        // Open a new tab and display the SingleViewWidget
        QTabWidget* tabWidget = parentWidget->findChild<QTabWidget*>("tabWidget");
        if (tabWidget) {
            int newIndex = tabWidget->addTab(singleViewWidget, "Single View");
            tabWidget->setCurrentIndex(newIndex);
        }
    }
}

void CameraScreens::onNextClicked()
{

}

void CameraScreens::onPreviousClicked()
{

}




