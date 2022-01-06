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

#ifndef PHOTOS_COMMON_H
#define PHOTOS_COMMON_H

struct DateParams
{
	enum Type
	{
		eInvalid,
		eYearOnly,
		eYearAndMonth
	};

	Type			type = eInvalid;
	unsigned int	year = -1u;
	unsigned int	month = -1u;
};

#endif // PHOTOS_COMMON_H
