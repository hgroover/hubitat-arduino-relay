#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QDebug>
#include <QUdpSocket>
#include <QByteArray>

// Receiving UDP datagram broadcast on Windows requires running this program as administrator
// FIXME find Windows Firewall setting to allow this without running as admin
void MainWindow::on_datagramReceived()
{
    int pendingCount = 0;
    while (m_udpListener.hasPendingDatagrams())
    {
        QByteArray ba;
        ba.resize(static_cast<int>(m_udpListener.pendingDatagramSize()));
        m_udpListener.readDatagram(ba.data(), ba.size());
        //qDebug().noquote() << "udp:" << ba.toHex();
        ui->statusBar->showMessage(ba.toHex(), 5000);
        // Now crack the datagram:
        // sig 0x64 0x65 0x66 0x67
        // relay states 1-4
        // ipv4 address
        // [MAC address]
        unsigned const char *raw = reinterpret_cast<const unsigned char *>(ba.constData());
        if ((ba.size() > 18) && raw[0] == '\x64' && raw[1] == '\x65' && raw[2]== '\x66' && raw[3] == '\x67')
        {
            ui->txtDeviceAddress->setText(QString().sprintf("%u.%u.%u.%u", raw[8], raw[9], raw[10], raw[11]));
            if (ba.size() < 18)
            {
                ui->txtDeviceMAC->setText("");
            }
            else
            {
                ui->txtDeviceMAC->setText(QString().sprintf("%02x:%02x:%02x:%02x:%02x:%02x", raw[12], raw[13], raw[14], raw[15], raw[16], raw[17]));
            }
            ui->chkRelay1->setChecked(raw[4] == 1);
            ui->chkRelay2->setChecked(raw[5] == 1);
            ui->chkRelay3->setChecked(raw[6] == 1);
            ui->chkRelay4->setChecked(raw[7] == 1);
            ui->statusBar->showMessage(QString().sprintf("Ver: %s", &raw[18]), 4000);
        }
        else
        {
            ui->statusBar->showMessage("Unknown: " + ba.toHex(), 6000);
        }
        pendingCount++;
    }
    if (pendingCount != 1)
    {
        qDebug() << pendingCount << "datagrams received";
    }
}
