local geo = require('geo');

local precision = 16;
local incr = 0.1;
local LAT_MAX = 89.9;
local LON_MAX = 179.9;
local count = 0;
local lat, lon, hash, dlat, dlon, diff, result;

for lat = -LAT_MAX, LAT_MAX, incr do
    for lon = -LON_MAX, LON_MAX, incr do
        count = count + 1;
        hash = geo.encode( lat, lon, precision );
        dlat, dlon = geo.decode( hash );
        -- check round off error
        diff = {
            lat = math.abs( lat - dlat ),
            lon = math.abs( lon - dlon )
        };
        if diff.lat < incr and diff.lon < incr then
            --[[
            print(
                ('OK %4d:%4d -> %s precision %d -> %4d:%4d'):format(
                    lat, lon, hash, precision, dlat, dlon
                )
            );
            --]]
            result = 'OK';
        else
            result = 'FAIL';
            print(
                ('( %d, %d, %d ) -> %s -> ( %d, %d ) -> margin: %d, %d -> %s')
                :format( lat, lon, precision, hash, diff.lat, diff.lon, dlat,
                         dlon, result )
            );
            return;
        end
    end
end

print( ('test %d position finished'):format( count ) );

