#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QDebug>
#include <QTimer>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    m_udpListener(this)
{
    ui->setupUi(this);
    ui->mainToolBar->hide();
    ui->menuBar->hide();
    m_relayValue[0] = 1;
    m_relayValue[1] = 1;
    m_relayValue[2] = 1;
    m_relayValue[3] = 1;
    m_requestSerial = qrand() % 1000; // Get value from 0 to 999
    QTimer::singleShot(0, this, SLOT(Init()));
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_btnQuit_clicked()
{
    close();
}

void MainWindow::on_chkRelay1_clicked(bool checked)
{
    qDebug() << "relay1" << checked;
    QTimer::singleShot(0, this, SLOT(sendState()));
}

void MainWindow::on_chkRelay2_clicked(bool checked)
{
    qDebug() << "relay2" << checked;
    QTimer::singleShot(0, this, SLOT(sendState()));
}

void MainWindow::on_chkRelay3_clicked(bool checked)
{
    qDebug() << "relay3" << checked;
    QTimer::singleShot(0, this, SLOT(sendState()));
}

void MainWindow::on_chkRelay4_clicked(bool checked)
{
    qDebug() << "relay4" << checked;
    QTimer::singleShot(0, this, SLOT(sendState()));
}

void MainWindow::signalRelay(QCheckBox *chk, int relayIndex, int durationValue)
{
    qDebug() << "relay" << (relayIndex + 1) << durationValue;
    if (relayIndex < 0 || relayIndex >= 4)
    {
        qWarning() << "Invalid index";
        return;
    }
    m_relayValue[relayIndex] = durationValue;
    chk->setChecked(true);
    QTimer::singleShot(0, this, SLOT(sendState()));
}

void MainWindow::on_btn250ms_clicked()
{
    signalRelay(ui->chkRelay1, 0, 2);
}

void MainWindow::on_btn500ms_clicked()
{
    signalRelay(ui->chkRelay1, 0, 5);
}

void MainWindow::on_btn250ms_2_clicked()
{
    signalRelay(ui->chkRelay2, 1, 2);
}

void MainWindow::on_btn250ms_3_clicked()
{
    signalRelay(ui->chkRelay3, 2, 2);
}

void MainWindow::on_btn250ms_4_clicked()
{
    signalRelay(ui->chkRelay4, 3, 2);
}

void MainWindow::on_btn500ms_2_clicked()
{
    signalRelay(ui->chkRelay2, 1, 5);
}

void MainWindow::on_btn500ms_3_clicked()
{
    signalRelay(ui->chkRelay3, 2, 5);
}

void MainWindow::on_btn500ms_4_clicked()
{
    signalRelay(ui->chkRelay4, 3, 5);
}
