/* Differential test: flags after BT/BTS/BTR/BTC following various flag-setting ops.
   Prints CF PF AF ZF SF OF and the resulting operand; run on hardware and under QEMU, then diff. */
#include <stdio.h>
#include <stdint.h>
#define FL 0x8d5 /* CF PF AF ZF SF OF */
static uint64_t rnd(void){static uint64_t x=0x9E3779B97F4A7C15ull; x^=x<<13; x^=x>>7; x^=x<<17; return x;}
#define PRE(op) op " %[b], %[a]\n\t"
/* T: one test = prior op on a (with b), then bt-variant with bit c on dst, then read flags */
#define T(name, pre, btins, sz, creg) do{ uint64_t a=A,b=B,d=D,f; \
  __asm__ volatile(pre btins "\n\tpushfq\n\tpopq %[f]" : [a]"+r"(a),[d]"+r"(d),[f]"=r"(f) : [b]"r"(b),[c]creg(C) : "cc"); \
  printf("%-28s %016llx %016llx %3llu -> fl=%03llx d=%016llx\n", name, (unsigned long long)A,(unsigned long long)B,(unsigned long long)C,(unsigned long long)(f&FL),(unsigned long long)d);}while(0)
#define TM(name, pre, btins) do{ uint64_t a=A,b=B,f; uint64_t m=D; \
  __asm__ volatile(pre btins "\n\tpushfq\n\tpopq %[f]" : [a]"+r"(a),[f]"=r"(f),[m]"+m"(m) : [b]"r"(b),[c]"r"(C) : "cc"); \
  printf("%-28s %016llx %016llx %3llu -> fl=%03llx m=%016llx\n", name, (unsigned long long)A,(unsigned long long)B,(unsigned long long)C,(unsigned long long)(f&FL),(unsigned long long)m);}while(0)
#define VARIANTS(pn, pre) \
  T(pn "/btq r",  pre, "btq %[c], %[d]", 64, "r"); \
  T(pn "/btl r",  pre, "btl %k[c], %k[d]", 32, "r"); \
  T(pn "/btw r",  pre, "btw %w[c], %w[d]", 16, "r"); \
  T(pn "/btq $5", pre, "btq $5, %[d]", 64, "r"); \
  T(pn "/btw $8", pre, "btw $8, %w[d]", 16, "r"); \
  T(pn "/btsq r", pre, "btsq %[c], %[d]", 64, "r"); \
  T(pn "/btrl r", pre, "btrl %k[c], %k[d]", 32, "r"); \
  T(pn "/btcq $63",pre,"btcq $63, %[d]", 64, "r"); \
  T(pn "/btsw $3",pre, "btsw $3, %w[d]", 16, "r"); \
  TM(pn "/btq r,mem", pre, "btq %[c], %[m]"); \
  TM(pn "/btsl $9,mem", pre, "btsl $9, %[m]"); \
  TM(pn "/btrq r,mem", pre, "btrq %[c], %[m]");
int main(void){
  for(int i=0;i<300;i++){
    uint64_t A=rnd(), B=rnd(), D=rnd(), C=rnd()&63;
    if(i%5==0) A=0; if(i%7==0) A=1; if(i%11==0) A=0x8000000000000000ull; if(i%13==0) B=A;
    VARIANTS("add",  PRE("addq"));
    VARIANTS("sub",  PRE("subq"));
    VARIANTS("cmp",  PRE("cmpq"));
    VARIANTS("and",  PRE("andq"));
    VARIANTS("xor",  PRE("xorq"));
    VARIANTS("test", PRE("testq"));
    VARIANTS("decl", "decl %k[a]\n\t");
    VARIANTS("incq", "incq %[a]\n\t");
    VARIANTS("negq", "negq %[a]\n\t");
    VARIANTS("shlq1","shlq $1, %[a]\n\t");
  }
  return 0;
}
