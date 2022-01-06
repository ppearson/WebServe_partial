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

#ifndef PHOTOS_REQUEST_HANDLER_H
#define PHOTOS_REQUEST_HANDLER_H

#include "sub_request_handler.h"

#include "photo_catalogue.h"

#include "photos_html_helpers.h"

#include "photos_common.h"

#include "authentication_controller.h"

#include "status_service.h"

class Logger;
struct WebRequestAuthenticationState;

class PhotosRequestHandler : public SubRequestHandler
{
public:
	PhotosRequestHandler();
	virtual ~PhotosRequestHandler();

	virtual void configure(const Configuration::SiteConfig& siteConfig, const Configuration& mainConfig, Logger& logger) override;

	virtual WebRequestHandlerResult handleRequest(RequestConnection& requestConnection, const WebRequest& request, const std::string& refinedURI) override;

protected:
	WebRequestHandlerResult handleLoginRequest(RequestConnection& requestConnection, const WebRequest& request);
	WebRequestHandlerResult handlePhotostreamRequest(RequestConnection& requestConnection, const WebRequest& request,
													 const WebRequestAuthenticationState& authenticationState);
	WebRequestHandlerResult handleDatesRequest(RequestConnection& requestConnection, const WebRequest& request,
												const WebRequestAuthenticationState& authenticationState, const std::string& refinedURI);
	WebRequestHandlerResult handleLocationsRequest(RequestConnection& requestConnection, const WebRequest& request,
												const WebRequestAuthenticationState& authenticationState);
	WebRequestHandlerResult handleStatusRequest(RequestConnection& requestConnection, const WebRequest& request,
												const WebRequestAuthenticationState& authenticationState);

	DateParams getDateParamsFromRequest(const WebRequest& request, bool checkURLPath, const std::string& refinedURI) const;

protected:

protected:
	std::string					m_photosBasePath;
	std::string					m_mainWebContentPath;

	bool						m_lazyPhotoLoadingEnabled;

	bool						m_authenticationEnabled;
	AuthenticationController	m_authenticationController;
	bool						m_authenticationRequired;

	// used for <base href> tag in all HTML to make all other paths relative and therefore dynamic depending on whether a domain URL address
	// or directory URL address
	std::string					m_htmlBaseHRef; // <base href> tag - needs to be "" for domain (no path)

	// relative path of Photos site - from HTTP's perspective (i.e. redirects, direct addressing)
	std::string					m_relativePath; // relative parth of Photos site - needs to be "/" for domain (no path)

	PhotoCatalogue				m_photoCatalogue;

	PhotosHTMLHelpers			m_photosHTMLHelpers;
	
	StatusService				m_statusService;
};

#endif // PHOTOS_REQUEST_HANDLER_H
