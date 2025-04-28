# Корневой Makefile для всего проекта myRPC

# Список модулей, которые нужно собирать и чистить
SUBDIRS	= libmysyslog libmysyslog-json myRPC-client myRPC-server

# Цель по умолчанию — собрать всё
all:
	for d in $(SUBDIRS); do \
		make -C $$d all; \
	done

# Упаковать .deb только для client и server
deb:
	for d in myRPC-client myRPC-server; do \
		make -C $$d deb; \
	done

# Очистить все модули
clean:
	for d in $(SUBDIRS); do \
		make -C $$d clean; \
	done

.PHONY: all deb clean
