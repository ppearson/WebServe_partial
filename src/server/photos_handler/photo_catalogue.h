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

#ifndef PHOTO_CATALOGUE_H
#define PHOTO_CATALOGUE_H

#include <vector>
#include <map>
#include <string>

#include "configuration.h"
#include "photo_item.h"
#include "photo_query_engine.h"

#include "core/item_file.h"

#include "utils/string_table.h"

class ImageReader;

class Logger;

// TODO: abstract this so we can have multiple backends, or at least multiple plugins to it,
//       i.e. a database one would likely make quite a bit of sense...

class PhotoCatalogue
{
public:
	PhotoCatalogue();

	bool buildPhotoCatalogue(const std::string& photosBasePath, Logger& logger);

	const std::vector<PhotoItem>& getRawItems() const
	{
		return m_aPhotoItems;
	}

	PhotoQueryEngine& getQueryEngine()
	{
		return m_queryEngine;
	}

protected:
	struct BuildContext
	{
		ImageReader*	pJPGReader = nullptr;
	};

	bool buildPhotoCatalogueFromRawImages(const std::string& photosBasePath, Logger& logger);
	bool buildPhotoCatalogueFromItemFiles(const std::string& photosBasePath, Logger& logger);

	void processItemFileItem(const BuildContext& buildContext, const std::string& photosBasePath,
							 const std::string& itemFileDirectoryPath, const ItemFile::Item& item);

protected:
	std::vector<PhotoItem>		m_aPhotoItems;

	// Note: this stores some string items contained within the above items.
	StringTable					m_stringTable;

	PhotoQueryEngine			m_queryEngine;
};

#endif // PHOTO_CATALOGUE_H
