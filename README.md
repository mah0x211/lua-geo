# lua-geo

geo location util.

## Installation

```sh
luarocks install geo --from=http://mah0x211.github.io/rocks/
```

## API

### Encode

#### hash, err = geo.encode( lat:number, lon:number, precision:uint )

returns the geohash encoded string from specified arguments.

```lua
local geo = require('geo');
local precision = 16;
local lat = 35.673343;
local lon = 139.710388;
local hash = geo.encode( lat, lon, precision );

print( hash ); -- 'xn76gnjurcpggseg'
```

**Parameters**

- `lat`: number - the latitude value range must be the `-90` to `90`.
- `lon`: number - the longitude value range must be the `-180` to `180`.
- `precision`: uint - the hash string length range must be the `1` to `16`.

**Returns**

1. `hash`: string.
2. `err`: string.


### Decode

#### tbl, err = geo.decode( geohash:string )

returns table that included of latitude and longitude values from the geohash string.

```lua
local inspect = require('util').inspect;
local geo = require('geo');
local hash = 'xn76gnjurcpggseg';
local latlon = geo.decode( hash );

print( inspect( latlon ) );
--[[
{
    lat = 35.673342999935,
    lon = 139.71038800008
}
--]]
```

**Parameters**

- `geohash`: string - geohash encoded string.

**Returns**

1. `latlon`: table - `{ lat = latitude:number, lon = longitude:number }`.
2. `err`: string.
