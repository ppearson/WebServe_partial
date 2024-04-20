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

#include "image_reader_heif.h"

#include <libheif/heif.h>

#include <memory.h>
#include <memory>
#include <vector>

#ifdef WEBSERVE_ENABLE_LCMS_SUPPORT
#include <lcms2.h>
#endif

#include "image/image3f.h"

#include "image/colour_space.h"

#include "data_conversion.h"

ImageReaderHEIF::ImageReaderHEIF()
{

}

ImageReaderHEIF::HEIFRawEXIFMetaDataPayload::~HEIFRawEXIFMetaDataPayload()
{
	if (pOriginalPayload && originalSize > 0)
	{
		delete pOriginalPayload;
		pOriginalPayload = nullptr;
		originalSize = 0;
	}
}

bool ImageReaderHEIF::getImageDetails(const std::string& filePath, bool extractEXIF, ImageDetails& imageDetails) const
{
	std::shared_ptr<heif_context> ctx(heif_context_alloc(), [](heif_context* c) { heif_context_free(c); });
	if (!ctx)
	{
		fprintf(stderr, "Error allocating heif_context object.\n");
		return false;
	}

	struct heif_error err;
	err = heif_context_read_from_file(ctx.get(), filePath.c_str(), nullptr);

	if (err.code != 0)
	{
		fprintf(stderr, "Error reading HEIF file: '%s', error: %s\n", filePath.c_str(), err.message);
		return false;
	}

	struct heif_image_handle* handle;
	err = heif_context_get_primary_image_handle(ctx.get(), &handle);
	if (err.code != 0)
	{
		fprintf(stderr, "Error: Could not get primary image handle for HEIF file: '%s', error: %s\n", filePath.c_str(), err.message);
		return false;
	}

	int width = heif_image_handle_get_width(handle);
	int height = heif_image_handle_get_height(handle);

	imageDetails.width = (unsigned int)width;
	imageDetails.height = (unsigned int)height;

	bool hasAlpha = heif_image_handle_has_alpha_channel(handle);
	bool hasDepth = heif_image_handle_has_depth_image(handle);

	unsigned int numChannels = 3;
	if (hasAlpha)
	{
		numChannels += 1;
	}
	if (hasDepth)
	{
		numChannels += 1;
	}

	imageDetails.channels = numChannels;

/*
	// we seem to need to do the full decode to get the next bit...
	struct heif_image* image;
	err = heif_decode_image(handle, &image, heif_colorspace_undefined, heif_chroma_undefined, nullptr);
	if (err.code != 0)
	{
		heif_image_handle_release(handle);
		fprintf(stderr, "Error: Could not decode HEIF image: '%s', error: %s\n", filePath.c_str(), err.message);
		return false;
	}

	// for the moment just look at the R channel, but each channel can have a different bit depth, so...
	int pixelBitDepth = heif_image_get_bits_per_pixel_range(image, heif_channel_R);
	imageDetails.pixelBitDepth = (unsigned int)pixelBitDepth;

*/

	int pixelBitDepth = heif_image_handle_get_luma_bits_per_pixel(handle);
	imageDetails.pixelBitDepth = (unsigned int)pixelBitDepth;

	enum heif_color_profile_type profileType = heif_image_handle_get_color_profile_type(handle);

	size_t profile_size = heif_image_handle_get_raw_color_profile_size(handle);

	// TODO: metadata

//	heif_image_release(image);
	heif_image_handle_release(handle);

	return true;
}

bool ImageReaderHEIF::extractEXIFMetaData(const std::string& filePath, RawEXIFMetaData& exifData) const
{
	std::shared_ptr<heif_context> ctx(heif_context_alloc(), [](heif_context* c) { heif_context_free(c); });
	if (!ctx)
	{
		fprintf(stderr, "Error allocating heif_context object.\n");
		return false;
	}

	struct heif_error err;
	err = heif_context_read_from_file(ctx.get(), filePath.c_str(), nullptr);

	if (err.code != 0)
	{
		fprintf(stderr, "Error reading HEIF file: '%s', error: %s\n", filePath.c_str(), err.message);
		return false;
	}

	struct heif_image_handle* handle;
	err = heif_context_get_primary_image_handle(ctx.get(), &handle);
	if (err.code != 0)
	{
		fprintf(stderr, "Error: Could not get primary image handle for HEIF file: '%s', error: %s\n", filePath.c_str(), err.message);
		return false;
	}

	heif_item_id metadataId;
	int metaDataBlocks = heif_image_handle_get_list_of_metadata_block_IDs(handle, "Exif", &metadataId, 1);

	for (int i = 0; i < metaDataBlocks; i++)
	{
		size_t metaDataSize = heif_image_handle_get_metadata_size(handle, metadataId);
		if (metaDataSize > 0)
		{
			uint8_t* data = new uint8_t[metaDataSize];

			heif_error error = heif_image_handle_get_metadata(handle, metadataId, data);
			if (error.code != heif_error_Ok)
			{
				delete [] data;
				continue;
			}

			// allocate a temp payload in the metadata struct, such that we can close the file down
			// but keep the metadata memory alive for a bit longer (the duration of the RawEXIFMetaData struct)
			ImageReaderHEIF::HEIFRawEXIFMetaDataPayload* pTypedPayload = new ImageReaderHEIF::HEIFRawEXIFMetaDataPayload();
			// we purposefully allocate this within the
			exifData.pTempPayload = pTypedPayload;

			// Note: for some reason, it looks like the pointer returned here often points at 4 bytes of padding of 0 bytes
			//       before the actual Exif header 'Exif\0\0' token, at least for .HEIC files from the iPhone 13...

			// keep track of the original pointer and size we got from heif_image_handle_get_metadata()...
			pTypedPayload->pOriginalPayload = data;
			pTypedPayload->originalSize = metaDataSize;

			// because of the 4 bytes of padding that appear to sometimes exist at the beginning of the data (from iPhone 13 images for example),
			// try and see if we can find where it starts a few bytes in...
			if (!std::equal(data, data + 6, "Exif\0\0"))
			{
				if (metaDataSize > 30)
				{
					for (unsigned int tempOffset = 1; tempOffset < 10; tempOffset++)
					{
						if (std::equal(data + tempOffset, data + tempOffset + 6, "Exif\0\0"))
						{
							// we've found it, so adjust the pointer (we're keeping track of the original in the typed payload so we can
							// correctly delete it later), and reduce the length to compensate...
							data += tempOffset;
							metaDataSize -= tempOffset;
							break;
						}
					}
				}
			}

			// now either way, set the data pointers to either the original pointer and size, or our altered values.
			exifData.pData = (unsigned char*)data;
			exifData.length = metaDataSize;

			return true;
		}
	}

	heif_image_handle_release(handle);

	return false;
}

Image3f* ImageReaderHEIF::readColour3fImage(const std::string& filePath) const
{
	std::shared_ptr<heif_context> ctx(heif_context_alloc(), [](heif_context* c) { heif_context_free(c); });
	if (!ctx)
	{
		fprintf(stderr, "Error allocating heif_context object.\n");
		return nullptr;
	}

	struct heif_error err;
	err = heif_context_read_from_file(ctx.get(), filePath.c_str(), nullptr);

	if (err.code != 0)
	{
		fprintf(stderr, "Error reading HEIF file: '%s', error: %s\n", filePath.c_str(), err.message);
		return nullptr;
	}

	struct heif_image_handle* handle;
	err = heif_context_get_primary_image_handle(ctx.get(), &handle);
	if (err.code != 0)
	{
		fprintf(stderr, "Error: Could not get primary image handle for HEIF file: '%s', error: %s\n", filePath.c_str(), err.message);
		return nullptr;
	}

	//

	struct heif_decoding_options* decodingOptions = heif_decoding_options_alloc();

	int bitDepth = heif_image_handle_get_luma_bits_per_pixel(handle);
	if (bitDepth < 0 || bitDepth > 14)
	{
		heif_decoding_options_free(decodingOptions);
		heif_image_handle_release(handle);
		fprintf(stderr, "Error: HEIF file has unrecognised bit depth, and can't be decoded: '%s'\n", filePath.c_str());
		return nullptr;
	}

	enum heif_colorspace colorspace = heif_colorspace_RGB;
	// Note: this assumes no alpha...
	enum heif_chroma chroma = heif_chroma_interleaved_RGB;
	if (bitDepth > 8)
	{
		chroma = heif_chroma_interleaved_RRGGBB_BE;
	}

	struct heif_image* image;
	err = heif_decode_image(handle, &image, colorspace, chroma, decodingOptions);
	heif_decoding_options_free(decodingOptions);
	if (err.code != 0)
	{
		heif_image_handle_release(handle);
		fprintf(stderr, "Error: Could not decode HEIF image: '%s', error: %s\n", filePath.c_str(), err.message);
		return nullptr;
	}

	unsigned int width = heif_image_handle_get_width(handle);
	unsigned int height = heif_image_handle_get_height(handle);

	int pixelStride;
	const uint8_t* pixelRowValues = heif_image_get_plane_readonly(image, heif_channel_interleaved, &pixelStride);
	if (pixelRowValues == nullptr)
	{
		heif_image_release(image);
		heif_image_handle_release(handle);

		fprintf(stderr, "Error: Could not obtain image pixel values from HEIF image: '%s'\n", filePath.c_str());
		return nullptr;
	}

	Image3f* pImage3f = new Image3f(width, height);

	if (bitDepth == 8)
	{
		// we can do things the easy way and just ingest the uint8_t values directly via the LUT...
		for (unsigned int i = 0; i < height; i++)
		{
			// need to flip the height round...
			unsigned int y = height - i - 1;

			Colour3f* pImageRow = pImage3f->getRowPtr(y);

			const uint8_t* pSrcPixel = pixelRowValues + ((width * i) * 3);

			for (unsigned int x = 0; x < width; x++)
			{
				uint8_t red = *pSrcPixel++;
				uint8_t green = *pSrcPixel++;
				uint8_t blue = *pSrcPixel++;

				pImageRow->r = ColourSpace::convertSRGBToLinearLUT(red);
				pImageRow->g = ColourSpace::convertSRGBToLinearLUT(green);
				pImageRow->b = ColourSpace::convertSRGBToLinearLUT(blue);

				pImageRow++;
			}
		}
	}
	else
	{
		// we asked for > 8-bit data to be returned as 16-bit data... The bits *seem* to always be 2 bytes,
		// but we have to scale the value by the number of bits apparently, not 1.0f / 65535.0f....

		// Note: for > 8-bit data, it's very likely that colour profile conversion is going to be *very* important
		//       for valid values on top of this.

		float invConvert = 1.0f / (float)(1 << bitDepth);

		for (unsigned int i = 0; i < height; i++)
		{
			// need to flip the height round...
			unsigned int y = height - i - 1;

			Colour3f* pImageRow = pImage3f->getRowPtr(y);

			const uint16_t* pSrcPixel = (const uint16_t*)pixelRowValues + ((width * i) * 3);

			for (unsigned int x = 0; x < width; x++)
			{
				uint16_t red = *pSrcPixel++;
				uint16_t green = *pSrcPixel++;
				uint16_t blue = *pSrcPixel++;

				red = DataConversionHelpers::reverseUInt16Bytes(red);
				green = DataConversionHelpers::reverseUInt16Bytes(green);
				blue = DataConversionHelpers::reverseUInt16Bytes(blue);

				pImageRow->r = (float)red * invConvert;
				pImageRow->g = (float)green * invConvert;
				pImageRow->b = (float)blue * invConvert;

				pImageRow++;
			}
		}

		// now try and work out what colourspace conversion to do based off the profile.
		enum heif_color_profile_type profileType = heif_image_handle_get_color_profile_type(handle);
		if (profileType == heif_color_profile_type_nclx)
		{
			struct heif_color_profile_nclx* pNCLXProfileInfo = nullptr;
			err = heif_image_handle_get_nclx_color_profile(handle, &pNCLXProfileInfo);
			if (err.code != 0)
			{
				heif_image_handle_release(handle);
				fprintf(stderr, "Warning: Could not extract NCLX colour profile of HEIF image: '%s', error: %s\n", filePath.c_str(), err.message);
			}
			else
			{
				// TODO:....
			}
		}
		else if (profileType == heif_color_profile_type_rICC || profileType == heif_color_profile_type_prof)
		{
			size_t profileSize = heif_image_handle_get_raw_color_profile_size(handle);

			uint8_t* profileData = new uint8_t[profileSize];
			err = heif_image_handle_get_raw_color_profile(handle, profileData);

			if (err.code != 0)
			{
				heif_image_handle_release(handle);
				fprintf(stderr, "Warning: Could not extract colour profile of HEIF image: '%s', error: %s\n", filePath.c_str(), err.message);
			}
			else
			{
#ifdef WEBSERVE_ENABLE_LCMS_SUPPORT
				cmsHPROFILE inputProfile = cmsOpenProfileFromMem(profileData, profileSize);

				cmsHPROFILE outputProfile = cmsCreateXYZProfile();

				cmsHTRANSFORM colorTransform = cmsCreateTransform(inputProfile, TYPE_RGB_FLT,
																	outputProfile, TYPE_RGB_FLT, INTENT_PERCEPTUAL, 0);

				void* pixels = pImage3f->getRowPtr(0);

				cmsDoTransform(colorTransform, pixels, pixels, width * height);

				cmsDeleteTransform(colorTransform);
				cmsCloseProfile(inputProfile);
				cmsCloseProfile(outputProfile);
#else
				fprintf(stderr, "Warning: littlecms color profile support was not compiled in. It is very likely that the image values from the image are incorrect.\n");
#endif
			}

		}

	}


	heif_image_release(image);

	return pImage3f;
}

namespace
{
	ImageReader* createImageReaderHEIF()
	{
		return new ImageReaderHEIF();
	}

	const bool registered = FileIORegistry::instance().registerImageReaderMultipleExtensions("heif;heic;hif", createImageReaderHEIF);
}
