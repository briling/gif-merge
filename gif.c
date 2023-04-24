#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>

#define HEAD_SIZE   13
#define GEX_SIZE     8
#define PHEAD_SIZE  10

typedef struct {
  uint8_t  sv[6];
  uint16_t w;
  uint16_t h;
  uint8_t  f;
  uint8_t  bg;
  uint8_t  r;
  int      CT;
  int      Color;
  int      SF;
  int      Size;
} header;

typedef struct {
  uint16_t  l;
  uint16_t  t;
  uint16_t  w;
  uint16_t  h;
  uint8_t   f;
  int       CT;
  int       I;
  int       SF;
  int       Size;
} picheader;

typedef struct {
  uint8_t head[HEAD_SIZE];
  uint8_t * pal;
  uint8_t gex[GEX_SIZE];
  uint8_t phead[PHEAD_SIZE];
  uint8_t MC;
  uint8_t * data;
  uint8_t z;
  /* ======= */
  int n;    /* palette */
  int size; /* data    */
  int k;    /* blocks  */
} picture;

int head_read(uint8_t head[HEAD_SIZE], FILE  * f){
  header  h;
  fread(head, HEAD_SIZE, 1, f);
  h.w     =  *(uint16_t *)(head+6        );
  h.h     =  *(uint16_t *)(head+6+2      );
  h.f     =  *(uint8_t  *)(head+6+2+2    );
  h.bg    =  *(uint8_t  *)(head+6+2+2+1  );
  h.r     =  *(uint8_t  *)(head+6+2+2+1+1);
  h.CT    = !!(h.f &  128);
  h.Color =   (h.f & (64|32|16))/16;
  h.SF    = !!(h.f &  8);
  h.Size  =   (h.f & (4|2|1));
#if 0
  printf("%.6s\n", head);
  printf("w     %d\n", h.w );
  printf("h     %d\n", h.h );
  printf("bg    %d\n", h.bg);
  printf("r     %d\n", h.r );
  printf("CT    %d\n", h.CT);
  printf("Color %d\n", h.Color);
  printf("SF    %d\n", h.SF);
  printf("Size  %d\n", h.Size);
#endif
  if(h.CT == 0){
    abort();
  }
  return ( 2 << h.Size );
}

uint8_t * pal_read(int n, FILE * f){
  int       N;
  uint8_t * pal;
  N    = n*3;
  pal  = malloc(N);
  fread (pal, N, 1, f);
#if 0
  for(int i=0; i<n; i++){
    printf("%3d    %3d    %3d\n", pal[3*i], pal[3*i+1], pal[3*i+2]);
  }
#endif
  return pal;
}

void phead_read(uint8_t phead[PHEAD_SIZE-1], FILE  * f){
  picheader  ph;
  fread(phead, PHEAD_SIZE-1, 1, f);
  ph.l     =  *(uint16_t *)(phead         );
  ph.t     =  *(uint16_t *)(phead+2       );
  ph.w     =  *(uint16_t *)(phead+2+2     );
  ph.h     =  *(uint16_t *)(phead+2+2+2   );
  ph.f     =  *(uint8_t  *)(phead+2+2+2+2 );
  ph.CT    = !!(ph.f &  128);
  ph.I     = !!(ph.f &   64);
  ph.SF    = !!(ph.f &   32);
  ph.Size  =   (ph.f &  (4|2|1));
#if 0
  printf("l     %d\n", ph.l);
  printf("t     %d\n", ph.t);
  printf("w     %d\n", ph.w);
  printf("h     %d\n", ph.h);
  printf("CT    %d\n", ph.CT);
  printf("I     %d\n", ph.I);
  printf("SF    %d\n", ph.SF);
  printf("Size  %d\n", ph.Size);
#endif
  if(ph.CT == 1){
    abort();
  }
  return;
}

picture pic_read(char * s){

  FILE * f = fopen(s, "r");
  if(!f){
    fprintf(stderr, "cannot open file %s\n", s);
    abort();
  }
  fseek(f, 0, SEEK_END);
  long max = ftell(f);
  fseek(f, 0, SEEK_SET);

  picture pic;
  pic.n   = head_read(pic.head, f);
  pic.pal = pal_read(pic.n, f);

  memset(pic.gex, 0, GEX_SIZE);
  uint8_t z;
  do{
    fread(&z, 1, 1, f);
    if (z == '!'){
      fread(&z,   1, 1, f);
      if (z == 0xF9){
        pic.gex[0] = '!';
        pic.gex[1] = z;
        fread(pic.gex+2, 6, 1, f);
#if 0
        printf("%d\n", *(uint8_t  *)(pic.gex+2));
        printf("%d\n", *(uint8_t  *)(pic.gex+3));
        printf("%d\n", *(uint16_t *)(pic.gex+4));
        printf("%d\n", *(uint8_t  *)(pic.gex+6));
#endif
      }
    }
  } while (z != ',');

  pic.phead[0] = z;
  phead_read(pic.phead+1, f);
  fread(&(pic.MC), 1, 1, f);
  pic.size = max - ftell(f);
  pic.data = malloc(pic.size);

  pic.k  = 0;
  long m = 0;
  fread(pic.data+m, 1, 1, f);
  do{
    pic.k++;
    fread(pic.data+m+1, pic.data[m], 1, f);
    m = m + pic.data[m] + 1;
    fread(pic.data+m, 1, 1, f);
  }
  while(pic.data[m] != 0) ;

  pic.size = m + 1;
  pic.data = realloc(pic.data, pic.size);
  if(!pic.data){
    abort();
  }
  pic.z = ';';
  fclose(f);
  return pic;
}

void frame_write(picture pic, uint16_t delay, uint8_t hf, FILE * f){
  uint8_t * phf;
  phf    =  (uint8_t  *)(pic.phead + 9 );
  *phf   = 0x00;
  *phf  |= 128;
  *phf  |= (hf & 8 ? 32 : 0);
  *phf  |= (hf & (4|2|1));
  if(pic.gex[0]){
    *(uint16_t *)(pic.gex+4) = delay;
    fwrite(pic.gex, GEX_SIZE, 1, f);
  }
  else{
    pic.gex[0] = '!';
    pic.gex[1] = 0xF9;
    pic.gex[2] = 0x04;
    pic.gex[3] = 0x00;
    pic.gex[6] = 0x00;
    pic.gex[7] = 0x00;
    *(uint16_t *)(pic.gex+4) = delay;
    fwrite(pic.gex, GEX_SIZE, 1, f);
  }
  fwrite(pic.phead, PHEAD_SIZE, 1, f);
  fwrite(pic.pal, pic.n*3, 1, f);
  fwrite(&(pic.MC), 1 , 1, f);
  fwrite((pic.data), pic.size, 1, f);
}

uint8_t head_cl(picture * pic){
  uint8_t * hfp;
  uint8_t   hf;
  hfp    =  (uint8_t  *)(pic->head + 10);
  hf     =  *hfp;
  *hfp  &= ~(128 |4 | 2 | 1);
  return hf;
}

void pic_free(picture pic){
  free(pic.pal);
  free(pic.data);
}

void pics_write(uint16_t delay, int loop, int n, char * fnames[], FILE * fout){
  for(int i=0; i<n; i++){
    picture pic = pic_read(fnames[i]);
    uint8_t hf  = head_cl(&pic);
    if(i==0){
      pic.head[4] = '9'; // GIF87a -> GIF89a
      fwrite(pic.head, HEAD_SIZE, 1, fout);
      if(loop){
        int8_t pex[] = {
          0x21, 0xFF, // magic number
          0x0B,       // next block size
          'N', 'E', 'T', 'S', 'C', 'A', 'P', 'E', '2', '.', '0',
          0x03,       // next block size
          0x01,       // fixed
          0x00, 0x00, // number of repeats (little-endian)
          0x00        // end
        };
        fwrite(pex, sizeof(pex), 1, fout);
      }
    }
    frame_write(pic, delay, hf, fout);
    if(i==n-1){
      fwrite(&(pic.z), 1, 1, fout);
    }
    pic_free(pic);
  }
  return;
}

int main(int argc, char * argv[]){
  if(argc<3){
    printf("Usage: %s [OPTIONS] INPUTFILE_1 [INPUTFILE_2 ... INPUTFILE_N] OUTFILE\n\n\
Options:\n\
  -d NUM    delay between frames in 1/100s, default 1\n\
  -l        loop the animation\n", argv[0]);
    return 1;
  }

  int n, delay=1, loop=0;
  while ( ( n = getopt(argc, argv, "d:l")) != -1){
    switch (n){
      case 'd':
        delay = atoi(optarg);
        break;
      case 'l':
        loop = 1;
    };
  };
  if(delay<1){
    delay = 1;
  }

  FILE * fout = fopen(argv[argc-1], "w");
  pics_write(delay, loop, argc-optind-1, argv+optind, fout);
  fclose(fout);
  return 0;
}

