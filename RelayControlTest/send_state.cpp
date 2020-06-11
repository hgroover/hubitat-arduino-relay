#include  "mainwindow.h"
#include "ui_mainwindow.h"

#include <QDebug>
#include <QNetworkAddressEntry>
#include <QTcpSocket>

void MainWindow::sendState()
{
    // Do we have address?
    QString addr(ui->txtDeviceAddress->text());
    if (addr.isEmpty())
    {
        ui->statusBar->showMessage("IP address not received yet", 6000);
        return;
    }
    QString sCmd(QString().sprintf("rn=%03d;a=%d;b=%d;c=%d;d=%d",
                                   m_requestSerial,
                                   ui->chkRelay1->isChecked() ? m_relay1Value : 0,
                                   ui->chkRelay2->isChecked() ? 1 : 0,
                                   ui->chkRelay3->isChecked() ? 1 : 0,
                                   ui->chkRelay4->isChecked() ? 1 : 0));
    m_requestSerial = (m_requestSerial + 1) % 1000;
    qDebug().noquote() << "sendState" << sCmd;
    // Reset relay 1 value
    if (m_relay1Value > 1)
    {
        m_relay1Value = 1;
        ui->chkRelay1->setChecked(false);
    }
    QTcpSocket sock;
    sock.connectToHost(addr, 8981);
    if (!sock.waitForConnected(500))
    {
        qWarning().noquote() << "Connect timeout";
        return;
    }
    sock.write(sCmd.toLocal8Bit());
    if (!sock.waitForBytesWritten(500))
    {
        qWarning().noquote() << "Write timeout";
        return;
    }
    sock.waitForReadyRead(1000);
    if (sock.bytesAvailable() > 0)
    {
        QByteArray response(sock.readAll());
        qDebug().noquote() << "response:" << response << "len=" << response.length();
    }
    else
    {
        qDebug() << "No response";
    }
}
