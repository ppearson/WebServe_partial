/*
 WebServe
 Copyright 2018 Peter Pearson.

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

#include "photo_item.h"

PhotoItem::PhotoItem() :
	m_sourceType(eSourceUnknown),
	m_itemType(eTypeUnknown),
	m_permissionType(ePermissionPublic),
	m_rating(0)
{

}

void PhotoItem::setInfoFromEXIF(const EXIFInfoBasic& exifInfoBasic)
{
	if (!exifInfoBasic.m_takenDateTime.empty())
	{
		m_timeTaken.setFromString(exifInfoBasic.m_takenDateTime, DateTime::eDTIF_EXIFDATETIME);
	}
}

void PhotoItem::setBasicDate(const std::string& date)
{
	m_timeTaken.setFromString(date, DateTime::eDTIF_DATE);
}

bool PhotoItem::operator<(const PhotoItem& rhs) const
{
	return m_timeTaken < rhs.m_timeTaken;
}
