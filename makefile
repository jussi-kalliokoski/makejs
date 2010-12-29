all: compile

compile:
	g++ -o bin/makejs makejs.cc -lv8

install:
	cp bin/makejs /usr/bin/

uninstall:
	rm /usr/bin/makejs

clean:
	rm bin/makejs
