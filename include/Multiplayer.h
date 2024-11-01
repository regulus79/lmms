/*
 * Multiplayer.h
 *
 * Copyright (c) 2024 regulus79
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

#ifndef LMMS_MULTIPLAYER_H
#define LMMS_MULTIPLAYER_H

#include <vector>

#include <QDataStream>
#include <QTcpServer>

namespace lmms
{

class Multiplayer : public QObject
{
    Q_OBJECT
public:
    Multiplayer();

    void startServer(QHostAddress addr, int port);

    void connectToServer(QHostAddress addr, int port);

    void sendProjectState();

private slots:
    void newConnection();
    void readyRead();

	void clientDisconnected();
private:
	//! changes between lmms being a server or a client
	void changeServerClientConfiguration(bool isClientNow);

    //! used when lmms is a host
    QTcpServer * m_server;
    //! used if lmms is a client and connected to a host
    QTcpSocket * m_thisClient;
    //! used when lmms is a host, stores connected clients
    std::vector<QTcpSocket*> m_connectedClients;
    QDataStream m_dataStream;
    
    //! true if lmms is a client
    bool m_isClient;
};


} // namespace lmms

#endif // LMMS_MULTIPLAYER_H
