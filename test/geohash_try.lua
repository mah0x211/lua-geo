local floor = math.floor;
local ceil = math.ceil;
local geo = require('geo.geohash');
local precision = 16;
local incr = 0.1;
local LAT_MAX = 90.0;
local LON_MAX = 180.0;
local lat, lon, hash, dec;

for lat = -LAT_MAX, LAT_MAX, incr do
    for lon = -LON_MAX, LON_MAX, incr do
        hash = ifNil( geo.encode( lat, lon, precision ) );
        dec = ifNil( geo.decode( hash ) );
        -- resolve round-off error
        ifNotEqual( floor( dec.lat * 1000000 + .5 ), ceil( lat * 1000000 ) );
        ifNotEqual( floor( dec.lon * 1000000 + .5 ), ceil( lon * 1000000 ) );
    end
end
