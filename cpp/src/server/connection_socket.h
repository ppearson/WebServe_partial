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

#ifndef CONNECTION_SOCKET_H
#define CONNECTION_SOCKET_H

#include <string>

#include "utils/logger.h"

#include "utils/socket.h"

struct ConnectionStatistics;

class ConnectionSocket
{
public:
	ConnectionSocket(Logger& logger) : m_logger(logger)
	{
	}
	
	virtual ~ConnectionSocket()
	{
	}
	
	enum SPECIAL_FLAGS
	{
		SEND_IGNORE_FAILURES = 1 << 0
	};
	
	virtual bool send(const std::string& data, unsigned int flags = 0) const = 0;
	virtual bool send(unsigned char* pData, size_t dataLength) const = 0;

	virtual SocketRecvReturnCode recv(std::string& data) const = 0;
	virtual SocketRecvReturnCode recvSmart(std::string& data, unsigned int timeoutSecs) const = 0;
	virtual SocketRecvReturnCode recvWithTimeout(std::string& data, unsigned int timeoutSecs) const = 0;
	
	virtual void accumulateSocketConnectionStatistics(ConnectionStatistics& connStatistics) const
	{
		
	}
	
	virtual bool close(bool deleteRawSocket) = 0;
	
protected:
	// not too happy with this, but don't really like pulling it through all the above either, so...
	Logger&		m_logger;
};

#endif // CONNECTION_SOCKET_H

