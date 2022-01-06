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

#ifndef IMAGE_HELPERS_H
#define IMAGE_HELPERS_H

#include <string>

class ImageHelpers
{
public:
	ImageHelpers();

	static bool getImageDimensionsCrap(const std::string& imagePath, uint16_t& width, uint16_t& height);

	static bool getImageDimensions(const std::string& imagePath, unsigned int& width, unsigned int& height);
};

#endif // IMAGE_HELPERS_H
