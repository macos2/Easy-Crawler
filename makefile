all:clean build 

clean:
	rm -rf *.o Easy_Crawler
gresource.h:gresource.xml $(wildcard *.glade)
	glib-compile-resources --generate-header gresource.xml --target=gresource.h
gresource.c:gresource.xml $(wildcard *.glade)
	glib-compile-resources --generate-source gresource.xml --target=gresource.c
%.o:%.c
	gcc -g -c -w -o $@ $< -O3 `pkg-config --cflags gtk+-3.0 libxml-2.0 libsoup-2.4 `
build:gresource.c gresource.h $(subst .c,.o,$(wildcard *.c)) $(wildcard *.h) 
	gcc $(subst .c,.o,$(wildcard *.c)) -w -O3 -o Easy_Crawler `pkg-config --libs gtk+-3.0 libxml-2.0 libsoup-2.4 `
