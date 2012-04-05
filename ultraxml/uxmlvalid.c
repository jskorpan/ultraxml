//=============================================================================
//= Tables and lookups ========================================================
//=============================================================================
static const UTF8 g_utf8LengthLookup[256] = 
{
/* 0x00 */ 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 
/* 0x10 */ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
/* 0x20 */ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 
/* 0x30 */ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
/* 0x40 */ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 
/* 0x50 */ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
/* 0x60 */ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 
/* 0x70 */ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
/* 0x80 */ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 
/* 0x90 */ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
/* 0xa0 */ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 
/* 0xb0 */ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
/* 0xc0 */ 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 
/* 0xd0 */ 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
/* 0xe0 */ 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 
/* 0xf0 */ 4, 4, 4, 4, 4, 4, 4, 4, 5, 5, 5, 5, 6, 6, 1, 1
};

/*
UCS32 uuReadNextChar(UUStr *str, ssize_t *byteOffset)
{

}
*/




UTF32 readNextChar(struct XMLPARSER *parser)
{
	UTF32 ucs;
  UTF8 *ptr = (UTF8 *) parser->offset;
  UTF8 *end = (UTF8 *) parser->end;
	UTF8 len = g_utf8LengthLookup[*ptr];

	switch (len)
	{
	case 1:
		ucs = *ptr;
    parser->offset ++;
		break;

	case 2:
	{
		UTF16 in;

    if (ptr + 1 > end)
		{
		  setError(parser, 0, "Incomplete UTF8 sequence detected");
      return 0;
		}

		in = *((UTF16 *) ptr);

		#ifdef __LITTLE_ENDIAN__
		ucs = ((in & 0x1f) << 6) | ((in >> 8) & 0x3f);
		#else
		ucs = ((in & 0x1f00) >> 2) | (in & 0x3f);
		#endif

		if (ucs < 0x80)
		{
		  setError(parser, 0, "Overlong UTF8 sequence detected");
      return 0;
		}

		ptr += 2;
    parser->offset = (XMLCHAR *) ptr;
		break;
	}

	case 3:
	{
		UTF32 in;
		if (ptr + 2 > end)
		{
		  setError(parser, 0, "Incomplete UTF8 sequence detected");
      return 0;
		}

		#ifdef __LITTLE_ENDIAN__
		in = *((UTF16 *) ptr);
		in |= *((UTF8 *) ptr + 2) << 16;
		ucs = ((in & 0x0f) << 12) | ((in & 0x3f00) >> 2) | ((in & 0x3f0000) >> 16);
		#else
		in = *((UTF16 *) ptr) << 8;
		in |= *((UTF8 *) ptr + 2);
		ucs = ((in & 0x0f0000) >> 4) | ((in & 0x3f00) >> 2) | (in & 0x3f);
		#endif

		if (ucs < 0x800)
		{
		  setError(parser, 0, "Overlong UTF8 sequence detected");
      return 0;
		}

		ptr += 3;
    parser->offset = (XMLCHAR *) ptr;
		break;
	}

	case 4:
	{
		UTF32 in;

		if (ptr + 3 > end)
		{
		  setError(parser, 0, "Incomplete UTF8 sequence detected");
      return 0;
		}

		#ifdef __LITTLE_ENDIAN__
		in = *((UCS32 *) ptr);
		ucs = ((in & 0x07) << 18) | ((in & 0x3f00) << 4) | ((in & 0x3f0000) >> 10) | ((in & 0x3f000000) >> 24);
		#else
		in = *((UTF32 *) ptr);
		ucs = ((in & 0x07000000) >> 6) | ((in & 0x3f0000) >> 4) | ((in & 0x3f00) >> 2) | (in & 0x3f);
		#endif
		if (ucs < 0x10000)
		{
		  setError(parser, 0, "Overlong UTF8 sequence detected");
      return 0;
		}

		ptr += 4;
    parser->offset = (XMLCHAR *) ptr;
		break;
	}

	case 0:
		return 0;

	case 5:
	case 6:
	default:
		setError(parser, 0, "Invalid UTF8 sequence length detected");
    return 0;
	}

	return ucs;
}

int validateNameStartChar(UTF32 chr)
{
  if (chr < 0xf8)
  {
    switch (chr)
    {
    case ':':
    case '_':
    case 'A': case 'B': case 'C': case 'D': case 'E':
    case 'F': case 'G': case 'H': case 'I': case 'J':
    case 'K': case 'L': case 'M': case 'N': case 'O':
    case 'P': case 'Q': case 'R': case 'S': case 'T':
    case 'U': case 'V': case 'W': case 'X': case 'Y': 
    case 'Z':
    case 'a': case 'b': case 'c': case 'd': case 'e':
    case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o':
    case 'p': case 'q': case 'r': case 's': case 't':
    case 'u': case 'v': case 'w': case 'x': case 'y': 
    case 'z':
    case 0xc0: case 0xc1: case 0xc2: case 0xc3:
    case 0xc4: case 0xc5: case 0xc6: case 0xc7:
    case 0xc8: case 0xc9: case 0xca: case 0xcb:
    case 0xcc: case 0xcd: case 0xce: case 0xcf:
    case 0xd0: case 0xd1: case 0xd2: case 0xd3:
    case 0xd4: case 0xd5: case 0xd6:

    case 0xd8: case 0xd9: case 0xda: case 0xdb:
    case 0xdc: case 0xdd: case 0xde: case 0xdf:
    case 0xe0: case 0xe1: case 0xe2: case 0xe3:
    case 0xe4: case 0xe5: case 0xe6: case 0xe7:
    case 0xe8: case 0xe9: case 0xea: case 0xeb:
    case 0xec: case 0xed: case 0xee: case 0xef:
    case 0xf0: case 0xf1: case 0xf2: case 0xf3:
    case 0xf4: case 0xf5: case 0xf6:
      return 1;

    default:
      return 0;

    }
  } 
  else if (chr <= 0x2ff)
    return 1;
  else if (chr >= 0x370 && chr <= 0x37d)
    return 1;
  else if (chr >= 0x37f && chr < 0x1fff)
    return 1;
  else if (chr >= 0x200c && chr < 0x200d)
    return 1;
  else if (chr >= 0x2070 && chr < 0x218f)
    return 1;
  else if (chr >= 0x2c00 && chr < 0x2fef)
    return 1;
  else if (chr >= 0x3001 && chr < 0xd7ff)
    return 1;
  else if (chr >= 0xf900 && chr < 0xfdcf)
    return 1;
  else if (chr >= 0xfdf0 && chr < 0xfffd)
    return 1;
  else if (chr >= 0x10000 && chr < 0xeffff)
    return 1;
  else
    return 0;
}

int validateNameChar(UTF32 chr)
{
  if (chr < 0xf8)
  {
    switch (chr)
    {
    case '0': case '1': case '2': case '3': case '4': 
    case '5': case '6': case '7': case '8': case '9': 
    case '.':
    case ':':
    case '_':
    case 'A': case 'B': case 'C': case 'D': case 'E':
    case 'F': case 'G': case 'H': case 'I': case 'J':
    case 'K': case 'L': case 'M': case 'N': case 'O':
    case 'P': case 'Q': case 'R': case 'S': case 'T':
    case 'U': case 'V': case 'W': case 'X': case 'Y': 
    case 'Z':
    case 'a': case 'b': case 'c': case 'd': case 'e':
    case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o':
    case 'p': case 'q': case 'r': case 's': case 't':
    case 'u': case 'v': case 'w': case 'x': case 'y': 
    case 'z':
    case 0xb7:
    case 0xc0: case 0xc1: case 0xc2: case 0xc3:
    case 0xc4: case 0xc5: case 0xc6: case 0xc7:
    case 0xc8: case 0xc9: case 0xca: case 0xcb:
    case 0xcc: case 0xcd: case 0xce: case 0xcf:
    case 0xd0: case 0xd1: case 0xd2: case 0xd3:
    case 0xd4: case 0xd5: case 0xd6:
    case 0xd8: case 0xd9: case 0xda: case 0xdb:
    case 0xdc: case 0xdd: case 0xde: case 0xdf:
    case 0xe0: case 0xe1: case 0xe2: case 0xe3:
    case 0xe4: case 0xe5: case 0xe6: case 0xe7:
    case 0xe8: case 0xe9: case 0xea: case 0xeb:
    case 0xec: case 0xed: case 0xee: case 0xef:
    case 0xf0: case 0xf1: case 0xf2: case 0xf3:
    case 0xf4: case 0xf5: case 0xf6:
      return 1;

    default:
      return 0;

    }
  } 
  else if (chr <= 0x36f)
    return 1;
  else if (chr >= 0x370 && chr <= 0x37d)
    return 1;
  else if (chr >= 0x37f && chr < 0x1fff)
    return 1;
  else if (chr >= 0x200c && chr < 0x200d)
    return 1;
  else if (chr >= 0x2070 && chr < 0x218f)
    return 1;
  else if (chr >= 0x2c00 && chr < 0x2fef)
    return 1;
  else if (chr >= 0x3001 && chr < 0xd7ff)
    return 1;
  else if (chr >= 0xf900 && chr < 0xfdcf)
    return 1;
  else if (chr >= 0xfdf0 && chr < 0xfffd)
    return 1;
  else if (chr >= 0x10000 && chr < 0xeffff)
    return 1;
  else
    return 0;
}

