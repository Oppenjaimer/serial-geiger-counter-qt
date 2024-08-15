#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QtSerialPort/qserialportinfo.h>
#include <QMainWindow>

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void on_list_ports_itemSelectionChanged();

    void on_button_refresh_clicked();

    void on_button_connect_clicked();

    void on_button_disconnect_clicked();

    void on_button_send_clicked();

    void on_button_clear_clicked();

    void on_text_input_returnPressed();

    void serial_read_data();

    void on_button_clear_formatted_clicked();

    void on_button_save_clicked();

    void on_button_export_clicked();

private:
    Ui::MainWindow *ui;
    QSerialPort *port;
    bool connected;
    bool done_reading;
    QString serial_data;
    QString serial_separator;
    QString serial_format;

    void get_ports();
    void toggle_ui_elements();
    void serial_send_data();
    void distribute_table_width();
    void output_add_row();
    void output_make_readonly_centered(int row);
    bool output_valid_format();
};
#endif // MAINWINDOW_H
