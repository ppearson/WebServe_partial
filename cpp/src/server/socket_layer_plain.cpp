/*
 WebServe
 Copyright 2018-2022 Peter Pearson.

 Licensed under the Apache License, Version 2.0 (the "License");
 You may not use this file except in compliance with the License.
 You may obtain a copy of the License at

 http://www.apache.org/licenses/LICENSE-2.0

 Unless required by applicable law or agreed to in writing, software
 distributed under the License is distributed on an "AS IS" BASIS,
 WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 See the License for the specific language governing permissions and
 limitations under the License.
 ---------
*/

#include "socket_layer_plain.h"

ConnectionSocketPlain::ConnectionSocketPlain(Socket* pRawSocket, Logger& logger) : ConnectionSocket(logger),
				m_pRawSocket(pRawSocket)
{
	
}

ConnectionSocketPlain::~ConnectionSocketPlain()
{
	
}

bool ConnectionSocketPlain::send(const std::string& data, unsigned int flags) const
{
	return m_pRawSocket->send(data);
}

bool ConnectionSocketPlain::send(unsigned char* pData, size_t dataLength) const
{
	return m_pRawSocket->send(pData, dataLength);
}

SocketRecvReturnCode ConnectionSocketPlain::recv(std::string& data) const
{
	return m_pRawSocket->recv(data);
}

SocketRecvReturnCode ConnectionSocketPlain::recvSmart(std::string& data, unsigned int timeoutSecs) const
{
	return m_pRawSocket->recvSmartWithTimeout(data, timeoutSecs);
}

SocketRecvReturnCode ConnectionSocketPlain::recvWithTimeout(std::string& data, unsigned int timeoutSecs) const
{
	return m_pRawSocket->recvWithTimeout(data, timeoutSecs);
}

bool ConnectionSocketPlain::close(bool deleteRawSocket)
{
	m_pRawSocket->close();
	
	// we don't really own this, but it's convenient doing this this way...
	if (deleteRawSocket)
	{
		delete m_pRawSocket;
		m_pRawSocket = nullptr;
	}
	
	return true;
}

//

SocketLayerPlain::SocketLayerPlain(Logger& logger) : SocketLayer(logger)
{
	
}

SocketLayerPlain::~SocketLayerPlain()
{
	
}

ReturnCodeType SocketLayerPlain::allocateSpecialisedConnectionSocket(RequestConnection& connection)
{
	connection.pConnectionSocket = new ConnectionSocketPlain(connection.pRawSocket, m_logger);
	
	return eReturnOK;
}
