.PHONY: build copy_output update_locales
.ONESHELL:

all: build copy_output

build:
	./build.sh

copy_output:
	mkdir -p ${CURDIR}/server/components
	cp ${CURDIR}/build/liboasis-gm.so ${CURDIR}/server/components

run:
	cd ${CURDIR}/server
	./samp03svr

update_locales:
	@./utils/update_translations.sh ./src ./server/locale ./server/locale/po messages en pt ru


DB_CONNECTION_STRING := "postgres://postgres:postgres@db:5432/samp?sslmode=disable"
migrate:
	migrate -path ../migrations -database $(DB_CONNECTION_STRING) up