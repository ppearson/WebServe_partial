#ifndef IMAGE_WRITER_EXR_H
#define IMAGE_WRITER_EXR_H

#include "io/image_writer.h"

class ImageWriterEXR : public ImageWriter
{
public:
	ImageWriterEXR();

	virtual bool writeImage(const std::string& filePath, const Image3f& image, const WriteParams& writeParams) override;

	virtual bool writeRawImageCopy(const std::string& originalFilePath, const std::string& newFilePath, const WriteRawParams& params) const override;
};

#endif // IMAGE_WRITER_EXR_H
