
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

extern struct passwd * wrapped_getpwuid_for_klibc (uid_t);
extern int wrapped_unlink_for_dosish_system (const char *);
extern char * wrapped_getenv_for_os2 (const char *);

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

extern int wrapped_poll_for_os2 (struct pollfd *, nfds_t, int);
extern int wrapped_pipe_for_os2 (int *);

extern int wrapped_execl_for_os2 (const char *, const char *, ...);
extern int wrapped_execlp_for_os2 (const char *, const char *, ...);
extern int wrapped_execv_for_os2 (const char *, char **);
extern int wrapped_execvp_for_os2 (const char *, char **);

extern const char *git_os2_runtime_prefix (void);
extern const char *git_os2_default_template_dir (void);
extern const char *git_os2_default_html_path (void);
extern const char *git_os2_default_info_path (void);
extern const char *git_os2_default_man_path (void);

extern ssize_t git_os2_read (int, void *, size_t);
extern ssize_t git_os2_write (int, const void *, size_t);

#ifndef BUILDING_COMPAT_OS2
# ifdef __EMX__
#  define chdir(d) _chdir2(d)
#  define getcwd(d,n) _getcwd2(d,n)
# endif
# define getenv(e) wrapped_getenv_for_os2 (e)
# define getpwuid(u) wrapped_getpwuid_for_klibc (u)
# define unlink(f) wrapped_unlink_for_dosish_system (f)
# define poll wrapped_poll_for_os2
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

#undef DEFAULT_GIT_TEMPLATE_DIR
#define DEFAULT_GIT_TEMPLATE_DIR (git_os2_default_template_dir ())
#undef GIT_HTML_PATH
#define GIT_HTML_PATH (git_os2_default_html_path ())
#undef GIT_INFO_PATH
#define GIT_INFO_PATH (git_os2_default_info_path ())
#undef GIT_MAN_PATH
#define GIT_MAN_PATH (git_os2_default_man_path ())


#ifndef _SOCKLEN_T_DECLARED
# ifndef socklen_t
typedef int socklen_t;
# endif
# define _SOCKLEN_T_DECLARED
#endif

#define has_dos_drive_prefix(path) (isalpha(*(path)) && (path)[1] == ':')
#define is_dir_sep(c) ((c) == '/' || (c) == '\\')
#define PATH_SEP ';'

#if defined(GIT_OS2_USE_DEFAULT_BROWSER)
extern void git_os2pm_open_html(const char *unixpath);
# define open_html git_os2pm_open_html
#endif

#define main(c,v) dummy_decl_git_os2_main(); \
extern int git_os2_main_prepare(int *, char ** *); \
extern int git_os2_main(c,v); \
int main(int argc, char **argv) \
{ \
  git_os2_main_prepare(&argc,&argv); \
  return git_os2_main(argc, (const char **)argv); \
} \
int git_os2_main(c,v)

#define SHUT_RD     0
#define SHUT_WR     1
#define SHUT_RDWR   2
