package = "geo"
version = "scm-1"
source = {
    url = "git://github.com/mah0x211/lua-geo.git"
}
description = {
    summary = "geo location util",
    homepage = "https://github.com/mah0x211/lua-geo",
    license = "MIT/X11",
    maintainer = "Masatoshi Teruya"
}
dependencies = {
    "lua >= 5.1"
}
build = {
    type = "builtin",
    modules = {
        geo = "src/geo.c",
        ["geo.geohash"] = {
            incdirs = { "deps/lauxhlib" },
            sources = { "src/geohash.c" }
        },
    }
}

