TARGET = testbed
OBJS = Grammar/Grammar.o \
	   ErrorMsg/ErrorMsg.o \
	   IO/IO.o \
	   Lib/Lib.o \
	   CodeGen/CodeGen.o \
	   main.o

LLVMCONFIG = llvm-config
CPPFLAGS = `$(LLVMCONFIG) --cppflags` -std=c++11 -g -Wall -pedantic
LDFLAGS = `$(LLVMCONFIG) --ldflags` -lpthread -ldl -lz -lncurses -rdynamic
LIBS = `$(LLVMCONFIG) --libs`

all: $(TARGET)

$(TARGET): $(OBJS)
	g++ -o $@ $(OBJS) $(LIBS) $(LDFLAGS)

testval:
	g++ -c $(CPPFLAGS) test.cpp -o testval.o
	g++ testval.o -o testval $(LIBS) $(LDFLAGS)

#default
%.o: %.cpp
	g++ -c $(CPPFLAGS) -o $@ $<

Grammar/Grammar.o:
	cd Grammar; $(MAKE)

CodeGen/CodeGen.o:
	cd CodeGen; $(MAKE)

ErrorMsg/ErrorMsg.o:
	cd ErrorMsg; $(MAKE)

IO/IO.o:
	cd IO; $(MAKE)

Lib/Lib.o:
	cd Lib; $(MAKE)

clean:
	cd Grammar; $(MAKE) clean
	cd CodeGen; $(MAKE) clean
	cd ErrorMsg; $(MAKE) clean
	cd IO; $(MAKE) clean
	cd Lib; $(MAKE) clean
	$(RM) -rf *.o *.ll $(TARGET) testval
