#ifndef CAMERASETTINGS_H
#define CAMERASETTINGS_H

#include <QWidget>
#include <QtSql/QSqlTableModel>

namespace Ui {
class CameraSettings;
}

class CameraSettings : public QWidget
{
    Q_OBJECT

public:
    explicit CameraSettings(QWidget *parent = nullptr);
    ~CameraSettings();

signals:
    void add_camera(const std::pair<QString, QString> camera);
    void delete_camera(const QString &cameraName);


private slots:
    void on_addbutton_clicked();

    void on_rtsp_radiobutton_clicked();

    void on_mp4_radiobutton_clicked();

    void on_clearbutton_clicked();

    void update_table();

    void update_log_table();

    void on_connectedcameras_tableView_clicked(const QModelIndex &index);

    void on_tableitem_edit_clicked();

    void on_tableitem_delete_clicked();

    void on_update_pushButton_clicked();

    void populate_camera_names();

    void on_applyfilter_button_clicked();

    void openFile(const QModelIndex &index);

private:
    Ui::CameraSettings *ui;
    QSqlTableModel *model;
    QSqlTableModel *logModel;
    QSqlDatabase db;
    bool rtsp = false;
    bool mp4 = false;

};

#endif // CAMERASETTINGS_H
