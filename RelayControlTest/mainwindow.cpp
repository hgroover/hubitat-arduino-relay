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

}

void MainWindow::on_chkRelay2_clicked(bool checked)
{

}

void MainWindow::on_chkRelay3_clicked(bool checked)
{

}

void MainWindow::on_chkRelay4_clicked(bool checked)
{

}
