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

#ifndef __GDEX_IO_HPP__
#define __GDEX_IO_HPP__

#include <gd.h>
#include <algorithm> // std::swap

BGDEX_DECLARE(gdIOCtx *) gdNewRangeCtx(gdIOCtx * inner, size_t offset, size_t size);

namespace gd
{

	class IOHandle
	{
	protected:
		gdIOCtxPtr ctx;
	public:
		IOHandle() : ctx(nullptr) {};
		explicit IOHandle(gdIOCtxPtr ctx) : ctx(ctx) {}
		IOHandle(const IOHandle& oth) : ctx(oth.ctx) {}
		IOHandle& operator=(const IOHandle& oth)
		{
			ctx = oth.ctx;
			return *this;
		}
		IOHandle(IOHandle&& oth)
			: ctx(nullptr)
		{
			*this = std::move(oth);
		}
		IOHandle& operator=(IOHandle&& oth)
		{
			std::swap(ctx, oth.ctx);
			return *this;
		}

		explicit operator bool() const { return ctx != nullptr; }
		gdIOCtxPtr get() const { return ctx; }

		// GD2
		int getC() const { return ctx->getC(ctx); }
		void putC(int c) const { ctx->putC(ctx, c); }
		int getBuf(void * ptr, int size) const { return ctx->getBuf(ctx, ptr, size); }
		int putBuf(const void * ptr, int size) const { return ctx->putBuf(ctx, ptr, size); }
		long tell() const { return ctx->tell(ctx); }
		bool seek(const int offset) const { return ctx->seek(ctx, offset) != 0; }
		void gd_free() const { ctx->gd_free(ctx); }

		template <typename T>
		bool read(T& obj) const { return getBuf(&obj, sizeof(obj)) == sizeof(obj); }
	};

	class IOCtx: public IOHandle
	{
	public:
		explicit IOCtx(gdIOCtxPtr ctx) : IOHandle(ctx) {}
		IOCtx() = delete;
		IOCtx(const IOCtx&) = delete;
		IOCtx& operator=(const IOCtx&) = delete;
		IOCtx(IOCtx&& oth)
			: IOHandle(oth.release())
		{
		}
		~IOCtx()
		{
			reset();
		}
		IOCtx& operator=(IOCtx&& oth)
		{
			std::swap(ctx, oth.ctx);
			return *this;
		}

		void reset(gdIOCtxPtr ctx = nullptr)
		{
			if (this->ctx == ctx)
				return;

			if (this->ctx)
				gd_free();

			this->ctx = ctx;
		}

		gdIOCtxPtr release()
		{
			auto tmp = ctx;
			ctx = nullptr;
			return tmp;
		}

		gdIOCtxPtr get() const { return ctx; }

		void swap(IOCtx& oth)
		{
			std::swap(ctx, oth.ctx);
		}

		// GD2

		static IOCtx createFromFile(FILE* f)
		{
			return IOCtx{ gdNewFileCtx(f) };
		}

		static IOCtx createFromReadOnlyMemory(int size, void *data)
		{
			return IOCtx{ gdNewDynamicCtxEx(size, data, 0) };
		}

		static IOCtx createFromRange(const IOHandle& io, size_t offset, size_t size)
		{
			return IOCtx{ gdNewRangeCtx(io.get(), offset, size) };
		}
	};
};

namespace std
{
	inline void swap(gd::IOCtx& left, gd::IOCtx& right)
	{
		left.swap(right);
	}
}

#endif // __GDEX_IO_HPP__
