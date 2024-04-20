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

#include "main_request_handler.h"

#include "web_server_common.h"
#include "web_request.h"
#include "web_response.h"

#include "configuration.h"

#include "web_response_generators.h"

#include "utils/uri_helpers.h"
#include "utils/string_helpers.h"

#include "photos_handler/photos_request_handler.h"
#include "files_handler/files_request_handler.h"
#include "proxy_handler/proxy_request_handler.h"

MainRequestHandler::MainRequestHandler() :
	m_accessControlEnabled(false),
	m_404NotFoundResponsesEnabled(true),
	m_hostnamePortRewriteRequiredForHTTPSRedirect(false),
	m_photosType(eSSOff), // Note: this doesn't match the default of Configuration.
	m_haveDirSubRequestHandlers(false),
	m_haveHostSubRequestHandlers(false),
	m_fallbackHandler(nullptr)
{

}

MainRequestHandler::~MainRequestHandler()
{
	for (SubRequestHandler* pHandler : m_aSubRequestHandlers)
	{
		delete pHandler;
	}
	
	m_aSubRequestHandlers.clear();
	
	m_dirHandlerLookup.clear();
	m_hostHandlerLookup.clear();

	if (m_fallbackHandler)
	{
		delete m_fallbackHandler;
		m_fallbackHandler = nullptr;
	}
}

void MainRequestHandler::configure(const Configuration& configuration, Logger& logger)
{
	configureSubRequestHandlers(configuration, logger);
	
	// Note: this needs to be called after the optional downgrading of username might have been done
	//       so that log file permissions are as expected (i.e. the downgraded username)
	m_accessController.configure(configuration, logger);

	m_accessControlEnabled = configuration.getAccessControlEnabled();

	m_404NotFoundResponsesEnabled = configuration.get404NotFoundResponsesEnabled();
	
	if (configuration.isRedirectToHTTPSEnabled())
	{
		if (configuration.getHTTPv4PortNumber() != 80 ||
			configuration.getHTTPSv4PortNumber() != 443)
		{
			m_hostnamePortRewriteRequiredForHTTPSRedirect = true;
		}
	}
}

void MainRequestHandler::handleRequest(RequestConnection& requestConnection)
{
	Logger& logger = requestConnection.logger();
	const Configuration& configuration = *requestConnection.pThreadConfig->pConfiguration;
	
	logger.debug("Request received at handleRequest() for IP: %s.", requestConnection.ipInfo.getIPAddress().c_str());

	// first of all, see if we should agressively abort the connection due to a client ban
	if (!m_accessController.shouldAcceptConnection(requestConnection))
	{
		// we shouldn't accept it, so close it from our side...
		requestConnection.closeConnectionAndFreeSockets();
		return;
	}

	unsigned int connectionKeepAliveCount = 0;
	bool shouldKeepAliveNextTime = configuration.getKeepAliveEnabled();

	std::string requestString;
	// TODO: add timeout here...
	SocketRecvReturnCode recvRetCode = requestConnection.pConnectionSocket->recvSmart(requestString, 5);

	if (recvRetCode.type == eSockRecv_Error)
	{
		logger.error("Invalid data received from client: %s. Ignoring and closing socket connection.", requestConnection.ipInfo.getIPAddress().c_str());
	}
	else if (recvRetCode.type == eSockRecv_NoData)
	{
		// empty response was received. This likely means that the client closed the connection.
		// TODO: we could do something else here like wait with timeout, etc.. ?
		logger.info("Empty response received. Aborting connection.");
	}
	
	if (recvRetCode.type == eSockRecv_PeerClosed)
	{
		logger.debug("Closing connection due to peer close from IP : %s.", requestConnection.ipInfo.getIPAddress().c_str());
	}

	if (recvRetCode.type == eSockRecv_Error ||
		recvRetCode.type == eSockRecv_NoData ||
		recvRetCode.type == eSockRecv_PeerClosed ||
		recvRetCode.type == eSockRecv_TimedOutNoData)
	{
		requestConnection.closeConnectionAndFreeSockets();
		return;
	}
	
	bool closedKeepAliveConnectionDueToTimeout = false;

	do
	{
		WebRequest newRequest(requestString);

		if (!newRequest.parse(logger))
		{
			logger.error("Invalid Request received from client: %s. Ignoring and aborting connection.", requestConnection.ipInfo.getIPAddress().c_str());

			requestConnection.closeConnectionAndFreeSockets();
			return;
		}

		if (newRequest.getPath().empty())
		{
			requestConnection.closeConnectionAndFreeSockets();
			return;
		}
		
		if (!requestConnection.https)
		{
			requestConnection.connStatistics.httpRequests += 1;
		}
		else
		{
			requestConnection.connStatistics.httpsRequests += 1;
		}

		// web browsers (and even wget and CURL these days) seem to sanitise this sort of stuff much better up-front these days, but
		// there's still telnet / custom apps to allow arbitrary relative path requests, so let's attempt to kill these connections
		// immediately...
		// TODO: need to have a real think about how to do this properly and robustly, especially with regards to future
		//       subsite expansion functionality...
		if (newRequest.getPath().find("../") != std::string::npos ||
			newRequest.getPath().find("//") != std::string::npos ||
			newRequest.getPath().find('~') != std::string::npos ||
			newRequest.getPath().find(".php") != std::string::npos ||
			newRequest.getPath().find(".sql") != std::string::npos ||
			newRequest.getPath().find(".asp") != std::string::npos)
		{
			logger.warning("Probable malicious request: '%s' received from client: %s. Aborting connection.", newRequest.getPath().c_str(), requestConnection.ipInfo.getIPAddress().c_str());
			
			if (m_accessControlEnabled)
			{
				m_accessController.addFailedConnection(requestConnection, true);
			}

			requestConnection.closeConnectionAndFreeSockets();
			return;
		}

		shouldKeepAliveNextTime = configuration.getKeepAliveEnabled() && newRequest.getConnectionType() == WebRequest::eConnectionKeepAlive;

		WebRequestHandlerResult handleRequestResult;

		std::string requestPath = newRequest.getPath();
		
		// see if we need to redirect to HTTPS
		if (!requestConnection.https && configuration.isRedirectToHTTPSEnabled())
		{
			// TODO: GET params as well..
			
			std::string targetURL;
			
			// we might need to re-write the host for different port numbers...
			if (m_hostnamePortRewriteRequiredForHTTPSRedirect)
			{
				std::string newHost;
				// see if we have a port number on the existing host
				const std::string& requestedHost = newRequest.getHost();
				// TODO: cache the position?
				if (requestedHost.find(':') != std::string::npos)
				{
					// we do have a port, so extract just the host bit without it
					newHost = requestedHost.substr(0, requestedHost.find(':'));
				}
				else
				{
					// we don't have an existing port number, so can just replace
					newHost = requestedHost;
				}
				
				if (configuration.getHTTPSv4PortNumber() != 443)
				{
					// we need to add the non-standard port
					newHost += ":" + std::to_string(configuration.getHTTPSv4PortNumber());
				}
				
				targetURL = "https://" + newHost + newRequest.getPath();
			}
			else
			{
				// hostname should be the same, so just create the new URI...
				targetURL = "https://" + newRequest.getHost() + newRequest.getPath();
			}
			
			WebResponseGeneratorRedirect redirectResponse(targetURL, 301);
			
			WebResponseParams emptyParams(configuration, requestConnection.https);
			
			std::string responseString = redirectResponse.getResponseString(emptyParams);
			
			logger.debug("Sending redirect to HTTPS response for IP: %s.", requestConnection.ipInfo.getIPAddress().c_str());

			// send the response
			requestConnection.pConnectionSocket->send(responseString);
			
			// we can't re-use the connection...
			requestConnection.closeConnectionAndFreeSockets();
			
			return;
		}

		// TODO: authentication


		// knock the leading slash off so everything' relative to our root...
		requestPath = requestPath.substr(1);
		
		// route request through any registered sub request handlers we have
		
		bool wasFailedHostname = false;
		
		// try hosts first
		if (m_haveHostSubRequestHandlers)
		{
			SRHandlerMap::const_iterator itFind = m_hostHandlerLookup.find(newRequest.getHost());
			if (itFind != m_hostHandlerLookup.end())
			{
				SubRequestHandler* pSubRequestHandler = itFind->second;
				handleRequestResult = pSubRequestHandler->handleRequest(requestConnection, newRequest, requestPath);
			}
			else
			{
				wasFailedHostname = true;
			}
		}
		else if (m_haveDirSubRequestHandlers)
		{
			// work out if we have a first level sub-dir
			std::string directory;
			std::string remainingURI;

			if (URIHelpers::splitFirstLevelDirectoryAndRemainder(requestPath, directory, remainingURI))
			{
				// do nothing more
			}
			else
			{
				// maybe there wasn't a trailing slash...				
				directory = requestPath;
			}
			
			if (!directory.empty())
			{
				SRHandlerMap::const_iterator itFind = m_dirHandlerLookup.find(directory);
				if (itFind != m_dirHandlerLookup.end())
				{
					SubRequestHandler* pSubRequestHandler = itFind->second;
					handleRequestResult = pSubRequestHandler->handleRequest(requestConnection, newRequest, remainingURI);
				}
			}
		}

		if (m_fallbackHandler && !handleRequestResult.wasHandled)
		{
			handleRequestResult = m_fallbackHandler->handleRequest(requestConnection, newRequest, requestPath);
		}
		
		if (handleRequestResult.accessFailure && m_accessControlEnabled)
		{
			m_accessController.addFailedConnection(requestConnection, false);
		}

		// TODO: something better than this...
		if (!handleRequestResult.wasHandled)
		{
			if (m_accessControlEnabled)
			{
				// ignore certain things like 'favicon.ico'
				// TODO: this is silly really, but actual browsers (especially Safari) seem to be quite agressive with requesting favicon.ico,
				//       so it's fairly easy to rack up failed requests with a valid web browser (compared to bots), so giving it a bit of leeway
				//       is slightly advantagous for the moment. We also don't really want to log all those invalid requests, as even for valid usage
				//       there are quite a lot of them (clients don't seem to remember the 404 response, so regularly ask again on subsequent requests).
				
				bool potentiallyMalicious = wasFailedHostname;
				m_accessController.addFailedConnection(requestConnection, potentiallyMalicious);
				
				if (requestPath != "favicon.ico")
				{
					// for the moment, don't log these, as valid browsers send these requests quite agressively...
					if (requestPath.size() < 10000)
					{
						logger.warning("Unhandled request: %s for host: %s from client: %s", requestPath.c_str(), newRequest.getHost().c_str(), requestConnection.ipInfo.getIPAddress().c_str());
					}
				}
			}
			else
			{
				if (requestPath != "favicon.ico")
				{
					logger.info("Unhandled request: %s for host: %s from client: %s", requestPath.c_str(), newRequest.getHost().c_str(), requestConnection.ipInfo.getIPAddress().c_str());
				}
			}

			// Note: if we have access control enabled, we need to return *something* in this situation,
			//       as otherwise, clients send multiple requests for the same single request understandably thinking
			//       there's something wrong and re-trying, so we need to return a 404 to prevent this from happening
			//       and allowing access control to work properly...

			if (m_404NotFoundResponsesEnabled)
			{
				if (wasFailedHostname)
				{
					// Note: in the case of unknown/invalid virtual hosts, RFC 2616 (section 5.2) states that
					//       the server response to these MUST be 400 (Bad Request) error.
					//       However, I don't like that, as it potentially allows easier sniffing of valid/invalid
					//       hosts supported, so for the moment, just return 404.
				}
				
				WebResponseParams responseParams(configuration, requestConnection.https);
				WebResponseGeneratorBasicText textResponse(404, "Not found.");

				std::string responseString = textResponse.getResponseString(responseParams);

				// send the response
				requestConnection.pConnectionSocket->send(responseString);
			}
			else
			{
				// otherwise we (badly in terms of HTTP compliance and niceness) don't want to respond at all...
				break;
			}
		}

		shouldKeepAliveNextTime = shouldKeepAliveNextTime && connectionKeepAliveCount++ < configuration.getKeepAliveLimit();

		if (shouldKeepAliveNextTime)
		{
			// do a receive with a timeout, such that we can abort this connection and close it if it's not being used
			requestString = "";

			recvRetCode = requestConnection.pConnectionSocket->recvWithTimeout(requestString, configuration.getKeepAliveTimeout());
			if (recvRetCode.type == eSockRecv_NoData ||
				recvRetCode.type == eSockRecv_PeerClosed ||
				recvRetCode.type == eSockRecv_Error)
			{
				logger.debug("Request socket Keep Alive receive failed/timed out. Closing");
				// TODO: we probably want to try and isolate this to just the timeout event, and not a failure as well as currently happens?
				closedKeepAliveConnectionDueToTimeout = true;
				break;
			}
			else
			{
				logger.debug("Request socket Keep Alive received further request: %i", connectionKeepAliveCount);
			}
		}
	}
	while (shouldKeepAliveNextTime);
	
	if (closedKeepAliveConnectionDueToTimeout)
	{
		// attempt to nicely tell the client that we're closing the connection...
		WebResponseParams responseParams(configuration, requestConnection.https);
		
		WebResponseGeneratorBasicText response(408, "timeout");
		
		// so as to send Connection: close
		responseParams.keepAliveEnabled = false;
		
		std::string stringResponse = response.getResponseString(responseParams);
		
		// don't worry if we can't send - it's likely the client will close it anyway...
		requestConnection.pConnectionSocket->send(stringResponse, ConnectionSocket::SEND_IGNORE_FAILURES);
	}

	requestConnection.closeConnectionAndFreeSockets();
}

bool MainRequestHandler::configureSubRequestHandlers(const Configuration& configuration, Logger& logger)
{
	const std::vector<Configuration::SiteConfig>& aSiteConfigs = configuration.getSiteConfigs();
	
	for (const Configuration::SiteConfig& siteConfig : aSiteConfigs)
	{
		SubRequestHandler* pNewRequestHandler = nullptr;
		
		if (siteConfig.m_type == "photos")
		{
			pNewRequestHandler = new PhotosRequestHandler();
		}
		else if (siteConfig.m_type == "files")
		{
			pNewRequestHandler = new FilesRequestHandler();
		}
		else if (siteConfig.m_type == "proxy")
		{
			pNewRequestHandler = new ProxyRequestHandler();
		}
		
		if (pNewRequestHandler)
		{
			pNewRequestHandler->configure(siteConfig, configuration, logger);
		}
		
		std::string siteDefConfigType;
		std::string siteDefConfigValue;
		StringHelpers::splitInTwo(siteConfig.m_definition, siteDefConfigType, siteDefConfigValue, ":");
		
		if (!siteDefConfigType.empty() && !siteDefConfigValue.empty())
		{
			if (siteDefConfigType == "dir")
			{
				// it's a directory
				
				m_dirHandlerLookup[siteDefConfigValue] = pNewRequestHandler;
				
				m_aSubRequestHandlers.emplace_back(pNewRequestHandler);
				
				m_haveDirSubRequestHandlers = true;
			}
			else if (siteDefConfigType == "host")
			{
				// it's a hostname
				
				m_hostHandlerLookup[siteDefConfigValue] = pNewRequestHandler;
				
				m_aSubRequestHandlers.emplace_back(pNewRequestHandler);
				
				m_haveHostSubRequestHandlers = true;
			}
			else if (siteDefConfigType == "*")
			{
				// it's a wildcard fallback

				if (m_fallbackHandler)
				{
					logger.error("A wildcard fallback handler already exists.");
				}
				else
				{
					m_fallbackHandler = pNewRequestHandler;
				}
			}
			else
			{
				// unknown...
				logger.error("Invalid config definition type specified for site: %s", siteConfig.m_name.c_str());
				if (pNewRequestHandler)
				{
					delete pNewRequestHandler;
				}
			}
		}
		else if (siteDefConfigType.empty() && siteDefConfigValue.empty() && siteConfig.m_definition == "*")
		{
			// allow a single fallback type without a config value (i.e. no ':' char in the string)
			if (m_fallbackHandler)
			{
				logger.error("A wildcard fallback handler already exists.");
			}
			else
			{
				m_fallbackHandler = pNewRequestHandler;
			}
		}
		else
		{
			logger.error("Invalid config definition specified for site: %s", siteConfig.m_name.c_str());
			if (pNewRequestHandler)
			{
				delete pNewRequestHandler;
			}
		}
	}
	
	// TODO: what are we doing about return codes here? some of them likely should be hard return falses, others might be soft failures...
	return true;
}
