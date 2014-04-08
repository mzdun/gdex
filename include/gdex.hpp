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

#ifndef __GDEX_HPP__
#define __GDEX_HPP__

#include <gd.h>
#include <string>
#include <map>

namespace gd
{
	class GdImage
	{
		gdImagePtr img;

	public:
		explicit GdImage(gdImagePtr img) : img(img) {}
		GdImage() = delete;
		GdImage(const GdImage&) = delete;
		GdImage& operator=(const GdImage&) = delete;
		GdImage(GdImage&& oth)
			: img(oth.release())
		{
		}
		GdImage& operator=(GdImage&& oth)
		{
			std::swap(img, oth.img);
			return *this;
		}

		~GdImage() { if (img) gdImageDestroy(img); }
		explicit operator bool() const { return img != nullptr; }
		gdImagePtr get() { return img; }

		void reset(gdImagePtr newImg)
		{
			if (img)
				gdImageDestroy(img);
			img = newImg;
		}

		gdImagePtr release()
		{
			auto tmp = img;
			img = nullptr;
			return tmp;
		}

		void swap(GdImage& oth)
		{
			std::swap(img, oth.img);
		}

		// GD2
		size_t width() const { return img->sx; }
		size_t height() const { return img->sy; }

		void alphaBlending(bool alpha) { gdImageAlphaBlending(img, alpha ? 1 : 0); }
		void saveAlpha(bool alpha) { gdImageSaveAlpha(img, alpha ? 1 : 0); }

		int colorAllocate(int r, int g, int b, int a)
		{
			return gdImageColorAllocateAlpha(img, r, g, b, a);
		}

		int colorAllocate(int r, int g, int b)
		{
			return gdImageColorAllocate(img, r, g, b);
		}

		void copy(const GdImage& src, int dstX = 0, int dstY = 0, int srcX = 0, int srcY = 0, int w = -1, int h = -1)
		{
			if (w < 0) w = src.width();
			if (h < 0) h = src.width();
			gdImageCopy(img, src.img, dstX, dstY, srcX, srcY, w, h);
		}

		void rectangle(int color, int x = 0, int y = 0, int w = -1, int h = -1)
		{
			if (w < 0) w = width();
			if (h < 0) h = width();
			gdImageRectangle(img, x, y, x + w, y + h, color);
		}

		void fillRectangle(int color, int x = 0, int y = 0, int w = -1, int h = -1)
		{
			if (w < 0) w = width();
			if (h < 0) h = width();
			gdImageFilledRectangle(img, x, y, x + w, x + h, color);
		}

		void fill(int color, int x, int y)
		{
			gdImageFill(img, x, y, color);
		}

		void resample(int w, int h)
		{
			if (width() == w && height() == h)
				return;

			GdImage tmp{ gdImageCreateTrueColor(w, h) };
			int bck = tmp.colorAllocate(255, 255, 255, gdAlphaTransparent);
			tmp.alphaBlending(false);
			tmp.fillRectangle(bck);
			tmp.alphaBlending(true);
			tmp.saveAlpha(true);
			gdImageCopyResampled(tmp.img, img, 0, 0, 0, 0, w, h, width(), height());
			swap(tmp);
		}
	};

	gdImagePtr loadImage(int size, void* data);
	gdImagePtr loadImage(const std::string& path);
};

#endif // __GDEX_HPP__