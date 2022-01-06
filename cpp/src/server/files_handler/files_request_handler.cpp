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

#include "files_request_handler.h"

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

FilesRequestHandler::FilesRequestHandler() :
    SubRequestHandler(),
	m_authenticationEnabled(false),
	m_allowDirectoryListing(false)
{
	
}

FilesRequestHandler::~FilesRequestHandler()
{
	
}

void FilesRequestHandler::configure(const Configuration::SiteConfig& siteConfig, const Configuration& mainConfig, Logger& logger)
{
	m_authenticationEnabled = siteConfig.getParamAsBool("authenticationEnabled", false);
	if (m_authenticationEnabled)
	{
		m_authenticationController.configure(siteConfig, logger);
	}
	
	m_basePath = siteConfig.getParam("basePath");
	
	m_allowDirectoryListing = siteConfig.getParamAsBool("allowDirectoryListing", false);

	m_defaultFile = siteConfig.getParam("defaultFile");
}

WebRequestHandlerResult FilesRequestHandler::handleRequest(RequestConnection& requestConnection, const WebRequest& request, const std::string& refinedURI)
{
	Logger& logger = requestConnection.logger();
	const Configuration& configuration = *requestConnection.pThreadConfig->pConfiguration;

	WebRequestAuthenticationState requestAuthenticationState;
	
	if (m_authenticationEnabled)
	{
		requestAuthenticationState = m_authenticationController.getAuthenticationStateFromRequest(requestConnection, request, "sessionID");
	}
	
	// we received a relative path to this handler

	std::string requestPath = refinedURI;

	WebRequestHandlerResult handleRequestResult;

	WebResponseParams responseParams(configuration, requestConnection.https);
	
	std::string responseString;

	// TODO: make this more robust
	if (requestPath.find('.') == std::string::npos && !m_defaultFile.empty())
	{
		requestPath = URIHelpers::combineURIs(requestPath, m_defaultFile);
	}


	// for the moment, always just use WebResponseAdvancedBinaryFile
	// to send the file, as we're not modifying the contents...

	std::string fullPath = FileHelpers::combinePaths(m_basePath, requestPath);

	WebResponseAdvancedBinaryFile fileResponse(fullPath);

	WebResponseAdvancedBinaryFile::ValidationResult validationResult = fileResponse.validateResponse();

	if (validationResult == WebResponseAdvancedBinaryFile::eFileNotFound)
	{
		WebResponseGeneratorBasicText responseGen(404, "File not found.");

		responseString = responseGen.getResponseString(responseParams);
	}
	else if (validationResult == WebResponseAdvancedBinaryFile::eFileTypeNotSupported)
	{
		WebResponseGeneratorBasicText responseGen(503, "File type not supported.");

		responseString = responseGen.getResponseString(responseParams);
	}
	else
	{
		// process the request

		responseParams.setCacheControlParams(WebResponseParams::CC_PUBLIC | WebResponseParams::CC_MAX_AGE, 60 * 24 * 25);

		// TODO: the return of false from this can be due to many different things,
		//       so it's not clear how to handle the different issues...
		if (!fileResponse.sendResponse(requestConnection.pConnectionSocket, responseParams))
		{
			// Safari (on iOS in particular) seems to bizarrely shut down sockets mid-transfer,
			// almost as if it knows it has the image already, but only when keep-alive is enabled which is a bit strange...
			logger.debug("Can't send binary file: %s. Connection was closed mid transfer by the remote side.", fullPath.c_str());

			handleRequestResult.inError = true;
			handleRequestResult.wasHandled = true;
			return handleRequestResult;
		}

		handleRequestResult.wasHandled = true;
		return handleRequestResult;
	}

	// send the response
	requestConnection.pConnectionSocket->send(responseString);

	handleRequestResult.wasHandled = true;
	return handleRequestResult;
}
