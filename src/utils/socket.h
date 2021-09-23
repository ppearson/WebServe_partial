/*
 WebServe
 Copyright 2018-2020 Peter Pearson.
 taken originally from:
 Sitemon
 Copyright 2010 Peter Pearson.
 
 Licensed under the Apache License, Version 2.0 (the "License");
 You may not use this file except in compliance with the License.
 You may obtain a copy of the License at
 
 http://www.apache.org/licenses/LICENSE-2.0
 
 Unless required by applicable law or agreed to in writing, software
 distributed under the License is distributed on an "AS IS" BASIS,
 WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 See the License for the specific language governing permissions and
 limitations under the License.
 
 */

#ifndef SOCKET_H
#define SOCKET_H

#include <cstdio>
#include <cstring>

#ifdef __SUNPRO_CC
	#include <sys/types.h>
	#include <sys/socket.h>
	#include <netinet/in.h>
	#include <netdb.h>
#else
	#include <sys/socket.h>
	#include <sys/types.h>
	#include <arpa/inet.h>
	#include <netdb.h>
	#include <string.h>
#endif

#define SOCK_LEAK_DETECTOR 0

#include "logger.h"

enum SocketRecvReturnCodeType
{
	eSockRecv_OK,
	eSockRecv_TimedOutNoData,
	eSockRecv_TimedOutWithData,
	eSockRecv_NoData,
	eSockRecv_PeerClosed,
	eSockRecv_Error
};

struct SocketRecvReturnCode
{
	SocketRecvReturnCode()
	{
		
	}

	SocketRecvReturnCode(SocketRecvReturnCodeType ty) : type(ty)
	{
		
	}

	SocketRecvReturnCodeType type = eSockRecv_Error;
};

class Socket
{
public:
	Socket();
	Socket(Logger* pLogger, int port);
	Socket(Logger* pLogger, const std::string& host, int port);

	friend class ClientConnectionIPInfo;
	
	~Socket();

	void setLogger(Logger* pLogger)
	{
		m_pLogger = pLogger;
	}

	enum SocketOptionFlags
	{
		SOCKOPT_FASTOPEN			= 1 << 0
	};
	
	bool create(Logger* pLogger, unsigned int flags = 0);

	bool bind(const int port);
	bool listen(const int connections) const;
	bool accept(Socket* sock) const;
	
	bool connect();
	bool connect(const std::string& host, const int port);
	void close();
	
	bool setRecvTimeoutOption(int timeoutSeconds);
	
	bool send(const std::string& data) const;
	bool send(unsigned char* pData, size_t dataLength) const;

	SocketRecvReturnCode recv(std::string& data) const;
	SocketRecvReturnCode recvSmart(std::string& data) const;
	SocketRecvReturnCode recvSmartWithTimeout(std::string& data, unsigned int timeoutSecs) const;
	SocketRecvReturnCode recvWithTimeout(std::string& data, unsigned int timeoutSecs) const;
	
	int peekRecv() const;
	
	bool isValid() const { return m_sock != -1; }
	
	// don't really like this...
	int getSocketFD() const { return m_sock; }
	
/*
	bool getSocketRecvTimeout(unsigned int& timeoutSeconds) const;
	bool setSocketRecvTimeout(unsigned int timeoutSeconds);
*/
	
protected:
	SocketRecvReturnCode recvWithTimeoutPoll(std::string& data, unsigned int& dataLength, unsigned int timeoutSecs) const;
	SocketRecvReturnCode recvWithTimeoutSockOpt(std::string& data, unsigned int& dataLength, unsigned int timeoutSecs) const;

protected:
	// TODO: this needs to be sockaddr_storage when we add IPv6 support...
	sockaddr_in		m_addr;
	int				m_sock;
	
	int				m_port;
	std::string		m_host;

	// really dislike this, but again, there aren't really "better" options...
	Logger*			m_pLogger;
	
#if SOCK_LEAK_DETECTOR
	char*			m_pLeak;
#endif
};

#endif
