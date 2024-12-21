NAMES = $(wildcard *.zip)
VMAPSB = $(patsubst %.zip, %_b.vmap2, ${NAMES})
VMAPST = $(patsubst %.zip, %_t.vmap2, ${NAMES})
PNGS  = $(patsubst %.zip, %_b.png, ${NAMES})
RENDER_CONF = ../render.cfg

all: ${VMAPSB} ${VMAPST} ${PNGS}

%_b.vmap2: %.zip
	../progs/make_base "$<" "$@"

%_b.png %_b.map: %_b.vmap2
	time ms2render "$<" --out "$@"\
    --config "${RENDER_CONF}" -t ""\
    --define "{\"nom_name\":\"$*\"}"\
    --mkref nom_fi --name "$*" --dpi "200"\
    --png_format pal --map "$*_b.map"\

%_t.vmap2: %.zip
	../progs/make_text "$<" "$@"

clean:
	rm -f *.png *.map *.vmap2