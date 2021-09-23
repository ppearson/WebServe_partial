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

#include "web_server_service.h"

#include <functional> // for bind()

#include "socket_layer_interface.h"
#include "socket_layer_plain.h"
#if WEBSERVE_ENABLE_HTTPS_SUPPORT
#include "socket_layer_s2n.h"
#endif

#include "web_request.h"
#include "configuration.h"
#include "utils/system.h"

#define USE_ADDITIONAL_ATOMIC_CONDITION_VARIABLE 1

WebServerService::WebServerService() :
	m_pNonSecureSocketLayer(nullptr),
	m_pSecureSocketLayer(nullptr),
    m_active(false),
	m_haveNewConnection(false),
	m_pRequestHandler(nullptr),
	m_freeHandler(false)
{

}

WebServerService::~WebServerService()
{
	if (m_pNonSecureSocketLayer)
	{
		delete m_pNonSecureSocketLayer;
		m_pNonSecureSocketLayer = nullptr;
	}
	
	if (m_pSecureSocketLayer)
	{
		delete m_pSecureSocketLayer;
		m_pSecureSocketLayer = nullptr;
	}
	
	for (WebServerThreadConfig& threadConfig : m_aThreadConfigs)
	{
		if (threadConfig.pSLThreadContext)
		{
			delete threadConfig.pSLThreadContext;
		}
	}
	
	if (m_freeHandler && m_pRequestHandler)
	{
		delete m_pRequestHandler;
		m_pRequestHandler = nullptr;
	}
}

bool WebServerService::configure(const Configuration& configuration)
{
	m_configuration = configuration;

	// not sure this is really the best place for it, but given we need to pass it elsewhere after this step, this is easiest for the moment,
	// although it's a bit of a mess...
	if (m_configuration.getLogOutputEnabled())
	{
		Logger::LogLevel level = Logger::convertStringToLogLevelEnum(m_configuration.getLogOutputLevel());
		const std::string& logTarget = m_configuration.getLogOutputTarget();
		if (level != Logger::eLevelOff)
		{
			if (logTarget == "stderr" || logTarget == "stdout")
			{
				m_logger.initialiseConsoleLogger(logTarget == "stderr" ? Logger::eLogStdErr : Logger::eLogStdOut, level, true);
			}
			else
			{
				// it's hopefully a file log
				m_logger.initialiseFileLogger(logTarget, level, Logger::eTimeStampTimeAndDate);
			}
		}
	}
	
	m_pNonSecureSocketLayer = new SocketLayerPlain(m_logger);
	if (!m_pNonSecureSocketLayer->configure(configuration))
	{
		// if there was an error, bail out...
		return false;
	}
	
#if WEBSERVE_ENABLE_HTTPS_SUPPORT
	if (m_configuration.isHTTPSEnabled())
	{
		m_pSecureSocketLayer = new SocketLayerS2N(m_logger);
		if (!m_pSecureSocketLayer->configure(configuration))
		{
			return false;
		}
	}
#endif
	
	return true;
}

bool WebServerService::bindSocketsAndPrepare()
{
	unsigned int socketCreationFlags = 0;
	if (m_configuration.getTcpFastOpen())
	{
		socketCreationFlags |= Socket::SOCKOPT_FASTOPEN;
	}
	
	if (m_configuration.isHTTPEnabled())
	{
		unsigned int portNumberHTTP = m_configuration.getHTTPPortNumber();
		
		m_mainSocketHTTP.create(&m_logger, socketCreationFlags);
	
		if (!m_mainSocketHTTP.bind(portNumberHTTP))
		{
			m_logger.critical("Can't bind to port: %u for HTTP listener", portNumberHTTP);
			return false;
		}
	}
	
#if WEBSERVE_ENABLE_HTTPS_SUPPORT
	if (m_configuration.isHTTPSEnabled())
	{
		unsigned int portNumberHTTPS = m_configuration.getHTTPSPortNumber();
		
		m_mainSocketHTTPS.create(&m_logger, socketCreationFlags);
	
		if (!m_mainSocketHTTPS.bind(portNumberHTTPS))
		{
			m_logger.critical("Can't bind to port: %u for HTTPS listener", portNumberHTTPS);
			return false;
		}
	}
#endif

	// Not great this being done in this class this way, but it's simpler for the moment...
	if (m_configuration.getDowngradeUserAfterBind())
	{
		// only do this if we're already root.

		if (m_configuration.getDowngradeUserName().empty())
		{
			m_logger.critical("downgradeUserAfterBind was specified, but no downgradeUserName was specified. Aborting.\n");
			return false;
		}

		if (!System::downgradeUserOfProcess(m_configuration.getDowngradeUserName(), true))
		{
			m_logger.critical("Could not downgrade process user. Aborting.");
			return false;
		}

		m_logger.notice("Downgraded process user to: %s", m_configuration.getDowngradeUserName().c_str());
	}
	
	return true;
}

void WebServerService::start()
{
	if (!m_pRequestHandler)
	{
		m_logger.critical("Request handler doesn't exist...");
		return;
	}

	if (m_configuration.isHTTPEnabled())
	{
		if (!m_mainSocketHTTP.listen(50))
		{
			m_logger.critical("Could not listen on HTTP socket.");
			return;
		}
		
		unsigned int portNumberHTTP = m_configuration.getHTTPPortNumber();
		m_logger.notice("Server listening on port: %u for HTTP", portNumberHTTP);
	}
	
#if WEBSERVE_ENABLE_HTTPS_SUPPORT
	if (m_configuration.isHTTPSEnabled())
	{
		if (!m_mainSocketHTTPS.listen(50))
		{
			m_logger.critical("Could not listen on HTTPS socket.");
			return;
		}
		
		unsigned int portNumberHTTPS = m_configuration.getHTTPSPortNumber();
		m_logger.notice("Server listening on port: %u for HTTPS", portNumberHTTPS);
	}
#endif

	m_active = true;

	if (m_configuration.isHTTPEnabled())
	{
		m_acceptHTTPConnectionThread = std::thread(&WebServerService::acceptHTTPConnectionThreadFunction, this);
	}
	
#if WEBSERVE_ENABLE_HTTPS_SUPPORT
	if (m_configuration.isHTTPSEnabled())
	{
		m_acceptHTTPSConnectionThread = std::thread(&WebServerService::acceptHTTPSConnectionThreadFunction, this);
	}
#endif

	for (unsigned int i = 0; i < m_configuration.getNumWorkerThreads(); i++)
	{
		m_aThreadConfigs.emplace_back(WebServerThreadConfig(i, &m_configuration, &m_logger));
	}
	
	// assumption here is only the secure socket layer will need this, which might need to be revisited...
	bool createSocketLayerPerThreadContexts = m_pSecureSocketLayer->supportsPerThreadContext();

	for (unsigned int i = 0; i < m_configuration.getNumWorkerThreads(); i++)
	{
		WebServerThreadConfig* pThreadConfig = &m_aThreadConfigs[i];
		
		if (createSocketLayerPerThreadContexts)
		{
			// TODO: these leak...
			SocketLayerThreadContext* pNewThreadContext = m_pSecureSocketLayer->allocatePerThreadContext();
			if (pNewThreadContext)
			{
				pThreadConfig->pSLThreadContext = pNewThreadContext;
			}
		}
		
		std::thread newThread = std::thread(&WebServerService::workerThreadFunction, this, pThreadConfig);
		m_aWorkerThreads.emplace_back(std::move(newThread));
	}
	
	m_logger.notice("%zu worker threads started.", m_aWorkerThreads.size());

	for (std::thread& workerThread : m_aWorkerThreads)
	{
		workerThread.join();
	}

	if (m_configuration.isHTTPEnabled())
	{
		m_acceptHTTPConnectionThread.join();
	}

#if WEBSERVE_ENABLE_HTTPS_SUPPORT
	if (m_configuration.isHTTPSEnabled())
	{
		m_acceptHTTPSConnectionThread.join();
	}
#endif
}

void WebServerService::stop()
{
	m_active = false;
	
	m_logger.notice("Stopping web service.");
	
	if (m_configuration.isHTTPEnabled())
	{
		m_mainSocketHTTP.close();
	}
	
#if WEBSERVE_ENABLE_HTTPS_SUPPORT
	if (m_configuration.isHTTPSEnabled())
	{
		m_mainSocketHTTPS.close();
	}
#endif
	
	{
		
#if USE_ADDITIONAL_ATOMIC_CONDITION_VARIABLE
		m_haveNewConnection = true;
#endif
		std::unique_lock<std::mutex> lock(m_lock);

		m_newConnectionEvent.notify_all();
	}
}

void WebServerService::workerThreadFunction(WebServerThreadConfig* pThreadConfig)
{
	while (m_active)
	{
		RequestConnection connection;

		{
			std::unique_lock<std::mutex> lock(m_lock);

			if (!m_active)
				break;

			if (m_aPendingConnections.empty())
			{
#if USE_ADDITIONAL_ATOMIC_CONDITION_VARIABLE
				m_haveNewConnection = false;
#endif
				// there's nothing currently in the queue for us to take, so wait for an event...

//				fprintf(stderr, "Waiting for event...\n");

#if USE_ADDITIONAL_ATOMIC_CONDITION_VARIABLE
				m_newConnectionEvent.wait(lock, [this]{ return m_haveNewConnection.load(); });
#else
				m_newConnectionEvent.wait(lock);
#endif

//				fprintf(stderr, "Event received...\n");
			}

			if (!m_active)
				break;

			if (m_aPendingConnections.empty())
			{
				// we shouldn't get here, but we do, a spurious wake-up??...
//				fprintf(stderr, "Error...\n");
				continue;
			}

			connection = std::move(m_aPendingConnections.front());
			m_aPendingConnections.pop();
		}

		m_logger.debug("Handling new connection.");

		connection.pThreadConfig = pThreadConfig;

		handleConnection(connection);
	}
}

void WebServerService::acceptHTTPConnectionThreadFunction()
{
	while (m_active)
	{
		RequestConnection newConnection(new Socket(), nullptr);

		if (m_mainSocketHTTP.accept(newConnection.pRawSocket))
		{
			newConnection.pRawSocket->setLogger(&m_logger);


			{
				std::unique_lock<std::mutex> lock(m_lock);

				bool wasEmpty = m_aPendingConnections.empty();

				m_aPendingConnections.emplace(newConnection);
//			}

				// TODO: is this a good idea?
				if (wasEmpty)
				{
#if USE_ADDITIONAL_ATOMIC_CONDITION_VARIABLE
					m_haveNewConnection = true;
#endif

//					fprintf(stderr, "Notifying pool...\n");

					// Note: ideally, we should be able to use notify_one() here...
//					m_newConnectionEvent.notify_one();

					// however, in practice, with keep-alive (pipelined requests) enabled,
					// we seem to end up with threads waiting on the condition, but with m_aPendingConnections
					// having multiple items in it ready to be consumed. using notify_all() appears to stop this
					// happening. It's possible this is due to the if (wasEmpty) condition above or some other
					// issue with the logic / condition_variable...
					m_newConnectionEvent.notify_all();
				}
			}
		}
		else
		{
			if (m_active)
			{
				// only log this if we're still active, otherwise we print this when stopping the service...
				m_logger.error("Can't accept connection.");
			}
		}
	}
}

#if WEBSERVE_ENABLE_HTTPS_SUPPORT
void WebServerService::acceptHTTPSConnectionThreadFunction()
{
	while (m_active)
	{
		RequestConnection newConnection(new Socket(), nullptr);
		newConnection.https = true;

		if (m_mainSocketHTTPS.accept(newConnection.pRawSocket))
		{
			newConnection.pRawSocket->setLogger(&m_logger);


			{
				std::unique_lock<std::mutex> lock(m_lock);

				bool wasEmpty = m_aPendingConnections.empty();

				m_aPendingConnections.emplace(newConnection);
//			}

				// TODO: is this a good idea?
				if (wasEmpty)
				{
#if USE_ADDITIONAL_ATOMIC_CONDITION_VARIABLE
					m_haveNewConnection = true;
#endif

//					fprintf(stderr, "Notifying pool...\n");

					// Note: ideally, we should be able to use notify_one() here...
//					m_newConnectionEvent.notify_one();

					// however, in practice, with keep-alive (pipelined requests) enabled,
					// we seem to end up with threads waiting on the condition, but with m_aPendingConnections
					// having multiple items in it ready to be consumed. using notify_all() appears to stop this
					// happening. It's possible this is due to the if (wasEmpty) condition above or some other
					// issue with the logic / condition_variable...
					m_newConnectionEvent.notify_all();
				}
			}
		}
		else
		{
			if (m_active)
			{
				// only log this if we're still active, otherwise we print this when stopping the service...
				int errorNumber = errno;
				m_logger.error("Can't accept() new connection, error code: %i", errorNumber);
				
				// throttle things a bit, so we don't get into an endless loop...
				std::this_thread::sleep_for(std::chrono::seconds(10));
			}
		}
	}
}
#endif

void WebServerService::handleConnection(RequestConnection& connection)
{
	connection.ipInfo.initInfo(connection.pRawSocket);
	
	if (connection.https && m_pSecureSocketLayer)
	{
		m_logger.debug("Client HTTPS connection accept()ed from IP: %s", connection.ipInfo.getIPAddress().c_str());
		
		connection.connStatistics.httpsConnections += 1;
		
		ReturnCodeType retCode = m_pSecureSocketLayer->allocateSpecialisedConnectionSocket(connection);
		if (retCode == eReturnFail)
		{
			connection.closeConnectionAndFreeSockets();
			m_logger.error("Error allocating specialised connection socket for HTTPS connection.");
			return;
		}
		else if (retCode == eReturnFailSilent)
		{
			// return, but don't log anything
			connection.closeConnectionAndFreeSockets();
			m_logger.debug("Fail silent when allocating S2N connection for IP: %s", connection.ipInfo.getIPAddress().c_str());
			return;
		}
	}
	else
	{
		m_logger.debug("Client HTTP connection accept()ed from IP: %s", connection.ipInfo.getIPAddress().c_str());
		
		connection.connStatistics.httpConnections += 1;
		
		// this can't really fail (although the allocation of the socket could in theory, but...)
		m_pNonSecureSocketLayer->allocateSpecialisedConnectionSocket(connection);
	}
	
	// check we've allocated a valid connection socket - if not, error and ignore connection.
	if (!connection.pConnectionSocket)
	{
		m_logger.error("Could not allocate connection socket for connection. Ignoring request.");
		return;
	}
	
	m_pRequestHandler->handleRequest(connection);
}
