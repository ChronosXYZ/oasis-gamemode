all:
	@echo $(CURDIR)
	docker compose -f ${CURDIR}/docker-compose.yaml run --rm builder
	mkdir -p ${CURDIR}/../server/components
	cp ${CURDIR}/build/src/liboasis-gm.so ${CURDIR}/../server/components