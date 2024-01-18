.PHONY: build copy_output
.ONESHELL:

all: build copy_output

build:
	./build.sh

copy_output:
	mkdir -p ${CURDIR}/server/components
	cp ${CURDIR}/build/src/liboasis-gm.so ${CURDIR}/server/components

run:
	cd ${CURDIR}/server
	./samp03svr