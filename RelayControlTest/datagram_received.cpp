#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QDebug>
#include <QUdpSocket>
#include <QByteArray>
#include <QTimer>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>

// Receiving UDP datagram broadcast on Windows requires running this program as administrator
// FIXME find Windows Firewall setting to allow this without running as admin
void MainWindow::on_datagramReceived()
{
    int pendingCount = 0;
    bool wasEmpty = ui->txtDeviceAddress->text().isEmpty();
    while (m_udpListener.hasPendingDatagrams())
    {
        QByteArray ba;
        ba.resize(static_cast<int>(m_udpListener.pendingDatagramSize()));
        m_udpListener.readDatagram(ba.data(), ba.size());
        // Only JSON supported - ignore all broadcasts. Note that ts= is supported only via TCP
        if (ba == "hgr-ardr-qry" || ba.startsWith("rn="))
        {
            qDebug() << "Ignoring broadcast echo";
        }
        else if (ba.startsWith("{"))
        {
            //qWarning().noquote() << "JSON: " << ba;
            QJsonDocument doc;
            QJsonParseError pe;
            doc = QJsonDocument::fromJson(ba, &pe);
            if (doc.isEmpty())
            {
                qWarning().noquote() << "parsed empty:" << pe.errorString();
            }
            else
            {
                //qDebug().noquote() << "parsed empty:" << doc.isEmpty() << doc.object();
                if (doc.object().contains("ip4"))
                {
                    ui->txtDeviceAddress->setText(doc.object()["ip4"].toString());
                    if (wasEmpty) QTimer::singleShot(250, this, SLOT(queryState()));
                }
                if (doc.object().contains("hgr-ardr"))
                {
                    if (doc.object()["hgr-ardr"].toString() != m_deviceVer)
                    {
                        m_deviceVer = doc.object()["hgr-ardr"].toString();
                        qDebug().noquote() << "ver:" << m_deviceVer;
                        ui->statusBar->showMessage("Device version: " + m_deviceVer, 8000);
                    }
                }
                ui->chkRelay1->setChecked(doc.object()["r1"].toInt() == 1);
                ui->chkRelay2->setChecked(doc.object()["r2"].toInt() == 1);
                ui->chkRelay3->setChecked(doc.object()["r3"].toInt() == 1);
                ui->chkRelay4->setChecked(doc.object()["r4"].toInt() == 1);
            }
        }
        else
        {
            ui->statusBar->showMessage("Unknown: " + ba.toHex(), 6000);
            qWarning().noquote() << "Unknown datagram: " << ba.toHex();
        }
        pendingCount++;
    }
    if (pendingCount != 1)
    {
        qDebug() << pendingCount << "datagrams received";
    }
}
