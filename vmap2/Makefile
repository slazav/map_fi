#NAMES = $(wildcard MML/???.zip)
#VMAPSB = $(patsubst MML/%.zip, %_b.vmap2, ${NAMES})
#VMAPST = $(patsubst MML/%.zip, %_t.vmap2, ${NAMES})
#PNGS  = $(patsubst MML/%.zip, %_b.png, ${NAMES})

# what I have in FI1
#NAMES = X43 X51 X52\
# W33 W34 W41 W43 W44 W51 W52\
# V34 V41 V42 V43 V44 V51 V52 V53\
# U41 U42 U43 U44 U51 U52 U53 U54\
# T41 T42 T43 T44 T51 T52 T53 T54
# S51 S52
# R51 R52

# what I have in FI2
NAMES =\
 W33 W34 W41\
 V34 V41 V42 V43 V44 V51 V52\
 U42\
 T52\
 S51\
 R52 R51\
 N43\
 M43 M44\
 L41 L42



VMAPSB  = $(patsubst %, %_b.vmap2.gz, ${NAMES})
VMAPST = $(patsubst %, %_t.vmap2.gz, ${NAMES})
PNGB = $(patsubst %, %_b.png, ${NAMES})
PNGT = $(patsubst %, %_t.png, ${NAMES})

RENDER_CONF = ../render.cfg
TYPEINFO = ../types.cfg

all: ${VMAPSB} ${VMAPST} ${VMAPSX} ${PNGB} ${PNGX}

%_b.vmap2.gz %_t.vmap2.gz:
	../progs/make_map "$*" "../vmap1"

%_t.png %_t.map: %_b.vmap2.gz %_t.vmap2.gz
	time ~/mapsoft2/programs/ms2render/ms2render $+ --out "$@"\
    --config "${RENDER_CONF}" --types "${TYPEINFO}"\
    --define "{\"nom_name\":\"$*\"}"\
    --mkref nom_fi --name "$*" --dpi "200"\
    --png_format pal --map "$*_t.map"

%_b.png %_b.map: %_b.vmap2.gz
	time ~/mapsoft2/programs/ms2render/ms2render $+ --out "$@"\
    --config "${RENDER_CONF}" --types "${TYPEINFO}"\
    --define "{\"nom_name\":\"$*\"}"\
    --mkref nom_fi --name "$*" --dpi "200"\
    --png_format pal --map "$*_b.map"

clean:
	rm -f *.png *.map *.vmap2.gz
