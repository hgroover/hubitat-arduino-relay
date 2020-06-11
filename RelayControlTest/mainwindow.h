#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QUdpSocket>
#include <QCheckBox>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void Init();
    void on_datagramReceived();
    void sendState();

    void on_btnQuit_clicked();

    void on_chkRelay1_clicked(bool checked);

    void on_chkRelay2_clicked(bool checked);

    void on_chkRelay3_clicked(bool checked);

    void on_chkRelay4_clicked(bool checked);

    void on_btn250ms_clicked();

    void on_btn500ms_clicked();

    void on_btn250ms_2_clicked();

    void on_btn250ms_3_clicked();

    void on_btn250ms_4_clicked();

    void on_btn500ms_2_clicked();

    void on_btn500ms_3_clicked();

    void on_btn500ms_4_clicked();

private:
    Ui::MainWindow *ui;
    QUdpSocket m_udpListener;
    int m_relayValue[4];
    int m_requestSerial;

    void signalRelay(QCheckBox *chk, int relayIndex, int durationValue);
    void resetRelayValue(QCheckBox *chk, int relayIndex);
};

#endif // MAINWINDOW_H
