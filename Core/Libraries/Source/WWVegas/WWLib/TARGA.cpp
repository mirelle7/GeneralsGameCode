/*
**	Command & Conquer Generals Zero Hour(tm)
**	Copyright 2025 Electronic Arts Inc.
**
**	This program is free software: you can redistribute it and/or modify
**	it under the terms of the GNU General Public License as published by
**	the Free Software Foundation, either version 3 of the License, or
**	(at your option) any later version.
**
**	This program is distributed in the hope that it will be useful,
**	but WITHOUT ANY WARRANTY; without even the implied warranty of
**	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**	GNU General Public License for more details.
**
**	You should have received a copy of the GNU General Public License
**	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

/****************************************************************************
*
*        C O N F I D E N T I A L --- W E S T W O O D   S T U D I O S
*
*----------------------------------------------------------------------------
*
* FILE
*     targa.cpp
*
* DESCRIPTION
*     Targa image file class.
*
* PROGRAMMER
*     Denzil E. Long, Jr.
*
* DATE
*     August 8, 1995
*
* MODIFICATIONS:
*     Converted to work with FileClass, FileFactory. Naty Hoffman, January 25, 2001
*     TheSuperHackers: The pixels are decoded and encoded by stb_image.
*
****************************************************************************/

#include "TARGA.h"
#include <malloc.h>
#include <memory.h>
#include "WWFILE.h"
#include "ffactory.h"
#include "WWDebug/wwdebug.h"

#include <stb_image.h>
#include <stb_image_write.h>

#include <algorithm>

Targa::Targa()
{
	mFileData = nullptr;
	mFileSize = 0;
	mFileImageDescriptor = 0;
	mFlags = 0;
	mImage = nullptr;
	memset(&Header, 0, sizeof(TGAHeader));
}

Targa::~Targa()
{
	Close();

	/* Free the image buffer if we allocated it. */
	if ((mImage != nullptr) && (mFlags & TGAF_IMAGE))
		free(mImage);
}

/****************************************************************************
*
* NAME
*     Targa::Open - Open Targa image file.
*
* FUNCTION
*     Read a Targa image file into memory and read in its header. Callers may
*     change the origin flags of the header before calling Load.
*
* RESULT
*     Error - Error code, 0 if okay.
*
****************************************************************************/

long Targa::Open(const char* name, long mode)
{
	if (mode != TGA_READMODE)
		return TGAERR_NOTSUPPORTED;

	/* File already open? */
	if (mFileData != nullptr)
		return 0;

	long error = 0;
	FileClass* file = _TheFileFactory->Get_File(name);
	if (file == nullptr)
		return TGAERR_OPEN;

	if (file->Is_Available() && file->Open(FileClass::READ)) {
		const int size = file->Size();
		if (size < (int)sizeof(TGAHeader)) {
			error = TGAERR_READ;
		} else if ((mFileData = (unsigned char*)malloc(size)) == nullptr) {
			error = TGAERR_NOMEM;
		} else {
			mFileSize = size;
			if (file->Read(mFileData, size) != size)
				error = TGAERR_READ;
		}
		file->Close();
	} else {
		error = TGAERR_OPEN;
	}
	_TheFileFactory->Return_File(file);

	if (!error)
		error = ReadHeader();

	/* Close on any error! */
	if (error)
		Close();

	return error;
}

void Targa::Close()
{
	free(mFileData);
	mFileData = nullptr;
	mFileSize = 0;
}

/****************************************************************************
*
* NAME
*     Targa::Load - Load Targa Image File.
*
* FUNCTION
*     Open and decode the Targa into the image buffer. The buffer is allocated
*     unless the client has assigned one. The rows are returned bottom-up,
*     unless the client toggled TGAIDF_YORIGIN after Open. With invert_image the
*     bytes of each true color pixel are reversed, from BGR(A) to (A)RGB.
*
* RESULT
*     Error - 0 if successful, or TGAERR_??? error code.
*
****************************************************************************/

long Targa::Load(const char* name, long flags, bool invert_image)
{
	long error = Open(name, TGA_READMODE);
	if (error)
		return error;

	/* Allocate image memory if requested to. */
	if (flags & TGAF_IMAGE) {

		/* Dispose of any previous image. */
		if ((mImage != nullptr) && (mFlags & TGAF_IMAGE)) {
			free(mImage);
			mImage = nullptr;
			mFlags &= ~TGAF_IMAGE;
		}

		/* Only allocate an image if the client hasn't assigned one. */
		if (mImage == nullptr) {
			const size_t size = (size_t)Header.Width * Header.Height * TGA_BytesPerPixel(Header.PixelDepth);
			if ((mImage = (char *)malloc(size)) != nullptr) {
				mFlags |= TGAF_IMAGE;
			} else {
				error = TGAERR_NOMEM;
			}
		}
	}

	if (!error && (mImage != nullptr))
		error = DecodeImage(invert_image);

	Close();

	return error;
}

/****************************************************************************
*
* NAME
*     Targa::Save - Save a Targa Image File.
*
* FUNCTION
*     Write the image as an 8, 24 or 32 bit Targa 2.0 file. The bytes of each
*     true color pixel are expected in (A)RGB order, as Load returns them by
*     default. TGAF_COMPRESS writes the pixels run length encoded.
*
* RESULT
*     Error - 0 if successful, or TGAERR_??? error code.
*
****************************************************************************/

struct TargaWriteContext
{
	FileClass* File;
	bool Failed;
};

static void Targa_Write_Func(void* context, void* data, int size)
{
	TargaWriteContext* write = (TargaWriteContext*)context;
	if (!write->Failed && (write->File->Write(data, size) != size))
		write->Failed = true;
}

long Targa::Save(const char* name, long flags)
{
	if (!(flags & TGAF_IMAGE) || (mImage == nullptr) || (Header.Width <= 0) || (Header.Height <= 0))
		return TGAERR_WRITE;

	const bool mono = (Header.ImageType == TGA_MONO) || (Header.ImageType == TGA_MONO_ENCODED);
	const int depth = TGA_BytesPerPixel(Header.PixelDepth);
	if ((depth != 1) && (depth != 3) && (depth != 4))
		return TGAERR_NOTSUPPORTED;

	/* stb_image_write takes top-down rows of grey, RGB or RGBA pixels. */
	const int width = Header.Width;
	const int height = Header.Height;
	unsigned char* pixels = (unsigned char*)malloc((size_t)width * height * depth);
	if (pixels == nullptr)
		return TGAERR_NOMEM;

	const bool topDown = (Header.ImageDescriptor & TGAIDF_YORIGIN) != 0;
	const bool rightToLeft = (Header.ImageDescriptor & TGAIDF_XORIGIN) != 0;
	for (int y = 0; y < height; ++y) {
		const unsigned char* src = (const unsigned char*)mImage + (size_t)(topDown ? y : (height - 1 - y)) * width * depth;
		unsigned char* dst = pixels + (size_t)y * width * depth;
		for (int x = 0; x < width; ++x, dst += depth) {
			const unsigned char* p = src + (rightToLeft ? (width - 1 - x) : x) * depth;
			if ((depth == 4) && !mono) {
				dst[0] = p[1];
				dst[1] = p[2];
				dst[2] = p[3];
				dst[3] = p[0];
			} else {
				memcpy(dst, p, depth);
			}
		}
	}

	long error = 0;
	FileClass* file = _TheWritingFileFactory->Get_File(name);
	if ((file != nullptr) && file->Open(FileClass::WRITE)) {
		TargaWriteContext context = { file, false };
		stbi_write_tga_with_rle = (flags & TGAF_COMPRESS) ? 1 : 0;
		if (!stbi_write_tga_to_func(Targa_Write_Func, &context, width, height, depth, pixels))
			context.Failed = true;

		/* The footer marks a Targa 2.0 file. Transferred map previews are rejected without it. */
		TGA2Footer footer;
		footer.Extension = 0;
		footer.Developer = 0;
		static_assert(sizeof(TGA2_SIGNATURE) - 1 == sizeof(footer.Signature), "TGA2 signature length mismatch");
		memcpy(footer.Signature, TGA2_SIGNATURE, sizeof(footer.Signature));
		footer.RsvdChar = '.';
		footer.BZST = 0;
		Targa_Write_Func(&context, &footer, sizeof(footer));

		file->Close();
		if (context.Failed)
			error = TGAERR_WRITE;
	} else {
		error = TGAERR_OPEN;
	}
	if (file != nullptr)
		_TheWritingFileFactory->Return_File(file);

	free(pixels);
	return error;
}

/****************************************************************************
*
* NAME
*     Targa::YFlip - Y flip the image.
*
* FUNCTION
*     Flip the image in memory on its Y axis. (top to bottom)
*
****************************************************************************/

void Targa::YFlip()
{
	const size_t stride = (size_t)Header.Width * TGA_BytesPerPixel(Header.PixelDepth);
	char* top = mImage;
	char* bottom = mImage + stride * (Header.Height - 1);
	for (int y = Header.Height / 2; y > 0; --y, top += stride, bottom -= stride)
		std::swap_ranges(top, top + stride, bottom);
}

/****************************************************************************
*
* NAME
*     Targa::SetImage - Set the image buffer.
*
* FUNCTION
*     Set the image buffer to one provided by the caller.
*
* RESULT
*     OldImage - Previous caller assigned image buffer.
*
****************************************************************************/

char *Targa::SetImage(char *buffer)
{
	char *oldbuffer = nullptr;

	/* Free any image buffer before assigning another. */
	if ((mImage != nullptr) && (mFlags & TGAF_IMAGE))
	{
		free(mImage);
		mImage = nullptr;
		mFlags &= ~TGAF_IMAGE;
	}

	/* Get the old user buffer. */
	if (mImage != nullptr)
		oldbuffer = mImage;

	/* Assign the new image buffer. */
	mImage = buffer;

	return (oldbuffer);
}

/****************************************************************************
*
* NAME
*     Targa::ReadHeader - Read and check the header of the file in memory.
*
* FUNCTION
*     Color mapped images are described as the true color images that they are
*     decoded to, and run length encoded images as the raw images that they are
*     decoded to.
*
****************************************************************************/

long Targa::ReadHeader()
{
	memcpy(&Header, mFileData, sizeof(TGAHeader));
	mFileImageDescriptor = Header.ImageDescriptor;

	if ((Header.Width <= 0) || (Header.Height <= 0) ||
			(Header.Width > TGA_MAX_DIMENSION) || (Header.Height > TGA_MAX_DIMENSION))
		return TGAERR_NOTSUPPORTED;

	const bool encoded = (Header.ImageType & 8) != 0;
	const int fileDepth = Header.PixelDepth;
	int depth = Header.PixelDepth;
	switch (Header.ImageType & ~8) {
		case TGA_CMAPPED:
			if ((Header.ColorMapType != 1) || (Header.PixelDepth != 8) ||
					(Header.CMapStart != 0) || (Header.CMapLength < 1) || (Header.CMapLength > 256))
				return TGAERR_NOTSUPPORTED;
			switch (Header.CMapDepth) {
				case 15: case 16: depth = 16; break;
				case 24: depth = 24; break;
				case 32: depth = 32; break;
				default: return TGAERR_NOTSUPPORTED;
			}
			break;

		case TGA_TRUECOLOR:
			if ((Header.ColorMapType != 0) || ((depth != 8) && (depth != 16) && (depth != 24) && (depth != 32)))
				return TGAERR_NOTSUPPORTED;
			break;

		case TGA_MONO:
			if ((Header.ColorMapType != 0) || (depth != 8))
				return TGAERR_NOTSUPPORTED;
			break;

		default:
			return TGAERR_NOTSUPPORTED;
	}

	/* Fail on truncated raw images, as the decoder does not. */
	if (!encoded) {
		const unsigned colorMapSize = Header.ColorMapType ? Header.CMapLength * TGA_BytesPerPixel((unsigned char)Header.CMapDepth) : 0;
		const unsigned imageSize = (unsigned)Header.Width * Header.Height * TGA_BytesPerPixel(fileDepth);
		if ((unsigned)mFileSize < sizeof(TGAHeader) + (unsigned char)Header.IDLength + colorMapSize + imageSize)
			return TGAERR_READ;
	}

	if ((Header.ImageType & ~8) == TGA_CMAPPED)
		Header.ImageType = TGA_TRUECOLOR;
	else
		Header.ImageType &= ~8;
	Header.PixelDepth = (char)depth;
	Header.ColorMapType = 0;
	Header.CMapStart = 0;
	Header.CMapLength = 0;
	Header.CMapDepth = 0;

	return 0;
}

/****************************************************************************
*
* NAME
*     Targa::DecodeImage - Decode the file in memory into the image buffer.
*
****************************************************************************/

long Targa::DecodeImage(bool invert_image)
{
	int width = 0;
	int height = 0;
	unsigned char* rgba = stbi_load_from_memory(mFileData, mFileSize, &width, &height, nullptr, 4);
	if (rgba == nullptr)
		return TGAERR_READ;

	long error = 0;
	if ((width == Header.Width) && (height == Header.Height))
		StoreImage(rgba, invert_image);
	else
		error = TGAERR_READ;

	stbi_image_free(rgba);
	return error;
}

/****************************************************************************
*
* NAME
*     Targa::StoreImage - Store top-down RGBA pixels into the image buffer.
*
* FUNCTION
*     Store the pixels in the layout of the header. The rows are stored in the
*     order of the file, flipped if TGAIDF_YORIGIN is set in the header, which is
*     what clients toggle after Open. Pixels of right-to-left files are stored
*     left-to-right.
*
****************************************************************************/

void Targa::StoreImage(const unsigned char* rgba, bool invert_image)
{
	const int width = Header.Width;
	const int height = Header.Height;
	const int depth = TGA_BytesPerPixel(Header.PixelDepth);
	const bool fileTopDown = (mFileImageDescriptor & TGAIDF_YORIGIN) != 0;
	const bool topDown = fileTopDown != ((Header.ImageDescriptor & TGAIDF_YORIGIN) != 0);
	const bool rightToLeft = (Header.ImageDescriptor & TGAIDF_XORIGIN) != 0;
	const bool invert = invert_image && (depth > 2);

	for (int y = 0; y < height; ++y) {
		const unsigned char* src = rgba + (size_t)(topDown ? y : (height - 1 - y)) * width * 4;
		unsigned char* dst = (unsigned char*)mImage + (size_t)y * width * depth;
		for (int x = 0; x < width; ++x) {
			const unsigned char* p = src + (rightToLeft ? (width - 1 - x) : x) * 4;
			switch (depth) {
				case 4:
					if (invert) {
						*dst++ = p[3]; *dst++ = p[0]; *dst++ = p[1]; *dst++ = p[2];
					} else {
						*dst++ = p[2]; *dst++ = p[1]; *dst++ = p[0]; *dst++ = p[3];
					}
					break;
				case 3:
					if (invert) {
						*dst++ = p[0]; *dst++ = p[1]; *dst++ = p[2];
					} else {
						*dst++ = p[2]; *dst++ = p[1]; *dst++ = p[0];
					}
					break;
				case 2:
				{
					/* stb_image ignores the attribute bit of 16 bit pixels, so they are stored opaque. */
					const unsigned v = 0x8000 | ((p[0] >> 3) << 10) | ((p[1] >> 3) << 5) | (p[2] >> 3);
					*dst++ = (unsigned char)(v & 0xFF);
					*dst++ = (unsigned char)(v >> 8);
					break;
				}
				default:
					*dst++ = p[0];
					break;
			}
		}
	}

	Header.ImageDescriptor &= ~(TGAIDF_XORIGIN | TGAIDF_YORIGIN);
}

// ----------------------------------------------------------------------------
//
// Output targa load error message.
//
// ----------------------------------------------------------------------------

long Targa_Error_Handler(long load_err,const char* filename)
{
	switch (load_err) {
	case 0:
		return 0;
	case TGAERR_OPEN:
		WWDEBUG_SAY(("Targa: Failed to open file \"%s\"", filename));
		break;

	case TGAERR_READ:
		WWDEBUG_SAY(("Targa: Failed to read file \"%s\"", filename));
		break;

	case TGAERR_NOTSUPPORTED:
		WWDEBUG_SAY(("Targa: File \"%s\" is an unsupported Targa type", filename));
		break;

	case TGAERR_NOMEM:
		WWDEBUG_SAY(("Targa: Failed to allocate memory for file \"%s\"", filename));
		break;

	default:
		WWDEBUG_SAY(("Targa: Unknown error when loading file \"%s\"", filename));
		break;
	}
	return load_err;
}
