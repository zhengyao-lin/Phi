TARGET = Lib.o
OBJS = \
	glmake/glmake_t.o

LLVMCONFIG = llvm-config
CPPFLAGS = `$(LLVMCONFIG) --cppflags` -std=c++11 -c -g -Wall -pedantic
LDFLAGS = `$(LLVMCONFIG) --ldflags` -lpthread -ldl -lz -lncurses -rdynamic
LIBS = `$(LLVMCONFIG) --libs`
INCLUDES = -I../

all: $(TARGET)

$(TARGET): $(OBJS)
	ld -r -o $@ $(OBJS)

glmake/glmake_t.o:
	cd glmake; $(MAKE)

#default
%.o: %.cpp
	g++ -c $(CPPFLAGS) $(INCLUDES) -o $@ $<

clean:
	$(RM) -rf *.o $(OBJS) $(TARGET)
	cd glmake; $(MAKE) clean
