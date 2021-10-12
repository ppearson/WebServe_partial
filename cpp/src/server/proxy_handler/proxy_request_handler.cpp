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

#include "proxy_request_handler.h"

#include "web_response_generators.h"
#include "web_server_common.h"
#include "web_request.h"
#include "web_request_common.h"
#include "web_response_advanced_binary_file.h"

#include "configuration.h"
#include "utils/uri_helpers.h"
#include "utils/file_helpers.h"
#include "utils/string_helpers.h"
#include "utils/logger.h"

// Reverse Proxy implementation

ProxyRequestHandler::ProxyRequestHandler() :
    SubRequestHandler()
{
	
}

ProxyRequestHandler::~ProxyRequestHandler()
{
	
}

void ProxyRequestHandler::configure(const Configuration::SiteConfig& siteConfig, const Configuration& mainConfig, Logger& logger)
{
	std::string target = siteConfig.getParam("target");
	
	if (target.substr(0, 7) != "http://")
	{
		logger.error("Proxy request handler cannot handle target of: %s", target.c_str());
		return;
	}
	
	std::string remainder = target.substr(7);
	size_t portPos = remainder.find(':');
	size_t nextSlashPos = remainder.find('/');
	if (portPos == std::string::npos)
	{
		// no port specified, so use default
		m_targetPort = 80;
		
		if (nextSlashPos == std::string::npos)
		{
			m_targetHostname = remainder;
			m_targetPath = "/";
		}
		else
		{
			// extract the hostname
			m_targetHostname = remainder.substr(0, nextSlashPos);
			m_targetPath = remainder.substr(nextSlashPos);
		}
	}
	else
	{
		// we have a port number
		
		if (nextSlashPos == std::string::npos)
		{
			// there's no trailing slash/path
			m_targetHostname = remainder.substr(0, portPos);
			
			std::string portString = remainder.substr(portPos + 1);
			m_targetPort = std::atoi(portString.c_str());
			
			m_targetPath = "/";
		}
		else
		{
			// we have a port number then a slash/path
			
			m_targetHostname = remainder.substr(0, portPos);
			
			std::string portString = remainder.substr(portPos + 1, nextSlashPos - portPos + 1);
			m_targetPort = std::atoi(portString.c_str());
			
			m_targetPath = remainder.substr(nextSlashPos);
		}
	}
	
	m_headerRewriter.initialise(m_targetHostname, m_targetPort, m_targetPath);
	
	logger.notice("Proxy request handler configured with port: %i, hostname: %s, path: %s", m_targetPort, m_targetHostname.c_str(), m_targetPath.c_str());
}

WebRequestHandlerResult ProxyRequestHandler::handleRequest(RequestConnection& requestConnection, const WebRequest& request, const std::string& refinedURI)
{
	Logger& logger = requestConnection.logger();

	// we received a relative path to this handler

	const std::string& requestPath = refinedURI;
	
	WebRequestHandlerResult handleRequestResult;
	
	Socket proxySocket(&logger, m_targetHostname, m_targetPort);
	if (!proxySocket.connect())
	{
		logger.error("Error connecting to proxy target.");
		return handleRequestResult;
	}
	
	std::string rewrittenHTTPRequest = m_headerRewriter.generateRewrittenProxyHeaderRequest(request, requestPath);
	
	if (!proxySocket.send(rewrittenHTTPRequest))
	{
		logger.error("Couldn't send request to proxy target.");
		return handleRequestResult;
	}
	
	std::string responseString;
	if (proxySocket.recvSmart(responseString).type == eSockRecv_Error)
	{
		logger.error("Error receiving response from proxy target.");
		return handleRequestResult;
	}

	// send the response
	requestConnection.pConnectionSocket->send(responseString);

	handleRequestResult.wasHandled = true;
	return handleRequestResult;
}
