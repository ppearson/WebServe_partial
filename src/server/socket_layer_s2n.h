/*
 WebServe
 Copyright 2019-2020 Peter Pearson.

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

#ifndef SOCKET_LAYER_S2N_H
#define SOCKET_LAYER_S2N_H

#include "connection_socket.h"
#include "socket_layer_interface.h"

#include "utils/socket.h"

#if WEBSERVE_ENABLE_HTTPS_SUPPORT
struct s2n_config;
struct s2n_connection;
#endif

class ConnectionSocketS2N : public ConnectionSocket
{
public:
	ConnectionSocketS2N(Logger& logger, Socket* pRawSocket, struct s2n_connection* pS2NConnection, bool ownConnection);
	virtual ~ConnectionSocketS2N();
	
	virtual bool send(const std::string& data, unsigned int flags = 0) const override;
	virtual bool send(unsigned char* pData, size_t dataLength) const override;

	virtual SocketRecvReturnCode recv(std::string& data) const override;
	virtual SocketRecvReturnCode recvSmart(std::string& data, unsigned int timeoutSecs) const override;
	virtual SocketRecvReturnCode recvWithTimeout(std::string& data, unsigned int timeoutSecs) const override;
	
	virtual void accumulateSocketConnectionStatistics(ConnectionStatistics& connStatistics) const override;
	
	virtual bool close(bool deleteRawSocket) override;
	
protected:
	SocketRecvReturnCode recvWithTimeoutPoll(std::string& data, unsigned int& dataLength, unsigned int timeoutSecs) const;
	
protected:
	bool		m_active;
	
	
	// we don't own this
	Socket*		m_pRawSocket; 
	
	// whether this connection was allocated just for this socket
	bool				m_ownConnection;
	struct s2n_connection*		m_pS2NConnection;
};

class SocketLayerS2N : public SocketLayer
{
public:
	SocketLayerS2N(Logger& logger);
	virtual ~SocketLayerS2N();
	
	virtual bool configure(const Configuration& configuration) override;
	
	virtual bool supportsPerThreadContext() const override
	{
		return true;
	}
	
	virtual SocketLayerThreadContext* allocatePerThreadContext() override;
	
	virtual ReturnCodeType allocateSpecialisedConnectionSocket(RequestConnection& connection) override;
	
	
protected:
	class S2NSocketLayerThreadContext : public SocketLayerThreadContext
	{
	public:
		S2NSocketLayerThreadContext() : m_s2nConnection(nullptr)
		{
		}
		
		virtual ~S2NSocketLayerThreadContext();		
		
		struct s2n_connection*		m_s2nConnection;
	};
	
protected:
	
#if WEBSERVE_ENABLE_HTTPS_SUPPORT
	struct s2n_config*				m_s2nConfig;
#endif
};

#endif // SOCKET_LAYER_S2N_H
