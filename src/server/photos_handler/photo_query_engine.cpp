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

#include "photo_query_engine.h"

#include <algorithm>

PhotoQueryEngine::PhotoQueryEngine(const std::vector<PhotoItem>& allPhotos) :
	m_aAllPhotos(allPhotos),
	m_numCachedResults(0),
	m_nextCachedIndex(0)
{

}

PhotoResultsPtr PhotoQueryEngine::getPhotoResults(const QueryParams& queryParams, unsigned int buildFlags)
{
	PhotoResultsPtr result;

	std::unique_lock<std::mutex> lock(m_cacheLock);

	// first of all see if we've got the result in our cache already...
	result = findCachedResult(queryParams);

	if (result)
	{
		lock.unlock();

		if (buildFlags)
		{
			PhotoResults* pEditableResult = const_cast<PhotoResults*>(result.get());

			if (buildFlags & BUILD_DATE_ACCESSOR)
			{
				pEditableResult->checkDateAccessorIsValid();
			}
			else if (buildFlags & BUILD_LOCATIONS_ACCESSOR)
			{
				pEditableResult->checkLocationAccessorIsValid();
			}
		}
		return result;
	}

	// otherwise, we'll need to perform an actual query to generate a new result

	result = performQuery(queryParams);

	if (buildFlags)
	{
		PhotoResults* pEditableResult = const_cast<PhotoResults*>(result.get());

		if (buildFlags & BUILD_DATE_ACCESSOR)
		{
			pEditableResult->checkDateAccessorIsValid();
		}
		else if (buildFlags & BUILD_LOCATIONS_ACCESSOR)
		{
			pEditableResult->checkLocationAccessorIsValid();
		}
	}

	addCachedResult(queryParams, result);

	return result;
}

PhotoResultsPtr PhotoQueryEngine::findCachedResult(const QueryParams& queryParams) const
{
	for (unsigned int i = 0; i < m_numCachedResults; i++)
	{
		if (m_cachedQueries[i] == queryParams)
			return m_cachedItemsPtr[i];
	}

	PhotoResultsPtr result;
	return result;
}

void PhotoQueryEngine::addCachedResult(const QueryParams& queryParams, PhotoResultsPtr photoResults)
{
	if (m_numCachedResults < kItemCacheSize)
	{
		// we haven't hit the limit yet, so just add it to the end
		m_cachedQueries[m_nextCachedIndex] = queryParams;
		m_cachedItemsPtr[m_nextCachedIndex] = photoResults;

		m_nextCachedIndex++;
		m_numCachedResults++;
	}
	else
	{
		// we're going to have to replace one of the existing ones..
		m_nextCachedIndex = (m_nextCachedIndex + 1) % kItemCacheSize;
		m_cachedQueries[m_nextCachedIndex] = queryParams;
		m_cachedItemsPtr[m_nextCachedIndex] = photoResults;
	}
}

PhotoResultsPtr PhotoQueryEngine::performQuery(const QueryParams& queryParams)
{
	std::shared_ptr<PhotoResults> editableResult = std::make_shared<PhotoResults>(PhotoResults(m_aAllPhotos));

	std::vector<const PhotoItem*> aResultsItems;

	for (const PhotoItem& item : m_aAllPhotos)
	{
		if (queryParams.sourceTypes)
		{
			if (!(queryParams.sourceTypes & item.getSourceType()))
				continue;
		}

		if (queryParams.itemTypes)
		{
			if (!(queryParams.itemTypes & item.getItemType()))
				continue;
		}
		
		if (!matchesPermissions(queryParams.permissionType, item))
			continue;

		if (queryParams.minRating)
		{
			if (item.getRating() < queryParams.minRating)
				continue;
		}

		aResultsItems.emplace_back(&item);
	}

	if (queryParams.sortOrderType == QueryParams::eSortYoungestFirst)
	{
		// reverse the items
		std::reverse(aResultsItems.begin(), aResultsItems.end());
	}

	editableResult->setResults(aResultsItems);

	PhotoResultsPtr result = editableResult;

	return result;
}

bool PhotoQueryEngine::matchesPermissions(QueryParams::PermissionType permissionType, const PhotoItem& item)
{
	if (item.getPermissionType() == PhotoItem::ePermissionPublic)
		return true;
	
	// for the moment, we can just do this as long as the values match exactly, but we'll need something
	// more robust at some point...
	if ((int)item.getPermissionType() > (int)permissionType)
		return false;
	
	return true;
}
