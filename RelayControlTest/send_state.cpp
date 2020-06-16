#include  "mainwindow.h"
#include "ui_mainwindow.h"

#include <QDebug>
#include <QNetworkAddressEntry>
#include <QTcpSocket>
#include <QJsonDocument>
#include <QJsonObject>

void MainWindow::sendState()
{
    QString sCmd(QString().sprintf("rn=%03d;a=%d;b=%d;c=%d;d=%d",
                                   m_requestSerial,
                                   ui->chkRelay1->isChecked() ? m_relayValue[0] : 0,
                                   ui->chkRelay2->isChecked() ? m_relayValue[1] : 0,
                                   ui->chkRelay3->isChecked() ? m_relayValue[2] : 0,
                                   ui->chkRelay4->isChecked() ? m_relayValue[3] : 0));
    m_requestSerial = (m_requestSerial + 1) % 1000;
    qDebug().noquote() << "sendState" << sCmd;
    // Reset relay values
    resetRelayValue(ui->chkRelay1, 0);
    resetRelayValue(ui->chkRelay2, 1);
    resetRelayValue(ui->chkRelay3, 2);
    resetRelayValue(ui->chkRelay4, 3);
    sendTcpCmd(sCmd);
}

void MainWindow::queryState()
{
    QString sCmd(QString().sprintf("rn=%03d;query", m_requestSerial));
    m_requestSerial = (m_requestSerial + 1) % 1000;
    qDebug().noquote() << "queryState" << sCmd;
    sendTcpCmd(sCmd);
}

void MainWindow::resetRelayValue(QCheckBox *chk, int relayIndex)
{
    if (relayIndex < 0 || relayIndex >= 4) return;
    if (m_relayValue[relayIndex] > 1)
    {
        m_relayValue[relayIndex] = 1;
        chk->setChecked(false);
    }
}

bool MainWindow::sendTcpCmd(QString sCmd)
{
    // Do we have address?
    QString addr(ui->txtDeviceAddress->text());
    if (addr.isEmpty())
    {
        ui->statusBar->showMessage("IP address not received yet", 6000);
        return false;
    }
    QTcpSocket sock;
    sock.connectToHost(addr, 8981);
    if (!sock.waitForConnected(500))
    {
        qWarning().noquote() << "Connect timeout";
        return false;
    }
    sock.write(sCmd.toLocal8Bit());
    if (!sock.waitForBytesWritten(500))
    {
        qWarning().noquote() << "Write timeout";
        return false;
    }
    sock.waitForReadyRead(1000);
    if (sock.bytesAvailable() > 0)
    {
        QByteArray response(sock.readAll());
        QJsonDocument doc = QJsonDocument::fromJson(response);
        if (doc.isEmpty())
        {
            qWarning().noquote() << "non-JSON response:" << response << "length" << response.length();
        }
        else
        {
            qDebug().noquote() << "response to rn:" << doc.object()["rn"].toInt() << doc.object()["a"].toInt() << doc.object()["b"].toInt() << doc.object()["c"].toInt() << doc.object()["d"].toInt();
        }
    }
    else
    {
        qDebug() << "No response";
        return false;
    }
    // Report success
    return true;

}
