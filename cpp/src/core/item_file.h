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

#ifndef ITEM_FILE_H
#define ITEM_FILE_H

#include <vector>
#include <string>
#include <map>

class ItemFile
{
public:
	ItemFile();

	struct Item
	{
		void addValue(const std::string& key, const std::string& value)
		{
			aValues[key] = value;
		}

		bool hasValue(const std::string& key) const
		{
			return aValues.count(key) > 0;
		}

		std::string getValue(const std::string& key) const
		{
			const auto& itFind = aValues.find(key);
			if (itFind != aValues.end())
			{
				return itFind->second;
			}

			return "";
		}

		std::map<std::string, std::string>  aValues;
	};

	bool load(const std::string& filePath);
	bool save(const std::string& filePath) const;

	// for getting the final values on a per-item basis after loading
	std::vector<Item> getFinalBakedItems() const;

	void addCommonValue(const std::string& key, const std::string& value);
	void addItem(const Item& item);

	void promoteSameValueItemValuesToCommon();

protected:
	// key/values applied to all items - although items can themselves then override these on a per-item basis
	std::map<std::string, std::string>	m_aCommonValues;

	std::vector<std::map<std::string, std::string> >	m_aOverrides;

	std::vector<Item>	m_aValueItems;

	// actual indices to items, with left-most bit indicating whether it's an item or override at the position.
	std::vector<unsigned int>	m_aItemIndices;
};

#endif // ITEM_FILE_H
