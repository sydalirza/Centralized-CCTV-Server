#ifndef CAMERASCREENS_H
#define CAMERASCREENS_H

#include <QWidget>
#include <QMessageBox>
#include "customlabel.h"
#include "singleviewwidget.h"
#include "focusview.h"
#include "camerahandler.h"
#include "camerasettings.h"
#include "rewindui.h"

namespace Ui {
class CameraScreens;
}

class CameraScreens : public QWidget
{
    Q_OBJECT
    static const int MAX_RECONNECT_ATTEMPTS = 3;



public:
    explicit CameraScreens(QWidget *parent = nullptr, QWidget *parentWidget = nullptr, const vector<std::pair<QString, QString>> &cameras = {});

    void connectCameras();
    ~CameraScreens();

public slots:
    void addCamera(const QString& cameraUrl, const QString& cameraName);
    void removeCamera(const QString& cameraName);
    void on_one_camera_clicked();
    void on_four_camera_clicked();
    void on_sixteen_camera_clicked();

private slots:

    void onImageClicked();

    void onImageDoubleClicked();

    void handleFrameUpdate(const QImage &frame, const QString &cameraname);

    void initialize();

    void handleCameraOpened();

    void handleCameraClosed();

    void handleCameraOpeningFailed(const QString &cameraName);

    void changeCamerastatus(const QString &cameraName);

    void on_scale_factor_slider_valueChanged(int value, const QString &cameraName);

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
    FocusView *focusView = nullptr;
    RewindUi *RewindUI = nullptr;

    QWidget* parentWidget;

    int totalWalls = 1; // Set an initial value, adjust as needed
    int currentWall = 16; // Set an initial value, adjust as needed
    int camerasPerWall = 16; // Set an initial value, adjust as needed

    const vector<std::pair<QString, QString>> cameras;
    QVector<QLabel*> cameraLabels;  // Keep track of the QLabel widgets
    CameraHandler cameraHandler;    // Instance of CameraHandler
    CameraSettings cameraSettings;
    QMap<QString, CustomLabel*> cameraLabelMap;
    QMap<QString, SingleViewWidget*> cameraSingleViewWidgets;

    void addCameraLabel(const QString &cameraname, int total_screens, int i);
    void showLayoutButtons(int numberOfConnectedCameras);
};

#endif // CAMERASCREENS_H
