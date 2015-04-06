#ifndef _GENERAL_H_
#define _GENERAL_H_

#define STDIN_FILENO 0
#define TIOCGWINSZ 0x5413
#define NULL ((void *)0)

extern int printf(char *str, ...);
extern int sprintf(char *dest, char *str, ...);
extern char *strlen(char *str);
extern void free(void *ptr);
extern char *malloc(int:64 size);
extern int ioctl(int fd, int cmd, ...);
extern int memset(void *ptr, char n, int:64 size);
extern int memcpy(void *dest, void *src, int:64 size);
extern char getchar(void);
extern int system(char *s);
extern int tcgetattr (int __fd, struct termios *__termios_p);
extern int tcsetattr (int __fd, int __optional_actions, struct termios *__termios_p);

struct winsize
{
	int:16 ws_row;
	int:16 ws_col;
	int:16 ws_xpixel;
	int:16 ws_ypixel;
};

typedef char cc_t;
typedef int speed_t;
typedef int tcflag_t;

struct termios {
	tcflag_t c_iflag;		/* input mode flags */
	tcflag_t c_oflag;		/* output mode flags */
	tcflag_t c_cflag;		/* control mode flags */
	tcflag_t c_lflag;		/* local mode flags */
	cc_t c_line;			/* line discipline */
	cc_t c_cc[19];	/* control characters */
};

#endif
