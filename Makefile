CFLAGS=-std=gnu99
CXXFLAGS=-g
#CXXFLAGS+= -pg
#CXXFLAGS+=-fprofile-arcs -ftest-coverage
fjc: FindJpeg.cc
validate: ValidJpeg.cc

find-bad:FindJpeg.cc ValidJpeg.cc

test:
	prove -I../Test-Directory/lib -r t
