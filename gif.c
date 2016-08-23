
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>

typedef struct {
  uint8_t   sv[6];
  uint16_t  w;
  uint16_t  h;
  uint8_t   f;
  uint8_t   bg;
  uint8_t   r;
  int CT   ;
  int Color;
  int SF   ;
  int Size ;
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
  uint8_t head [13];
  uint8_t * pal;
  uint8_t gex[8];
  uint8_t phead[10];
  uint8_t MC;
  uint8_t * data;
  uint8_t z;
  /* ======= */
  int     n;    /* палитра */
  int     size; /* данные */
  int     k;    /* число блоков */
} picture;

int head_read(uint8_t head[13], FILE  * f){

  header  h;

  fread(head, 13, 1, f);
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
  int i;
  for (i=0; i<n; i++){
    printf("%3d    %3d    %3d\n", pal[3*i], pal[3*i+1], pal[3*i+2]);
  }
#endif
  return pal;
}

void phead_read(uint8_t phead[9], FILE  * f){

  picheader  ph;

  fread(phead, 9, 1, f);
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

  FILE *  f;
  long    max;
  long    m;
  uint8_t z;
  picture pic;

  f = fopen(s, "r");
  if (!f){
    abort();
  }
  fseek(f, 0, SEEK_END);
  max = ftell(f);
  fseek(f, 0, SEEK_SET);

  pic.n   = head_read(pic.head, f);
  pic.pal = pal_read(pic.n, f);

  memset(pic.gex, 0, 8);
  do{
    fread(&z,   1, 1, f);
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
  fread(&(pic.MC),   1, 1, f);
  pic.size = max - ftell(f);
  pic.data = malloc(pic.size);

  pic.k = 0;
  m = 0;

  fread(pic.data+m,    1, 1, f);
  do{
    pic.k++;
    fread(pic.data+m+1, pic.data[m], 1, f);
    m = m + pic.data[m] + 1;
    fread(pic.data+m, 1, 1, f);
  }
  while(pic.data[m] != 0) ;

  pic.size = m+1;
  pic.data = realloc(pic.data, pic.size);
  if(!pic.data){
    abort();
  }
  pic.z = ';';
  fclose(f);
  return pic;
}

void pic_write1(picture pic, char * s){
  FILE * f;
  f = fopen(s, "w");
  fwrite(pic.head, 13, 1, f);
  fwrite(pic.pal, pic.n*3, 1, f);
  if(pic.gex[0]){
    fwrite(pic.gex, 8, 1, f);
  }
  fwrite(pic.phead, 10, 1, f);
  fwrite(&(pic.MC), 1 , 1, f);
  fwrite(pic.data, pic.size, 1, f);
  fwrite(&(pic.z), 1 , 1, f);
  fclose(f);
  return;
}

void frame_write(picture pic, uint16_t d, uint8_t hf, FILE * f){
  uint8_t * phf;
  phf    =  (uint8_t  *)(pic.phead + 9 );
  *phf   = 0x00;
  *phf  |= 128;
  *phf  |= (hf & 8 ? 32 : 0);
  *phf  |= (hf & (4|2|1));
  if(pic.gex[0]){
    *(uint16_t *)(pic.gex+4) = d;
    fwrite(pic.gex, 8, 1, f);
  }
  else{
    if(d){
      pic.gex[0] = '!';
      pic.gex[1] = 0xF9;
      pic.gex[2] = 0x04;
      pic.gex[3] = 0x00;
      pic.gex[6] = 0x00;
      pic.gex[7] = 0x00;
      *(uint16_t *)(pic.gex+4) = d;
      fwrite(pic.gex, 8, 1, f);
    }
  }
  fwrite(pic.phead, 10, 1, f);
  fwrite(pic.pal, pic.n*3, 1, f);
  fwrite(&(pic.MC), 1 , 1, f);
  fwrite((pic.data), pic.size, 1, f);
}

uint8_t head_cl(picture * pic){
  uint8_t * hfp;
  uint8_t   hf;
  hfp    =  (uint8_t  *)(pic->head  + 10);
  hf     =  *hfp;
  *hfp  &= ~(128 |4 | 2 | 1);
  return hf;
}

void pic_write2(picture p, char * s){
  picture   pic;
  uint8_t   hf;
  FILE    * f;
  pic = p;
  hf  = head_cl(&pic);
  f   = fopen(s, "w");
  fwrite(pic.head, 13, 1, f);
  frame_write(pic, 0, hf, f);
  fwrite(&(pic.z), 1 , 1, f);
  fclose(f);
  return;
}

void pic_write3(picture p1, picture p2, char * s){
  picture   pic1, pic2;
  uint8_t   hf;
  FILE    * f;
  pic1 = p1;
  pic2 = p2;
  f   = fopen(s, "w");
  hf  = head_cl(&pic1);
  fwrite(pic1.head, 13, 1, f);
  frame_write(pic1, 100, hf, f);
  hf  = head_cl(&pic2);
  frame_write(pic2, 100, hf, f);
  fwrite(&(pic1.z), 1 , 1, f);
  fclose(f);
  return;
}

void pic_free(picture pic){
  free(pic.pal);
  free(pic.data);
}

void pic_write4(int d, int n, char * argv[], FILE * f){

  picture pic;
  int i;
  uint8_t   hf;

  for(i=0; i<n; i++){
#if 0
    printf("%s\n", argv[i]);
#endif
    pic  = pic_read(argv[i]);
    hf  = head_cl(&pic);
    if(!i){
      fwrite(pic.head, 13, 1, f);
    }
    frame_write(pic, d, hf, f);
    if(i==n-1){
      fwrite(&(pic.z), 1 , 1, f);
    }
    pic_free(pic);
  }
  return;
}

int main(int argc, char * argv[]){
  FILE * f;
  int d;
  f = fopen(argv[argc-1], "w");
  d = atoi(argv[1]);
  if(d<1){
    d = 1;
  }
  pic_write4(d, argc-3, argv+2, f);
  fclose(f);
  return 0;
}

