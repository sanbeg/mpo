#include <stdio.h>
#include "ValidJpeg.hh"

extern bool valid_jpeg_debug;

int main (int ac, const char ** argv)
{
  valid_jpeg_debug=true;

  if (ac < 2)
    {
      fprintf(stderr, "Usage: %s file ...\n", *argv);
      return 1;
    }
  ValidJpeg jpeg;
  
  for (int i=1; i<ac; ++i)
    {
      const char * fn = argv[i];
      if (jpeg.open(fn) == false) 
	{
	  perror(fn);
	  continue;
	}
      int rv = jpeg.valid_jpeg( false );
      printf("%s: %d\n", fn, rv);
    }
}
