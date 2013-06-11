#include "ValidJpeg.hh"
#include <arpa/inet.h>
#include <cstring>

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


  
int ValidJpeg::valid_jpeg (bool seek_over_entropy) 
{
  char in_entropy=0;

  while (! feof(fh))
    {
      unsigned char marker=0;

      if ( fread(&marker, 1, 1, fh) < 1 )
	return short_file;


      if ( in_entropy ) 
	{
	  if ( marker == 0xff ) 
	    {
	      if ( fread(&marker, 1, 1, fh) < 1 )
		return short_file;
	      if ( marker == 0 )
		//escaped 0xff00
		continue;
	      else if ( (marker >= 0xd0) && (marker <= 0xd7) )
		//RST
		continue;
	      else 
		{
		  //marker after data may be padded
		  while ( marker == 0xff )
		    if ( fread(&marker, 1, 1, fh) < 1 )
		      return short_file;
		  in_entropy = 0;
		  
		}
	    } else continue;
	  
	}
      else 
	{
	if (marker != 0xff)
	  return missing_ff;
	
	
	if ( fread(&marker, 1, 1, fh) < 1 )
	  return short_file;
	}
      /*	
      if ( marker == 0 )
	return BAD_;
      */

      if (marker == 0xd8)
	debug("got start (SOI)");
      else if (marker == 0xd9) 
	{
	  #if 0
	  unsigned char junk;
	  fread(&junk, 1, 1, fh);
	  if (feof(fh))
	    return ok;
	  else
	    return trailing_junk;
	  #else
	  printf("got end (EOI)\n");
	  #endif
	  
	}
      
      else if ( (marker >= 0xd0) && (marker <= 0xd7) ) 
	{
	  /*
	  if (valid_jpeg_debug)
	    printf("got RST%d\n", marker-0xd0+1);
	  */
	  in_entropy = 1;
	  
	}
      
      else if ( marker == 0x01 )
	debug("got TEM");
      
      else
	{
	  unsigned short length;
	  
	  if (marker == 0xda /* SOS - start of scan */) {
	    debug("got SOS");
	    
	    if ( seek_over_entropy ) 
	      {
		if( fseek(fh,-2,SEEK_END) ) 
		  return -1;
		else
		  continue;
	      }
	    else 
	      {
		debug("...entropy");
		in_entropy = 1;
		
	      }
	  } 


	  else if ((marker >= 0xe0) && (marker <= 0xffef)) 
	    {

	      fread(&length, 2, 1, fh);
	      length = ntohs(length);

	      char buf[64];
	      int buf_len = length-2;

	      if (buf_len > 64)
		buf_len = 64;
	      int n_read = fread(buf, 1, buf_len, fh);
	      buf[buf_len-1]=0;
	      
	      if (strlen(buf) < 63)
		printf("APP%d=%s\n", marker-0xe0, buf);
	      else
		printf ("got APP%d\n", marker-0xe0);
	      
	      printf("  length is %d\n", length);
	      
	      length -= n_read;
	      
		for (int j=2; j<length; ++j)
		  {
		    char junk;
		    if ( fread(&junk, 1, 1, fh) < 1 )
		      return short_file;
		  }
	      
	    }
	  else 
	    {
	      
	      printf ("got mark %x\n", marker);
	      
	      fread(&length, 2, 1, fh);
	      length = ntohs(length);
	      
	      
	      
	      if (valid_jpeg_debug) printf ("Length is %d\n", length);
#if 1
	      if (length > 512) 
		fseek(fh,length-2,SEEK_CUR);
	      else
#endif
		for (int j=2; j<length; ++j)
		  {
		    char junk;
		    if ( fread(&junk, 1, 1, fh) < 1 )
		      return short_file;
		  }
	    }
	}
      
      
    }
  return short_file;
}
