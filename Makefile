all: shelperbot

shelperbot: main.o jsmn.o
	g++ main.o jsmn.o -lcurl -o shelperbot

main.o: main.cpp
	g++ -c -Ijsmn main.cpp

jsmn.o: jsmn/jsmn.c
	g++ -c jsmn/jsmn.c

clean:
	rm -rf *.o shelperbot
