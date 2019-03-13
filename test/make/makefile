CPPLIST = $(wildcard *.cpp ./core/*.cpp)    		# get cpp file list
CLIST = $(wildcard *.c ./core/*.c)			# get c file list
#TARGET = $(patsubst %.cpp, %, $(CPPLIST)) # get corresponding target file
NOBJS=$(notdir $(OBJS))
OBJECTDIR=objs

CC = gcc
OO = g++
#CFLAGS   = -shared -Wall -g -O -fPIC -Wl,'-z muldefs'
#CFLAGS   = -fPIC -static-libgcc -Wl,-whole-archive -lm -Wl,-whole-archive -lstdc++ -Wl,-whole-archive -lc -Wl,-no-whole-archive -Wl,-O1  -shared -static
CFLAGS   = -fPIC -static-libgcc -Wl,-whole-archive -lm -Wl,-whole-archive -lstdc++ -Wl,-whole-archive -lc -Wl,-no-whole-archive -Wl,-O1 -shared
CCFLAGS  = -pipe -fvisibility=hidden -O2 -Wall -W -fPIC
CXXFLAGS = 
OBJDIR = objs
VPATH = /usr/local/include
TARGET = libappsafekb.so

INCLUDE += -I ./core -I./ 
INCLUDE += -I /home/xl/tool/openssl-1.0.1e
INCLUDE += -I /home/xl/tool/openssl-1.0.1e/include
LIBS+= /usr/local/ssl/lib/libcrypto.a



OBJ_DIR = objs
#OBJS = $(addprefix $(OBJ_DIR)/,$(patsubst %.c,%.o,$(notdir $(CLIST))))
#OBJS = $(patsubst %.c,%.o, $(notdir $(CLIST)))
OBJS = $(patsubst %.c,%.o, $(CLIST))


$(info $(OBJS))

$(TARGET) : $(OBJS) 
	$(OO) $(CFLAGS) -o $(TARGET) $(OBJECTDIR)/core/*.o $(OBJECTDIR)/*.o $(LIBS)
	mv $(TARGET) ../../env/x86

$(OBJECTDIR) :
	mkdir $(OBJECTDIR)
	mkdir $(OBJECTDIR)/core

$(OBJS) : %.o : %.c ./core
	$(CC) $(CCFLAGS) $(INCLUDE) -c $< -o $(OBJECTDIR)/$@



clean:
	rm -f $(TARGET)
	rm -f $(OBJECTDIR)/*.o $(OBJECTDIR)/core/*.o

info:$(CLIST)
	@echo $(basename $(CLIST))
	@echo $(OBJS)

ls: 
	@echo $(CLIST)

