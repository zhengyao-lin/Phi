TARGET = Grammar.o
OBJS = Parser.o \
	   Tokens.o

LLVMCONFIG = llvm-config
CPPFLAGS = `$(LLVMCONFIG) --cppflags` -std=c++11 -c -g -Wall -pedantic
LDFLAGS = `$(LLVMCONFIG) --ldflags` -lpthread -ldl -lz -lncurses -rdynamic
LIBS = `$(LLVMCONFIG) --libs`
INCLUDES = -I../

all: $(TARGET)

$(TARGET): $(OBJS)
	ld -r -o $@ $(OBJS)

#tokens
Tokens.cpp: glass.l Parser.hpp
	lex -o $@ $^

#parser
Parser.cpp: glass.y
	yacc -dv -o $@ $^
Parser.hpp: Parser.cpp

#default
%.o: %.cpp
	g++ -c $(CPPFLAGS) $(INCLUDES) -o $@ $<

clean:
	$(RM) -rf *.o *.output Parser.cpp Parser.hpp Tokens.cpp $(OBJS) $(TARGET)
