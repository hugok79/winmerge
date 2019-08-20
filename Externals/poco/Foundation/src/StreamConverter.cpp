//
// StreamConverter.cpp
//
// $Id: //poco/1.4/Foundation/src/StreamConverter.cpp#1 $
//
// Library: Foundation
// Package: Text
// Module:  StreamConverter
//
// Copyright (c) 2004-2006, Applied Informatics Software Engineering GmbH.
// and Contributors.
//
// Permission is hereby granted, free of charge, to any person or organization
// obtaining a copy of the software and accompanying documentation covered by
// this license (the "Software") to use, reproduce, display, distribute,
// execute, and transmit the Software, and to prepare derivative works of the
// Software, and to permit third-parties to whom the Software is furnished to
// do so, all subject to the following:
// 
// The copyright notices in the Software and this entire statement, including
// the above license grant, this restriction and the following disclaimer,
// must be included in all copies of the Software, in whole or in part, and
// all derivative works of the Software, unless such copies or derivative
// works are solely in the form of machine-executable object code generated by
// a source language processor.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT. IN NO EVENT
// SHALL THE COPYRIGHT HOLDERS OR ANYONE DISTRIBUTING THE SOFTWARE BE LIABLE
// FOR ANY DAMAGES OR OTHER LIABILITY, WHETHER IN CONTRACT, TORT OR OTHERWISE,
// ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.
//


#include "Poco/StreamConverter.h"
#include "Poco/TextEncoding.h"


namespace Poco {


StreamConverterBuf::StreamConverterBuf(std::istream& istr, const TextEncoding& inEncoding, const TextEncoding& outEncoding, int defaultChar):
	_pIstr(&istr),
	_pOstr(0),
	_inEncoding(inEncoding),
	_outEncoding(outEncoding),
	_defaultChar(defaultChar),
	_sequenceLength(0),
	_pos(0),
	_errors(0)
{
}


StreamConverterBuf::StreamConverterBuf(std::ostream& ostr, const TextEncoding& inEncoding, const TextEncoding& outEncoding, int defaultChar):
	_pIstr(0),
	_pOstr(&ostr),
	_inEncoding(inEncoding),
	_outEncoding(outEncoding),
	_defaultChar(defaultChar),
	_sequenceLength(0),
	_pos(0),
	_errors(0)
{
}


StreamConverterBuf::~StreamConverterBuf()
{
}


int StreamConverterBuf::readFromDevice()
{
	poco_assert_dbg (_pIstr);

	if (_pos < _sequenceLength) return _buffer[_pos++];

	_pos = 0;
	_sequenceLength = 0;
	int c = _pIstr->get();
	if (c == -1) return -1;	

	poco_assert (c < 256);
	int uc;
	_buffer [0] = (unsigned char) c;
	int n = _inEncoding.queryConvert(_buffer, 1);
	int read = 1;

	while (-1 > n)
	{
		poco_assert_dbg(-n <= sizeof(_buffer));
		_pIstr->read((char*) _buffer + read, -n - read);
		read = -n;
		n = _inEncoding.queryConvert(_buffer, -n);
	}

	if (-1 >= n)
	{
		uc = _defaultChar;
		++_errors;
	}
	else
	{
		uc = n;
	}

	_sequenceLength = _outEncoding.convert(uc, _buffer, sizeof(_buffer));
	if (_sequenceLength == 0)
		_sequenceLength = _outEncoding.convert(_defaultChar, _buffer, sizeof(_buffer));
	if (_sequenceLength == 0)
		return -1;
	else
		return _buffer[_pos++];
}


int StreamConverterBuf::writeToDevice(char c)
{
	poco_assert_dbg (_pOstr);

	_buffer[_pos++] = (unsigned char) c;
	if (_sequenceLength == 0 || _sequenceLength == _pos)
	{
		int n = _inEncoding.queryConvert(_buffer, _pos);
		if (-1 <= n)
		{
			int uc = n;
			if (-1 == n)
			{
				++_errors;
				return -1;
			}
			int n1 = _outEncoding.convert(uc, _buffer, sizeof(_buffer));
			if (n1 == 0) n1 = _outEncoding.convert(_defaultChar, _buffer, sizeof(_buffer));
			poco_assert_dbg (n1 <= sizeof(_buffer));
			_pOstr->write((char*) _buffer, n1);
			_sequenceLength = 0;
			_pos = 0;
		}
		else
		{
			_sequenceLength = -n;
		}
	}

	return charToInt(c);
}


int StreamConverterBuf::errors() const
{
	return _errors;
}


StreamConverterIOS::StreamConverterIOS(std::istream& istr, const TextEncoding& inEncoding, const TextEncoding& outEncoding, int defaultChar): 
	_buf(istr, inEncoding, outEncoding, defaultChar)
{
	poco_ios_init(&_buf);
}


StreamConverterIOS::StreamConverterIOS(std::ostream& ostr, const TextEncoding& inEncoding, const TextEncoding& outEncoding, int defaultChar): 
	_buf(ostr, inEncoding, outEncoding, defaultChar)
{
	poco_ios_init(&_buf);
}


StreamConverterIOS::~StreamConverterIOS()
{
}


StreamConverterBuf* StreamConverterIOS::rdbuf()
{
	return &_buf;
}


int StreamConverterIOS::errors() const
{
	return _buf.errors();
}


InputStreamConverter::InputStreamConverter(std::istream& istr, const TextEncoding& inEncoding, const TextEncoding& outEncoding, int defaultChar): 
	StreamConverterIOS(istr, inEncoding, outEncoding, defaultChar),
	std::istream(&_buf)
{
}


InputStreamConverter::~InputStreamConverter()
{
}


OutputStreamConverter::OutputStreamConverter(std::ostream& ostr, const TextEncoding& inEncoding, const TextEncoding& outEncoding, int defaultChar): 
	StreamConverterIOS(ostr, inEncoding, outEncoding, defaultChar),
	std::ostream(&_buf)
{
}


OutputStreamConverter::~OutputStreamConverter()
{
}


} // namespace Poco