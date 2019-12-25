
all: circle
	cd src && make

clean:
	cd src && make clean

circle: circle-stdlib/Config.mk

circle-stdlib/Config.mk:
	cd circle-stdlib && ./configure && make

mrproper:
	cd circle-stdlib && make mrproper
