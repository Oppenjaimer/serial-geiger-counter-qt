#include <QtSerialPort/QSerialPortInfo>
#include <QtSerialPort/QSerialPort>

#include <QtCharts>

#include <QFileDialog>
#include <QMessageBox>
#include <QScrollBar>

#include "mainwindow.h"
#include "ui_mainwindow.h"

/* ========== CONSTRUCTOR/DESTRUCTOR ========= */

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

    cpm_series = new QLineSeries();
    avg_cpm_series = new QScatterSeries();
    usv_series = new QLineSeries();
    avg_usv_series = new QScatterSeries();
    chart = new QChart();
    axis_time = new QValueAxis();
    axis_cpm = new QValueAxis();
    axis_usv = new QValueAxis();

    yellow = QColor(0xfa, 0xbd, 0x2f);
    red = QColor(0xfb, 0x49, 0x34);
    blue = QColor(0x45, 0x85, 0x88);
    green = QColor(0x98, 0x97, 0x1a);

    graph_setup();
    channels_set_style();
}

MainWindow::~MainWindow()
{
    delete ui;
}

/* ========== METHODS ========= */

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
        ui->text_output->moveCursor(QTextCursor::End);
        ui->button_save->setEnabled(true);

        if (output_valid_format()) {
            output_add_row();
            graph_plot();
        }

        serial_data = "";
        done_reading = false;
    }
}

void MainWindow::distribute_table_width()
{
    ui->table_output->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
}

void MainWindow::graph_setup()
{
    chart->legend()->hide();
    chart->setTitle("Radiation Data");
    QFont chart_font;
    chart_font.setBold(true);
    chart->setTitleFont(chart_font);

    axis_time->setTitleText("Time (s)");
    QFont axis_font;
    axis_font.setBold(false);
    axis_time->setTitleFont(axis_font);
    chart->addAxis(axis_time, Qt::AlignBottom);

    chart->addSeries(cpm_series);
    chart->addSeries(avg_cpm_series);

    axis_cpm->setTitleText("Count rate (CPM)");
    axis_cpm->setTitleFont(axis_font);
    chart->addAxis(axis_cpm, Qt::AlignLeft);

    cpm_series->setPointsVisible(true);
    cpm_series->setColor(yellow);
    cpm_series->attachAxis(axis_time);
    cpm_series->attachAxis(axis_cpm);

    avg_cpm_series->setMarkerSize(8);
    avg_cpm_series->setMarkerShape(QScatterSeries::MarkerShapeRectangle);
    avg_cpm_series->setColor(red);
    avg_cpm_series->attachAxis(axis_time);
    avg_cpm_series->attachAxis(axis_cpm);

    chart->addSeries(usv_series);
    chart->addSeries(avg_usv_series);

    axis_usv->setTitleText("Dose rate (µSv/h)");
    axis_usv->setTitleFont(axis_font);
    chart->addAxis(axis_usv, Qt::AlignRight);

    usv_series->setPointsVisible(true);
    usv_series->setColor(blue);
    usv_series->attachAxis(axis_time);
    usv_series->attachAxis(axis_usv);

    avg_usv_series->setMarkerSize(8);
    avg_usv_series->setMarkerShape(QScatterSeries::MarkerShapeRectangle);
    avg_usv_series->setColor(green);
    avg_usv_series->attachAxis(axis_time);
    avg_usv_series->attachAxis(axis_usv);

    ui->graph->setRenderHint(QPainter::Antialiasing);
    ui->graph->setChart(chart);
}

void MainWindow::graph_plot()
{
    QList<QString> split = serial_data.split(serial_separator.trimmed());
    int time = split.at(0).toInt();
    int cpm = split.at(1).toInt();
    double avg_cpm = split.at(2).toDouble();
    double usv = split.at(3).toDouble();
    double avg_usv = split.at(4).toDouble();

    cpm_series->append(time, cpm);
    avg_cpm_series->append(time, avg_cpm);
    usv_series->append(time, usv);
    avg_usv_series->append(time, avg_usv);

    if (cpm_series->count() > 10) {
        cpm_series->removePoints(0, cpm_series->count() - 10);
    } else {
        axis_time->setTickCount(cpm_series->count());
    }

    int min_time = cpm_series->at(0).x();
    int max_time = cpm_series->at(cpm_series->count() - 1).x();
    axis_time->setRange(min_time, max_time);

    int max_cpm = 0;
    for (QPointF point : cpm_series->points()) {

        if (point.y() > max_cpm) max_cpm = point.y();
    }
    axis_cpm->setRange(0, max_cpm + 10);

    int max_usv = 0;
    for (QPointF point : usv_series->points()) {
        if (point.y() > max_usv) max_usv = point.y();
    }
    axis_usv->setRange(0, max_usv + 1);

    ui->graph->repaint();
    ui->button_save_graph->setEnabled(true);
}

void MainWindow::channels_set_style()
{
    ui->checkbox_cpm->setStyleSheet(
        "QCheckBox {"
        "   border-radius: 4px;"
        "}"
        "QCheckBox::indicator::checked {"
        "   background: #fabd2f;"
        "   border-radius: 4px;"
        "}"
    );

    ui->checkbox_avg_cpm->setStyleSheet(
        "QCheckBox {"
        "   border-radius: 4px;"
        "}"
        "QCheckBox::indicator::checked {"
        "   background: #fb4934;"
        "   border-radius: 4px;"
        "}"
    );

    ui->checkbox_usv->setStyleSheet(
        "QCheckBox {"
        "   border-radius: 4px;"
        "}"
        "QCheckBox::indicator::checked {"
        "   background: #458588;"
        "   border-radius: 4px;"
        "}"
    );

    ui->checkbox_avg_usv->setStyleSheet(
        "QCheckBox {"
        "   border-radius: 4px;"
        "}"
        "QCheckBox::indicator::checked {"
        "   background: #98971a;"
        "   border-radius: 4px;"
        "}"
    );
}

/* ========== UI EVENTS ========= */

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

void MainWindow::on_button_clear_graph_clicked()
{
    cpm_series->clear();
    avg_cpm_series->clear();
    usv_series->clear();
    avg_usv_series->clear();

    ui->button_save_graph->setEnabled(false);
}

void MainWindow::on_button_save_graph_clicked()
{
    QPixmap pixmap = ui->graph->grab();

    QString filename = QFileDialog::getSaveFileName(this, "Save Image", "", "PNG Files (*.png);;JPG Files (*.jpg);;All Files (*)");
    if (filename.isEmpty()) return;

    if (pixmap.save(filename)) {
        QMessageBox::information(this, "Success", QString("Image '%1' saved successfully").arg(filename));
    } else {
        QMessageBox::critical(this, "Error", "Unable to save image");
    }
}

void MainWindow::on_checkbox_cpm_checkStateChanged(const Qt::CheckState &state)
{
    cpm_series->setVisible(state == Qt::Unchecked ? false : true);
}

void MainWindow::on_checkbox_avg_cpm_checkStateChanged(const Qt::CheckState &state)
{
    avg_cpm_series->setVisible(state == Qt::Unchecked ? false : true);
}

void MainWindow::on_checkbox_usv_checkStateChanged(const Qt::CheckState &state)
{
    usv_series->setVisible(state == Qt::Unchecked ? false : true);
}

void MainWindow::on_checkbox_avg_usv_checkStateChanged(const Qt::CheckState &state)
{
    avg_usv_series->setVisible(state == Qt::Unchecked ? false : true);
}

