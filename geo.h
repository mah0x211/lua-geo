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
 */
/*
   geo.h
   
     range of latitude: -90 - 90
    range of longitude: -180 - 180
    
    DMS(Degrees Minutes Seconds): hh:mm:ss.sss
        range of latitude:
            hh: -89 - 89
        range of longitude:
            hh: -179 - 179
        latitude/longitude
            mm: 0 - 59
        ss.sss: 0 - 59.999
*/
#ifndef ___GEO___
#define ___GEO___

#include <unistd.h>
#include <stdint.h>
#include "lua.h"

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


#define GEO_MAX_PRECISION_RANGE     16
#define GEO_IS_PRECISION_RANGE(p)   ( p > 0 && p < 17 )
#define GEO_IS_LAT_RANGE(l)         ( l > -90 && l < 90 )
#define GEO_IS_LON_RANGE(l)         ( l > -180 && l < 180 )
#define GEO_IS_LATLON_RANGE(la,lo) \
    ( GEO_IS_LAT_RANGE(la) && GEO_IS_LON_RANGE( la ) )

int geo_init( geo_t *geo, double lat, double lon, int with_math );
int geo_init_by_tokyo( geo_t *geo, double lat, double lon, int with_math );

double geo_get_distance( geo_t *from, geo_t *dest );

void geo_get_dest( geodest_t *dest, geo_t *from, double dist, double angle );
void geo_dest_update( geodest_t *dest );
void geo_set_distance( geodest_t *dest, double dist, int update );
void geo_set_angle( geodest_t *dest, double angle, int update );

#define GEO_MAX_HASH_LEN    16

char *geo_hash_encode( char *hash, double lat, double lon, uint8_t precision );
int geo_hash_decode( const char *hash, size_t len, double *lat, double *lon );

// lua binding
LUALIB_API int luaopen_geohash( lua_State *L );


#endif

