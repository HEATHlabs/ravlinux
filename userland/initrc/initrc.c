#include <stdint.h>

#define SYS_OUT_DATA    (uint32_t*)0xFFFF0000  
#define SYS_INT_STATUS  (uint32_t*)0xFFFF0020

static inline void putChar(uint8_t c) {
  // wait
  while(*(SYS_INT_STATUS) & 2)
  // write word
  *SYS_OUT_DATA = c;
}

static void putString(char *s) {
  while(*s != 0) {
    if(*s == '\n') putChar('\r');
    putChar(*s);
    s++;
  }
}

int main(){
    int i = 0;
    while(i < 10)
        putString("USERLAND SAYS HELLO TEN TIMES\n");
}
