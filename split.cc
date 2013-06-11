#include <stdio.h>
#include "ValidJpeg.hh"

extern bool valid_jpeg_debug;

int main (int ac, const char ** argv)
{
  valid_jpeg_debug=true;

  if (ac != 3)
    {
      fprintf(stderr, "Usage: %s file template\n", *argv);
      return 1;
    }
  ValidJpeg jpeg;
  
  const char * fn = argv[1];
  const char * tm = argv[2];
  
  if (jpeg.open(fn) == false) 
    {
      perror(fn);
      return 1;
      
    }
  int rv = jpeg.valid_jpeg(tm);
  printf("%s: %d\n", fn, rv);
}

