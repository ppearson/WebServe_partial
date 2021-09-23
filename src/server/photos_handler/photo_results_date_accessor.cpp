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

#include "photo_results_date_accessor.h"

PhotoResultsDateAccessor::PhotoResultsDateAccessor()
{

}

void PhotoResultsDateAccessor::build(const std::vector<const PhotoItem*>& rawItems)
{
	for (const PhotoItem* pPhotoItem : rawItems)
	{
		const PhotoItem& photoItem = *pPhotoItem;

		if (!photoItem.getTimeTaken().isValid())
			continue;

		// we've got a date, so work out the year and month of it

		uint16_t year = photoItem.getTimeTaken().getYear();
		uint8_t month = photoItem.getTimeTaken().getMonth();

		auto yearIt = m_aYearItems.find(year);
		if (yearIt == m_aYearItems.end())
		{
			// we don't have a key for this one yet
			std::vector<const PhotoItem*> newVector;
			newVector.emplace_back(&photoItem);

			m_aYearItems[year] = newVector;

			std::vector<uint8_t> yearNewVector;
			m_aYearMonthIndices[year] = yearNewVector;
		}
		else
		{
			std::vector<const PhotoItem*>& aItems = yearIt->second;
			aItems.emplace_back(&photoItem);
		}

		YearMonth yearMonth(year, month);
		auto yearMonthIt = m_aYearMonthItems.find(yearMonth);
		if (yearMonthIt == m_aYearMonthItems.end())
		{
			// we don't have a key for this one yet
			std::vector<const PhotoItem*> newVector;
			newVector.emplace_back(&photoItem);

			m_aYearMonthItems[yearMonth] = newVector;

			auto yearIndexIt = m_aYearMonthIndices.find(year);
			std::vector<uint8_t>& monthsForYear = yearIndexIt->second;
			monthsForYear.push_back(month);
		}
		else
		{
			std::vector<const PhotoItem*>& aItems = yearMonthIt->second;
			aItems.emplace_back(&photoItem);
		}
	}
}

const std::vector<const PhotoItem*>* PhotoResultsDateAccessor::getPhotosForYear(unsigned int year) const
{
	auto yearIt = m_aYearItems.find(year);
	if (yearIt != m_aYearItems.end())
		return &yearIt->second;

	return nullptr;
}

const std::vector<const PhotoItem*>* PhotoResultsDateAccessor::getPhotosForYearMonth(unsigned int year, unsigned int month) const
{
	auto yearMonthIt = m_aYearMonthItems.find(YearMonth(year, month));
	if (yearMonthIt != m_aYearMonthItems.end())
		return &yearMonthIt->second;

	return nullptr;
}

// TODO: is it really worth using uint16_t if we're not storing the results perminantly?
std::vector<uint16_t> PhotoResultsDateAccessor::getListOfYears() const
{
	std::vector<uint16_t> years;

	for (auto it = m_aYearMonthIndices.begin(); it != m_aYearMonthIndices.end(); ++it)
	{
		years.push_back(it->first);
	}

	return years;
}

const std::vector<uint8_t>& PhotoResultsDateAccessor::getListOfMonthsForYear(unsigned int year) const
{
	auto yearIt = m_aYearMonthIndices.find(year);
	return yearIt->second;
}

