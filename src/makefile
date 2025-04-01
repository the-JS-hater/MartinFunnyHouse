commondir = ../common/

all: final

final : main.cpp $(commondir)GL_utilities.c $(commondir)LoadTGA.c $(commondir)Linux/MicroGlut.c
	gcc -Wall -Wextra -o martin -I$(commondir) -I../common/Linux -DGL_GLEXT_PROTOTYPES main.cpp $(commondir)GL_utilities.c $(commondir)LoadTGA.c $(commondir)Linux/MicroGlut.c -lXt -lX11 -lGL -lm -lstdc++

run: final
	./martin
