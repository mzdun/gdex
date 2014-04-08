/*
 * Copyright (C) 2013 midnightBITS
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * ranges (the "Software"), to deal in the Software without
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

namespace gd
{
	struct RangeContext : gdIOCtx
	{

		IOHandle inner;
		size_t offset;
		size_t size;
		size_t ptr;

		static RangeContext* _this(gdIOCtx* ctx) { return static_cast<RangeContext*>(ctx); }
		static const IOHandle& _inner(gdIOCtx* ctx) { return _this(ctx)->inner; }

		static void rangePutchar(gdIOCtx* ctx, int c)
		{
			if (_this(ctx)->ptr < _this(ctx)->size)
			{
				++_this(ctx)->ptr;
				return _inner(ctx).putC(c);
			}
		}

		static int rangeGetchar(gdIOCtx* ctx)
		{
			if (_this(ctx)->ptr < _this(ctx)->size)
			{
				++_this(ctx)->ptr;
				return _inner(ctx).getC();
			}
			return EOF;
		}

		static int rangeGetbuf(gdIOCtx* ctx, void * ptr, int size)
		{
			auto rest = _this(ctx)->size - _this(ctx)->ptr;
			if ((size_t)size > rest)
				size = rest;

			auto read = _inner(ctx).getBuf(ptr, size);
			_this(ctx)->ptr += read;
			return read;
		}
		static int rangePutbuf(gdIOCtx* ctx, const void * ptr, int size)
		{
			auto rest = _this(ctx)->size - _this(ctx)->ptr;
			if ((size_t)size > rest)
				size = rest;

			auto written = _inner(ctx).putBuf(ptr, size);
			_this(ctx)->ptr += written;
			return written;
		}

		static int rangeSeek(struct gdIOCtx* ctx, const int offset)
		{
			if ((size_t)offset > _this(ctx)->size || offset < 0)
				return 0;

			_this(ctx)->ptr = offset;
			return _inner(ctx).seek(offset + _this(ctx)->offset);
		}
		static long rangeTell(struct gdIOCtx* ctx)
		{
			return _this(ctx)->ptr;
		}
		static void gdFreeRangeCtx(gdIOCtx* ctx)
		{
			delete _this(ctx);
		}

		void vtable()
		{
			getC = rangeGetchar;
			putC = rangePutchar;

			getBuf = rangeGetbuf;
			putBuf = rangePutbuf;

			tell = rangeTell;
			seek = rangeSeek;

			gd_free = gdFreeRangeCtx;
		}

		RangeContext(gdIOCtx * inner, size_t offset, size_t size)
			: inner(inner)
			, offset(offset)
			, size(size)
			, ptr(0)
		{
			vtable();
		}
	};
}

BGDEX_DECLARE(gdIOCtx *) gdNewRangeCtx(gdIOCtx * inner, size_t offset, size_t size)
{
	if (!inner)
		return nullptr;

	gd::IOHandle ret{ new (std::nothrow) gd::RangeContext(inner, offset, size) };
	if (!ret)
		return nullptr;

	if (!ret.seek(0))
	{
		ret.gd_free();
		return nullptr;
	}

	return ret.get();
}