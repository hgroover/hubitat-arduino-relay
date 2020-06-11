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
    m_relay1Value = 1;
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

void MainWindow::on_btn250ms_clicked()
{
    m_relay1Value = 2;
    qDebug() << "relay1" << m_relay1Value;
    ui->chkRelay1->setChecked(true);
    QTimer::singleShot(0, this, SLOT(sendState()));
}

void MainWindow::on_btn500ms_clicked()
{
    m_relay1Value = 5;
    qDebug() << "relay1" << m_relay1Value;
    ui->chkRelay1->setChecked(true);
    QTimer::singleShot(0, this, SLOT(sendState()));
}
