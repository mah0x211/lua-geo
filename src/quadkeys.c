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
// lv: Level of detail, from 1 (lowest detail) to 23 (highest detail).
// returns: The map width and height in pixels.
static inline unsigned int getmapsize( int lv )
{
    return 256 << lv;
}


// Converts a point from latitude/longitude WGS-84 coordinates (in degrees)
// into pixel XY coordinates at a specified level of detail.
// lat: latitude of the point, in degrees.
// lon: longitude of the point, in degrees.
// lv: from 1 (lowest detail) to 23 (highest detail)
// px: Output parameter receiving the X coordinate in pixels.
// py: Output parameter receiving the Y coordinate in pixels.
static void latlon2pixel( double lat, double lon, int lv, int *px, int *py )
{
    lat = getclip( lat, LATITUDE_MIN, LATITUDE_MAX );
    lon = getclip( lon, LONGITUDE_MIN, LONGITUDE_MAX );

    double x = ( lon  + 180) / 360;
    double sinlat = sin( lat * M_PI / 180 );
    double y = 0.5 - log( ( 1 + sinlat ) / ( 1 - sinlat ) ) / ( 4 * M_PI );
    unsigned int mapsize = getmapsize( lv );

    *px = (int)getclip( x * mapsize + 0.5, 0, mapsize - 1 );
    *py = (int)getclip( y * mapsize + 0.5, 0, mapsize - 1 );
}


// Converts a pixel from pixel XY coordinates at a specified level of detail
// into latitude/longitude WGS-84 coordinates (in degrees).
// px: X coordinate of the point, in pixels.
// py: Y coordinates of the point, in pixels.
// lv: Level of detail, from 1 (lowest detail) to 23 (highest detail).
// lat: Output parameter receiving the latitude in degrees.
// lon: Output parameter receiving the longitude in degrees.
static void pixel2latlon( int px, int py, int lv, double *lat, double *lon )
{
    double mapSize = getmapsize( lv );
    double x = ( getclip( px, 0, mapSize - 1 ) / mapSize ) - 0.5;
    double y = 0.5 - ( getclip( py, 0, mapSize - 1 ) / mapSize );

    *lat = 90 - 360 * atan( exp( -y * 2 * M_PI ) ) / M_PI;
    *lon = 360 * x;
}


// Converts pixel XY coordinates into tile XY coordinates of the tile containing
// the specified pixel.
// px: Pixel X coordinate.
// py: Pixel Y coordinate.
// tx: Output parameter receiving the tile X coordinate.
// ty: Output parameter receiving the tile Y coordinate.
static void pixel2tile( int px, int py, int *tx, int *ty )
{
    *tx = px / 256;
    *ty = py / 256;
}


// Converts tile XY coordinates into pixel XY coordinates of the upper-left pixel
// of the specified tile.
// tx: Tile X coordinate.
// ty: Tile Y coordinate.
// px: Output parameter receiving the pixel X coordinate.
// py: Output parameter receiving the pixel Y coordinate.
static void tile2pixel( int tx, int ty, int *px, int *py )
{
    *px = tx * 256;
    *py = ty * 256;
}


// Converts tile XY coordinates into a QuadKey at a specified level of detail.
// tx: Tile X coordinate.
// ty: Tile Y coordinate.
// lv: Level of detail, from 1 (lowest detail) to 23 (highest detail).
// returns: A string containing the QuadKey.
static int tile2quadkey( char *quadkeys, int tx, int ty, int lv )
{
    int i = lv;
    char digit = 0;
    int mask = 0;
    int len = 0;

    for(; i > 0; i-- )
    {
        digit = '0';
        mask = 1 << ( i - 1 );
        if( ( tx & mask ) != 0 ){
            digit++;
        }
        if( ( ty & mask ) != 0 ){
            digit += 2;
        }

        quadkeys[len++] = digit;
    }

    return len;
}


static int encode_lua( lua_State *L )
{
    lua_Number lat = lauxh_checknumber( L, 1 );
    lua_Number lon = lauxh_checknumber( L, 2 );
    lua_Integer lv = lauxh_optinteger( L, 3, 23 );
    char quadkey[23] = {0};
    int len = 0;
    int px = 0;
    int py = 0;
    int tx = 0;
    int ty = 0;

    lauxh_argcheck(
        L, lv >= 1 && lv <= 23, 3, "1-23 expected, got an out of range value"
    );

    // Converts a point from latitude/longitude WGS-84 coordinates (in degrees)
    latlon2pixel( lat,  lon, lv, &px, &py );
    // Converts pixel XY coordinates into tile XY coordinates of the tile
    // containing the specified pixel.
    pixel2tile( px, py, &tx, &ty );
    // Converts tile XY coordinates into a QuadKey at a specified level of detail.
    len = tile2quadkey( quadkey, tx, ty, lv );
    lua_pushlstring( L, quadkey, len );

    return 1;
}


// Converts a QuadKey into tile XY coordinates.
// quadKey: QuadKey of the tile.
// lv: Output parameter receiving the level of detail.
// tx: Output parameter receiving the tile X coordinate.
// ty: Output parameter receiving the tile Y coordinate.
static int quadkey2tile( const char *quadKey, int lv, int *tx, int *ty )
{
    int i = lv;
    int mask = 0;

    *tx = *ty = 0;
    for(; i > 0; i-- )
    {
        mask = 1 << ( i - 1 );
        switch( quadKey[lv - i] )
        {
            case '0':
                break;

            case '1':
                *tx |= mask;
                break;

            case '2':
                *ty |= mask;
                break;

            case '3':
                *tx |= mask;
                *ty |= mask;
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
    int tx = 0;
    int ty = 0;

    lauxh_argcheck(
        L, len >= 1 && len <= 23, 1,
        "length between 1 and 23 expected, got an out of range value"
    );

    if( quadkey2tile( quadkeys, (int)len, &tx, &ty ) == 0 ){
        int px = 0;
        int py = 0;
        double lat = 0;
        double lon = 0;

        // Converts tile XY coordinates into pixel XY coordinates of the upper-left
        // pixel of the specified tile.
        tile2pixel( tx, ty, &px, &py );
        // Converts a pixel from pixel XY coordinates at a specified level of detail
        // into latitude/longitude WGS-84 coordinates (in degrees).
        pixel2latlon( px, py, (int)len, &lat, &lon );

        lua_createtable( L, 0, 2 );
        lauxh_pushnum2tbl( L, "lat", lat );
        lauxh_pushnum2tbl( L, "lon", lon );
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
