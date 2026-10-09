/*
**	Command & Conquer Generals Zero Hour(tm)
**	Copyright 2026 TheSuperHackers
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

#include <gtest/gtest.h>
#include <cstdio>
#include <cstring>
#include <vector>

#include "WWLib/TARGA.h"

namespace
{

typedef std::vector<unsigned char> Bytes;

const char* const TestFileName = "TargaTest.tga";

const int TestWidth = 3;
const int TestHeight = 2;

struct Pixel
{
	unsigned char b, g, r, a;
};

// The test image, top row first.
const Pixel TestImage[TestHeight][TestWidth] =
{
	{ { 0x10, 0x20, 0x30, 0xFF }, { 0x40, 0x50, 0x60, 0x80 }, { 0x70, 0x80, 0x90, 0x00 } },
	{ { 0xA0, 0xB0, 0xC0, 0x7F }, { 0xD0, 0xE0, 0xF0, 0x01 }, { 0x08, 0x18, 0x28, 0xFE } },
};

// Pixels whose channels survive a round trip through 5 bits per channel.
const Pixel TestImage16[TestHeight][TestWidth] =
{
	{ { 0x08, 0x10, 0x18, 0xFF }, { 0x20, 0x28, 0x30, 0xFF }, { 0x38, 0x40, 0x48, 0xFF } },
	{ { 0xF8, 0xF0, 0xE8, 0xFF }, { 0x00, 0x88, 0x50, 0xFF }, { 0xC8, 0x00, 0x98, 0xFF } },
};

enum PixelLayout
{
	Layout_Bgra32,
	Layout_Bgr24,
	Layout_Argb32,
	Layout_Rgb24,
	Layout_A1r5g5b5,
	Layout_Grey8,
};

void appendPixel(Bytes& out, const Pixel& p, PixelLayout layout)
{
	switch (layout)
	{
	case Layout_Bgra32: out.push_back(p.b); out.push_back(p.g); out.push_back(p.r); out.push_back(p.a); break;
	case Layout_Bgr24: out.push_back(p.b); out.push_back(p.g); out.push_back(p.r); break;
	case Layout_Argb32: out.push_back(p.a); out.push_back(p.r); out.push_back(p.g); out.push_back(p.b); break;
	case Layout_Rgb24: out.push_back(p.r); out.push_back(p.g); out.push_back(p.b); break;
	case Layout_A1r5g5b5:
	{
		const unsigned v = 0x8000 | ((p.r >> 3) << 10) | ((p.g >> 3) << 5) | (p.b >> 3);
		out.push_back((unsigned char)(v & 0xFF));
		out.push_back((unsigned char)(v >> 8));
		break;
	}
	case Layout_Grey8: out.push_back(p.g); break;
	}
}

// Rows of the given image in the requested order and layout.
Bytes imageRows(const Pixel (&image)[TestHeight][TestWidth], PixelLayout layout, bool bottomUp, bool rightToLeft = false)
{
	Bytes out;
	for (int row = 0; row < TestHeight; ++row)
	{
		const int y = bottomUp ? (TestHeight - 1 - row) : row;
		for (int col = 0; col < TestWidth; ++col)
		{
			const int x = rightToLeft ? (TestWidth - 1 - col) : col;
			appendPixel(out, image[y][x], layout);
		}
	}
	return out;
}

Bytes tgaHeader(int imageType, int pixelDepth, int descriptor, int width = TestWidth, int height = TestHeight,
	int colorMapType = 0, int colorMapLength = 0, int colorMapDepth = 0)
{
	Bytes out(18, 0);
	out[1] = (unsigned char)colorMapType;
	out[2] = (unsigned char)imageType;
	out[5] = (unsigned char)(colorMapLength & 0xFF);
	out[6] = (unsigned char)(colorMapLength >> 8);
	out[7] = (unsigned char)colorMapDepth;
	out[12] = (unsigned char)(width & 0xFF);
	out[13] = (unsigned char)(width >> 8);
	out[14] = (unsigned char)(height & 0xFF);
	out[15] = (unsigned char)(height >> 8);
	out[16] = (unsigned char)pixelDepth;
	out[17] = (unsigned char)descriptor;
	return out;
}

void append(Bytes& out, const Bytes& more)
{
	out.insert(out.end(), more.begin(), more.end());
}

void writeFile(const Bytes& data)
{
	FILE* file = fopen(TestFileName, "wb");
	ASSERT_TRUE(file != nullptr);
	ASSERT_EQ(fwrite(&data[0], 1, data.size(), file), data.size());
	fclose(file);
}

Bytes readFile()
{
	Bytes data;
	FILE* file = fopen(TestFileName, "rb");
	if (file != nullptr)
	{
		unsigned char buffer[256];
		size_t count;
		while ((count = fread(buffer, 1, sizeof(buffer), file)) > 0)
			data.insert(data.end(), buffer, buffer + count);
		fclose(file);
	}
	return data;
}

Bytes loadedImage(const Targa& targa)
{
	const size_t size = TestWidth * TestHeight * TGA_BytesPerPixel(targa.Header.PixelDepth);
	const unsigned char* image = (const unsigned char*)targa.GetImage();
	return image != nullptr ? Bytes(image, image + size) : Bytes();
}

class TargaTest : public ::testing::Test
{
protected:
	void TearDown() override
	{
		remove(TestFileName);
	}
};

} // namespace

// Targa::Load always returns the rows bottom-up, whatever order the file stores them in.
TEST_F(TargaTest, LoadsBottomUp32BitImage)
{
	Bytes file = tgaHeader(TGA_TRUECOLOR, 32, 0x08);
	append(file, imageRows(TestImage, Layout_Bgra32, true));
	writeFile(file);

	Targa targa;
	ASSERT_EQ(targa.Load(TestFileName, TGAF_IMAGE, false), 0);
	EXPECT_EQ(targa.Header.Width, TestWidth);
	EXPECT_EQ(targa.Header.Height, TestHeight);
	EXPECT_EQ(targa.Header.PixelDepth, 32);
	EXPECT_EQ(loadedImage(targa), imageRows(TestImage, Layout_Bgra32, true));
}

TEST_F(TargaTest, LoadsTopDown32BitImageBottomUp)
{
	Bytes file = tgaHeader(TGA_TRUECOLOR, 32, 0x08 | TGAIDF_YORIGIN);
	append(file, imageRows(TestImage, Layout_Bgra32, false));
	writeFile(file);

	Targa targa;
	ASSERT_EQ(targa.Load(TestFileName, TGAF_IMAGE, false), 0);
	EXPECT_EQ(targa.Header.ImageDescriptor & TGAIDF_YORIGIN, 0);
	EXPECT_EQ(loadedImage(targa), imageRows(TestImage, Layout_Bgra32, true));
}

// The texture loader flips the origin flag between Open and Load to get the rows top-down.
TEST_F(TargaTest, LoadsTopDownWhenOriginFlagIsFlippedAfterOpen)
{
	Bytes file = tgaHeader(TGA_TRUECOLOR, 32, 0x08);
	append(file, imageRows(TestImage, Layout_Bgra32, true));
	writeFile(file);

	Targa targa;
	ASSERT_EQ(targa.Open(TestFileName, TGA_READMODE), 0);
	targa.Header.ImageDescriptor ^= TGAIDF_YORIGIN;
	ASSERT_EQ(targa.Load(TestFileName, TGAF_IMAGE, false), 0);
	EXPECT_EQ(loadedImage(targa), imageRows(TestImage, Layout_Bgra32, false));
}

TEST_F(TargaTest, LoadsRightToLeftImage)
{
	Bytes file = tgaHeader(TGA_TRUECOLOR, 24, TGAIDF_XORIGIN);
	append(file, imageRows(TestImage, Layout_Bgr24, true, true));
	writeFile(file);

	Targa targa;
	ASSERT_EQ(targa.Load(TestFileName, TGAF_IMAGE, false), 0);
	EXPECT_EQ(targa.Header.ImageDescriptor & TGAIDF_XORIGIN, 0);
	EXPECT_EQ(loadedImage(targa), imageRows(TestImage, Layout_Bgr24, true));
}

TEST_F(TargaTest, InvertsChannelOrderByDefault)
{
	Bytes file = tgaHeader(TGA_TRUECOLOR, 24, 0);
	append(file, imageRows(TestImage, Layout_Bgr24, true));
	writeFile(file);

	Targa targa;
	ASSERT_EQ(targa.Load(TestFileName, TGAF_IMAGE), 0);
	EXPECT_EQ(loadedImage(targa), imageRows(TestImage, Layout_Rgb24, true));

	Bytes file32 = tgaHeader(TGA_TRUECOLOR, 32, 0x08);
	append(file32, imageRows(TestImage, Layout_Bgra32, true));
	writeFile(file32);

	Targa targa32;
	ASSERT_EQ(targa32.Load(TestFileName, TGAF_IMAGE), 0);
	EXPECT_EQ(loadedImage(targa32), imageRows(TestImage, Layout_Argb32, true));
}

// The image is 3 pixels wide, so the first run continues from the bottom row into the top row.
TEST_F(TargaTest, LoadsRunLengthEncodedImageWithRunsAcrossRows)
{
	const Pixel red = { 0x00, 0x00, 0xFF, 0xFF };
	const Pixel blue = { 0xFF, 0x00, 0x00, 0xFF };
	const Pixel green = { 0x00, 0xFF, 0x00, 0xFF };

	Bytes file = tgaHeader(TGA_TRUECOLOR_ENCODED, 24, 0);
	file.push_back(0x80 | 3); // run of 4
	appendPixel(file, red, Layout_Bgr24);
	file.push_back(1); // 2 raw pixels
	appendPixel(file, blue, Layout_Bgr24);
	appendPixel(file, green, Layout_Bgr24);
	writeFile(file);

	Bytes expected;
	appendPixel(expected, red, Layout_Bgr24);
	appendPixel(expected, red, Layout_Bgr24);
	appendPixel(expected, red, Layout_Bgr24);
	appendPixel(expected, red, Layout_Bgr24);
	appendPixel(expected, blue, Layout_Bgr24);
	appendPixel(expected, green, Layout_Bgr24);

	Targa targa;
	ASSERT_EQ(targa.Load(TestFileName, TGAF_IMAGE, false), 0);
	EXPECT_EQ(loadedImage(targa), expected);
}

TEST_F(TargaTest, LoadsGreyscaleImage)
{
	Bytes file = tgaHeader(TGA_MONO, 8, 0);
	append(file, imageRows(TestImage, Layout_Grey8, true));
	writeFile(file);

	Targa targa;
	ASSERT_EQ(targa.Load(TestFileName, TGAF_IMAGE, false), 0);
	EXPECT_EQ(targa.Header.ImageType, TGA_MONO);
	EXPECT_EQ(targa.Header.PixelDepth, 8);
	EXPECT_EQ(loadedImage(targa), imageRows(TestImage, Layout_Grey8, true));
}

TEST_F(TargaTest, Loads16BitImage)
{
	Bytes file = tgaHeader(TGA_TRUECOLOR, 16, 0x01);
	append(file, imageRows(TestImage16, Layout_A1r5g5b5, true));
	writeFile(file);

	Targa targa;
	ASSERT_EQ(targa.Load(TestFileName, TGAF_IMAGE, false), 0);
	EXPECT_EQ(targa.Header.PixelDepth, 16);
	EXPECT_EQ(loadedImage(targa), imageRows(TestImage16, Layout_A1r5g5b5, true));
}

// Color mapped images are expanded to true color, which the texture loader converts them to anyway.
TEST_F(TargaTest, LoadsColorMappedImageAsTrueColor)
{
	const Pixel palette[3] = { { 0x11, 0x22, 0x33, 0xFF }, { 0x44, 0x55, 0x66, 0xFF }, { 0x77, 0x88, 0x99, 0xFF } };
	const unsigned char indices[TestHeight][TestWidth] = { { 0, 1, 2 }, { 2, 2, 1 } };

	Bytes file = tgaHeader(TGA_CMAPPED, 8, 0, TestWidth, TestHeight, 1, 3, 24);
	for (int i = 0; i < 3; ++i)
		appendPixel(file, palette[i], Layout_Bgr24);
	Bytes expected;
	for (int y = TestHeight - 1; y >= 0; --y)
	{
		for (int x = 0; x < TestWidth; ++x)
		{
			file.push_back(indices[y][x]);
			appendPixel(expected, palette[indices[y][x]], Layout_Bgr24);
		}
	}
	writeFile(file);

	Targa targa;
	ASSERT_EQ(targa.Open(TestFileName, TGA_READMODE), 0);
	EXPECT_EQ(targa.Header.ColorMapType, 0);
	EXPECT_EQ(targa.Header.ImageType, TGA_TRUECOLOR);
	EXPECT_EQ(targa.Header.PixelDepth, 24);
	ASSERT_EQ(targa.Load(TestFileName, TGAF_IMAGE, false), 0);
	EXPECT_EQ(loadedImage(targa), expected);
}

TEST_F(TargaTest, FailsOnMissingFile)
{
	Targa targa;
	EXPECT_EQ(targa.Load(TestFileName, TGAF_IMAGE, false), TGAERR_OPEN);
}

TEST_F(TargaTest, FailsOnTruncatedImage)
{
	Bytes file = tgaHeader(TGA_TRUECOLOR, 32, 0x08);
	Bytes rows = imageRows(TestImage, Layout_Bgra32, true);
	rows.resize(rows.size() - 5);
	append(file, rows);
	writeFile(file);

	Targa targa;
	EXPECT_NE(targa.Load(TestFileName, TGAF_IMAGE, false), 0);
}

// A run that is longer than the image must not write past the image buffer.
TEST_F(TargaTest, StopsRunAtEndOfImage)
{
	Bytes file = tgaHeader(TGA_TRUECOLOR_ENCODED, 32, 0x08, 1, 1);
	for (int i = 0; i < 64; ++i)
	{
		file.push_back(0x80 | 127);
		file.push_back(0xEF); file.push_back(0xBE); file.push_back(0xAD); file.push_back(0xDE);
	}
	writeFile(file);

	Bytes buffer(4 + 1024, 0x55);
	Targa targa;
	targa.SetImage((char*)&buffer[0]);
	targa.Load(TestFileName, TGAF_IMAGE, false);
	targa.SetImage(nullptr);

	for (size_t i = 4; i < buffer.size(); ++i)
	{
		ASSERT_EQ(buffer[i], 0x55) << "Byte " << i << " past the image was overwritten";
	}
}

TEST_F(TargaTest, FailsOnNegativeDimensions)
{
	Bytes file = tgaHeader(TGA_TRUECOLOR, 32, 0x08, 0xFFFF, 0xFFFF);
	append(file, imageRows(TestImage, Layout_Bgra32, true));
	writeFile(file);

	Targa targa;
	EXPECT_NE(targa.Load(TestFileName, TGAF_IMAGE, false), 0);
}

// Save takes the channel order that Load produces by default and writes the file in Targa order.
// Transferred map previews are only accepted when they end with the Targa 2.0 footer.
TEST_F(TargaTest, SavesLoadableImageWithTga2Footer)
{
	const long flags[2] = { TGAF_IMAGE, TGAF_IMAGE | TGAF_COMPRESS };
	const int depths[2] = { 24, 32 };
	for (int f = 0; f < 2; ++f)
	{
		for (int d = 0; d < 2; ++d)
		{
			Bytes image = imageRows(TestImage, depths[d] == 32 ? Layout_Argb32 : Layout_Rgb24, true);

			Targa saver;
			saver.Header.Width = TestWidth;
			saver.Header.Height = TestHeight;
			saver.Header.PixelDepth = depths[d];
			saver.Header.ImageType = TGA_TRUECOLOR;
			saver.SetImage((char*)&image[0]);
			ASSERT_EQ(saver.Save(TestFileName, flags[f]), 0);
			saver.SetImage(nullptr);

			const Bytes saved = readFile();
			ASSERT_GE(saved.size(), sizeof(TGAHeader) + sizeof(TGA2Footer));
			TGA2Footer footer;
			memcpy(&footer, &saved[saved.size() - sizeof(footer)], sizeof(footer));
			EXPECT_EQ(memcmp(footer.Signature, TGA2_SIGNATURE, sizeof(footer.Signature)), 0);
			EXPECT_EQ(footer.RsvdChar, '.');
			EXPECT_EQ(footer.BZST, '\0');

			Targa loader;
			ASSERT_EQ(loader.Load(TestFileName, TGAF_IMAGE, false), 0);
			EXPECT_EQ(loader.Header.PixelDepth, depths[d]);
			EXPECT_EQ(loadedImage(loader), imageRows(TestImage, depths[d] == 32 ? Layout_Bgra32 : Layout_Bgr24, true));
		}
	}
}
