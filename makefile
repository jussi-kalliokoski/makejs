LIBDIR = /usr/share
BINDIR = /usr/bin

all: compile

compile:
	mkdir bin -p
	g++ -o bin/makejs makejs.cc -lv8

install:
	cp bin/makejs $(BINDIR)/
	mkdir $(LIBDIR)/makejs -p
	cp lib/* $(LIBDIR)/makejs/ -f

uninstall:
	rm $(BINDIR)/makejs -f
	rm $(LIBDIR)/makejs/* -f
	rmdir $(LIBDIR)/makejs

clean:
	rm bin/makejs
