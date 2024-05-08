#include <QCoreApplication>
#include <QHBoxLayout>
#include <QLabel>
#include <QImage>
#include <QTimer>
#include <QPixmap>
#include <QWidget>
#include <QCoreApplication>
#include "opencv2/core.hpp"
#include "opencv2/highgui.hpp"
#include "opencv2/imgproc.hpp"
#include "opencv2/videoio.hpp"
#include "opencv2/objdetect.hpp"
#include <QThread>

using namespace std;
using namespace cv;

class FocusView : public QWidget {
    Q_OBJECT

public:
    FocusView(QWidget *parent = nullptr, const QString& cameraName = "", const std::string& cameraUrl = "");
    ~FocusView();

private slots:
    void updateFrame();
    void stopTimer();

private:
    CascadeClassifier face_cascade;
    VideoCapture capture;
    QLabel *label;
    QTimer *timer;
    QString displayedCamera;
    bool persondetected = false;
    VideoWriter videoWriter;
    bool detectfaces = true;
    QString cameraUrl;
    QThread workerThread;
};
