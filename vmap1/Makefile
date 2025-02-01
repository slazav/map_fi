PDIR=~/MYMAPS/map_fi/progs

FI1 =            X52\
             X43 X51\
     W34     W44\
     W33 W41 W43 W51\
     V34 V42 V44 V52\
         V41 V43 V51 V53\
         U42 U44 U52 U54\
         U41 U43 U51 U53\
         T42 T44 T52 T54\
         T41 T43 T51 T53\
                 S52\
                 S51\
                 R52\
                 R51\
                 N43\
                 M44\
                 M43\
L41 L42


FI2 =\
     W34\
     W33 W41\
     V34 V42 V44 V52\
         V41 V43 V51\
         U42\
                 T52\
\
                 S51\
                 R52\
                 R51\
                 N43\
                 M44\
                 M43\
L41 L42

NO =\
     W34 W42 W44\
     W33 W41 W43\
     V34 V42 V44\

SE = W33 W34\
     V34 V42 V41

all:\
  ${patsubst %, %_fi1.vmap2.gz, ${FI1}}\
  ${patsubst %, %_fi2.vmap2.gz, ${FI2}}\
  ${patsubst %, %_no1.vmap2.gz, ${NO}}\
  ${patsubst %, %_se1.vmap2.gz, ${SE}}

########################

%_fi1.vmap2.gz:
	${PDIR}/import_fi1 "$*" "FI"

%_fi2.vmap2.gz:
	${PDIR}/import_fi2 "$*" "FI"

########################
Finnm=NO/Basisdata_56_Finnmark_25835_N50Kartdata_PostGIS.sql
Troms=NO/Basisdata_55_Troms_25833_N50Kartdata_PostGIS.sql

V34_no1.vmap2.gz:
	${PDIR}/import_no  V34 ${Troms}
V42_no1.vmap2.gz:
	${PDIR}/import_no  V42 ${Finnm}
V44_no1.vmap2.gz:
	${PDIR}/import_no  V44 ${Finnm}
W33_no1.vmap2.gz:
	${PDIR}/import_no  W33 ${Finnm} ${Troms}
W34_no1.vmap2.gz:
	${PDIR}/import_no  W34 ${Troms}
W41_no1.vmap2.gz:
	${PDIR}/import_no  W41 ${Finnm} ${Troms}
W42_no1.vmap2.gz:
	${PDIR}/import_no  W42 ${Finnm} ${Troms}
W43_no1.vmap2.gz:
	${PDIR}/import_no  W43 ${Finnm}
W44_no1.vmap2.gz:
	${PDIR}/import_no  W44 ${Finnm}
W51_no1.vmap2.gz:
	${PDIR}/import_no  W51 ${Finnm}
W52_no1.vmap2.gz:
	${PDIR}/import_no  W52 ${Finnm}
X41_no1.vmap2.gz:
	${PDIR}/import_no  X41 ${Finnm} ${Troms}
X42_no1.vmap2.gz:
	${PDIR}/import_no  X42 ${Finnm} ${Troms}
X43_no1.vmap2.gz:
	${PDIR}/import_no  X43 ${Finnm}
X44_no1.vmap2.gz:
	${PDIR}/import_no  X44 ${Finnm}
X51_no1.vmap2.gz:
	${PDIR}/import_no  X51 ${Finnm}
X52_no1.vmap2.gz:
	${PDIR}/import_no  X52 ${Finnm}

########################

W33_se1.vmap2.gz:
	${PDIR}/import_se W33 SE/ln25

W34_se1.vmap2.gz:
	${PDIR}/import_se W34 SE/ln25

V34_se1.vmap2.gz:
	${PDIR}/import_se V34 SE/ln25

V41_se1.vmap2.gz:
	${PDIR}/import_se V41 SE/ln25

V42_se1.vmap2.gz:
	${PDIR}/import_se V42 SE/ln25
