VERSION = 1
UNAME := $(shell uname -s)
CC = clang

OBJDIRDEV = bin/obj/dev
OBJDIRREL = bin/obj/rel
OBJDIRTEST = bin/obj/test

SOURCES = \
	$(wildcard src/overview/xdg-shell/*.c) \
	$(wildcard src/modules/zen_core/*.c) \
	$(wildcard src/modules/zen_math/*.c) \
	$(wildcard src/modules/zen_text/*.c) \
	$(wildcard src/overview/tree/*.c) \
	$(wildcard src/overview/config/*.c) \
	$(wildcard src/modules/json/*.c) \
	$(wildcard src/modules/storage/*.c)

SOURCES_MAIN = $(SOURCES) \
	src/overview/waytest.c

SOURCES_TEST = $(SOURCES) \
	src/overview/overview_test.c

CFLAGS = \
	-Isrc/overview/ \
	-Isrc/overview/xdg-shell \
	-I/usr/include \
	-Isrc/modules/zen_core \
	-Isrc/modules/zen_math \
	-Isrc/modules/zen_text \
	-I/usr/include/freetype2 \
	-Isrc/overview/tree \
	-Isrc/overview/config \
	-Isrc/modules/json \
	-Isrc/modules/storage
#	-I/usr/include/X11 \

LDFLAGS = \
	-lm \
	-lwayland-client \
	-lrt \
	-lfreetype
#	-lX11 \
#	-lXi \

OBJECTSDEV := $(addprefix $(OBJDIRDEV)/,$(SOURCES_MAIN:.c=.o))
OBJECTSREL := $(addprefix $(OBJDIRREL)/,$(SOURCES_MAIN:.c=.o))
OBJECTSTEST := $(addprefix $(OBJDIRTEST)/,$(SOURCES_TEST:.c=.o))

dev: $(OBJECTSDEV)
	$(CC) $^ -o bin/sway-overviewdev $(LDFLAGS) -fsanitize=address

rel: $(OBJECTSREL)
	$(CC) $^ -o bin/sway-overview $(LDFLAGS)

test: $(OBJECTSTEST)
	$(CC) $^ -o bin/sway-overview-test $(LDFLAGS) -fsanitize=address
	bin/sway-overview-test -c tst/test_config_1 -w tst/tst_ws1.json -t tst/tst_tree1.json -o tst/curr_tst1.bmp
	bin/sway-overview-test -c tst/test_config_2 -w tst/tst_ws1.json -t tst/tst_tree1.json -o tst/curr_tst2.bmp
	bin/sway-overview-test -c tst/test_config_1 -w tst/tst_ws2.json -t tst/tst_tree2.json -o tst/curr_tst3.bmp
	bin/sway-overview-test -c tst/test_config_2 -w tst/tst_ws2.json -t tst/tst_tree2.json -o tst/curr_tst4.bmp
	bin/sway-overview-test -c tst/test_config_1 -w tst/tst_ws3.json -t tst/tst_tree3.json -o tst/curr_tst5.bmp
	bin/sway-overview-test -c tst/test_config_2 -w tst/tst_ws3.json -t tst/tst_tree3.json -o tst/curr_tst6.bmp
	diff tst/tst1.bmp tst/curr_tst1.bmp
	diff tst/tst2.bmp tst/curr_tst2.bmp
	diff tst/tst3.bmp tst/curr_tst3.bmp
	diff tst/tst4.bmp tst/curr_tst4.bmp
	diff tst/tst5.bmp tst/curr_tst5.bmp
	diff tst/tst6.bmp tst/curr_tst6.bmp

$(OBJECTSDEV): $(OBJDIRDEV)/%.o: %.c
	mkdir -p $(@D)
	$(CC) -c $< -o $@ $(CFLAGS) -g -DDEBUG -DVERSION=0 -DBUILD=0 -fsanitize=address

$(OBJECTSREL): $(OBJDIRREL)/%.o: %.c
	mkdir -p $(@D)
	$(CC) -c $< -o $@ $(CFLAGS) -O3 -DVERSION=$(VERSION) -DBUILD=$(shell cat version.num)

$(OBJECTSTEST): $(OBJDIRTEST)/%.o: %.c
	mkdir -p $(@D)
	$(CC) -c $< -o $@ $(CFLAGS) -g -DDEBUG -DVERSION=0 -DBUILD=0 -fsanitize=address

clean:
	rm -f $(OBJECTSDEV) sway-overview
	rm -f $(OBJECTSREL) sway-overview
	rm -f $(OBJECTSTEST) sway-overview

vjump: 
	$(shell ./version.sh "$$(cat version.num)" > version.num)

install: rel
	/usr/bin/install -c -s -m 755 bin/sway-overview /usr/bin
	/usr/bin/install -d -m 755 /usr/share/sway-overview
	cp config /usr/share/sway-overview

remove:
	rm /usr/bin/sway-overview
	rm -r /usr/share/sway-overview
