#include <mach/debug-shmac.h>




/* Defines */
#define PAD_RIGHT 1
#define PAD_ZERO  2

/* the following should be enough for 32 bit int */
#define PRINT_BUF_LEN 16



inline void __putChar(char c) {
  // wait
  while((*DEBUG_SYS_INT_STATUS) & 2); 
  // write word
  *DEBUG_SYS_OUT_DATA = c;
}

inline void outbyte(char** dst, char c)
{

        __putChar(c);
}


void __putString(char *s) {
  while(*s != 0) {
    if(*s == '\n') 
            __putChar('\r');
    __putChar(*s);
    s++;
  }
}

void __newline(void){
        __putString("\n");
}
void __putUint4(char val) {
  if(val > 9) 
          __putChar(val - 10 + 'a');
  else 
          __putChar(val + '0');
}

void __putUint32(unsigned long val) {
        int i;
        for(i = 7; i >= 0; i--) {
                __putUint4((val >> (i * 4))&0xf);
        }
}



int shmac__debug__sprintf(char* dst, const char *fmt, ...)
{
    register unsigned long *varg = (unsigned long *)(&fmt);
    *varg++;
   
    /* Need to pass a pointer to a pointer to the location to
       write the character, to that the pointer to the location
       can be incremented by the final outpute function
    */   
   
    return shmac__debug__print(&dst, fmt, varg);
}

int shmac_printk(const char *fmt, ...)
{
    register unsigned long *varg = (unsigned long *)(&fmt);
    char *dst = 0;    
    *varg++;
    
   
    return shmac__debug__print((char**)&dst, fmt, varg);
}

/*  printf supports the following types of syntax ---
    char *ptr = "Hello world!";
    char *np = 0;
    int i = 5;
    unsigned int bs = sizeof(int)*8;
    int mi;
    mi = (1 << (bs-1)) + 1;

    printf("%s\n", ptr);
    printf("printf test\n");
    printf("%s is null pointer\n", np);
    printf("%d = 5\n", i);
    printf("%d = - max int\n", mi);
    printf("char %c = 'a'\n", 'a');
    printf("hex %x = ff\n", 0xff);
    printf("hex %02x = 00\n", 0);
    printf("signed %d = unsigned %u = hex %x\n", -3, -3, -3);
    printf("%d %s(s)%", 0, "message");
    printf("\n");
    printf("%d %s(s) with %%\n", 0, "message");

*/

int shmac__debug__print(char** dst, const char *format, unsigned long *varg)
{
    register int width, pad;
    register int pc = 0;
    char scr[2];
     
    for (; *format != 0; ++format) {
       if (*format == '%') {
          ++format;
          width = pad = 0;
          if (*format == '\0') break;
          if (*format == '%') goto out;
          if (*format == '-') {
             ++format;
             pad = PAD_RIGHT;
          }
          while (*format == '0') {
             ++format;
             pad |= PAD_ZERO;
          }
          for ( ; *format >= '0' && *format <= '9'; ++format) {
             width *= 10;
             width += *format - '0';
          }
          if( *format == 's' ) {
             register char *s = *((char **)varg++);
             pc += shmac__debug__prints (dst, s?s:"(null)", width, pad);
             continue;
          }
          if( *format == 'd' ) {
             pc += shmac__debug__printi (dst, *varg++, 10, 1, width, pad, 'a');
             continue;
          }
          if( *format == 'x' ) {
             pc += shmac__debug__printi (dst, *varg++, 16, 0, width, pad, 'a');
             continue;
          }
          if( *format == 'X' ) {
             pc += shmac__debug__printi (dst, *varg++, 16, 0, width, pad, 'A');
             continue;
          }
          if( *format == 'u' ) {
             pc += shmac__debug__printi (dst, *varg++, 10, 0, width, pad, 'a');
             continue;
          }
          if( *format == 'c' ) {
             /* char are converted to int then pushed on the stack */
             scr[0] = *varg++;
             scr[1] = '\0';
             pc += shmac__debug__prints (dst, scr, width, pad);
             continue;
          }
       }
       else {
       out:
          if (*format=='\n') outbyte(dst,'\r');
          outbyte (dst, *format);
          ++pc;
       }
    }

    return pc;
}


/* Print a string - no formatting characters will be interpreted here */
int shmac__debug__prints(char** dst, const char *string, int width, int pad)
{
    register int pc = 0, padchar = ' ';

    if (width > 0) {                          
       register int len = 0;                  
       register const char *ptr;              
       for (ptr = string; *ptr; ++ptr) ++len; 
       if (len >= width) width = 0;           
       else width -= len;                     
       if (pad & PAD_ZERO) padchar = '0';     
    }                                         
    if (!(pad & PAD_RIGHT)) {                 
       for ( ; width > 0; --width) {          
          outbyte(dst, padchar);              
          ++pc;                               
       }                                      
    }                                         
    for ( ; *string ; ++string) {             
       outbyte(dst, *string);                 
       ++pc;                                  
    }                                         
    for ( ; width > 0; --width) {             
       outbyte(dst, padchar);                 
       ++pc;                                  
    }                                         

    return pc;                                
}


/* shmac__debug__Printf an integer */
int shmac__debug__printi(char** dst, int i, int b, int sg, int width, int pad, int letbase)
{
    char print_buf[PRINT_BUF_LEN];
    char *s;
    int t, neg = 0, pc = 0;
    unsigned int u = i;

    if (i == 0) {
       print_buf[0] = '0';
       print_buf[1] = '\0';
       return shmac__debug__prints (dst, print_buf, width, pad);
    }

    if (sg && b == 10 && i < 0) {
       neg = 1;
       u = -i;
    }

    s = print_buf + PRINT_BUF_LEN-1;
    *s = '\0';

    while (u) {
       if ( b == 16 )    t = u & 0xf;                  /* hex modulous */
       else              t = u - ( (u / b ) * b );  /* Modulous */
       
       if( t >= 10 )
          t += letbase - '0' - 10;
       *--s = t + '0';
       
    /*   u /= b;  */
       if ( b == 16)  u = u >> 4;    /* divide by 16 */
       else           u = u / b;
    }

    if (neg) {
       if( width && (pad & PAD_ZERO) ) {
          /* _outbyte('-'); */
          outbyte(dst,'-'); 
          ++pc;
          --width;
       }
       else {
          *--s = '-';
       }
    }

    return pc + shmac__debug__prints (dst, s, width, pad);
}
