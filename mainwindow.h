#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QtSerialPort/qserialportinfo.h>
#include <QtCharts>
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

    void on_button_clear_graph_clicked();

    void on_button_save_graph_clicked();

    void on_checkbox_cpm_checkStateChanged(const Qt::CheckState &state);

    void on_checkbox_avg_cpm_checkStateChanged(const Qt::CheckState &state);

    void on_checkbox_usv_checkStateChanged(const Qt::CheckState &state);

    void on_checkbox_avg_usv_checkStateChanged(const Qt::CheckState &state);

private:
    Ui::MainWindow *ui;
    QSerialPort *port;
    bool connected;
    bool done_reading;
    QString serial_data;
    QString serial_separator;
    QString serial_format;
    QLineSeries *cpm_series;
    QScatterSeries *avg_cpm_series;
    QLineSeries *usv_series;
    QScatterSeries *avg_usv_series;
    QChart *chart;
    QValueAxis *axis_time;
    QValueAxis *axis_cpm;
    QValueAxis *axis_usv;
    QColor yellow;
    QColor red;
    QColor blue;
    QColor green;

    void get_ports();
    void toggle_ui_elements();
    void serial_send_data();
    void distribute_table_width();
    void output_add_row();
    void output_make_readonly_centered(int row);
    bool output_valid_format();
    void graph_setup();
    void graph_plot();
    void channels_set_style();
};
#endif // MAINWINDOW_H
