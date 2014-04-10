/*
 *  Copyright (C) 2013 Masatoshi Teruya
 *
 *  Permission is hereby granted, free of charge, to any person obtaining a copy
 *  of this software and associated documentation files (the "Software"), to deal
 *  in the Software without restriction, including without limitation the rights
 *  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 *  copies of the Software, and to permit persons to whom the Software is
 *  furnished to do so, subject to the following conditions:
 *
 *  The above copyright notice and this permission notice shall be included in
 *  all copies or substantial portions of the Software.
 *
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
 *  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 *  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 *  THE SOFTWARE.
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
#include "lauxlib.h"
#include "lualib.h"
#include "lua.h"

#define GEO_MAX_HASH_LEN    16
#define GEO_MAX_PRECISION_RANGE     16
#define GEO_IS_PRECISION_RANGE(p)   ( p > 0 && p < 17 )
#define GEO_IS_LAT_RANGE(l)         ( l > -90 && l < 90 )
#define GEO_IS_LON_RANGE(l)         ( l > -180 && l < 180 )
#define GEO_IS_LATLON_RANGE(la,lo) \
    ( GEO_IS_LAT_RANGE(la) && GEO_IS_LON_RANGE( la ) )

// semi-major axis of ellipse
#define GEO_WGS84MAJOR  6378137.0
// semi-minor axis of ellipse
#define GEO_WGS84MINOR  6356752.314245

// eccentricity
// ( GEO_WGS84MAJOR^2 - GEO_WGS84MINOR^2 ) / GEO_WGS84MAJOR^2 = 0.006694379990197
#define GEO_ECCENTRICITY    0.006694379990197

// meridian numerator
//    = GEO_WGS84MAJOR * ( 1 - GEO_ECCENTRICITY )
//    = 6378137.0 * 1- 0.006694379990197
//    = 6335439.327292464877011
#define GEO_MERIDIAN_NUM    6335439.327292464877011

// lat_ave = ( org_lat_rad + dst_lat_rad ) / 2;
// W = sqrt( 1 - GEO_ECCENTRICITY * lat_ave_sin^2 )
// meridian curvature: GEO_MERIDIAN_NUM / pow( W, 3 )
#define GEO_MERIDIAN(w)     (GEO_MERIDIAN_NUM/pow(w,3))
// prime vertical: GEO_WGS84MAJOR / W
#define GEO_PRIME_VERT(w)   (GEO_WGS84MAJOR/w)

static const double GEO_RAD = M_PI/180;
static const double GEO_DEG = 180/M_PI;
static const double GEO_PI2 = M_PI*2;

#define GEO_DEG2RAD(d)  (d*GEO_RAD)
#define GEO_RAD2DEG(r)  (r/GEO_DEG)

static const uint8_t GEO_BITMASK[5] = { 16, 8, 4, 2, 1 };


typedef struct {
    double lat;
    double lon;
    double lat_rad;
    double lon_rad;
    double lat_sin;
    double lat_cos;
    double lon_sin;
    double lon_cos;
} geo_t;


typedef struct {
    double lat;
    double lon;
    double lat_rad;
    double lon_rad;
    double dist;
    double dist_sin;
    double dist_cos;
    double angle;
    double angle_rad;
    geo_t *pivot;
} geodest_t;


static int geo_init( geo_t *geo, double lat, double lon, int with_math )
{
    if( GEO_IS_LATLON_RANGE( lat, lon ) )
    {
        geo->lat = lat;
        geo->lon = lon;
        geo->lat_rad = GEO_DEG2RAD(lat);
        geo->lon_rad = GEO_DEG2RAD(lon);
        if( with_math ){
            geo->lat_sin = sin( geo->lat_rad );
            geo->lat_cos = cos( geo->lat_rad );
            geo->lon_sin = sin( geo->lon_rad );
            geo->lon_cos = cos( geo->lon_rad );
        }
        return 0;
    }
    
    errno = EINVAL;
    return -1;
}

static int geo_init_by_tokyo( geo_t *geo, double lat, double lon, int with_math )
{
    return geo_init( geo, 
                     lon - lat * 0.000046038 - lon * 0.000083043 + 0.010040,
                     lat - lat * 0.00010695 + lon * 0.000017464 + 0.0046017,
                     with_math );
}

// merter
static double geo_get_distance( geo_t *from, geo_t *dest )
{
    double lat_ave = ( from->lat_rad + dest->lat_rad ) / 2;
    double W = sqrt( 1 - GEO_ECCENTRICITY * pow( sin( lat_ave ), 2 ) );
    
    return sqrt( pow( ( from->lat_rad - dest->lat_rad ) * GEO_MERIDIAN(W), 2 ) +
                 pow( ( from->lon_rad - dest->lon_rad ) * GEO_PRIME_VERT(W) * 
                      cos( lat_ave ), 2 ) );
};

static void geo_dest_update( geodest_t *dest )
{
    double platc_ds = dest->pivot->lat_cos * dest->dist_sin;
    
    dest->lat_rad = asin( dest->pivot->lat_sin * dest->dist_cos +
                          platc_ds * cos( dest->angle_rad ) );
    dest->lon_rad = fmod( dest->pivot->lon_rad + 
                          atan2( platc_ds * sin( dest->angle_rad ),
                                 dest->dist_cos - 
                                 pow( dest->pivot->lat_sin, 2 ) ) + 
                          M_PI, GEO_PI2 ) - M_PI;
    
    dest->lat = dest->lat_rad * GEO_DEG;
    dest->lon = dest->lon_rad * GEO_DEG;
}

static void geo_set_angle( geodest_t *dest, double angle, int update )
{
    dest->angle = angle;
    dest->angle_rad = angle * GEO_RAD;
    
    if( update ){
        geo_dest_update( dest );
    }
}

static void geo_set_distance( geodest_t *dest, double dist, int update )
{
    dest->dist = dist / GEO_WGS84MAJOR;
    dest->dist_sin = sin( dest->dist );
    dest->dist_cos = cos( dest->dist );
    
    if( update ){
        geo_dest_update( dest );
    }
}

static void geo_get_dest( geodest_t *dest, geo_t *from, double dist, double angle )
{
    dest->pivot = from;
    geo_set_distance( dest, dist, 0 );
    geo_set_angle( dest, angle, 0 );
    geo_dest_update( dest );
}


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
    //      0  1  2  3  4  5  6  7  8  9 
            1, 2, 3, 4, 5, 6, 7, 8, 9, 10,
            0, 0, 0, 0, 0, 0, 0, 0, 
    //      B   C   D   E   F   G   H      J   K      M   N      P   Q   R   S
            11, 12, 13, 14, 15, 16, 17, 0, 18, 19, 0, 20, 21, 0, 22, 23, 24, 25,
    //      T   U   V   W   X   Y   Z 
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
            if( !geohash32code[c] ){
                errno = EINVAL;
                return -1;
            }
            
            for( j = 0; j < 5; j++ ){
                mask = ( geohash32code[c] - 1 ) & GEO_BITMASK[j];
                latlon[is_lon][!mask] = ( latlon[is_lon][0] + latlon[is_lon][1] ) / 2;
                is_lon = !is_lon;
            }
        }
        
        *lat = ( latlon[0][0] + latlon[0][1] ) / 2;
        *lon = ( latlon[1][0] + latlon[1][1] ) / 2;
    
        return 0;
    }
    
    errno = EINVAL;
    return -1;
}


// MARK: lua binding
static int geo_encode_lua( lua_State *L )
{
    int rc = 0;
    double lat = luaL_checknumber( L, 1 );
    double lon = luaL_checknumber( L, 2 );
    uint8_t precision = (uint8_t)luaL_checknumber( L, 3 );
    char hash[GEO_MAX_HASH_LEN+1] = {0};
    
    lua_settop( L, 0 );
    // encode
    if( !geo_hash_encode( hash, lat, lon, precision ) ){
        return luaL_error( L, "invalid value range" );
    }
    else {
        rc = 1;
        lua_pushstring( L, hash );
    }
    
    return rc;
}

static int geo_decode_lua( lua_State *L )
{
    int rc = 0;
    size_t len = 0;
    const char *hash = luaL_checklstring( L, 1, &len );
    double lat = 0;
    double lon = 0;
    
    // deecode
    if( geo_hash_decode( hash, len, &lat, &lon ) == -1 ){
        return luaL_argerror( L, 1, "invalid hash string" );
    }
    else {
        rc = 1;
        lua_settop( L, 0 );
        lua_createtable( L, 0, 2 );
        lua_pushstring( L, "lat" );
        lua_pushnumber( L, lat );
        lua_rawset( L, -3 );
        lua_pushstring( L, "lon" );
        lua_pushnumber( L, lon );
        lua_rawset( L, -3 );
    }
    
    return rc;
}

#define lstate_fn2tbl(L,k,v) do{ \
    lua_pushstring(L,k); \
    lua_pushcfunction(L,v); \
    lua_rawset(L,-3); \
}while(0)

LUALIB_API int luaopen_geo( lua_State *L )
{
    static struct luaL_Reg method[] = {
        { "encode", geo_encode_lua },
        { "decode", geo_decode_lua },
        { NULL, NULL }
    };
    int i = 0;
    
    lua_newtable( L );
    while( method[i].name ){
        lua_pushstring( L, method[i].name );
        lua_pushcfunction( L, method[i].func );
        lua_rawset( L,-3 );
        i++;
    }
    
    return 1;
}

