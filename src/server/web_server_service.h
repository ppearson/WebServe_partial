/*
 WebServe
 Copyright 2018-2019 Peter Pearson.

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

#ifndef WEB_SERVER_SERVICE_H
#define WEB_SERVER_SERVICE_H

#include <thread>
#include <atomic>
#include <condition_variable>

#include <vector>
#include <queue>

#include "utils/socket.h"
#include "utils/logger.h"

#include "configuration.h"

#include "web_server_common.h"
#include "main_request_handler.h"

class SocketLayer;

class WebServerService
{
public:
	WebServerService();
	~WebServerService();

	bool configure(const Configuration& configuration);

	void setRequestHandler(MainRequestHandler* requestHandler, bool freeHandler)
	{
		m_pRequestHandler = requestHandler;
		m_freeHandler = freeHandler;
	}

	// TODO: don't like this, but...
	Logger& getLogger()
	{
		return m_logger;
	}

	// bind the listening socket, and optionally downgrade the username.
	// This needs to be done first before configuring the main RequestHandler (which isn't great, but...)
	bool bindSocketsAndPrepare();
	
	// start the actual threads accepting connections.
	void start();
	void stop();


protected:

	void workerThreadFunction(WebServerThreadConfig* pThreadConfig);
	
	void acceptHTTPConnectionThreadFunction();
#if WEBSERVE_ENABLE_HTTPS_SUPPORT
	void acceptHTTPSConnectionThreadFunction();
#endif

	void handleConnection(RequestConnection& connection);

protected:
	std::vector<std::thread>		m_aWorkerThreads;
	
	std::thread						m_acceptHTTPConnectionThread;
#if WEBSERVE_ENABLE_HTTPS_SUPPORT
	std::thread						m_acceptHTTPSConnectionThread;
#endif
	
	Socket							m_mainSocketHTTP;
#if WEBSERVE_ENABLE_HTTPS_SUPPORT
	Socket							m_mainSocketHTTPS;
#endif
	
	// socket layers
	SocketLayer*					m_pNonSecureSocketLayer;
	SocketLayer*					m_pSecureSocketLayer;
	
	std::vector<WebServerThreadConfig> m_aThreadConfigs;

	Configuration					m_configuration;

	Logger							m_logger;

	std::mutex						m_lock;
	bool							m_active;

	std::queue<RequestConnection>	m_aPendingConnections;

	std::atomic<bool>				m_haveNewConnection;
	std::condition_variable			m_newConnectionEvent;

	// once this has been set, we own it, and clean it up at the end...
	MainRequestHandler*				m_pRequestHandler;
	bool							m_freeHandler;
};

#endif // WEB_SERVER_SERVICE_H
