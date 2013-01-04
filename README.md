LibRaw-contrib
==============

Code contributed by LibRaw users


raw2tiff 
=========

This program converts a raw image-(such as canon's cr2 or nikon's nef) to
a tiff image. It accomplishes this using the libraw library available at
www.libraw.org. It emulates the dcraw -D -4 -T command. It has only been
tested using canon CR2 files.

In addition to writing out a tiff image it also allows the user to specify
that only a region of the captured raw image be inserted into the tiff 
image. In this way it emulates the dcraw_emu code which has crop box option.

This program is free software: you can use, modify and/or
redistribute it under the terms of the simplified BSD License.

Author: West Suhanic <west.suhanic@gmail.com>

Copyright (c) 2012, West Suhanic, gDial Inc.
All rights reserved.
