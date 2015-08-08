/*
This program is distributed under the terms of the 'MIT license'. The text
of this licence follows...

Copyright (c) 2004 J.D.Medhurst (a.k.a. Tixy)

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/

//Implementation of ITU-T (formerly CCITT) Recomendation %G711
//g711 Audio Codec - ITU-T Recomendation G711
//This encodes and decodes uniform PCM values to/from 8 bit A-law and u-Law values.
//Note, the methods in this class use uniform PCM values which are of 16 bits precision,
//these are 'left justified' values corresponding to the 13 and 14 bit values described
//in G711.

#ifndef G711_H
#define G711_H

#include "stdint.h"

class G711
{
public:
	//Encode a buffer of 16 bit uniform PCM values into A-Law values
	//param dst	   Pointer to location to store A-Law encoded values
	//param src	   Pointer to the buffer of 16 bit uniform PCM values to be encoded
	//param srcSize The size, in bytes, of the buffer at \a src
	//return 	   The number of bytes which were stored at dst (equal to srcSize>>1)
	unsigned int ALawEncode(uint8_t* dst, int16_t* src, int srcSize);

	//Decode a buffer of A-Law values into 16 bit uniform PCM values
	//param dst	   Pointer to location to store decoded 16 bit uniform PCM values
	//param src	   Pointer to the buffer of A-Law values to be decoded
	//param srcSize The size, in bytes, of the buffer at \a src
	//return 	   The number of bytes which were stored at \a dst (equal to srcSize<<1)
	unsigned int ALawDecode(int16_t* dst, const uint8_t* src, int srcSize);

	//Encode a buffer of 16 bit uniform PCM values into u-Law values
	//dst	   Pointer to location to store u-Law encoded values
	//src	   Pointer to the buffer of 16 bit uniform PCM values to be encoded
	//srcSize The size, in bytes, of the buffer at \a src
	//return 	   The number of bytes which were stored at \a dst (equal to srcSize>>1)
	unsigned int ULawEncode(uint8_t* dst, int16_t* src, int srcSize);

	//Decode a buffer of u-Law values into 16 bit uniform PCM values
	//dst	   Pointer to location to store decoded 16 bit uniform PCM values
	//src	   Pointer to the buffer of u-Law values to be decoded
	//srcSize The size, in bytes, of the buffer at \a src
	//return 	   The number of bytes which were stored at \a dst (equal to srcSize<<1)
	unsigned int ULawDecode(int16_t* dst, const uint8_t* src, int srcSize);

private:
	//Encode a single 16 bit uniform PCM value into an A-Law value
	//pcm16 A 16 bit uniform PCM value
	//returns The A-Law encoded value corresponding to pcm16
	uint8_t ALawEncode(int16_t pcm16);

	//	Decode a single A-Law value into a 16 bit uniform PCM value
	// alaw An A-Law encoded value
	//return 	The 16 bit uniform PCM value corresponding to alaw
	int ALawDecode(uint8_t alaw);

	//Encode single 16 bit uniform PCM value into an u-Law value
	//pcm16 A 16 bit uniform PCM value
	//return 	 The u-Law encoded value corresponding to pcm16
	uint8_t ULawEncode(int16_t pcm16);

	//Decode a single u-Law value into a 16 bit uniform PCM value
	//param ulaw An u-Law encoded value
	//return 	The 16 bit uniform PCM value corresponding to ulaw
	int ULawDecode(uint8_t ulaw);
};

#endif

