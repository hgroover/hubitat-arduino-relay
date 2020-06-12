#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QDebug>
#include <QObject>
#include <QNetworkDatagram>
#include <QNetworkInterface>
#include <QHostAddress>
#include <QAbstractSocket>
#include <QTimer>

void MainWindow::Init()
{
    int addrCount = 0;
    foreach (QHostAddress addr, QNetworkInterface::allAddresses())
    {
        if (addr.isLoopback()) continue;
        // To select only IPv4 addresses, take only those with a blank scopeId
        if (!addr.scopeId().isEmpty()) continue;
        qDebug().noquote() << addr.toString() << "protocol" << addr.protocol();
        addrCount++;
    }
    qDebug() << addrCount <<  "addresses found";
    if (!m_udpListener.bind(QHostAddress::AnyIPv4, 8981))
    {
        ui->statusBar->showMessage("Failed to bind port 8981", 10000);
    }
    else
    {
        ui->statusBar->showMessage("Listening on port 8981", 5000);
        qDebug() << "Connecting listener";
        connect( &m_udpListener, SIGNAL(readyRead()), this, SLOT(on_datagramReceived()) );
        // This won't help at all, since we don't have an IP address until we get the first
        // broadcast, which will also have the state values...
        //QTimer::singleShot(0, this, SLOT(queryState()));
    }
}
