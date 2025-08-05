#ifndef COMPAT_OS2_POSIX_H
#define COMPAT_OS2_POSIX_H

#ifndef NO_ICONV
# include <iconv.h>
# if defined(__INNOTEK_LIBC__) && defined(__ICONV_H__)
  /* override klibc's builtin iconv_open */
   extern iconv_t wrapped_iconv_open_for_klibc (const char *, const char *);
#  ifndef BUILDING_COMPAT_OS2
#   define iconv_open(t,f) wrapped_iconv_open_for_klibc (t,f)
#  endif
# endif
#endif

#if defined(NO_POLL_H) && defined(NO_SYS_POLL_H)
#ifndef _NFDS_T_DECLARED
typedef unsigned int  nfds_t;
# define _NFDS_T_DECLARED
#endif

struct pollfd {
  int fd;
  short events;
  short revents;
};
#define POLLIN      0x0001
#define POLLPRI     0x0002
#define POLLOUT     0x0004
#define POLLERR     0x0008  /* not supported */
#define POLLHUP     0x0010  /* not supported */
#define POLLNVAL    0x0020  /* not supported */
#endif

extern char * wrapped_getenv_for_os2 (const char *);
extern struct passwd * wrapped_getpwuid_for_klibc (uid_t);
extern int wrapped_unlink_for_dosish_system (const char *);

extern int wrapped_poll_for_os2 (struct pollfd *, nfds_t, int);
extern int wrapped_pipe_for_os2 (int *);

extern int wrapped_execl_for_os2 (const char *, const char *, ...);
extern int wrapped_execlp_for_os2 (const char *, const char *, ...);
extern int wrapped_execv_for_os2 (const char *, char **);
extern int wrapped_execvp_for_os2 (const char *, char **);

extern ssize_t git_os2_read (int, void *, size_t);
extern ssize_t git_os2_write (int, const void *, size_t);

extern const char *git_os2_runtime_prefix (void);

#ifndef BUILDING_COMPAT_OS2
# ifdef __EMX__
#  define chdir(d) _chdir2(d)
#  define getcwd(d,n) _getcwd2(d,n)
# endif
# define getenv(e) wrapped_getenv_for_os2 (e)
# define getpwuid(u) wrapped_getpwuid_for_klibc (u)
# define unlink(f) wrapped_unlink_for_dosish_system (f)
# if defined(NO_POLL_H) && defined(NO_SYS_POLL_H)
#  define poll wrapped_poll_for_os2
# endif
# define pipe wrapped_pipe_for_os2
# define execl wrapped_execl_for_os2
# define execlp wrapped_execlp_for_os2
# define execv wrapped_execv_for_os2
# define execvp wrapped_execvp_for_os2
# define read git_os2_read
# define write git_os2_write
# ifdef PREFIX
#  undef PREFIX
#  define PREFIX (git_os2_runtime_prefix ())
# endif
#endif

#ifndef _SOCKLEN_T_DECLARED
# ifndef socklen_t
typedef int socklen_t;
# endif
# define _SOCKLEN_T_DECLARED
#endif

#define SHUT_RD     0
#define SHUT_WR     1
#define SHUT_RDWR   2

#endif
