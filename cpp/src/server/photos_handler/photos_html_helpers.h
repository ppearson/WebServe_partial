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

#ifndef PHOTOS_HTML_HELPERS_H
#define PHOTOS_HTML_HELPERS_H

#include <vector>
#include <string>

#include "photo_item.h"
#include "photo_results.h"
#include "photos_common.h"

class WebRequest;

class PhotosHTMLHelpers
{
public:
	PhotosHTMLHelpers();

	void setMainWebContentPath(const std::string& mainWebContentPath);

	struct GenMainSitenavCodeParams
	{
		GenMainSitenavCodeParams(bool addPlaySlideshow, bool addViewSettings, const std::string& vsPrefix) :
			addPlaySlideshowIcon(addPlaySlideshow),
			addViewSettingsIcon(addViewSettings),
			viewSettingsTemplatePrefix(vsPrefix)
		{

		}

		bool				addPlaySlideshowIcon = false;
		bool				addViewSettingsIcon = false;
		std::string			viewSettingsTemplatePrefix;
	};

	std::string generateMainSitenavCode(const GenMainSitenavCodeParams& params);

	// assumption here is that returning std::string with move semantics is efficient...

	static std::string loadTemplateSnippet(const std::string& filePath);

	static std::string getPaginationCode(const std::string& url, const WebRequest& request, unsigned int totalCount, unsigned int startIndex, unsigned int perPage, bool addFirstAndLast,
										 bool addToExistingGetParams);

	static std::string getDatesDatesbarHTML(PhotoResultsPtr photoResults, unsigned int activeYear, unsigned int activeMonth, bool useURIForComponents);
	static std::string getDatesPhotosContentHTML(PhotoResultsPtr photoResults, const DateParams& dateParams, const WebRequest& request, bool overallLazyLoading,
												 const std::string& slideShowURL, bool useURIForComponents);

	static std::string getLocationsLocationBarHTML(const WebRequest& request);
	static std::string getLocationsOverviewPageHTML(PhotoResultsPtr photoResults, const WebRequest& request);

	static std::string getSimpleImageList1(const std::vector<PhotoItem>& photos);
	static std::string getSimpleImageListWithinCustomDivTag(const std::vector<PhotoItem>& photos, const std::string& divTag);

	static std::string getSimpleElementListWithinCustomDivTagWithBGImage(const std::vector<PhotoItem>& photos, const std::string& elementClassName,
																		 const std::string& divTag);

	static std::string getSimpleImageListWithinCustomDivTagWithStyle(const std::vector<const PhotoItem*>& photos, const std::string& divTag,
																	 unsigned int startIndex, unsigned int perPage,
																	 unsigned int minThumbnailSize,
																	 bool lazyLoad,
																	 const std::string& slideShowURL);

	static std::string getPhotoSwipeJSItemList(const std::vector<const PhotoItem*>& photoItems, unsigned int startIndex, unsigned int perPage);

protected:
	std::string		m_mainWebContentPath;
};

#endif // PHOTOS_HTML_HELPERS_H
