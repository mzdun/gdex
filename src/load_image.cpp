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
#include <sys/stat.h>
#include <sys/types.h>

namespace gd
{
	template <typename T>
	struct unique_array
	{
		T* ptr = nullptr;
		size_t size = 0;
		bool resize(size_t size)
		{
			T* tmp = (T*)realloc(ptr, size);
			if (!tmp)
				return false;

			ptr = tmp;
			this->size = size;
			return true;
		}
		bool grow()
		{
			if (!size)
				return resize(10240);
			return resize(size << 1);
		}
		~unique_array() { free(ptr); }
	};

	class File
	{
		FILE* f;

	public:
		explicit File(FILE* f) : f(f) {}
		~File() { if (f) fclose(f); }
		explicit operator bool() const { return f != nullptr; }

		size_t read(char* buffer, size_t size)
		{
			return fread(buffer, 1, size, f);
		}
	};

	BGDEX_DECLARE_CC(gdImagePtr) loadImage(int size, void* data)
	{
#ifdef WIN32
#define CALLTYPE _stdcall
#else
#define CALLTYPE
#endif
		using creator_t = gdImagePtr(CALLTYPE*)(int size, void *data);
		creator_t creators[] = {
			gdImageCreateFromPngPtr,
			gdImageCreateFromJpegPtr,
			gdImageCreateFromGifPtr
		};

		for (auto&& creator : creators)
		{
			auto ret = creator(size, data);
			if (ret)
				return ret;
		}
		return nullptr;
	}

	BGDEX_DECLARE_CC(gdImagePtr) loadImage(const std::string& path)
	{

		File f{ fopen(path.c_str(), "rb") };
		if (!f)
			return nullptr;

		unique_array<char> buffer;

		struct stat st;
		if (!stat(path.c_str(), &st))
		{
			if (!buffer.resize((size_t)st.st_size))
				return nullptr;
		}
		else
			return nullptr;

		if (f.read(buffer.ptr, buffer.size) != buffer.size)
			return nullptr;

		return loadImage(buffer.size, buffer.ptr);
	}
}
