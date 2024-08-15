#include <QtSerialPort/QSerialPortInfo>
#include <QtSerialPort/QSerialPort>

#include <QFileDialog>
#include <QMessageBox>
#include <QScrollBar>

#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    port = new QSerialPort(this);

    setWindowTitle("Serial Geiger Counter");
    get_ports();
    connected = false;
    done_reading = false;
    serial_data = "";
    serial_separator = " - ";
    serial_format = QString("\\d+(%1\\d+\\.?\\d*){4}").arg(serial_separator);

    connect(port, SIGNAL(readyRead()), this, SLOT(serial_read_data()));

    distribute_table_width();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::get_ports()
{
    const QList<QSerialPortInfo> ports = QSerialPortInfo::availablePorts();
    for (const QSerialPortInfo &port : ports) {
        QListWidgetItem *item = new QListWidgetItem(port.portName(), ui->list_ports);
        ui->list_ports->addItem(item);
    }
}

void MainWindow::toggle_ui_elements()
{
    ui->button_connect->setEnabled(!connected);
    ui->button_disconnect->setEnabled(connected);
    ui->text_input->setEnabled(connected);
    ui->button_send->setEnabled(connected);
}

void MainWindow::serial_send_data() {
    if (!port->isOpen()) return;
    if (ui->text_input->text().isEmpty()) return;

    port->write(ui->text_input->text().toLatin1());

    ui->text_input->clear();
}

void MainWindow::output_add_row()
{
    int row = ui->table_output->rowCount();
    ui->table_output->insertRow(row);

    QList<QString> split = serial_data.split(serial_separator.trimmed());
    for (int col = 0; col < 5; ++col) {
        ui->table_output->setItem(row, col, new QTableWidgetItem(split.at(col).trimmed()));
    }

    output_make_readonly_centered(row);

    ui->table_output->scrollToItem(ui->table_output->item(row, 0), QAbstractItemView::PositionAtTop);
    ui->table_output->selectRow(row);
    ui->table_output->clearSelection();

    ui->button_export->setEnabled(true);
}

void MainWindow::output_make_readonly_centered(int row)
{
    for (int col = 0; col < ui->table_output->columnCount(); ++col) {
        QTableWidgetItem *item = ui->table_output->item(row, col);
        if (item) {
            item->setFlags(item->flags() & ~Qt::ItemIsEditable);
            item->setTextAlignment(Qt::AlignCenter);
        }
    }
}

bool MainWindow::output_valid_format()
{
    QRegularExpression regex(serial_format);
    QRegularExpressionMatch match = regex.match(serial_data.trimmed());

    return match.hasMatch();
}

void MainWindow::serial_read_data()
{
    if (!port->isOpen()) return;

    while (port->bytesAvailable()) {
        serial_data += port->readAll();
        if (serial_data.at(serial_data.length() - 1) == '\n') {
            done_reading = true;
        }
    }

    if (done_reading) {
        ui->text_output->insertPlainText(serial_data);
        ui->button_save->setEnabled(true);

        if (output_valid_format()) output_add_row();

        serial_data = "";
        done_reading = false;
    }
}

void MainWindow::distribute_table_width()
{
    ui->table_output->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
}

void MainWindow::on_list_ports_itemSelectionChanged()
{
    if (ui->list_ports->selectedItems().isEmpty()) {
        ui->button_connect->setEnabled(false);
    } else {
        if (!connected) ui->button_connect->setEnabled(true);
    }
}

void MainWindow::on_button_refresh_clicked()
{
    ui->list_ports->clear();
    get_ports();
}

void MainWindow::on_button_connect_clicked()
{
    if (port->isOpen()) {
        QMessageBox::critical(this, "Error", "Port is already open", QMessageBox::Close);
        return;
    }

    port->setPortName(ui->list_ports->selectedItems().at(0)->text());
    port->setBaudRate(QSerialPort::BaudRate::Baud9600);
    port->setParity(QSerialPort::Parity::NoParity);
    port->setDataBits(QSerialPort::DataBits::Data8);
    port->setStopBits(QSerialPort::StopBits::OneStop);
    port->setFlowControl(QSerialPort::FlowControl::NoFlowControl);
    port->open(QIODevice::ReadWrite);

    if (!port->isOpen()) {
        QMessageBox::critical(this, "Error", "Unable to connect to port", QMessageBox::Close);
        return;
    }

    connected = true;
    toggle_ui_elements();
}

void MainWindow::on_button_disconnect_clicked()
{
    if (!port->isOpen()) {
        QMessageBox::critical(this, "Error", "Port is not open", QMessageBox::Close);
        return;
    }

    port->close();

    connected = false;
    toggle_ui_elements();
    if (ui->list_ports->selectedItems().isEmpty()) ui->button_connect->setEnabled(false);
}

void MainWindow::on_button_send_clicked()
{
    serial_send_data();
}

void MainWindow::on_text_input_returnPressed()
{
    serial_send_data();
}

void MainWindow::on_button_clear_clicked()
{
    ui->text_output->clear();
    ui->button_save->setEnabled(false);
}

void MainWindow::on_button_clear_formatted_clicked()
{
    ui->table_output->clearContents();
    ui->table_output->setRowCount(0);
    ui->button_export->setEnabled(false);
}


void MainWindow::on_button_save_clicked()
{
    QString filename = QFileDialog::getSaveFileName(this, "Save File", "", "Text Files (*.txt);;All Files (*)");
    if (filename.isEmpty()) return;

    QFile file(filename);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
        QTextStream out(&file);
        out << ui->text_output->toPlainText();
        file.close();
        QMessageBox::information(this, "Success", QString("File '%1' saved successfully").arg(filename));
    } else {
        QMessageBox::critical(this, "Error", "Unable to save file");
    }
}


void MainWindow::on_button_export_clicked()
{
    QString filename = QFileDialog::getSaveFileName(this, "Export File", "", "CSV Files (*.csv);;All Files (*)");
    if (filename.isEmpty()) return;

    QFile file(filename);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
        QTextStream out(&file);

        QList<QString> headers;
        for (int i = 0; i < ui->table_output->columnCount(); ++i) {
            headers << ui->table_output->horizontalHeaderItem(i)->data(Qt::DisplayRole).toString();
        }
        out << headers.join(",") << "\n";

        for (int row = 0; row < ui->table_output->rowCount(); ++row) {
            QList<QString> row_data;
            for (int col = 0; col < ui->table_output->columnCount(); ++col) {
                QTableWidgetItem *item = ui->table_output->item(row, col);
                if (item) row_data << item->text();
                else row_data << "";
            }
            out << row_data.join(",") << "\n";
        }

        file.close();
        QMessageBox::information(this, "Success", QString("File '%1' exported successfully").arg(filename));
    } else {
        QMessageBox::critical(this, "Error", "Unable to export file");
    }
}

