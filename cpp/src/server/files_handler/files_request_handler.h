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

#ifndef FILES_REQUEST_HANDLER_H
#define FILES_REQUEST_HANDLER_H

#include "sub_request_handler.h"

#include "authentication_controller.h"

#include "status_service.h"

class Logger;
struct WebRequestAuthenticationState;

class FilesRequestHandler : public SubRequestHandler
{
public:
	FilesRequestHandler();
	virtual ~FilesRequestHandler();
	
	virtual void configure(const Configuration::SiteConfig& siteConfig, const Configuration& mainConfig, Logger& logger) override;

	virtual WebRequestHandlerResult handleRequest(RequestConnection& requestConnection, const WebRequest& request, const std::string& refinedURI) override;
	
protected:
	bool						m_authenticationEnabled;
	AuthenticationController	m_authenticationController;
	
	bool						m_allowDirectoryListing;
	std::string					m_defaultFile;
	
	std::string					m_basePath;
};

#endif // FILES_REQUEST_HANDLER_H
