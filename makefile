###########################
#	Makefile
###########################

#source object target
SOURCE 	:= test.cpp pthreadpool.h pthreadpool.cpp timer.cpp network.cpp network.h server.h server.cpp package.h 
OBJS 	:= 
TARGET	:= test

#compile and lib parameter
CXX		:= g++
LIBS	:= 
LDFLAGS	:= -lpthread
DEFINES	:=
CFLAGS	:= 
CXXFLAGS:= -std=c++11
 
.PHONY: clean

#link parameter
LIB := 

#link
$(TARGET): $(OBJS) $(SOURCE)
	$(CXX) -o $@ $(CFLAGS) $(CXXFLAGS) $(LDFLAGS) $^

#clean
clean:
	rm -fr *.o
	rm -fr *.gch
	rm -rf test

