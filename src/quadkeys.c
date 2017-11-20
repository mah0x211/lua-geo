/*
 *  Copyright (C) 2017 Masatoshi Teruya
 *
 *  Permission is hereby granted, free of charge, to any person obtaining a copy
 *  of this software and associated documentation files (the "Software"), to
 *  deal in the Software without restriction, including without limitation the
 *  rights to use, copy, modify, merge, publish, distribute, sublicense,
 *  and/or sell copies of the Software, and to permit persons to whom the
 *  Software is furnished to do so, subject to the following conditions:
 *
 *  The above copyright notice and this permission notice shall be included in
 *  all copies or substantial portions of the Software.
 *
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
 *  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 *  FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 *  DEALINGS IN THE SOFTWARE.
 *
 *  src/quadkeys.c
 *  lua-geo
 *  Created by Masatoshi Teruya on 17/11/20.
 *
 */

#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <math.h>
// lua
#include "lauxhlib.h"


#define LATITUDE_MIN    -85.05112878
#define LATITUDE_MAX    85.05112878
#define LONGITUDE_MIN   -180
#define LONGITUDE_MAX   180


// Clips a number to the specified minimum and maximum values.
// n: The number to clip.
// minValue: Minimum allowable value.
// maxValue: Maximum allowable value.
// returns: The clipped value.
static inline double getclip( double n, double minValue, double maxValue )
{
    return fmin( fmax( n, minValue ), maxValue );
}


// Determines the map width and height (in pixels) at a specified level
// of detail.
// levelOfDetail: Level of detail, from 1 (lowest detail) to 23 (highest detail).
// returns: The map width and height in pixels.
static inline unsigned int getmapsize( int levelOfDetail )
{
    return 256 << levelOfDetail;
}


// Converts a point from latitude/longitude WGS-84 coordinates (in degrees)
// into pixel XY coordinates at a specified level of detail.
// latitude: latitude of the point, in degrees.
// longitude: longitude of the point, in degrees.
// levelOfDetail: from 1 (lowest detail) to 23 (highest detail)
// pixelX: Output parameter receiving the X coordinate in pixels.
// pixelY: Output parameter receiving the Y coordinate in pixels.
static void LatLongToPixelXY( double latitude, double longitude,
                              int levelOfDetail, int *pixelX, int *pixelY )
{
    latitude = getclip( latitude, LATITUDE_MIN, LATITUDE_MAX );
    longitude = getclip( longitude, LONGITUDE_MIN, LONGITUDE_MAX );

    double x = ( longitude  + 180) / 360;
    double sinLatitude = sin( latitude * M_PI / 180);
    double y = 0.5 - log( ( 1 + sinLatitude ) / ( 1 - sinLatitude ) ) / ( 4 * M_PI );
    unsigned int mapsize = getmapsize( levelOfDetail );

    *pixelX = (int)getclip( x * mapsize + 0.5, 0, mapsize - 1 );
    *pixelY = (int)getclip( y * mapsize + 0.5, 0, mapsize - 1 );
}


// Converts a pixel from pixel XY coordinates at a specified level of detail
// into latitude/longitude WGS-84 coordinates (in degrees).
// pixelX: X coordinate of the point, in pixels.
// pixelY: Y coordinates of the point, in pixels.
// levelOfDetail: Level of detail, from 1 (lowest detail) to 23 (highest detail).
// latitude: Output parameter receiving the latitude in degrees.
// longitude: Output parameter receiving the longitude in degrees.
static void pixelXYToLatLong( int pixelX, int pixelY, int levelOfDetail,
                              double *latitude, double *longitude )
{
    double mapSize = getmapsize( levelOfDetail );
    double x = ( getclip( pixelX, 0, mapSize - 1 ) / mapSize ) - 0.5;
    double y = 0.5 - ( getclip( pixelY, 0, mapSize - 1 ) / mapSize );

    *latitude = 90 - 360 * atan( exp( -y * 2 * M_PI ) ) / M_PI;
    *longitude = 360 * x;
}


// Converts pixel XY coordinates into tile XY coordinates of the tile containing
// the specified pixel.
// pixelX: Pixel X coordinate.
// pixelY: Pixel Y coordinate.
// tileX: Output parameter receiving the tile X coordinate.
// tileY: Output parameter receiving the tile Y coordinate.
static void pixelXYToTileXY( int pixelX, int pixelY, int *tileX, int *tileY )
{
    *tileX = pixelX / 256;
    *tileY = pixelY / 256;
}


// Converts tile XY coordinates into pixel XY coordinates of the upper-left pixel
// of the specified tile.
// tileX: Tile X coordinate.
// tileY: Tile Y coordinate.
// pixelX: Output parameter receiving the pixel X coordinate.
// pixelY: Output parameter receiving the pixel Y coordinate.
static void tileXYToPixelXY( int tileX, int tileY, int *pixelX, int *pixelY )
{
    *pixelX = tileX * 256;
    *pixelY = tileY * 256;
}


// Converts tile XY coordinates into a QuadKey at a specified level of detail.
// tileX: Tile X coordinate.
// tileY: Tile Y coordinate.
// levelOfDetail: Level of detail, from 1 (lowest detail) to 23 (highest detail).
// returns: A string containing the QuadKey.
static int tileXYToQuadKey( char *quadkeys, int tileX, int tileY,
                            int levelOfDetail )
{
    int i = levelOfDetail;
    char digit = 0;
    int mask = 0;
    int len = 0;

    for(; i > 0; i-- )
    {
        digit = '0';
        mask = 1 << ( i - 1 );
        if( ( tileX & mask ) != 0 ){
            digit++;
        }
        if( ( tileY & mask ) != 0 ){
            digit += 2;
        }

        quadkeys[len++] = digit;
    }

    return len;
}


static int encode_lua( lua_State *L )
{
    lua_Number latitude = lauxh_checknumber( L, 1 );
    lua_Number longitude = lauxh_checknumber( L, 2 );
    lua_Integer levelOfDetail = lauxh_optinteger( L, 3, 23 );
    char quadkey[23] = {0};
    int len = 0;
    int pixelX = 0;
    int pixelY = 0;
    int tileX = 0;
    int tileY = 0;

    lauxh_argcheck(
        L, levelOfDetail >= 1 && levelOfDetail <= 23, 3,
        "1-23 expected, got an out of range value"
    );

    // Converts a point from latitude/longitude WGS-84 coordinates (in degrees)
    LatLongToPixelXY( latitude,  longitude, levelOfDetail, &pixelX, &pixelY );
    // Converts pixel XY coordinates into tile XY coordinates of the tile
    // containing the specified pixel.
    pixelXYToTileXY( pixelX, pixelY, &tileX, &tileY );
    // Converts tile XY coordinates into a QuadKey at a specified level of detail.
    len = tileXYToQuadKey( quadkey, tileX, tileY, levelOfDetail );
    lua_pushlstring( L, quadkey, len );

    return 1;
}


// Converts a QuadKey into tile XY coordinates.
// quadKey: QuadKey of the tile.
// tileX: Output parameter receiving the tile X coordinate.
// tileY: Output parameter receiving the tile Y coordinate.
// levelOfDetail: Output parameter receiving the level of detail.
static int quadKeyToTileXY( const char *quadKey, int levelOfDetail, int *tileX,
                            int *tileY )
{
    int i = levelOfDetail;
    int mask = 0;

    *tileX = *tileY = 0;
    for(; i > 0; i-- )
    {
        mask = 1 << ( i - 1 );
        switch( quadKey[levelOfDetail - i] )
        {
            case '0':
                break;

            case '1':
                *tileX |= mask;
                break;

            case '2':
                *tileY |= mask;
                break;

            case '3':
                *tileX |= mask;
                *tileY |= mask;
                break;

            // Invalid QuadKey digit sequence
            default:
                return -1;
        }
    }

    return 0;
}


static int decode_lua( lua_State *L )
{
    size_t len = 0;
    const char *quadkeys = lauxh_checklstring( L, 1, &len );
    int pixelX = 0;
    int pixelY = 0;
    int tileX = 0;
    int tileY = 0;
    double latitude = 0;
    double longitude = 0;

    lauxh_argcheck(
        L, len >= 1 && len <= 23, 1,
        "length between 1 and 23 expected, got an out of range value"
    );

    if( quadKeyToTileXY( quadkeys, (int)len, &tileX, &tileY ) == 0 ){
        // Converts tile XY coordinates into pixel XY coordinates of the upper-left
        // pixel of the specified tile.
        tileXYToPixelXY( tileX, tileY, &pixelX, &pixelY );
        // Converts a pixel from pixel XY coordinates at a specified level of detail
        // into latitude/longitude WGS-84 coordinates (in degrees).
        pixelXYToLatLong( pixelX, pixelY, (int)len, &latitude, &longitude );

        lua_createtable( L, 0, 2 );
        lauxh_pushnum2tbl( L, "lat", latitude );
        lauxh_pushnum2tbl( L, "lon", longitude );
        return 1;
    }

    // got error
    lua_pushnil( L );
    // invalid quadkey digit sequence
    lua_pushstring( L, strerror( EILSEQ ) );

    return 2;
}


LUALIB_API int luaopen_geo_quadkeys( lua_State *L )
{
    lua_createtable( L, 0, 2 );
    lauxh_pushfn2tbl( L, "encode", encode_lua );
    lauxh_pushfn2tbl( L, "decode", decode_lua );

    return 1;
}
