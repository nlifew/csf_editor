


DEBUG   =   0

CC  =   gcc
CPP	=	g++
RM  =   rm
OUT	=	csf_editor.exe # for windows


CFLAGS	=	-std=c99
CPPFLAGS	=	-std=c++11

ifeq (1, $(DEBUG))
CFLAGS	+=	-g -O0
CPPFLAGS	+=	-g -O0
else
CFLAGS	+=	-O2
CPPFLAGS	+=	-O2
endif


#LDFLAGS	=	
#LIBS	=	


all	:	 main.o hashmap.o
	$(CPP) $(CPPFLAGS) -o $(OUT) main.o hashmap.o

main.o	:	main.cpp cmdline.hpp csf.hpp label.hpp string.hpp \
		header.hpp global.hpp string_utils.hpp
	$(CPP) $(CPPFLAGS) -c -o main.o main.cpp

hashmap.o	:	hashmap.h hashmap.c
	$(CC) -c $(CFLAGS) -o hashmap.o hashmap.c


.PHONY	:	clean
clean	:
	-$(RM) $(OUT) *.o
