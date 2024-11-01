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

#include "Multiplayer.h"

#include <QTcpSocket>

#include "Engine.h"
#include "JournallingObject.h"
#include "ProjectJournal.h"

namespace lmms
{

Multiplayer::Multiplayer() :
	QObject(),
	m_server(nullptr),
	m_thisClient(nullptr)
{
	qDebug() << "Multiplayer class constructor!";
	//startServer(QHostAddress("127.0.0.1"), 30000);
}

void Multiplayer::startServer(QHostAddress addr, int port)
{
	changeServerClientConfiguration(false);
	connect(m_server, SIGNAL(newConnection()), this, SLOT(newConnection()));
	qDebug() << "Listening:" << m_server->listen(addr, port);
}

void Multiplayer::newConnection()
{
	qDebug() << "newConnection";
	while (m_server->hasPendingConnections())
	{
		QTcpSocket * clientConnection = m_server->nextPendingConnection();
		if (clientConnection)
		{
			m_connectedClients.push_back(clientConnection);
			connect(clientConnection, &QTcpSocket::disconnected, this, &Multiplayer::clientDisconnected);
			connect(clientConnection, &QTcpSocket::readyRead, this, &Multiplayer::readyRead);
		}
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

void Multiplayer::clientDisconnected()
{
	QTcpSocket * clientConnection = static_cast<QTcpSocket*>(sender());
	for (size_t i = 0; i < m_connectedClients.size(); i++)
	{
		if (m_connectedClients[i] == clientConnection)
		{
			// `m_server` owns the connected `QTcpSocket`s but they can be deleted from elsewhere to fee up memory
			m_connectedClients[i]->deleteLater();
			// swap with last element
			m_connectedClients[i] = m_connectedClients[m_connectedClients.size() - 1];
			m_connectedClients.pop_back();
			break;
		}
	}
}

void Multiplayer::connectToServer(QHostAddress addr, int port)
{
	changeServerClientConfiguration(true);
	qDebug() << "Connecting to server";
	m_dataStream.setDevice(m_thisClient);
	// closting socket, reseting object
	m_thisClient->abort();
	m_thisClient->connectToHost(addr, port);
	connect(m_thisClient, &QTcpSocket::readyRead, this, &Multiplayer::readyRead);
}

void Multiplayer::sendProjectState()
{

}

void Multiplayer::changeServerClientConfiguration(bool isClientNow)
{
	if (m_isClient != isClientNow)
	{
		if (isClientNow == true)
		{
			// if the server configuration is selected
			if (m_thisClient != nullptr)
			{
				delete m_thisClient;
				m_thisClient = nullptr;
			}
		}
		else
		{
			// if the client configuration is selected
			if (m_server != nullptr)
			{
				delete m_server;
				m_server = nullptr;
			}
		}
		m_isClient = isClientNow;
	}

	// making sure that the needed object isn't nullptr
	// (for example after `this` is constructed)
	if (m_isClient == true)
	{
		if (m_thisClient == nullptr) { new QTcpSocket(this); }
	}
	else
	{
		if (m_server == nullptr) { new QTcpServer(this); }
	}
}

void Multiplayer::getChangedJournallingData()
{
	ProjectJournal::JoIdMap journallingObjects = Engine::projectJournal()->getJoIdMap();
	for (auto& it : journallingObjects)
	{
		std::time_t changeTime;
		bool isChanged = it->isJournallingDataChanged(&changeTime);
		if (isChanged)
		{
			DataFile curState(DataFile::Type::JournalData);
			it->saveState(curState, curState.content());
			addJournallingCheckPoint(it->id(), curState, changeTime);
		}
	}
}

void Multiplayer::addJournallingCheckPoint(jo_id_t id, DataFile& dataFile, std::time_t changeTime)
{
	auto it = m_changeList.find(id);
	if (it != m_changeList.end())
	{
		// if found
		if (it->second.changeTime < changeTime)
		{
			// if the new data is more up to date
			it->second.changeTime = changeTime;
			it->second.storedData = dataFile;
		}
	}
	else
	{
		// if not found
		m_changeList.insert(std::make_pair(id, NetworkJournallingCheckPoint(id, dataFile, changeTime)));
	}
}

} // namespace lmms
