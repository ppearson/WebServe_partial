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

#include "photos_request_handler.h"

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

#include "photos_handler/photos_html_helpers.h"

#include <cstdio>

PhotosRequestHandler::PhotosRequestHandler() :
	SubRequestHandler()
{
	
}

PhotosRequestHandler::~PhotosRequestHandler()
{
	m_statusService.stop();
}

void PhotosRequestHandler::configure(const Configuration::SiteConfig& siteConfig, const Configuration& mainConfig, Logger& logger)
{
	m_photosBasePath = siteConfig.getParam("photosBasePath");
	m_mainWebContentPath = siteConfig.getParam("webContentPath");
	m_lazyPhotoLoadingEnabled = siteConfig.getParamAsBool("lazyPhotoLoadingEnabled", true);

	// TODO: there's duplication here with MainRequestHandler - ought to try and re-use that functionality or pass down the final results...
	std::string siteDefConfigType;
	std::string siteDefConfigValue;
	if (StringHelpers::splitInTwo(siteConfig.m_definition, siteDefConfigType, siteDefConfigValue, ":"))
	{
		if (siteDefConfigType == "dir")
		{
			// set things up so our URI is a directory
			m_htmlBaseHRef = "<base href=\"/" + siteDefConfigValue + "/\"/>";
			m_relativePath = "/" + siteDefConfigValue + "/";
		}
		else if (siteDefConfigType == "host")
		{
			// set things up so our URI is a hostname with no directory
			m_htmlBaseHRef = "<base href=\"/\"/>";
			m_relativePath = "/";
		}
	}

	m_authenticationEnabled = siteConfig.getParamAsBool("authenticationEnabled", false);
	if (m_authenticationEnabled)
	{
		m_authenticationController.configure(siteConfig, logger);
	}
	m_authenticationRequired = siteConfig.getParamAsBool("authenticationRequired", false);

	m_photoCatalogue.buildPhotoCatalogue(m_photosBasePath, logger);

	m_photosHTMLHelpers.setMainWebContentPath(m_mainWebContentPath);
	
	m_statusService.start();
}

WebRequestHandlerResult PhotosRequestHandler::handleRequest(RequestConnection& requestConnection, const WebRequest& request, const std::string& refinedURI)
{
	Logger& logger = requestConnection.logger();
	const Configuration& configuration = *requestConnection.pThreadConfig->pConfiguration;

	WebRequestAuthenticationState requestAuthenticationState;
	
	requestConnection.pStatusService = &m_statusService;

	if (m_authenticationEnabled)
	{
		requestAuthenticationState = m_authenticationController.getAuthenticationStateFromRequest(requestConnection, request, "sessionID");
	}

	// we received a relative path to photos/<...>

	const std::string& requestPath = refinedURI;

	WebRequestHandlerResult handleRequestResult;

	WebResponseParams responseParams(configuration, requestConnection.https);

	// see if it's a file request - if so, short-circuit it to handle it immediately
	// TODO: make this more robust
	if (requestPath.find('.') != std::string::npos)
	{
		// is file
		// TODO: improve all this!
		std::string extension = URIHelpers::getFileExtension(requestPath);
		
		if (m_authenticationRequired)
		{
			// if authentication is required, only allow .css for stylesheet if not authenticated
			if (!requestAuthenticationState.isAuthenticated())
			{
				if (extension != "css")
				{
					// if it's not .css, don't allow...
					WebResponseParams responseParams(configuration, requestConnection.https);
					WebResponseGeneratorBasicText textResponse(404, "Not found.");
	
					std::string responseString = textResponse.getResponseString(responseParams);
	
					// send the response
					requestConnection.pConnectionSocket->send(responseString);
					
					handleRequestResult.wasHandled = true;
					return handleRequestResult;
				}
			}
		}
		
		// currently photo items themselves are jpg files, everything else is web content
		std::string fullPath;
		bool isLargeBinary = false;
		if (extension == "jpg")
		{
			fullPath = m_photosBasePath + requestPath;
			isLargeBinary = true;

			responseParams.useChunkedLargeFiles = configuration.getChunkedTransferJPEGsEnabled();
		}
		else
		{
			fullPath = m_mainWebContentPath + requestPath;
		}

		if (isLargeBinary)
		{
			// if it's a large binary, we handle it separately in order to send it
			// more efficiently down the socket.

			responseParams.setCacheControlParams(WebResponseParams::CC_PUBLIC | WebResponseParams::CC_MAX_AGE, 60 * 24 * 25);

			WebResponseAdvancedBinaryFile fileResponse(fullPath);

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
		}
		else
		{
			// just handle it as string / small stuff
			// TODO: is this really worth doing separately any more?

			WebResponseGeneratorFile fileResponse(fullPath);

			// TODO: during development we'll keep this max-age value small, but eventually this should be increased significantly...
			responseParams.setCacheControlParams(WebResponseParams::CC_PUBLIC | WebResponseParams::CC_MAX_AGE, 60 * 24 * 2);

			std::string responseString = fileResponse.getResponseString(responseParams);

			// send the response
			requestConnection.pConnectionSocket->send(responseString);
		}

		handleRequestResult.wasHandled = true;
		return handleRequestResult;
	}

	// work out if we have a further first level sub-dir
	std::string nextLevel;
	std::string remainingURI;

	bool unknown = false;

	std::string responseString;

	if (!URIHelpers::splitFirstLevelDirectoryAndRemainder(requestPath, nextLevel, remainingURI))
	{
		nextLevel = requestPath;
	}
	
	if (nextLevel == "login")
	{
		return handleLoginRequest(requestConnection, request);
	}
	
	if (m_authenticationRequired)
	{
		if (!requestAuthenticationState.isAuthenticated())
		{
			// don't allow...
			WebResponseParams responseParams(configuration, requestConnection.https);
			WebResponseGeneratorBasicText textResponse(404, "Not found.");

			std::string responseString = textResponse.getResponseString(responseParams);

			// send the response
			requestConnection.pConnectionSocket->send(responseString);
			
			handleRequestResult.wasHandled = true;
			return handleRequestResult;
		}
	}
	
	if (nextLevel == "photostream")
	{
		return handlePhotostreamRequest(requestConnection, request, requestAuthenticationState);
	}
	else if (nextLevel == "dates")
	{
		// dates supports further directory levels...
		return handleDatesRequest(requestConnection, request, requestAuthenticationState, remainingURI);
	}
	else if (nextLevel == "locations")
	{
		return handleLocationsRequest(requestConnection, request, requestAuthenticationState);
	}
	else if (nextLevel == "status")
	{
		return handleStatusRequest(requestConnection, request, requestAuthenticationState);
	}
/*		
	else if (nextLevel == "gallery_simple")
	{
		PhotoQueryEngine::QueryParams queryParams;
		PhotoResultsPtr photoResults = m_photoCatalogue.getQueryEngine().getPhotoResults(queryParams);

		std::string photosListHTML = PhotosHTMLHelpers::getSimpleImageListWithinCustomDivTagWithStyle(photoResults, "gallery_item", 0, 0, 500, false);
		std::string item = "test/gallery_simple3.tmpl";

		WebResponseGeneratorTemplateFile galleryResponse(FileHelpers::combinePaths(m_mainWebContentPath, item), photosListHTML);
		responseString = galleryResponse.getResponseString(responseParams);
	}
	else if (nextLevel == "gallery_advanced")
	{
		std::string photosListHTML = PhotosHTMLHelpers::getSimpleElementListWithinCustomDivTagWithBGImage(m_photoCatalogue.getRawItems(), "thumbnail", "thumbnail-wrapper");
		std::string item = "test/gallery_advanced2.tmpl";

		WebResponseGeneratorTemplateFile galleryResponse(FileHelpers::combinePaths(m_mainWebContentPath, item), photosListHTML);
		responseString = galleryResponse.getResponseString(responseParams);
	}
	else if (nextLevel == "slide_show")
	{
		PhotoQueryEngine::QueryParams queryParams;
		PhotoResultsPtr photoResults = m_photoCatalogue.getQueryEngine().getPhotoResults(queryParams);

		std::string photosListJS = PhotosHTMLHelpers::getPhotoSwipeJSItemList(photoResults, 0, 0);
		std::string item = "test/slideshow1.tmpl";

		WebResponseGeneratorTemplateFile slideShowResponse(FileHelpers::combinePaths(m_mainWebContentPath, item), photosListJS);
		responseString = slideShowResponse.getResponseString(responseParams);
	}
*/	else
	{
		unknown = true;
	}

	if (unknown)
	{
		if (m_authenticationRequired)
		{
			if (!requestAuthenticationState.isAuthenticated())
			{
				// don't allow...
				// TODO: redirect to login?
				WebResponseParams responseParams(configuration, requestConnection.https);
				WebResponseGeneratorBasicText textResponse(404, "Not found.");

				std::string responseString = textResponse.getResponseString(responseParams);

				// send the response
				requestConnection.pConnectionSocket->send(responseString);
				
				handleRequestResult.wasHandled = true;
				return handleRequestResult;
			}
		}
		
		std::string siteNavHeaderHTML = m_photosHTMLHelpers.generateMainSitenavCode(PhotosHTMLHelpers::GenMainSitenavCodeParams(false, false, ""));

		if (requestAuthenticationState.state == WebRequestAuthenticationState::eSAuthenticated)
		{
			siteNavHeaderHTML += "<br><br>Logged in.<br>\n";
		}
		else
		{
			siteNavHeaderHTML += "<br><br>Logged out.<br>\n";
		}
		WebResponseGeneratorTemplateFile responseGen(FileHelpers::combinePaths(m_mainWebContentPath, "photos_main.tmpl"), m_htmlBaseHRef, siteNavHeaderHTML);
		responseString = responseGen.getResponseString(responseParams);
	}

	// send the response
	requestConnection.pConnectionSocket->send(responseString);

	handleRequestResult.wasHandled = true;
	return handleRequestResult;
}

WebRequestHandlerResult PhotosRequestHandler::handleLoginRequest(RequestConnection& requestConnection, const WebRequest& request)
{
	const Configuration& configuration = *requestConnection.pThreadConfig->pConfiguration;

	WebResponseParams responseParams(configuration, requestConnection.https);

	WebRequestHandlerResult handleRequestResult;

	std::string responseString;

	if (request.getRequestType() == WebRequest::eRequestGET)
	{
		// it's a GET type, so display the login form

		std::string siteNavHeaderHTML = m_photosHTMLHelpers.generateMainSitenavCode(PhotosHTMLHelpers::GenMainSitenavCodeParams(false, false, ""));
		WebResponseGeneratorTemplateFile responseGen(FileHelpers::combinePaths(m_mainWebContentPath, "login.tmpl"), m_htmlBaseHRef, siteNavHeaderHTML);
		responseString = responseGen.getResponseString(responseParams);
	}
	else if (request.getRequestType() == WebRequest::eRequestPOST)
	{
		// it's a POST type, so we need to pass this on to the AuthenticationController...

		AuthenticationController::LoginResult loginResult = m_authenticationController.validateLoginCredentials(requestConnection, request);

		if (loginResult.type == AuthenticationController::eLRSuccess)
		{
			// logged in successfully, so send a redirect with a setCookie, back to the main page

			WebResponseGeneratorRedirectSetCookie responseGen(m_relativePath, "sessionID", loginResult.newSessionID);

			responseGen.setCookieHttpOnly(true);
			responseGen.setCookieMaxAge(loginResult.newSessionExpiry);

			responseString = responseGen.getResponseString(responseParams);
		}
		else
		{
			requestConnection.logger().error("Invalid login attempt from IP: %s", requestConnection.ipInfo.getIPAddress().c_str());
			
			WebResponseGeneratorBasicText responseGen(503, "Invalid login credentials.");
			
			responseString = responseGen.getResponseString(responseParams);
			
			handleRequestResult.accessFailure = true;
		}
	}
	else
	{
		// we don't accept anything else for the moment...
		
	}

	// send the response
	requestConnection.pConnectionSocket->send(responseString);

	handleRequestResult.wasHandled = true;
	return handleRequestResult;
}

WebRequestHandlerResult PhotosRequestHandler::handlePhotostreamRequest(RequestConnection& requestConnection, const WebRequest& request,
																	   const WebRequestAuthenticationState& authenticationState)
{
	const Configuration& configuration = *requestConnection.pThreadConfig->pConfiguration;

	WebResponseParams responseParams(configuration, requestConnection.https);

	WebRequestHandlerResult handleRequestResult;

	std::string responseString;

	unsigned int perPage = request.getParamAsInt("perPage", 100);
	unsigned int startIndex = request.getParamAsInt("startIndex", 0);
	unsigned int slideShow = request.getParamAsInt("slideshow", 0);

	// TODO: using a bool from the above would be a bit better, but...
	bool isSlideShow = slideShow == 1;

	PhotoQueryEngine::QueryParams queryParams;

	int sortOrderType = request.getParamOrCookieAsInt("sortOrder", "photostream_sortOrderIndex", 1);
	queryParams.setSortOrderType(sortOrderType == 0 ? PhotoQueryEngine::QueryParams::eSortOldestFirst : PhotoQueryEngine::QueryParams::eSortYoungestFirst);

	// TODO: do this properly...
	queryParams.setPermissionType((PhotoQueryEngine::QueryParams::PermissionType)authenticationState.authenticationPermission.level);
	
	bool wantSLR = request.getParamOrCookieAsInt("typeSLR", "photostream_typeSLR", 1) == 1;
	bool wantDrone = request.getParamOrCookieAsInt("typeDrone", "photostream_typeDrone", 0) == 1;
	
	queryParams.setSourceTypesFlag(PhotoQueryEngine::QueryParams::buildSourceTypesFlags(wantSLR, wantDrone));
	PhotoResultsPtr photoResults = m_photoCatalogue.getQueryEngine().getPhotoResults(queryParams);

	// check that we can actually get the requested index
	if (startIndex > 0 && startIndex >= photoResults->getAllResults().size())
	{
		// if not, short-circuit with a redirect to the main photostream page
		WebResponseGeneratorRedirect redirectResponse("photostream/");

		responseString = redirectResponse.getResponseString(responseParams);

		// send the response
		requestConnection.pConnectionSocket->send(responseString);

		handleRequestResult.wasHandled = true;
		return handleRequestResult;
	}
	
	// TODO: generateMainSitenavCode() shows up in profiles - can we cache it?
	PhotosHTMLHelpers::GenMainSitenavCodeParams params(!isSlideShow, !isSlideShow, "photostream_");
	std::string siteNavHeaderHTML = m_photosHTMLHelpers.generateMainSitenavCode(params);

	std::string photosListHTML;
	std::string paginationHTML;

	if (slideShow == 1)
	{
		// slideshow version of same thing
		// TODO: is obeying the pagination stuff wanted/worth it?
		//       can we jump to a particular photo index in some other way?

		std::string contentAndPaginationHTML = "<a href=\"javascript:openPhotoSwipe();\">slide show overlay</a><br><br>\n";

		if (perPage > 0)
		{
			unsigned int totalPhotos = photoResults->getAllResults().size();
			contentAndPaginationHTML += PhotosHTMLHelpers::getPaginationCode("photostream/", request, totalPhotos, startIndex, perPage, true, true);
		}

		std::string photosListJS = PhotosHTMLHelpers::getPhotoSwipeJSItemList(photoResults->getAllResults(), startIndex, perPage);
		
		WebResponseGeneratorTemplateFile responseGen(FileHelpers::combinePaths(m_mainWebContentPath, "photostream_slideshow.tmpl"),
													 m_htmlBaseHRef, siteNavHeaderHTML,
													 contentAndPaginationHTML, photosListJS);

		responseString = responseGen.getResponseString(responseParams);
	}
	else
	{
		unsigned int thumbnailSize = request.getParamOrCookieAsInt("thumbnailSize", "photostream_thumbnailSizeValue", 500);
		
		// normal gallery
		if (perPage > 0)
		{
			unsigned int totalPhotos = photoResults->getAllResults().size();
			paginationHTML = PhotosHTMLHelpers::getPaginationCode("photostream/", request, totalPhotos, startIndex, perPage, true, false);
		}

		bool lazyLoad = m_lazyPhotoLoadingEnabled && request.getParamOrCookieAsInt("lazyLoading", "photostream_lazyLoading", 1) == 1;

		std::string slideshowURL;
		bool useSlideshowURL = request.getCookieAsInt("photostream_galleryLinkToSlideshow", 1) == 1;
		if (useSlideshowURL)
		{
			std::string currentPageParams = request.getParamsAsGETString(false);
			slideshowURL = "photostream/?" + currentPageParams + "&slideshow=1&";
		}

		photosListHTML = PhotosHTMLHelpers::getSimpleImageListWithinCustomDivTagWithStyle(photoResults->getAllResults(), "gallery_item",
																						  startIndex, perPage, thumbnailSize, lazyLoad,
																						  slideshowURL);

		WebResponseGeneratorTemplateFile responseGen(FileHelpers::combinePaths(m_mainWebContentPath, "photostream_gallery.tmpl"),
													 m_htmlBaseHRef,
													 siteNavHeaderHTML,
													 photosListHTML,
													 paginationHTML);

		responseString = responseGen.getResponseString(responseParams);
	}

	// send the response
	requestConnection.pConnectionSocket->send(responseString);

	handleRequestResult.wasHandled = true;
	return handleRequestResult;
}

WebRequestHandlerResult PhotosRequestHandler::handleDatesRequest(RequestConnection& requestConnection, const WebRequest& request,
																 const WebRequestAuthenticationState& authenticationState, const std::string& refinedURI)
{
	const Configuration& configuration = *requestConnection.pThreadConfig->pConfiguration;

	WebResponseParams responseParams(configuration, requestConnection.https);

	WebRequestHandlerResult handleRequestResult;

	bool specifyDateValsAsDirs = true;

	std::string responseString;
	
	unsigned int slideShow = request.getParamAsInt("slideshow", 0);

	PhotoQueryEngine::QueryParams queryParams;
	bool wantSLR = request.getParamOrCookieAsInt("typeSLR", "dates_typeSLR", 1) == 1;
	bool wantDrone = request.getParamOrCookieAsInt("typeDrone", "dates_typeDrone", 0) == 1;
	
	// TODO: do this properly...
	queryParams.setPermissionType((PhotoQueryEngine::QueryParams::PermissionType)authenticationState.authenticationPermission.level);
	
	unsigned int sourceTypeFlags = PhotoQueryEngine::QueryParams::buildSourceTypesFlags(wantSLR, wantDrone);
	queryParams.setSourceTypesFlag(sourceTypeFlags);
	PhotoResultsPtr photoResults = m_photoCatalogue.getQueryEngine().getPhotoResults(queryParams, PhotoQueryEngine::BUILD_DATE_ACCESSOR);

	DateParams dateParams = getDateParamsFromRequest(request, true, refinedURI);
	
	if (slideShow == 1)
	{
		PhotosHTMLHelpers::GenMainSitenavCodeParams params(false, false, "dates_");
		std::string siteNavHeaderHTML = m_photosHTMLHelpers.generateMainSitenavCode(params);

		unsigned int startIndex = request.getParamAsInt("startIndex", 0);
		unsigned int perPage = request.getParamAsInt("perPage", 2000);

		std::string contentHTML = "<a href=\"javascript:openPhotoSwipe();\">slide show overlay</a><br><br>\n";

		std::string photosListJS;
		if (dateParams.type == DateParams::eYearAndMonth)
		{
			const std::vector<const PhotoItem*>* photos = photoResults->getDateAccessor().getPhotosForYearMonth(dateParams.year, dateParams.month);

			photosListJS = PhotosHTMLHelpers::getPhotoSwipeJSItemList(*photos, startIndex, perPage);
		}
		else if (dateParams.type == DateParams::eYearOnly)
		{
			const std::vector<const PhotoItem*>* photos = photoResults->getDateAccessor().getPhotosForYear(dateParams.year);

			photosListJS = PhotosHTMLHelpers::getPhotoSwipeJSItemList(*photos, startIndex, perPage);
		}

		WebResponseGeneratorTemplateFile responseGen(FileHelpers::combinePaths(m_mainWebContentPath, "dates_slideshow.tmpl"),
													 m_htmlBaseHRef, siteNavHeaderHTML, contentHTML, photosListJS);

		responseString = responseGen.getResponseString(responseParams);
	}
	else
	{
		// TODO: generateMainSitenavCode() shows up in profiles - can we cache it?
		PhotosHTMLHelpers::GenMainSitenavCodeParams params(true, true, "dates_");
		std::string siteNavHeaderHTML = m_photosHTMLHelpers.generateMainSitenavCode(params);
	
		std::string datesBarHTML = PhotosHTMLHelpers::getDatesDatesbarHTML(photoResults, dateParams.year, dateParams.month, specifyDateValsAsDirs);

		std::string slideshowURL;
		bool useSlideshowURL = request.getCookieAsInt("dates_galleryLinkToSlideshow", 1) == 1;
		if (useSlideshowURL)
		{
			std::string currentPageParams = request.getParamsAsGETString(false);
			if (specifyDateValsAsDirs)
			{
				slideshowURL = "dates/" + refinedURI + "?" + currentPageParams + "&slideshow=1&";
			}
			else
			{
				slideshowURL = "dates/?" + currentPageParams + "&slideshow=1&";
			}
		}

		std::string contentHTML = PhotosHTMLHelpers::getDatesPhotosContentHTML(photoResults, dateParams, request, m_lazyPhotoLoadingEnabled, slideshowURL, specifyDateValsAsDirs);
	
		WebResponseGeneratorTemplateFile responseGen(FileHelpers::combinePaths(m_mainWebContentPath, "dates_gallery.tmpl"),
													 m_htmlBaseHRef,
													 siteNavHeaderHTML,
													 datesBarHTML,
													 contentHTML);
	
		responseString = responseGen.getResponseString(responseParams);
	}

	// send the response
	requestConnection.pConnectionSocket->send(responseString);

	handleRequestResult.wasHandled = true;
	return handleRequestResult;
}

WebRequestHandlerResult PhotosRequestHandler::handleLocationsRequest(RequestConnection& requestConnection, const WebRequest& request,
																 const WebRequestAuthenticationState& authenticationState)
{
	const Configuration& configuration = *requestConnection.pThreadConfig->pConfiguration;

	WebResponseParams responseParams(configuration, requestConnection.https);

	WebRequestHandlerResult handleRequestResult;

	std::string responseString;

	unsigned int slideShow = request.getParamAsInt("slideshow", 0);
	unsigned int gallery = request.getParamAsInt("gallery", 0);

	unsigned int perPage = request.getParamAsInt("perPage", 100);
	unsigned int startIndex = request.getParamAsInt("startIndex", 0);

	unsigned int thumbnailSize = request.getParamOrCookieAsInt("thumbnailSize", "locations_thumbnailSizeValue", 500);

	PhotoQueryEngine::QueryParams queryParams;
	bool wantSLR = request.getParamOrCookieAsInt("typeSLR", "locations_typeSLR", 1) == 1;
	bool wantDrone = request.getParamOrCookieAsInt("typeDrone", "locations_typeDrone", 0) == 1;

	int sortOrderType = request.getParamOrCookieAsInt("sortOrder", "locations_sortOrderIndex", 1);
	queryParams.setSortOrderType(sortOrderType == 0 ? PhotoQueryEngine::QueryParams::eSortOldestFirst : PhotoQueryEngine::QueryParams::eSortYoungestFirst);

	// TODO: do this properly...
	queryParams.setPermissionType((PhotoQueryEngine::QueryParams::PermissionType)authenticationState.authenticationPermission.level);

	unsigned int sourceTypeFlags = PhotoQueryEngine::QueryParams::buildSourceTypesFlags(wantSLR, wantDrone);
	queryParams.setSourceTypesFlag(sourceTypeFlags);
	PhotoResultsPtr photoResults = m_photoCatalogue.getQueryEngine().getPhotoResults(queryParams, PhotoQueryEngine::BUILD_LOCATIONS_ACCESSOR);

	std::string locationPath = request.getParam("locationPath");

	std::string locationBarHTML = PhotosHTMLHelpers::getLocationsLocationBarHTML(request);

	if (!locationPath.empty() && slideShow == 1)
	{
		PhotosHTMLHelpers::GenMainSitenavCodeParams params(false, false, "locations_");
		std::string siteNavHeaderHTML = m_photosHTMLHelpers.generateMainSitenavCode(params);

		// add locations bar on to siteNavHeaderHTML...
		siteNavHeaderHTML += "\n" + locationBarHTML + "\n";
		
		// TODO: is obeying the pagination stuff wanted/worth it?
		//       can we jump to a particular photo index in some other way?

		std::string contentAndPaginationHTML = "<a href=\"javascript:openPhotoSwipe();\">slide show overlay</a><br><br>\n";

		std::string currentLocationPath = request.getParam("locationPath");

		const PhotoResultsLocationAccessor& rsLocationAccessor = photoResults->getLocationAccessor();

		const std::vector<const PhotoItem*>* pPhotos = rsLocationAccessor.getPhotosForLocation(currentLocationPath);

		if (perPage > 0)
		{
			contentAndPaginationHTML += PhotosHTMLHelpers::getPaginationCode("locations/", request, pPhotos->size(), startIndex, perPage, true, true);
		}
		
		std::string photosListJS = PhotosHTMLHelpers::getPhotoSwipeJSItemList(*pPhotos, startIndex, perPage);

		WebResponseGeneratorTemplateFile responseGen(FileHelpers::combinePaths(m_mainWebContentPath, "locations_slideshow.tmpl"),
													 m_htmlBaseHRef, siteNavHeaderHTML, contentAndPaginationHTML, photosListJS);

		responseString = responseGen.getResponseString(responseParams);
	}
	else if (!locationPath.empty() && gallery == 1)
	{
		// TODO: generateMainSitenavCode() shows up in profiles - can we cache it?
		PhotosHTMLHelpers::GenMainSitenavCodeParams params(true, true, "locations_");
		std::string siteNavHeaderHTML = m_photosHTMLHelpers.generateMainSitenavCode(params);

		// add locations bar on to siteNavHeaderHTML...
		siteNavHeaderHTML += "\n" + locationBarHTML + "\n";

		const PhotoResultsLocationAccessor& rsLocationAccessor = photoResults->getLocationAccessor();

		const std::vector<const PhotoItem*>* pPhotos = rsLocationAccessor.getPhotosForLocation(locationPath);

		std::string photosListHTML;
		std::string paginationHTML;

		if (pPhotos && !pPhotos->empty())
		{
			// normal gallery
			if (perPage > 0)
			{
				unsigned int totalPhotos = pPhotos->size();
				paginationHTML = PhotosHTMLHelpers::getPaginationCode("locations/", request, totalPhotos, startIndex, perPage, true, true);
			}

			bool lazyLoad = m_lazyPhotoLoadingEnabled && request.getParamOrCookieAsInt("lazyLoading", "locations_lazyLoading", 1) == 1;

			std::string slideshowURL;
			bool useSlideshowURL = request.getCookieAsInt("locations_galleryLinkToSlideshow", 1) == 1;
			if (useSlideshowURL)
			{
				std::string currentPageParams = request.getParamsAsGETString(false);
				slideshowURL = "locations/?" + currentPageParams + "&slideshow=1&";
			}

			photosListHTML = PhotosHTMLHelpers::getSimpleImageListWithinCustomDivTagWithStyle(*pPhotos, "gallery_item",
																							  startIndex, perPage, thumbnailSize, lazyLoad,
																							  slideshowURL);
		}

		WebResponseGeneratorTemplateFile responseGen(FileHelpers::combinePaths(m_mainWebContentPath, "locations_gallery.tmpl"),
													 m_htmlBaseHRef,
													 siteNavHeaderHTML,
													 photosListHTML,
													 paginationHTML);

		responseString = responseGen.getResponseString(responseParams);
	}
	else
	{
		// otherwise, it's the main overview page with lists of locations two levels down, plus a few photos per location.

		// TODO: generateMainSitenavCode() shows up in profiles - can we cache it?
		PhotosHTMLHelpers::GenMainSitenavCodeParams params(false, true, "locations_");
		std::string siteNavHeaderHTML = m_photosHTMLHelpers.generateMainSitenavCode(params);

		std::string contentHTML = PhotosHTMLHelpers::getLocationsOverviewPageHTML(photoResults, request);

		WebResponseGeneratorTemplateFile responseGen(FileHelpers::combinePaths(m_mainWebContentPath, "locations_overview.tmpl"),
													 m_htmlBaseHRef,
													 siteNavHeaderHTML,
													 locationBarHTML,
													 contentHTML);

		responseString = responseGen.getResponseString(responseParams);
	}

	// send the response
	requestConnection.pConnectionSocket->send(responseString);

	handleRequestResult.wasHandled = true;
	return handleRequestResult;
}

WebRequestHandlerResult PhotosRequestHandler::handleStatusRequest(RequestConnection& requestConnection, const WebRequest& request,
											const WebRequestAuthenticationState& authenticationState)
{
	WebRequestHandlerResult handleRequestResult;
	
	const Configuration& configuration = *requestConnection.pThreadConfig->pConfiguration;

	WebResponseParams responseParams(configuration, requestConnection.https);
	
	std::string siteNavHeaderHTML = m_photosHTMLHelpers.generateMainSitenavCode(PhotosHTMLHelpers::GenMainSitenavCodeParams(false, false, ""));
	
	std::string statusHTML = m_statusService.getCurrentStatusHTML();
	
	WebResponseGeneratorTemplateFile responseGen(FileHelpers::combinePaths(m_mainWebContentPath, "status.tmpl"),
												 m_htmlBaseHRef,
												 siteNavHeaderHTML,
												 statusHTML);
	
	std::string responseString = responseGen.getResponseString(responseParams);
	
	// send the response
	requestConnection.pConnectionSocket->send(responseString);

	handleRequestResult.wasHandled = true;
	return handleRequestResult;
}

DateParams PhotosRequestHandler::getDateParamsFromRequest(const WebRequest& request, bool checkURLPath, const std::string& refinedURI) const
{
	DateParams params;

	if (checkURLPath && !refinedURI.empty())
	{
		std::string firstLevelDirectory;
		std::string remainder;
		if (URIHelpers::splitFirstLevelDirectoryAndRemainder(refinedURI, firstLevelDirectory, remainder))
		{
			if (StringHelpers::isNumber(firstLevelDirectory))
			{
				// first dir is year
				params.year = std::atol(firstLevelDirectory.c_str());

				if (!remainder.empty() && StringHelpers::isNumber(remainder))
				{
					// we have a remainder, so it's hopefully the 0-based month?
					params.month = std::atol(remainder.c_str());
					params.type = DateParams::eYearAndMonth;
				}
				else
				{
					params.type = DateParams::eYearOnly;
				}
			}
		}
		else
		{
			// likely just the year
			if (StringHelpers::isNumber(refinedURI))
			{
				params.year = std::atol(refinedURI.c_str());
				params.type = DateParams::eYearOnly;
			}
		}
	}

	if (request.hasParam("year"))
	{
		params.year = request.getParamAsInt("year", 0);

		if (request.hasParam("month"))
		{
			params.month = request.getParamAsInt("month", 0);

			params.type = DateParams::eYearAndMonth;
		}
		else
		{
			params.type = DateParams::eYearOnly;
		}
	}

	return params;
}
