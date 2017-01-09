# All variables suffixed with 1 denote HDF4, 2 HDF5
# This makefile is currently set up to be run on the 
# Blue Waters computer.
CC=cc
CFLAGS=-c -g -O0 -Wall -std=c99
LINKFLAGS= -g -std=c99
INCLUDE1=.
INCLUDE2=
LIB1=.
LIB2=
TARGET=./exe/TERRArepackage
SRCDIR=./src
OBJDIR=./obj
DEPS=$(OBJDIR)/main.o $(OBJDIR)/libTERRA.o $(OBJDIR)/MOPITT.o $(OBJDIR)/CERES.o $(OBJDIR)/MODIS.o $(OBJDIR)/ASTER.o $(OBJDIR)/MISR.o

all: $(TARGET)

$(TARGET): $(DEPS)
	$(CC) $(LINKFLAGS) $(DEPS) -L$(LIB1) -I$(INCLUDE1) -lmfhdf -ldf -lz -ljpeg -o $(TARGET)
	
$(OBJDIR)/main.o: $(SRCDIR)/main.c
	$(CC) $(CFLAGS) -L$(LIB1) -I$(INCLUDE1) $(SRCDIR)/main.c -o $(OBJDIR)/main.o
	
$(OBJDIR)/libTERRA.o: $(SRCDIR)/libTERRA.c
	$(CC) $(CFLAGS) -I$(INCLUDE1) $(SRCDIR)/libTERRA.c -o $(OBJDIR)/libTERRA.o
	
$(OBJDIR)/MOPITT.o: $(SRCDIR)/MOPITT.c
	$(CC) $(CFLAGS) -I$(INCLUDE1) $(SRCDIR)/MOPITT.c -o $(OBJDIR)/MOPITT.o
	
$(OBJDIR)/CERES.o: $(SRCDIR)/CERES.c
	$(CC) $(CFLAGS) -I$(INCLUDE1) $(SRCDIR)/CERES.c -o $(OBJDIR)/CERES.o
	
$(OBJDIR)/MODIS.o: $(SRCDIR)/MODIS.c
	$(CC) $(CFLAGS) -I$(INCLUDE1) $(SRCDIR)/MODIS.c -o $(OBJDIR)/MODIS.o
	
$(OBJDIR)/ASTER.o: $(SRCDIR)/ASTER.c
	$(CC) $(CFLAGS) -I$(INCLUDE1) $(SRCDIR)/ASTER.c -o $(OBJDIR)/ASTER.o
	
$(OBJDIR)/MISR.o: $(SRCDIR)/MISR.c
	$(CC) $(CFLAGS) -I$(INCLUDE1) $(SRCDIR)/MISR.c -o $(OBJDIR)/MISR.o

clean:
	rm -f $(TARGET) $(OBJDIR)/*.o
	
run:
	$(TARGET) out.h5
