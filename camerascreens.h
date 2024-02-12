#ifndef CAMERASCREENS_H
#define CAMERASCREENS_H

#include <QWidget>
#include <QMessageBox>
#include "customlabel.h"
#include "singleviewwidget.h"
#include "camerahandler.h"

namespace Ui {
class CameraScreens;
}

class CameraScreens : public QWidget
{
    Q_OBJECT

public:
    explicit CameraScreens(QWidget *parent = nullptr, QWidget *parentWidget = nullptr);
    void connectCameras();
    ~CameraScreens();

public slots:
    void addCamera(const QString& cameraUrl, const QString& cameraName);
    void removeCamera(const QString& cameraName);

private slots:

    void on_one_camera_clicked();

    void on_four_camera_clicked();

    void on_sixteen_camera_clicked();

    void onImageClicked();

    void onImageDoubleClicked();

    void onNextClicked();

    void onPreviousClicked();

    void handleFrameUpdate(const QImage &frame, const QString &cameraname);

    void initialize();

private:
    Ui::CameraScreens *ui;
    QTimer *timer;

    // Function to dynamically update the camera layout based on the number of connected cameras
    void updateCameraLayout(int numberOfConnectedCameras, int total_screens);

    // Assume you have a function to get the number of connected cameras
    int getNumberOfConnectedCameras();

    CustomLabel* lastClickedLabel = nullptr;
    CustomLabel *selectedLabel;

    SingleViewWidget *singleViewWidget = nullptr;
    QWidget* parentWidget;

    int totalWalls = 1; // Set an initial value, adjust as needed
    int currentWall = 0; // Set an initial value, adjust as needed
    int camerasPerWall = 16; // Set an initial value, adjust as needed

    QVector<QLabel*> cameraLabels;  // Keep track of the QLabel widgets
    CameraHandler cameraHandler;    // Instance of CameraHandler
    QMap<QString, CustomLabel*> cameraLabelMap;

    void addCameraLabel(const QString &cameraname, int total_screens, int i);
    void showLayoutButtons(int numberOfConnectedCameras);
};

#endif // CAMERASCREENS_H
