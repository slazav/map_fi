### Custom render of Finnish/Norwegian topo maps

Finnish topo maps are really great, but I do not like the way how
they are generalized to small scales. For long distance running/hiking,
I prefere a relatively small scale map (say, 1:50000). Larger scales
are not good for printing and for planning.

The problem is that many important objects are missing at small scales:
all hiking trails and small roads, autiatupa's, etc. Fortunately
original vector data are available. In this project I try to produce a
nice-looking hiking map (probably in a half-manual way, for some regions
which are interesting for me).


![example 2](https://github.com/slazav/map_fi/blob/main/example2.png)

### Methods

There are three main data sources:

* [Topographic map (vector)](https://www.maanmittauslaitos.fi/en/maps-and-spatial-data/datasets-and-interfaces/product-descriptions/topographic-map-series-raster)
from National Land Surway of Finland (https://maanmittauslaitos.fi).
This is 1:100'000 map, I'm using it as the main data source. I'm using
Shape format splitted in the standard Finnish map division.

* [Topographic Database](https://www.maanmittauslaitos.fi/en/maps-and-spatial-data/datasets-and-interfaces/product-descriptions/topographic-database)
from National Land Surway of Finland (https://maanmittauslaitos.fi).
This is most detailed data, I'm taking some objects from there.

* [Norwegian N50 map](https://kartkatalog.geonorge.no/metadata/n50-map-data/ea192681-d039-42ec-b1bc-f3ce04c189ac?search=N50)
from Norwegian Mapping Authority (https://www.kartverket.no).

I'm using my mapsoft2 ( https://github.com/slazav/mapsoft2 ) system, as
in my mountain map project ( https://github.com/slazav/map_hr )

The work is splitted in a few steps:

1. Converting source data at the map structure level (without knowledge
about specific objects or object types). This is done separately
for every source (see programs `import_fi1`, `import_fi2`, `import_no1`).

  * Maps are converted to my vmap2 format.
  * Coordinates are converted to WGS84.
  * Points in line and area objects are filtered.
  * Some names of data fields are translated, some default/useless fields are dropped.
  * Data is combined and cropped to 100'000 Finnish map standard division.
  * A 100m margin is introduced for line/area objects, to avoid boundary effects.

2. Converting data at the object type level (without knowledge about
specific objects) (see `make_map` program). Types are selected and
converted, using type translation tables. Some special types are treated
separately.

At this step data are splitted into three parts:

  * Base map: objects which I want to keep without any modifications.

  * Label map: labels with their reference points (because of original
    data structure labels are always attached to point objects).
    In the next step I want to adjust label position and sizes manually but update
    reference points (positions and names) from original data.

  * Extra map: some objects which I want to modify manually.
    In the next step I update data once and then do any modifications.

3. Conversions/editing at the object level. This step is done only
   with Label/Extra maps. I'm keeping "manual version" of this data
   and use 




