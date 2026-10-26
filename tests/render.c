/* Renderiza las pantallas a archivos PPM para revisarlas en el PC */
#include <stdio.h>
#include <string.h>
#include "../src/ui.h"
static unsigned short buf[SW*SH];
static void dump(const char*fn){
  FILE*f=fopen(fn,"wb"); fprintf(f,"P6\n%d %d\n255\n",SW,SH);
  for(int i=0;i<SW*SH;i++){unsigned c=buf[i]; unsigned char p[3]={((c>>11)&31)*255/31,((c>>5)&63)*255/63,(c&31)*255/31}; fwrite(p,1,3,f);} fclose(f);}
int main(int argc,char**argv){
  g_vram=buf; static Out o; char vals[3][VLEN+1];
  memset(vals,0,sizeof vals);
  int menus[][2]={{0,0},{6,1},{12,7}};
  for(int i=0;i<3;i++){draw_menu(menus[i][0],menus[i][1]); char n[40]; sprintf(n,"/tmp/menu%d.ppm",i); dump(n);}
  for(int fi=0;fi<NFORM;fi++){
    strcpy(vals[0],"3"); strcpy(vals[1],"10"); strcpy(vals[2],"2");
    draw_input(fi,1,"",vals); char n[40]; sprintf(n,"/tmp/in%d.ppm",fi); dump(n);
    double v[3]={3,10,2};
    /* valores razonables por formula */
    if(fi==1||fi==2||fi==4||fi==10||fi==11){v[0]=3;v[1]=21;v[2]=(fi==1)?2:10;}
    if(fi==5){v[0]=21;v[1]=10;v[2]=2;}
    if(fi==6){v[0]=3;v[1]=21;}
    if(fi>=7){v[0]=2;v[1]=5;v[2]=3;}
    if(fi==8){v[0]=2;v[1]=3;v[2]=5;}
    if(fi==9){v[0]=12;v[1]=0.5;}
    if(fi==10){v[0]=2;v[1]=162;v[2]=5;}
    if(fi==11){v[0]=2;v[1]=162;v[2]=5;}
    if(fi==12){v[0]=2;v[1]=162;v[2]=3;}
    FORMS[fi].fn(v,&o);
    draw_result(&o,0,fi); sprintf(n,"/tmp/res%d.ppm",fi); dump(n);
  }
  return 0;}
