/*
 WebServe
 Copyright 2019 Peter Pearson.

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

#ifndef SOCKET_LAYER_PLAIN_H
#define SOCKET_LAYER_PLAIN_H

#include "connection_socket.h"
#include "socket_layer_interface.h"

#include "utils/socket.h"

// Plain/Unsecure connection socket, which does almost nothing and is really
// just a wrapper...
class ConnectionSocketPlain : public ConnectionSocket
{
public:
	ConnectionSocketPlain(Socket* pRawSocket, Logger& logger);
	virtual ~ConnectionSocketPlain();
	
	virtual bool send(const std::string& data, unsigned int flags = 0) const override;
	virtual bool send(unsigned char* pData, size_t dataLength) const override;

	virtual SocketRecvReturnCode recv(std::string& data) const override;
	virtual SocketRecvReturnCode recvSmart(std::string& data, unsigned int timeoutSecs) const override;
	virtual SocketRecvReturnCode recvWithTimeout(std::string& data, unsigned int timeoutSecs) const override;
	
	virtual bool close(bool deleteRawSocket) override;
	
protected:
	// we don't own this
	Socket*		m_pRawSocket; 
};

class SocketLayerPlain : public SocketLayer
{
public:
	SocketLayerPlain(Logger& logger);
	virtual ~SocketLayerPlain();
	
	virtual ReturnCodeType allocateSpecialisedConnectionSocket(RequestConnection& connection) override;
};

#endif // SOCKET_LAYER_PLAIN_H
