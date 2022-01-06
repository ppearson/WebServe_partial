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

#include "photo_results_location_accessor.h"

#include "utils/string_helpers.h"

static const std::string kBuildLocationItemSeparator = "/";
static const std::string kLookupLocationItemSeparator = "/";

PhotoResultsLocationAccessor::PhotoResultsLocationAccessor()
{

}

PhotoResultsLocationAccessor::~PhotoResultsLocationAccessor()
{
	for (LocationHierarchyItem* pItem : m_aItems)
	{
		delete pItem;
	}
}

void PhotoResultsLocationAccessor::build(const std::vector<const PhotoItem*>& rawItems)
{
	// build a temporary cache of full path lookups from string hashes, so we can fairly quickly append the photo pointer
	// to the appropriate vectors without having to parse the item.

	std::map<HashValue, TempLocationHierarchyItemPtrs*> cachedFullLookup;

	for (const PhotoItem* pPhotoItem : rawItems)
	{
		const PhotoItem& photoItem = *pPhotoItem;

		if (!photoItem.getTimeTaken().isValid())
			continue;

		if (photoItem.getGeoLocationPath().isEmpty())
			continue;

		// first of all see if we've seen the full location value before, in which case we should
		// have a cache of the vector items
		HashValue fullLocationHash = photoItem.getGeoLocationPath().getHashValue();

		auto itFindFull = cachedFullLookup.find(fullLocationHash);

		if (itFindFull != cachedFullLookup.end())
		{
			// we have seen it before, so we can just append pointers

			const TempLocationHierarchyItemPtrs* pCachedVecPointers = itFindFull->second;
			for (unsigned int i = 0; i < TempLocationHierarchyItemPtrs::eTempLocationPointersNumber; i++)
			{
				LocationHierarchyItem* pVec = pCachedVecPointers->items[i];
				if (pVec)
				{
					pVec->photos.emplace_back(pPhotoItem);
				}
			}

			continue;
		}

		// otherwise, we haven't seen it before, so we need to break down the location hierarchy string into component items

		std::string locationString = photoItem.getGeoLocationPath().getString();
		StringHelpers::stripWhitespace(locationString);

		std::vector<std::string> locationComponents;
		StringHelpers::split(locationString, locationComponents, "/");

		// build up a temporary cache of pointers to the LocationHierarchyItems
		TempLocationHierarchyItemPtrs* pNewTempPointers = new TempLocationHierarchyItemPtrs();

		unsigned int level = 0;
		for (std::string& compStr : locationComponents)
		{
			StringHelpers::stripWhitespace(compStr);

			Hash hasher;
			hasher.addString(compStr);

			HashValue componentHash = hasher.getHash();

			if (level == 0)
			{
				auto itFindComponent = m_locationLookup.find(componentHash);

				if (itFindComponent == m_locationLookup.end())
				{
					// we didn't find it, so we need to create it
					LocationHierarchyItem* pNewItem = new LocationHierarchyItem(compStr);

					unsigned int newItemIndex = m_aItems.size();

					m_aItems.emplace_back(pNewItem);

					pNewTempPointers->items[0] = pNewItem;

					m_locationLookup.insert(std::pair<HashValue, unsigned int>(componentHash, newItemIndex));
					m_locationLookupAlphabetical.insert(std::pair<std::string, unsigned int>(compStr, newItemIndex));
				}
				else
				{
					// we've seen this first level item before
					unsigned int firstLevelComponentItemIndex = itFindComponent->second;
					LocationHierarchyItem* pFirstLevelItem = m_aItems[firstLevelComponentItemIndex];

					pNewTempPointers->items[0] = pFirstLevelItem;
				}
			}
			else
			{
				// do the remaining items

				// for the moment, clamp number of components we can handle...
				if (level >= TempLocationHierarchyItemPtrs::eTempLocationPointersNumber)
					break;

				LocationHierarchyItem* previousLevelItem = pNewTempPointers->items[level - 1];

				auto itFindComponent = previousLevelItem->subLocationLookup.find(componentHash);

				if (itFindComponent == previousLevelItem->subLocationLookup.end())
				{
					// we didn't find it, so create a new item...

					LocationHierarchyItem* pNewSubItem = new LocationHierarchyItem(compStr);

					unsigned int newItemIndex = m_aItems.size();

					m_aItems.push_back(pNewSubItem);

					pNewTempPointers->items[level] = pNewSubItem;

					previousLevelItem->subLocationLookup.insert(std::pair<HashValue, unsigned int>(componentHash, newItemIndex));
					previousLevelItem->subLocationLookupAlphabetical.insert(std::pair<std::string, unsigned int>(compStr, newItemIndex));
				}
				else
				{
					// we've seen this level already...
					unsigned int thisSubItemIndex = itFindComponent->second;
					LocationHierarchyItem* pThisSubItem = m_aItems[thisSubItemIndex];

					pNewTempPointers->items[level] = pThisSubItem;
				}
			}

			level++;
		}

		// so we now should have an array of the items to add the photo pointer to within tempPointers.items[]...

		// as we haven't seen this full item before, add it
		cachedFullLookup.insert(std::pair<HashValue, TempLocationHierarchyItemPtrs*>(fullLocationHash, pNewTempPointers));

		for (unsigned int i = 0; i < level; i++)
		{
			if (!pNewTempPointers->items[i])
				break;

			LocationHierarchyItem* pLocationItem = pNewTempPointers->items[i];
			pLocationItem->photos.emplace_back(pPhotoItem);
		}
	}
}

std::vector<std::string> PhotoResultsLocationAccessor::getSubLocationsForLocation(const std::string& locationPath) const
{
	std::vector<std::string> subLocations;

	if (locationPath.empty())
	{
		// we're at the root, so return the first level items...
		for (const auto& item : m_locationLookupAlphabetical)
		{
			subLocations.push_back(item.first);
		}
		return subLocations;
	}

	// otherwise, follow the components...

	std::vector<std::string> locationComponents;
	StringHelpers::split(locationPath, locationComponents, "/");
	for (std::string& compStr : locationComponents)
	{
		StringHelpers::stripWhitespace(compStr);
	}

	unsigned int itemIndex = 0;
	LocationHierarchyItem* pItem = nullptr;
	unsigned int componentCount = 0;
	for (const std::string& component : locationComponents)
	{
		if (componentCount++ == 0)
		{
			auto itFind = m_locationLookupAlphabetical.find(component);
			if (itFind == m_locationLookupAlphabetical.end())
				return subLocations;

			itemIndex = itFind->second;
		}
		else if (pItem)
		{
			auto itFind = pItem->subLocationLookupAlphabetical.find(component);
			if (itFind == pItem->subLocationLookupAlphabetical.end())
				return subLocations;

			itemIndex = itFind->second;
		}
		else
		{
			return subLocations;
		}

		pItem = m_aItems[itemIndex];
	}

	if (pItem)
	{
		// we want them to be alphabetical, so we iterate the map
		for (const auto& subItem : pItem->subLocationLookupAlphabetical)
		{
			subLocations.push_back(subItem.first);
		}
	}

	return subLocations;
}

const std::vector<const PhotoItem*>* PhotoResultsLocationAccessor::getPhotosForLocation(const std::string& locationPath) const
{
	if (locationPath.empty())
		return nullptr;

	std::vector<std::string> locationComponents;
	StringHelpers::split(locationPath, locationComponents, "/");
	for (std::string& compStr : locationComponents)
	{
		StringHelpers::stripWhitespace(compStr);
	}

	unsigned int itemIndex = 0;
	LocationHierarchyItem* pItem = nullptr;
	unsigned int componentCount = 0;
	for (const std::string& component : locationComponents)
	{
		if (componentCount++ == 0)
		{
			auto itFind = m_locationLookupAlphabetical.find(component);
			if (itFind == m_locationLookupAlphabetical.end())
				return nullptr;

			itemIndex = itFind->second;
		}
		else if (pItem)
		{
			auto itFind = pItem->subLocationLookupAlphabetical.find(component);
			if (itFind == pItem->subLocationLookupAlphabetical.end())
				return nullptr;

			itemIndex = itFind->second;
		}
		else
		{
			return nullptr;
		}

		pItem = m_aItems[itemIndex];
	}

	if (pItem)
	{
		return &pItem->photos;
	}

	return nullptr;
}
