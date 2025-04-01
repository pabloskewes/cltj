
all: main rank select ranksel access nada memory

nada: nada.o hybridId.o leafId.o hybridBV.o staticBV.o leafBV.o basics.o
	gcc -O9 -o nada nada.o hybridId.o leafId.o hybridBV.o staticBV.o leafBV.o basics.o

nada.o: nada.c include/hybridBV/hybridId.h include/hybridBV/leafId.h include/hybridBV/hybridBV.h include/hybridBV/staticBV.h include/hybridBV/leafBV.h
	gcc -O9 -c nada.c

main: main.o hybridId.o leafId.o hybridBV.o staticBV.o leafBV.o basics.o

memory: memory.o hybridId.o leafId.o hybridBV.o staticBV.o leafBV.o basics.o
	gcc -O9 -o memory memory.o hybridId.o leafId.o hybridBV.o staticBV.o leafBV.o basics.o

memory.o: memory.c include/hybridBV/hybridId.h include/hybridBV/leafId.h include/hybridBV/hybridBV.h include/hybridBV/staticBV.h include/hybridBV/leafBV.h
	gcc -O9 -c memory.c

rank: rank.o hybridId.o leafId.o hybridBV.o staticBV.o leafBV.o basics.o
	gcc -O9 -o rank rank.o hybridId.o leafId.o hybridBV.o staticBV.o leafBV.o basics.o

rank.o: rank.c include/hybridBV/hybridId.h include/hybridBV/leafId.h include/hybridBV/hybridBV.h include/hybridBV/staticBV.h include/hybridBV/leafBV.h
	gcc -O9 -c rank.c

select: select.o hybridId.o leafId.o hybridBV.o staticBV.o leafBV.o basics.o
	gcc -O9 -o select select.o hybridId.o leafId.o hybridBV.o staticBV.o leafBV.o basics.o

select.o: select.c include/hybridBV/hybridId.h include/hybridBV/leafId.h include/hybridBV/hybridBV.h include/hybridBV/staticBV.h include/hybridBV/leafBV.h
	gcc -O9 -c select.c

ranksel: ranksel.o hybridId.o leafId.o hybridBV.o staticBV.o leafBV.o basics.o
	gcc -O9 -o ranksel ranksel.o hybridId.o leafId.o hybridBV.o staticBV.o leafBV.o basics.o

ranksel.o: ranksel.c include/hybridBV/hybridId.h include/hybridBV/leafId.h include/hybridBV/hybridBV.h include/hybridBV/staticBV.h include/hybridBV/leafBV.h
	gcc -O9 -c ranksel.c

access: access.o hybridId.o leafId.o hybridBV.o staticBV.o leafBV.o basics.o
	gcc -O9 -o access access.o hybridId.o leafId.o hybridBV.o staticBV.o leafBV.o basics.o

access.o: access.c include/hybridBV/hybridId.h include/hybridBV/leafId.h include/hybridBV/hybridBV.h include/hybridBV/staticBV.h include/hybridBV/leafBV.h
	gcc -O9 -c access.c

main: main.o hybridId.o leafId.o hybridBV.o staticBV.o leafBV.o basics.o
	gcc -O9 -o main main.o hybridId.o leafId.o hybridBV.o staticBV.o leafBV.o basics.o

main.o: main.c include/hybridBV/hybridId.h include/hybridBV/leafId.h include/hybridBV/hybridBV.h include/hybridBV/staticBV.h include/hybridBV/leafBV.h
	gcc -O9 -c main.c

hybridId.o: src/hybridBV/hybridId.c include/hybridBV/hybridId.h include/hybridBV/leafId.h include/hybridBV/basics.h
	gcc -O9 -c hybridId.c

leafId.o: src/hybridBV/leafId.c include/hybridBV/leafId.h include/hybridBV/basics.h
	gcc -O9 -c leafId.c

hybridBV.o: src/hybridBV/hybridBV.c include/hybridBV/hybridBV.h include/hybridBV/staticBV.h include/hybridBV/leafBV.h include/hybridBV/basics.h
	gcc -O9 -c hybridBV.c

staticBV.o: src/hybridBV/staticBV.c include/hybridBV/staticBV.h include/hybridBV/basics.h
	gcc -O9 -c staticBV.c

leafBV.o: src/hybridBV/leafBV.c include/hybridBV/leafBV.h include/hybridBV/basics.h
	gcc -O9 -c leafBV.c

basics.o: src/hybridBV/basics.c include/hybridBV/basics.h
	gcc -O9 -c basics.c
