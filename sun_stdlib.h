/*
 * sun_stdlib.h 
 *
 * This header file is redundant/wrong for sun architectures that have sane 
 * header files. Grrr.
 */
extern int fclose (FILE *);
extern int fread (char *, int, int, FILE *);
extern int fflush (FILE *);
extern int getpid(void);
extern int getppid(void);
extern int pipe(int *);
extern int printf( const char *, ... );
extern int fprintf( FILE *, const char *, ... );
#if 0
extern char *sprintf( char *, const char *, ... );
#endif
extern int fork(void);
extern int close(int);
extern int dup(int);
extern int execl(char *, ... );
extern int execv(char *, char *[]);
extern int execle(char *, ... );
extern int execlp(char *, ... );
extern int execvp(char *, char *[]);

extern int getuid(void);
extern int geteuid(void);
extern int gethostname (char *, int);
extern int sethostname (char *, int);

extern int sleep(unsigned int);
extern int ioctl(int, int, caddr_t);
extern void perror(char *);

#if 0
extern int openlog(char *, int int);
#endif
extern int syslog(int, char *, ... );
extern int closelog(void);
extern int setlogmask(int);

extern unsigned int alarm(unsigned int);
extern int write(int, char *, int);

extern char *crypt(char *, char *);
extern char *_crypt(char *, char *);
extern int setkey(char *);
extern int encrypt(char *, int);

extern int putenv(char *);
extern void bcopy(char *, char *, int);
extern void bcmp(char *, char *, int);
extern void bzero(char *, int);
extern int ffs(int);

extern int sigblock(int);
extern int sigsetmask(int);

extern int usleep(unsigned);

extern int _filbuf( FILE * );
extern int _flsbuf(unsigned char, FILE*);
