#include <stdio.h>
#include "../src/core.h"
static void run(int i, double a, double b, double c){
  static Out o; double v[3]={a,b,c};
  printf("### %d %s | %s\n", i+1, FORMS[i].cat, FORMS[i].desc);
  FORMS[i].fn(v,&o);
  for(int k=0;k<o.n;k++) printf("  |%-21s|\n", o.rows[k]);
}
int main(void){
  run(0,3,10,2); run(1,3,21,2); run(2,3,21,10); run(3,3,10,2); run(4,3,21,10); run(5,21,10,2); run(6,3,21,0);
  run(7,2,5,3); run(8,2,3,5); run(9,12,0.5,0); run(10,2,162,5); run(11,2,162,5); run(12,2,162,3);
  run(7,5,-4,-0.5); run(9,1,1,0); run(8,1e6,10,7); run(1,0,5,0);
  double x; const char*t[]={"3/4","-2.5","1.","abc","1/0","-","12"};
  for(int i=0;i<7;i++){char b[40]; int ok=c_parse(t[i],&x); if(ok){c_fmt(x,b);printf("%s -> %s\n",t[i],b);} else printf("%s -> invalido\n",t[i]);}
  return 0;}
