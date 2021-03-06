/***************************************************************************
 *   Copyright (C) 1998-2013 by authors (see AUTHORS.txt)                  *
 *                                                                         *
 *   This file is part of LuxRender.                                       *
 *                                                                         *
 *   Lux Renderer is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 3 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   Lux Renderer is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>. *
 *                                                                         *
 *   This project is based on PBRT ; see http://www.pbrt.org               *
 *   Lux Renderer website : http://www.luxrender.org                       *
 ***************************************************************************/

#ifndef LUX_IMAGEREADER_H
#define LUX_IMAGEREADER_H
#include "lux.h"
#include "mipmap.h"

namespace lux
{

class ImageData {
public:
	enum PixelDataType {
		UNSIGNED_CHAR_TYPE,
		UNSIGNED_SHORT_TYPE,
		FLOAT_TYPE
	};

	ImageData(u_int width, u_int height, PixelDataType type, u_int noChannels, void *data) {
		width_ = width;
		height_ = height;
		pixel_type_ = type;
		noChannels_ = noChannels;
		// NOTE - Ratow - Not using auto_ptr here because of undefined behavior when deleting array data
		data_ = data;
		isExrImage_ = false;

	}

	~ImageData();

	u_int getWidth() { return width_; }

	u_int getHeight() { return height_; }

	u_int getChannels() { return noChannels_; }

	PixelDataType getPixelDataType() { return pixel_type_; }

	void *getData() { return data_; }

	bool isExrImage() { return isExrImage_; }

	void setIsExrImage(bool exrImage) { isExrImage_ = exrImage; }

	MIPMap *createMIPMap(ImageTextureFilterType filterType = BILINEAR,
		float maxAniso = 8.f, ImageWrap wrapMode = TEXTURE_REPEAT,
		float gain = 1.0f, float gamma = 1.0f);

private:
	u_int width_;
	u_int height_;
	u_int noChannels_;
	void *data_;
	PixelDataType pixel_type_;
	bool isExrImage_;
};

class ImageReader {
public:

	virtual ~ImageReader() {
	}
	virtual ImageData* read(const string &name) = 0;
};

}
#endif // LUX_IMAGEREADER_H
