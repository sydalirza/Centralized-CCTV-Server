#ifndef FOCUSVIEW_H
#define FOCUSVIEW_H

#include <QWidget>
#include <QLabel>
#include <QHBoxLayout>
#include <QThread>
#include "cameraworker.h"

class FocusView : public QWidget {
    Q_OBJECT

public:
    FocusView(QWidget *parent, const QString& cameraName, const std::string& cameraUrl, double scaleFactor);
    ~FocusView();

public slots:
    void stopWorker();
    void onDestroyed();

signals:
    void stopWorkerSignal();

private slots:
    void onFrameReady(const QImage& frame);


private:
    QLabel *label;
    QThread workerThread;
    CameraWorker *cameraWorker;
};

#endif // FOCUSVIEW_H
