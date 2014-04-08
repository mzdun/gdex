/*
 * Copyright (C) 2013 midnightBITS
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy,
 * modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "gdex.hpp"

namespace le
{
	template <typename T>
	struct impl
	{
		template <size_t length>
		inline static T get(const unsigned char* ptr)
		{
			return get<length / 2>(ptr) | (get<length / 2>(ptr + length / 2) << (length * 4));
		}

		template <>
		inline static T get<1>(const unsigned char* ptr) { return *ptr; }

		inline static T read(const unsigned char* ptr) { return get<sizeof(T)>(ptr); }
	};

	template <typename T>
	inline static T read(const unsigned char*& ptr)
	{
		auto tmp = impl<T>::read(ptr);
		ptr += sizeof(T);
		return tmp;
	}
	template <typename T>
	inline static void fix(T& value)
	{
		value = impl<T>::read(reinterpret_cast<const unsigned char*>(&value));
	}
};

namespace gd { namespace bmp {

	namespace
	{
		struct BITMAPINFOHEADER
		{
			uint32_t biSize;
			int32_t  biWidth;
			int32_t  biHeight;
			uint16_t biPlanes;
			uint16_t biBitCount;
			uint32_t biCompression;
			uint32_t biSizeImage;
			int32_t  biXPelsPerMeter;
			int32_t  biYPelsPerMeter;
			uint32_t biClrUsed;
			uint32_t biClrImportant;
		};
	}

	gdImagePtr readDeviceIndependentBitmap(const IOHandle& io)
	{
		return nullptr;
	}
}}

namespace gd { namespace ico {

	namespace
	{
		enum class TYPE
		{
			ICON = 1,
			CURSOR = 2
		};

		struct ICONDIR
		{
			uint16_t idReserved;    // Reserved (must be 0)
			uint16_t idType;        // Resource Type (1 for icons)
			uint16_t idCount;       // How many images?
		};

		struct ICONDIRENTRY
		{
			uint8_t  bWidth;        // Width, in pixels, of the image
			uint8_t  bHeight;       // Height, in pixels, of the image
			uint8_t  bColorCount;   // Number of colors in image (0 if >=8bpp)
			uint8_t  bReserved;     // Reserved ( must be 0)
			uint16_t wPlanes;       // Color Planes
			uint16_t wBitCount;     // Bits per pixel
			uint32_t dwBytesInRes;  // How many bytes in this resource?
			uint32_t dwImageOffset; // Where in the file is this image?
		};
	}

	IconEntry select(const IconDirectory& entries, int iconSize)
	{
		IconEntry exact = {};
		IconEntry over = {};
		IconEntry under = {};

		for (auto&& entry : entries)
		{
			auto size = std::max(entry.width, entry.height);
			if (size == iconSize)
			{
				if (exact.bpp < entry.bpp)
					exact = entry;
			}
			else if (size < iconSize)
			{
				auto tmp = std::max(under.width, under.height);
				if (tmp <= size && under.bpp <= entry.bpp)
					under = entry;
			}
			else
			{
				auto tmp = std::max(over.width, over.height);
				if ((!tmp || tmp >= size) && over.bpp <= entry.bpp)
					over = entry;
			}
		}

		if (exact.width && exact.height)
			return exact;

		if (over.width && over.height)
			return over;

		return under;
	}

	bool readEntry(const IOHandle& io, IconDirectory& out)
	{
		ICONDIRENTRY entry;
		if (!io.read(entry))
			return false;

		le::fix(entry.wPlanes);
		le::fix(entry.wBitCount);
		le::fix(entry.dwBytesInRes);
		le::fix(entry.dwImageOffset);

		if (!entry.wPlanes)
			entry.wPlanes = 1;

		IconEntry ret
		{
			entry.bWidth,
			entry.bHeight,
			entry.wPlanes * entry.wBitCount,
			COMPRESSION::RGB,
			entry.dwBytesInRes,
			entry.dwImageOffset,
		};

		if (!ret.width)
			ret.width = 256;
		if (!ret.height)
			ret.height = 256;

		out.push_back(ret);
		return true;
	}

	bool readEntries(const IOHandle& io, size_t count, IconDirectory& out)
	{
		out.clear();
		out.reserve(count);
		if (out.capacity() < count)
			return false;

		for (size_t i = 0; i < count; ++i)
		{
			if (!readEntry(io, out))
				return false;
		}

		return true;
	}

	bool fixEntry(const IOHandle& io, IconEntry& entry)
	{
		static const unsigned char signature[] = { 0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A };
		if (!io.seek(entry.offset))
			return false;

		union
		{
			struct
			{
				unsigned char sig[sizeof(signature)];
				unsigned char rest[sizeof(bmp::BITMAPINFOHEADER) - sizeof(signature)];
			} png;
			bmp::BITMAPINFOHEADER bmp;
		} header;

		if (!io.read(header.png.sig))
			return false;

		if (!memcmp(signature, header.png.sig, sizeof(signature)))
		{
			entry.compression = COMPRESSION::PNG;
			return true;
		}

		if (!io.read(header.png.rest))
			return false;

		le::fix(header.bmp.biSize);
		le::fix(header.bmp.biWidth);
		le::fix(header.bmp.biHeight);
		le::fix(header.bmp.biPlanes);
		le::fix(header.bmp.biBitCount);
		le::fix(header.bmp.biCompression);
		le::fix(header.bmp.biSizeImage);
		le::fix(header.bmp.biXPelsPerMeter);
		le::fix(header.bmp.biYPelsPerMeter);
		le::fix(header.bmp.biClrUsed);
		le::fix(header.bmp.biClrImportant);

		if (header.bmp.biSize != sizeof(bmp::BITMAPINFOHEADER))
			return false;

		entry.width = header.bmp.biWidth;
		entry.height = header.bmp.biHeight / 2;
		entry.bpp = header.bmp.biPlanes * header.bmp.biBitCount;

		return true;
	}

	bool loadDirectory(const IOHandle& io, IconDirectory& out)
	{
		ICONDIR dir;
		if (!io.read(dir))
			return false;

		le::fix(dir.idType);
		le::fix(dir.idCount);
		if (dir.idReserved != 0 || dir.idType != (int)TYPE::ICON)
			return false;

		if (!readEntries(io, dir.idCount, out))
			return false;

		for (auto& entry : out)
		{
			if (!fixEntry(io, entry))
				return false;
		}

		return true;
	}

	gdImagePtr loadIconEntry(const IOHandle& io, const IconEntry& entry)
	{
		auto range = io.createRange(entry.offset, entry.size);
		if (!range)
			return false;

		auto image = gdImageCreateFromPngCtx(range.get());
		if (image)
			return image;

		if (!range.seek(0))
			return false;
		return bmp::readDeviceIndependentBitmap(range);
	}
}}

BGDEX_DECLARE(gdImagePtr) gdImageCreateFromIconCtx(gdIOCtx * ctx, int iconSize)
{
	gd::IOHandle io{ctx};
	gd::ico::IconDirectory dir;
	auto anchor = io.tell();
	if (!gdImageLoadIconDirectoryCtx(ctx, dir))
		return nullptr;

	auto entry = gd::ico::select(dir, iconSize);

	if (!io.seek(anchor))
		return nullptr;

	gd::GdImage image{ gdImageLoadIconEntryCtx(ctx, entry) };
	if (!image)
		return nullptr;

	if (entry.width == iconSize && entry.height == iconSize)
		return image.release();

	image.resample(iconSize, iconSize);
	return image.release();
}

BGDEX_DECLARE(gdImagePtr) gdImageCreateFromIcon(FILE * infile, int iconSize)
{
	auto ctx = gd::IOCtx::createFromFile(infile);
	return gdImageCreateFromIconCtx(ctx.get(), iconSize);
}
BGDEX_DECLARE(gdImagePtr) gdImageCreateFromIconPtr(int size, void *data, int iconSize)
{
	auto ctx = gd::IOCtx::createFromReadOnlyMemory(size, data);
	return gdImageCreateFromIconCtx(ctx.get(), iconSize);
}


BGDEX_DECLARE(bool) gdImageLoadIconDirectoryCtx(gdIOCtx * ctx, gd::ico::IconDirectory& out)
{
	return gd::ico::loadDirectory(gd::IOHandle{ ctx }, out);
}
BGDEX_DECLARE(bool) gdImageLoadIconDirectory(FILE * infile, gd::ico::IconDirectory& out)
{
	auto ctx = gd::IOCtx::createFromFile(infile);
	return gdImageLoadIconDirectoryCtx(ctx.get(), out);
}
BGDEX_DECLARE(bool) gdImageLoadIconDirectoryPtr(int size, void *data, gd::ico::IconDirectory& out)
{
	auto ctx = gd::IOCtx::createFromReadOnlyMemory(size, data);
	return gdImageLoadIconDirectoryCtx(ctx.get(), out);
}

BGDEX_DECLARE(gdImagePtr) gdImageLoadIconEntryCtx(gdIOCtx * ctx, const gd::ico::IconEntry& entry)
{
	return gd::ico::loadIconEntry(gd::IOHandle{ ctx }, entry);
}
BGDEX_DECLARE(gdImagePtr) gdImageLoadIconEntry(FILE * infile, const gd::ico::IconEntry& entry)
{
	auto ctx = gd::IOCtx::createFromFile(infile);
	return gdImageLoadIconEntryCtx(ctx.get(), entry);
}
BGDEX_DECLARE(gdImagePtr) gdImageLoadIconEntryPtr(int size, void *data, const gd::ico::IconEntry& entry)
{
	auto ctx = gd::IOCtx::createFromReadOnlyMemory(size, data);
	return gdImageLoadIconEntryCtx(ctx.get(), entry);
}
