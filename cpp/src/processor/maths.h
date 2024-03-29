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

#ifndef MATHS_H
#define MATHS_H

class MathsHelpers
{
public:

	template <typename T>
	inline static T clamp(const T& value, const T& lower, const T& upper)
	{
		return std::max(lower, std::min(value, upper));
	}

	inline static float clamp(const float& value, float lower = 0.0f, float upper = 1.0f)
	{
		return std::max(lower, std::min(value, upper));
	}
};

#endif // MATHS_H
