/**
 * author: Triones
 * date  : 2014-08-26
 */

#include <stdio.h>
#include <string.h>
#ifndef _UNIX
#include <malloc.h>
#else
#include <stdlib.h>
#endif

#include "base64.h"

namespace triones
{

/* {{{ */
static const char base64_table[] = { 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L',
        'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', 'a', 'b', 'c', 'd',
        'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v',
        'w', 'x', 'y', 'z', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '+', '/', '\0' };

static const char base64_pad = '=';

static const short base64_reverse_table[256] = { -2, -2, -2, -2, -2, -2, -2, -2, -2, -1, -1, -2, -2,
        -1, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -1, -2, -2, -2,
        -2, -2, -2, -2, -2, -2, -2, 62, -2, -2, -2, 63, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, -2,
        -2, -2, -2, -2, -2, -2, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18,
        19, 20, 21, 22, 23, 24, 25, -2, -2, -2, -2, -2, -2, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35,
        36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, -2, -2, -2, -2, -2, -2, -2,
        -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2,
        -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2,
        -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2,
        -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2,
        -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2,
        -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2 };
/* }}} */

/* {{{ base64_encode */
static unsigned char *base64_encode(const unsigned char *str, size_t length, size_t *ret_length)
{
	const unsigned char *current = str;
	unsigned char *p;
	unsigned char *result;

	if (((length + 2) / 3) >= (1 << (sizeof(int) * 8 - 2)))
	{
		if (ret_length != NULL)
		{
			*ret_length = 0;
		}
		return NULL;
	}

	result = (unsigned char *) malloc((((length + 2) / 3) * 4 + 4) * sizeof(char));
	p = result;

	while (length > 2)
	{ /* keep going until we have less than 24 bits */
		*p++ = base64_table[current[0] >> 2];
		*p++ = base64_table[((current[0] & 0x03) << 4) + (current[1] >> 4)];
		*p++ = base64_table[((current[1] & 0x0f) << 2) + (current[2] >> 6)];
		*p++ = base64_table[current[2] & 0x3f];

		current += 3;
		length -= 3; /* we just handle 3 octets of data */
	}

	/* now deal with the tail end of things */
	if (length != 0)
	{
		*p++ = base64_table[current[0] >> 2];
		if (length > 1)
		{
			*p++ = base64_table[((current[0] & 0x03) << 4) + (current[1] >> 4)];
			*p++ = base64_table[(current[1] & 0x0f) << 2];
			*p++ = base64_pad;
		}
		else
		{
			*p++ = base64_table[(current[0] & 0x03) << 4];
			*p++ = base64_pad;
			*p++ = base64_pad;
		}
	}
	if (ret_length != NULL)
	{
		*ret_length = (int) (p - result);
	}
	*p = '\0';
	return result;
}
/* }}} */

/* {{{ base64_decode */
/* as above, but backwards. :) */
static unsigned char *base64_decode(const unsigned char *str, size_t length, size_t *ret_length)
{
	const unsigned char *current = str;
	int ch, i = 0, j = 0, k;
	/* this sucks for threaded environments */
	unsigned char *result;

	result = (unsigned char *) malloc(length + 1);

	/* run through the whole string, converting as we go */
	while ((ch = *current++) != '\0' && length-- > 0)
	{
		if (ch == base64_pad)
		{
			if (*current != '=' && (i % 4) == 1)
			{
				free(result);
				return NULL;
			}
			continue;
		}

		ch = base64_reverse_table[ch];
		if (ch < 0 || ch == -1)
		{ /* a space or some other separator character, we simply skip over */
			continue;
		}
		else if (ch == -2)
		{
			free(result);
			return NULL;
		}

		switch (i % 4)
		{
			case 0:
				result[j] = ch << 2;
				break;
			case 1:
				result[j++] |= ch >> 4;
				result[j] = (ch & 0x0f) << 4;
				break;
			case 2:
				result[j++] |= ch >> 2;
				result[j] = (ch & 0x03) << 6;
				break;
			case 3:
				result[j++] |= ch;
				break;
		}
		i++;
	}

	k = j;
	/* mop things up if we ended on a boundary */
	if (ch == base64_pad)
	{
		switch (i % 4)
		{
			case 1:
				free(result);
				return NULL;
			case 2:
				k++;
			case 3:
				result[k++] = 0;
		}
	}
	if (ret_length)
	{
		*ret_length = j;
	}
	result[j] = '\0';
	return result;
}
/* }}} */

static void base64_free(void *ptr)
{
	if (ptr == NULL)
	{
		return;
	}
	free(ptr);
}

////////////////////////////// Base64 /////////////////////////////

Base64::Base64()
{
	_buf = NULL;
	_len = 0;
}

Base64::~Base64()
{
	if (_buf != NULL)
	{
		base64_free(_buf);
		_buf = NULL;
	}
}

// Base64编码
bool Base64::encode(const char *data, const size_t len)
{
	if (_buf != NULL) base64_free(_buf);

	_buf = (char *) base64_encode((const unsigned char *) data, len, &_len);

	if (_buf == NULL || _len == 0)
	{
		return false;
	}

	return true;
}

// Base64解码
bool Base64::decode(const char *data, const size_t len)
{
	if (_buf != NULL) base64_free(_buf);

	_buf = (char *) base64_decode((const unsigned char *) data, len, &_len);

	if (_buf == NULL || _len == 0)
	{
		return false;
	}

	return true;
}

char * Base64::data()
{
	return _buf;
}

size_t Base64::size()
{
	return _len;
}

} // namespace triones
