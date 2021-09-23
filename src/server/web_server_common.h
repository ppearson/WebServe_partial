/*
 WebServe
 Copyright 2018 Peter Pearson.

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

#ifndef WEB_SERVER_COMMON_H
#define WEB_SERVER_COMMON_H

#include "utils/socket.h"
// arguably is better to forward-declare it, but that means we need the include everywhere else, so...
#include "utils/logger.h"

#include "configuration.h"

#include "connection_socket.h"

#include "client_connection_ip_info.h"
#include "status_service.h"

enum ReturnCodeType
{
	eReturnOK,
	eReturnFailSilent, // it failed, but we can handle it
	eReturnFail
};

class SocketLayerThreadContext
{
public:
	SocketLayerThreadContext()
	{
		
	}
	
	virtual ~SocketLayerThreadContext()
	{
		
	}
};

struct WebServerThreadConfig
{
	// TODO: this is annoying needing this due to vector storage usage...
	WebServerThreadConfig() :
		threadID(-1),
		pConfiguration(nullptr),
		pLogger(nullptr),
	    pSLThreadContext(nullptr)
	{

	}

	WebServerThreadConfig(unsigned int thdID,
						  const Configuration* pConfig,
						  Logger* pLog) :
		threadID(thdID),
		pConfiguration(pConfig),
		pLogger(pLog),
	    pSLThreadContext(nullptr)
	{

	}


	unsigned int			threadID;

	// we don't own this
	const Configuration*	pConfiguration;

	// the assumption here is that this will ALWAYS be valid. It's only a pointer
	// due to the way this class is stored in an std::vector, which arguably should be improved...
	Logger*					pLogger;
	
	SocketLayerThreadContext*	pSLThreadContext;
};

struct ConnectionStatistics
{
	
	uint64_t		httpConnections = 0;
	uint64_t		httpsConnections = 0;
	
	uint64_t		httpRequests = 0;
	uint64_t		httpsRequests = 0;
	
	uint64_t		httpBytesReceived = 0;
	uint64_t		httpBytesSent = 0;
	uint64_t		httpsBytesReceived = 0;
	uint64_t		httpsBytesSent = 0;
};

struct RequestConnection
{
	RequestConnection()
	{
	}

	RequestConnection(Socket* pRawSock,
					  WebServerThreadConfig* pWSThreadConfig) :
		pRawSocket(pRawSock),
		pThreadConfig(pWSThreadConfig)
	{

	}
	
	~RequestConnection()
	{
		if (pConnectionSocket)
		{
			delete pConnectionSocket;
			pConnectionSocket = nullptr;
		}
		
		// TODO: something weird is going on with pRawSocket getting cleaned up..
		//       deleting pRawSocket here crashes. Looks like the std::queue pop in WebServerService
		//       is half-copying this class or something...
	}
	
	void closeConnectionAndFreeSockets()
	{
		if (pConnectionSocket)
		{
			if (pStatusService)
			{
				pConnectionSocket->accumulateSocketConnectionStatistics(connStatistics);
			}
			
			// this should also close and free the raw socket we own...
			pConnectionSocket->close(true);

			// Raw socket was closed and free'd above, but we need to null this
			// pointer to it...			
			pRawSocket = nullptr;
			
			delete pConnectionSocket;
			pConnectionSocket = nullptr;
		}
		
		// if we didn't close the raw socket above (i.e. because HTTPS negotiation failed), clean up
		// the raw socket as well...
		if (pRawSocket)
		{
			pRawSocket->close();
			delete pRawSocket;
			pRawSocket = nullptr;
		}
		
		if (pStatusService)
		{
			pStatusService->accumulateConnectionStatistics(connStatistics);
		}
	}
	
	bool					https = false;

	// this should not be used for communication...
	Socket*					pRawSocket = nullptr;
	// this is the one which should be used...
	ConnectionSocket*		pConnectionSocket = nullptr;


	ClientConnectionIPInfo	ipInfo;
	
	ConnectionStatistics	connStatistics;
	
	StatusService*			pStatusService = nullptr;

	// hacky convenience function...
	Logger& logger()
	{
		return *pThreadConfig->pLogger;
	}

	WebServerThreadConfig*	pThreadConfig = nullptr;
};

#endif // WEB_SERVER_COMMON_H
