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

#ifndef PHOTO_QUERY_ENGINE_H
#define PHOTO_QUERY_ENGINE_H

#include <vector>
#include <memory>
#include <condition_variable>

#include "photo_item.h"

#include "photo_results.h"

class PhotoQueryEngine
{
public:
	PhotoQueryEngine(const std::vector<PhotoItem>& allPhotos);

	struct QueryParams
	{
		QueryParams() : queryType(eQueryAll),
			sortOrderType(eSortOldestFirst),
			sourceTypes(0),
			itemTypes(0),
			permissionType(ePermPublic),
			minRating(0)
		{

		}

		enum QueryType
		{
			eQueryAll,
			eQueryYear,
			eQueryLocation
		};
		
		enum PermissionType
		{
			ePermPublic,
			ePermAuthBasic,
			ePermAuthAdvanced,
			ePermPrivate
		};

		enum SortOrderType
		{
			eSortOldestFirst,
			eSortYoungestFirst
		};

		// not really the place for this, but...
		static unsigned int buildSourceTypesFlags(bool slrType, bool droneType)
		{
			unsigned int finalFlags = (slrType) ? PhotoItem::eSourceSLR : 0;
			if (droneType)
			{
				finalFlags |= PhotoItem::eSourceDrone;
			}

			return finalFlags;
		}

		void setBasicItems(QueryType qType, SortOrderType sortType, unsigned int sourceTypesFlags, unsigned int itemTypesFlags, unsigned int minRatingVal)
		{
			queryType = qType;
			sortOrderType = sortType;
			sourceTypes = sourceTypesFlags;
			itemTypes = itemTypesFlags;
			minRating = minRatingVal;
		}

		void setSortOrderType(SortOrderType sortType)
		{
			sortOrderType = sortType;
		}

		void setSourceTypesFlag(unsigned int flag)
		{
			sourceTypes |= flag;
		}

		void clearSourceTypesFlag(unsigned int flag)
		{
			sourceTypes &= ~flag;
		}
		
		void setPermissionType(PermissionType permType)
		{
			permissionType = permType;
		}

		// not really sure why we need to provide this and the compiler can't generate it itself, but...
		bool operator==(const QueryParams& rhs) const
		{
			return queryType == rhs.queryType &&
					sortOrderType == rhs.sortOrderType &&
					sourceTypes == rhs.sourceTypes &&
					itemTypes == rhs.itemTypes &&
					permissionType == rhs.permissionType &&
					minRating == rhs.minRating;
		}

		//

		QueryType				queryType;

		SortOrderType			sortOrderType;

		unsigned int			sourceTypes;
		unsigned int			itemTypes;
		
		PermissionType			permissionType;

		unsigned int			minRating;
	};

	enum AccessorBuildFlags
	{
		BUILD_DATE_ACCESSOR =		1 << 0,
		BUILD_LOCATIONS_ACCESSOR =	1 << 1
	};

	PhotoResultsPtr getPhotoResults(const QueryParams& queryParams, unsigned int buildFlags = 0);


protected:
	PhotoResultsPtr findCachedResult(const QueryParams& queryParams) const;

	void addCachedResult(const QueryParams& queryParams, PhotoResultsPtr photoResults);

	PhotoResultsPtr performQuery(const QueryParams& queryParams);
	
	bool matchesPermissions(QueryParams::PermissionType permissionType, const PhotoItem& item);

protected:
	const std::vector<PhotoItem>& m_aAllPhotos;

	enum
	{
		kItemCacheSize = 10
	};



	std::mutex						m_cacheLock;
	// TODO: uses hashes of this instead of the item directly...
	QueryParams						m_cachedQueries[kItemCacheSize];
	PhotoResultsPtr					m_cachedItemsPtr[kItemCacheSize];
	unsigned int					m_numCachedResults;
	unsigned int					m_nextCachedIndex;
};

#endif // PHOTO_QUERY_ENGINE_H
