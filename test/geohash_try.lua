local floor = math.floor;
local ceil = math.ceil;
local geo = require('geo.geohash');
local precision = 16;
local incr = 0.1;
local LAT_MAX = 90.0;
local LON_MAX = 180.0;
local lat, lon, hash, dlat, dlon, err;

for lat = -LAT_MAX, LAT_MAX, incr do
    for lon = -LON_MAX, LON_MAX, incr do
        hash = ifNil( geo.encode( lat, lon, precision ) );
        dlat, dlon, err = geo.decode( hash );
        ifNil( dlat, err );
        -- resolve round-off error
        ifNotEqual( floor( dlat * 1000000 + .5 ), ceil( lat * 1000000 ) );
        ifNotEqual( floor( dlon * 1000000 + .5 ), ceil( lon * 1000000 ) );
    end
end
