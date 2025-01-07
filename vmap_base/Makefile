#NAMES = $(wildcard MML/???.zip)
#VMAPSB = $(patsubst MML/%.zip, %_b.vmap2, ${NAMES})
#VMAPST = $(patsubst MML/%.zip, %_t.vmap2, ${NAMES})
#PNGS  = $(patsubst MML/%.zip, %_b.png, ${NAMES})

NAMES = V41 V42 V44 V51 V52 T52
VMAPSB = $(patsubst %, %_b.vmap2, ${NAMES})
VMAPST = $(patsubst %, %_t.vmap2, ${NAMES})
PNGSB  = $(patsubst %, %_b.png, ${NAMES})
PNGST  = $(patsubst %, %_t.png, ${NAMES})

RENDER_CONF = ../render.cfg
TYPEINFO = ../types.cfg

all: ${VMAPSB} ${VMAPST} ${PNGSB} ${PNGST}

%_b.vmap2: MML/%.zip
	../progs/make_base "../tconvs.txt" "MML" "$*" "$@"
#	../progs/make_base2 "../tconvs.txt" "MML" "$*" "$@"

%_b.png %_b.map: %_b.vmap2
	time ms2render "$<" --out "$@"\
    --config "${RENDER_CONF}" --types "${TYPEINFO}"\
    --define "{\"nom_name\":\"$*\"}"\
    --mkref nom_fi --name "$*" --dpi "200"\
    --png_format pal --map "$*_b.map"\

%_t.vmap2: MML/%.zip
	../progs/make_text "MML" "$*" "$@"

%_t.png %_t.map: %_b.vmap2 %_t.vmap2
	time ms2render $+ --out "$@"\
    --config "${RENDER_CONF}"  --types "${TYPEINFO}"\
    --define "{\"nom_name\":\"$*\"}"\
    --mkref nom_fi --name "$*" --dpi "200"\
    --png_format pal --map "$*_t.map"\

clean:
	rm -f *.png *.map *.vmap2