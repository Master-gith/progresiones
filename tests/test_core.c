#include <stdio.h>
#include "../src/core.h"
static void run(int i, double a, double b, double c){
  static Out o; double v[3]={a,b,c};
  FORMS[i].fn(v,&o);
  printf("%2d %-30s ->", i+1, FORMS[i].name);
  if(o.err) printf(" ERROR\n"); else printf(" %s\n", o.l[o.res]);
}
int main(void){
  run(0,3,10,2); run(1,3,21,2); run(2,3,21,10); run(3,3,10,2); run(4,3,21,10); run(5,21,10,2); run(6,3,21,0);
  run(7,2,5,3); run(8,2,3,5); run(9,12,0.5,0); run(10,2,162,5); run(11,2,162,5); run(12,2,162,3);
  run(1,0,5,0);
  return 0;}
