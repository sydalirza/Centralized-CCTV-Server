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

}


void CameraSettings::on_rtsp_radiobutton_clicked()
{

}


void CameraSettings::on_mp4_radiobutton_clicked()
{

}

