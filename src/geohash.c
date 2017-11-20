/*
 *  Copyright (C) 2013-2017 Masatoshi Teruya
 *
 *  Permission is hereby granted, free of charge, to any person obtaining a
 *  copy of this software and associated documentation files (the "Software"),
 *  to deal in the Software without restriction, including without limitation
 *  the rights to use, copy, modify, merge, publish, distribute, sublicense,
 *  and/or sell copies of the Software, and to permit persons to whom the
 *  Software is furnished to do so, subject to the following conditions:
 *
 *  The above copyright notice and this permission notice shall be included in
 *  all copies or substantial portions of the Software.
 *
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 *  THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 *  FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 *  DEALINGS IN THE SOFTWARE.
 *
 *
 *  range of latitude           :  -90 - 90
 *  range of longitude          : -180 - 180
 *
 *  DMS(Degrees Minutes Seconds): hh:mm:ss.sss
 *      range of latitude           : hh:        -89 - 89
 *      range of longitude          : hh:       -179 - 179
 *      latitude/longitude          : mm:          0 - 59
 *                                  : ss.sss:      0 - 59.999
 */

#include <unistd.h>
#include <stdint.h>
#include <math.h>
#include <errno.h>
#include <string.h>
#include "lauxhlib.h"


#define GEO_MAX_HASH_LEN    16
#define GEO_IS_PRECISION_RANGE(p)   ( p > 0 && p < 17 )
#define GEO_IS_LAT_RANGE(l)         ( l >= -90 && l <= 90 )
#define GEO_IS_LON_RANGE(l)         ( l >= -180 && l <= 180 )
#define GEO_IS_LATLON_RANGE(la,lo) \
    ( GEO_IS_LAT_RANGE(la) && GEO_IS_LON_RANGE( la ) )


static const uint8_t GEO_BITMASK[5] = { 16, 8, 4, 2, 1 };


static char *geo_hash_encode( char *hash, double lat, double lon, uint8_t precision )
{
    if( !GEO_IS_PRECISION_RANGE( precision ) ||
        !GEO_IS_LATLON_RANGE( lat, lon ) ){
        errno = EINVAL;
        return NULL;
    }
    else
    {
        static const char geobase32[] = "0123456789bcdefghjkmnpqrstuvwxyz";
        double latlon[2][2] = { { -90.0, 90.0 }, { -180.0, 180.0 } };
        double latlon_org[2] = { lat, lon };
        int is_lon = 1;
        double mid;
        int bit = 0;
        uint8_t idx = 0;
        int i = 0;

        // precision: 1 - 16
        while( i < precision )
        {
            mid = ( latlon[is_lon][0] + latlon[is_lon][1] ) / 2;
            if( latlon_org[is_lon] >= mid ){
                idx |= GEO_BITMASK[bit];
                latlon[is_lon][0] = mid;
            }
            else{
                latlon[is_lon][1] = mid;
            }
            is_lon = !is_lon;

            if( bit < 4 ){
                bit++;
            }
            else {
                hash[i++] = geobase32[idx];
                bit = 0;
                idx = 0;
            }
        }
        hash[i] = '\0';
    }

    return hash;
}


static int geo_hash_decode( const char *hash, size_t len, double *lat, double *lon )
{
    if( GEO_IS_PRECISION_RANGE( len ) )
    {
        static const unsigned char geohash32code[256] = {
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            0, 0, 0, 0,
        //  0  1  2  3  4  5  6  7  8  9
            1, 2, 3, 4, 5, 6, 7, 8, 9, 10,
            0, 0, 0, 0, 0, 0, 0, 0,
        //  B   C   D   E   F   G   H      J   K      M   N      P   Q   R   S
            11, 12, 13, 14, 15, 16, 17, 0, 18, 19, 0, 20, 21, 0, 22, 23, 24, 25,
        //  T   U   V   W   X   Y   Z
            26, 27, 28, 29, 30, 31, 32,
            0
        };
        const unsigned char *code = (const unsigned char*)hash;
        double latlon[2][2] = { { -90.0, 90.0 }, { -180.0, 180.0 } };
        uint8_t c;
        int is_lon = 1;
        int mask = 0;
        size_t i, j;

        for( i = 0; i < len; i++ )
        {
            // to uppercase
            c = ( code[i] > '`' && code[i] < '{' ) ? code[i] - 32 : code[i];
            c = geohash32code[c];
            // invalid charcode
            if( !c ){
                errno = EINVAL;
                return -1;
            }
            // decode
            for( j = 0; j < 5; j++ ){
                mask = ( c - 1 ) & GEO_BITMASK[j];
                latlon[is_lon][!mask] = ( latlon[is_lon][0] + latlon[is_lon][1] ) / 2;
                is_lon = !is_lon;
            }
        }

        *lat = ( latlon[0][0] + latlon[0][1] ) / 2;
        *lon = ( latlon[1][0] + latlon[1][1] ) / 2;

        return 0;
    }
    else {
        errno = EOVERFLOW;
    }

    return -1;
}


// MARK: lua binding
static int encode_lua( lua_State *L )
{
    int rc = 0;
    double lat = luaL_checknumber( L, 1 );
    double lon = luaL_checknumber( L, 2 );
    uint8_t precision = (uint8_t)luaL_checknumber( L, 3 );
    char hash[GEO_MAX_HASH_LEN+1] = {0};

    // encode
    if( geo_hash_encode( hash, lat, lon, precision ) ){
        lua_pushstring( L, hash );
        return 1;
    }

    // got error
    lua_pushnil( L );
    lua_pushstring( L, strerror( errno ) );

    return 2;
}


static int decode_lua( lua_State *L )
{
    size_t len = 0;
    const char *hash = luaL_checklstring( L, 1, &len );
    double lat = 0;
    double lon = 0;

    // decode
    if( geo_hash_decode( hash, len, &lat, &lon ) == 0 ){
        lua_createtable( L, 0, 2 );
        lauxh_pushnum2tbl( L, "lat", lat );
        lauxh_pushnum2tbl( L, "lon", lon );
        return 1;
    }

    // got error
    lua_pushnil( L );
    lua_pushstring( L, strerror( errno ) );

    return 2;
}


LUALIB_API int luaopen_geo_geohash( lua_State *L )
{
    lua_createtable( L, 0, 2 );
    lauxh_pushfn2tbl( L, "encode", encode_lua );
    lauxh_pushfn2tbl( L, "decode", decode_lua );

    return 1;
}

