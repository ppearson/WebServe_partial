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

#include "photos_html_helpers.h"

#include <cstring> // for memset()
#include <algorithm>
#include <fstream>

#include "utils/file_helpers.h"
#include "utils/string_helpers.h"
#include "web_request.h"

static const char* kMonthNames[] = { "January", "February", "March", "April", "May", "June",
									 "July", "August", "September", "October", "November", "December" };

PhotosHTMLHelpers::PhotosHTMLHelpers()
{

}

void PhotosHTMLHelpers::setMainWebContentPath(const std::string& mainWebContentPath)
{
	m_mainWebContentPath = mainWebContentPath;
}

std::string PhotosHTMLHelpers::generateMainSitenavCode(const GenMainSitenavCodeParams& params)
{
	std::string finalHTML = "<div class=\"topnav\">\n" \
							"<a href=\"\">Home</a>\n" \
							"<a href=\"photostream/\">Photostream</a>\n" \
							"<a href=\"dates/\">Dates</a>\n" \
							"<a href=\"locations/\">Locations</a>\n" \
							"<a href=\"sets/\">Sets</a>\n";
	
	// TODO: these std::string += appends() really show up in profiles...

	if (params.addPlaySlideshowIcon || params.addViewSettingsIcon)
	{
		finalHTML += "<div class=\"subbarRight\">\n";

		if (params.addPlaySlideshowIcon)
		{
			finalHTML += "<img src=\"icons/play_main_navbar.svg\" onclick=\"redirectToMainSlideshow()\" style=\"float:right;\">\n";
		}

		if (params.addViewSettingsIcon)
		{
			finalHTML += R"(<img src="icons/settings_main_navbar.svg" onclick="toggleViewSettingsPopupPanel(')" + params.viewSettingsTemplatePrefix + "')\" style=\"float:right;\">\n";
			finalHTML += loadTemplateSnippet(FileHelpers::combinePaths(m_mainWebContentPath, params.viewSettingsTemplatePrefix + "view_settings_popup_div.stmpl"));
		}

		finalHTML += "</div>\n";
	}

	finalHTML += "</div>\n";

	if (params.addViewSettingsIcon)
	{
		// somewhat distasteful, but for positioning it needs to be near the button invoking the popup...
		finalHTML += "\n";
		finalHTML += loadTemplateSnippet(FileHelpers::combinePaths(m_mainWebContentPath, params.viewSettingsTemplatePrefix + "view_settings_popup_scripts.stmpl"));
		finalHTML += "\n";
	}

	return finalHTML;
}

std::string PhotosHTMLHelpers::loadTemplateSnippet(const std::string& filePath)
{
	std::fstream fileStream(filePath.c_str(), std::ios::in);

	if (fileStream.fail())
		return "";

	std::string finalContent;

	std::string line;
	char buf[1024];

	while (fileStream.getline(buf, 1024))
	{
		line.assign(buf);

		finalContent += line + "\n";
	}

	fileStream.close();

	return finalContent;
}

std::string PhotosHTMLHelpers::getPaginationCode(const std::string& url, const WebRequest& request, unsigned int totalCount, unsigned int startIndex,
												 unsigned int perPage, bool addFirstAndLast,
												 bool addToExistingGetParams)
{
	if (perPage == 0)
		return "";

	std::string finalHTML;

	finalHTML += "<div class=\"pagination\">\n";

	unsigned int numPages = totalCount / perPage;
	numPages += (totalCount % perPage > 0);

	if (numPages == 0 || numPages == 1)
		return "";

	static const unsigned int kNumTotalPagesToShow = 9;
	static const unsigned int kNumBookendPagesToShow = 4; // pages either side (excluding first and last buttons)

	unsigned int maxPages = kNumTotalPagesToShow;

	unsigned int firstPage = 0;

	unsigned int pageItemsToShow = std::min(maxPages, numPages);

	unsigned int currentPageIndex = startIndex / perPage;

	if (numPages > maxPages)
	{
		// if we've got more pages than the limit to paginate,
		// and our current page index is greater than that,
		// we need to offset so the current page is roughly in the centre
		// of a windowed range

		if (currentPageIndex > kNumBookendPagesToShow)
		{
			firstPage = currentPageIndex - kNumBookendPagesToShow;
		}

		pageItemsToShow = std::min(numPages - firstPage, maxPages);
	}

	char szTemp[256];
	
	std::string existingGetParamsString;
	if (addToExistingGetParams)
	{
		// Note: this is a re-creation of the request GET params, so the params are likely in a different order
		//       (which shouldn't really matter).
		
		// Note: we ask to ignore pagination params, so we don't get them twice. We'd override the existing ones later
		//       just by appending the newer ones on the end (WebRequest's param parsing means last one wins), but
		//       we'd get increasingly long GET params for each subsequest pagination item with loads of duplicates,
		//       so this is nicer.
		existingGetParamsString = request.getParamsAsGETString(true);
	}

	if (addFirstAndLast && firstPage > 0 && currentPageIndex >= kNumBookendPagesToShow)
	{
		if (addToExistingGetParams)
		{
			sprintf(szTemp, "  <a href=\"%s?%s&startIndex=0&perPage=%u\">«&nbsp;First</a>\n", url.c_str(), existingGetParamsString.c_str(), perPage);
		}
		else
		{
			sprintf(szTemp, "  <a href=\"%s?startIndex=0&perPage=%u\">«&nbsp;First</a>\n", url.c_str(), perPage);
		}
		finalHTML += szTemp;
	}

	unsigned int endPage = firstPage + pageItemsToShow;

	for (unsigned int i = firstPage; i < endPage; i++)
	{
		unsigned int linkStartIndex = i * perPage;
		if (i == currentPageIndex)
		{
			if (addToExistingGetParams)
			{
				sprintf(szTemp, "  <a href=\"%s?%s&startIndex=%u&perPage=%u\" class=\"active\">%u</a>\n", url.c_str(), existingGetParamsString.c_str(), linkStartIndex, perPage, i + 1);
			}
			else
			{
				sprintf(szTemp, "  <a href=\"%s?startIndex=%u&perPage=%u\" class=\"active\">%u</a>\n", url.c_str(), linkStartIndex, perPage, i + 1);
			}
		}
		else
		{
			if (addToExistingGetParams)
			{
				sprintf(szTemp, "  <a href=\"%s?%s&startIndex=%u&perPage=%u\">%u</a>\n", url.c_str(), existingGetParamsString.c_str(), linkStartIndex, perPage, i + 1);
			}
			else
			{
				sprintf(szTemp, "  <a href=\"%s?startIndex=%u&perPage=%u\">%u</a>\n", url.c_str(), linkStartIndex, perPage, i + 1);
			}
		}
		finalHTML += szTemp;
	}

	if (addFirstAndLast)
	{
		unsigned int lastPageStartIndex = (numPages - 1) * perPage;
		if (addToExistingGetParams)
		{
			sprintf(szTemp, "  <a href=\"%s?%s&startIndex=%u&perPage=%u\">Last&nbsp;»</a>\n", url.c_str(), existingGetParamsString.c_str(), lastPageStartIndex, perPage);
		}
		else
		{
			sprintf(szTemp, "  <a href=\"%s?startIndex=%u&perPage=%u\">Last&nbsp;»</a>\n", url.c_str(), lastPageStartIndex, perPage);
		}
		finalHTML += szTemp;
	}

	finalHTML += "</div>\n";

	return finalHTML;
}

std::string PhotosHTMLHelpers::getDatesDatesbarHTML(PhotoResultsPtr photoResults, unsigned int activeYear, unsigned int activeMonth, bool useURIForComponents)
{
	std::string finalHTML;

	std::vector<uint16_t> aYears = photoResults->getDateAccessor().getListOfYears();

	char szTemp[128];
	for (unsigned int year : aYears)
	{
		if (year == activeYear)
		{
			finalHTML += "<button class=\"dropdown-btn active\">";
		}
		else
		{
			finalHTML += "<button class=\"dropdown-btn\">";
		}
		sprintf(szTemp, "%u", year);
		finalHTML += szTemp;
		finalHTML += "</button>\n";

		// set the active year item to be expanded...
		if (year == activeYear)
		{
			finalHTML += "<div class=\"dropdown-container\" style=\"display: block;\">\n";
		}
		else
		{
			finalHTML += "<div class=\"dropdown-container\">\n";
		}

		// TODO: work out how to get the above collapsable item to be a link as well
		if (useURIForComponents)
		{
			sprintf(szTemp, "  <a href=\"dates/%u\">(all)</a>\n", year);
		}
		else
		{
			sprintf(szTemp, "  <a href=\"dates/?year=%u\">(all)</a>\n", year);
		}
		finalHTML += szTemp;

		std::vector<unsigned char> months = photoResults->getDateAccessor().getListOfMonthsForYear(year);
		for (unsigned char month : months)
		{
			if (year == activeYear && month == activeMonth)
			{
				finalHTML += " <div class=\"activeMonth\">\n";
			}

			if (useURIForComponents)
			{
				sprintf(szTemp, "  <a href=\"dates/%u/%u\">%s</a>\n", year, month, kMonthNames[month]);
			}
			else
			{
				sprintf(szTemp, "  <a href=\"dates/?year=%u&month=%u\">%s</a>\n", year, month, kMonthNames[month]);
			}
			finalHTML += szTemp;

			if (year == activeYear && month == activeMonth)
			{
				finalHTML += " </div>\n";
			}
		}

		finalHTML += "</div>";
	}

	return finalHTML;
}

std::string PhotosHTMLHelpers::getDatesPhotosContentHTML(PhotoResultsPtr photoResults, const DateParams& dateParams, const WebRequest& request, bool overallLazyLoading,
														 const std::string& slideShowURL, bool useURIForComponents)
{
	std::string finalHTML;

	if (dateParams.type == DateParams::eInvalid)
		return finalHTML;
	
	char szTemp[128];

	unsigned int thumbnailSize = request.getParamOrCookieAsInt("thumbnailSize", "dates_thumbnailSizeValue", 500);

	float mainWidth = (float)thumbnailSize / 2.0f;

	bool lazyLoad = overallLazyLoading && request.getParamOrCookieAsInt("lazyLoading", "dates_lazyLoading", 1) == 1;

	unsigned int index = 0; // index for slideshow goto link

	bool useSlideShowURL = !slideShowURL.empty();
	
	auto processPhotoItems = [&] (const std::vector<const PhotoItem*>* photos)
	{
		for (const PhotoItem* pPhoto : *photos)
		{
			const PhotoRepresentations::PhotoRep* pThumbnailRepr = pPhoto->getRepresentations().getSmallestRepresentationMatchingCriteriaMinDimension(thumbnailSize);
			if (!pThumbnailRepr)
			{
				// for the moment, ignore...
				continue;
			}
			float aspectRatio = pThumbnailRepr->getAspectRatio();
			
			// try and bodge very wide images, so that we get higher res ones for those
			if (aspectRatio > 2.2f)
			{
				const PhotoRepresentations::PhotoRep* pThumbnailReprBigger = pPhoto->getRepresentations().getSmallestRepresentationMatchingCriteriaMinDimension(thumbnailSize + 200);
				if (pThumbnailReprBigger)
				{
					pThumbnailRepr = pThumbnailReprBigger;
				}
			}
			
			const std::string& thumbNailImage = pThumbnailRepr->getRelativeFilePath();

			sprintf(szTemp, "flex-basis: %upx; flex-grow: %f;", (unsigned int )(mainWidth * aspectRatio), aspectRatio);

			const PhotoRepresentations::PhotoRep* pLargeRep = pPhoto->getRepresentations().getFirstRepresentationMatchingCriteriaMinDimension(1000, true);

			std::string styleString = szTemp;

			finalHTML += R"(<div class="gallery_item" style=")" + styleString + "\">\n";

			if (pLargeRep)
			{
				if (useSlideShowURL)
				{
					sprintf(szTemp, "gotoIndex=%u", index);

					std::string url = slideShowURL + szTemp;
					finalHTML += R"( <a target="_blank" href=")" + url + "\">\n";
				}
				else
				{
					finalHTML += R"( <a target="_blank" href=")" + pLargeRep->getRelativeFilePath() + "\">\n";
				}
			}

			if (lazyLoad)
			{
				finalHTML += " <img data-src=\"" + thumbNailImage + "\" class=\"lazyload\"/>\n";
			}
			else
			{
				finalHTML += " <img src=\"" + thumbNailImage + "\">\n";
			}

			if (pLargeRep)
			{
				finalHTML += " </a>\n";
			}

			index++;

			finalHTML += "</div>\n";
		}
	};

	if (dateParams.type == DateParams::eYearAndMonth)
	{
		finalHTML += "<div class=\"gallery\">\n";

		const std::vector<const PhotoItem*>* photos = photoResults->getDateAccessor().getPhotosForYearMonth(dateParams.year, dateParams.month);

		if (photos)
		{
			processPhotoItems(photos);
		}

		finalHTML += "</div>\n";
	}
	else
	{
		// list all the months
		const std::vector<unsigned char>& monthsForYear = photoResults->getDateAccessor().getListOfMonthsForYear(dateParams.year);
		for (unsigned int monthIndex : monthsForYear)
		{
			// print month heading
			finalHTML += "<h3>" + std::string(kMonthNames[monthIndex]) + "</h3>\n";

			// link to slide show version for the month/year
			if (useURIForComponents)
			{
				sprintf(szTemp, "<a href=\"dates/%u/%u?slideshow=1\"><img src=\"icons/play_main_navbar.svg\" style=\"float:right;\"></a><br><br>\n", dateParams.year, monthIndex);
			}
			else
			{
				sprintf(szTemp, "<a href=\"dates/?year=%u&month=%u&slideshow=1\"><img src=\"icons/play_main_navbar.svg\" style=\"float:right;\"></a><br><br>\n", dateParams.year, monthIndex);
			}
			finalHTML += szTemp;

			// now the gallery of photos for this month
			finalHTML += "<div class=\"gallery\">\n";

			const std::vector<const PhotoItem*>* photos = photoResults->getDateAccessor().getPhotosForYearMonth(dateParams.year, monthIndex);

			if (photos)
			{
				processPhotoItems(photos);
			}

			finalHTML += "</div>\n";
		}
	}

	return finalHTML;
}

// could just have passed through the locationPath value directly, but we might want to do something a bit different
// in the future, so...
std::string PhotosHTMLHelpers::getLocationsLocationBarHTML(const WebRequest& request)
{
	std::string currentLocationPath = request.getParam("locationPath");
	if (currentLocationPath.empty())
		return "";

	std::vector<std::string> locationComponents;
	StringHelpers::split(currentLocationPath, locationComponents, "/");
	for (std::string& compStr : locationComponents)
	{
		StringHelpers::stripWhitespace(compStr);
	}

	std::string finalHTML = "<div class=\"subbar\">\n";
	finalHTML += "<div class=\"subbarLeft\">\n";
	finalHTML += "<div class=\"breadcrumb\">\n";

	finalHTML += " <a href=\"locations\">All</a>\n";

	std::string fullPath;
	for (const std::string& compStr : locationComponents)
	{
		fullPath = FileHelpers::combinePaths(fullPath, compStr);

		std::string encodedLocationPath = StringHelpers::simpleEncodeString(fullPath);
		finalHTML += " <a href=\"locations?locationPath=" + encodedLocationPath + "\">" + compStr + "</a>\n";
	}

	finalHTML += "</div></div></div>\n";

	return finalHTML;
}

std::string PhotosHTMLHelpers::getLocationsOverviewPageHTML(PhotoResultsPtr photoResults, const WebRequest& request)
{
	std::string finalHTML;

	std::string currentLocationPath = request.getParam("locationPath");

	const PhotoResultsLocationAccessor& rsLocationAccessor = photoResults->getLocationAccessor();

	std::vector<std::string> subLocationsNames = rsLocationAccessor.getSubLocationsForLocation(currentLocationPath);

	if (!subLocationsNames.empty())
	{
		// we have further sub-locations, so list these locations and their sub-locations

		for (const std::string& subName : subLocationsNames)
		{
			std::string fullSubPath = FileHelpers::combinePaths(currentLocationPath, subName);
			std::string encodedLocationPath = StringHelpers::simpleEncodeString(fullSubPath);

			std::vector<std::string> subSubLocations = rsLocationAccessor.getSubLocationsForLocation(fullSubPath);
			
			finalHTML += "<div class=\"locationPanel\">\n";
			
			// only add an actual link if there are sub-locations...
			if (!subSubLocations.empty())
			{
				finalHTML += R"(<div class="locationPanel-header"><a href="locations?locationPath=)" + encodedLocationPath + "\">" + subName + "</a></div>\n";
			}
			else
			{
				// otherwise, just the title
				finalHTML += R"(<div class="locationPanel-header">)" + subName + "</a></div>\n";
			}

			finalHTML += R"(<div class="locationPanel-body"><a href="locations?locationPath=)" + encodedLocationPath + "&gallery=1\">View all photos</a></div>\n";

			finalHTML += "<div class=\"locationPanel-footer\">\n";

			for (const std::string& subSubName : subSubLocations)
			{
				std::string encodedSubLocationPath = StringHelpers::simpleEncodeString(fullSubPath + "/" + subSubName);
				finalHTML += "<div class=\"subLocationChip\">\n";
				finalHTML += " <a href=\"locations?locationPath=" + encodedSubLocationPath + "&gallery=1\"><img src=\"icons/photo_stack.png\"></a>\n";

				finalHTML += " <a href=\"locations?locationPath=" + encodedSubLocationPath + "\">" + subSubName + "</a>\n";
				finalHTML += "</div>\n";
			}

			finalHTML += "</div>\n"; // end footer

			finalHTML += "</div>\n";
		}
	}
	else
	{
		// no further sub-locations, so just display photos
		// TODO: display some photos...
	}

	return finalHTML;
}

std::string PhotosHTMLHelpers::getSimpleImageList1(const std::vector<PhotoItem>& photos)
{
	std::string finalHTML;

	for (const PhotoItem& photo : photos)
	{
		const PhotoRepresentations::PhotoRep* pRepr = photo.getRepresentations().getFirstRepresentationMatchingCriteriaMaxDimension(500, true);
		if (!pRepr)
		{
			// for the moment, ignore...
			continue;
		}
		const std::string& thumbNailImage = pRepr->getRelativeFilePath();

		finalHTML += "<img src=\"" + thumbNailImage + "\">\n";
	}

	return finalHTML;
}

std::string PhotosHTMLHelpers::getSimpleImageListWithinCustomDivTag(const std::vector<PhotoItem>& photos, const std::string& divTag)
{
	std::string finalHTML;

	for (const PhotoItem& photo : photos)
	{
		const PhotoRepresentations::PhotoRep* pRepr = photo.getRepresentations().getFirstRepresentationMatchingCriteriaMaxDimension(500, true);
		if (!pRepr)
		{
			// for the moment, ignore...
			continue;
		}
		const std::string& thumbNailImage = pRepr->getRelativeFilePath();

		finalHTML += "<div class=\"" + divTag + "\">\n";

		finalHTML += " <img src=\"" + thumbNailImage + "\">\n";

		finalHTML += "</div>\n";
	}

	return finalHTML;
}

std::string PhotosHTMLHelpers::getSimpleElementListWithinCustomDivTagWithBGImage(const std::vector<PhotoItem>& photos, const std::string& elementClassName,
																		 const std::string& divTag)
{
	std::string finalHTML;

	for (const PhotoItem& photo : photos)
	{
		const PhotoRepresentations::PhotoRep* pRepr = photo.getRepresentations().getFirstRepresentationMatchingCriteriaMaxDimension(500, true);
		if (!pRepr)
		{
			// for the moment, ignore...
			continue;
		}
		const std::string& thumbNailImage = pRepr->getRelativeFilePath();

		finalHTML += "<div class=\"" + divTag + "\">\n";

//		finalHTML += " <div role=\"img\" class=\"" + elementClassName + "\"" + " style=\"background-image:url(" + thumbNailImage + ");\"></div>\n";
		finalHTML += " <a class=\"" + elementClassName + "\"  style=\"background-image:url(" + thumbNailImage + ");\"></a>\n";

		finalHTML += "</div>\n";
	}

	return finalHTML;
}

std::string PhotosHTMLHelpers::getSimpleImageListWithinCustomDivTagWithStyle(const std::vector<const PhotoItem*>& photos, const std::string& divTag,
																			 unsigned int startIndex, unsigned int perPage,
																			 unsigned int minThumbnailSize,
																			 bool lazyLoad,
																			 const std::string& slideShowURL)
{
	std::string finalHTML;
	
	if (startIndex >= photos.size())
		return finalHTML;

	char szTemp[256];

	float mainWidth = (float)(minThumbnailSize) / 2.0f;

	// index for slideshow link
	unsigned int index = 0; // needs to start at 0, not startIndex (it's an index within the set of photos being displayed)

	std::vector<const PhotoItem*>::const_iterator itStart = photos.begin() + startIndex;
	std::vector<const PhotoItem*>::const_iterator itEnd = photos.end();
	if (perPage > 0 && itStart != itEnd)
	{
		// we need to clamp to the end
		unsigned int endIndex = startIndex + perPage;
		if (endIndex < photos.size())
		{
			itEnd = itStart + perPage;
		}
	}

	bool useSlideShowURL = !slideShowURL.empty();

	for (; itStart != itEnd; ++itStart)
	{
		const PhotoItem* photo = *itStart;
		const PhotoRepresentations::PhotoRep* pThumbnailRepr = photo->getRepresentations().getSmallestRepresentationMatchingCriteriaMinDimension(minThumbnailSize);
		if (!pThumbnailRepr)
		{
			// for the moment, ignore...
			continue;
		}

		float aspectRatio = pThumbnailRepr->getAspectRatio();
		
		// try and bodge very wide images, so that we get higher res ones for those
		if (aspectRatio > 2.2f)
		{
			const PhotoRepresentations::PhotoRep* pThumbnailReprBigger = photo->getRepresentations().getSmallestRepresentationMatchingCriteriaMinDimension(minThumbnailSize + 200);
			if (pThumbnailReprBigger)
			{
				pThumbnailRepr = pThumbnailReprBigger;
			}
		}
		
		const std::string& thumbNailImage = pThumbnailRepr->getRelativeFilePath();

		sprintf(szTemp, "flex-basis: %upx; flex-grow: %f;", (unsigned int )(mainWidth * aspectRatio), aspectRatio);
//		sprintf(szTemp, "flex-basis: %upx; flex-shrink: %f;", (unsigned int )(mainWidth * aspectRatio), aspectRatio);

		const PhotoRepresentations::PhotoRep* pLargeRep = photo->getRepresentations().getFirstRepresentationMatchingCriteriaMinDimension(1000, true);

		std::string styleString = szTemp;

		finalHTML += "<div class=\"" + divTag + "\" style=\"" + styleString + "\">\n";

		if (pLargeRep)
		{
			if (useSlideShowURL)
			{
				sprintf(szTemp, "gotoIndex=%u", index);

				std::string url = slideShowURL + szTemp;
				finalHTML += R"( <a target="_blank" href=")" + url + "\">\n";
			}
			else
			{
				finalHTML += R"( <a target="_blank" href=")" + pLargeRep->getRelativeFilePath() + "\">\n";
			}
		}

		if (lazyLoad)
		{
			finalHTML += " <img data-src=\"" + thumbNailImage + "\" class=\"lazyload\"/>\n";
		}
		else
		{
			finalHTML += " <img src=\"" + thumbNailImage + "\">\n";
		}

		if (pLargeRep)
		{
			finalHTML += " </a>\n";
		}

		finalHTML += "</div>\n";

		index++;
	}
	
	finalHTML += "<div class=\"" + divTag + "\"></div>\n";
	finalHTML += "<div class=\"" + divTag + "\"></div>\n";

	return finalHTML;
}

std::string PhotosHTMLHelpers::getPhotoSwipeJSItemList(const std::vector<const PhotoItem*>& photoItems, unsigned int startIndex, unsigned int perPage)
{
	std::string finalJS;

	finalJS = "var items = [\n";

	char szTemp[2048];

	// TODO: for the moment, we'll do this...
	std::vector<const PhotoItem*>::const_iterator itStart = photoItems.begin() + startIndex;
	std::vector<const PhotoItem*>::const_iterator itEnd = photoItems.end();
	if (perPage > 0)
	{
		// we need to clamp to the end
		unsigned int endIndex = startIndex + perPage;
		if (endIndex < photoItems.size())
		{
			itEnd = itStart + perPage;
		}
	}

	unsigned int numItems = itEnd - itStart;

	unsigned int count = 0;

	for (; itStart != itEnd; ++itStart)
	{
		const PhotoItem* photo = *itStart;

		const PhotoRepresentations::PhotoRep* pRepr = photo->getRepresentations().getFirstRepresentationMatchingCriteriaMinDimension(100, true);
		if (!pRepr)
		{
			// for the moment, ignore...
			continue;
		}
		const std::string& imagePath = pRepr->getRelativeFilePath();

		const std::string& imageLinkPath = imagePath;

		unsigned int mainWidth = pRepr->getWidth();
		unsigned int mainHeight = pRepr->getHeight();

		// technically, this isn't needed, as sprintf() adds a NULL-terminator... But as this is a web server, it *might*
		// make things a bit more secure if there is an issue somewhere, but it's obviously no guarentee for things like
		// possible stack corruptions, etc, etc, so probably not worth it if performance suffers...
		memset(szTemp, 0, 2048);

		if (count != (numItems - 1))
		{
			// if we're not the last one, we need a comma on the end
			sprintf(szTemp, "\t{\n\t\tsrc: '%s',\n\t\tw: %u,\n\t\th: %u\n\t},\n", imageLinkPath.c_str(), mainWidth, mainHeight);
		}
		else
		{
			// otherwise, we don't... - TODO: could combine these two?
			sprintf(szTemp, "\t{\n\t\tsrc: '%s',\n\t\tw: %u,\n\t\th: %u\n\t}\n", imageLinkPath.c_str(), mainWidth, mainHeight);
		}

		finalJS.append(szTemp);

		count++;
	}

	finalJS += "\t];\n";

	return finalJS;
}
