#include "ValidJpeg.hh"
#include <arpa/inet.h>
#include <cstring>
#include <cstdlib>

bool valid_jpeg_debug = false;

static void debug(const char * msg)
{
  if (valid_jpeg_debug)
    puts(msg);
}

bool ValidJpeg::open( const char * fn ) 
{
  if (fh != 0)
    fclose (fh);
  
  fh = fopen( fn, "r" );
  if (fh == 0) 
    return false;
  else
    return true;
  
}


int ValidJpeg::short_jpeg () 
{
  if( fseek(fh,-2,SEEK_END) ) 
    return -1;

  unsigned char bytes[2];
  int n_read = fread(bytes, 1, 2, fh);
  if ( (n_read==2) and (bytes[0]==0xff) and (bytes[1]==0xd9) )
    return 0;
  return 1;
}


  
int ValidJpeg::valid_jpeg (const char * format) 
{
  char in_entropy=0;
  char have_eoi = 0;
  int image_cnt = 0;
  FILE * image_fh = 0;
  
  while (! feof(fh))
    {
      unsigned char marker=0;

      if ( fread(&marker, 1, 1, fh) < 1 )
	return have_eoi ? ok : short_file;


      if ( in_entropy ) 
	{
	  if ( marker == 0xff ) 
	    {
	      if ( fread(&marker, 1, 1, fh) < 1 )
		return short_file;
	      if ( marker == 0 ) 
		{
		  //escaped 0xff00
		  if (image_fh)
		    fwrite("\xff\x00", 1, 2, image_fh);
		  continue;
		}
	      
	      else if ( (marker >= 0xd0) && (marker <= 0xd7) )
		{
		//RST
		  if (image_fh) 
		    {
		      fwrite("\xff", 1, 1, image_fh);
		      fwrite(&marker, 1, 1, image_fh);
		    }
		  continue;
		}
	      
	      else 
		{
		  //marker after data may be padded
		  while ( marker == 0xff )
		    if ( fread(&marker, 1, 1, fh) < 1 )
		      return short_file;
		  in_entropy = 0;
		  
		}
	    } 
	  else 
	    {
	      if (image_fh)
		fwrite(&marker, 1, 1, image_fh);
	      
	      continue;
	    }
	  
	  
	}
      else 
	{
	  //getting 0s after EOI
	  if (have_eoi)
	    while (marker == 0)
	      if ( fread(&marker, 1, 1, fh) < 1 )
		return short_file;
		
	  if (marker != 0xff)
	    return missing_ff;
	  
	  
	  if ( fread(&marker, 1, 1, fh) < 1 )
	    return short_file;
	}

      if ( marker == 0 )
	return stray_0;


      if (marker == 0xd8) 
	{
	  debug("got start (SOI)");
	  have_eoi = 0;
	  if (format) 
	    {
	      char * str;
	      asprintf(&str, format, ++image_cnt);
	      image_fh = fopen(str, "w");
	      free(str);
	      if (image_fh)
		fwrite("\xff\xd8", 1, 2, image_fh);
	      
	    }
	  
	}
      
      else if (marker == 0xd9) 
	{
	  debug("got end (EOI)");
	  have_eoi = 1;
	  if (image_fh) 
	    {
	      fwrite("\xff\xd9", 1, 2, image_fh);
	      fclose(image_fh);
	      image_fh = 0;
	    }
	}
      
      else if ( (marker >= 0xd0) && (marker <= 0xd7) ) 
	{
	  //shouldn't see RST ouside of entropy
	  if (valid_jpeg_debug)
	    printf("got bogus RST%d\n", marker-0xd0+1);
	}
      
      else if ( marker == 0x01 ) 
	{
	  debug("got TEM");
	  if (image_fh) 
	      fwrite("\xff\x01", 1, 2, image_fh);
	}
      
      else
	{
	  unsigned short length, net_length;
	  bool skip_me = false;
	  
	  if (marker == 0xda /* SOS - start of scan */) {
	    debug("got SOS");
	    in_entropy = 1;
	    if (image_fh) 
	      fwrite("\xff\xda", 1, 2, image_fh);
	  } 


	  else if ((marker >= 0xe0) && (marker <= 0xffef)) 
	    {
	      // APPx
	      char app[5]="";
	      int nr=0;
	      
	      fread(&net_length, 2, 1, fh);
	      length = ntohs(net_length);

	      if (valid_jpeg_debug)
		printf ("got APP%d length=%d\n", marker-0xe0, length);

	      //trim some extraneous app2 blocks
	      if ( (marker-0xe0 == 2) && (length >= 7) ) 
		{
		  nr=fread(app, 1, 5, fh);
		  if (strncmp("MPF", app, 4) == 0) 
		    {
		      debug("got APP2/MPF");
		      skip_me = true;
		    } 
		  length -= nr;
		}
	      // TODO - should also strip out MPF exif tags from app1

	      if (image_fh && ! skip_me) 
		{
		  fwrite("\xff",  1, 1, image_fh);
		  fwrite(&marker, 1, 1, image_fh);
		  fwrite(&net_length, 2, 1, image_fh);
		  if (nr) fwrite(app, 1, nr, image_fh);
		}
	      
	      for (int j=2; j<length; ++j)
		{
		  char byte;
		  if ( fread(&byte, 1, 1, fh) < 1 )
		    return short_file;
		  else if (image_fh && ! skip_me)
		    fwrite(&byte, 1, 1, image_fh);
		  
		}
	      
	    }
	  else 
	    {
	      if (valid_jpeg_debug)
		printf ("got mark %x\n", marker);
	      
	      fread(&length, 2, 1, fh);
	      if (image_fh)
		{
		  fwrite("\xff",  1, 1, image_fh);
		  fwrite(&marker, 1, 1, image_fh);
		  fwrite(&length, 2, 1, image_fh);
		}
	      length = ntohs(length);
	      
	      if (valid_jpeg_debug) printf ("Length is %d\n", length);

	      for (int j=2; j<length; ++j)
		{
		  char byte;
		  if ( fread(&byte, 1, 1, fh) < 1 )
		    return short_file;
		  else if (image_fh)
		    fwrite(&byte, 1, 1, image_fh);
		}
	    }
	}
      
      
    }
  return short_file;
}
