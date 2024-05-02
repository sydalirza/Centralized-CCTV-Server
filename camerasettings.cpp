#include "camerasettings.h"
#include "ui_camerasettings.h"
#include <QtSql/QSqlDatabase>
#include <QtSql/QSqlQuery>
#include <QtSql/QSqlError>
#include <QtSql/QSqlTableModel>
#include <QMessageBox>

CameraSettings::CameraSettings(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::CameraSettings)
{
    ui->setupUi(this);
    // Connect to the SQLite database
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName("cameras.db"); // Assuming the SQLite database file is named cameras.db
    if (!db.open()) {
        qDebug() << "Error: Failed to open database";
    }

    model = new QSqlTableModel(this);
    update_table();


    ui->tableitem_edit->setEnabled(false);
    ui->tableitem_delete->setEnabled(false);
    connect(ui->connectedcameras_tableView, &QTableView::clicked, this, &CameraSettings::on_connectedcameras_tableView_clicked);
}

CameraSettings::~CameraSettings()
{
    delete ui;
}

void CameraSettings::update_table()
{
    model->setTable("cameradetails");

    // Set the model for the table view
    ui->connectedcameras_tableView->setModel(model);

    // Populate the model with data from the database
    model->select();
}

void CameraSettings::on_addbutton_clicked()
{
    QString name_camera = ui->camera_name->text();
    QString url_camera = ui->url_address->text();
    QString port = ui->port->text();
    QString ip_address = ui->ip_address->text();
    QString username = ui->username->text();
    QString password = ui->password->text();

    qDebug() << name_camera;
    qDebug() << url_camera;

    // Check if camera_name already exists in the database
    QSqlQuery checkQuery;
    checkQuery.prepare("SELECT COUNT(*) FROM cameradetails WHERE camera_name = :name");
    checkQuery.bindValue(":name", name_camera);
    if (checkQuery.exec() && checkQuery.next()) {
        int count = checkQuery.value(0).toInt();
        if (count > 0) {

            QMessageBox::StandardButton reply;
            reply = QMessageBox::question(this, "Update Camera", "The camera already exists. Do you want to update it?", QMessageBox::Yes|QMessageBox::No);
            if (reply == QMessageBox::Yes) {
                // User wants to update the camera, proceed with the update
                QSqlQuery updateQuery;
                updateQuery.prepare("UPDATE cameradetails SET camera_url = :url, port = :port, ip_address = :ip_address, username = :username, password = :password WHERE camera_name = :name");
                updateQuery.bindValue(":url", url_camera);
                updateQuery.bindValue(":port", port);
                updateQuery.bindValue(":ip_address", ip_address);
                updateQuery.bindValue(":username", username);
                updateQuery.bindValue(":password", password);
                updateQuery.bindValue(":name", name_camera);

                if (!updateQuery.exec()) {
                    qDebug() << "Error executing update query:" << updateQuery.lastError().text();
                    QMessageBox::critical(this, "Error", "Error executing update query: " + updateQuery.lastError().text());
                } else {
                    qDebug() << "Updated";
                    update_table();
                }
            }

        }

        else {
            // Camera doesn't exist, insert new row
            QSqlQuery insertQuery;
            insertQuery.prepare("INSERT INTO cameradetails(camera_name, camera_url, port, ip_address, username, password) VALUES (:name, :url, :port, :ip_address, :username, :password)");
            insertQuery.bindValue(":name", name_camera);
            insertQuery.bindValue(":url", url_camera);
            insertQuery.bindValue(":port", port);
            insertQuery.bindValue(":ip_address", ip_address);
            insertQuery.bindValue(":username", username);
            insertQuery.bindValue(":password", password);

            if (!insertQuery.exec()) {
                qDebug() << "Error executing insert query:" << insertQuery.lastError().text();
                QMessageBox::critical(this, "Error", "Error executing insert query: " + insertQuery.lastError().text());
            } else {
                qDebug() << "Added";
                emit add_camera({url_camera, name_camera});
                update_table();
            }
        }
    }

    else {
        qDebug() << "Error executing check query:" << checkQuery.lastError().text();
        QMessageBox::critical(this, "Error", "Error executing check query: " + checkQuery.lastError().text());
    }
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

void CameraSettings::on_connectedcameras_tableView_clicked(const QModelIndex &index)
{
    if (index.isValid())
    {
        ui->tableitem_edit->setEnabled(true);
        ui->tableitem_delete->setEnabled(true);

    }
    else
    {
        ui->tableitem_edit->setEnabled(false);
        ui->tableitem_delete->setEnabled(false);
    }
}


void CameraSettings::on_tableitem_edit_clicked()
{
    QModelIndexList selectedIndexes = ui->connectedcameras_tableView->selectionModel()->selectedIndexes();
    if (!selectedIndexes.isEmpty()) {
        // Find the index of the camera_name column
        int columnIndex = 0; // Assuming camera_name is in the first column
        QModelIndex cameraNameIndex = selectedIndexes.at(columnIndex);

        // Retrieve the camera_name of the selected row
        QString cameraName = model->data(cameraNameIndex).toString();

        // Find the index of the camera_name column for all rows
        QModelIndexList allCameraNameIndexes = model->match(model->index(0, columnIndex), Qt::DisplayRole, cameraName);
        if (!allCameraNameIndexes.isEmpty()) {
            // Retrieve the details for the selected row based on the camera_name
            QModelIndex firstCameraNameIndex = allCameraNameIndexes.first();
            int selectedRow = firstCameraNameIndex.row();
            QString cameraUrl = model->data(model->index(selectedRow, 1)).toString();
            QString port = model->data(model->index(selectedRow, 2)).toString();
            QString ipAddress = model->data(model->index(selectedRow, 3)).toString();
            QString username = model->data(model->index(selectedRow, 4)).toString();
            QString password = model->data(model->index(selectedRow, 5)).toString();

            // Set the values to the textboxes
            ui->camera_name->setText(cameraName);
            ui->url_address->setText(cameraUrl);
            ui->port->setText(port);
            ui->ip_address->setText(ipAddress);
            ui->username->setText(username);
            ui->password->setText(password);

            // Disable the "Edit" button
            ui->tableitem_edit->setEnabled(false);
            // Disable the "Delete" button
            ui->tableitem_delete->setEnabled(false);
        }
    }
}


void CameraSettings::on_tableitem_delete_clicked()
{
    // Get the selected row index
    QModelIndexList selectedRows = ui->connectedcameras_tableView->selectionModel()->selectedRows();
    if (!selectedRows.isEmpty()) {
        QModelIndex index = selectedRows.at(0); // Assuming only one row can be selected at a time

        // Get the camera_name of the selected row
        QString cameraName = model->data(model->index(index.row(), 0)).toString();

        // Prepare and execute the SQL query to delete the row
        QSqlQuery deleteQuery;
        deleteQuery.prepare("DELETE FROM cameradetails WHERE camera_name = :name");
        deleteQuery.bindValue(":name", cameraName);

        if (!deleteQuery.exec()) {
            qDebug() << "Error executing delete query:" << deleteQuery.lastError().text();
            QMessageBox::critical(this, "Error", "Error executing delete query: " + deleteQuery.lastError().text());
        } else {
            qDebug() << "Deleted";
            update_table(); // Refresh the table view after deletion
            emit delete_camera(cameraName);

            // Disable the "Edit" button
            ui->tableitem_edit->setEnabled(false);
            // Disable the "Delete" button
            ui->tableitem_delete->setEnabled(false);
        }
    }
    else {
        qDebug() << "No row selected to delete";
    }
}
