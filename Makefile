all: myls myshell

myls: myshell
	g++ -Wall -o myls myls.cpp

myshell:
	g++ -Wall -o myshell myshell.cpp

clean:
	@rm myshell myls