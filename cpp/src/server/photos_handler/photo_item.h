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

#ifndef PHOTO_ITEM_H
#define PHOTO_ITEM_H

#include <string>

#include "photo_representations.h"

#include "core/date_time.h"

#include "utils/exif_parser.h"
#include "utils/string_table.h"

class PhotoItem
{
public:
	PhotoItem();

	// These are bitsets so that it makes selecting multiple choices
	// more easier...
	enum SourceType
	{
		eSourceUnknown		=	0,
		eSourceSLR			=	1 << 0,
		eSourcePhone		=	1 << 1,
		eSourceCompact		=	1 << 2,
		eSourceDrone		=	1 << 3
	};

	enum ItemType
	{
		eTypeUnknown		=	0,
		eTypeStill			=	1 << 0,
		eTypeMovie			=	1 << 1,
		eTypePanorama		=	1 << 2,
		eTypeSpherical360	=	1 << 3,
		eTypeTimelapse		=	1 << 4
	};
	
	enum PermissionType
	{
		ePermissionPublic,
		ePermissionAuthorisedBasic,
		ePermissionAuthorisedAdvanced,
		ePermissionPrivate
	};

	void setInfoFromEXIF(const EXIFInfoBasic& exifInfoBasic);
	void setBasicDate(const std::string& date);

	PhotoRepresentations& getRepresentations()
	{
		return m_representations;
	}

	const PhotoRepresentations& getRepresentations() const
	{
		return m_representations;
	}

	void setSourceType(SourceType sourceType)
	{
		m_sourceType = sourceType;
	}

	SourceType getSourceType() const
	{
		return m_sourceType;
	}

	void setItemType(ItemType itemType)
	{
		m_itemType = itemType;
	}

	ItemType getItemType() const
	{
		return m_itemType;
	}
	
	void setPermissionType(PermissionType permissionType)
	{
		m_permissionType = permissionType;
	}
	
	PermissionType getPermissionType() const
	{
		return m_permissionType;
	}

	void setRating(unsigned int ratingLevel)
	{
		m_rating = ratingLevel;
	}

	unsigned int getRating() const
	{
		return m_rating;
	}

	DateTime& getTimeTaken()
	{
		return m_timeTaken;
	}

	const DateTime& getTimeTaken() const
	{
		return m_timeTaken;
	}

	void setGeoLocationPath(const StringInstance& geoLocationPath)
	{
		m_geoLocationPath = geoLocationPath;
	}

	const StringInstance& getGeoLocationPath() const
	{
		return m_geoLocationPath;
	}
	
	bool operator<(const PhotoItem& rhs) const;

protected:
	PhotoRepresentations	m_representations;

	// TODO: at some point, when memory usage becomes a concern, we ought to optimise
	//       for memory usage (maybe optionally?!?) as opposed for performance, by
	//       storing the relative path to the dir containing all photo representation of this
	//       photo item here, and PhotoRep can contain just the local filename within that directory.
	//       But that would involve concatenating strings on-the-fly which is going to be a bit slower,
	//       so we need to be careful.
//	std::string				m_basePhotoPath;

	DateTime				m_timeTaken;

	SourceType				m_sourceType;
	ItemType				m_itemType;
	
	PermissionType			m_permissionType;

	unsigned char			m_rating; // 0 == none

	StringInstance			m_geoLocationPath;
};

#endif // PHOTO_ITEM_H
