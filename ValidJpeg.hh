#include <cstdio>

class ValidJpeg 
{
 private:
  FILE *fh;
 public:

enum valid_jpeg_status 
  {
    ok = 0,
    missing_ff,
    trailing_junk,
    stray_0,
    short_file,
  };

  ValidJpeg()
    {
      fh=0;
    }

  ~ValidJpeg() 
  {
    if (fh != 0)
      fclose (fh);
  }

  bool open(const char*);
  int short_jpeg();
  int valid_jpeg(bool seek_over_entropy=true);
};

