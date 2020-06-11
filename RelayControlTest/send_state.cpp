#include  "mainwindow.h"
#include "ui_mainwindow.h"

#include <QDebug>
#include <QNetworkAddressEntry>

void MainWindow::sendState()
{
    // Do we have address?
    QString addr(ui->txtDeviceAddress->text());
    if (addr.isEmpty())
    {
        ui->statusBar->showMessage("IP address not received yet", 6000);
        return;
    }
}
