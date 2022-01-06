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

#ifndef PHOTO_RESULTS_LOCATION_ACCESSOR_H
#define PHOTO_RESULTS_LOCATION_ACCESSOR_H

#include <vector>
#include <map>
#include <string>
#include <cstring> // for memset()

#include "photo_item.h"

#include "utils/hash.h"

class PhotoResultsLocationAccessor
{
public:
	PhotoResultsLocationAccessor();
	~PhotoResultsLocationAccessor();

	void build(const std::vector<const PhotoItem*>& rawItems);

	std::vector<std::string> getSubLocationsForLocation(const std::string& locationPath) const;

	const std::vector<const PhotoItem*>* getPhotosForLocation(const std::string& locationPath) const;

protected:
	
	struct LocationHierarchyItem
	{
		LocationHierarchyItem(const std::string& sName) : name(sName)
		{

		}

		std::string					name;
		std::string					description;

		std::map<HashValue, unsigned int>	subLocationLookup;
		std::map<std::string, unsigned int>	subLocationLookupAlphabetical;

		std::vector<const PhotoItem*> photos;
	};

	struct TempLocationHierarchyItemPtrs
	{
		enum TempLocationPointers
		{
			eTempLocationPointersNumber = 4
		};

		TempLocationHierarchyItemPtrs()
		{
			memset(items, 0, sizeof(LocationHierarchyItem*) * eTempLocationPointersNumber);
		}

		LocationHierarchyItem*		items[eTempLocationPointersNumber];

	};

	std::vector<LocationHierarchyItem*>	m_aItems;

	// first level lookups
	// based on hash
	std::map<HashValue, unsigned int>	m_locationLookup;
	std::map<std::string, unsigned int> m_locationLookupAlphabetical;
};

#endif // PHOTO_RESULTS_LOCATION_ACCESSOR_H
