/*
	tinysid.h - v1.0

	SUMMARY:
		Implements a compile-time string hasher via preprocessing.
		Preprocesses an input file and turns all SID( "string" ) instances
		into compile-time hashed integers that look more like:
		
		SID( "string" )
		
		turns into
		
		0x10293858 // "string"

	To use this lib just call tsPreprocess on a file. See the example file at
	https://github.com/RandyGaul/tinyheaders on how to recursively visit and call
	tsPreprocess no all source files in a directory.
*/

#if !defined( TINYSID_H )

// path and out_path can point to the same file, or to different files
int tsPreprocess( const char* path, const char* out_path );

// Feel free to use SID at run-time to hash strings on the spot. Then at
// some point in time (as a pre-build step or even when your application
// is running) preprocess your files to create hard-coded constants.
// The underlying hash function can be swapped out as-needed by modifying
// this source file. Just replace all instances of djb2 with your own
// hash function, along with a matching signature.
#define SID( str ) djb2( str, str + strlen( str ) )
unsigned djb2( char* str, char* end );

#define TINYSID_H
#endif

#ifdef TINYSID_IMPL

#include <stdlib.h> // malloc, free
#include <stdio.h>  // fopen, fseek, ftell, fclose, fwrite, fread
#include <ctype.h>  // isspace

unsigned djb2( char* str, char* end )
{
	unsigned h = 5381;
	int c;

	while ( str != end )
	{
		c = *str;
		h = ((h << 5) + h) + c;
		++str;
	}

	return h;
}

static void tsSkipWhite_internal( char** dataPtr, char** outPtr )
{
	char* data = *dataPtr;
	char* out = *outPtr;

	while ( isspace( *data ) )
		*out++ = *data++;

	*dataPtr = data;
	*outPtr = out;
}

static int tsNext_internal( char** dataPtr, char** outPtr )
{
	char* data = *dataPtr;
	char* out = *outPtr;
	int result = 1;

	while ( 1 )
	{
		if ( !*data )
		{
			result = 0;
			goto return_result;
		}

		tsSkipWhite_internal( &data, &out );

		if ( !*data )
		{
			result = 0;
			goto return_result;
		}

		if ( !isalnum( *data ) )
		{
			*out++ = *data++;
			continue;
		}

		int index = 0;
		int matched = 1;
		const char* match = "SID(";
		while ( 1 )
		{
			if ( !match[ index ] )
				break;

			if ( !*data )
			{
				result = 0;
				goto return_result;
			}

			if ( *data == match[ index ] )
			{
				++data;
				++index;
			}

			else
			{
				matched = 0;
				break;
			}
		}

		data -= index;

		if ( matched )
			break;

		while ( isalnum( *data ) )
			*out++ = *data++;
	}

	return_result:

	*dataPtr = data;
	*outPtr = out;

	return result;
}

int tsPreprocess( const char* path, const char* out_path )
{
	char* data = 0;
	int size = 0;
	FILE* fp = fopen( path, "rb" );

	if ( fp )
	{
		fseek( fp, 0, SEEK_END );
		size = ftell( fp );
		fseek( fp, 0, SEEK_SET );
		data = (char*)malloc( size + 1 );
		fread( data, size, 1, fp );
		fclose( fp );
		data[ size ] = 0;
	}

	else
	{
		printf( "SID ERROR: sid.exe could not open input file %s. Skipping this file.\n", path );
		return 0;
	}

	char* out = (char*)malloc( size * 2 );
	char* outOriginal = out;
	
	int fileWasModified = 0;

	while ( tsNext_internal( &data, &out ) )
	{
		fileWasModified = 1;
		data += 4;
		while ( isspace( *data ) )
			data++;

		// TODO: handle escaped quotes
		if ( *data != '"' )
		{
			printf( "SID WARN (%s): Only strings can be placed inside of the SID macro. Skipping this file.\n", path );
			return 0;
		}

		char* ptr = ++data;

		// TODO: make sure that strings in here aren't over the maximum length
		while ( 1 )
		{
			if ( *ptr == '\\' )
			{
				ptr += 2;
				continue;
			}

			if ( *ptr == '"' )
				break;

			++ptr;
		}

		unsigned h = djb2( data, ptr ); // TODO: detect and report collisions
		int bytes = ptr - data;
		sprintf( out, "0x%.8x /* \"%.*s\" */", h, bytes, data );
		out += 19 + bytes;

		data = ptr + 1;
		while ( isspace( *data ) )
			data++;
		if ( data[ 0 ] != ')' )
		{
			printf( "SID ERROR (%s): Must have ) immediately after the SID macro (look near the string \"%.*s\"). Skipping this file.\n", path, (int)(ptr - data), data );
			return 0;
		}
		data += 1;
	}

	if ( fileWasModified )
	{
		fp = fopen( out_path, "wb" );
		fwrite( outOriginal, out - outOriginal, 1, fp );
		fclose( fp );
	}

	return fileWasModified;
}

#endif

/*
	zlib license:
	
	Copyright (c) 2016 Randy Gaul http://www.randygaul.net
	This software is provided 'as-is', without any express or implied warranty.
	In no event will the authors be held liable for any damages arising from
	the use of this software.
	Permission is granted to anyone to use this software for any purpose,
	including commercial applications, and to alter it and redistribute it
	freely, subject to the following restrictions:
	  1. The origin of this software must not be misrepresented; you must not
	     claim that you wrote the original software. If you use this software
	     in a product, an acknowledgment in the product documentation would be
	     appreciated but is not required.
	  2. Altered source versions must be plainly marked as such, and must not
	     be misrepresented as being the original software.
	  3. This notice may not be removed or altered from any source distribution.
*/
