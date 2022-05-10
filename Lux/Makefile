CXXFLAGS = -g -std=c++20
LDLIBS=-lm
CC=g++

frgb: frgb.o gamma_lut.o

gamma_lut.o: gamma_lut.cpp gamma_lut.hpp

frgb.o: frgb.cpp frgb.hpp gamma_lut.hpp