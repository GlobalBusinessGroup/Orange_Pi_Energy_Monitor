/*
 * EM100 - Energy Monitor for Volts / Amps / Power displayed on LCD, with USB and Ethernet interfaces.
 * Copyright (C) 2016-2017 Stephan de Georgio
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA
 */

/*
 * This file sets up a TCP/IP server bound to port 23.
 * Incomming connections will be sent the CSV data.
 */

/* 
 * File:   DataLogServer.cpp
 * Author: Stephan de Georgio
 * 
 * Created on 04 November 2016, 14:33
 */

#include <QDateTime>
#include <QHostAddress>
#include <QList>
#include <QNetworkInterface>
#include <QString>

#include "DataLogServer.h"
#include "EnergyMonitorAppGlobal.h"

DataLogServer::DataLogServer(QObject *parent) {
	setParent(parent);
    serverRunning = false;
}

DataLogServer::~DataLogServer() {
    stopServer();
}

bool DataLogServer::startServer() {
    if(!serverRunning) {
        logServer = new QTcpServer(this);
        connect(logServer, SIGNAL(newConnection()), this, SLOT(acceptConnection()));
        if(!logServer->listen(QHostAddress::Any, DATA_LOG_SERVER_LISTEN_PORT)) {
            logServer->close();
            return false;
        }
        serverRunning = true;
        qDebug() << "Server started and listening on address: " << getLocalIPAddress(QAbstractSocket::IPv4Protocol) << getLocalIPAddress(QAbstractSocket::IPv6Protocol);
        return true;
    }
    return false;
}

bool DataLogServer::isServerRunning() {
    return serverRunning;
}

void DataLogServer::stopServer() {
    logServer->close();
    serverRunning = false;
}

QList<QString> DataLogServer::getLocalIPAddress(QAbstractSocket::NetworkLayerProtocol protocolType) {
    QList<QHostAddress> list = QNetworkInterface::allAddresses();
    QList<QString> addressList;
    for(int nIter=0; nIter<list.count(); nIter++) {
        if(!list[nIter].isLoopback()) {
            if (list[nIter].protocol() == protocolType) {
                addressList.append(list[nIter].toString());
            }
        }
    }
    return addressList;
}

void DataLogServer::slotMeasurementsReady(DecodedMeasurements values) {
    int i;
    qint64 currentTimeInt = QDateTime::currentMSecsSinceEpoch();
    QString currentTimeString = QString::number(currentTimeInt);
    QString data(QString(SOFTWARE_VERSION) + ","
            + currentTimeString + ","
            + QString::number(values.powerActive) + ","
            + QString::number(values.voltageRms) + ","
            + QString::number(values.currentRms) + ","
            + QString::number(values.frequency) + ","
            + QString::number(values.powerFactor) + ","
            + QString::number(values.powerApparent) + ","
            + QString::number(values.powerReactive)
            + "\r\n");
    for(i = 0; i < socketList.size(); i++) {
        socketList.at(i)->write(data.toLocal8Bit(), data.length());
        socketList.at(i)->flush(); 
    }
}

void DataLogServer::acceptConnection() {
    while (logServer->hasPendingConnections()) {
        QTcpSocket* clientSocket;
        clientSocket = logServer->nextPendingConnection();
        socketList.append(clientSocket);
        connect(clientSocket, SIGNAL(disconnected()), this, SLOT(clientDisconnected()));
    }
}

void DataLogServer::clientDisconnected() {
    QTcpSocket* socket = static_cast<QTcpSocket*>(sender());
    socketList.removeAll(socket);
    socket->deleteLater();
}
