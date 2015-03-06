TARGET = testbed
OBJS = Grammar/Grammar.o \
	   CodeGen/CodeGen.o \
	   ErrorMsg/ErrorMsg.o \
	   llc.o \
	   main.o

LLVMCONFIG = llvm-config
CPPFLAGS = `$(LLVMCONFIG) --cppflags` -std=c++11 -g -Wall -pedantic
LDFLAGS = `$(LLVMCONFIG) --ldflags` -lpthread -ldl -lz -lncurses -rdynamic
LIBS = `$(LLVMCONFIG) --libs`

all: $(TARGET)

$(TARGET): $(OBJS)
	g++ -o $@ $(OBJS) $(LIBS) $(LDFLAGS)

#default
%.o: %.cpp
	g++ -c $(CPPFLAGS) -o $@ $<

Grammar/Grammar.o:
	cd Grammar; $(MAKE)

CodeGen/CodeGen.o:
	cd CodeGen; $(MAKE)

ErrorMsg/ErrorMsg.o:
	cd ErrorMsg; $(MAKE)

clean:
	cd Grammar; $(MAKE) clean
	cd CodeGen; $(MAKE) clean
	cd ErrorMsg; $(MAKE) clean
	$(RM) -rf *.o *.ll $(TARGET)
