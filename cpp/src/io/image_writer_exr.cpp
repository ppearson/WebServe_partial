#include "image_writer_exr.h"

#include <cstdio>
#include <vector>

#include <ImfOutputFile.h>
#include <ImfChannelList.h>
#include <ImfFrameBuffer.h>
#include <ImfStringAttribute.h>
#include <ImfMultiPartOutputFile.h>
#include <half.h>

#include "maths.h"

#include "image/colour_space.h"
#include "image/image3f.h"

using namespace Imath;

ImageWriterEXR::ImageWriterEXR()
{

}

bool ImageWriterEXR::writeImage(const std::string& filePath, const Image3f& image, const WriteParams& writeParams)
{
	unsigned int width = image.getWidth();
	unsigned int height = image.getHeight();

	Imf::Header header(width, height);

	Imf::StringAttribute sourceAttribute;
//	sourceAttribute.value() = "";
//	header.insert("comments", sourceAttribute);
//	header.lineOrder() = Imf::DECREASING_Y;

	Imf::PixelType pixelType = writeParams.bitDepth == eBitDepth_32 ? Imf::FLOAT : Imf::HALF;

	if (writeParams.bitDepth != eBitDepth_16 && writeParams.bitDepth != eBitDepth_32)
	{
		fprintf(stderr, "Warning: An unsupport bitdepth was request for writing to EXR: %u bits, but as it is < 32, WebServe process will automatically write a 16-bit half EXR.\n",
				writeParams.getRawBitDepth());
	}

	header.channels().insert("R", Imf::Channel(pixelType));
	header.channels().insert("G", Imf::Channel(pixelType));
	header.channels().insert("B", Imf::Channel(pixelType));

	Imf::FrameBuffer fb;

	std::unique_ptr<float> RGBFloat;
	std::unique_ptr<half> RGBHalf;

	if (writeParams.bitDepth == eBitDepth_32)
	{
		// in theory, we can just use Image3f contents directly, but...
		RGBFloat.reset(new float[width * height * 3]);

		float* pixelData = RGBFloat.get();

		for (unsigned int y = 0; y < height; y++)
		{
			unsigned int actualY = height - y - 1;
			const Colour3f* pRow = image.getRowPtr(actualY);

			unsigned int rgbStartPos = width * y * 3;

			for (unsigned int x = 0; x < width; x++)
			{
				unsigned int pixelPos = rgbStartPos + (x * 3);

				float red = pRow->r;
				float green = pRow->g;
				float blue = pRow->b;

				pixelData[pixelPos++] = red;
				pixelData[pixelPos++] = green;
				pixelData[pixelPos] = blue;

				pRow++;
			}
		}

		fb.insert("R", Imf::Slice(pixelType, (char*)pixelData, 3 * sizeof(float), 3 * width * sizeof(float)));
		fb.insert("G", Imf::Slice(pixelType, (char*)pixelData + sizeof(float), 3 * sizeof(float), 3 * width * sizeof(float)));
		fb.insert("B", Imf::Slice(pixelType, (char*)pixelData + 2 * sizeof(float), 3 * sizeof(float), 3 * width * sizeof(float)));
	}
	else
	{
		RGBHalf.reset(new half[width * height * 3]);

		half* pixelData = RGBHalf.get();

		for (unsigned int y = 0; y < height; y++)
		{
			unsigned int actualY = height - y - 1;
			const Colour3f* pRow = image.getRowPtr(actualY);

			unsigned int rgbStartPos = width * y * 3;

			for (unsigned int x = 0; x < width; x++)
			{
				unsigned int pixelPos = rgbStartPos + (x * 3);

				// TODO: technically, we should clamp here, in case the value is more than half can cope with,
				//       otherwise we'll get half INF values...
				half red = pRow->r;
				half green = pRow->g;
				half blue = pRow->b;

				pixelData[pixelPos++] = red;
				pixelData[pixelPos++] = green;
				pixelData[pixelPos] = blue;

				pRow++;
			}
		}

		fb.insert("R", Imf::Slice(pixelType, (char*)pixelData, 3 * sizeof(half), 3 * width * sizeof(half)));
		fb.insert("G", Imf::Slice(pixelType, (char*)pixelData + sizeof(half), 3 * sizeof(half), 3 * width * sizeof(half)));
		fb.insert("B", Imf::Slice(pixelType, (char*)pixelData + 2 * sizeof(half), 3 * sizeof(half), 3 * width * sizeof(half)));
	}

	try
	{
		Imf::OutputFile file(filePath.c_str(), header);
		file.setFrameBuffer(fb);
		file.writePixels(height);
	}
	catch (const Iex::BaseExc& e)
	{
		fprintf(stderr, "Error writing EXR file: '%s', : %s\n", filePath.c_str(), e.what());
		return false;
	}

	return true;
}

bool ImageWriterEXR::writeRawImageCopy(const std::string& originalFilePath, const std::string& newFilePath, const WriteRawParams& params) const
{
	return false;
}


namespace
{
	ImageWriter* createImageWriterEXR()
	{
		return new ImageWriterEXR();
	}

	const bool registered = FileIORegistry::instance().registerImageWriter("exr", createImageWriterEXR);
}
