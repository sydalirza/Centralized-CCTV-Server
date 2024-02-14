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
