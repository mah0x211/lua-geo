# lua-geo

geo location util.

## Installation

```sh
luarocks install --from=http://mah0x211.github.io/rocks/ geo
```

or 

```sh
git clone https://github.com/mah0x211/lua-geo.git
cd lua-geo
luarocks make rockspecs/geo-<version>.rockspec
```


### encode( lat, lon, precision )

returns the geohash encoded string from specified arguments.

**Parameters**

- lat: latitude must be larger than -90  and less than 90.
- lon: longitude must be larger than -180 and less than 180.
- precision: hash string length must be larger than 0 and less than 17.

**Returns**

1. geohash: string.


### decode( geohash )

returns two values of latitude and longitude from geohash encoded string.

**Parameters**

- geohash: geohash encoded string.

**Returns**

1. lat: latitude.
2. lon: longitude.

