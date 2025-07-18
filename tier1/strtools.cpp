//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: String Tools
//
//===========================================================================//
// These are redefined in the project settings to prevent anyone from using them.
// We in this module are of a higher caste and thus are privileged in their use.
#ifdef strncpy
	#undef strncpy
#endif

#ifdef _snprintf
	#undef _snprintf
#endif

#if defined( sprintf )
	#undef sprintf
#endif

#if defined( vsprintf )
	#undef vsprintf
#endif

#ifdef _vsnprintf
#ifdef _WIN32
	#undef _vsnprintf
#endif
#endif

#ifdef vsnprintf
#ifndef _WIN32
	#undef vsnprintf
#endif
#endif

#if defined( strcat )
	#undef strcat
#endif

#ifdef strncat
	#undef strncat
#endif

#include "tier1/strtools.h"

// NOTE: I have to include stdio + stdarg first so vsnprintf gets compiled in
#include <cstdio>
#include <cstdarg>
#include <cstring>
#include <cstdlib>
#include <ctime>
#include <cinttypes>

#ifdef POSIX
#include <iconv.h>
#include <ctype.h>
#include <unistd.h>
#define _getcwd getcwd
#elif _WIN32
#include <direct.h>
#include "winlite.h"
#endif

#include "tier0/dbg.h"
#include "tier0/basetypes.h"
#include "tier1/utldict.h"
#include "tier1/utlbuffer.h"
#include "tier1/utlstring.h"
#include "tier1/fmtstr.h"

#include "tier0/memdbgon.h"

static int FastToLower( char c )
{
	int i = (unsigned char) c;
	if ( i < 0x80 )
	{
		// Brutally fast branchless ASCII tolower():
		i += (((('A'-1) - i) & (i - ('Z'+1))) >> 26) & 0x20;
	}
	else
	{
		i += isupper( i ) ? 0x20 : 0;
	}
	return i;
}

void _V_memset (const char*, int, void *dest, int fill, intp count)
{
	Assert( count >= 0 );

	memset(dest,fill,count);
}

void _V_memcpy (const char*, int, void *dest, const void *src, intp count)
{
	Assert( count >= 0 );

	memcpy( dest, src, count );
}

void _V_memmove(const char*, int, void *dest, const void *src, intp count)
{
	Assert( count >= 0 );

	memmove( dest, src, count );
}

int _V_memcmp (const char*, int, const void *m1, const void *m2, intp count)
{
	Assert( count >= 0 );

	return memcmp( m1, m2, count );
}

intp _V_strlen(const char*, int, IN_Z const char *str)
{
	return strlen( str );
}

void _V_strcpy (const char*, int, IN_Z char *dest, IN_Z const char *src)
{
	AssertValidWritePtr(dest);
	AssertValidStringPtr(src);

	strcpy( dest, src );
}

intp _V_wcslen (const char*, int, IN_Z const wchar_t *pwch)
{
	return wcslen( pwch );
}

RET_MAY_BE_NULL char *_V_strrchr (const char*, int, IN_Z const char *s, char c)
{
	AssertValidStringPtr( s );
    intp len = V_strlen(s);
    s += len;
    while (len--)
	if (*--s == c) return (char *)s;
    return 0;
}

int _V_strcmp (const char*, int, IN_Z const char *s1, IN_Z const char *s2)
{
	AssertValidStringPtr( s1 );
	AssertValidStringPtr( s2 );

	return strcmp( s1, s2 );
}

int _V_wcscmp (const char*, int, IN_Z const wchar_t *s1, IN_Z const wchar_t *s2)
{
	AssertValidReadPtr( s1 );
	AssertValidReadPtr( s2 );

	while ( *s1 == *s2 )
	{
		if ( !*s1 )
			return 0;			// strings are equal

		s1++;
		s2++;
	}

	return *s1 > *s2 ? 1 : -1;	// strings not equal
}


RET_MAY_BE_NULL char *_V_strstr(const char*, int, IN_Z const char *s1, IN_Z const char *search )
{
	AssertValidStringPtr( s1 );
	AssertValidStringPtr( search );

	return (char *)strstr( s1, search );
}

wchar_t *_V_wcsupr (const char*, int, INOUT_Z wchar_t *start)
{
	return _wcsupr( start );
}


wchar_t *_V_wcslower (const char*, int, INOUT_Z wchar_t *start)
{
	return _wcslwr(start);
}



char *V_strupr( INOUT_Z char *start )
{
	unsigned char *str = (unsigned char*)start;
	while( *str )
	{
		if ( (unsigned char)(*str - 'a') <= ('z' - 'a') )
			*str -= 'a' - 'A';
		else if ( (unsigned char)*str >= 0x80 ) // non-ascii, fall back to CRT
			*str = static_cast<unsigned char>(toupper( *str ));
		str++;
	}
	return start;
}

char *V_strlower( INOUT_Z char *start )
{
	unsigned char *str = (unsigned char*)start;
	while( *str )
	{
		if ( (unsigned char)(*str - 'A') <= ('Z' - 'A') )
			*str += 'a' - 'A';
		else if ( (unsigned char)*str >= 0x80 ) // non-ascii, fall back to CRT
			*str = static_cast<unsigned char>(tolower( *str ));
		str++;
	}
	return start;
}

char *V_strnlwr( INOUT_Z_CAP(count) char *s, size_t count )
{
	// Assert( count >= 0 ); tautology since size_t is unsigned
	AssertValidStringPtr( s, count );

	unsigned char *it = reinterpret_cast<unsigned char *>(s);
	char* pRet = s;
	if ( !s || !count )
		return s;

	while ( -- count > 0 )
	{
		if ( !*it )
			return pRet; // reached end of string

		*it = static_cast<unsigned char>(tolower( static_cast<unsigned char>(*it) ));
		++it;
	}

	*it = 0; // null-terminate original string at "count-1"
	return pRet;
}

int V_stricmp( IN_Z const char *str1, IN_Z const char *str2 )
{
	// It is not uncommon to compare a string to itself. See
	// VPanelWrapper::GetPanel which does this a lot. Since stricmp
	// is expensive and pointer comparison is cheap, this simple test
	// can save a lot of cycles, and cache pollution.
	if ( str1 == str2 )
	{
		return 0;
	}
	const unsigned char *s1 = (const unsigned char*)str1;
	const unsigned char *s2 = (const unsigned char*)str2;
	for ( ; *s1; ++s1, ++s2 )
	{
		if ( *s1 != *s2 )
		{
			// in ascii char set, lowercase = uppercase | 0x20
			unsigned char c1 = *s1 | 0x20;
			unsigned char c2 = *s2 | 0x20;
			if ( c1 != c2 || (unsigned char)(c1 - 'a') > ('z' - 'a') )
			{
				// if non-ascii mismatch, fall back to CRT for locale
				if ( (c1 | c2) >= 0x80 ) return stricmp( (const char*)s1, (const char*)s2 );
				// ascii mismatch. only use the | 0x20 value if alphabetic.
				if ((unsigned char)(c1 - 'a') > ('z' - 'a')) c1 = *s1;
				if ((unsigned char)(c2 - 'a') > ('z' - 'a')) c2 = *s2;
				return c1 > c2 ? 1 : -1;
			}
		}
	}
	return *s2 ? -1 : 0;
}

int V_strnicmp( IN_Z const char *str1, IN_Z const char *str2, intp n )
{
	const unsigned char *s1 = (const unsigned char*)str1;
	const unsigned char *s2 = (const unsigned char*)str2;
	for ( ; n > 0 && *s1; --n, ++s1, ++s2 )
	{
		if ( *s1 != *s2 )
		{
			// in ascii char set, lowercase = uppercase | 0x20
			unsigned char c1 = *s1 | 0x20;
			unsigned char c2 = *s2 | 0x20;
			if ( c1 != c2 || (unsigned char)(c1 - 'a') > ('z' - 'a') )
			{
				// if non-ascii mismatch, fall back to CRT for locale
				if ( (c1 | c2) >= 0x80 ) return strnicmp( (const char*)s1, (const char*)s2, n );
				// ascii mismatch. only use the | 0x20 value if alphabetic.
				if ((unsigned char)(c1 - 'a') > ('z' - 'a')) c1 = *s1;
				if ((unsigned char)(c2 - 'a') > ('z' - 'a')) c2 = *s2;
				return c1 > c2 ? 1 : -1;
			}
		}
	}
	return (n > 0 && *s2) ? -1 : 0;
}

int V_strncmp( IN_Z const char *s1, IN_Z const char *s2, intp count )
{
	Assert( count >= 0 );
	AssertValidStringPtr( s1, count );
	AssertValidStringPtr( s2, count );

	while ( count > 0 )
	{
		if ( *s1 != *s2 )
			return (unsigned char)*s1 < (unsigned char)*s2 ? -1 : 1; // string different
		if ( *s1 == '\0' )
			return 0; // null terminator hit - strings the same
		s1++;
		s2++;
		count--;
	}

	return 0; // count characters compared the same
}


RET_MAY_BE_NULL const char *StringAfterPrefix( IN_Z const char *str, IN_Z const char *prefix )
{
	AssertValidStringPtr( str );
	AssertValidStringPtr( prefix );
	do
	{
		if ( !*prefix )
			return str;
	}
	while ( FastToLower( *str++ ) == FastToLower( *prefix++ ) );
	return NULL;
}

RET_MAY_BE_NULL const char *StringAfterPrefixCaseSensitive( IN_Z const char *str, IN_Z const char *prefix )
{
	AssertValidStringPtr( str );
	AssertValidStringPtr( prefix );
	do
	{
		if ( !*prefix )
			return str;
	}
	while ( *str++ == *prefix++ );
	return NULL;
}


int64 V_atoi64( IN_Z const char *str )
{
	AssertValidStringPtr( str );

	int64             val;
	int64             sign;
	int64             c;
	
	Assert( str );
	if (*str == '-')
	{
		sign = -1;
		str++;
	}
	else if (*str == '+')
	{
		sign = 1;
		str++;
	}
	else
	{
		sign = 1;
	}
		
	val = 0;

//
// check for hex
//
	if (str[0] == '0' && (str[1] == 'x' || str[1] == 'X') )
	{
		str += 2;
		while (1)
		{
			c = *str++;
			if (c >= '0' && c <= '9')
				val = (val<<4) + c - '0';
			else if (c >= 'a' && c <= 'f')
				val = (val<<4) + c - 'a' + 10;
			else if (c >= 'A' && c <= 'F')
				val = (val<<4) + c - 'A' + 10;
			else
				return val*sign;
		}
	}
	
//
// check for character
//
	if (str[0] == '\'')
	{
		return sign * str[1];
	}
	
//
// assume decimal
//
	while (1)
	{
		c = *str++;
		if (c <'0' || c > '9')
			return val*sign;
		val = val*10 + c - '0';
	}
	
	return 0;
}

uint64 V_atoui64( IN_Z const char *str )
{
	AssertValidStringPtr( str );

	uint64             val;
	uint64             c;

	Assert( str );

	val = 0;

	//
	// check for hex
	//
	if (str[0] == '0' && (str[1] == 'x' || str[1] == 'X') )
	{
		str += 2;
		while (1)
		{
			c = *str++;
			if (c >= '0' && c <= '9')
				val = (val<<4) + c - '0';
			else if (c >= 'a' && c <= 'f')
				val = (val<<4) + c - 'a' + 10;
			else if (c >= 'A' && c <= 'F')
				val = (val<<4) + c - 'A' + 10;
			else
				return val;
		}
	}

	//
	// check for character
	//
	if (str[0] == '\'')
	{
		return str[1];
	}

	//
	// assume decimal
	//
	while (1)
	{
		c = *str++;
		if (c <'0' || c > '9')
			return val;
		val = val*10 + c - '0';
	}

	return 0;
}

int V_atoi( IN_Z const char *str )
{ 
	return (int)V_atoi64( str );
}

float V_atof (IN_Z const char *str)
{
	AssertValidStringPtr( str );
	float           val;
	int             sign;
	int             c;
	int             decimal, total;

	if (*str == '-')
	{
		sign = -1;
		str++;
	}
	else if (*str == '+')
	{
		sign = 1;
		str++;
	}
	else
	{
		sign = 1;
	}

	val = 0;

	//
	// check for hex
	//
	if (str[0] == '0' && (str[1] == 'x' || str[1] == 'X') )
	{
		str += 2;
		while (1)
		{
			c = *str++;
			if (c >= '0' && c <= '9')
				val = (val*16) + c - '0';
			else if (c >= 'a' && c <= 'f')
				val = (val*16) + c - 'a' + 10;
			else if (c >= 'A' && c <= 'F')
				val = (val*16) + c - 'A' + 10;
			else
				return val*sign;
		}
	}

	//
	// check for character
	//
	if (str[0] == '\'')
	{
		// dimhotepus: Safe to cast as withing float range.
		return static_cast<float>(sign * str[1]);
	}

	//
	// assume decimal
	//
	decimal = -1;
	total = 0;
	int exponent = 0;
	while (1)
	{
		c = *str++;
		if (c == '.')
		{
			if ( decimal != -1 )
			{
				break;
			}

			decimal = total;
			continue;
		}
		if (c <'0' || c > '9')
		{
			if ( c == 'e' || c == 'E' )
			{
				exponent = V_atoi(str);
			}
			break;
		}
		val = val*10 + c - '0';
		total++;
	}

	if ( exponent != 0 )
	{
		// dimhotepus: Safe to cast as exponent within float range.
		val *= powf( 10.0f, static_cast<float>(exponent) );
	}
	if (decimal == -1)
		return val*sign;
	while (total > decimal)
	{
		val /= 10;
		total--;
	}

	return val*sign;
}

//-----------------------------------------------------------------------------
// Normalizes a float string in place.  
//
// (removes leading zeros, trailing zeros after the decimal point, and the decimal point itself where possible)
//-----------------------------------------------------------------------------
void V_normalizeFloatString( INOUT_Z char* pFloat )
{
	// If we have a decimal point, remove trailing zeroes:
	if( strchr( pFloat,'.' ) )
	{
		size_t len = strlen(pFloat);

		while( len > 1 && pFloat[len - 1] == '0' )
		{
			pFloat[len - 1] = '\0';
			len--;
		}

		if( len > 1 && pFloat[ len - 1 ] == '.' )
		{
			pFloat[len - 1] = '\0';
			len--;
		}
	}

	// TODO: Strip leading zeros

}


//-----------------------------------------------------------------------------
// Finds a string in another string with a case insensitive test
//-----------------------------------------------------------------------------
RET_MAY_BE_NULL char const* V_stristr( IN_Z char const* pStr, IN_Z char const* pSearch )
{
	AssertValidStringPtr(pStr);
	AssertValidStringPtr(pSearch);

	if (!pStr || !pSearch) 
		return 0;

	char const* pLetter = pStr;

	// Check the entire string
	while (*pLetter != 0)
	{
		// Skip over non-matches
		if (FastToLower((unsigned char)*pLetter) == FastToLower((unsigned char)*pSearch))
		{
			// Check for match
			char const* pMatch = pLetter + 1;
			char const* pTest = pSearch + 1;
			while (*pTest != 0)
			{
				// We've run off the end; don't bother.
				if (*pMatch == 0)
					return 0;

				if (FastToLower((unsigned char)*pMatch) != FastToLower((unsigned char)*pTest))
					break;

				++pMatch;
				++pTest;
			}

			// Found a match!
			if (*pTest == 0)
				return pLetter;
		}

		++pLetter;
	}

	return 0;
}

RET_MAY_BE_NULL char* V_stristr( IN_Z char* pStr, IN_Z char const* pSearch )
{
	AssertValidStringPtr( pStr );
	AssertValidStringPtr( pSearch );

	return (char*)V_stristr( (char const*)pStr, pSearch );
}

//-----------------------------------------------------------------------------
// Finds a string in another string with a case insensitive test w/ length validation
//-----------------------------------------------------------------------------

RET_MAY_BE_NULL char const* V_strnistr( IN_Z char const* pStr, IN_Z char const* pSearch, intp n )
{
	AssertValidStringPtr(pStr);
	AssertValidStringPtr(pSearch);

	if (!pStr || !pSearch) 
		return 0;

	char const* pLetter = pStr;

	// Check the entire string
	while (*pLetter != 0)
	{
		if ( n <= 0 )
			return 0;

		// Skip over non-matches
		if (FastToLower(*pLetter) == FastToLower(*pSearch))
		{
			intp n1 = n - 1;

			// Check for match
			char const* pMatch = pLetter + 1;
			char const* pTest = pSearch + 1;
			while (*pTest != 0)
			{
				if ( n1 <= 0 )
					return 0;

				// We've run off the end; don't bother.
				if (*pMatch == 0)
					return 0;

				if (FastToLower(*pMatch) != FastToLower(*pTest))
					break;

				++pMatch;
				++pTest;
				--n1;
			}

			// Found a match!
			if (*pTest == 0)
				return pLetter;
		}

		++pLetter;
		--n;
	}

	return 0;
}

RET_MAY_BE_NULL const char* V_strnchr( IN_Z const char* pStr, char c, intp n )
{
	char const* pLetter = pStr;
	char const* pLast = pStr + n;

	// Check the entire string
	while ( (pLetter < pLast) && (*pLetter != 0) )
	{
		if (*pLetter == c)
			return pLetter;
		++pLetter;
	}
	return NULL;
}

void V_strncpy( OUT_Z_CAP(maxLenInChars) char *pDest, IN_Z char const *pSrc, intp maxLenInChars )
{
	AssertValidWritePtr( pDest, maxLenInChars );
	AssertValidStringPtr( pSrc );

	if ( maxLenInChars > 0 )
	{
		strncpy( pDest, pSrc, maxLenInChars - 1 );
		pDest[maxLenInChars - 1] = '\0';
	}
}

void V_wcsncpy( OUT_Z_BYTECAP(maxLenInBytes) wchar_t *pDest, IN_Z wchar_t const *pSrc, intp maxLenInBytes )
{
	AssertValidWritePtr( pDest, maxLenInBytes );
	AssertValidReadPtr( pSrc );

	intp maxLenInChars = maxLenInBytes / static_cast<intp>(sizeof(wchar_t));
	if ( maxLenInChars > 0 )
	{
		wcsncpy( pDest, pSrc, maxLenInChars - 1 );
		pDest[maxLenInChars - 1] = L'\0';
	}
}



int V_snwprintf( OUT_Z_CAP(maxLenInChars) wchar_t *pDest, intp maxLenInChars, PRINTF_FORMAT_STRING const wchar_t *pFormat, ... )
{
	Assert( maxLenInChars > 0 );
	AssertValidWritePtr( pDest, maxLenInChars );
	AssertValidReadPtr( pFormat );

	va_list marker;

	va_start( marker, pFormat );
#ifdef _WIN32
	int len = _vsnwprintf( pDest, maxLenInChars, pFormat, marker );
#elif POSIX
	int len = vswprintf( pDest, maxLenInChars, pFormat, marker );
#else
#error "Please define vsnwprintf type."
#endif
	va_end( marker );

	// Len > maxLen represents an overflow on POSIX, < 0 is an overflow on windows
	if( len < 0 || len >= maxLenInChars )
	{
		len = (int)maxLenInChars;
		pDest[maxLenInChars-1] = '\0';
	}
	
	return len;
}


int V_vsnwprintf( OUT_Z_CAP(maxLenInChars) wchar_t *pDest, intp maxLenInChars, PRINTF_FORMAT_STRING const wchar_t *pFormat, va_list params )
{
	Assert( maxLenInChars > 0 );

#ifdef _WIN32
	int len = _vsnwprintf( pDest, maxLenInChars, pFormat, params );
#elif POSIX
	int len = vswprintf( pDest, maxLenInChars, pFormat, params );
#else
#error "Please define vsnwprintf type."
#endif

	// Len < 0 represents an overflow
	// Len == maxLen represents exactly fitting with no NULL termination
	// Len >= maxLen represents overflow on POSIX
	if ( len < 0 || len >= maxLenInChars )
	{
		len = (int)maxLenInChars;
		pDest[maxLenInChars-1] = '\0';
	}

	return len;
}


int V_snprintf( OUT_Z_CAP(maxLenInChars) char *pDest, intp maxLenInChars, PRINTF_FORMAT_STRING char const *pFormat, ... )
{
	Assert( maxLenInChars > 0 );
	AssertValidWritePtr( pDest, maxLenInChars );
	AssertValidStringPtr( pFormat );

	va_list marker;

	va_start( marker, pFormat );
	// dimhotepus: vsnprintf always null-terminate.
	int len = vsnprintf( pDest, maxLenInChars, pFormat, marker );
	va_end( marker );

	// Len > maxLen represents an overflow on POSIX, < 0 is an overflow on windows
	if( len < 0 || len >= maxLenInChars )
	{
		len = (int)maxLenInChars;
		pDest[maxLenInChars-1] = '\0';
	}

	return len;
}


int V_vsnprintf( OUT_Z_CAP(maxLenInChars) char *pDest, intp maxLenInChars, PRINTF_FORMAT_STRING char const *pFormat, va_list params )
{
	Assert( maxLenInChars > 0 );
	AssertValidWritePtr( pDest, maxLenInChars );
	AssertValidStringPtr( pFormat );
	
	// dimhotepus: vsnprintf always null-terminate.
	int len = vsnprintf( pDest, maxLenInChars, pFormat, params );

	// Len > maxLen represents an overflow on POSIX, < 0 is an overflow on windows
	if( len < 0 || len >= maxLenInChars )
	{
		len = (int)maxLenInChars;
		pDest[maxLenInChars-1] = '\0';
	}

	return len;
}


int V_vsnprintfRet( OUT_Z_CAP(maxLenInChars) char *pDest, intp maxLenInChars, PRINTF_FORMAT_STRING const char *pFormat, va_list params, bool *pbTruncated )
{
	Assert( maxLenInChars > 0 );
	AssertValidWritePtr( pDest, maxLenInChars );
	AssertValidStringPtr( pFormat );
	
	// dimhotepus: vsnprintf always null-terminate.
	int len = vsnprintf( pDest, maxLenInChars, pFormat, params );

	if ( pbTruncated )
	{
		*pbTruncated = ( len < 0 || len >= maxLenInChars );
	}

	if	( len < 0 || len >= maxLenInChars )
	{
		len = (int)maxLenInChars;
		pDest[maxLenInChars-1] = '\0';
	}

	return len;
}



//-----------------------------------------------------------------------------
// Purpose: If COPY_ALL_CHARACTERS == max_chars_to_copy then we try to add the whole pSrc to the end of pDest, otherwise
//  we copy only as many characters as are specified in max_chars_to_copy (or the # of characters in pSrc if thats's less).
// Input  : *pDest - destination buffer
//			*pSrc - string to append
//			destBufferSize - sizeof the buffer pointed to by pDest
//			max_chars_to_copy - COPY_ALL_CHARACTERS in pSrc or max # to copy
// Output : char * the copied buffer
//-----------------------------------------------------------------------------
char *V_strncat( INOUT_Z_CAP(destBufferSize) char *pDest, IN_Z const char *pSrc, size_t destBufferSize, intp max_chars_to_copy )
{
	AssertValidStringPtr( pDest);
	AssertValidStringPtr( pSrc );

	size_t charstocopy = 0;
	size_t len = strlen( pDest );
	size_t srclen = strlen( pSrc );

	if ( max_chars_to_copy <= COPY_ALL_CHARACTERS )
	{
		charstocopy = srclen;
	}
	else
	{
		charstocopy = (size_t)min( max_chars_to_copy, (intp)srclen );
	}

	if ( len + charstocopy >= destBufferSize )
	{
		charstocopy = destBufferSize - len - 1;
	}

	char *pOut = strncat( pDest, pSrc, charstocopy );
	return pOut;
}

wchar_t *V_wcsncat( INOUT_Z_CAP(cchDest) wchar_t *pDest, IN_Z const wchar_t *pSrc, size_t cchDest, intp max_chars_to_copy )
{
	size_t charstocopy = 0;
	size_t len = wcslen( pDest );
	size_t srclen = wcslen( pSrc );

	if ( max_chars_to_copy <= COPY_ALL_CHARACTERS )
	{
		charstocopy = srclen;
	}
	else
	{
		charstocopy = (size_t)min( max_chars_to_copy, (intp)srclen );
	}

	if ( len + charstocopy >= cchDest )
	{
		charstocopy = cchDest - len - 1;
	}

	wchar_t *pOut = wcsncat( pDest, pSrc, charstocopy );
	return pOut;
}



//-----------------------------------------------------------------------------
// Purpose: Converts value into x.xx MiB/ x.xx KiB, x.xx bytes format, including commas
// Input  : value - 
//			2 - 
//			false - 
// Output : char
//-----------------------------------------------------------------------------
#define NUM_PRETIFYMEM_BUFFERS 8
char *V_pretifymem( double value, int digitsafterdecimal /*= 2*/, bool usebinaryonek /*= false*/ )
{
	static char output[ NUM_PRETIFYMEM_BUFFERS ][ 32 ];
	static intp  current;

	double		onekb = usebinaryonek ? 1024.0 : 1000.0;
	double		onemb = onekb * onekb;
	double		onegb = onemb * onekb;
	double		onetb = onegb * onekb;

	char *out = output[ current ];
	current = ( current + 1 ) & ( NUM_PRETIFYMEM_BUFFERS -1 );

	char suffix[ 8 ];

	// First figure out which bin to use
	if ( value > onetb )
	{
		value /= onetb;
		V_sprintf_safe(suffix, usebinaryonek ? " TiB" : " TB" );
	}
	else if ( value > onegb )
	{
		value /= onegb;
		V_sprintf_safe(suffix, usebinaryonek ? " GiB" : " GB" );
	}
	else if ( value > onemb )
	{
		value /= onemb;
		V_sprintf_safe(suffix, usebinaryonek ? " MiB" : " MB" );
	}
	else if ( value > onekb )
	{
		value /= onekb;
		V_sprintf_safe( suffix, usebinaryonek ? " KiB" : " KB" );
	}
	else
	{
		V_sprintf_safe( suffix, " bytes" );
	}

	char val[ 32 ];

	// Clamp to >= 0
	digitsafterdecimal = max( digitsafterdecimal, 0 );

	// If it's basically integral, don't do any decimals
	if ( abs( value - (intp)value ) < 0.00001 )
	{
		V_sprintf_safe( val, "%i%s", (int)value, suffix );
	}
	else
	{
		char fmt[ 32 ];

		// Otherwise, create a format string for the decimals
		V_sprintf_safe( fmt, "%%.%if%s", digitsafterdecimal, suffix );
		V_sprintf_safe( val, fmt, value );
	}

	// Copy from in to out
	char *i = val;
	char *o = out;

	// Search for decimal or if it was integral, find the space after the raw number
	char *dot = strchr( i, '.' );
	if ( !dot )
	{
		dot = strchr( i, ' ' );
	}

	// Compute position of dot
	intp pos = dot - i;
	// Don't put a comma if it's <= 3 long
	pos -= 3;

	while ( *i )
	{
		// If pos is still valid then insert a comma every third digit, except if we would be
		//  putting one in the first spot
		if ( pos >= 0 && !( pos % 3 ) )
		{
			// Never in first spot
			if ( o != out )
			{
				*o++ = ',';
			}
		}

		// Count down comma position
		pos--;

		// Copy rest of data as normal
		*o++ = *i++;
	}

	// Terminate
	*o = 0;

	return out;
}

//-----------------------------------------------------------------------------
// Purpose: Returns a string representation of an integer with commas
//			separating the 1000s (ie, 37,426,421)
// Input  : value -		Value to convert
// Output : Pointer to a static buffer containing the output
//-----------------------------------------------------------------------------
#define NUM_PRETIFYNUM_BUFFERS 8 // Must be a power of two
char *V_pretifynum( int64 inputValue )
{
	static char output[ NUM_PRETIFYNUM_BUFFERS ][ 32 ];
	static intp  current;

	// Point to the output buffer.
	char * const out = output[ current ];
	// Track the output buffer end for easy calculation of bytes-remaining.
	const char* const outEnd = out + sizeof( output[ current ] );

	// Point to the current output location in the output buffer.
	char *pchRender = out;
	// Move to the next output pointer.
	current = ( current + 1 ) & ( NUM_PRETIFYMEM_BUFFERS -1 );

	*out = 0;

	// In order to handle the most-negative int64 we need to negate it
	// into a uint64.
	uint64 value;
	// Render the leading minus sign, if necessary
	if ( inputValue < 0 )
	{
		V_snprintf( pchRender, ssize(output[0]), "-" );
		value = (uint64)-inputValue;
		// Advance our output pointer.
		pchRender += V_strlen( pchRender );
	}
	else
	{
		value = (uint64)inputValue;
	}

	// Now let's find out how big our number is. The largest number we can fit
	// into 63 bits is about 9.2e18. So, there could potentially be six
	// three-digit groups.

	// We need the initial value of 'divisor' to be big enough to divide our
	// number down to 1-999 range.
	uint64 divisor = 1;
	// Loop more than six times to avoid integer overflow.
	for ( intp i = 0; i < 6; ++i )
	{
		// If our divisor is already big enough then stop.
		if ( value < divisor * 1000 )
			break;

		divisor *= 1000;
	}

	// Print the leading batch of one to three digits.
	uint64 toPrint = value / divisor;
	V_snprintf( pchRender, outEnd - pchRender, "%" PRIu64, toPrint );

	for (;;)
	{
		// Advance our output pointer.
		pchRender += V_strlen( pchRender );
		// Adjust our value to be printed and our divisor.
		value -= toPrint * divisor;
		divisor /= 1000;
		if ( !divisor )
			break;

		// The remaining blocks of digits always include a comma and three digits.
		toPrint = value / divisor;
		V_snprintf( pchRender, outEnd - pchRender, ",%03" PRIu64, toPrint );
	}

	return out;
}


//-----------------------------------------------------------------------------
// Purpose: returns true if a wide character is a "mean" space; that is,
//			if it is technically a space or punctuation, but causes disruptive
//			behavior when used in names, web pages, chat windows, etc.
//
//			characters in this set are removed from the beginning and/or end of strings
//			by Q_AggressiveStripPrecedingAndTrailingWhitespaceW() 
//-----------------------------------------------------------------------------
bool Q_IsMeanSpaceW( wchar_t wch )
{
	bool bIsMean = false;

	switch ( wch )
	{
	case L'\x0082':	  // BREAK PERMITTED HERE
	case L'\x0083':	  // NO BREAK PERMITTED HERE
	case L'\x00A0':	  // NO-BREAK SPACE
	case L'\x034F':   // COMBINING GRAPHEME JOINER
	case L'\x2000':   // EN QUAD
	case L'\x2001':   // EM QUAD
	case L'\x2002':   // EN SPACE
	case L'\x2003':   // EM SPACE
	case L'\x2004':   // THICK SPACE
	case L'\x2005':   // MID SPACE
	case L'\x2006':   // SIX SPACE
	case L'\x2007':   // figure space
	case L'\x2008':   // PUNCTUATION SPACE
	case L'\x2009':   // THIN SPACE
	case L'\x200A':   // HAIR SPACE
	case L'\x200B':   // ZERO-WIDTH SPACE
	case L'\x200C':   // ZERO-WIDTH NON-JOINER
	case L'\x200D':   // ZERO WIDTH JOINER
	case L'\x200E':	  // LEFT-TO-RIGHT MARK
	case L'\x2028':   // LINE SEPARATOR
	case L'\x2029':   // PARAGRAPH SEPARATOR
	case L'\x202F':   // NARROW NO-BREAK SPACE
	case L'\x2060':   // word joiner
	case L'\xFEFF':   // ZERO-WIDTH NO BREAK SPACE
	case L'\xFFFC':   // OBJECT REPLACEMENT CHARACTER
		bIsMean = true;
		break;
	}

	return bIsMean;
}


//-----------------------------------------------------------------------------
// Purpose: strips trailing whitespace; returns pointer inside string just past
// any leading whitespace.
//
// bAggresive = true causes this function to also check for "mean" spaces,
// which we don't want in persona names or chat strings as they're disruptive
// to the user experience.
//-----------------------------------------------------------------------------
static wchar_t *StripWhitespaceWorker( intp cchLength, wchar_t *pwch, bool *pbStrippedWhitespace, bool bAggressive )
{
	// walk backwards from the end of the string, killing any whitespace
	*pbStrippedWhitespace = false;

	wchar_t *pwchEnd = pwch + cchLength;
	while ( --pwchEnd >= pwch )
	{
		if ( !iswspace( *pwchEnd ) && ( !bAggressive || !Q_IsMeanSpaceW( *pwchEnd ) ) )
			break;

		*pwchEnd = 0;
		*pbStrippedWhitespace = true;
	}

	// walk forward in the string
	while ( pwch < pwchEnd )
	{
		if ( !iswspace( *pwch ) )
			break;

		*pbStrippedWhitespace = true;
		pwch++;
	}

	return pwch;
}

//-----------------------------------------------------------------------------
// Purpose: Strips all evil characters (ie. zero-width no-break space)
//			from a string.
//-----------------------------------------------------------------------------
bool Q_RemoveAllEvilCharacters( INOUT_Z char *pch )
{
	AssertValidStringPtr( pch );

	// convert to unicode
	size_t cch = strlen( pch );
	size_t cubDest = (cch + 1 ) * sizeof( wchar_t );
	wchar_t *pwch = stackallocT( wchar_t, cubDest );
	size_t cwch = Q_UTF8ToWString( pch, pwch, cubDest ) / sizeof( wchar_t );

	bool bStrippedWhitespace = false;

	// Walk through and skip over evil characters
	size_t nWalk = 0;
	for( size_t i=0; i<cwch; ++i )
	{
		if( !Q_IsMeanSpaceW( pwch[i] ) )
		{
			pwch[nWalk] = pwch[i];
			++nWalk;
		}
		else
		{
			bStrippedWhitespace = true;
		}
	}

	// Null terminate
	pwch[nWalk-1] = L'\0';
	

	// copy back, if necessary
	if ( bStrippedWhitespace )
	{
		Q_WStringToUTF8( pwch, pch, cch );
	}

	return bStrippedWhitespace;
}


//-----------------------------------------------------------------------------
// Purpose: strips leading and trailing whitespace
//-----------------------------------------------------------------------------
bool Q_StripPrecedingAndTrailingWhitespaceW( INOUT_Z wchar_t *pwch )
{
	intp cch = V_wcslen( pwch );

	// Early out and don't convert if we don't have any chars or leading/trailing ws.
	if ( ( cch < 1 ) || ( !iswspace( pwch[ 0 ] ) && !iswspace( pwch[ cch - 1 ] ) ) )
		return false;

	// duplicate on stack
	intp cubDest = ( cch + 1 ) * static_cast<intp>(sizeof( wchar_t ));
	wchar_t *pwchT = stackallocT( wchar_t, cubDest );
	V_wcsncpy( pwchT, pwch, cubDest );

	bool bStrippedWhitespace = false;
	pwchT = StripWhitespaceWorker( cch, pwch, &bStrippedWhitespace, false /* not aggressive */ );

	// copy back, if necessary
	if ( bStrippedWhitespace )
	{
		Q_wcsncpy( pwch, pwchT, cubDest );
	}

	return bStrippedWhitespace;
}



//-----------------------------------------------------------------------------
// Purpose: strips leading and trailing whitespace,
//		and also strips punctuation and formatting characters with "clear"
//		representations.
//-----------------------------------------------------------------------------
bool Q_AggressiveStripPrecedingAndTrailingWhitespaceW( INOUT_Z wchar_t *pwch )
{
	// duplicate on stack
	size_t cch = wcslen( pwch );
	size_t cubDest = ( cch + 1 ) * sizeof( wchar_t );
	wchar_t *pwchT = stackallocT( wchar_t, cubDest );
	Q_wcsncpy( pwchT, pwch, cubDest );

	bool bStrippedWhitespace = false;
	pwchT = StripWhitespaceWorker( cch, pwch, &bStrippedWhitespace, true /* is aggressive */ );

	// copy back, if necessary
	if ( bStrippedWhitespace )
	{
		Q_wcsncpy( pwch, pwchT, cubDest );
	}

	return bStrippedWhitespace;
}


//-----------------------------------------------------------------------------
// Purpose: strips leading and trailing whitespace
//-----------------------------------------------------------------------------
bool Q_StripPrecedingAndTrailingWhitespace( INOUT_Z char *pch )
{
	intp cch = V_strlen( pch );

	// Early out and don't convert if we don't have any chars or leading/trailing ws.
	if ( ( cch < 1 ) || ( !V_isspace( (unsigned char)pch[ 0 ] ) && !V_isspace( (unsigned char)pch[ cch - 1 ] ) ) )
		return false;

	// convert to unicode
	intp cubDest = (cch + 1 ) * static_cast<intp>(sizeof( wchar_t ));
	wchar_t *pwch = stackallocT( wchar_t, cubDest );
	intp cwch = Q_UTF8ToWString( pch, pwch, cubDest ) / sizeof( wchar_t );

	bool bStrippedWhitespace = false;
	pwch = StripWhitespaceWorker( cwch-1, pwch, &bStrippedWhitespace, false /* not aggressive */ );

	// copy back, if necessary
	if ( bStrippedWhitespace )
	{
		Q_WStringToUTF8( pwch, pch, cch );
	}

	return bStrippedWhitespace;
}

//-----------------------------------------------------------------------------
// Purpose: strips leading and trailing whitespace
//-----------------------------------------------------------------------------
bool Q_AggressiveStripPrecedingAndTrailingWhitespace( INOUT_Z char *pch )
{
	// convert to unicode
	size_t cch = strlen( pch );
	size_t cubDest = (cch + 1 ) * sizeof( wchar_t );
	wchar_t *pwch = stackallocT( wchar_t, cubDest );
	size_t cwch = Q_UTF8ToUnicode( pch, pwch, cubDest ) / sizeof( wchar_t );

	bool bStrippedWhitespace = false;
	pwch = StripWhitespaceWorker( cwch-1, pwch, &bStrippedWhitespace, true /* is aggressive */ );

	// copy back, if necessary
	if ( bStrippedWhitespace )
	{
		Q_UnicodeToUTF8( pwch, pch, cch );
	}

	return bStrippedWhitespace;
}

//-----------------------------------------------------------------------------
// Purpose: Converts a ucs2 string to a unicode (wchar_t) one, no-op on win32
//-----------------------------------------------------------------------------
intp _V_UCS2ToUnicode( const ucs2 *pUCS2, OUT_Z_BYTECAP(cubDestSizeInBytes) wchar_t *pUnicode, intp cubDestSizeInBytes )
{
	Assert( cubDestSizeInBytes >= static_cast<intp>(sizeof( *pUnicode )) );
	AssertValidWritePtr(pUnicode);
	AssertValidReadPtr(pUCS2);
	
	pUnicode[0] = 0;
#ifdef _WIN32
	intp cchResult = V_wcslen( pUCS2 );
	V_memcpy( pUnicode, pUCS2, cubDestSizeInBytes );
#else
	iconv_t conv_t = iconv_open( "UCS-4LE", "UCS-2LE" );
	size_t cchResult = -1;
	size_t nLenUnicde = cubDestSizeInBytes;
	size_t nMaxUTF8 = cubDestSizeInBytes;
	char *pIn = (char *)pUCS2;
	char *pOut = (char *)pUnicode;
	if ( conv_t != (iconv_t)-1 )
	{
		cchResult = iconv( conv_t, &pIn, &nLenUnicde, &pOut, &nMaxUTF8 );
		iconv_close( conv_t );
		if ( cchResult == -1 )
			cchResult = 0;
		else
			cchResult = nMaxUTF8;
	}
#endif
	pUnicode[(cubDestSizeInBytes / sizeof(wchar_t)) - 1] = 0;
	return cchResult;	

}

//-----------------------------------------------------------------------------
// Purpose: Converts a wchar_t string into a UCS2 string -noop on windows
//-----------------------------------------------------------------------------
intp _V_UnicodeToUCS2( const wchar_t *pUnicode, intp cubSrcInBytes, OUT_Z_BYTECAP(cubDestSizeInBytes) char *pUCS2, intp cubDestSizeInBytes )
{
#ifdef _WIN32
	// Figure out which buffer is smaller and convert from bytes to character
	// counts.
	intp cchResult = min( cubSrcInBytes/(intp)sizeof(wchar_t), cubDestSizeInBytes/(intp)sizeof(wchar_t) );
	wchar_t *pDest = (wchar_t*)pUCS2;
	wcsncpy( pDest, pUnicode, cchResult );
	// Make sure we NULL-terminate.
	pDest[ cchResult - 1 ] = L'\0';
#elif defined (POSIX)
	iconv_t conv_t = iconv_open( "UCS-2LE", "UTF-32LE" );
	size_t cchResult = -1;
	size_t nLenUnicde = cubSrcInBytes;
	size_t nMaxUCS2 = cubDestSizeInBytes;
	char *pIn = (char*)pUnicode;
	char *pOut = pUCS2;
	if ( conv_t != (iconv_t) -1 )
	{
		cchResult = iconv( conv_t, &pIn, &nLenUnicde, &pOut, &nMaxUCS2 );
		iconv_close( conv_t );
		if ( cchResult == -1 )
			cchResult = 0;
		else
			cchResult = cubSrcInBytes / sizeof( wchar_t );
	}
#else
	#error "Please define your platform"
#endif
	return cchResult;	
}


//-----------------------------------------------------------------------------
// Purpose: Converts a ucs-2 (windows wchar_t) string into a UTF8 (standard) string
//-----------------------------------------------------------------------------
int _V_UCS2ToUTF8( const ucs2 *pUCS2, OUT_Z_BYTECAP(cubDestSizeInBytes) char *pUTF8, int cubDestSizeInBytes )
{
	AssertValidStringPtr(pUTF8, cubDestSizeInBytes);
	AssertValidReadPtr(pUCS2);
	
	pUTF8[0] = '\0';
#ifdef _WIN32
	// under win32 wchar_t == ucs2, sigh
	int cchResult = WideCharToMultiByte( CP_UTF8, 0, pUCS2, -1, pUTF8, cubDestSizeInBytes, NULL, NULL );
#elif defined(POSIX)
	iconv_t conv_t = iconv_open( "UTF-8", "UCS-2LE" );
	size_t cchResult = -1;

	// pUCS2 will be null-terminated so use that to work out the input
	// buffer size. Note that we shouldn't assume iconv will stop when it
	// finds a zero, and nLenUnicde should be given in bytes, so we multiply
	// it by sizeof( ucs2 ) at the end.
	size_t nLenUnicde = 0;
	while ( pUCS2[nLenUnicde] )
	{
		++nLenUnicde;
	}
	nLenUnicde *= sizeof( ucs2 );

	// Calculate number of bytes we want iconv to write, leaving space
	// for the null-terminator
	size_t nMaxUTF8 = cubDestSizeInBytes - 1;
	char *pIn = (char *)pUCS2;
	char *pOut = (char *)pUTF8;
	if ( conv_t != (iconv_t) -1 )
	{
		const size_t nBytesToWrite = nMaxUTF8;
		cchResult = iconv( conv_t, &pIn, &nLenUnicde, &pOut, &nMaxUTF8 );

		// Calculate how many bytes were actually written and use that to
		// null-terminate our output string.
		const size_t nBytesWritten = nBytesToWrite - nMaxUTF8;
		pUTF8[nBytesWritten] = 0;

		iconv_close( conv_t );
		if ( cchResult == -1 )
			cchResult = 0;
		else
			cchResult = nMaxUTF8;
	}
#endif
	pUTF8[cubDestSizeInBytes - 1] = '\0';
	return cchResult;	
}


//-----------------------------------------------------------------------------
// Purpose: Converts a UTF8 to ucs-2 (windows wchar_t)
//-----------------------------------------------------------------------------
int _V_UTF8ToUCS2( const char *pUTF8, [[maybe_unused]] int cubSrcInBytes, OUT_Z_BYTECAP(cubDestSizeInBytes) ucs2 *pUCS2, int cubDestSizeInBytes )
{
	Assert( cubDestSizeInBytes >= static_cast<intp>(sizeof(pUCS2[0])) );
	AssertValidStringPtr(pUTF8, cubDestSizeInBytes);
	AssertValidReadPtr(pUCS2);

	pUCS2[0] = 0;
#ifdef _WIN32
	// under win32 wchar_t == ucs2, sigh
	int cchResult = MultiByteToWideChar( CP_UTF8, 0, pUTF8, -1, pUCS2, cubDestSizeInBytes / (int)sizeof(wchar_t) );
#elif defined( _PS3 ) // bugbug JLB
	int cchResult = 0;
	Assert( 0 );
#elif defined(POSIX)
	iconv_t conv_t = iconv_open( "UCS-2LE", "UTF-8" );
	size_t cchResult = -1;
	size_t nLenUnicde = cubSrcInBytes;
	size_t nMaxUTF8 = cubDestSizeInBytes;
	char *pIn = (char *)pUTF8;
	char *pOut = (char *)pUCS2;
	if ( conv_t != (iconv_t) -1 )
	{
		cchResult = iconv( conv_t, &pIn, &nLenUnicde, &pOut, &nMaxUTF8 );
		iconv_close( conv_t );
		if ( cchResult == -1 )
			cchResult = 0;
		else
			cchResult = cubSrcInBytes;

	}
#endif
	pUCS2[ (cubDestSizeInBytes/sizeof(ucs2)) - 1] = 0;
	return cchResult;	
}



//-----------------------------------------------------------------------------
// Purpose: Returns the 4 bit nibble for a hex character
// Input  : c - 
// Output : unsigned char
//-----------------------------------------------------------------------------
unsigned char V_nibble( char c )
{
	unsigned char nibble;
	return V_nibble(c, nibble) ? nibble : '0';
}

bool V_nibble( char c, unsigned char &nibble )
{
	if ( ( c >= '0' ) &&
		 ( c <= '9' ) )
	{
		 nibble = (unsigned char)(c - '0');
		 return true;
	}

	if ( ( c >= 'A' ) &&
		 ( c <= 'F' ) )
	{
		 nibble = (unsigned char)(c - 'A' + 0x0a);
		 return true;
	}

	if ( ( c >= 'a' ) &&
		 ( c <= 'f' ) )
	{
		 nibble = (unsigned char)(c - 'a' + 0x0a);
		 return true;
	}

	// dimhotepus: To match original behavior.
	nibble = '0';
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *in - 
//			numchars - 
//			*out - 
//			maxoutputbytes - 
//-----------------------------------------------------------------------------
bool V_hextobinary( IN_Z_CAP(numchars) char const *in, intp numchars, OUT_BYTECAP(maxoutputbytes) byte *out, intp maxoutputbytes )
{
	numchars = min( V_strlen( in ), numchars );
	Assert((numchars & 0x1) != 0x1);

	if (numchars & 0x1)
	{
		// dimhotepus: Requre even buffer.
		return false;
	}

	// Must be an even # of input characters (two chars per output byte)
	Assert( numchars >= 2 );
	if ( numchars < 2) return false;

	BitwiseClear( out, maxoutputbytes );
	
	unsigned char nibble1, nibble2;

	byte *p = out;
	for ( intp i = 0;
		 ( i < numchars ) && ( ( p - out ) < maxoutputbytes ); 
		 i+=2, p++ )
	{
		if ( V_nibble( in[i], nibble1 ) && V_nibble( in[i+1], nibble2 ) )
		{
			*p = ( nibble1 << 4 ) | nibble2;
		}
		else
		{
			return false;
		}
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *in - 
//			inputbytes - 
//			*out - 
//			outsize - 
//-----------------------------------------------------------------------------
void V_binarytohex( IN_BYTECAP(inputbytes) const byte *in, intp inputbytes, OUT_Z_CAP(outsize) char *out, intp outsize )
{
	Assert( outsize >= 1 );
	char doublet[10];
	intp i;

	out[0]=0;

	for ( i = 0; i < inputbytes; i++ )
	{
		unsigned char c = in[i];
		V_snprintf( doublet, sizeof( doublet ), "%02x", c );
		V_strncat( out, doublet, outsize, COPY_ALL_CHARACTERS );
	}
}

// Even though \ on Posix (Linux&Mac) isn't techincally a path separator we are
// now counting it as one even Posix since so many times our filepaths aren't actual
// paths but rather text strings passed in from data files, treating \ as a pathseparator
// covers the full range of cases
bool PATHSEPARATOR( char c )
{
	return c == '\\' || c == '/';
}


//-----------------------------------------------------------------------------
// Purpose: Extracts the base name of a file (no path, no extension, assumes '/' or '\' as path separator)
// Input  : *in - 
//			*out - 
//			maxlen - 
//-----------------------------------------------------------------------------
void V_FileBase( IN_Z const char *in, OUT_Z_CAP(maxlen) char *out, intp maxlen )
{
	Assert( maxlen >= 1 );
	Assert( in );
	Assert( out );

	if ( !in || !in[ 0 ] )
	{
		*out = 0;
		return;
	}

	intp len, start, end;

	len = V_strlen( in );
	
	// scan backward for '.'
	end = len - 1;
	while ( end&& in[end] != '.' && !PATHSEPARATOR( in[end] ) )
	{
		end--;
	}
	
	if ( in[end] != '.' )		// no '.', copy to end
	{
		end = len-1;
	}
	else 
	{
		end--;					// Found ',', copy to left of '.'
	}

	// Scan backward for '/'
	start = len-1;
	while ( start >= 0 && !PATHSEPARATOR( in[start] ) )
	{
		start--;
	}

	if ( start < 0 || !PATHSEPARATOR( in[start] ) )
	{
		start = 0;
	}
	else 
	{
		start++;
	}

	// Length of new sting
	len = end - start + 1;

	intp maxcopy = min( len + 1, maxlen );

	// Copy partial string
	V_strncpy( out, &in[start], maxcopy );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *ppath - 
//-----------------------------------------------------------------------------
void V_StripTrailingSlash( INOUT_Z char *ppath )
{
	Assert( ppath );

	intp len = V_strlen( ppath );
	if ( len > 0 )
	{
		if ( PATHSEPARATOR( ppath[ len - 1 ] ) )
		{
			ppath[ len - 1 ] = 0;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *ppline - 
//-----------------------------------------------------------------------------
void V_StripTrailingWhitespace( INOUT_Z char *ppline )
{
	Assert( ppline );

	intp len = V_strlen( ppline );
	while ( len > 0 )
	{
		if ( !V_isspace( ppline[ len - 1 ] ) )
			break;
		ppline[ len - 1 ] = 0;
		len--;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *ppline - 
//-----------------------------------------------------------------------------
void V_StripLeadingWhitespace( INOUT_Z char *ppline )
{
	Assert( ppline );

	// Skip past initial whitespace
	intp skip = 0;
	while( V_isspace( ppline[ skip ] ) )
		skip++;
	// Shuffle the rest of the string back (including the NULL-terminator)
	if ( skip )
	{
		while( ( ppline[0] = ppline[skip] ) != 0 )
			ppline++;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *ppline - 
//-----------------------------------------------------------------------------
void V_StripSurroundingQuotes( INOUT_Z char *ppline )
{
	Assert( ppline );

	intp len = V_strlen( ppline ) - 2;
	if ( ( ppline[0] == '"' ) && ( len >= 0 ) && ( ppline[len+1] == '"' ) )
	{
		for ( intp i = 0; i < len; i++ )
			ppline[i] = ppline[i+1];
		ppline[len] = 0;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *in - 
//			*out - 
//			outSize - 
//-----------------------------------------------------------------------------
void V_StripExtension( IN_Z const char *in, OUT_Z_CAP(outSize) char *out, intp outSize )
{
	// Find the last dot. If it's followed by a dot or a slash, then it's part of a 
	// directory specifier like ../../somedir/./blah.

	// scan backward for '.'
	intp end = V_strlen( in ) - 1;
	while ( end > 0 && in[end] != '.' && !PATHSEPARATOR( in[end] ) )
	{
		--end;
	}

	if (end > 0 && !PATHSEPARATOR( in[end] ) && end < outSize)
	{
		intp nChars = min( end, outSize-1 );
		if ( out != in )
		{
			memcpy( out, in, nChars );
		}
		out[nChars] = 0;
	}
	else
	{
		// nothing found
		if ( out != in )
		{
			V_strncpy( out, in, outSize );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *path - 
//			*extension - 
//			pathStringLength - 
//-----------------------------------------------------------------------------
void V_DefaultExtension( INOUT_Z_CAP(pathStringLength) char *path, IN_Z const char *extension, intp pathStringLength )
{
	Assert( path );
	Assert( pathStringLength >= 1 );
	Assert( extension );
	Assert( extension[0] == '.' );

	// if path doesn't have a .EXT, append extension
	// (extension should include the .)
	char *src = path + V_strlen(path) - 1;

	while ( !PATHSEPARATOR( *src ) && ( src > path ) )
	{
		if (*src == '.')
		{
			// it has an extension
			return;                 
		}
		src--;
	}

	// Concatenate the desired extension
	V_strncat( path, extension, pathStringLength, COPY_ALL_CHARACTERS );
}

//-----------------------------------------------------------------------------
// Purpose: Force extension...
// Input  : *path - 
//			*extension - 
//			pathStringLength - 
//-----------------------------------------------------------------------------
void V_SetExtension( INOUT_Z_CAP(pathStringLength) char *path, IN_Z const char *extension, intp pathStringLength )
{
	V_StripExtension( path, path, pathStringLength );

	// We either had an extension and stripped it, or didn't have an extension
	// at all. Either way, we need to concatenate our extension now.

	// extension is not required to start with '.', so if it's not there,
	// then append that first.
	if ( extension[0] != '.' )
	{
		V_strncat( path, ".", pathStringLength, COPY_ALL_CHARACTERS );
	}

	V_strncat( path, extension, pathStringLength, COPY_ALL_CHARACTERS );
}

//-----------------------------------------------------------------------------
// Purpose: Remove final filename from string
// Input  : *path - 
// Output : void  V_StripFilename
//-----------------------------------------------------------------------------
void  V_StripFilename ( INOUT_Z char *path)
{
	intp length = V_strlen( path )-1;
	if ( length <= 0 )
		return;

	while ( length > 0 && 
		!PATHSEPARATOR( path[length] ) )
	{
		length--;
	}

	path[ length ] = 0;
}

#ifdef _WIN32
#define CORRECT_PATH_SEPARATOR '\\'
#define INCORRECT_PATH_SEPARATOR '/'
#elif POSIX
#define CORRECT_PATH_SEPARATOR '/'
#define INCORRECT_PATH_SEPARATOR '\\'
#endif

//-----------------------------------------------------------------------------
// Purpose: Changes all '/' or '\' characters into separator
// Input  : *pname - 
//			separator - 
//-----------------------------------------------------------------------------
void V_FixSlashes( INOUT_Z char *pname, char separator /* = CORRECT_PATH_SEPARATOR */ )
{
	while ( *pname )
	{
		if ( *pname == INCORRECT_PATH_SEPARATOR || *pname == CORRECT_PATH_SEPARATOR )
		{
			*pname = separator;
		}
		pname++;
	}
}


//-----------------------------------------------------------------------------
// Purpose: This function fixes cases of filenames like materials\\blah.vmt or somepath\otherpath\\ and removes the extra double slash.
//-----------------------------------------------------------------------------
void V_FixDoubleSlashes( INOUT_Z char *pStr )
{
	intp len = V_strlen( pStr );

	for ( intp i=1; i < len-1; i++ )
	{
		if ( (pStr[i] == '/' || pStr[i] == '\\') && (pStr[i+1] == '/' || pStr[i+1] == '\\') )
		{
			// This means there's a double slash somewhere past the start of the filename. That 
			// can happen in Hammer if they use a material in the root directory. You'll get a filename 
			// that looks like 'materials\\blah.vmt'
			V_memmove( &pStr[i], &pStr[i+1], len - i );
			--len;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Strip off the last directory from dirName
// Input  : *dirName - 
//			maxlen - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool V_StripLastDir( INOUT_Z_CAP(maxlen) char *dirName, intp maxlen )
{
	if( dirName[0] == 0 || 
		!V_stricmp( dirName, "./" ) || 
		!V_stricmp( dirName, ".\\" ) )
		return false;
	
	intp len = V_strlen( dirName );

	Assert( len < maxlen );

	// skip trailing slash
	if ( PATHSEPARATOR( dirName[len-1] ) )
	{
		len--;
	}

	while ( len > 0 )
	{
		if ( PATHSEPARATOR( dirName[len-1] ) )
		{
			dirName[len] = 0;
			V_FixSlashes( dirName, CORRECT_PATH_SEPARATOR );
			return true;
		}
		len--;
	}

	// Allow it to return an empty string and true. This can happen if something like "tf2/" is passed in.
	// The correct behavior is to strip off the last directory ("tf2") and return true.
	if( len == 0 )
	{
		V_snprintf( dirName, maxlen, ".%c", CORRECT_PATH_SEPARATOR );
		return true;
	}

	return true;
}


//-----------------------------------------------------------------------------
// Purpose: Returns a pointer to the beginning of the unqualified file name 
//			(no path information)
// Input:	in - file name (may be unqualified, relative or absolute path)
// Output:	pointer to unqualified file name
//-----------------------------------------------------------------------------
const char * V_UnqualifiedFileName( IN_Z const char * in )
{
	// dimhotepus: ASAN catch. Fix one before range read.
	if ( !in[0] ) return in;

	// back up until the character after the first path separator we find,
	// or the beginning of the string
	const char * out = in + strlen( in ) - 1;
	while ( ( out > in ) && ( !PATHSEPARATOR( *( out-1 ) ) ) )
		out--;
	return out;
}


//-----------------------------------------------------------------------------
// Purpose: Composes a path and filename together, inserting a path separator
//			if need be
// Input:	path - path to use
//			filename - filename to use
//			dest - buffer to compose result in
//			destSize - size of destination buffer
//-----------------------------------------------------------------------------
void V_ComposeFileName( IN_Z const char *path, IN_Z const char *filename, OUT_Z_CAP(destSize) char *dest, intp destSize )
{
	V_strncpy( dest, path, destSize );
	V_FixSlashes( dest );
	V_AppendSlash( dest, destSize );
	V_strncat( dest, filename, destSize, COPY_ALL_CHARACTERS );
	V_FixSlashes( dest );
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *path - 
//			*dest - 
//			destSize - 
// Output : void V_ExtractFilePath
//-----------------------------------------------------------------------------
bool V_ExtractFilePath ( IN_Z const char *path,  OUT_Z_CAP(destSize) char *dest, intp destSize )
{
	Assert( destSize >= 1 );
	if ( destSize < 1 )
	{
		return false;
	}

	// Last char
	intp len = V_strlen(path);
	const char *src = path + (len ? len-1 : 0);

	// back up until a \ or the start
	while ( src != path && !PATHSEPARATOR( *(src-1) ) )
	{
		src--;
	}

	intp copysize = min(src - path, destSize - 1 );
	memcpy( dest, path, copysize );
	dest[copysize] = 0;

	return copysize != 0 ? true : false;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *path - 
//			*dest - 
//			destSize - 
// Output : void V_ExtractFileExtension
//-----------------------------------------------------------------------------
void V_ExtractFileExtension( IN_Z const char *path, OUT_Z_CAP(destSize) char *dest, intp destSize )
{
	if (destSize)
		*dest = NULL;
	const char * extension = V_GetFileExtension( path );
	if ( NULL != extension )
		V_strncpy( dest, extension, destSize );
}


//-----------------------------------------------------------------------------
// Purpose: Returns a pointer to the file extension within a file name string
// Input:	in - file name 
// Output:	pointer to beginning of extension (after the "."), or NULL
//				if there is no extension
//-----------------------------------------------------------------------------
RET_MAY_BE_NULL const char * V_GetFileExtension( IN_Z const char * path )
{
	// dimhotepus: ASAN catch. Fix one before range read.
	if ( !path[0] ) return nullptr;

	const char   *src = path + strlen(path) - 1;

//
// back up until a . or the start
//
	while (src != path && *(src-1) != '.' )
		src--;

	// check to see if the '.' is part of a pathname
	if (src == path || PATHSEPARATOR( *src ) )
	{		
		return NULL;  // no extension
	}

	return src;
}


//-----------------------------------------------------------------------------
// Purpose: Returns a pointer to the filename part of a path string
// Input:	in - file name 
// Output:	pointer to beginning of filename (after the "/"). If there were no /, 
//          output is identical to input
//-----------------------------------------------------------------------------
const char * V_GetFileName( IN_Z const char * path )
{
	return V_UnqualifiedFileName( path );
}


bool V_RemoveDotSlashes( INOUT_Z char *pFilename, char separator, bool bRemoveDoubleSlashes /* = true */ )
{
	char *pIn = pFilename;
	char *pOut = pFilename;
	bool bRetVal = true;

	bool bBoundary = true;
	while ( *pIn )
	{
		if ( bBoundary && pIn[0] == '.' && pIn[1] == '.' && ( PATHSEPARATOR( pIn[2] ) || !pIn[2] ) )
		{
			// Get rid of /../ or trailing /.. by backing pOut up to previous separator

			// Eat the last separator (or repeated separators) we wrote out
			while ( pOut != pFilename && pOut[-1] == separator )
			{
				--pOut;
			}

			while ( true )
			{
				if ( pOut == pFilename )
				{
					bRetVal = false; // backwards compat. return value, even though we continue handling
					break;
				}
				--pOut;
				if ( *pOut == separator )
				{
					break;
				}
			}

			// Skip the '..' but not the slash, next loop iteration will handle separator
			pIn += 2;
			bBoundary = ( pOut == pFilename );
		}
		else if ( bBoundary && pIn[0] == '.' && ( PATHSEPARATOR( pIn[1] ) || !pIn[1] ) )
		{
			// Handle "./" by simply skipping this sequence. bBoundary is unchanged.
			if ( PATHSEPARATOR( pIn[1] ) )
			{
				pIn += 2;
			}
			else
			{
				// Special case: if trailing "." is preceded by separator, eg "path/.",
				// then the final separator should also be stripped. bBoundary may then
				// be in an incorrect state, but we are at the end of processing anyway
				// so we don't really care (the processing loop is about to terminate).
				if ( pOut != pFilename && pOut[-1] == separator )
				{
					--pOut;
				}
				pIn += 1;
			}
		}
		else if ( PATHSEPARATOR( pIn[0] ) )
		{
			*pOut = separator;
			pOut += 1 - (bBoundary & bRemoveDoubleSlashes & (pOut != pFilename));
			pIn += 1;
			bBoundary = true;
		}
		else
		{
			if ( pOut != pIn )
			{
				*pOut = *pIn;
			}
			pOut += 1;
			pIn += 1;
			bBoundary = false;
		}
	}
	*pOut = 0;

	return bRetVal;
}


void V_AppendSlash( INOUT_Z_CAP(strSize) char *pStr, intp strSize )
{
	intp len = V_strlen( pStr );
	if ( len > 0 && !PATHSEPARATOR(pStr[len-1]) )
	{
		if ( len+1 >= strSize )
			Error( "V_AppendSlash: ran out of space on %s.", pStr );
		
		pStr[len] = CORRECT_PATH_SEPARATOR;
		pStr[len+1] = 0;
	}
}


void V_MakeAbsolutePath( OUT_Z_CAP(outLen) char *pOut, int outLen, IN_Z const char *pPath, IN_OPT_Z const char *pStartingDir )
{
	if ( V_IsAbsolutePath( pPath ) )
	{
		// pPath is not relative.. just copy it.
		V_strncpy( pOut, pPath, outLen );
	}
	else
	{
		// Make sure the starting directory is absolute..
		if ( pStartingDir && V_IsAbsolutePath( pStartingDir ) )
		{
			V_strncpy( pOut, pStartingDir, outLen );
		}
		else
		{
			if ( !_getcwd( pOut, outLen ) )
				Error( "V_MakeAbsolutePath: _getcwd failed." );

			if ( pStartingDir )
			{
				V_AppendSlash( pOut, outLen );
				V_strncat( pOut, pStartingDir, outLen, COPY_ALL_CHARACTERS );
			}
		}

		// Concatenate the paths.
		V_AppendSlash( pOut, outLen );
		V_strncat( pOut, pPath, outLen, COPY_ALL_CHARACTERS );
	}

	if ( !V_RemoveDotSlashes( pOut ) )
		Error( "V_MakeAbsolutePath: tried to \"..\" past the root." );

	//V_FixSlashes( pOut ); - handled by V_RemoveDotSlashes
}


//-----------------------------------------------------------------------------
// Makes a relative path
//-----------------------------------------------------------------------------
bool V_MakeRelativePath( IN_Z const char *pFullPath, IN_Z const char *pDirectory, OUT_Z_CAP(nBufLen) char *pRelativePath, intp nBufLen )
{
	// dimhotepus: Prevent zero size buffer overflow.
	if (nBufLen > 0)
		pRelativePath[0] = '\0';

	const char *pPath = pFullPath;
	const char *pDir = pDirectory;

	// Strip out common parts of the path
	const char *pLastCommonPath = NULL;
	const char *pLastCommonDir = NULL;
	while ( *pPath && ( FastToLower( *pPath ) == FastToLower( *pDir ) || 
						( PATHSEPARATOR( *pPath ) && ( PATHSEPARATOR( *pDir ) || (*pDir == 0) ) ) ) )
	{
		if ( PATHSEPARATOR( *pPath ) )
		{
			pLastCommonPath = pPath + 1;
			pLastCommonDir = pDir + 1;
		}
		if ( *pDir == 0 )
		{
			--pLastCommonDir;
			break;
		}
		++pDir; ++pPath;
	}

	// Nothing in common
	if ( !pLastCommonPath )
		return false;

	// For each path separator remaining in the dir, need a ../
	intp nOutLen = 0;
	bool bLastCharWasSeparator = true;
	for ( ; *pLastCommonDir; ++pLastCommonDir )
	{
		if ( PATHSEPARATOR( *pLastCommonDir ) )
		{
			AssertMsg( nOutLen < nBufLen,
				"Overflow relative path buffer (%zd) for full path '%s' and directory '%s'.",
				nBufLen, pFullPath, pDirectory );

			if ( nOutLen < nBufLen )
				pRelativePath[nOutLen++] = '.';
			else
			{
				pRelativePath[0] = '\0';
				return false;
			}

			AssertMsg( nOutLen < nBufLen,
				"Overflow relative path buffer (%zd) for full path '%s' and directory '%s'.",
				nBufLen, pFullPath, pDirectory );

			if ( nOutLen < nBufLen )
				pRelativePath[nOutLen++] = '.';
			else
			{
				pRelativePath[0] = '\0';
				return false;
			}

			AssertMsg( nOutLen < nBufLen,
				"Overflow relative path buffer (%zd) for full path '%s' and directory '%s'.",
				nBufLen, pFullPath, pDirectory );

			if ( nOutLen < nBufLen )
				pRelativePath[nOutLen++] = CORRECT_PATH_SEPARATOR;
			else
			{
				pRelativePath[0] = '\0';
				return false;
			}

			bLastCharWasSeparator = true;
		}
		else
		{
			bLastCharWasSeparator = false;
		}
	}

	// Deal with relative paths not specified with a trailing slash
	if ( !bLastCharWasSeparator )
	{
		AssertMsg( nOutLen < nBufLen,
			"Overflow relative path buffer (%zd) for full path '%s' and directory '%s'.",
			nBufLen, pFullPath, pDirectory );

		if ( nOutLen < nBufLen )
			pRelativePath[nOutLen++] = '.';
		else
		{
			pRelativePath[0] = '\0';
			return false;
		}

		AssertMsg( nOutLen < nBufLen,
			"Overflow relative path buffer (%zd) for full path '%s' and directory '%s'.",
			nBufLen, pFullPath, pDirectory );

		if ( nOutLen < nBufLen )
			pRelativePath[nOutLen++] = '.';
		else
		{
			pRelativePath[0] = '\0';
			return false;
		}

		AssertMsg( nOutLen < nBufLen,
			"Overflow relative path buffer (%zd) for full path '%s' and directory '%s'.",
			nBufLen, pFullPath, pDirectory );

		if ( nOutLen < nBufLen )
			pRelativePath[nOutLen++] = CORRECT_PATH_SEPARATOR;
		else
		{
			pRelativePath[0] = '\0';
			return false;
		}
	}

	// Copy the remaining part of the relative path over, fixing the path separators
	for ( ; *pLastCommonPath; ++pLastCommonPath )
	{
		if ( PATHSEPARATOR( *pLastCommonPath ) )
		{
			AssertMsg( nOutLen < nBufLen,
				"Overflow relative path buffer (%zd) for full path '%s' and directory '%s'.",
				nBufLen, pFullPath, pDirectory );

			if ( nOutLen < nBufLen )
				pRelativePath[nOutLen++] = CORRECT_PATH_SEPARATOR;
			else
			{
				pRelativePath[0] = '\0';
				return false;
			}
		}
		else
		{
			AssertMsg( nOutLen < nBufLen,
				"Overflow relative path buffer (%zd) for full path '%s' and directory '%s'.",
				nBufLen, pFullPath, pDirectory );

			if ( nOutLen < nBufLen )
				pRelativePath[nOutLen++] = *pLastCommonPath;
			else
			{
				pRelativePath[0] = '\0';
				return false;
			}
		}

		// Check for overflow
		if ( nOutLen == nBufLen - 1 )
			break;
	}

	if ( nOutLen < nBufLen )
	{
		pRelativePath[nOutLen] = 0;
		return true;
	}

	AssertMsg( nOutLen < nBufLen,
		"Overflow relative path buffer (%zd) for full path '%s' and directory '%s'.",
		nBufLen, pFullPath, pDirectory );
	
	pRelativePath[0] = '\0';
	return false;
}


//-----------------------------------------------------------------------------
// small helper function shared by lots of modules
//-----------------------------------------------------------------------------
bool V_IsAbsolutePath( IN_Z const char *pStr )
{
	char first = pStr[0];
	bool bIsAbsolute = ( first && pStr[1] == ':' ) || first == '/' || first == '\\';
	return bIsAbsolute;
}


// Copies at most nCharsToCopy bytes from pIn into pOut.
// Returns false if it would have overflowed pOut's buffer.
static bool CopyToMaxChars( char *pOut, intp outSize, const char *pIn, intp nCharsToCopy )
{
	if ( outSize == 0 )
		return false;

	intp iOut = 0;
	while ( *pIn && nCharsToCopy > 0 )
	{
		if ( iOut == (outSize-1) )
		{
			pOut[iOut] = 0;
			return false;
		}
		pOut[iOut] = *pIn;
		++iOut;
		++pIn;
		--nCharsToCopy;
	}
	
	pOut[iOut] = 0;
	return true;
}


//-----------------------------------------------------------------------------
// Fixes up a file name, removing dot slashes, fixing slashes, converting to lowercase, etc.
//-----------------------------------------------------------------------------
void V_FixupPathName( OUT_Z_CAP(nOutLen) char *pOut, intp nOutLen, IN_Z const char *pPath )
{
	V_strncpy( pOut, pPath, nOutLen );
	V_RemoveDotSlashes( pOut, CORRECT_PATH_SEPARATOR, true );
#ifdef WIN32
	V_strlower( pOut );
#endif
}


// Returns true if it completed successfully.
// If it would overflow pOut, it fills as much as it can and returns false.
bool V_StrSubst( 
	IN_Z const char *pIn, 
	IN_Z const char *pMatch,
	IN_Z const char *pReplaceWith,
	OUT_Z_CAP(outLen) char *pOut,
	intp outLen,
	bool bCaseSensitive
	)
{
	intp replaceFromLen = strlen( pMatch );
	intp replaceToLen = strlen( pReplaceWith );

	const char *pInStart = pIn;
	char *pOutPos = pOut;
	pOutPos[0] = 0;

	while ( 1 )
	{
		intp nRemainingOut = outLen - (pOutPos - pOut);

		const char *pTestPos = ( bCaseSensitive ? strstr( pInStart, pMatch ) : V_stristr( pInStart, pMatch ) );
		if ( pTestPos )
		{
			// Found an occurence of pMatch. First, copy whatever leads up to the string.
			intp copyLen = pTestPos - pInStart;
			if ( !CopyToMaxChars( pOutPos, nRemainingOut, pInStart, copyLen ) )
				return false;
			
			// Did we hit the end of the output string?
			if ( copyLen > nRemainingOut-1 )
				return false;

			pOutPos += strlen( pOutPos );
			nRemainingOut = outLen - (pOutPos - pOut);

			// Now add the replacement string.
			if ( !CopyToMaxChars( pOutPos, nRemainingOut, pReplaceWith, replaceToLen ) )
				return false;

			pInStart += copyLen + replaceFromLen;
			pOutPos += replaceToLen;			
		}
		else
		{
			// We're at the end of pIn. Copy whatever remains and get out.
			intp copyLen = strlen( pInStart );
			V_strncpy( pOutPos, pInStart, nRemainingOut );
			return ( copyLen <= nRemainingOut-1 );
		}
	}
}


char* AllocString( const char *pStr, intp nMaxChars )
{
	intp allocLen = (intp)strlen( pStr );
	if ( nMaxChars == -1 )
		allocLen += 1;
	else
		allocLen = min( allocLen, nMaxChars ) + 1;

	char *pOut = new char[allocLen];
	V_strncpy( pOut, pStr, allocLen );
	return pOut;
}


void V_SplitString2( IN_Z const char *pString, const char **pSeparators, intp nSeparators, CUtlVector<char*> &outStrings )
{
	outStrings.Purge();
	const char *pCurPos = pString;
	while ( 1 )
	{
		intp iFirstSeparator = -1;
		const char *pFirstSeparator = 0;
		for ( intp i=0; i < nSeparators; i++ )
		{
			const char *pTest = V_stristr( pCurPos, pSeparators[i] );
			if ( pTest && (!pFirstSeparator || pTest < pFirstSeparator) )
			{
				iFirstSeparator = i;
				pFirstSeparator = pTest;
			}
		}

		if ( pFirstSeparator )
		{
			// Split on this separator and continue on.
			intp separatorLen = strlen( pSeparators[iFirstSeparator] );
			if ( pFirstSeparator > pCurPos )
			{
				outStrings.AddToTail( AllocString( pCurPos, pFirstSeparator-pCurPos ) );
			}

			pCurPos = pFirstSeparator + separatorLen;
		}
		else
		{
			// Copy the rest of the string
			if ( strlen( pCurPos ) )
			{
				outStrings.AddToTail( AllocString( pCurPos, -1 ) );
			}
			return;
		}
	}
}


void V_SplitString( IN_Z const char *pString, IN_Z const char *pSeparator, CUtlVector<char*> &outStrings )
{
	V_SplitString2( pString, &pSeparator, 1, outStrings );
}


bool V_GetCurrentDirectory( OUT_Z_CAP(maxLen) char *pOut, int maxLen )
{
	return _getcwd( pOut, maxLen ) == pOut;
}


bool V_SetCurrentDirectory( IN_Z const char *pDirName )
{
	return chdir( pDirName ) == 0;
}


// This function takes a slice out of pStr and stores it in pOut.
// It follows the Python slice convention:
// Negative numbers wrap around the string (-1 references the last character).
// Numbers are clamped to the end of the string.
void V_StrSlice( IN_Z const char *pStr, intp firstChar, intp lastCharNonInclusive, OUT_Z_CAP(outSize) char *pOut, intp outSize )
{
	if ( outSize == 0 )
		return;
	
	intp length = strlen( pStr );

	// Fixup the string indices.
	if ( firstChar < 0 )
	{
		firstChar = length - (-firstChar % length);
	}
	else if ( firstChar >= length )
	{
		pOut[0] = 0;
		return;
	}

	if ( lastCharNonInclusive < 0 )
	{
		lastCharNonInclusive = length - (-lastCharNonInclusive % length);
	}
	else if ( lastCharNonInclusive > length )
	{
		lastCharNonInclusive %= length;
	}

	if ( lastCharNonInclusive <= firstChar )
	{
		pOut[0] = 0;
		return;
	}

	intp copyLen = lastCharNonInclusive - firstChar;
	if ( copyLen <= (outSize-1) )
	{
		memcpy( pOut, &pStr[firstChar], copyLen );
		pOut[copyLen] = 0;
	}
	else
	{
		memcpy( pOut, &pStr[firstChar], outSize-1 );
		pOut[outSize-1] = 0;
	}
}


void V_StrLeft( IN_Z const char *pStr, intp nChars, OUT_Z_CAP(outSize) char *pOut, intp outSize )
{
	if ( nChars == 0 )
	{
		if ( outSize != 0 )
			pOut[0] = 0;

		return;
	}

	V_StrSlice( pStr, 0, nChars, pOut, outSize );
}


void V_StrRight( IN_Z const char *pStr, intp nChars, OUT_Z_CAP(outSize) char *pOut, intp outSize )
{
	intp len = strlen( pStr );
	if ( nChars >= len )
	{
		V_strncpy( pOut, pStr, outSize );
	}
	else
	{
		V_StrSlice( pStr, -nChars, strlen( pStr ), pOut, outSize );
	}
}

//-----------------------------------------------------------------------------
// Convert multibyte to wchar + back
//-----------------------------------------------------------------------------
void V_strtowcs( IN_Z_CAP(nInSize) const char *pString, int nInSize, OUT_Z_BYTECAP(nOutSizeInBytes) wchar_t *pWString, int nOutSizeInBytes )
{
	Assert( nOutSizeInBytes >= static_cast<intp>(sizeof(pWString[0])) );
#ifdef _WIN32
	int nOutSizeInChars = nOutSizeInBytes / sizeof(pWString[0]);
	int result = MultiByteToWideChar( CP_UTF8, 0, pString, nInSize, pWString, nOutSizeInChars );
	// If the string completely fails to fit then MultiByteToWideChar will return 0.
	// If the string exactly fits but with no room for a null-terminator then MultiByteToWideChar
	// will happily fill the buffer and omit the null-terminator, returning nOutSizeInChars.
	// Either way we need to return an empty string rather than a bogus and possibly not
	// null-terminated result.
	if ( result <= 0 || result >= nOutSizeInChars )
	{
		// If nInSize includes the null-terminator then a result of nOutSizeInChars is
		// legal. We check this by seeing if the last character in the output buffer is
		// a zero.
		if ( result == nOutSizeInChars && pWString[ nOutSizeInChars - 1 ] == 0)
		{
			// We're okay! Do nothing.
		}
		else
		{
			// The string completely to fit. Null-terminate the buffer.
			*pWString = L'\0';
		}
	}
	else
	{
		// We have successfully converted our string. Now we need to null-terminate it, because
		// MultiByteToWideChar will only do that if nInSize includes the source null-terminator!
		pWString[ result ] = L'\0';
	}
#elif POSIX
	if ( mbstowcs( pWString, pString, nOutSizeInBytes / sizeof(pWString[0]) ) <= 0 )
	{
		*pWString = 0;
	}
#endif
}

void V_wcstostr( IN_Z_CAP(nInSize) const wchar_t *pWString, int nInSize, OUT_Z_CAP(nOutSizeInBytes) char *pString, int nOutSizeInBytes )
{
#ifdef _WIN32
	int result = WideCharToMultiByte( CP_UTF8, 0, pWString, nInSize, pString, nOutSizeInBytes, NULL, NULL );
	// If the string completely fails to fit then MultiByteToWideChar will return 0.
	// If the string exactly fits but with no room for a null-terminator then MultiByteToWideChar
	// will happily fill the buffer and omit the null-terminator, returning nOutSizeInChars.
	// Either way we need to return an empty string rather than a bogus and possibly not
	// null-terminated result.
	if ( result <= 0 || result >= nOutSizeInBytes )
	{
		// If nInSize includes the null-terminator then a result of nOutSizeInChars is
		// legal. We check this by seeing if the last character in the output buffer is
		// a zero.
		if ( result == nOutSizeInBytes && pWString[ nOutSizeInBytes - 1 ] == 0)
		{
			// We're okay! Do nothing.
		}
		else
		{
			*pString = '\0';
		}
	}
	else
	{
		// We have successfully converted our string. Now we need to null-terminate it, because
		// MultiByteToWideChar will only do that if nInSize includes the source null-terminator!
		pString[ result ] = '\0';
	}
#elif POSIX
	if ( wcstombs( pString, pWString, nOutSizeInBytes ) <= 0 )
	{
		*pString = '\0';
	}
#endif
}



//--------------------------------------------------------------------------------
// backslashification
//--------------------------------------------------------------------------------

static constexpr char s_BackSlashMap[]="\tt\nn\rr\"\"\\\\";

char *V_AddBackSlashesToSpecialChars( IN_Z const char *pSrc )
{
	// first, count how much space we are going to need
	intp nSpaceNeeded = 0;
	for( char const *pScan = pSrc; *pScan; pScan++ )
	{
		nSpaceNeeded++;
		for(char const *pCharSet=s_BackSlashMap; *pCharSet; pCharSet += 2 )
		{
			if ( *pCharSet == *pScan )
				nSpaceNeeded++;								// we need to store a bakslash
		}
	}
	char *pRet = new char[ nSpaceNeeded + 1 ];				// +1 for null
	char *pOut = pRet;

	if (!pOut) return pOut;
	
	for( char const *pScan = pSrc; *pScan; pScan++ )
	{
		bool bIsSpecial = false;
		for(char const *pCharSet=s_BackSlashMap; *pCharSet; pCharSet += 2 )
		{
			if ( *pCharSet == *pScan )
			{
				*( pOut++ ) = '\\';
				*( pOut++ ) = pCharSet[1];
				bIsSpecial = true;
				break;
			}
		}
		if (! bIsSpecial )
		{
			*( pOut++ ) = *pScan;
		}
	}
	*( pOut++ ) = 0;
	return pRet;
}

//-----------------------------------------------------------------------------
// Purpose: Helper for converting a numeric value to a hex digit, value should be 0-15.
//-----------------------------------------------------------------------------
static constexpr char cIntToHexDigit( int nValue )
{
	Assert( nValue >= 0 && nValue <= 15 );
	return "0123456789ABCDEF"[ nValue & 15 ];
}

//-----------------------------------------------------------------------------
// Purpose: Helper for converting a hex char value to numeric, return -1 if the char
//          is not a valid hex digit.
//-----------------------------------------------------------------------------
static constexpr int iHexCharToInt( char cValue )
{
	int32 iValue = cValue;

	const int32 zero = iValue - '0';
	if ( (uint32)zero < 10 ) return zero;

	iValue |= 0x20;

	const int32 a = iValue - 'a';
	if ( (uint32)a < 6 ) return a + 10;

	return -1;
}

//-----------------------------------------------------------------------------
// Purpose: Internal implementation of encode, works in the strict RFC manner, or
//          with spaces turned to + like HTML form encoding.
//-----------------------------------------------------------------------------
void Q_URLEncodeInternal( char *pchDest, intp nDestLen, const char *pchSource, intp nSourceLen, bool bUsePlusForSpace )
{
	if ( nDestLen < 3*nSourceLen )
	{
		pchDest[0] = '\0';
		AssertMsg( false, "Target buffer for Q_URLEncode needs to be 3 times larger than source to guarantee enough space\n" );
		return;
	}

	intp iDestPos = 0;
	for ( intp i=0; i < nSourceLen; ++i )
	{
		// We allow only a-z, A-Z, 0-9, period, underscore, and hyphen to pass through unescaped.
		// These are the characters allowed by both the original RFC 1738 and the latest RFC 3986.
		// Current specs also allow '~', but that is forbidden under original RFC 1738.
		if ( !( pchSource[i] >= 'a' && pchSource[i] <= 'z' ) && !( pchSource[i] >= 'A' && pchSource[i] <= 'Z' ) && !(pchSource[i] >= '0' && pchSource[i] <= '9' )
			&& pchSource[i] != '-' && pchSource[i] != '_' && pchSource[i] != '.'	
			)
		{
			if ( bUsePlusForSpace && pchSource[i] == ' ' )
			{
				pchDest[iDestPos++] = '+';
			}
			else
			{
				pchDest[iDestPos++] = '%';
				uint8 iValue = pchSource[i];
				if ( iValue == 0 )
				{
					pchDest[iDestPos++] = '0';
					pchDest[iDestPos++] = '0';
				}
				else
				{
					char cHexDigit1 = cIntToHexDigit( iValue % 16 );
					iValue /= 16;
					char cHexDigit2 = cIntToHexDigit( iValue );
					pchDest[iDestPos++] = cHexDigit2;
					pchDest[iDestPos++] = cHexDigit1;
				}
			}
		}
		else
		{
			pchDest[iDestPos++] = pchSource[i];
		}
	}

	// Null terminate
	pchDest[iDestPos++] = 0;
}


//-----------------------------------------------------------------------------
// Purpose: Internal implementation of decode, works in the strict RFC manner, or
//          with spaces turned to + like HTML form encoding.
//
//			Returns the amount of space used in the output buffer.
//-----------------------------------------------------------------------------
size_t Q_URLDecodeInternal( char *pchDecodeDest, intp nDecodeDestLen, const char *pchEncodedSource, intp nEncodedSourceLen, bool bUsePlusForSpace )
{
	if ( nDecodeDestLen < nEncodedSourceLen )
	{
		AssertMsg( false, "Q_URLDecode needs a dest buffer at least as large as the source" );
		return 0;
	}

	intp iDestPos = 0;
	for( intp i=0; i < nEncodedSourceLen; ++i )
	{
		if ( bUsePlusForSpace && pchEncodedSource[i] == '+' )
		{
			pchDecodeDest[ iDestPos++ ] = ' ';
		}
		else if ( pchEncodedSource[i] == '%' )
		{
			// Percent signifies an encoded value, look ahead for the hex code, convert to numeric, and use that

			// First make sure we have 2 more chars
			if ( i < nEncodedSourceLen - 2 )
			{
				char cHexDigit1 = pchEncodedSource[i+1];
				char cHexDigit2 = pchEncodedSource[i+2];

				// Turn the chars into a hex value, if they are not valid, then we'll
				// just place the % and the following two chars direct into the string,
				// even though this really shouldn't happen, who knows what bad clients
				// may do with encoding.
				bool bValid = false;
				int iValue = iHexCharToInt( cHexDigit1 );
				if ( iValue != -1 )
				{
					iValue *= 16;
					int iValue2 = iHexCharToInt( cHexDigit2 );
					if ( iValue2 != -1 )
					{
						iValue += iValue2;
						pchDecodeDest[ iDestPos++ ] = iValue;
						bValid = true;
					}
				}

				if ( !bValid )
				{
					pchDecodeDest[ iDestPos++ ] = '%';
					pchDecodeDest[ iDestPos++ ] = cHexDigit1;
					pchDecodeDest[ iDestPos++ ] = cHexDigit2;
				}
			}

			// Skip ahead
			i += 2;
		}
		else
		{
			pchDecodeDest[ iDestPos++ ] = pchEncodedSource[i];
		}
	}

	// We may not have extra room to NULL terminate, since this can be used on raw data, but if we do
	// go ahead and do it as this can avoid bugs.
	if ( iDestPos < nDecodeDestLen )
	{
		pchDecodeDest[iDestPos] = 0;
	}

	return (size_t)iDestPos;
}

//-----------------------------------------------------------------------------
// Purpose: Encodes a string (or binary data) from URL encoding format, see rfc1738 section 2.2.  
//          This version of the call isn't a strict RFC implementation, but uses + for space as is
//          the standard in HTML form encoding, despite it not being part of the RFC.
//
//          Dest buffer should be at least as large as source buffer to guarantee room for decode.
//-----------------------------------------------------------------------------
void Q_URLEncode( OUT_Z_CAP(nDestLen) char *pchDest, intp nDestLen, IN_Z_CAP(nSourceLen) const char *pchSource, intp nSourceLen )
{
	return Q_URLEncodeInternal( pchDest, nDestLen, pchSource, nSourceLen, true );
}


//-----------------------------------------------------------------------------
// Purpose: Decodes a string (or binary data) from URL encoding format, see rfc1738 section 2.2.  
//          This version of the call isn't a strict RFC implementation, but uses + for space as is
//          the standard in HTML form encoding, despite it not being part of the RFC.
//
//          Dest buffer should be at least as large as source buffer to guarantee room for decode.
//			Dest buffer being the same as the source buffer (decode in-place) is explicitly allowed.
//-----------------------------------------------------------------------------
size_t Q_URLDecode( OUT_CAP(nDecodeDestLen) char *pchDecodeDest, intp nDecodeDestLen, IN_Z_CAP(nEncodedSourceLen) const char *pchEncodedSource, intp nEncodedSourceLen )
{
	return Q_URLDecodeInternal( pchDecodeDest, nDecodeDestLen, pchEncodedSource, nEncodedSourceLen, true );
}


//-----------------------------------------------------------------------------
// Purpose: Encodes a string (or binary data) from URL encoding format, see rfc1738 section 2.2.  
//          This version will not encode space as + (which HTML form encoding uses despite not being part of the RFC)
//
//          Dest buffer should be at least as large as source buffer to guarantee room for decode.
//-----------------------------------------------------------------------------
void Q_URLEncodeRaw( OUT_Z_CAP(nDestLen) char *pchDest, intp nDestLen, IN_Z_CAP(nSourceLen) const char *pchSource, intp nSourceLen )
{
	return Q_URLEncodeInternal( pchDest, nDestLen, pchSource, nSourceLen, false );
}


//-----------------------------------------------------------------------------
// Purpose: Decodes a string (or binary data) from URL encoding format, see rfc1738 section 2.2.  
//          This version will not recognize + as a space (which HTML form encoding uses despite not being part of the RFC)
//
//          Dest buffer should be at least as large as source buffer to guarantee room for decode.
//			Dest buffer being the same as the source buffer (decode in-place) is explicitly allowed.
//-----------------------------------------------------------------------------
size_t Q_URLDecodeRaw( OUT_Z_CAP(nDecodeDestLen) char *pchDecodeDest, intp nDecodeDestLen, IN_Z_CAP(nEncodedSourceLen) const char *pchEncodedSource, intp nEncodedSourceLen )
{
	return Q_URLDecodeInternal( pchDecodeDest, nDecodeDestLen, pchEncodedSource, nEncodedSourceLen, false );
}

#if defined( LINUX ) || defined( _PS3 )
extern "C" void qsort_s( void *base, size_t num, size_t width, int (*compare )(void *, const void *, const void *), void * context );
#endif

void V_qsort_s( INOUT_BYTECAP(num) void *base, size_t num, size_t width, int ( __cdecl *compare )(void *, const void *, const void *), void * context ) 
{
#if defined OSX
	// the arguments are swapped 'round on the mac - awesome, huh?
	return qsort_r( base, num, width, context, compare );
#else
	return qsort_s( base, num, width, compare, context );
#endif
}

//-----------------------------------------------------------------------------
// Purpose: format the time and/or date with the user's current locale
// If timeVal is 0, gets the current time
//
// This is generally for use with chatroom dialogs, etc. which need to be
// able to say "Last message received: %date% at %time%"
//
// Note that this uses time_t because RTime32 is not hooked-up on the client
//-----------------------------------------------------------------------------
bool BGetLocalFormattedDateAndTime( time_t timeVal, char *pchDate, int cubDate, char *pchTime, int cubTime )
{
	if ( 0 == timeVal || timeVal < 0 )
	{
		// get the current time
		time( &timeVal );
	}

	if ( timeVal )
	{
		// Convert it to our local time
		struct tm tmStruct;
		struct tm tmToDisplay = *( Plat_localtime( ( const time_t* )&timeVal, &tmStruct ) );
#ifdef POSIX
		if ( pchDate != NULL )
		{
			pchDate[ 0 ] = 0;
			if ( 0 == strftime( pchDate, cubDate, "%A %b %d", &tmToDisplay ) )
				return false;
		}

		if ( pchTime != NULL )
		{
			pchTime[ 0 ] = 0;
			if ( 0 == strftime( pchTime, cubTime - 6, "%I:%M ", &tmToDisplay ) )
				return false;

			// append am/pm in lower case (since strftime doesn't have a lowercase formatting option)
			if (tmToDisplay.tm_hour >= 12)
			{
				V_strcat( pchTime, "p.m.", cubTime );
			}
			else
			{
				V_strcat( pchTime, "a.m.", cubTime );
			}
		}
#else // WINDOWS
		// convert time_t to a SYSTEMTIME
		SYSTEMTIME st;
		st.wHour = static_cast<WORD>(tmToDisplay.tm_hour);
		st.wMinute = static_cast<WORD>(tmToDisplay.tm_min);
		st.wSecond = static_cast<WORD>(tmToDisplay.tm_sec);
		st.wDay = static_cast<WORD>(tmToDisplay.tm_mday);
		st.wMonth = static_cast<WORD>(tmToDisplay.tm_mon + 1);
		st.wYear = static_cast<WORD>(tmToDisplay.tm_year + 1900);
		st.wDayOfWeek = static_cast<WORD>(tmToDisplay.tm_wday);
		st.wMilliseconds = 0;

		WCHAR rgwch[ MAX_PATH ];

		if ( pchDate != NULL )
		{
			pchDate[ 0 ] = 0;
			if ( !GetDateFormatW( LOCALE_USER_DEFAULT, DATE_LONGDATE, &st, NULL, rgwch, MAX_PATH ) )
				return false;
			Q_strncpy( pchDate, CStrAutoEncode( rgwch ).ToString(), cubDate );
		}

		if ( pchTime != NULL )
		{
			pchTime[ 0 ] = 0;
			if ( !GetTimeFormatW( LOCALE_USER_DEFAULT, TIME_NOSECONDS, &st, NULL, rgwch, MAX_PATH ) )
				return false;
			Q_strncpy( pchTime, CStrAutoEncode( rgwch ).ToString(), cubTime );
		}
#endif
		return true;
	}

	return false;
}


// And a couple of helpers so people don't have to remember the order of the parameters in the above function
bool BGetLocalFormattedDate( time_t timeVal, char *pchDate, int cubDate )
{
	return BGetLocalFormattedDateAndTime( timeVal, pchDate, cubDate, NULL, 0 );
}
bool BGetLocalFormattedTime( time_t timeVal, char *pchTime, int cubTime )
{
	return BGetLocalFormattedDateAndTime( timeVal, NULL, 0, pchTime, cubTime );
}

// Prints out a memory dump where stuff that's ascii is human readable, etc.
void V_LogMultiline( bool input, char const *label, const char *data, size_t len, CUtlString &output )
{
	constexpr char HEX[] = "0123456789abcdef";
	const char * direction = (input ? " << " : " >> ");
	constexpr size_t LINE_SIZE = 24;
	char hex_line[LINE_SIZE * 9 / 4 + 2], asc_line[LINE_SIZE + 1];
	while (len > 0) 
	{
		V_memset(asc_line, ' ', sizeof(asc_line));
		V_memset(hex_line, ' ', sizeof(hex_line));
		size_t line_len = MIN(len, LINE_SIZE);
		for (size_t i=0; i<line_len; ++i) {
			unsigned char ch = static_cast<unsigned char>(data[i]);
			asc_line[i] = ( V_isprint(ch) && !V_iscntrl(ch) ) ? data[i] : '.';
			hex_line[i*2 + i/4] = HEX[ch >> 4];
			hex_line[i*2 + i/4 + 1] = HEX[ch & 0xf];
		}
		asc_line[sizeof(asc_line)-1] = 0;
		hex_line[sizeof(hex_line)-1] = 0;
		output += CFmtStr( "%s %s %s %s\n", label, direction, asc_line, hex_line );
		data += line_len;
		len -= line_len;
	}
}


#ifdef WIN32
// Win32 CRT doesn't support the full range of UChar32, has no extended planes
inline int V_iswspace( int c ) { return ( c <= 0xFFFF ) ? iswspace( (wint_t)c ) : 0; }
#else
#define V_iswspace(x) iswspace(x)
#endif


//-----------------------------------------------------------------------------
// Purpose: Slightly modified strtok. Does not modify the input string. Does
//			not skip over more than one separator at a time. This allows parsing
//			strings where tokens between separators may or may not be present:
//
//			Door01,,,0 would be parsed as "Door01"  ""  ""  "0"
//			Door01,Open,,0 would be parsed as "Door01"  "Open"  ""  "0"
//
// Input  : token - Returns with a token, or zero length if the token was missing.
//			str - String to parse.
//			sep - Character to use as separator. UNDONE: allow multiple separator chars
// Output : Returns a pointer to the next token to be parsed.
//-----------------------------------------------------------------------------
RET_MAY_BE_NULL const char *nexttoken(OUT_Z_CAP(nMaxTokenLen) char *token, size_t nMaxTokenLen, IN_OPT_Z const char *str, char sep)
{
	if (nMaxTokenLen < 1)
	{
		Assert(nMaxTokenLen > 0);
		return NULL;
	}

	if ((str == NULL) || (*str == '\0'))
	{
		*token = '\0';
		return NULL;
	}

	char *pTokenLast = token + nMaxTokenLen - 1;

	//
	// Copy everything up to the first separator into the return buffer.
	// Do not include separators in the return buffer.
	//
	while ((*str != sep) && (*str != '\0') && (token < pTokenLast))
	{
		*token++ = *str++;
	}
	*token = '\0';

	//
	// Advance the pointer unless we hit the end of the input string.
	//
	if (*str == '\0')
	{
		return str;
	}

	return ++str;
}

intp V_StrTrim( IN_Z char *pStr )
{
	Assert(pStr);

	char *pSource = pStr;
	char *pDest = pStr;

	// skip white space at the beginning
	while ( *pSource != 0 && V_isspace( *pSource ) )
	{
		pSource++;
	}

	// copy everything else
	char *pLastWhiteBlock = NULL;
	char *pStart = pDest;
	while ( *pSource != 0 )
	{
		*pDest = *pSource++;
		if ( V_isspace( *pDest ) )
		{
			if ( pLastWhiteBlock == NULL )
				pLastWhiteBlock = pDest;
		}
		else
		{
			pLastWhiteBlock = NULL;
		}
		pDest++;
	}
	*pDest = 0;

	// did we end in a whitespace block?
	if ( pLastWhiteBlock != NULL )
	{
		// yep; shorten the string
		pDest = pLastWhiteBlock;
		*pLastWhiteBlock = 0;
	}

	return pDest - pStart;
}

#ifdef _WIN32
int64 V_strtoi64( IN_Z const char *nptr, char **endptr, int base )
{
	return _strtoi64( nptr, endptr, base );
}

uint64 V_strtoui64( IN_Z const char *nptr, char **endptr, int base )
{
	return _strtoui64( nptr, endptr, base );
}
#elif POSIX
int64 V_strtoi64( const char *nptr, char **endptr, int base )
{
	return strtoll( nptr, endptr, base );
}

uint64 V_strtoui64( const char *nptr, char **endptr, int base )
{
	return strtoull( nptr, endptr, base );
}
#endif


struct HtmlEntity_t
{
	unsigned short uCharCode;
	const char *pchEntity;
	intp nEntityLength;
};

constexpr static HtmlEntity_t g_BasicHTMLEntities[] = {
		{ '"', "&quot;", 6 },
		{ '\'', "&#039;", 6 },
		{ '<', "&lt;", 4 },
		{ '>', "&gt;", 4 },
		{ '&', "&amp;", 5 },
		{ 0, NULL, 0 } // sentinel for end of array
};

constexpr static HtmlEntity_t g_WhitespaceEntities[] = {
		{ ' ', "&nbsp;", 6 },
		{ '\n', "<br>", 4 },
		{ 0, NULL, 0 } // sentinel for end of array
};


struct Tier1FullHTMLEntity_t
{
	uchar32 uCharCode;
	const char *pchEntity;
	intp nEntityLength;
};


const Tier1FullHTMLEntity_t g_Tier1_FullHTMLEntities[] =
{
	{ L'"', "&quot;", 6 },
	{ L'\'', "&apos;", 6 },
	{ L'&', "&amp;", 5 },
	{ L'<', "&lt;", 4 },
	{ L'>', "&gt;", 4 },
	{ L' ', "&nbsp;", 6 },
	{ L'\u2122', "&trade;", 7 },
	{ L'\u00A9', "&copy;", 6 },
	{ L'\u00AE', "&reg;", 5 },
	{ L'\u2013', "&ndash;", 7 },
	{ L'\u2014', "&mdash;", 7 },
	{ L'\u20AC', "&euro;", 6 },
	{ L'\u00A1', "&iexcl;", 7 },
	{ L'\u00A2', "&cent;", 6 },
	{ L'\u00A3', "&pound;", 7 },
	{ L'\u00A4', "&curren;", 8 },
	{ L'\u00A5', "&yen;", 5 },
	{ L'\u00A6', "&brvbar;", 8 },
	{ L'\u00A7', "&sect;", 6 },
	{ L'\u00A8', "&uml;", 5 },
	{ L'\u00AA', "&ordf;", 6 },
	{ L'\u00AB', "&laquo;", 7 },
	{ L'\u00AC', "&not;", 8 },
	{ L'\u00AD', "&shy;", 5 },
	{ L'\u00AF', "&macr;", 6 },
	{ L'\u00B0', "&deg;", 5 },
	{ L'\u00B1', "&plusmn;", 8 },
	{ L'\u00B2', "&sup2;", 6 },
	{ L'\u00B3', "&sup3;", 6 },
	{ L'\u00B4', "&acute;", 7 },
	{ L'\u00B5', "&micro;", 7 },
	{ L'\u00B6', "&para;", 6 },
	{ L'\u00B7', "&middot;", 8 },
	{ L'\u00B8', "&cedil;", 7 },
	{ L'\u00B9', "&sup1;", 6 },
	{ L'\u00BA', "&ordm;", 6 },
	{ L'\u00BB', "&raquo;", 7 },
	{ L'\u00BC', "&frac14;", 8 },
	{ L'\u00BD', "&frac12;", 8 },
	{ L'\u00BE', "&frac34;", 8 },
	{ L'\u00BF', "&iquest;", 8 },
	{ L'\u00D7', "&times;", 7 },
	{ L'\u00F7', "&divide;", 8 },
	{ L'\u00C0', "&Agrave;", 8 },
	{ L'\u00C1', "&Aacute;", 8 },
	{ L'\u00C2', "&Acirc;", 7 },
	{ L'\u00C3', "&Atilde;", 8 },
	{ L'\u00C4', "&Auml;", 6 },
	{ L'\u00C5', "&Aring;", 7 },
	{ L'\u00C6', "&AElig;", 7 },
	{ L'\u00C7', "&Ccedil;", 8 },
	{ L'\u00C8', "&Egrave;", 8 },
	{ L'\u00C9', "&Eacute;", 8 },
	{ L'\u00CA', "&Ecirc;", 7 },
	{ L'\u00CB', "&Euml;", 6 },
	{ L'\u00CC', "&Igrave;", 8 },
	{ L'\u00CD', "&Iacute;", 8 },
	{ L'\u00CE', "&Icirc;", 7 },
	{ L'\u00CF', "&Iuml;", 6 },
	{ L'\u00D0', "&ETH;", 5 },
	{ L'\u00D1', "&Ntilde;", 8 },
	{ L'\u00D2', "&Ograve;", 8 },
	{ L'\u00D3', "&Oacute;", 8 },
	{ L'\u00D4', "&Ocirc;", 7 },
	{ L'\u00D5', "&Otilde;", 8 },
	{ L'\u00D6', "&Ouml;", 6 },
	{ L'\u00D8', "&Oslash;", 8 },
	{ L'\u00D9', "&Ugrave;", 8 },
	{ L'\u00DA', "&Uacute;", 8 },
	{ L'\u00DB', "&Ucirc;", 7 },
	{ L'\u00DC', "&Uuml;", 6 },
	{ L'\u00DD', "&Yacute;", 8 },
	{ L'\u00DE', "&THORN;", 7 },
	{ L'\u00DF', "&szlig;", 7 },
	{ L'\u00E0', "&agrave;", 8 },
	{ L'\u00E1', "&aacute;", 8 },
	{ L'\u00E2', "&acirc;", 7 },
	{ L'\u00E3', "&atilde;", 8 },
	{ L'\u00E4', "&auml;", 6 },
	{ L'\u00E5', "&aring;", 7 },
	{ L'\u00E6', "&aelig;", 7 },
	{ L'\u00E7', "&ccedil;", 8 },
	{ L'\u00E8', "&egrave;", 8 },
	{ L'\u00E9', "&eacute;", 8 },
	{ L'\u00EA', "&ecirc;", 7 },
	{ L'\u00EB', "&euml;", 6 },
	{ L'\u00EC', "&igrave;", 8 },
	{ L'\u00ED', "&iacute;", 8 },
	{ L'\u00EE', "&icirc;", 7 },
	{ L'\u00EF', "&iuml;", 6 },
	{ L'\u00F0', "&eth;", 5 },
	{ L'\u00F1', "&ntilde;", 8 },
	{ L'\u00F2', "&ograve;", 8 },
	{ L'\u00F3', "&oacute;", 8 },
	{ L'\u00F4', "&ocirc;", 7 },
	{ L'\u00F5', "&otilde;", 8 },
	{ L'\u00F6', "&ouml;", 6 },
	{ L'\u00F8', "&oslash;", 8 },
	{ L'\u00F9', "&ugrave;", 8 },
	{ L'\u00FA', "&uacute;", 8 },
	{ L'\u00FB', "&ucirc;", 7 },
	{ L'\u00FC', "&uuml;", 6 },
	{ L'\u00FD', "&yacute;", 8 },
	{ L'\u00FE', "&thorn;", 7 },
	{ L'\u00FF', "&yuml;", 6 },
	{ 0, NULL, 0 } // sentinel for end of array
};



bool V_BasicHtmlEntityEncode( OUT_Z_CAP( nDestSize ) char *pDest, const intp nDestSize, IN_Z_CAP(nInSize) char const *pIn, const intp nInSize, bool bPreserveWhitespace /*= false*/ )
{
	Assert( nDestSize == 0 || pDest != NULL );
	intp iOutput = 0;
	for ( intp iInput = 0; iInput < nInSize; ++iInput )
	{
		bool bReplacementDone = false;
		// See if the current char matches any of the basic entities
		for ( intp i = 0; g_BasicHTMLEntities[ i ].uCharCode != 0; ++i )
		{
			if ( pIn[ iInput ] == g_BasicHTMLEntities[ i ].uCharCode )
			{
				bReplacementDone = true;
				for ( intp j = 0; j < g_BasicHTMLEntities[ i ].nEntityLength; ++j )
				{
					if ( iOutput >= nDestSize - 1 )
					{
						pDest[ nDestSize - 1 ] = 0;
						return false;
					}
					pDest[ iOutput++ ] = g_BasicHTMLEntities[ i ].pchEntity[ j ];
				}
			}
		}

		if ( bPreserveWhitespace && !bReplacementDone )
		{
			// See if the current char matches any of the basic entities
			for ( intp i = 0; g_WhitespaceEntities[ i ].uCharCode != 0; ++i )
			{
				if ( pIn[ iInput ] == g_WhitespaceEntities[ i ].uCharCode )
				{
					bReplacementDone = true;
					for ( intp j = 0; j < g_WhitespaceEntities[ i ].nEntityLength; ++j )
					{
						if ( iOutput >= nDestSize - 1 )
						{
							pDest[ nDestSize - 1 ] = 0;
							return false;
						}
						pDest[ iOutput++ ] = g_WhitespaceEntities[ i ].pchEntity[ j ];
					}
				}
			}
		}

		if ( !bReplacementDone )
		{
			pDest[ iOutput++ ] = pIn[ iInput ];
		}
	}

	// Null terminate the output
	// dimhotepus: Ensure no overflow.
	pDest[ std::min( iOutput, nDestSize - 1 ) ] = '\0';
	return true;
}


bool V_HtmlEntityDecodeToUTF8( OUT_Z_CAP( nDestSize ) char *pDest, const intp nDestSize, IN_Z_CAP(nInSize) char const *pIn, const intp nInSize )
{
	Assert( nDestSize == 0 || pDest != NULL );
	intp iOutput = 0;
	char rgchReplacement[8];
	for ( intp iInput = 0; iInput < nInSize && iOutput < nDestSize; ++iInput )
	{
		bool bReplacementDone = false;
		if ( pIn[ iInput ] == '&' )
		{
			rgchReplacement[ 0 ] = '\0';
			bReplacementDone = true;

			uchar32 wrgchReplacement[ 2 ] = { 0, 0 };

			const char *pchEnd = Q_strstr( pIn + iInput + 1, ";" );
			if ( pchEnd )
			{
				if ( iInput + 1 < nInSize && pIn[ iInput + 1 ] == '#' )
				{
					// Numeric
					int iBase = 10;
					int iOffset = 2;
					if ( iInput + 3 < nInSize && pIn[ iInput + 2 ] == 'x' )
					{
						iBase = 16;
						iOffset = 3;
					}

					wrgchReplacement[ 0 ] = (uchar32)V_strtoi64( pIn + iInput + iOffset, NULL, iBase );
					if ( !Q_UTF32ToUTF8( wrgchReplacement, rgchReplacement, sizeof( rgchReplacement ) ) )
					{
						rgchReplacement[ 0 ] = 0;
					}
				}
				else
				{
					// Lookup in map
					const Tier1FullHTMLEntity_t *pFullEntities = g_Tier1_FullHTMLEntities;
					for ( intp i = 0; pFullEntities[ i ].uCharCode != 0; ++i )
					{
						if ( nInSize - iInput - 1 >= pFullEntities[ i ].nEntityLength )
						{
							if ( Q_memcmp( pIn + iInput, pFullEntities[ i ].pchEntity, pFullEntities[ i ].nEntityLength ) == 0 )
							{
								wrgchReplacement[ 0 ] = pFullEntities[ i ].uCharCode;
								if ( !Q_UTF32ToUTF8( wrgchReplacement, rgchReplacement, sizeof( rgchReplacement ) ) )
								{
									rgchReplacement[ 0 ] = 0;
								}
								break;
							}
						}
					}
				}

				// make sure we found a replacement. If not, skip
				intp cchReplacement = V_strlen( rgchReplacement );
				if ( cchReplacement > 0 )
				{
					if ( cchReplacement + iOutput < nDestSize )
					{
						for ( intp i = 0; rgchReplacement[ i ] != 0; ++i )
						{
							pDest[ iOutput++ ] = rgchReplacement[ i ];
						}
					}

					// Skip extra space that we passed
					iInput += pchEnd - ( pIn + iInput );
				}
				else
				{
					bReplacementDone = false;
				}
			}
		}

		if ( !bReplacementDone )
		{
			pDest[ iOutput++ ] = pIn[ iInput ];
		}
	}

	// Null terminate the output
	if ( iOutput < nDestSize )
	{
		pDest[ iOutput ] = 0;
	}
	else
	{
		pDest[ nDestSize - 1 ] = 0;
	}

	return true;
}

static const char * const g_pszSimpleBBCodeReplacements[] = {
	"[b]", "<b>",
	"[/b]", "</b>",
	"[i]", "<i>",
	"[/i]", "</i>",
	"[u]", "<u>",
	"[/u]", "</u>",
	"[s]", "<s>",
	"[/s]", "</s>",
	"[code]", "<pre>",
	"[/code]", "</pre>",
	"[h1]", "<h1>",
	"[/h1]", "</h1>",
	"[list]", "<ul>",
	"[/list]", "</ul>",
	"[*]", "<li>",
	"[/url]", "</a>",
	"[img]", "<img src=\"",
	"[/img]", "\"></img>",
};

// Converts BBCode tags to HTML tags
bool V_BBCodeToHTML( OUT_Z_CAP( nDestSize ) char *pDest, const intp nDestSize, IN_Z_CAP(nInSize) char const *pIn, const intp nInSize )
{
	Assert( nDestSize == 0 || pDest != NULL );
	intp iOutput = 0;

	for ( intp iInput = 0; iInput < nInSize && iOutput < nDestSize && pIn[ iInput ]; ++iInput )
	{
		if ( pIn[ iInput ] == '[' )
		{
			// check simple replacements
			bool bFoundReplacement = false;
			for ( intp r = 0; r < static_cast<intp>(std::size( g_pszSimpleBBCodeReplacements )); r += 2 )
			{
				intp nBBCodeLength = V_strlen( g_pszSimpleBBCodeReplacements[ r ] );
				if ( !V_strnicmp( &pIn[ iInput ], g_pszSimpleBBCodeReplacements[ r ], nBBCodeLength ) )
				{
					intp nHTMLReplacementLength = V_strlen( g_pszSimpleBBCodeReplacements[ r + 1 ] );
					for ( intp c = 0; c < nHTMLReplacementLength && iOutput < nDestSize; c++ )
					{
						pDest[ iOutput ] = g_pszSimpleBBCodeReplacements[ r + 1 ][ c ];
						iOutput++;
					}
					iInput += nBBCodeLength - 1;
					bFoundReplacement = true;
					break;
				}
			}
			// check URL replacement
			if ( !bFoundReplacement && !V_strnicmp( &pIn[ iInput ], "[url=", 5 ) && nDestSize - iOutput > 9 )
			{
				iInput += 5;
				pDest[ iOutput++ ] = '<';
				pDest[ iOutput++ ] = 'a';
				pDest[ iOutput++ ] = ' ';
				pDest[ iOutput++ ] = 'h';
				pDest[ iOutput++ ] = 'r';
				pDest[ iOutput++ ] = 'e';
				pDest[ iOutput++ ] = 'f';
				pDest[ iOutput++ ] = '=';
				pDest[ iOutput++ ] = '\"';

				// copy all characters up to the closing square bracket
				while ( iInput < nInSize && pIn[ iInput ] != ']' && iOutput < nDestSize )
				{
					pDest[ iOutput++ ] = pIn[ iInput++ ];
				}
				if ( pIn[ iInput ] == ']' && nDestSize - iOutput > 2 )
				{
					pDest[ iOutput++ ] = '\"';
					pDest[ iOutput++ ] = '>';
				}
				bFoundReplacement = true;
			}
			// otherwise, skip over everything up to the closing square bracket
			if ( !bFoundReplacement )
			{
				while ( iInput < nInSize && pIn[ iInput ] != ']' )
				{
					iInput++;
				}
			}
		}
		else if ( pIn[ iInput ] == '\r' &&
			      // dimhotepus: Prevent out-of bounds read.
			      iInput < nInSize - 1 &&
			      pIn[ iInput + 1 ] == '\n' )
		{
			// convert carriage return and newline to a <br>
			if ( nDestSize - iOutput > 4 )
			{
				pDest[ iOutput++ ] = '<';
				pDest[ iOutput++ ] = 'b';
				pDest[ iOutput++ ] = 'r';
				pDest[ iOutput++ ] = '>';
			}
			iInput++;
		}
		else if ( pIn[ iInput ] == '\n' )
		{
			// convert newline to a <br>
			if ( nDestSize - iOutput > 4 )
			{
				pDest[ iOutput++ ] = '<';
				pDest[ iOutput++ ] = 'b';
				pDest[ iOutput++ ] = 'r';
				pDest[ iOutput++ ] = '>';
			}
		}
		else
		{
			// copy character to destination
			pDest[ iOutput++ ] = pIn[ iInput ];
		}
	}
	// always terminate string
	if ( iOutput >= nDestSize )
	{
		iOutput = nDestSize - 1;
	}
	pDest[ iOutput ] = 0;
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: returns true if a wide character is a "mean" space; that is,
//			if it is technically a space or punctuation, but causes disruptive
//			behavior when used in names, web pages, chat windows, etc.
//
//			characters in this set are removed from the beginning and/or end of strings
//			by Q_AggressiveStripPrecedingAndTrailingWhitespaceW() 
//-----------------------------------------------------------------------------
bool V_IsMeanUnderscoreW( wchar_t wch )
{
	bool bIsMean = false;

	switch ( wch )
	{
	case L'\x005f':	  // low line (normal underscore)
	case L'\xff3f':	  // fullwidth low line
	case L'\x0332':	  // combining low line
		bIsMean = true;
		break;
	default:
		break;
	}

	return bIsMean;
}


//-----------------------------------------------------------------------------
// Purpose: returns true if a wide character is a "mean" space; that is,
//			if it is technically a space or punctuation, but causes disruptive
//			behavior when used in names, web pages, chat windows, etc.
//
//			characters in this set are removed from the beginning and/or end of strings
//			by Q_AggressiveStripPrecedingAndTrailingWhitespaceW() 
//-----------------------------------------------------------------------------
bool V_IsMeanSpaceW( wchar_t wch )
{
	bool bIsMean = false;

	switch ( wch )
	{
	case L'\x0080':	  // PADDING CHARACTER
	case L'\x0081':	  // HIGH OCTET PRESET
	case L'\x0082':	  // BREAK PERMITTED HERE
	case L'\x0083':	  // NO BREAK PERMITTED HERE
	case L'\x0084':	  // INDEX
	case L'\x0085':	  // NEXT LINE
	case L'\x0086':	  // START OF SELECTED AREA
	case L'\x0087':	  // END OF SELECTED AREA
	case L'\x0088':	  // CHARACTER TABULATION SET
	case L'\x0089':	  // CHARACTER TABULATION WITH JUSTIFICATION
	case L'\x008A':	  // LINE TABULATION SET
	case L'\x008B':	  // PARTIAL LINE FORWARD
	case L'\x008C':	  // PARTIAL LINE BACKWARD
	case L'\x008D':	  // REVERSE LINE FEED
	case L'\x008E':	  // SINGLE SHIFT 2
	case L'\x008F':	  // SINGLE SHIFT 3
	case L'\x0090':	  // DEVICE CONTROL STRING
	case L'\x0091':	  // PRIVATE USE
	case L'\x0092':	  // PRIVATE USE
	case L'\x0093':	  // SET TRANSMIT STATE
	case L'\x0094':	  // CANCEL CHARACTER
	case L'\x0095':	  // MESSAGE WAITING
	case L'\x0096':	  // START OF PROTECTED AREA
	case L'\x0097':	  // END OF PROTECED AREA
	case L'\x0098':	  // START OF STRING
	case L'\x0099':	  // SINGLE GRAPHIC CHARACTER INTRODUCER
	case L'\x009A':	  // SINGLE CHARACTER INTRODUCER
	case L'\x009B':	  // CONTROL SEQUENCE INTRODUCER
	case L'\x009C':	  // STRING TERMINATOR
	case L'\x009D':	  // OPERATING SYSTEM COMMAND
	case L'\x009E':	  // PRIVACY MESSAGE
	case L'\x009F':	  // APPLICATION PROGRAM COMMAND
	case L'\x00A0':	  // NO-BREAK SPACE
	case L'\x034F':   // COMBINING GRAPHEME JOINER
	case L'\x2000':   // EN QUAD
	case L'\x2001':   // EM QUAD
	case L'\x2002':   // EN SPACE
	case L'\x2003':   // EM SPACE
	case L'\x2004':   // THICK SPACE
	case L'\x2005':   // MID SPACE
	case L'\x2006':   // SIX SPACE
	case L'\x2007':   // figure space
	case L'\x2008':   // PUNCTUATION SPACE
	case L'\x2009':   // THIN SPACE
	case L'\x200A':   // HAIR SPACE
	case L'\x200B':   // ZERO-WIDTH SPACE
	case L'\x200C':   // ZERO-WIDTH NON-JOINER
	case L'\x200D':   // ZERO WIDTH JOINER
	case L'\x2028':   // LINE SEPARATOR
	case L'\x2029':   // PARAGRAPH SEPARATOR
	case L'\x202F':   // NARROW NO-BREAK SPACE
	case L'\x2060':   // word joiner
	case L'\xFEFF':   // ZERO-WIDTH NO BREAK SPACE
	case L'\xFFFC':   // OBJECT REPLACEMENT CHARACTER
		bIsMean = true;
		break;
	}

	return bIsMean;
}


//-----------------------------------------------------------------------------
// Purpose: tell us if a Unicode character is deprecated
//
// See Unicode Technical Report #20: https://www.unicode.org/reports/tr20/
//
// Some characters are difficult or unreliably rendered. These characters eventually
// fell out of the Unicode standard, but are abusable by users. For example,
// setting "RIGHT-TO-LEFT override" without popping or undoing the action causes
// the layout instruction to bleed into following characters in HTML renderings,
// or upset layout calculations in vgui panels.
//
// Many games don't cope with these characters well, and end up providing opportunities
// for griefing others. For example, a user might join a game with a malformed player
// name and it turns out that player name can't be selected or typed into the admin
// console or UI to mute, kick, or ban the disruptive player. 
//
// Ideally, we'd perfectly support these end-to-end but we never realistically will.
// The benefit of doing so far outweighs the cost, anyway.
//-----------------------------------------------------------------------------
bool V_IsDeprecatedW( wchar_t wch )
{
	bool bIsDeprecated = false;

	switch ( wch )
	{
	case L'\x202A':		// LEFT-TO-RIGHT EMBEDDING
	case L'\x202B':		// RIGHT-TO-LEFT EMBEDDING
	case L'\x202C':		// POP DIRECTIONAL FORMATTING
	case L'\x202D':		// LEFT-TO-RIGHT override
	case L'\x202E':		// RIGHT-TO-LEFT override

	case L'\x206A':		// INHIBIT SYMMETRIC SWAPPING
	case L'\x206B':		// ACTIVATE SYMMETRIC SWAPPING
	case L'\x206C':		// INHIBIT ARABIC FORM SHAPING
	case L'\x206D':		// ACTIVATE ARABIC FORM SHAPING
	case L'\x206E':		// NATIONAL DIGIT SHAPES
	case L'\x206F':		// NOMINAL DIGIT SHAPES
		bIsDeprecated = true;
	}

	return bIsDeprecated;
}


//-----------------------------------------------------------------------------
// returns true if the character is allowed in a DNS doman name, false otherwise
//-----------------------------------------------------------------------------
bool V_IsValidDomainNameCharacter( IN_Z const char *pch, IN_OPT intp *pAdvanceBytes )
{
	if ( pAdvanceBytes )
		*pAdvanceBytes = 0;


	// We allow unicode in Domain Names without the an encoding unless it corresponds to 
	// a whitespace or control sequence or something we think is an underscore looking thing.
	// If this character is the start of a UTF-8 sequence, try decoding it.
	unsigned char ch = (unsigned char)*pch;
	if ( ( ch & 0xC0 ) == 0xC0 )
	{
		uchar32 rgch32Buf;
		bool bError = false;
		int iAdvance = Q_UTF8ToUChar32( pch, rgch32Buf, bError );
		if ( bError || iAdvance == 0 )
		{
			// Invalid UTF8 sequence, lets consider that invalid
			return false;
		}

		if ( pAdvanceBytes )
			*pAdvanceBytes = iAdvance;

		if ( iAdvance )
		{
			// Ick. Want uchar32 versions of unicode character classification functions.
			// Really would like Q_IsWhitespace32 and Q_IsNonPrintable32, but this is OK.
			if ( rgch32Buf < 0x10000 && ( V_IsMeanSpaceW( (wchar_t)rgch32Buf ) || V_IsDeprecatedW( (wchar_t)rgch32Buf ) || V_IsMeanUnderscoreW( (wchar_t)rgch32Buf ) ) )
			{
				return false;
			}

			return true;
		}
		else
		{
			// Unreachable but would be invalid utf8
			return false;
		}
	}
	else
	{
		// Was not unicode
		if ( pAdvanceBytes )
			*pAdvanceBytes = 1;

		// The only allowable non-unicode chars are a-z A-Z 0-9 and -
		if ( ( ch >= 'a' && ch <= 'z' ) || ( ch >= 'A' && ch <= 'Z' ) || ( ch >= '0' && ch <= '9' ) || ch == '-' || ch == '.' )
			return true;

		return false;
	}
}


//-----------------------------------------------------------------------------
// returns true if the character is allowed in a URL, false otherwise
//-----------------------------------------------------------------------------
bool V_IsValidURLCharacter( IN_Z const char *pch, IN_OPT intp *pAdvanceBytes )
{
	if ( pAdvanceBytes )
		*pAdvanceBytes = 0;


	// We allow unicode in URLs unless it corresponds to a whitespace or control sequence.
	// If this character is the start of a UTF-8 sequence, try decoding it.
	unsigned char ch = (unsigned char)*pch;
	if ( ( ch & 0xC0 ) == 0xC0 )
	{
		uchar32 rgch32Buf;
		bool bError = false;
		int iAdvance = Q_UTF8ToUChar32( pch, rgch32Buf, bError );
		if ( bError || iAdvance == 0 )
		{
			// Invalid UTF8 sequence, lets consider that invalid
			return false;
		}

		if ( pAdvanceBytes )
			*pAdvanceBytes = iAdvance;

		if ( iAdvance )
		{
			// Ick. Want uchar32 versions of unicode character classification functions.
			// Really would like Q_IsWhitespace32 and Q_IsNonPrintable32, but this is OK.
			if ( rgch32Buf < 0x10000 && ( V_IsMeanSpaceW( (wchar_t)rgch32Buf ) || V_IsDeprecatedW( (wchar_t)rgch32Buf ) ) )
			{
				return false;
			}

			return true;
		}
		else
		{
			// Unreachable but would be invalid utf8
			return false;
		}
	}
	else
	{
		// Was not unicode
		if ( pAdvanceBytes )
			*pAdvanceBytes = 1;

		// Spaces, control characters, quotes, and angle brackets are not legal URL characters.
		if ( ch <= 32 || ch == 127 || ch == '"' || ch == '<' || ch == '>' )
			return false;

		return true;
	}

}


//-----------------------------------------------------------------------------
// Purpose: helper function to get a domain from a url
//			Checks both standard url and steam://openurl/<url>
//-----------------------------------------------------------------------------
bool V_ExtractDomainFromURL( IN_Z const char *pchURL, OUT_Z_CAP( cchDomain ) char *pchDomain, intp cchDomain )
{
	pchDomain[ 0 ] = 0;

	constexpr char k_pchSteamOpenUrl[] = "steam://openurl/";
	constexpr char k_pchSteamOpenUrlExt[] = "steam://openurl_external/";

	const char *pchOpenUrlSuffix = StringAfterPrefix( pchURL, k_pchSteamOpenUrl );
	if ( pchOpenUrlSuffix == NULL )
		pchOpenUrlSuffix = StringAfterPrefix( pchURL, k_pchSteamOpenUrlExt );

	if ( pchOpenUrlSuffix )
		pchURL = pchOpenUrlSuffix;

	if ( !pchURL || pchURL[ 0 ] == '\0' )
		return false;

	const char *pchDoubleSlash = strstr( pchURL, "//" );

	// Put the domain and everything after into pchDomain.
	// We'll find where to terminate it later.
	if ( pchDoubleSlash )
	{
		// Skip the slashes
		pchDoubleSlash += 2;

		// If that's all there was, then there's no domain here. Bail.
		if ( *pchDoubleSlash == '\0' )
		{
			return false;
		}

		// Skip any extra slashes
		// ex: http:///steamcommunity.com/
		while ( *pchDoubleSlash == '/' )
		{
			pchDoubleSlash++;
		}

		Q_strncpy( pchDomain, pchDoubleSlash, cchDomain );
	}
	else
	{
		// No double slash, so pchURL has no protocol.
		Q_strncpy( pchDomain, pchURL, cchDomain );
	}

	// First character has to be valid
	if ( *pchDomain == '?' || *pchDomain == '\0' )
	{
		return false;
	}

	// terminate the domain after the first non domain char
	intp iAdvance = 0;
	intp iStrLen = 0;
	char cLast = 0;
	while ( pchDomain[ iStrLen ] )
	{
		if ( !V_IsValidDomainNameCharacter( pchDomain + iStrLen, &iAdvance ) || ( pchDomain[ iStrLen ] == '.' && cLast == '.' ) )
		{
			pchDomain[ iStrLen ] = 0;
			break;
		}

		cLast = pchDomain[ iStrLen ];
		iStrLen += iAdvance;
	}

	return ( pchDomain[ 0 ] != 0 );
}


//-----------------------------------------------------------------------------
// Purpose: helper function to get a domain from a url
//-----------------------------------------------------------------------------
bool V_URLContainsDomain( IN_Z const char *pchURL, IN_Z const char *pchDomain )
{
	char rgchExtractedDomain[ 2048 ];
	if ( V_ExtractDomainFromURL( pchURL, rgchExtractedDomain, sizeof( rgchExtractedDomain ) ) )
	{
		// see if the last part of the domain matches what we extracted
		size_t cchExtractedDomain = strlen( rgchExtractedDomain );
		if ( pchDomain[ 0 ] == '.' )
		{
			++pchDomain;		// If the domain has a leading '.', skip it. The test below assumes there is none.
		}
		size_t cchDomain = strlen( pchDomain );

		if ( cchDomain > cchExtractedDomain )
		{
			return false;
		}
		else if ( cchExtractedDomain >= cchDomain )
		{
			// If the actual domain is longer than what we're searching for, the character previous
			// to the domain we're searching for must be a period
			if ( cchExtractedDomain > cchDomain && rgchExtractedDomain[ cchExtractedDomain - cchDomain - 1 ] != '.' )
				return false;

			if ( 0 == V_stricmp( rgchExtractedDomain + cchExtractedDomain - cchDomain, pchDomain ) )
				return true;
		}
	}
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Strips all HTML tags not specified in rgszPreserveTags
//			Does some additional formatting, like turning <li> into * when not preserving that tag,
//          and auto-closing unclosed tags if they aren't specified in rgszNoCloseTags
//-----------------------------------------------------------------------------
void V_StripAndPreserveHTMLCore( CUtlBuffer *pbuffer, IN_Z const char *pchHTML, const char **rgszPreserveTags, size_t cPreserveTags, const char **rgszNoCloseTags, size_t cNoCloseTags, size_t cMaxResultSize )
{
	size_t cHTMLCur = 0;

	bool bStripNewLines = true;
	if ( cPreserveTags > 0 )
	{
		for ( size_t i = 0; i < cPreserveTags; ++i )
		{
			if ( !Q_stricmp( rgszPreserveTags[ i ], "\n" ) )
				bStripNewLines = false;
		}
	}

	//state-
	bool bInStrippedTag = false;
	bool bInStrippedContentTag = false;
	bool bInPreservedTag = false;
	bool bInListItemTag = false;
	bool bLastCharWasWhitespace = true; //set to true to strip leading whitespace
	bool bInComment = false;
	bool bInDoubleQuote = false;
	bool bInSingleQuote = false;
	intp nPreTagDepth = 0;
	CUtlVector< const char* > vecTagStack;

	for ( intp iContents = 0; pchHTML[ iContents ] != '\0' && cHTMLCur < cMaxResultSize; iContents++ )
	{
		char c = pchHTML[ iContents ];

		// If we are entering a comment, flag as such and skip past the begin comment tag
		const char *pchCur = &pchHTML[ iContents ];
		if ( !Q_strnicmp( pchCur, "<!--", 4 ) )
		{
			bInComment = true;
			iContents += 3;
			continue;
		}

		// If we are in a comment, check if we are exiting
		if ( bInComment )
		{
			if ( !Q_strnicmp( pchCur, "-->", 3 ) )
			{
				bInComment = false;
				iContents += 2;
				continue;
			}
			else
			{
				continue;
			}
		}

		if ( bInStrippedTag || bInPreservedTag )
		{
			// we're inside a tag, keep stripping/preserving until we get to a >
			if ( bInPreservedTag )
				pbuffer->PutChar( c );

			// While inside a tag, ignore ending > properties if they are inside a property value in "" or ''
			if ( c == '"' )
			{
				if ( bInDoubleQuote )
					bInDoubleQuote = false;
				else
					bInDoubleQuote = true;
			}

			if ( c == '\'' )
			{
				if ( bInSingleQuote )
					bInSingleQuote = false;
				else
					bInSingleQuote = true;
			}

			if ( !bInDoubleQuote && !bInSingleQuote && c == '>' )
			{
				if ( bInPreservedTag )
					bLastCharWasWhitespace = false;

				bInPreservedTag = false;
				bInStrippedTag = false;
			}
		}
		else if ( bInStrippedContentTag )
		{
			if ( c == '<' && !Q_strnicmp( pchCur, "</script>", 9 ) )
			{
				bInStrippedContentTag = false;
				iContents += 8;
				continue;
			}
			else
			{
				continue;
			}
		}
		else if ( c & 0x80 && !bInStrippedContentTag )
		{
			// start/continuation of a multibyte sequence, copy to output.
			int nMultibyteRemaining = 0;
			if ( ( c & 0xF8 ) == 0xF0 )	// first 5 bits are 11110
				nMultibyteRemaining = 3;
			else if ( ( c & 0xF0 ) == 0xE0 ) // first 4 bits are 1110
				nMultibyteRemaining = 2;
			else if ( ( c & 0xE0 ) == 0xC0 ) // first 3 bits are 110
				nMultibyteRemaining = 1;

			// cHTMLCur is in characters, so just +1
			cHTMLCur++;
			pbuffer->Put( pchCur, 1 + nMultibyteRemaining );

			iContents += nMultibyteRemaining;

			// Need to determine if we just added whitespace or not
			wchar_t rgwch[ 3 ] = { 0 };
			Q_UTF8CharsToWString( pchCur, 1, rgwch, sizeof( rgwch ) );
			if ( !V_iswspace( rgwch[ 0 ] ) )
				bLastCharWasWhitespace = false;
			else
				bLastCharWasWhitespace = true;
		}
		else
		{
			//not in a multibyte sequence- do our parsing/stripping
			if ( c == '<' )
			{
				if ( !rgszPreserveTags || cPreserveTags == 0 )
				{
					//not preserving any tags, just strip it
					bInStrippedTag = true;
				}
				else
				{
					//look ahead, is this our kind of tag?
					bool bPreserve = false;
					bool bEndTag = false;
					const char *szTagStart = &pchHTML[ iContents + 1 ];
					// if it's a close tag, skip the /
					if ( *szTagStart == '/' )
					{
						bEndTag = true;
						szTagStart++;
					}
					if ( Q_strnicmp( "script", szTagStart, 6 ) == 0 )
					{
						bInStrippedTag = true;
						bInStrippedContentTag = true;
					}
					else
					{
						//see if this tag is one we want to preserve
						for ( size_t iTag = 0; iTag < cPreserveTags; iTag++ )
						{
							const char *szTag = rgszPreserveTags[ iTag ];
							size_t cchTag = strlen( szTag );

							//make sure characters match, and are followed by some non-alnum char 
							//  so "i" can match <i> or <i class=...>, but not <img>
							if ( Q_strnicmp( szTag, szTagStart, cchTag ) == 0 && !V_isalnum( szTagStart[ cchTag ] ) )
							{
								bPreserve = true;
								if ( bEndTag )
								{
									// ending a paragraph tag is optional. If we were expecting to find one, and didn't, skip
									if ( Q_stricmp( szTag, "p" ) != 0 )
									{
										while ( vecTagStack.Count() > 0 && Q_stricmp( vecTagStack[ vecTagStack.Count() - 1 ], "p" ) == 0 )
										{
											vecTagStack.Remove( vecTagStack.Count() - 1 );
										}
									}

									if ( vecTagStack.Count() > 0 && vecTagStack[ vecTagStack.Count() - 1 ] == szTag )
									{
										vecTagStack.Remove( vecTagStack.Count() - 1 );

										if ( Q_stricmp( szTag, "pre" ) == 0 )
										{
											nPreTagDepth--;
											if ( nPreTagDepth < 0 )
											{
												nPreTagDepth = 0;
											}
										}
									}
									else
									{
										// don't preserve this unbalanced tag.  All open tags will be closed at the end of the blurb
										bPreserve = false;
									}
								}
								else
								{
									bool bNoCloseTag = false;
									for ( size_t iNoClose = 0; iNoClose < cNoCloseTags; iNoClose++ )
									{
										if ( Q_stricmp( szTag, rgszNoCloseTags[ iNoClose ] ) == 0 )
										{
											bNoCloseTag = true;
											break;
										}
									}

									if ( !bNoCloseTag )
									{
										vecTagStack.AddToTail( szTag );
										if ( Q_stricmp( szTag, "pre" ) == 0 )
										{
											nPreTagDepth++;
										}
									}
								}
								break;
							}
						}
						if ( !bPreserve )
						{
							bInStrippedTag = true;
						}
						else
						{
							bInPreservedTag = true;
							pbuffer->PutChar( c );
						}

					}
				}
				if ( bInStrippedTag )
				{
					const char *szTagStart = &pchHTML[ iContents ];
					if ( Q_strnicmp( szTagStart, "<li>", ssize( "<li>" ) - 1 ) == 0 )
					{
						if ( bInListItemTag )
						{
							pbuffer->PutChar( ';' );
							cHTMLCur++;
							bInListItemTag = false;
						}

						if ( !bLastCharWasWhitespace )
						{
							pbuffer->PutChar( ' ' );
							cHTMLCur++;
						}

						pbuffer->PutChar( '*' );
						pbuffer->PutChar( ' ' );
						cHTMLCur += 2;
						bInListItemTag = true;
					}
					else if ( !bLastCharWasWhitespace )
					{

						if ( bInListItemTag )
						{
							char cLastChar = ' ';

							if ( pbuffer->TellPut() > 0 )
							{
								cLastChar = ( ( pbuffer->Base<char>() ) + pbuffer->TellPut() - 1 )[ 0 ];
							}
							if ( cLastChar != '.' && cLastChar != '?' && cLastChar != '!' )
							{
								pbuffer->PutChar( ';' );
								cHTMLCur++;
							}
							bInListItemTag = false;
						}

						//we're decided to remove a tag, simulate a space in the original text
						pbuffer->PutChar( ' ' );
						cHTMLCur++;
					}
					bLastCharWasWhitespace = true;
				}
			}
			else
			{
				//just a normal character, nothin' special.
				if ( nPreTagDepth == 0 && V_isspace( c ) && ( bStripNewLines || c != '\n' ) )
				{
					if ( !bLastCharWasWhitespace )
					{
						//replace any block of whitespace with a single space
						cHTMLCur++;
						pbuffer->PutChar( ' ' );
						bLastCharWasWhitespace = true;
					}
					// don't put anything for whitespace if the previous character was whitespace 
					//  (effectively trimming all blocks of whitespace down to a single ' ')
				}
				else
				{
					cHTMLCur++;
					pbuffer->PutChar( c );
					bLastCharWasWhitespace = false;
				}
			}
		}
	}
	if ( cHTMLCur >= cMaxResultSize )
	{
		// we terminated because the blurb was full.  Add a '...' to the end
		pbuffer->Put( "...", 3 );
	}
	//close any preserved tags that were open at the end.
	FOR_EACH_VEC_BACK( vecTagStack, iTagStack )
	{
		pbuffer->PutChar( '<' );
		pbuffer->PutChar( '/' );
		pbuffer->Put( vecTagStack[ iTagStack ], Q_strlen( vecTagStack[ iTagStack ] ) );
		pbuffer->PutChar( '>' );
	}

	// Null terminate
	pbuffer->PutChar( '\0' );
}

//-----------------------------------------------------------------------------
// Purpose: Strips all HTML tags not specified in rgszPreserveTags
//			Does some additional formatting, like turning <li> into * when not preserving that tag
//-----------------------------------------------------------------------------
void V_StripAndPreserveHTML( CUtlBuffer *pbuffer, IN_Z const char *pchHTML, const char **rgszPreserveTags, size_t cPreserveTags, size_t cMaxResultSize )
{
	const char *rgszNoCloseTags[] = { "br", "img" };
	V_StripAndPreserveHTMLCore( pbuffer, pchHTML, rgszPreserveTags, cPreserveTags, rgszNoCloseTags, ssize( rgszNoCloseTags ), cMaxResultSize );
}


