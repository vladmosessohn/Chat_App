CC = g++ -std=c++17 -Wall -Wextra -lm
USER1 = vlad
USER2 = andrei
USER3 = marcel
IP = 127.0.0.1
PORT = 1234

all: build

build: server subscriber

server: server.o
	$(CC) $^ -o $@

server.o : server.cpp
	$(CC) $^ -c

subscriber: subscriber.o
	$(CC) $^ -o $@

subscriber.o: subscriber.cpp
	$(CC) $^ -c

clang: 
	clang-format -i -style=google *.cpp

messages:
	python3 udp_client.py $(IP) $(PORT)

serverinit:
	./server $(PORT)

client1:
	./subscriber $(USER1) $(IP) $(PORT)

client2:
	./subscriber $(USER2) $(IP) $(PORT)

client3:
	./subscriber $(USER3) $(IP) $(PORT)

clean:
	rm *.o server subscriber




