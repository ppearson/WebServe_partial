/*
 WebServe
 Copyright 2018-2022 Peter Pearson.
 taken originally from:
 Imagine
 Copyright 2012-2016 Peter Pearson.

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

#include "colour_space.h"

bool ColourSpace::m_SRGBLutTableInit = false;

float ColourSpace::m_SRGBToLinearLUT[256];

ColourSpace::ColourSpace()
{
}

void ColourSpace::convertSRGBToLinearAccurate(Colour3f& colour)
{
	colour.r = convertSRGBToLinearAccurate(colour.r);
	colour.g = convertSRGBToLinearAccurate(colour.g);
	colour.b = convertSRGBToLinearAccurate(colour.b);
}

void ColourSpace::convertLinearToSRGBAccurate(Colour3f& colour)
{
	colour.r = convertLinearToSRGBAccurate(colour.r);
	colour.g = convertLinearToSRGBAccurate(colour.g);
	colour.b = convertLinearToSRGBAccurate(colour.b);
}

void ColourSpace::convertLinearToSRGBFast(Colour3f& colour)
{
	colour.r = convertLinearToSRGBFast(colour.r);
	colour.g = convertLinearToSRGBFast(colour.g);
	colour.b = convertLinearToSRGBFast(colour.b);
}

void ColourSpace::initLUTs()
{
	if (!m_SRGBLutTableInit)
	{
		const float inv255 = (1.0f / 255.0f);
		for (unsigned int i = 0; i < 256; i++)
		{
			float linearValue = convertSRGBToLinearAccurate(float(i) * inv255);
			m_SRGBToLinearLUT[i] = linearValue;
		}
	}
}


