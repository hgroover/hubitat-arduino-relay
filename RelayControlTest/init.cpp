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
        // Send initial query via UDP
        QString sQuery("hgr-ardr-qry");
        m_udpListener.writeDatagram(sQuery.toLocal8Bit(),QHostAddress("255.255.255.255"), 8981);
        // This won't help at all, since we don't have an IP address until we get the first
        // broadcast, which will also have the state values...
        //QTimer::singleShot(0, this, SLOT(queryState()));
    }
    // Binding on QHostAddress::AnyIPv4 does not capture SSDP traffic.
    // We need the shared address option with a local port assignment, then join the specific multicast group
    if (!m_ssdpListener.bind(QHostAddress::AnyIPv4 /*QHostAddress("239.255.255.250")*/, 1900, QUdpSocket::ReuseAddressHint | QUdpSocket::ShareAddress))
    {
        ui->statusBar->showMessage("Failed to bind port 1900 (SSDP)", 10000);
    }
    else
    {
        qDebug() << "Bound SSDP listener, joining multicast group";
        if (!m_ssdpListener.joinMulticastGroup(QHostAddress("239.255.255.250")))
        {
            qWarning() << "Failed to join multicast group";
        }
        m_ssdpListener.setSocketOption(QAbstractSocket::MulticastTtlOption, 4);
        ui->statusBar->showMessage("Listening to SSDP on port 1900", 5000);
        QString s;
        s = "SSDP request M-SEARCH * HTTP/1.1\r\n\
HOST: 239.255.255.250:1900\r\n\
MAN: \"ssdp:discover\"\r\n\
MX: 1\r\n\
ST: urn:schemas-sony-com:service:ScalarWebAPI:1\r\n\r\n";
        int bytesWritten = m_ssdpListener.writeDatagram(s.toLocal8Bit(), QHostAddress("239.255.255.250"), 1900);
        qDebug() << "Initial query bytes:" << bytesWritten;
        connect( &m_ssdpListener, SIGNAL(readyRead()), this, SLOT(on_ssdpReceived()) );
    }
}
