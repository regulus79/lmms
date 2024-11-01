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

#include <ctime>
#include <unordered_map>
#include <vector>

#include <QDataStream>
#include <QTcpServer>

#include "DataFile.h"
#include "lmms_basics.h"

namespace lmms
{

class Multiplayer : public QObject
{
    Q_OBJECT
public:
    Multiplayer();

	struct NetworkJournallingCheckPoint
	{
	public:
		NetworkJournallingCheckPoint(jo_id_t id, DataFile data, std::time_t time) :
			joId(id),
			storedData(data),
			changeTime(time)
		{
		}

		jo_id_t joId;
		DataFile storedData;
		// time of last change in seconds
		std::time_t changeTime;
	};
	using checkPointMap = std::unordered_map<jo_id_t, NetworkJournallingCheckPoint>;

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

	//! adds this project's journalling data to `m_changeList`
	void getChangedJournallingData();
	//! adds new checkpoint to `m_changeList` if it doesn't exist or the time is more up to date
	void addJournallingCheckPoint(jo_id_t id, DataFile& dataFile, std::time_t changeTime);

    //! used when lmms is a host
    QTcpServer * m_server;
    //! used if lmms is a client and connected to a host
    QTcpSocket * m_thisClient;
    //! used when lmms is a host, stores connected clients
    std::vector<QTcpSocket*> m_connectedClients;
    QDataStream m_dataStream;
    
    //! true if lmms is a client
    bool m_isClient;
    
    checkPointMap m_changeList;
};


} // namespace lmms

#endif // LMMS_MULTIPLAYER_H
