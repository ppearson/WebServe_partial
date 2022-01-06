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

#include "photo_catalogue.h"

#include <algorithm>
#include <cstdlib>

#include "io/file_io_registry.h"
#include "io/image_reader.h"

#include "utils/file_helpers.h"
#include "utils/string_helpers.h"
#include "utils/image_helpers.h"
#include "utils/exif_parser.h"
#include "utils/logger.h"

PhotoCatalogue::PhotoCatalogue() : m_queryEngine(m_aPhotoItems)
{

}

bool PhotoCatalogue::buildPhotoCatalogue(const std::string& photosBasePath, Logger& logger)
{
//	bool ret = buildPhotoCatalogueFromRawImages(photosBasePath, logger);
	bool ret = buildPhotoCatalogueFromItemFiles(photosBasePath, logger);
	if (!ret)
		return false;

	std::sort(m_aPhotoItems.begin(), m_aPhotoItems.end());
	
	logger.notice("Loaded %s photos.", StringHelpers::formatNumberThousandsSeparator(m_aPhotoItems.size()).c_str());
	
	return true;
}

bool PhotoCatalogue::buildPhotoCatalogueFromRawImages(const std::string& photosBasePath, Logger& logger)
{
	// find all .jpg files recursively...

	std::vector<std::string> aImages;
	FileHelpers::getRelativeFilesInDirectoryRecursive(photosBasePath, "", "jpg", aImages);

	std::map<std::string, unsigned int> aIndexMappings;

	bool haveEXIF = false;

	for (const std::string& image : aImages)
	{
		std::string fullPath = FileHelpers::combinePaths(photosBasePath, image);

		uint16_t imageWidth = 0;
		uint16_t imageHeight = 0;
		ImageHelpers::getImageDimensionsCrap(fullPath, imageWidth, imageHeight);

		EXIFInfoBasic exifInfo;
		haveEXIF = EXIFParser::readEXIFFromJPEGFile(fullPath, exifInfo);

		size_t thumbPos = image.find("_t.j");
		size_t halfPos = image.find("_2.j");

		std::string mainFile;

		if (thumbPos != std::string::npos)
		{
			mainFile = image.substr(0, thumbPos) + ".jpg";
		}
		else if (halfPos != std::string::npos)
		{
			mainFile = image.substr(0, halfPos) + ".jpg";
		}
		else
		{
			mainFile = image;
		}

		auto itFind = aIndexMappings.find(mainFile);
		if (itFind == aIndexMappings.end())
		{
			unsigned int newIndex = m_aPhotoItems.size();

			PhotoItem newItem;
			// TODO: can't remember what this duplication is for... doesn't really make sense... Some future intent?
			if (thumbPos != std::string::npos)
			{
				newItem.getRepresentations().addRepresentation(PhotoRepresentations::PhotoRep(image, imageWidth, imageHeight));
			}
			else if (halfPos != std::string::npos)
			{
				newItem.getRepresentations().addRepresentation(PhotoRepresentations::PhotoRep(image, imageWidth, imageHeight));
			}
			else
			{
				newItem.getRepresentations().addRepresentation(PhotoRepresentations::PhotoRep(image, imageWidth, imageHeight));
			}

			m_aPhotoItems.push_back(newItem);

			aIndexMappings[mainFile] = newIndex;
		}
		else
		{
			unsigned int itemIndex = itFind->second;
			PhotoItem& existingItem = m_aPhotoItems[itemIndex];

			// TODO: can't remember what this duplication is for... doesn't really make sense... Some future intent?
			if (thumbPos != std::string::npos)
			{
				existingItem.getRepresentations().addRepresentation(PhotoRepresentations::PhotoRep(image, imageWidth, imageHeight));
			}
			else if (halfPos != std::string::npos)
			{
				existingItem.getRepresentations().addRepresentation(PhotoRepresentations::PhotoRep(image, imageWidth, imageHeight));
			}
			else
			{
				existingItem.getRepresentations().addRepresentation(PhotoRepresentations::PhotoRep(image, imageWidth, imageHeight));
			}
		}
	}

	return true;
}

bool PhotoCatalogue::buildPhotoCatalogueFromItemFiles(const std::string& photosBasePath, Logger& logger)
{
	logger.notice("Building photo catalogue...");
	
	m_stringTable.init(32768);
	
	std::vector<std::string> aItemFiles;
	FileHelpers::getRelativeFilesInDirectoryRecursive(photosBasePath, "", "txt", aItemFiles);

	BuildContext buildContext;
	buildContext.pJPGReader = FileIORegistry::instance().createImageReaderForExtension("jpg");

	for (const std::string& relativeItemFile : aItemFiles)
	{
		std::string fullItemFilePath = FileHelpers::combinePaths(photosBasePath, relativeItemFile);

		ItemFile itemFile;
		if (!itemFile.load(fullItemFilePath))
		{
			fprintf(stderr, "Error: couldn't load item file: %s\n", fullItemFilePath.c_str());
			continue;
		}

		std::string directoryPathOfItemFile = FileHelpers::getFileDirectory(fullItemFilePath);

		// TODO: add ignore property to "comment" out files?

		std::vector<ItemFile::Item> finalItems = itemFile.getFinalBakedItems();

		for (const ItemFile::Item& item : finalItems)
		{
			processItemFileItem(buildContext, photosBasePath, directoryPathOfItemFile, item);
		}
	}

	return true;
}

void PhotoCatalogue::processItemFileItem(const BuildContext& buildContext, const std::string& photosBasePath,
										 const std::string& itemFileDirectoryPath, const ItemFile::Item& item)
{
	if (!item.hasValue("res-0-img"))
	{
		// if we haven't got a full res property, ignore it completely
		return;
	}

	// this is inefficient doing this for each item, but in theory it could be different per-item, so not really
	// sure what else to do...
	std::string itemPhotoBasePath;
	if (item.hasValue("basePath"))
	{
		itemPhotoBasePath = item.getValue("basePath");

		if (itemPhotoBasePath == ".")
		{
			itemPhotoBasePath = itemFileDirectoryPath;
		}

		// make it relative if needed
		FileHelpers::removePrefixFromPath(itemPhotoBasePath, photosBasePath);
	}
	else
	{
		itemPhotoBasePath = photosBasePath;
	}

	PhotoItem newItem;

	// see if we've got an overall date for the item, and set that.
	// This may be overwritten by the hopefully more accurate EXIF timestamp below
	if (item.hasValue("date"))
	{
		std::string dateString = item.getValue("date");
		if (!dateString.empty())
		{
			newItem.setBasicDate(dateString);
		}
	}

	if (item.hasValue("sourceType"))
	{
		std::string sourceTypeString = item.getValue("sourceType");
		if (sourceTypeString == "slr")
		{
			newItem.setSourceType(PhotoItem::eSourceSLR);
		}
		else if (sourceTypeString == "drone")
		{
			newItem.setSourceType(PhotoItem::eSourceDrone);
		}
	}

	if (item.hasValue("itemType"))
	{
		std::string itemTypeString = item.getValue("itemType");
		if (itemTypeString == "still")
		{
			newItem.setItemType(PhotoItem::eTypeStill);
		}
	}
	
	if (item.hasValue("permission"))
	{
		std::string permissionTypeString = item.getValue("permission");
		if (permissionTypeString == "authBasic")
		{
			newItem.setPermissionType(PhotoItem::ePermissionAuthorisedBasic);
		}
		else if (permissionTypeString == "authAdvanced")
		{
			newItem.setPermissionType(PhotoItem::ePermissionAuthorisedAdvanced);
		}
		else if (permissionTypeString == "private")
		{
			newItem.setPermissionType(PhotoItem::ePermissionPrivate);
		}
	}

	if (item.hasValue("geoLocationPath"))
	{
		// TODO: we could think about fully-parsing the string geo location path to components on catalogue ingestion, instead of doing this
		//       for each query...
		std::string geoLocationPathValueString = item.getValue("geoLocationPath");
		StringInstance geoLocationPathValue = m_stringTable.createString(geoLocationPathValueString);
		newItem.setGeoLocationPath(geoLocationPathValue);
	}

	char szResTemp[8];

	// try and find all the resolutions we have.
	// TODO: this isn't really ideal, although as all properties are currently strings, there's not much else we can do,
	//       although it's something to think about improving in the future...

	for (unsigned int i = 0; i < 6; i++)
	{
		sprintf(szResTemp, "res-%u", i);

		std::string resTemp = std::string(szResTemp);

		if (!item.hasValue(resTemp))
			break;

		std::string resImageValue = item.getValue(resTemp + "-img");
		if (resImageValue.empty())
			continue;

		std::string itemResValue = item.getValue(resTemp);

		unsigned int imageWidth;
		unsigned int imageHeight;

		// recreate the full path so we can open the image
		std::string relativeImagePath = FileHelpers::combinePaths(itemPhotoBasePath, resImageValue);

		std::string fullImagePath = FileHelpers::combinePaths(photosBasePath, relativeImagePath);

		// if we're the main full res (which has been copied from the original), try and extract EXIF info from it
		if (i == 0)
		{
			if (buildContext.pJPGReader)
			{
				ImageReader::RawEXIFMetaData rawEXIFMetaData;
				if (buildContext.pJPGReader->extractEXIFMetaData(fullImagePath, rawEXIFMetaData))
				{
					EXIFParser exifParser;
					EXIFInfoBasic exifInfo;
					if (exifParser.readEXIFFromMemory(rawEXIFMetaData.pData, rawEXIFMetaData.length, exifInfo))
					{
						newItem.setInfoFromEXIF(exifInfo);
					}
				}
			}
			else
			{
				EXIFParser exifParser;
				EXIFInfoBasic exifInfo;
				if (exifParser.readEXIFFromJPEGFile(fullImagePath, exifInfo))
				{
					newItem.setInfoFromEXIF(exifInfo);
				}
			}
		}

		bool haveImageRes = false;
		if (!itemResValue.empty())
		{
			// if we've got a res string, assume that's a valid image res and we don't need to open the image...
			std::string xRes;
			std::string yRes;
			if (StringHelpers::splitInTwo(itemResValue, xRes, yRes, ","))
			{
				if (!xRes.empty() && !yRes.empty())
				{
					imageWidth = atoi(xRes.c_str());
					imageHeight = atoi(yRes.c_str());
					haveImageRes = true;
				}
			}
		}

		bool imageExists = false;

		if (haveImageRes)
		{
			// do a stat to make sure image exists
			if (FileHelpers::checkFileExists(fullImagePath))
			{
				imageExists = true;
			}
		}
		else
		{
			// open the image to get the image dimensions and make sure it exists
			if (buildContext.pJPGReader)
			{
				ImageReader::ImageDetails imageDetails;
				if (buildContext.pJPGReader->getImageDetails(fullImagePath, false, imageDetails))
				{
					imageExists = true;
					imageWidth = imageDetails.width;
					imageHeight = imageDetails.height;
				}
			}
			else
			{
				if (ImageHelpers::getImageDimensions(fullImagePath, imageWidth, imageHeight))
				{
					imageExists = true;
				}
			}
		}

		if (!imageExists && i == 0)
			return;

		if (imageExists)
		{
			newItem.getRepresentations().addRepresentation(PhotoRepresentations::PhotoRep(relativeImagePath, imageWidth, imageHeight));
		}
	}

	if (item.hasValue("timeOffset"))
	{
		std::string timeOffsetValue = item.getValue("timeOffset");

		newItem.getTimeTaken().applyTimeOffset(0, 0);
	}

	m_aPhotoItems.push_back(newItem);
}
