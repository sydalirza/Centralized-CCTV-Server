#include "camerasettings.h"
#include "ui_camerasettings.h"
#include <QtSql/QSqlDatabase>
#include <QtSql/QSqlQuery>
#include <QtSql/QSqlError>
#include <QtSql/QSqlTableModel>
#include <QMessageBox>
#include <QDesktopServices>

CameraSettings::CameraSettings(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::CameraSettings)
{
    ui->setupUi(this);
    // Connect to the SQLite database
    db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName("cameras.db"); // Assuming the SQLite database file is named cameras.db
    if (!db.open()) {
        qDebug() << "Error: Failed to open database";
    }

    model = new QSqlTableModel(this);
    logModel = new QSqlTableModel(this);

    update_table();
    update_log_table();

    ui->tableitem_edit->setEnabled(false);
    ui->tableitem_delete->setEnabled(false);
    connect(ui->connectedcameras_tableView, &QTableView::clicked, this, &CameraSettings::on_connectedcameras_tableView_clicked);
    connect(ui->logs_tableView, &QTableView::doubleClicked, this, &CameraSettings::openFile);
}

CameraSettings::~CameraSettings()
{
    db.close();
    delete ui;
}

void CameraSettings::update_table()
{
    model->setTable("cameradetails");

    // Set header labels for the table view
    model->setHeaderData(0, Qt::Horizontal, "Camera Name");
    model->setHeaderData(1, Qt::Horizontal, "Camera URL");
    model->setHeaderData(2, Qt::Horizontal, "Port");
    model->setHeaderData(3, Qt::Horizontal, "IP Address");
    model->setHeaderData(4, Qt::Horizontal, "Username");
    model->setHeaderData(5, Qt::Horizontal, "Password");

    // Set the model for the table view
    ui->connectedcameras_tableView->setModel(model);

    // Populate the model with data from the database
    model->select();

    // Resize columns to contents
    ui->connectedcameras_tableView->resizeColumnsToContents();

    // Set stretch factors to make columns fill the entire table view
    for (int i = 0; i < model->columnCount(); ++i) {
        ui->connectedcameras_tableView->horizontalHeader()->setSectionResizeMode(i, QHeaderView::Stretch);
    }
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
    checkQuery.prepare("SELECT COUNT(*) FROM cameradetails WHERE camera_name = :name OR camera_url = :url");
    checkQuery.bindValue(":name", name_camera);
    checkQuery.bindValue(":url", url_camera);

    if (checkQuery.exec() && checkQuery.next()) {
        int count = checkQuery.value(0).toInt();
        if (count > 0) {

            QMessageBox::StandardButton reply;
            reply = QMessageBox::question(this, "Update Camera", "The camera already exists. Do you want to update it?", QMessageBox::Yes|QMessageBox::No);
            if (reply == QMessageBox::Yes) {
                // User wants to update the camera, proceed with the update
                QSqlQuery updateQuery;
                updateQuery.prepare("UPDATE cameradetails SET camera_name = :name, camera_url = :url, port = :port, ip_address = :ip_address, username = :username, password = :password WHERE camera_name = :name OR camera_url = :url");
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
                    emit add_camera({url_camera, name_camera});
                    update_table();
                    update_log_table();
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
                update_log_table();
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
        // Check if the selected index represents the entire row
        if (index.column() == 0) {
            // Enable the "Edit" and "Delete" buttons
            ui->tableitem_edit->setEnabled(true);
            ui->tableitem_delete->setEnabled(true);
        } else {
            // Disable the buttons if only a cell within the row is selected
            ui->tableitem_edit->setEnabled(false);
            ui->tableitem_delete->setEnabled(false);
        }
    }
    else
    {
        // No row selected, disable the buttons
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

        // Ask the user for confirmation
        QMessageBox::StandardButton reply;
        reply = QMessageBox::question(this, "Confirm Deletion", "Are you sure you want to delete this camera?",
                                      QMessageBox::Yes | QMessageBox::No);
        if (reply == QMessageBox::Yes) {
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
                populate_camera_names();
                emit delete_camera(cameraName);

                // Disable the "Edit" button
                ui->tableitem_edit->setEnabled(false);
                // Disable the "Delete" button
                ui->tableitem_delete->setEnabled(false);
            }
        }
    }
    else {
        qDebug() << "No row selected to delete";
    }
}

void CameraSettings::update_log_table()
{
    logModel->setTable("camera_logs");

    // Set header labels for the table view

    logModel->setHeaderData(0, Qt::Horizontal, "Recording ID");
    logModel->setHeaderData(1, Qt::Horizontal, "Camera Name");
    logModel->setHeaderData(2, Qt::Horizontal, "File Name");
    logModel->setHeaderData(3, Qt::Horizontal, "Start Time");
    logModel->setHeaderData(4, Qt::Horizontal, "End Time");

    // Set the model for the table view
    ui->logs_tableView->setModel(logModel);

    // Populate the model with data from the database
    logModel->select();

    // Resize columns to contents
    ui->logs_tableView->resizeColumnsToContents();

    // Set stretch factors to make columns fill the entire table view
    for (int i = 0; i < logModel->columnCount(); ++i) {
        ui->logs_tableView->horizontalHeader()->setSectionResizeMode(i, QHeaderView::Stretch);
    }

    populate_camera_names();
}

void CameraSettings::populate_camera_names()
{
    QSqlQuery query;
    query.prepare("SELECT camera_name FROM cameradetails");

    if (query.exec()) {
        ui->camera_name_combobox->clear(); // Clear existing items in the combo box

        // Add "All" entry
        ui->camera_name_combobox->addItem("All");

        // Populate with camera names
        while (query.next()) {
            QString cameraName = query.value(0).toString();
            ui->camera_name_combobox->addItem(cameraName);
        }
    } else {
        qDebug() << "Error executing query to fetch camera names:" << query.lastError().text();
    }
}


void CameraSettings::on_update_pushButton_clicked()
{
    update_log_table();
}


void CameraSettings::on_applyfilter_button_clicked()
{
    QString selectedCamera = ui->camera_name_combobox->currentText();
    QString selectedSort = ui->sort_combobox->currentText();

    // Prepare the query based on the selected camera and sort order
    QSqlQuery query;
    if (selectedCamera == "All") {
        if (selectedSort == "Newest First") {
            query.prepare("SELECT * FROM camera_logs ORDER BY id DESC");
        } else {
            query.prepare("SELECT * FROM camera_logs ORDER BY id ASC");
        }
    } else {
        if (selectedSort == "Newest First") {
            query.prepare("SELECT * FROM camera_logs WHERE camera_name = :cameraName ORDER BY id DESC");
        } else {
            query.prepare("SELECT * FROM camera_logs WHERE camera_name = :cameraName ORDER BY id ASC");
        }
        query.bindValue(":cameraName", selectedCamera);
    }

    // Execute the query and update the table view
    if (query.exec()) {
        // Set the query model to the table view
        QSqlQueryModel *queryModel = new QSqlQueryModel();
        queryModel->setQuery(query);
        queryModel->setHeaderData(0, Qt::Horizontal, "Recording ID");
        queryModel->setHeaderData(1, Qt::Horizontal, "Camera Name");
        queryModel->setHeaderData(2, Qt::Horizontal, "File Name");
        queryModel->setHeaderData(3, Qt::Horizontal, "Start Time");
        queryModel->setHeaderData(4, Qt::Horizontal, "End Time");
        ui->logs_tableView->setModel(queryModel);
    } else {
        qDebug() << "Error executing filter query:" << query.lastError().text();
    }
}

void CameraSettings::openFile(const QModelIndex &index)
{
    // Check if the clicked index is valid
    if (index.isValid()) {
        // Retrieve the file name from the selected row
        QString fileName = ui->logs_tableView->model()->data(ui->logs_tableView->model()->index(index.row(), 2)).toString();

        // Open the file with its default application
        QDesktopServices::openUrl(QUrl::fromLocalFile(fileName));
    }
}

