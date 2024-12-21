### Custom render of Finnish topo maps

Finnish topo maps are really great, but I do not like the way how
they are generalized to small scales. For long distance running/hiking,
I prefere a relatively small scale map (say, 1:50000). Larger scales
are not good for printing and for planning.

The problem is that many important objects are missing at small scales:
all hiking trails and small roads, autiatupa's, etc. In this project I
try to use original vector data and produce a nice-looking map (probably
in a half-manual way, for some regions which are interesting for me).

I'm using my mapsoft2 ( https://github.com/slazav/mapsoft2 ) system, as
in my mountain map project ( https://github.com/slazav/map_hr )

![example 1](https://github.com/slazav/map_fi/blob/main/example1.png)

###

There are a few types of data in the project:

* Original 1:100'000 vector map data from https://maanmittauslaitos.fi .
It is automatically filtered and simplified, types are substituded to
something reasonable for mp/img format (see `make_base.cpp` program).

* Text objects and their references from the same 1:100'000 dataset.
They are also filtered and simplified (see `make_text.cpp` program).
Label positions (but not reference points and names) can be manually
adjusted.

* Additional data. Currently I draw it manually (it's not a big work for
a reasonable regions). If I would know how to get more detailed map
data, I could use some of it here. This data can contain any
types from the base map, other types, labels.
