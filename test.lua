local geo = require('geo');

local precision = 16;
local incr = 0.1;
local dlat, dlon, latlon, hash, result;

for lat = -89.9, 89.9, incr do
    for lon = -179.9, 179.9, incr do
        hash = geo.encode( lat, lon, precision );
        latlon = geo.decode( hash );
        -- check round off error
        dlat = math.abs( lat - latlon.lat );
        dlon = math.abs( lon - latlon.lon );
        if dlat < incr and dlon < incr then
            result = 'OK';
        else
            result = 'FAIL';
        end
        
        if result == 'FAIL' then 
            print( 
                '( ' .. tostring( lat ) .. ', ' .. tostring( lon ) .. ', ' .. tostring( precision ) .. ' ) -> ' ..
                hash .. ' -> ' .. 
                '( ' .. tostring( latlon.lat ) .. ', ' .. tostring( latlon.lon ) .. ' ) -> ' ..
                'margin: ' .. tostring( dlat ) .. ', ' .. tostring( dlon ) .. ' -> ' ..
                result
            );
            return; 
        end
    end
end

print( 'test finished' );

