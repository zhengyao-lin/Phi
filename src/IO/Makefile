TARGET = IO.o
OBJS = \
	IOSetting.o

LLVMCONFIG = llvm-config
CPPFLAGS = `$(LLVMCONFIG) --cppflags` -std=c++11 -c -g -Wall -pedantic
LDFLAGS = `$(LLVMCONFIG) --ldflags` -lpthread -ldl -lz -lncurses -rdynamic
LIBS = `$(LLVMCONFIG) --libs`
INCLUDES = -I../

all: $(TARGET)

$(TARGET): $(OBJS)
	ld -r -o $@ $(OBJS)

#default
%.o: %.cpp
	g++ -c $(CPPFLAGS) $(INCLUDES) -o $@ $<

clean:
	$(RM) -rf *.o $(OBJS) $(TARGET)
