
all: circle
	cd src && make

circle: circle-stdlib/Config.mk

circle-stdlib/Config.mk:
	cd circle-stdlib && ./configure && make

clean:
	cd circle-stdlib && make mrproper
