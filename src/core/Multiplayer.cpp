/*
 * Multiplayer.cpp
 *
 * Copyright (c) 2024 --TODO, I got some help from stack exchange, unsure if/how to properly credit
 *
 * This file is part of LMMS - https://lmms.io
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program (see COPYING); if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301 USA.
 *
 */

#include <QDebug>
#include <QTcpServer>
#include <QTcpSocket>

#include "Multiplayer.h"

namespace lmms
{

Multiplayer::Multiplayer() :
    QObject(),
    m_server(new QTcpServer(this)),
    m_client(new QTcpSocket(this))
{
    qDebug() << "Multiplayer class constructor!";
    //startServer(QHostAddress("127.0.0.1"), 30000);
}

void Multiplayer::startServer(QHostAddress addr, int port)
{
	connect(m_server, SIGNAL(newConnection()), this, SLOT(newConnection()));
	qDebug() << "Listening:" << m_server->listen(addr, port);
}

void Multiplayer::newConnection()
{
	qDebug() << "newConnection";
	while (m_server->hasPendingConnections())
	{
        QTcpSocket * clientConnection = m_server->nextPendingConnection();
        connect(clientConnection, &QTcpSocket::disconnected, clientConnection, &QObject::deleteLater);
		connect(clientConnection, &QTcpSocket::readyRead, this, &Multiplayer::readyRead);
    }
}

void Multiplayer::readyRead()
{
	QTcpSocket * clientConnection = static_cast<QTcpSocket*>(sender());
	QByteArray * buffer = new QByteArray();
	while (clientConnection->bytesAvailable() > 0)
	{
        buffer->append(clientConnection->readAll());
    }
    qDebug() << "Recieved:" << QString(*buffer);
}

void Multiplayer::connectToServer(QHostAddress addr, int port)
{
    qDebug() << "Connecting to server";
    dataStream.setDevice(m_client);
    m_client->abort();
    m_client->connectToHost(addr, port);
    connect(m_client, &QTcpSocket::readyRead, this, &Multiplayer::readyRead);
}

void Multiplayer::sendProjectState()
{

}

}