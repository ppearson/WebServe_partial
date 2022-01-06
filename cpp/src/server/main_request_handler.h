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

#ifndef MAIN_REQUEST_HANDLER_H
#define MAIN_REQUEST_HANDLER_H

#include <string>
#include <map>
#include <vector>

#include "request_handler_common.h"
#include "access_controller.h"

struct RequestConnection;
class Configuration;
class SubRequestHandler;
class Logger;

class MainRequestHandler
{
public:
	MainRequestHandler();
	~MainRequestHandler();

	// TODO: pass through some kind of configuration config context instead, containing both?
	void configure(const Configuration& configuration, Logger& logger);

	void handleRequest(RequestConnection& requestConnection);
	
protected:
	bool configureSubRequestHandlers(const Configuration& configuration, Logger& logger);

protected:
	typedef std::map<std::string, SubRequestHandler*> SRHandlerMap;

protected:
	bool						m_accessControlEnabled;
	AccessController			m_accessController;

	bool						m_404NotFoundResponsesEnabled;
	
	// whether we need to also alter port numbers in the hostname
	// when redirecting HTTP to HTTPS
	bool						m_hostnamePortRewriteRequiredForHTTPSRedirect;

	// TODO: move this stuff into the SubRequestHandler?
	SubSiteType					m_photosType;
	std::string					m_photosItemName; // this is the name of either the directory or host depending on the type

	// vector
	std::vector<SubRequestHandler*>			m_aSubRequestHandlers;
	
	
	// more specific lists per type of the above...	
	bool						m_haveDirSubRequestHandlers;
	SRHandlerMap				m_dirHandlerLookup;
	
	bool						m_haveHostSubRequestHandlers;
	SRHandlerMap				m_hostHandlerLookup;
};

#endif // MAIN_REQUEST_HANDLER_H
