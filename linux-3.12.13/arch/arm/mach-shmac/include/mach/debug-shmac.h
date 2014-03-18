#ifndef __DEBUG_SHMAC_H__
#define __DEBUG_SHMAC_H__

#define SYS_INT_STATUS ((volatile unsigned long*)(0xffff0020))
#define SYS_OUT_DATA ((volatile unsigned long*)(0xffff0000))
int shmac_printk(const char *fmt, ...);
int shmac__debug__prints(char** dst, const char *string, int width, int pad);
int shmac__debug__printi(char** dst, int i, int b, int sg, int width, int pad, int letbase);
int shmac__debug__print(char** dst, const char *format, unsigned long *varg);

void __putString(char *s);
void __newline(void);
void __putUint4(char val);
void __putUint32(unsigned long val);

#endif
