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

#ifndef PHOTO_RESULTS_DATE_ACCESSOR_H
#define PHOTO_RESULTS_DATE_ACCESSOR_H

#include <vector>
#include <map>
#include <string>

#include "photo_item.h"

class PhotoResultsDateAccessor
{
public:
	PhotoResultsDateAccessor();

	void build(const std::vector<const PhotoItem*>& rawItems);

	const std::vector<const PhotoItem*>* getPhotosForYear(unsigned int year) const;
	const std::vector<const PhotoItem*>* getPhotosForYearMonth(unsigned int year, unsigned int month) const;

	std::vector<uint16_t> getListOfYears() const;
	const std::vector<uint8_t>& getListOfMonthsForYear(unsigned int year) const;

protected:

	struct YearMonth
	{
		YearMonth(unsigned int yr, unsigned int mnth) :
			year(yr), month(mnth)
		{

		}

		bool operator<(const YearMonth& rhs) const
		{
			return (year < rhs.year) || (year == rhs.year && month < rhs.month);
		}

		uint16_t	year;
		uint8_t		month;
	};

	// TODO: for the moment we'll store pointers, but we'll probably have to switch
	// to indices in the future to reduce memory usage...
	// TODO: also, would make sense to store pointer to vector to reduce re-allocations?

	std::map<unsigned short, std::vector<const PhotoItem*> >	m_aYearItems;

	std::map<YearMonth, std::vector<const PhotoItem*> >			m_aYearMonthItems;

	std::map<uint16_t, std::vector<uint8_t> >					m_aYearMonthIndices;
};

#endif // PHOTO_RESULTS_DATE_ACCESSOR_H
