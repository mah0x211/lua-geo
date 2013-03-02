package = "geo-native"
version = "1.0-0"
source = {
    url = "https://github.com/mah0x211/lua-geo-native.git"
}
description = {
    summary = "geo location util",
    detailed = [[]],
    homepage = "https://github.com/mah0x211/lua-geo-native", 
    license = "MIT/X11",
    maintainer = "Masatoshi Teruya"
}
dependencies = {
    "lua >= 5.1"
}
build = {
    type = "builtin",
    modules = {
        geo = {
            sources = { "geo.c" }
        }
    }
}

