#include "camerasettings.h"
#include "ui_camerasettings.h"

CameraSettings::CameraSettings(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::CameraSettings)
{
    ui->setupUi(this);

}

CameraSettings::~CameraSettings()
{
    delete ui;
}

void CameraSettings::on_addbutton_clicked()
{
    QString name_camera = ui->camera_name->text();
    QString url_camera = ui->url_address->text();

    qDebug() << name_camera;
    qDebug() << url_camera;

    emit add_camera(name_camera, url_camera);
}


void CameraSettings::on_rtsp_radiobutton_clicked()
{
    rtsp = true;
    mp4 = false;
    ui->port->setEnabled(true);
    ui->username->setEnabled(true);
    ui->password->setEnabled(true);
    ui->ip_address->setEnabled(true);
}


void CameraSettings::on_mp4_radiobutton_clicked()
{
    rtsp = false;
    mp4 = true;
    ui->port->setEnabled(false);
    ui->username->setEnabled(false);
    ui->password->setEnabled(false);
    ui->ip_address->setEnabled(false);
}


void CameraSettings::on_clearbutton_clicked()
{
    ui->port->clear();
    ui->username->clear();
    ui->password->clear();
    ui->ip_address->clear();
    ui->camera_name->clear();
    ui->url_address->clear();
}

