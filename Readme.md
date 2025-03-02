### Custom render of Finnish/Norwegian/Swedish topo maps

Finnish topo maps are really great, but I do not like the way how they
are generalized to small scales. For long distance running/hiking, I
prefer a relatively small scale map (say, 1:50000), for both route
planning and paper map printing.

The problem is that many important objects are missing at small scales:
all hiking trails and small roads, tourist infrastructure (open huts and
shelters), etc. Fortunately original vector data are available. In this
project I try to produce a nice-looking hiking map for some regions
which are interesting for me.

At the moment I have a few regions ready: Kilpisj&auml;rvi area, UKK park,
Eastern Finland (Lahti - Jyv&auml;skyl&auml; - Imatra)

I produce raster images with OziExplorer reference, raster tiles
(mbtiles and Osmand sqlitedb), IMG vector map for Garmin devices.

Maps can be downloaded from [my website](https://slazav.xyz/maps/?lang=en#h3) or viewed in
[nakarte.me](https://nakarte.me/#m=5/65.68543/19.81934&l=O/Fmk/-cseyJuYW1lIjoic2xhemF2X2ZpIiwidXJsIjoiaHR0cHM6Ly90aWxlcy5zbGF6YXYueHl6L2ZpL3t6fS97eH0ve3l9LnBuZyIsInRtcyI6ZmFsc2UsInNjYWxlRGVwZW5kZW50IjpmYWxzZSwibWF4Wm9vbSI6MTMsImlzT3ZlcmxheSI6dHJ1ZSwiaXNUb3AiOnRydWV9)

![example 2](https://github.com/slazav/map_fi/blob/main/example2.png)

### Methods

There are four main data sources:

* [Finnish Topographic map (vector)](https://www.maanmittauslaitos.fi/en/maps-and-spatial-data/datasets-and-interfaces/product-descriptions/topographic-map-series-raster)
from National Land Survey of Finland (https://maanmittauslaitos.fi).
This is 1:100'000 map, I'm using it as the main data source. I'm using
Shape format splitted in the standard Finnish map division.

* [Finnish Topographic Database](https://www.maanmittauslaitos.fi/en/maps-and-spatial-data/datasets-and-interfaces/product-descriptions/topographic-database)
from National Land Survey of Finland (https://maanmittauslaitos.fi).
This is most detailed data, I'm taking some objects from there.

* [Norwegian N50 map](https://kartkatalog.geonorge.no/metadata/n50-map-data/ea192681-d039-42ec-b1bc-f3ce04c189ac?search=N50)
from Norwegian Mapping Authority (https://www.kartverket.no).

* [Swedish Topografi 50 Nedladdning, vektor](https://www.lantmateriet.se/sv/geodata/vara-produkter/produktlista/topografi-50-nedladdning-vektor/)

I'm using my mapsoft2 ( https://github.com/slazav/mapsoft2 ) system, as
in my mountain map project ( https://github.com/slazav/map_hr )

The work is splitted in a few steps:

1. Converting source data at the map structure level (without knowledge
about specific objects or object types). This is done separately for
every source (see programs `import_fi1`, `import_fi2`, `import_no1`,
`import_se1` and `vmap1` directory).

  * Maps are converted to my vmap2 format.
  * Coordinates are converted to WGS84.
  * Points in line and area objects are filtered.
  * Some names of data fields are translated, some default/useless fields are dropped.
  * Data is combined and cropped to 100'000 Finnish map standard division.
  * A 100m margin is introduced for line/area objects, to avoid boundary effects.

2. Converting data at the object type level (without knowledge about
specific objects) (see `make_map` program and `vmap2` directory). Types
are selected and converted, using type translation tables. Some special
types are treated separately.

At this step data from all sources are mixed together and splitted into two parts:

  * Base map, objects which I want to keep without any modifications.

  * Label map, point objects with names which I want to modify.

3. Conversions/editing at the object level. This step is done only
with Label maps. There is a "manual" of this data (see `vmap3` directory).
A few things are done at this step:

  * Label sizes and positions are adjusted

  * Points can be edited: moved, deleted, renamed.
    There is a program `update_map` which can create or apply a patch
    for such modifications. This allows me to update map from
    source without loosing edited data.

  * Any additional objects can be added at this step. For example I added a few forest clearings
    near UKK park.

  * There is a separate manually edited file for adding point objects
    (`points_add.vmap2`). If you want to add a missing house, fireplace,
    etc., send me a pull request.

```
Original data:   [FI1]         [FI2]         [NO1]         [SE1]
                   |             |             |             |
              (import_fi1)  (import_fi2)  (import_no1)  (import_se1)
                   |             |             |             |
               *.fi1.vmap2   *.fi2.vmap2   *.no1.vmap2   *.se1.vmap2
                   |             |             |             |
                    --------> (make_map) <-----  <-----------
                              /  \
                             /    \
base+label maps:    *_b.vmap2   *_t.vmap2    points_add.vmap2
                        |          |          /
                        |      (update_map), manual editing
                        |          |
                        |       *.vmap2
                        |          |
                         --------->|
                                 output
```


