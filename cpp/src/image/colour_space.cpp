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

// Not sure about this...
void ColourSpace::convertAdobeRGBToLinearAccurate(Colour3f& colour)
{
	// the assumption here is that input values will be in "normalised" "space", i.e.
	// for uint16_t values, they'll just be the raw 1.0f / 65535.0f value.

	// first of all, convert each colour value with a different (Adobe) mapping...
	colour.r = convertAdobeRGBToLinearAccurate(colour.r);
	colour.g = convertAdobeRGBToLinearAccurate(colour.g);
	colour.b = convertAdobeRGBToLinearAccurate(colour.b);

	// now convert from AdobeRGB to XYZ
	Colour3f XYZ = fromAdobeRGBtoXYZ(colour.r, colour.g, colour.b);
	// now convert to linear RGB...
	Colour3f linear = fromXYZtoLinearRGB(XYZ.r, XYZ.g, XYZ.b);

	colour.r = linear.r;
	colour.g = linear.g;
	colour.b = linear.b;
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

Colour3f ColourSpace::fromAdobeRGBtoXYZ(float r, float g, float b)
{
	float X = r * 0.57667f + g * 0.18556f + b * 0.18823f;
	float Y = r * 0.29734f + g * 0.62736f + b * 0.07529f;
	float Z = r * 0.02703f + g * 0.07069f + b * 0.99134f;

	return Colour3f(X, Y, Z);
}

Colour3f ColourSpace::fromXYZtoLinearRGB(float X, float Y, float Z)
{
//	float r = (3.240479f * X) + (-1.537150f * Y) + (-0.498535f * Z);
//	float g = (-0.969256f * X) + (1.875991f * Y) + (0.041556f * Z);
//	float b = (0.055648f * X) + (-0.204043f * Y) + (1.057311f * Z);

	float r = (3.240479f * X) - (1.537150f * Y) - (0.498535f * Z);
	float g = (-0.969256f * X) + (1.875991f * Y) + (0.041556f * Z);
	float b = (0.055648f * X) - (0.204043f * Y) + (1.057311f * Z);

	// clamp negatives
	Colour3f final(std::max(r, 0.0f), std::max(g, 0.0f), std::max(b, 0.0f));
	return final;
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


