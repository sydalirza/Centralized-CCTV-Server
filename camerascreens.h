#ifndef CAMERASCREENS_H
#define CAMERASCREENS_H

#include <QWidget>
#include <QMessageBox>
#include "customlabel.h"
#include "singleviewwidget.h"

namespace Ui {
class CameraScreens;
}

class CameraScreens : public QWidget
{
    Q_OBJECT

public:
    explicit CameraScreens(QWidget *parent = nullptr, QWidget *parentWidget = nullptr);
    ~CameraScreens();

private slots:
    void openImage(int boxNumber);

    void on_one_camera_clicked();

    void on_four_camera_clicked();

    void on_sixteen_camera_clicked();

    void onImageClicked();

    void onImageDoubleClicked();

private:
    Ui::CameraScreens *ui;

    // Function to dynamically update the camera layout based on the number of connected cameras
    void updateCameraLayout(int numberOfConnectedCameras, int total_screens);

    // Assume you have a function to get the number of connected cameras
    int getNumberOfConnectedCameras();

    CustomLabel* lastClickedLabel = nullptr;
    CustomLabel *selectedLabel;

    SingleViewWidget *singleViewWidget = nullptr;
    QWidget* parentWidget;
};

#endif // CAMERASCREENS_H
