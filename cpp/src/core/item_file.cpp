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

#include "item_file.h"

#include <fstream>
#include <string.h> // for memset
#include <set>

#include "utils/string_helpers.h"

static const std::set<std::string> kSetKeyNames = { "tags", "geoLocationTags" };

static const unsigned int kCommonOverrideFlagMask = 1 << 31; // left-most bit

ItemFile::ItemFile()
{

}

bool ItemFile::load(const std::string& filePath)
{
	char buf[2048];
	memset(buf, 0, 2048);

	std::fstream fileStream(filePath.c_str(), std::ios::in);

	// currently the assumption here is that common key/values are specified first before any items...

	std::string line;
	line.resize(2048);

	Item tempItem;

	unsigned int itemCount = 0;

	bool haveNewItem = false;

	while (fileStream.getline(buf, 2048))
	{
		if (buf[0] == 0 || buf[0] == '#')
			continue;

		if (buf[0] == '*')
		{
			// it's a new item

			itemCount ++;

			// if we've previously created a new item, add the previous temp one to the list, in its final state
			if (haveNewItem)
			{
				unsigned int itemIndex = m_aValueItems.size();

				m_aValueItems.emplace_back(tempItem);

				m_aItemIndices.emplace_back(itemIndex); // just the item index as-is

				haveNewItem = false;
			}

			// clear the per-item key/values.
			tempItem = Item();
		}
		else if (buf[0] == '\t')
		{
			line.assign(buf + 1); // lose the tab - TODO: make this more robust

			std::string key;
			std::string value;

			if (StringHelpers::splitInTwo(line, key, value))
			{
				StringHelpers::stripWhitespace(key);
				if (!key.empty())
				{
					// TODO: might want to be a bit conservative about the below in the future
					StringHelpers::stripWhitespace(value);
					
					tempItem.aValues[key] = value;
					haveNewItem = true;
				}
			}
		}
		else
		{
			// it's a common key/value
			line.assign(buf);

			std::string key;
			std::string value;

			if (!StringHelpers::splitInTwo(line, key, value))
				continue;
			
			StringHelpers::stripWhitespace(key);
		
			if (key.empty())
				continue;

			// TODO: might want to be a bit conservative about the below in the future
			StringHelpers::stripWhitespace(value);
			
			if (itemCount == 0)
			{
				// if we haven't added any items yet, set it as a common value
				m_aCommonValues[key] = value;
			}
			else
			{
				// otherwise, it's an override...

				// if we've previously created a new item, add the previous temp one to the list, in its final state
				if (haveNewItem)
				{
					unsigned int itemIndex = m_aValueItems.size();

					m_aValueItems.emplace_back(tempItem);

					m_aItemIndices.emplace_back(itemIndex); // just the item index as-is

					haveNewItem = false;
				}

				// add the common override
				std::map<std::string, std::string> tempMap;
				tempMap[key] = value;

				// TODO: batching up of any consecutive overrides into one single map item...

				unsigned int overrideIndex = m_aOverrides.size();

				m_aOverrides.emplace_back(tempMap);

				// because it's an override, set the left-most bit
				overrideIndex |= kCommonOverrideFlagMask;

				m_aItemIndices.emplace_back(overrideIndex);
			}
		}
	}

	// add any remaining item
	if (haveNewItem)
	{
		unsigned int itemIndex = m_aValueItems.size();

		m_aValueItems.emplace_back(tempItem);

		m_aItemIndices.emplace_back(itemIndex); // just the item index as-is
	}

	fileStream.close();

	return true;
}

bool ItemFile::save(const std::string& filePath) const
{
	std::fstream fileStream(filePath.c_str(), std::ios::out | std::ios::trunc);
	if (!fileStream)
	{
		return false;
	}

	// write out the common key/values
	for (const auto& keyVal : m_aCommonValues)
	{
		fileStream << keyVal.first << ": " << keyVal.second << "\n";
	}

	for (unsigned int itemIndex : m_aItemIndices)
	{
		// if it has left-most bit set, it's a common override, otherwise it's just a normal item
		if (itemIndex & kCommonOverrideFlagMask)
		{
			// extract the actual index to the override item
			unsigned int overrideIndex = itemIndex & ~kCommonOverrideFlagMask;

			const std::map<std::string, std::string>& overrideItem = m_aOverrides[overrideIndex];
			for (const auto& keyVal : overrideItem)
			{
				fileStream << keyVal.first << ": " << keyVal.second << "\n";
			}
		}
		else
		{
			// we can just use the index as-is

			fileStream << "*\n";

			const Item& item = m_aValueItems[itemIndex];

			for (const auto& keyVal : item.aValues)
			{
				fileStream << "\t" << keyVal.first << ": " << keyVal.second << "\n";
			}
		}
	}

	fileStream.close();

	return true;
}

std::vector<ItemFile::Item> ItemFile::getFinalBakedItems() const
{
	std::vector<ItemFile::Item> finalItems;

	// for each item, create a temp copy, assign the common values, and then
	// apply on top any overrides, so we have the final baked values within each item

	// take a copy of the common values, so we can apply common overrides
	std::map<std::string, std::string> localCommonValues = m_aCommonValues;

	for (unsigned int itemIndex : m_aItemIndices)
	{
		// if it has left-most bit set, it's a common override, otherwise it's just a normal item
		if (itemIndex & kCommonOverrideFlagMask)
		{
			// extract the actual index to the override item
			unsigned int overrideIndex = itemIndex & ~kCommonOverrideFlagMask;

			const std::map<std::string, std::string>& overrideItem = m_aOverrides[overrideIndex];
			for (const auto& keyVal : overrideItem)
			{
				localCommonValues[keyVal.first] = keyVal.second;
			}
		}
		else
		{
			// we can just use the index as-is
			const Item& item = m_aValueItems[itemIndex];

			// create the new item
			Item tempItem;
			tempItem.aValues = localCommonValues;

			for (const auto& keyVal : item.aValues)
			{
				// this set handling method is pretty bad, but...
				// TODO: make this type of thing a discrete type which can handle this natively
				if (kSetKeyNames.count(keyVal.first) > 0)
				{
					// if it's a known set type, don't overwrite it, append it...

					auto& currentValue = tempItem.aValues[keyVal.first];

					currentValue = StringHelpers::combineSetTokens(currentValue, keyVal.second);
				}
				else
				{
					tempItem.aValues[keyVal.first] = keyVal.second;
				}
			}

			finalItems.emplace_back(tempItem);
		}
	}

	return finalItems;
}

void ItemFile::addCommonValue(const std::string& key, const std::string& value)
{
	m_aCommonValues[key] = value;

	// TODO: support common overrides
}

void ItemFile::addItem(const Item& item)
{
	// we can just use the index as-is
	unsigned int itemIndex = m_aValueItems.size();

	m_aValueItems.emplace_back(item);

	m_aItemIndices.emplace_back(itemIndex);
}

void ItemFile::promoteSameValueItemValuesToCommon()
{
	// for the moment, just detect value items that are the same for each item
	// and convert them to common values, but we can improve this in the future
	// i.e. if the majority are the same we can promote them to common, and other values
	// can then be overrides on a per-item basis

	// just brute force it for the moment...

	std::map<std::string, unsigned int>	aValueCounts;

	unsigned int itemCount = 0;

	for (unsigned int itemIndex : m_aItemIndices)
	{
		// if it hasn't got the left-most bit set, it's an item which we want
		if (!(itemIndex & kCommonOverrideFlagMask))
		{
			itemCount ++;

			const Item& item = m_aValueItems[itemIndex];

			for (const auto& valueIt : item.aValues)
			{
				const std::string& valueName = valueIt.first;

				auto findIt = aValueCounts.find(valueName);
				if (findIt != aValueCounts.end())
				{
					unsigned int& valueCount = findIt->second;
					valueCount += 1;
				}
				else
				{
					aValueCounts.insert(std::pair<std::string, unsigned int>(valueName, 1));
				}
			}
		}
	}

	unsigned int numItems = itemCount;

	// now go through the map of value key -> counts, and for each item which has a count matching the number of items we have
	// brute force check all values are the same, and if so, make it a common value, and remove the values from each item.

	const Item& firstItem = m_aValueItems[0];

	for (const auto& countIt : aValueCounts)
	{
		if (countIt.second != numItems)
			continue;

		//

		const std::string& valueKey = countIt.first;

		if (valueKey == "description" || valueKey == "tags" ||
			valueKey.substr(0, 4) == "res-")
		{
			// skip these
			continue;
		}

		const std::string& firstItemValue = firstItem.getValue(valueKey);
		bool isSame = true;
		for (unsigned int i = 1; i < numItems; i++)
		{
			const Item& testItem = m_aValueItems[i];
			if (testItem.getValue(valueKey) != firstItemValue)
			{
				// it's different, so skip...
				isSame = false;
				continue;
			}
		}

		if (isSame)
		{
			// otherwise, they were all the same, so make it a common item..
			addCommonValue(valueKey, firstItemValue);

			// and remove this item value from all items
			for (Item& item : m_aValueItems)
			{
				auto itFind = item.aValues.find(valueKey);
				// this check shouldn't really be necessary, but...
				if (itFind != item.aValues.end())
				{
					item.aValues.erase(itFind);
				}
			}
		}
	}
}
