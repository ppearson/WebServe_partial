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

#ifndef DATE_TIME_H
#define DATE_TIME_H

#include <ctime>
#include <string>

class DateTime
{
public:
	DateTime();

	enum DateOutputFormat
	{
		eDOF_UK,
		eDOF_US
	};

	enum TimeOutputFormat
	{
		eTOF_24H,
		eTOF_12H
	};

	enum DateTimeInputFormat
	{
		eDTIF_EXIFDATETIME,
		eDTIF_DATE
	};

	bool isValid() const
	{
		return m_time != 0;
	}

	void setNow();

	std::string getFormattedDate(DateOutputFormat format) const;

	std::string getFormattedTime(TimeOutputFormat format) const;

	void setFromString(const std::string& dateString, DateTimeInputFormat format);

	void applyTimeOffset(int hours, int minutes);
	
	unsigned int getYear() const;
	// this is 0-based
	unsigned int getMonth() const;

	unsigned int getDay() const;

	bool haveTime() const
	{
		return m_haveTime;
	}

	bool operator<(const DateTime& rhs) const;


protected:
	std::time_t			m_time;

	// for the moment, also store these to make things faster, but we need to do this properly at some point...
	unsigned short		m_year;
	unsigned char		m_month;
	unsigned char		m_day;

	bool				m_haveTime;
};

#endif // DATE_TIME_H
