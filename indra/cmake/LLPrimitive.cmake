# -*- cmake -*-

# these should be moved to their own cmake file
include(Prebuilt)
include(Boost)

use_prebuilt_binary(colladadom)
use_prebuilt_binary(minizip-ng) # needed for colladadom
use_prebuilt_binary(pcre)
use_prebuilt_binary(libxml2)

set(LLPRIMITIVE_INCLUDE_DIRS
    ${LIBS_OPEN_DIR}/llprimitive
    )
if (WINDOWS)
    set(LLPRIMITIVE_LIBRARIES 
        debug llprimitive
        optimized llprimitive
        debug libcollada14dom23-sd
        optimized libcollada14dom23-s
        libxml2_a
        debug pcrecppd
        optimized pcrecpp
        debug pcred
        optimized pcre
        debug libminizip
        optimized libminizip
        ${BOOST_SYSTEM_LIBRARIES}
        )
elseif (DARWIN)
    set(LLPRIMITIVE_LIBRARIES 
        llprimitive
        debug collada14dom-d
        optimized collada14dom
        minizip           # for collada libminizip.a
        xml2
        pcrecpp
        pcre
        iconv           # Required by libxml2
        )
elseif (LINUX)
    set(LLPRIMITIVE_LIBRARIES 
        llprimitive
        debug collada14dom-d
        optimized collada14dom
        minizip
        xml2
        pcrecpp
        pcre
        )
#   if(LINUX)
#      list(APPEND LLPRIMITIVE_LIBRARIES minizip xml2 )
#    endif(LINUX)
endif (WINDOWS)


