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

#pragma once

/****************************************************************************
*
*         C O N F I D E N T I A L --- W E S T W O O D   S T U D I O S
*
*----------------------------------------------------------------------------
*
* FILE
*     Targa.h
*
* DESCRIPTION
*     Targa image file class definitions.
*
* PROGRAMMER
*     Denzil E. Long, Jr.
*
* DATE
*     July 15, 1998
*
****************************************************************************/

#pragma pack(push, 1)

// If you wish to display loading error messages call targa functions inside of
// the following macro - for example TARGA_ERROR_HANDLER(targa.Open(filename, TGA_READMODE));
// The error code is returned back from the handler so it can be used in an expression.
long Targa_Error_Handler(long error_code,const char* filename);
#define TARGA_ERROR_HANDLER(call,filename) Targa_Error_Handler(call,filename)

/*---------------------------------------------------------------------------
 * STRUCTURES AND RELATED DEFINITIONS
 *-------------------------------------------------------------------------*/

/* TGAHeader - Targa Image File header.
 *
 * IDLength        - Size of Image ID field
 * ColorMapType    - Color map type.
 * ImageType       - Image type code.
 * CMapStart       - Color map origin.
 * CMapLength      - Color map length.
 * CMapDepth       - Depth of color map entries.
 * XOffset         - X origin of image.
 * YOffset         - Y origin of image.
 * Width           - Width of image.
 * Height          - Height of image.
 * PixelDepth      - Image pixel size
 * ImageDescriptor - Image descriptor byte.
 */
typedef struct _TGAHeader
	{
	char  IDLength;
	char  ColorMapType;
	char  ImageType;
	short CMapStart;
	short CMapLength;
	char  CMapDepth;
	short XOffset;
	short YOffset;
	short Width;
	short Height;
	char  PixelDepth;
	char  ImageDescriptor;
	} TGAHeader;

/* ImageType definiton */
#define TGA_NOIMAGE           0  /* No image data included in file */
#define TGA_CMAPPED           1  /* Color-mapped image data */
#define TGA_TRUECOLOR         2  /* Truecolor image data */
#define TGA_MONO              3  /* Monochrome image data */
#define TGA_CMAPPED_ENCODED   9  /* Color-mapped image data (Encoded) */
#define TGA_TRUECOLOR_ENCODED 10 /* Truecolor image data (Encoded) */
#define TGA_MONO_ENCODED      11 /* Monochrome image data (Encoded) */

/* ImageDescriptor definition */
#define TGAIDF_ATTRIB_BITS (0x0F<<0) /* Number of attribute bits per pixel */
#define TGAIDF_XORIGIN     (1<<4)
#define TGAIDF_YORIGIN     (1<<5)

/* Access modes. */
#define TGA_READMODE  0

/* Error codes */
#define TGAERR_OPEN         -1
#define TGAERR_READ         -2
#define TGAERR_WRITE        -3
#define TGAERR_SYNTAX       -4
#define TGAERR_NOMEM        -5
#define TGAERR_NOTSUPPORTED -6

/* Flags definitions */
#define TGAF_IMAGE    (1<<0)
#define TGAF_COMPRESS (1<<2)

/* Macro definitions */
#define TGA_BytesPerPixel(a) ((a+7) >> 3)

/* Largest width or height accepted when loading. */
#define TGA_MAX_DIMENSION 8192

/*---------------------------------------------------------------------------
 * TARGA 2.0 DEFINITIONS
 *-------------------------------------------------------------------------*/

#define TGA2_SIGNATURE "TRUEVISION-XFILE"

/* TGA2Footer - Targa 2.0 footer
 *
 * Extension - Offset to the Extension area from start of file.
 * Developer - Offset to the Developer area from start of file.
 * Signature - 16 byte Targa 2.0 signature "TRUEVISION-XFILE"
 * RsvdChar  - Reserved character, must be ASCII "." (period)
 * BZST      - Binary Zero String Terminator.
 */
typedef struct _TGA2Footer
	{
	long Extension;
	long Developer;
	char Signature[16];
	char RsvdChar;
	char BZST;
	_TGA2Footer() {}
	} TGA2Footer;

#pragma pack(pop)

/*---------------------------------------------------------------------------
 * CLASS DEFINITION
 *-------------------------------------------------------------------------*/

// Reads and writes Targa files through the WWLib file factories. The pixels are
// decoded and encoded by stb_image. Color mapped images are expanded to true color
// when opened, so the header describes the pixels that Load returns.
class Targa
	{
	public:
		/* Constructor/destructor */
		Targa();
		~Targa();

		/* Function prototypes. */
		long Open(const char* name, long mode);
		void Close();

		long Load(const char* name, long flags, bool invert_image=true);
		long Save(const char* name, long flags);

		void YFlip();

		char* SetImage(char* buffer);
		char* GetImage() const {return (mImage);}

		TGAHeader Header;

	protected:
		unsigned char* mFileData;
		int mFileSize;
		char mFileImageDescriptor;
		long mFlags;
		char* mImage;

	private:
		long ReadHeader();
		long DecodeImage(bool invert_image);
		void StoreImage(const unsigned char* rgba, bool invert_image);
	};
