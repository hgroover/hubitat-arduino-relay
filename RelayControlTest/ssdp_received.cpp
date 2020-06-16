#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QDebug>
#include <QNetworkDatagram>

void MainWindow::on_ssdpReceived()
{
    int pendingCount = 0;
    while (m_ssdpListener.hasPendingDatagrams())
    {
        QByteArray ba;
        ba.resize(static_cast<int>(m_ssdpListener.pendingDatagramSize()));
        m_ssdpListener.readDatagram(ba.data(), ba.size());
        // Now this is working and we're getting everything
        //qDebug().noquote() << "udp:" << ba;
        //ui->statusBar->showMessage(ba, 5000);
        pendingCount++;
    }
    // By nature SSDP will send in clumps of 3 or 4. Only report if we have an unusually long cluster
    if (pendingCount > 4)
    {
        qDebug() << pendingCount << "ssdp messages received";
    }

}
