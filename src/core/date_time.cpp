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

#include "date_time.h"

// example EXIF date string: 2017:12:16 08:29:37

const static size_t kEXIFDateTimeStringLength = 19;


DateTime::DateTime() : m_time(0),
	m_year(0),
	m_month(0),
	m_day(0),
	m_haveTime(false)
{

}

void DateTime::setNow()
{
//	time(&m_time);
}

std::string DateTime::getFormattedDate(DateOutputFormat format) const
{
	return "";
}

std::string DateTime::getFormattedTime(TimeOutputFormat format) const
{
	return "";
}

void DateTime::setFromString(const std::string& dateString, DateTimeInputFormat format)
{
	struct tm time;

	time.tm_isdst = 0; // not really sure how to handle this robustly...

	if (format == eDTIF_EXIFDATETIME)
	{
		if (dateString.size() == kEXIFDateTimeStringLength)
		{
			// fast path... - this is *much* faster than using atoi() multiple times

			time.tm_year = (dateString[0] - '0') * 1000;
			time.tm_year += (dateString[1] - '0') * 100;
			time.tm_year += (dateString[2] - '0') * 10;
			time.tm_year += (dateString[3] - '0');

			m_year = time.tm_year;

			time.tm_mon = (dateString[5] - '0') * 10;
			time.tm_mon += (dateString[6] - '0');

			m_month = time.tm_mon - 1;

			time.tm_mday = (dateString[8] - '0') * 10;
			time.tm_mday += (dateString[9] - '0');

			m_day = time.tm_mday;

			time.tm_hour = (dateString[11] - '0') * 10;
			time.tm_hour += (dateString[12] - '0');

			time.tm_min = (dateString[14] - '0') * 10;
			time.tm_min += (dateString[15] - '0');

			time.tm_sec = (dateString[17] - '0') * 10;
			time.tm_sec += (dateString[18] - '0');

			m_haveTime = true;
		}
		else
		{
//			size_t timeStartPos = dateString.find(" ");

			return;
		}
	}
	else if (format == eDTIF_DATE)
	{
		// fast path... - this is *much* faster than using atoi() multiple times

		time.tm_year = (dateString[0] - '0') * 1000;
		time.tm_year += (dateString[1] - '0') * 100;
		time.tm_year += (dateString[2] - '0') * 10;
		time.tm_year += (dateString[3] - '0');

		m_year = time.tm_year;

		time.tm_mon = (dateString[5] - '0') * 10;
		time.tm_mon += (dateString[6] - '0');

		m_month = time.tm_mon - 1;

		time.tm_mday = (dateString[8] - '0') * 10;
		time.tm_mday += (dateString[9] - '0');

		m_day = time.tm_mday;

		time.tm_hour = 0;
		time.tm_min = 0;
		time.tm_sec = 0;
	}
	else
	{
		return;
	}

	// month is 0-based, and year is from 1900...
	time.tm_mon -= 1;
	time.tm_year -= 1900;

	m_time = mktime(&time);
}

void DateTime::applyTimeOffset(int hours, int minutes)
{
	// TODO: something better than this...

	m_time += hours * 60 * 60;
	m_time += minutes * 60;

	// TODO: correct m_day maybe?
}

unsigned int DateTime::getYear() const
{
	return m_year;
}

// this is 0-based
unsigned int DateTime::getMonth() const
{
	return m_month;
}

unsigned int DateTime::getDay() const
{
	return m_day;
}

bool DateTime::operator<(const DateTime& rhs) const
{
	if (isValid())
	{
		if (haveTime())
		{
			return m_time < rhs.m_time;
		}
		else
		{
			// TODO: this is somewhat unwise - there are better ways we could handle this in the future...
			return (m_year < rhs.m_year && m_month < rhs.m_month && m_day < rhs.m_day);
		}
	}

	return false;
}
