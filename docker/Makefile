.PHONY: run_compose copy_output

all: run_compose copy_output

run_compose:
	docker compose -f ${CURDIR}/docker-compose.yaml run --rm builder

copy_output:
	mkdir -p ${CURDIR}/../server/components
	cp ${CURDIR}/build/src/liboasis-gm.so ${CURDIR}/../server/components