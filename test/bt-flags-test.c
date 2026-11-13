#include <stdio.h>
#include <stdint.h>
/* dec r10d ; bt $8,r15w ; then read flags. Compare SF/ZF/OF/CF with dec alone. */
static uint64_t run(uint32_t start, uint16_t r15, int with_bt){
  uint64_t fl;
  if(with_bt)
   __asm__ volatile("movl %1,%%r10d\n\tmovw %2,%%r15w\n\tdecl %%r10d\n\tbtw $8,%%r15w\n\tpushfq\n\tpopq %0"
     :"=r"(fl):"r"(start),"r"(r15):"r10","r15","cc");
  else
   __asm__ volatile("movl %1,%%r10d\n\tmovw %2,%%r15w\n\tdecl %%r10d\n\tpushfq\n\tpopq %0"
     :"=r"(fl):"r"(start),"r"(r15):"r10","r15","cc");
  return fl;
}
static void show(const char*t,uint64_t f){printf("%-28s CF=%d ZF=%d SF=%d OF=%d  jg=%s\n",t,(int)(f&1),(int)(f>>6&1),(int)(f>>7&1),(int)(f>>11&1),
  (!((f>>6)&1) && (((f>>7)&1)==((f>>11)&1)))?"TAKEN":"not taken");}
int main(){
  uint32_t starts[]={1,0,0xfffff9af,5};
  for(int i=0;i<4;i++){ for(int c=0;c<2;c++){
    char b[64]; sprintf(b,"dec from %d, r15bit8=%d",(int)starts[i],c);
    show(b, run(starts[i], c?0x100:0, 0));
    sprintf(b,"  + btw $8,r15w");
    show(b, run(starts[i], c?0x100:0, 1)); }}
}
