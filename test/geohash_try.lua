local geo = require('geo');
local precision = 16;
local incr = 0.1;
local LAT_MAX = 90.0;
local LON_MAX = 180.0;
local lat, lon, hash, dec, dlat, dlon, diff;

for lat = -LAT_MAX, LAT_MAX, incr do
    for lon = -LON_MAX, LON_MAX, incr do
        hash = ifNil( geo.encode( lat, lon, precision ) );
        dec = ifNil( geo.decode( hash ) );
        -- check round off error
        diff = {
            lat = math.abs( lat - dec.lat ),
            lon = math.abs( lon - dec.lon )
        };
        ifFalse( 
            diff.lat < incr and diff.lon < incr,
            '( %d, %d, %d ) -> %s -> ( %d, %d ) -> margin: %d, %d',
            lat, lon, precision, hash, diff.lat, diff.lon, dec.lat, dec.lon
        );
    end
end
