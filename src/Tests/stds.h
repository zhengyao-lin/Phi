#define NULL ((void *)0)
#define EOF -1

/* Macros */
#define isspace(c) (c == ' ' || c == '\t')
#define isdigit(c) (c <= '9' && c >= '0')

/* Types */
/*typedef int:1 Bit;
typedef int:8 Byte;
typedef int:16 Int16;
typedef int:32 Int32;
typedef int:64 Int64;
typedef int:128 Int128;
typedef int:256 Int256;*/
typedef int:128 Tremend;

extern int printf(char *str, ...);
extern int scanf(char *str, ...);
extern char *strdup(char *str);
extern int sprintf(char *dest, char *str, ...);
extern void *malloc(int:64 size);
extern void memset(void *ptr, int value, int:64 size);
extern int system(char *s);
extern void free(void *ptr);
extern int read(int fd, char *buf, int n);
extern int strlen(char *str);
