#define INCL_DOS
#define INCL_DOSERRORS
#define INCL_WIN
#include <os2.h>
/* conflict: object_type (cache.h) */
#undef OBJ_ANY
#include <uconv.h>

#include <sys/types.h>
#include <assert.h>
#include <float.h>
#include <dirent.h>
#include <malloc.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <io.h>
#include <sys/stat.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <pwd.h>
#include <unistd.h>

/* klibc06_npipe */
#include <emx/io.h>
#include <emx/syscalls.h>
#include <InnoTekLIBC/errno.h>

#define BUILDING_COMPAT_OS2
#include "../cache.h"


#ifndef PREFIX
#define PREFIX "/git"
#endif
#ifndef BINDIR
#define BINDIR "bin"
#endif
#define RELATIVE_TEMPLATE_DIR "share/git-core/templates"
#define RELATIVE_EXEC_PATH "libexec/git-core"

#define HTML_PATH_ENVIRONMENT "GIT_HTML_PATH"
#define INFO_PATH_ENVIRONMENT "GIT_INFO_PATH"
#define MAN_PATH_ENVIRONMENT "GIT_MAN_PATH"
#define RELATIVE_MANPATH "share/man"
#define RELATIVE_INFOPATH "share/info"
#define RELATIVE_HTMLPATH "share/doc/git-doc"


extern int _fmode_bin;


#ifndef PATH_MAX
#define PATH_MAX 260
#endif


#if defined(GIT_OS2_USE_DEFAULT_BROWSER)

static char *szBrowserExe;
static char *szBrowserParam;
static char *szBrowserDir;

static
int
git_os2pm_prepare_default_browser(void)
{
  static const char szPMSHAPI[] = "PMSHAPI.DLL";
  static const char sz_app[] = "WPURLDEFAULTSETTINGS";
  static const char sz_prog[] = "DefaultBrowserExe";
  static const char sz_param[] = "DefaultParameters";
  static const char sz_execdir[] = "DefaultWorkingDir";

  /* PrfQueryProfileString PMSHAPI.115 */
  HMODULE hm = 0;
  ULONG (APIENTRY *myQueryProfileString)(HINI, PCSZ, PCSZ, PCSZ, PVOID, ULONG);

  int rc;
  HINI hini;
  const size_t buflim = 4096;
  char *buf;
  ULONG ul;
  struct stat st;

  rc = 0;
  /* check running under PM: */
  rc = (int)DosQueryModuleHandle(szPMSHAPI, &hm);
  if (rc == 0) rc = DosLoadModule(0, 0, szPMSHAPI, &hm);
  if (rc == 0)
    rc = (int)DosQueryProcAddr(hm, 115L, 0, (PFN *)&myQueryProfileString);

  if (rc != 0) return -1;

  buf = alloca(buflim + 1);
  buf[buflim] = '\0'; /* just a proof */
  hini = HINI_USERPROFILE;

  ul = (*myQueryProfileString)(hini, sz_app, sz_prog, "", buf, buflim);
  if (stat(buf, &st) >= 0 || S_ISREG(st.st_mode)) {
    szBrowserExe = strdup(buf);
    ul = (*myQueryProfileString)(hini, sz_app, sz_param, "", buf, buflim);
    if (ul > 0 && ul <= buflim) szBrowserParam = strdup(buf);
    ul = (*myQueryProfileString)(hini, sz_app, sz_execdir, "", buf, buflim);
    if (ul > 0 && ul <= buflim) szBrowserDir = strdup(buf);
  }
  else
    rc = -1;

  DosFreeModule(hm);

  return rc;
}

void
git_os2pm_open_html(const char *unixpath)
{
  APIRET rc;
  char *prog;
  char *html;
  char prev_cwd[CCHMAXPATH+1];
  char *cmdline;
  size_t n_param;
  size_t n_unixpath;
  size_t n_cmdline;
  char prevdrv;
  STARTDATA sd;
  PID pid;
  ULONG ultmp;

  if (!szBrowserExe) {
    error ("default browser not found.");
    return;
  }
  if (!unixpath) unixpath = "";
  n_unixpath = strlen(unixpath);
  html = alloca(n_unixpath+1);
  memcpy(html, unixpath, n_unixpath + 1);

  prog = _getname(szBrowserExe);
  /* WebExplorer requires backslashified pathname to open local files */
  if (stricmp(prog, "EXPLORE.EXE")==0 || stricmp(prog, "EXPLORE")==0) {
    size_t n;
    for(n=0; html[n]; ++n)
      if (html[n] == '/') html[n] = '\\';
  }

  n_param = szBrowserParam ? strlen(szBrowserParam) : 0;
  n_cmdline = n_param + 1 + 1 + n_unixpath + 1 + 1; /* param+' "html"' */
  cmdline = alloca(n_cmdline);
  snprintf(cmdline, n_cmdline, "%s%s\"%s\"",
	     szBrowserParam ? szBrowserParam : "",
	     szBrowserParam ? " " : "",
	     html);

  memset(&sd, 0, sizeof(sd));
  sd.Length = sizeof(sd);
  sd.PgmName = szBrowserExe;
  sd.PgmInputs = cmdline;

  prevdrv = _getdrive();
  if (szBrowserDir && szBrowserDir[0]) {
    if (szBrowserDir[1] == ':') _chdrive(szBrowserDir[1]);
  }
  _getcwd2(prev_cwd, sizeof(prev_cwd));
  if (szBrowserDir && szBrowserDir[0]) _chdir2(szBrowserDir);

  rc = DosStartSession(&sd, &pid, &ultmp);

  if (rc != 0)
    error ("DosStartSession failure (rc=%d)", (int)rc);

  _chdir2(prev_cwd);
  _chdrive(prevdrv);

}

#endif

static
int
klibc06_npipe(int fd[2])
{
  /*
    anonymous pipe emulation by using named-pipe (like the genuine EMX).

    reference:
    kLIBC
      svn: branches/libc-0.6/src/emx/src/lib/io/pipe.c
      svn: branches/libc-0.6/src/emx/src/lib/sys/__pipe.c

    emx
      emx/src/os2/fileio.c (do_pipe)

  */
  const ULONG pipesize = 8192;
  const char NPS_ID[] = "\\PIPE\\klibc_pipeemu\\%u\\%u\\%u";
  static unsigned pipe_count = 0;
  unsigned pn;
  char np_name[256];

  APIRET rc;
  const int retry_limit = 3;
  int retry;
  int h_r, h_w;
  PLIBCFH pFHRead, pFHWrite;

  pn = ++pipe_count;  /* need mutex for MP-safe, maybe */
  snprintf(np_name, sizeof(np_name), NPS_ID,
	   (unsigned)getpid(), (unsigned)_gettid(), pn);

  for(retry=0; retry < retry_limit; retry++) {
    rc = DosCreateNPipe((PCSZ)np_name, (PHPIPE)&h_r, NP_ACCESS_INBOUND,
	 NP_NOWAIT | NP_TYPE_BYTE | NP_READMODE_BYTE | (1 & 0xff),
	 pipesize, pipesize,
	 0);
    if (rc != ERROR_TOO_MANY_OPEN_FILES) break;
    __libc_FHMoreHandles();
  }
  if (rc == NO_ERROR)
    rc = DosConnectNPipe(h_r);
  else
    h_r = -1;
  if (rc == NO_ERROR || rc == ERROR_PIPE_NOT_CONNECTED) for(retry=0; retry < retry_limit; retry++) {
    ULONG ulAction;
    DosSetNPHState(h_r, NP_WAIT | NP_TYPE_BYTE | NP_READMODE_BYTE);
    rc = DosOpen((PCSZ)np_name, (PHPIPE)&h_w, &ulAction, 0, FILE_NORMAL,
	 OPEN_ACTION_FAIL_IF_NEW | OPEN_ACTION_OPEN_IF_EXISTS,
	 OPEN_ACCESS_WRITEONLY | OPEN_SHARE_DENYREADWRITE,
	 NULL);
    if (rc != ERROR_TOO_MANY_OPEN_FILES) break;
    __libc_FHMoreHandles();
  }
  if (rc != NO_ERROR)
    h_w = -1;

  if (h_r != -1 && h_w != -1) {
    rc = __libc_FHAllocate((HFILE)h_r, F_PIPE | O_RDONLY, sizeof(LIBCFH), NULL, &pFHRead, NULL);
    if (!rc) rc = __libc_FHAllocate((HFILE)h_w, F_PIPE | O_WRONLY, sizeof(LIBCFH), NULL, &pFHWrite, NULL);
  }
  if (rc == 0) {
    if (!_fmode_bin) {
      pFHRead->fFlags |= O_TEXT;
      pFHWrite->fFlags |= O_TEXT;
    }
    fd[0] = h_r;
    fd[1] = h_w;
  } else {
#if 0
    __libc_native2errno(rc);
#else
    errno = EINVAL;  /* workaround */
#endif
    return -1;
  }

  return 0;
}

int wrapped_pipe_for_os2 (int *fd)
{
  static int checked_using_npipe = 0;
  static int use_npipe = 2;  /* use socket by default */
  int rc;

  if (!checked_using_npipe) {
    char *env = getenv("GIT_USE_NPIPE");
    if (env && *env) {
      switch(*env) {
	case 'S': case 's':
	  use_npipe = 2;
	  break;
	case 'T': case 't': case 'Y': case 'y': case 'O': case 'o':
	  use_npipe = 1;
	  break;
	default:
	  use_npipe = atoi(env) > 0;
	  break;
      }
    }
    checked_using_npipe = 1;
  }
  if (use_npipe == 2)
    rc = socketpair(AF_LOCAL, SOCK_STREAM, 0, fd);
  else if (use_npipe == 1)
    rc = klibc06_npipe(fd);
  else
    rc = pipe(fd);

  return rc;
}

int wrapped_poll_for_os2 (struct pollfd *fds, nfds_t nfds, int timeout)
{
#if defined(POLLIN)
  fd_set fd_r, *pfd_r;
#else
  fd_set *pfd_r = NULL;
#endif
#if defined(POLLOUT)
  fd_set fd_w, *pfd_w;
#else
  fd_set *pfd_w = NULL;
#endif
#if defined(POLLPRI)
  fd_set fd_x, *pfd_x;
#else
  fd_set *pfd_x = NULL;
#endif
  int fd, fd_cnt;
  int rc = 0;
  struct timeval tv;
  struct timeval *ptv = NULL;
  nfds_t i;
  int non_sockets = 0;

#if defined(POLLIN)
  pfd_r = &fd_r;
  FD_ZERO(&fd_r);
#endif
#if defined(POLLOUT)
  pfd_w = &fd_w;
  FD_ZERO(&fd_w);
#endif
#if defined(POLLPRI)
  pfd_x = &fd_x;
  FD_ZERO(&fd_x);
#endif

  for(i=0, fd_cnt=0; i<nfds; i++) {
    fd = fds[i].fd;
    fds[i].revents = 0;
    if (fd >= 0) {
      if (fds[i].events &
	  ( 0
#if defined(POLLIN)
	      | POLLIN
#endif
#if defined(POLLOUT)
	      | POLLOUT
#endif
#if defined(POLLPRI)
	      | POLLPRI
#endif
		       ) ) {
    struct stat stbuf;
	assert(fd < FD_SETSIZE);
	if (fstat(fd, &stbuf) == -1 || (errno = 0, !S_ISSOCK(stbuf.st_mode))) {
	  if (errno == 0 && (S_ISREG(stbuf.st_mode) || S_ISFIFO(stbuf.st_mode)))
	    fds[i].revents = fds[i].events & (POLLIN | POLLOUT | POLLPRI);
	  else
	    fds[i].revents = POLLNVAL;
	  non_sockets++;
	  continue;
	}
	if (fd >= fd_cnt) fd_cnt = fd + 1;
#if defined(POLLIN)
	if (fds[i].events & POLLIN) FD_SET(fd, &fd_r);
#endif
#if defined(POLLOUT)
	if (fds[i].events & POLLOUT) FD_SET(fd, &fd_w);
#endif
#if defined(POLLPRI)
	if (fds[i].events & POLLPRI) FD_SET(fd, &fd_x);
#endif
      }
    }
  }

  if (non_sockets > 0)
    timeout = 0;

  if (timeout >= 0) {
    if (timeout == 0) {
      tv.tv_sec = tv.tv_usec = 0;
    }
    else {
      tv.tv_sec = timeout / 1000;
      tv.tv_usec = (timeout % 1000) * 1000;
    }

    ptv = &tv;
  }

  if (fd_cnt > 0)
    rc = select(fd_cnt, pfd_r, pfd_w, pfd_x, ptv);

  if (rc >= 0)
    rc += non_sockets;

  if (rc > 0) for (i=0; i<nfds; i++) {
    short rev;
    fd = fds[i].fd;
    if (fd >= 0 && fds[i].revents == 0) {
      rev = 0;
#if defined(POLLIN)
      if (FD_ISSET(fd, &fd_r)) rev |= POLLIN;
#endif
#if defined(POLLOUT)
      if (FD_ISSET(fd, &fd_w)) rev |= POLLOUT;
#endif
#if defined(POLLPRI)
      if (FD_ISSET(fd, &fd_w)) rev |= POLLPRI;
#endif
      fds[i].revents = rev;
    }
  }

  return rc;
}



enum {
  FILE_EXETYPE_ERROR = -1,
  FILE_EXETYPE_UNKNOWN = 0,
  FILE_EXETYPE_SH,
  FILE_EXETYPE_CMD,
  FILE_EXETYPE_BINARY
};

enum {
  FILE_EXTENSION_NONE = 1,
  FILE_EXTENSION_COM,
  FILE_EXTENSION_EXE,
  FILE_EXTENSION_SH,
  FILE_EXTENSION_CMD,
  FILE_EXTENSION_BAT
};


static
int
is_file(const char *fn)
{
  struct stat st;
  int rc;

  rc = stat(fn, &st);
  if (rc >= 0)
    rc = (S_ISREG(st.st_mode) && !S_ISDIR(st.st_mode)) ? 0 : -1;

  return rc;
}

struct callback_prog_struct {
  char *buffer;
  size_t limit_buffer;
  size_t length_filename;
};

static
int
callback_prog(const char *path, void *extra)
{
  const int MAX_EXT_LENGTH = 4;
  struct {
    int ext_type;
    char *ext_str;
  } exttbl[] = {
    { FILE_EXTENSION_NONE, "" },
    { FILE_EXTENSION_COM, ".com" },
    { FILE_EXTENSION_EXE, ".exe" },
    { FILE_EXTENSION_SH, ".sh" },
    { FILE_EXTENSION_CMD, ".cmd" },
#if !defined(__OS2__)
    { FILE_EXTENSION_BAT, ".bat" },
#endif
    { 0, NULL }
  };
  int rc = 0;
  char *s;
  char *progext;

  s = alloca(strlen(path) + 1 + MAX_EXT_LENGTH + 1);
  memcpy(s, path, strlen(path) + 1);
  progext = _getext(s);
  if (!progext)
    progext = s + strlen(s);
  if (*progext == '.') {
    rc = (is_file(s) >= 0);
  }
  else {
    int i;
    for(i=0; exttbl[i].ext_type != 0; i++) {
      memcpy(progext, exttbl[i].ext_str, strlen(exttbl[i].ext_str) + 1);
      rc = (is_file(s) >= 0);
      if (rc) break;
    }
  }
  if (rc > 0 && extra) {
    struct callback_prog_struct *cp = extra;
    cp->length_filename = strlen(s);
    if (cp->buffer) {
      if (cp->limit_buffer > 0) {
	snprintf(cp->buffer, cp->limit_buffer, "%s", s);
      }
    }
    else
      cp->buffer = strdup(s);
  }
  return rc;
}


static
int
enum_in_path(const char *pathlist, const char *filename, int (*callback)(const char *pathname, void *extra), void *extra)
{
  int rc;
  size_t n_pathlist;
  char *stmp;
  const char *path;

  if (!filename) filename = "";
  if (filename != _getname(filename))       /* if not filename_only */
    return (*callback)(filename, extra);

  if (!pathlist || !*pathlist) pathlist = ".";
  n_pathlist = strlen(pathlist) + 1;
  stmp = alloca(strlen(pathlist) + 1 + strlen(filename) + 4 + 1);

  rc = 0;

  for(path=pathlist; *path; ) {
    char *p2;
    size_t n_p2;
    p2 = strchr(path, ';');
    n_p2 = p2 ? p2 - path : strlen(path);
    if (n_p2 > 0) {
      memcpy(stmp, path, n_p2);
      stmp[n_p2] = '\0';
      _fnslashify(stmp);
      if (*filename) {
	if (stmp[n_p2 - 1] != '/') {
	  stmp[n_p2] = '/';
	  stmp[n_p2 + 1] = '\0';
	}
      }
      else {
	if (stmp[n_p2 - 1] == '/')
	  stmp[n_p2 - 1] = '\0';
      }
      memcpy(stmp + strlen(stmp), filename, strlen(filename) + 1);
      rc = (*callback)(stmp, extra);
      if (rc != 0) break;
    }
    if (path[n_p2] == ';') ++n_p2;
    path += n_p2;
  }

  return rc;
}

static
int
search_prog_in_path(char *result, size_t lim_result, const char *filename)
{
  int n;
  struct callback_prog_struct cp;

  cp.buffer = result;
  cp.limit_buffer = lim_result;
  cp.length_filename = 0;
  n = enum_in_path(getenv ("PATH"), filename, callback_prog, &cp);
  if (n > 0) {
    n = cp.length_filename;
  }
  return n;
}


static
const char *
git_os2_rootprefix(void)
{
  char *rt;

  rt = getenv ("ROOT");
  if (!rt) rt = getenv ("UNIXROOT");
  if (!rt) rt = "";

  return rt;
}

#define setenv_override(n,v) setenv (n,v,!0)
#define setenv_non_override(n,v) setenv (n,v,0)


static char * wrapped_get_gitshell(int *warned)
{
  static char dummy[PATH_MAX + 1];
  char *p, *altsh;
  int rc_gitsh = -1, rc = -1;

  if (dummy[0])
    return dummy;

  altsh = "cmd.exe";
  p = getenv("GIT_SHELL");
  if (p) {
    rc_gitsh  = search_prog_in_path(dummy, sizeof(dummy), p);

    if (rc_gitsh >= 0)
      altsh = "$GIT_SHELL";
    else {
      if (warned && !*warned)
	warning("$GIT_SHELL (%s) is not found.", p);
    }
  }
  if (rc_gitsh < 0) {
    const char *p_rt;
    size_t n;
    p_rt = git_os2_rootprefix();
    n = strlen(p_rt);
    p = alloca(n + sizeof( "/bin/sh" ) + 1);
    memcpy(p, p_rt, n + 1);
    _fnslashify(p);
    if (n > 0 && p[n-1] == '/') {
      --n;
      p[n] = '\0';
    }
    memcpy(p + strlen(p), "/bin/sh", sizeof("/bin/sh"));
    rc_gitsh = search_prog_in_path(dummy, sizeof(dummy), p);
    if (rc_gitsh >= 0)
      altsh = "$UNIXROOT/bin/sh";
    else {
      p = getenv("PERL_SH_DIR");
      if (p && *p) {
	char *p_psh;
	n = strlen(p);
	p_psh = alloca(n + 4);
	snprintf(p_psh, n + 4, "%s/sh", p);
	rc = search_prog_in_path(dummy, sizeof(dummy), p_psh);
      }
      if (rc >= 0)
	altsh = "$PERL_SH/sh";
      else {
	p = getenv("SHELL");
	if (p && *p)
	  rc = search_prog_in_path(dummy, sizeof(dummy), p);
      }
      if (rc >= 0)
	altsh = "$SHELL";
      else {
	p = getenv("OS2SHELL");
	if (p && *p)
	  rc = search_prog_in_path(dummy, sizeof(dummy), p);
      }
      if (rc >= 0)
	altsh = "$OS2SHELL";
      else {
	p = getenv("EMXSHELL");
	if (p && *p)
	  rc = search_prog_in_path(dummy, sizeof(dummy), p);
      }
      if (rc >= 0)
	altsh = "$EMXSHELL";
      else {
	snprintf(dummy, sizeof(dummy), "%s", "cmd.exe");
      }
    }
  }

  if (rc_gitsh < 0) {
    if (warned && !*warned) {
      warning("/bin/sh is not exist. %s will be used alternatively.", altsh);
    }
  }

  _fnslashify(dummy);

#if !defined(ALLOW_OS2_CMD_AS_SHELL)
  {
    char *p;
    p = _getname(dummy);
    if (p && *p) {
      if ( stricmp(p,"cmd.exe")==0 || stricmp(p,"cmd")==0 ||
	   stricmp(p,"4os2.exe")==0 || stricmp(p,"4os2")==0 )
	die("Unix-style shell (sh) is required.");
    }
  }
#endif

  return dummy[0] ? dummy : NULL;
}
static char * wrapped_getenv_shell (const char *name, int *warned)
{
  return wrapped_get_gitshell(warned);
}

static char * wrapped_getenv_user (const char *name, int *warned)
{
  static char dummy_noname[] = "NONAME";
  char *p, *pu;

  p = getenv(name);
  if (!p) {
    pu = getenv("USERNAME");
    if (pu) {
      if (!*warned) warning("$%s is not set. $USERNAME (%s) will be used alternatively.", name, pu);
    }
    else {
      pu = dummy_noname;
      if (!*warned) warning("$%s is not set. NONAME will be used alternatively.", name);
    }
    return pu;
  }
  return p;
}

static char * wrapped_getenv_home (const char *name, int *warned)
{
  static char dummy[PATH_MAX + 1];
  char *p;

  p = getenv(name);
  if (p) {
    snprintf(dummy, sizeof(dummy), "%s", p);
  }
  if (!dummy[0]) {
#ifdef __EMX__
    _getcwd2(dummy, PATH_MAX);
#else
    getcwd(dummy, PATH_MAX);
#endif
    if (warned && !*warned)
      warning("$%s is not set. Current directory (%s) will be used.", name, dummy);
  }
  _fnslashify(dummy);

  return dummy;
}

static char * wrapped_getenv_tmpdir (const char *name, int *warned)
{
  static char dummy[PATH_MAX + 1];
  char *p;

  p = getenv(name);
  if (p) {
    snprintf(dummy, sizeof(dummy), "%s", p);
  }
  else {
    char *pt;
    pt = getenv("TMP");
    if (pt) {
      snprintf(dummy, sizeof(dummy), "%s", pt);
      if (!*warned) warning("$%s is not set. $TMP will be used.", name);
    }
    else {
#ifdef __EMX__
      _getcwd2(dummy, PATH_MAX);
#else
      getcwd(dummy, PATH_MAX);
#endif
      if (!*warned) warning("$%s is not set. Current directory will be used.", name);
    }
  }
  _fnslashify(dummy);
  return &(dummy[0]);
}

static char * wrapped_getenv_email (const char *name, int *warned)
{
  static char dummy[128];
  char *p;

  p = getenv(name);
  if (!p || !*p) {
    if (warned && !*warned) warning("$%s is not set.", name);
#if 0
    snprintf(dummy, sizeof(dummy), "%s", "i_have_no_email@nowhere.org");
#endif
    p = dummy; /* zero-length string "" */
  }

  return p;
}


char *
wrapped_getenv_for_os2 (const char *name)
{
  struct altered_env_table {
    char *envname;
    int warned_unset;
    char * (*altered_getenv)(const char *, int *);
  };

  static struct altered_env_table envtbl[] = \
    {
      { "HOME", 0, wrapped_getenv_home },
      { "USER", 0, wrapped_getenv_user },
      { "TMPDIR", 0, wrapped_getenv_tmpdir },
      { "EMAIL", 0, wrapped_getenv_email },
      { "GIT_SHELL", 0 ,wrapped_getenv_shell },
      { "SHELL", 0 ,wrapped_getenv_shell },
      { NULL, 0, NULL }
    };

  if (name && *name) {
    int i;
    char *p;
    for(i=0; envtbl[i].envname && envtbl[i].altered_getenv; i++) {
      if (strcmp(name, envtbl[i].envname) == 0) {
	p = (*(envtbl[i].altered_getenv))(name, &(envtbl[i].warned_unset));
	envtbl[i].warned_unset ++;
	return p;
      }
    }
  }
  return getenv(name);
}


static const char *
remap_charset(const char *org)
{
  size_t i, i_s, n_org;
  char c, *s, *s_up;

  enum {
    CMP_END = 0,
    CMP_CASE = 1,
    CMP_NOCASE
  };
  struct remap_table {
    int flag;
    const char *map_from;
    const char *map_to;
  } rmt[] = {
    { CMP_NOCASE, "SJIS", "IBM-932" },
    { CMP_NOCASE, "SHIFTJIS", "IBM-932" },
    { CMP_NOCASE, "UJIS", "IBM-eucJP" },
    { CMP_NOCASE, "EUCCN", "IBM-eucCN" },
    { CMP_NOCASE, "EUCJP", "IBM-eucJP" },
    { CMP_NOCASE, "EUCKR", "IBM-eucKR" },
    { CMP_NOCASE, "EUCTW", "IBM-eucTW" },
    { CMP_NOCASE, "BIG5", "IBM-950" }, /* workaround */
    { CMP_NOCASE, "ISO88591", "ISO-8859-1" }, /* workaround */
    { CMP_NOCASE, "ISO88592", "ISO-8859-2" }, /* workaround */
    { CMP_NOCASE, "ISO88593", "ISO-8859-3" }, /* workaround */
    { CMP_NOCASE, "ISO88594", "ISO-8859-4" }, /* workaround */
    { CMP_NOCASE, "ISO88595", "ISO-8859-5" }, /* workaround */
    { CMP_NOCASE, "ISO88596", "ISO-8859-6" }, /* workaround */
    { CMP_NOCASE, "ISO88597", "ISO-8859-7" }, /* workaround */
    { CMP_NOCASE, "ISO88598", "ISO-8859-8" }, /* workaround */
    { CMP_NOCASE, "ISO88599", "ISO-8859-9" }, /* workaround */
    { CMP_NOCASE, "ASCII", "IBM-367" }, /* workaround */
    { CMP_NOCASE, "ANSIX3.4", "IBM-367" }, /* workaround */
    { CMP_NOCASE, "USASCII", "IBM-367" }, /* workaround */
    { CMP_NOCASE, "ISO646", "IBM-367" }, /* workaround */
    { CMP_NOCASE, "ISO646US", "IBM-367" }, /* workaround */
    { CMP_NOCASE, "UTF8", "UTF-8" }, /* workaround */
    { CMP_NOCASE, "UCS2", "UCS-2" }, /* workaround */
    { CMP_NOCASE, "UTF16", "UCS-2" }, /* workaround */
    { CMP_NOCASE, "UTF16LE", "UCS-2@endian=little" }, /* workaround */
    { CMP_NOCASE, "UTF16BE", "UCS-2@endian=big" }, /* workaround */
    { CMP_NOCASE, "UCS2INTERNAL", "UCS-2" }, /* gnu libiconv extension */
    { CMP_NOCASE, "WCHART", "UCS-2" }, /* gnu libiconv extension */
#ifdef MS932_IS_AVAILABLE
    { CMP_NOCASE, "CP932", "MS-932" },
    { CMP_NOCASE, "WINDOWS31J", "MS-932" },
#else
    { CMP_NOCASE, "MS932", "IBM-932" }, /* better than nothing, maybe... */
    { CMP_NOCASE, "WINDOWS31J", "IBM-932" },
#endif

    { CMP_END, NULL, NULL }
  };

  if (!org || !*org) return org;
  n_org = strlen(org);
  s = alloca(n_org + 1);
  s_up = alloca(n_org + 1);
  for(i=0, i_s=0 ; i<n_org; i++) {
    c = org[i];
    if ((unsigned char)c < 0x20) break;
    if (c == '-' || c == '_') continue;
    s[i_s] = c;
    s_up[i_s] = isalpha(c) ? toupper(c) : c;
    ++i_s;
  }
  s[i_s] = '\0';
  s_up[i_s] = '\0';

  for(i=0; rmt[i].flag != CMP_END; i++) {
    if (strcmp(rmt[i].map_from, rmt[i].flag == CMP_CASE ? s : s_up) == 0)
      return rmt[i].map_to;
  }

  return org;
}


/*
  iconv_open wrapper (for klibc's builtin iconv)
  treat \ and ~ characters as path separater under DBCS codepage.
*/

iconv_t wrapped_iconv_open_for_klibc (const char *cp_to, const char *cp_from)
{
  struct k_iconv_t {
    UconvObject uo_from;
    UconvObject uo_to;
    UniChar *ucp_to;
    UniChar *ucp_from;
  };
  iconv_t ic;

  ic = iconv_open(remap_charset(cp_to), remap_charset(cp_from));
  if (ic != (iconv_t)-1) {
    struct k_iconv_t *kic = (struct k_iconv_t *)ic;
    uconv_attribute_t ua;
    APIRET rc;
    rc = UniQueryUconvObject(kic->uo_from, &ua, sizeof(ua), 0, 0, 0);
    if (rc == 0) {
      ua.converttype |= CVTTYPE_PATH;
      UniSetUconvObject(kic->uo_from, &ua);
    }
    rc = UniQueryUconvObject(kic->uo_to, &ua, sizeof(ua), 0, 0, 0);
    if (rc == 0) {
      ua.converttype |= CVTTYPE_PATH;
      UniSetUconvObject(kic->uo_to, &ua);
    }
  }

  return ic;
}

/*
  unlink (for DOSish system )
*/

int wrapped_unlink_for_dosish_system (const char *pn)
{
  int rc;
  int errno_bak;

  rc = unlink(pn);
  errno_bak = errno;
  if (rc) {
    if (chmod(pn, 666) == 0) {
      rc = unlink(pn);
    }
    else {
      errno = errno_bak;
    }
  }
  return rc;
}

/*
  getpwuid
*/


struct passwd * wrapped_getpwuid_for_klibc (uid_t uid)
{
  static struct passwd pw_dummy;
  struct passwd *pw;

  pw = getpwuid(uid);
  if (pw) {
    memcpy(&pw_dummy, pw, sizeof(struct passwd));
  }
  pw = &pw_dummy;

#if 1
  if (!pw->pw_name || !pw->pw_name[0])
    pw->pw_name = wrapped_getenv_for_os2("USER");
  if (!pw->pw_gecos || !pw->pw_gecos[0])
    pw->pw_gecos = wrapped_getenv_for_os2("EMAIL");
  if (!pw->pw_dir || !pw->pw_dir[0])
    pw->pw_dir = wrapped_getenv_for_os2("HOME");
  if (!pw->pw_shell || ! pw->pw_shell[0])
    pw->pw_shell = wrapped_getenv_for_os2("SHELL");
#else
  {
  static char myhome[PATH_MAX];
  char *p;

  p = pw->pw_name;
  if (!p || !*p) {
    p = getenv("USERNAME");
    if (!p || !*p) p = getenv("USER");
    if (!p || !*p) {
      warning ("Your $USERNAME (or $USER) is not defined.");
      p = "NONAME";
    }
    pw->pw_name = p;
  }
  p = pw->pw_gecos;
  if (!p || !*p) {
    if (!p || !*p) p = getenv("EMAIL");
    if (!p || !*p) {
      warning ("Your $EMAIL is not defined.");
      p = "(no email)";
    }
    pw->pw_gecos = p;
  }
  p = pw->pw_dir;
  if (!p || !*p) {
    p = getenv("HOME");
    if (p && *p) {
      snprintf(myhome, sizeof(myhome), "%s", p);
    }
    if (!p && !myhome[0]) {
      warning ("Home directory $HOME is not defined.");
      getcwd(myhome, sizeof(myhome)-1);
    }
    _fnslashify(myhome);
    pw->pw_dir = myhome;
  }
  p = pw->pw_shell;
  if (!p || !*p) {
    p = getenv("SHELL");
    if (!p || !*p) p = getenv("OS2SHELL");
    if (!p || !*p) p = getenv("EMXSHELL");
    if (!p || !*p) {
      warning ("Default shell $SHELL is not defined.");
      /* p = getenv ("COMSPEC"); */
      p = "CMD.EXE";
    }
    pw->pw_shell = p;
  }
  }
#endif

  return pw;
}

int
check_file_exetype(const char *fn, char *lbuf, size_t n_lbuf)
{
  int rc;
  int fd;
  char buf[2];

  rc = FILE_EXETYPE_UNKNOWN;

  fd = open(fn, O_RDONLY | O_BINARY);
  if (fd < 0) return FILE_EXETYPE_ERROR;
  memset(buf, 0, sizeof(buf));
  read(fd, buf, sizeof(buf));

  if (buf[0]=='M'&&buf[1]=='Z')
    rc = FILE_EXETYPE_BINARY;
  else if (buf[0]=='N'&&buf[1]=='E')
    rc = FILE_EXETYPE_BINARY;
  else if (buf[0]=='L'&&buf[1]=='X')
    rc = FILE_EXETYPE_BINARY;
  else if (buf[0]=='#'&&buf[1]=='!') {
    if (lbuf && n_lbuf > 1) {
      int len;
      len = read(fd, lbuf, n_lbuf - 1);
      if (len >= 1) {
	lbuf[len] = '\0';
      }
      else
	lbuf[0] = '\0';
    }
    rc = FILE_EXETYPE_SH;
  }
  else if (!stricmp(_getext2(fn), ".cmd")) {
    /* todo: check CRLF */
    rc = FILE_EXETYPE_CMD;
  }
  close(fd);

  return rc;
}

static
char *
my_skipspc(char *p)
{
  while(*p == ' ' || *p == '\t') ++p;
  return p;
}
static
int
my_iseol(char *p)
{
  return (*p == '\0' || *p == '\r' || *p == '\n' || *p == 0x1a);
}
char *
my_skipnospc(char *p)
{
  while(!my_iseol(p)) {
    if (*p == ' ' || *p == '\t') break;
    ++p;
  }
  return p;
}

static
int
divide_str_to_args(char ***pargv, char *cmdline)
{
  int argc;

  argc = 0;
  *pargv = calloc(argc + 1, sizeof(char *));

  while(1) {
    cmdline = my_skipspc(cmdline);
    if (my_iseol(cmdline)) {
      *cmdline = '\0';
      break;
    }
    ++argc;
    *pargv = realloc(*pargv, (argc+1)*sizeof(char *));
    (*pargv)[argc-1] = cmdline;
    (*pargv)[argc] = NULL;
    cmdline = my_skipnospc(cmdline);
    if (*cmdline && !my_iseol(cmdline)) {
      *cmdline++ = '\0';
    }
  }

  return argc;
}

#ifdef i_need_debug_output
int
my_execv_debug(const char *p, char **argv)
{
  int i;
  printf("execv...\n");
  printf("  prog \"%s\"\n", p);
  for(i=0; argv[i]; i++) {
    printf("  argv[%d] \"%s\"\n", i, argv[i]);
  }
  return execv(p, argv);
}

#define execv(p,a) my_execv_debug(p,a)
#endif

static
int
wrapped_exec_2(int with_path, const char *progname_by_caller, char **argv_by_caller)
{
  int rc;
  char progname[_MAX_PATH + 1];
  char sharpbang[_MAX_PATH + 1];
  struct callback_prog_struct cp;
  char *execprog;
  char **argv;
  char *argv_extra;
  int argc;

  /* treat special case "/bin/sh" */
  if (strcmp(progname_by_caller, "/bin/sh") == 0) {
    progname_by_caller = wrapped_get_gitshell(NULL);
  }

  cp.buffer = progname;
  cp.limit_buffer = sizeof(progname);
  cp.length_filename = 0;
  rc = enum_in_path(with_path ? getenv ("PATH") : "", progname_by_caller, callback_prog, &cp);
  if (rc <= 0) {
    errno = ENOENT;
    return -1;
  }

  execprog = progname;
  for(argc=0; argv_by_caller[argc];)
    ++argc;
  argv = alloca((argc + 2 + 1) * sizeof(char *));
  argv[0] = argv_by_caller[0];
  argv_extra = alloca(strlen(progname)+1);
  memcpy(argv_extra, progname, strlen(progname) + 1);
  switch(check_file_exetype(progname, sharpbang, sizeof(sharpbang))) {
    case FILE_EXETYPE_SH:
    {
      char interp[_MAX_PATH + 1];
      struct callback_prog_struct cp_intp;
      int shargc;
      char **shargv = 0;
      shargc = divide_str_to_args(&shargv, sharpbang);
      argv = alloca( (shargc + argc + 1) * sizeof(char *));
      execprog = shargv[0];
      if (shargc >= 1) memcpy(argv, shargv, shargc * sizeof(char*));
      memcpy(argv + shargc, argv_by_caller, (argc + 1) * sizeof(char*));
      argv[shargc] = argv_extra;
      if (shargv) free(shargv);
      if (shargc < 1) {
	errno = ENOENT;
	return -1;
      }
      if (strcmp(argv[0], "/bin/sh") == 0) {
	argv[0] = wrapped_get_gitshell(NULL);
      }
      cp_intp.buffer = interp;
      cp_intp.limit_buffer = sizeof(interp);
      cp_intp.length_filename = 0;
      rc = enum_in_path(getenv("PATH"), argv[0], callback_prog, &cp_intp);
      if (rc <= 0) {
	errno = ENOENT;
	return -1;
      }
#if 1
      argv[0] = _getname(argv[0]);
#else
      argv[0] = _getname(argv[shargc]);
#endif
      rc = execv(interp, argv);
      break;
    }
    case FILE_EXETYPE_CMD:
    {
      int n;
      execprog = getenv ("COMSPEC"); /* cmd.exe (or 4os2.exe) */
      argv = alloca((argc + 2 + 1) * sizeof(char *));
      argv[0] = argv_by_caller[0];
      for(n=0; argv_extra[n]; n++)
	if (argv_extra[n] == '/') argv_extra[n] = '\\';
      argv[1] = argv_extra;
      memcpy(argv+2, argv_by_caller + 1, sizeof(char *) * argc);
      rc = execv(execprog, argv);
      break;
    }
    case FILE_EXETYPE_BINARY:
    {
      argv = alloca((argc + 2 + 1) * sizeof(char *));
      argv[0] = argv_by_caller[0];
      memcpy(argv + 1, argv_by_caller + 1, sizeof(char *) * argc);
      rc = execv(execprog, argv);
      break;
    }
    case FILE_EXETYPE_UNKNOWN:
    {
      /* assume a shell script */
      execprog = wrapped_get_gitshell(NULL);
      argv = alloca((argc + 1 + 1) * sizeof(char *));
      argv[0] = execprog;
      memcpy(argv + 1, argv_by_caller, sizeof(char *) * (argc + 1));
      rc = execv(execprog, argv);
      break;
    }
    default:
      errno = ENOEXEC;
      return -1;
  }

  return rc;
}


#if 0
static
int wrapped_exec_2(int with_path, const char *progname, char **argv)
{
  if (!progname && !*progname) return -1;

  /* treat special case '/bin/sh' */
  if (strcmp(progname, "/bin/sh") == 0) {
    char *sh;
    progname = wrapped_get_gitshell(NULL);
    sh = _getname(progname);
    if (argv[0] && argv[1] && strcmp(argv[1], "-c")==0 &&
	(stricmp(sh, "cmd.exe")==0 || stricmp(sh, "cmd")==0 ||
	 stricmp(sh, "4os2.exe")==0 || stricmp(sh, "4os2")==0 ) )
    {
      argv[1] = "/c";
    }
  }
  return with_path ? execvp(progname, argv) : execv(progname, argv);
}
#endif

int wrapped_execv_for_os2 (const char *progname, char **argv)
{
  return wrapped_exec_2(0, progname, argv);
}
int wrapped_execvp_for_os2 (const char *progname, char **argv)
{
  return wrapped_exec_2(1, progname, argv);
}

int wrapped_execl_for_os2 (const char *progname, const char *argv0, ...)
{
  int rc;
#if defined(__OS2__) && defined(__KLIBC__)
  rc = wrapped_exec_2(0, progname, (char**)&argv0);
#else
  size_t argc;
  const char **argv;
  const char *s;
  va_list args;

  va_start (args, argv0);
  for(argc=0,argv=NULL,s=argv0; ; s=va_arg(args, const char *)) {
    argv = (const char **)realloc(argv, (argc+1) * sizeof(const char *));
    argv[argc] = s;
    if (!s) break;
    ++argc;
  }
  rc = wrapped_exec_2(0, progname, (char **)argv);
  free(argv);
  va_end (args);
#endif
  return rc;
}
int wrapped_execlp_for_os2 (const char *progname, const char *argv0, ...)
{
  int rc;
#if defined(__OS2__) && defined(__KLIBC__)
  rc = wrapped_exec_2(1, progname, (char**)&argv0);
#else
  size_t argc;
  const char **argv;
  const char *s;
  va_list args;

  va_start (args, argv0);
  for(argc=0,argv=NULL,s=argv0; ; s=va_arg(args, const char *)) {
    argv = (const char **)realloc(argv, (argc+1) * sizeof(const char *));
    argv[argc] = s;
    if (!s) break;
    ++argc;
  }
  rc = wrapped_exec_2(1, progname, (char **)argv);
  free(argv);
  va_end (args);
#endif
  return rc;
}


#define SAFETY_LENGTH_READWRITE  16384
ssize_t git_os2_read (int fd, void *buf, size_t len)
{
  if ((uintptr_t)buf >= 512 * 1024 * 1024 && len > SAFETY_LENGTH_READWRITE)
    len = SAFETY_LENGTH_READWRITE;
  return read(fd, buf, len);
}

ssize_t git_os2_write (int fd, const void *buf, size_t len)
{
  if ((uintptr_t)buf >= 512 * 1024 * 1024 && len > SAFETY_LENGTH_READWRITE)
    len = SAFETY_LENGTH_READWRITE;
  return write(fd, buf, len);
}


static const char git_xpath[] = "/" RELATIVE_EXEC_PATH;
static const char *git_base_dir;
static int git_os2_std_layout;

static
const char *
git_os2_prepare_runtime_prefix(int *std_layout)
{
  static const char *tbl[] = {
    "/" BINDIR,
    git_xpath,
    NULL
  };
  static char *prefix;
  APIRET rc;
  PPIB pib;
  PTIB tib;
  CHAR stmp[_MAX_PATH+1];
  size_t ns;
  int i;
  char *s;
  int stdlay = 0;

  if (!prefix) {
    prefix = malloc (_MAX_PATH + 1);
    if (!prefix)
      return NULL;
  }
  rc = DosGetInfoBlocks(&tib, &pib);
  if (rc != 0) return NULL;
  rc = DosQueryModuleName((HMODULE)(pib->pib_hmte), sizeof(stmp), stmp);
  if (rc != 0) return NULL;
  _fnslashify(stmp);
  s = _getname(stmp);
  if (!s || !*s || s == stmp) return NULL;
  --s;
  if (*s != '/') return NULL;
  *s = '\0';
  ns = strlen(stmp);
  for(i=0, s=0; tbl[i]; i++) {
    size_t nt;
    nt = strlen(tbl[i]);
    if (nt <= ns && stricmp(tbl[i], stmp + ns - nt) == 0) {
      stmp[ns - nt] = '\0';
      s = stmp;
      stdlay = !0;
      break;
    }
  }
  if (!s) s = stmp;
  memcpy(prefix, s, strlen(s) + 1);
  if (std_layout) *std_layout = stdlay;
  return prefix;
}

const char *
git_os2_runtime_prefix (void)
{
  if (!git_base_dir)
    git_base_dir = git_os2_prepare_runtime_prefix(&git_os2_std_layout);
  return git_base_dir;
}


static
void
git_os2_prepare_direnv()
{
  static const char *perllibdirs[] = {
    "/share/perl5",
    "/lib/site_perl",
    "/perl/blib/lib",
    NULL
  };

  char stmp[_MAX_PATH];
  char gitpm[_MAX_PATH];
  struct stat st;
  char *s;
  int n;
  int i;

  if (!git_base_dir) git_os2_runtime_prefix();

  n = snprintf(stmp, sizeof(stmp), "%s%s", git_base_dir,
		 git_os2_std_layout ? git_xpath : "");
  if (n < sizeof(stmp))
    setenv_non_override(EXEC_PATH_ENVIRONMENT, stmp);

  /* find the location of Git.pm file for GITPERLLIB */
  s = NULL;
  for (i = 0; perllibdirs[i]; i++) {
    snprintf(stmp, sizeof(stmp), "%s%s", git_base_dir, perllibdirs[i]);
    snprintf(gitpm, sizeof(gitpm), "%s/%s", stmp, "Git.pm");
    if (stat(gitpm, &st) == 0 && S_ISREG(st.st_mode)) {
      s = stmp;
      break;
    }
  }

  if (s)
    setenv_non_override("GITPERLLIB", s);
}

const
char *
git_os2_default_template_dir (void)
{
  static char tdir[_MAX_PATH];

  if (!tdir[0]) {
    struct stat st;
    const char *rt;
    char *stmp;
    size_t nstmp;
    nstmp = sizeof(tdir);
    stmp = alloca(nstmp);
    rt = git_os2_rootprefix();
    if (!git_os2_std_layout && snprintf(stmp, nstmp, "%s/templates/blt", git_base_dir) < nstmp && stat(stmp, &st) >= 0 && S_ISDIR(st.st_mode))
      memcpy(tdir, stmp, strlen(stmp) + 1);
    else if (snprintf(stmp, nstmp, "%s/%s", git_base_dir, RELATIVE_TEMPLATE_DIR) < nstmp && stat(stmp, &st) >= 0 && S_ISDIR(st.st_mode))
      memcpy(tdir, stmp, strlen(stmp) + 1);
    else if (snprintf(stmp, nstmp, "%s/usr/%s", git_os2_rootprefix(), RELATIVE_TEMPLATE_DIR) < nstmp && stat(stmp, &st) >= 0 && S_ISDIR(st.st_mode))
      memcpy(tdir, stmp, strlen(stmp) + 1);
  }

  return tdir[0] ? (const char *)tdir : PREFIX "/" RELATIVE_TEMPLATE_DIR;
}

const
char *
git_os2_default_html_path (void)
{
  char *env;

  env = getenv(HTML_PATH_ENVIRONMENT);
  if (!env) {
    char *s;
    size_t ns;
    ns = strlen(git_base_dir) + 1 + sizeof("/" RELATIVE_HTMLPATH) + 1;
    s = alloca(ns);
    snprintf(s, ns, "%s/" RELATIVE_HTMLPATH, git_base_dir);
    setenv_non_override(HTML_PATH_ENVIRONMENT, s);
    env = getenv(HTML_PATH_ENVIRONMENT);
  }

  return env ? (const char *)env : PREFIX "/" RELATIVE_HTMLPATH;
}

const
char *
git_os2_default_info_path (void)
{
  char *env;

  env = getenv(INFO_PATH_ENVIRONMENT);
  if (!env) {
    char *s;
    size_t ns;
    ns = strlen(git_base_dir) + 1 + sizeof("/" RELATIVE_INFOPATH) + 1;
    s = alloca(ns);
    snprintf(s, ns, "%s/" RELATIVE_INFOPATH, git_base_dir);
    setenv_non_override(INFO_PATH_ENVIRONMENT, s);
    env = getenv(INFO_PATH_ENVIRONMENT);
  }

  return env ? (const char *)env : PREFIX "/" RELATIVE_INFOPATH;
}

const
char *
git_os2_default_man_path (void)
{
  char *env;

  env = getenv(MAN_PATH_ENVIRONMENT);
  if (!env) {
    char *s;
    size_t ns;
    ns = strlen(git_base_dir) + 1 + sizeof("/" RELATIVE_MANPATH) + 1;
    s = alloca(ns);
    snprintf(s, ns, "%s/" RELATIVE_MANPATH, git_base_dir);
    setenv_non_override(MAN_PATH_ENVIRONMENT, s);
    env = getenv(MAN_PATH_ENVIRONMENT);
  }

  return env ? (const char *)env : PREFIX "/" RELATIVE_MANPATH;
}


static
int
git_os2_find_altcmd(char *result, size_t lim_result, const char *filename)
{
  const char usrlbin[] = "/usr/local/bin";
  const char moz[] =     "/Moztools";
  const char usrbin[] =  "/usr/bin";
#if 0
  const char rtbin[] =   "/bin";
#endif
  int rc = -1;
  const char *rt;
  char *tmpbuf;
  size_t n_filename;
  size_t n_tmplim;

  if (!filename) return rc;
  rt = git_os2_rootprefix();

  n_filename = strlen(filename);
  n_tmplim = strlen(rt) + sizeof(usrlbin) + 3 + n_filename + 1;
  tmpbuf = alloca(n_tmplim);

  /* gnu???? in PATH */
  snprintf(tmpbuf, n_tmplim, "gnu%s", filename);
  rc = search_prog_in_path(result, lim_result, tmpbuf);
  if (rc > 0) return rc;
  /* g????  in PATH */
  snprintf(tmpbuf, n_tmplim, "g%s", filename);
  rc = search_prog_in_path(result, lim_result, tmpbuf);
  if (rc > 0) return rc;

  /* $ROOT/usr/bin/???? */
  snprintf(tmpbuf, n_tmplim, "%s%s/%s", rt, usrbin, filename);
  rc = search_prog_in_path(result, lim_result, tmpbuf);
  if (rc > 0) return rc;
  /* $ROOT/usr/local/bin/???? */
  snprintf(tmpbuf, n_tmplim, "%s%s/%s", rt, usrlbin, filename);
  rc = search_prog_in_path(result, lim_result, tmpbuf);
  if (rc > 0) return rc;

  /* $ROOT/Moztools/gnu???? */
  snprintf(tmpbuf, n_tmplim, "%s%s/gnu%s", rt, moz, filename);
  rc = search_prog_in_path(result, lim_result, tmpbuf);
  if (rc > 0) return rc;
  /* $ROOT/Moztools/g???? */
  snprintf(tmpbuf, n_tmplim, "%s%s/g%s", rt, moz, filename);
  rc = search_prog_in_path(result, lim_result, tmpbuf);
  if (rc > 0) return rc;
  /* $ROOT/Moztools/???? */
  snprintf(tmpbuf, n_tmplim, "%s%s/%s", rt, moz, filename);
  rc = search_prog_in_path(result, lim_result, tmpbuf);

  return rc;
}


static
void
git_os2_prepare_unixcmd()
{
  char stmp[_MAX_PATH+1];
  int rc;
  char *e;

  e = "GIT_FIND";
  if (!getenv (e)) {
    rc = git_os2_find_altcmd(stmp, sizeof(stmp), "find");
    if (rc > 0) {
      rc = (setenv_non_override(e, stmp) >= 0) ? 1 : -1;
      warning ("$%s is not defined. Assume %s.", e, stmp);
    }
    if (rc < 0) {
      error ("$s is not defined. Some commands should not work correctly.");
    }
  }
  e = "GIT_SORT";
  if (!getenv (e)) {
    rc = git_os2_find_altcmd(stmp, sizeof(stmp), "sort");
    if (rc > 0) {
      rc = (setenv_non_override(e, stmp) >= 0) ? 1 : -1;
      warning ("$%s is not defined. Assume %s.", e, stmp);
    }
    if (rc < 0) {
      error ("$s is not defined. Some commands should not work correctly.");
    }
  }
}


static
int setbinmode_for_file(int fd)
{
  struct stat st;
  int rc;

  rc = fstat(fd, &st);
  if (rc == 0) {
    rc = setmode(fd, S_ISCHR(st.st_mode) ? O_TEXT : O_BINARY);
  }
  return rc;
}

/* Conversion functions between UTF-8 and system encoding */

static int fromutf8(char *out, int outsize, const char *in)
{
	static iconv_t cd = (iconv_t)-1;

	const char *inbuf = in;
	size_t inleft = strlen(inbuf) + 1;
	char *outbuf = out;
	size_t outleft = outsize;

	if (cd == (iconv_t)-1)
		cd = iconv_open("", "UTF-8");

	if (iconv(cd, &inbuf, &inleft, &outbuf, &outleft) == (size_t)-1)
		return -1;

	return 0;
}

static char *fromutf8dup(const char *in)
{
	char *out = NULL;
	size_t size = 256;
	int ret = -1;

	while (ret == -1) {
		out = realloc(out, size);
		if (!out)
			die("Out of memory, realloc failed");

		ret = fromutf8(out, size, in);
		if (ret == -1 && errno != E2BIG) {
			free(out);
			return NULL;
		}

		size <<= 1;
	}

	/* Fit memory size to length of string */
	return realloc(out, strlen(out) + 1);
}

static int toutf8(char *out, int outsize, const char *in)
{
	static iconv_t cd = (iconv_t)-1;

	const char *inbuf = in;
	size_t inleft = strlen(inbuf) + 1;
	char *outbuf = out;
	size_t outleft = outsize;

	if (cd == (iconv_t)-1)
		cd = iconv_open("UTF-8", "");

	if (iconv(cd, &inbuf, &inleft, &outbuf, &outleft) == (size_t)-1)
		return -1;

	return 0;
}

static char *toutf8dup(const char *in)
{
	char *out = NULL;
	size_t size = 256;
	int ret = -1;

	while( ret == -1 ) {
		out = realloc(out, size);
		if (!out)
			die("Out of memory, realloc failed");

		ret = toutf8(out, size, in);
		if (ret == -1 && errno != E2BIG) {
			free(out);
			return NULL;
		}

		size <<= 1;
	}

	/* Fit memory size to length of string */
	return realloc(out, strlen(out) + 1);
}

int git_os2_main_prepare (int * p_argc, char ** * p_argv)
{
  _control87(MCW_EM, MCW_EM); /* mask all FPEs */
  _fmode_bin = 1; /* -Zbin-files */

  if (p_argc && p_argv && *p_argc > 0 && *p_argv) {
    int i;

    /* glob (I don't know correct order...) */
    _response(p_argc, p_argv);
    _wildcard(p_argc, p_argv);

    /* strip CRs from arguments */
    for(i=0; i<*p_argc && (*p_argv)[i]; ++i) {
      char *s;
      for(s=(*p_argv)[i]; (s = strchr(s, '\r'))!=NULL && *s;) {
	memmove(s, s+1, strlen(s));
      }
    }

#if 1
    _fnslashify((*p_argv)[0]);
#else
    {
      /* shashify and strip the extension ".exe" */
      char *p = (*p_argv)[0];
      _fnslashify(p);
      p = _getname(p);
      if (p) {
	char *prog, *pext;
	prog = strdup(p);
	pext = _getext(prog);
	if (pext) *pext = '\0';
	(*p_argv)[0] = prog;
      }
    }
#endif
  }

  /* Conversion system encoding path to UTF-8 path */
  for (int i = *p_argc - 1; i >= 0; i--) {
    char *p = toutf8dup((*p_argv)[i]);
    if (!p)
      die("Cannot convert %s to UTF-8 encoding.\n", p);
    (*p_argv)[i] = p;
  }

  setbinmode_for_file(fileno(stdin));
  setbinmode_for_file(fileno(stdout));
  setbinmode_for_file(fileno(stderr));

  git_os2_prepare_runtime_prefix(&git_os2_std_layout);
#if defined(GIT_OS2_USE_DEFAULT_BROWSER)
  git_os2pm_prepare_default_browser();
#endif

  if (!getenv ("HOME"))
    die("Set your home directory to the environment variable `HOME'.\n");
  git_os2_prepare_direnv();
  git_os2_prepare_unixcmd(); /* check Unixish sort/find */
  wrapped_getenv_for_os2 ("GIT_SHELL"); /* warn if /bin/sh or $GIT_SHELL not exist */

  /*
   * Workaround the bug of kLIBC v0.6.6 which is to fail to execute the
   * program when a directory with the same name in the current directory.
   * For example, some commands such as `git push' fails if a directory named
   * `git' in the current directory.
   * This has effects on codes using _path2() such as spawnvpe().
   */
   if (!getenv("EMXPATH"))
     putenv("EMXPATH=");

  /*
   * Make perl not translate CRLF to LF and vice versa.
   * From git 2.20.0, applying patches using `git add -p' fails due to LF to
   * CRLF conversion of perl.
   */
   putenv("PERLIO=perlio");

#ifdef i_need_debug_output
  {
  extern const char *system_path(const char *path);
  extern const char *git_exec_path(void);

  warning ("htmlpath: %s", system_path(GIT_HTML_PATH));
  warning ("infopath: %s", system_path(GIT_INFO_PATH));
  warning ("manpath: %s", system_path(GIT_MAN_PATH));
  warning ("execpath: %s", git_exec_path());
  warning ("template: %s", system_path(DEFAULT_GIT_TEMPLATE_DIR));
  warning ("perllib: %s", getenv("GITPERLLIB"));
  }
#endif

  return 0;
}

/*
   On kLIBC, if high-mem enabled modules and high-mem disabled moduels
   are linked together, high-mem support is not enabled.
   For example, in case of high-mem enabled executable linking against
   high-mem disabled DLLs, high-mem is not enabled.
   To avoid this, explicitly call high-mem allocation functions, and fall
   back into low-mem allocation functions if it fails.
*/

#include <emx/umalloc.h>

void *malloc (size_t size)
{
	void *ptr = _hmalloc (size);

	return ptr ? ptr : _lmalloc (size);
}

void *calloc (size_t elements, size_t size)
{
	void *ptr = _hcalloc (elements, size);

	return ptr ? ptr : _lcalloc (elements, size);
}

void *realloc (void *mem, size_t size)
{
	void *ptr = _hrealloc (mem, size);

	return ptr ? ptr : _lrealloc (mem, size);
}

int git_os2_offset_1st_component(const char *path)
{
	char *pos = (char *)path;

	/* unc paths */
	if (!skip_dos_drive_prefix(&pos) &&
			is_dir_sep(pos[0]) && is_dir_sep(pos[1])) {
		/* skip server name */
		pos = strpbrk(pos + 2, "\\/");
		if (!pos)
			return 0; /* Error: malformed unc path */

		do {
			pos++;
		} while (*pos && !is_dir_sep(*pos));
	}

	return pos + is_dir_sep(*pos) - path;
}

/* Copied from run-command.c */
static char **prep_childenv(const char *const *deltaenv)
{
	extern char **environ;
	char **childenv;
	struct string_list env = STRING_LIST_INIT_DUP;
	struct strbuf key = STRBUF_INIT;
	const char *const *p;
	int i;

	/* Construct a sorted string list consisting of the current environ */
	for (p = (const char *const *) environ; p && *p; p++) {
		const char *equals = strchr(*p, '=');

		if (equals) {
			strbuf_reset(&key);
			strbuf_add(&key, *p, equals - *p);
			string_list_append(&env, key.buf)->util = (void *) *p;
		} else {
			string_list_append(&env, *p)->util = (void *) *p;
		}
	}
	string_list_sort(&env);

	/* Merge in 'deltaenv' with the current environ */
	for (p = deltaenv; p && *p; p++) {
		const char *equals = strchr(*p, '=');

		if (equals) {
			/* ('key=value'), insert or replace entry */
			strbuf_reset(&key);
			strbuf_add(&key, *p, equals - *p);
			string_list_insert(&env, key.buf)->util = (void *) *p;
		} else {
			/* otherwise ('key') remove existing entry */
			string_list_remove(&env, *p, 0);
		}
	}

	/* Create an array of 'char *' to be used as the childenv */
	childenv = xmalloc((env.nr + 1) * sizeof(char *));
	for (i = 0; i < env.nr; i++)
		childenv[i] = env.items[i].util;
	childenv[env.nr] = NULL;

	string_list_clear(&env, 0);
	strbuf_release(&key);
	return childenv;
}

/* conflict: exit() macro in git-compat-util.h */
#undef exit
#include <process.h>

#include "strvec.h"

int os2_spawnvpe(const char *ucmd, const char **uargv, char **udeltaenv,
		const char *udir, int fhin, int fhout, int fherr)
{
	char cmd[_MAX_PATH];
	const char **argv;
	char **deltaenv;
	char dir[_MAX_PATH];
	char path[_MAX_PATH];
	char *saveddir;
	int savedin;
	int savedout;
	int savederr;
	char **childenv;
	fd_set fds;
	int fl;
	int i;
	int pid = -1;
	int savederrno;
	int size;
	int delta;

	if (fromutf8(cmd, sizeof(cmd), ucmd) == -1)
		return -1;

	delta = 8;
	size = 1;   /* 1 for last NULL */
	ALLOC_ARRAY(argv, size);
	for (i = 0; uargv[i]; i++) {
		if (i + 1 == size) {
			size += delta;
			REALLOC_ARRAY(argv, size);
		}

		argv[i] = fromutf8dup(uargv[i]);
		if (!argv[i])
			goto exit_cleanup;
	}
	argv[i] = NULL;

	delta = 16;
	size = 1;   /* 1 for last NULL */
	ALLOC_ARRAY(deltaenv, size);
	for (i = 0; udeltaenv[i]; i++) {
		if (i + 1 == size) {
			size += delta;
			REALLOC_ARRAY(deltaenv, size);
		}

		deltaenv[i] = fromutf8dup(udeltaenv[i]);
		if (!deltaenv[i])
			goto exit_cleanup;
	}
	deltaenv[i] = NULL;

	if (udir && fromutf8(dir, sizeof(dir), udir) == -1)
		goto exit_cleanup;

	if (_path2(cmd, ".exe", path, sizeof(path)) == -1)
		return -1;
	_fnslashify(path);

	/* Duplicate the given file handle to standard IO handle if needed */
	if (fhin != 0) {
		savedin = dup(0);
		dup2(fhin, 0);
	}

	if (fhout != 1) {
		savedout = dup(1);
		dup2(fhout, 1);
	}

	if (fherr != 2) {
		savederr = dup(2);
		dup2(fherr, 2);
	}

	/*
	 * Set FD_CLOEXEC bit of file handles other than standard IO handles
	 * to prevent a child from inheriting them. This is the main cause of
	 * a hang when redirecting pipe.
	 */
	FD_ZERO(&fds);
	for (i = 3; i < FD_SETSIZE; i++) {
		fl = fcntl(i, F_GETFD);
		if (fl != -1 && !(fl & FD_CLOEXEC)) {
			fcntl(i, F_SETFD, fl | FD_CLOEXEC);
			FD_SET(i, &fds);
		}
	}

	/* Build an environment from deltaenv for a child */
	childenv = prep_childenv((const char *const *)deltaenv);

	if (udir) {
		saveddir = _getcwd2(NULL, 0);
		if (_chdir2(dir) == -1)
			die("cannot chdir to %s", dir);
	}

	pid = spawnve(P_NOWAIT, path, (char *const *)argv, childenv);
	savederrno = errno;
	if (pid == -1 && errno == EACCES &&
	    check_file_exetype(path, NULL, 0) == FILE_EXETYPE_SH) {
		/*
		 * Workaround for a bug of kLIBC v0.6.6 which is to fail to execute
		 * a script in the directory whose name is the same as the interpreter
		 * of it. Use sh -c to execute it.
		 */
		struct strvec shargv = STRVEC_INIT;

		if (_getname(path) == path) {
			/*
			 * sh -c does not search a script in a current directory. To
			 * execute the script in the current directory, prepend ./
			 * explicitly. Assume path is sufficient to hold ./ + path itself.
			 */
			memmove(path + 2, path, strlen(path) + 1);
			path[0] = '.';
			path[1] = '/';
		}

		/* Construct arguments list for sh -c */
		strvec_push(&shargv, wrapped_get_gitshell(NULL));
		strvec_push(&shargv, "-c");
		strvec_pushf(&shargv, "%s \"$@\"", path);
		strvec_pushv(&shargv, argv);
		pid = spawnvpe(P_NOWAIT, shargv.v[0],
			       (char *const *)shargv.v, childenv);
		savederrno = errno;

		strvec_clear(&shargv);
	}

	if (udir) {
		if (_chdir2(saveddir) == -1)
			die("cannot chdir to %s", saveddir);
		free(saveddir);
	}

	free(childenv);

	/* Clear FD_CLOEXEC bit which was set above */
	for (i = 3; i < FD_SETSIZE; i++) {
		if (FD_ISSET(i, &fds)) {
			fcntl(i, F_SETFD, fcntl(i, F_GETFD) & ~FD_CLOEXEC);
		}
	}

	/* Restore standard IO handles */
	if (fhin != 0) {
		dup2(savedin, 0);
		close(savedin);
	}

	if (fhout != 1) {
		dup2(savedout, 1);
		close(savedout);
	}

	if (fherr != 2) {
		dup2(savederr, 2);
		close(savederr);
	}

exit_cleanup:
	for (i = 0; deltaenv[i]; i++)
		free(deltaenv[i]);
	free(deltaenv);

	for (i = 0; argv[i]; i++)
		free((char *)argv[i]);
	free(argv);

	errno = savederrno;

	return pid;
}

/* UTF-8 version file IO functions */

int _std_unlink(const char *);

int unlink(const char *upath)
{
	char path[PATH_MAX];

	if (fromutf8(path, sizeof(path), upath) == -1)
		return -1;

	return _std_unlink(path);
}

int _std_rmdir(const char *);

int rmdir(const char *upath)
{
	char path[PATH_MAX];

	if (fromutf8(path, sizeof(path), upath) == -1)
		return -1;

	return _std_rmdir(path);
}

int _std_mkdir(const char *, mode_t);

int mkdir(const char *upath, mode_t mode)
{
	char path[PATH_MAX];

	if (fromutf8(path, sizeof(path), upath) == -1)
		return -1;

	return _std_mkdir(path, mode);
}

int _std_open(const char *, int, ...);

int open(const char *upath, int oflag, ...)
{
	char path[PATH_MAX];
	va_list args;
	int pmode = 0;
	int ret;

	if (fromutf8(path, sizeof(path), upath) == -1)
		return -1;

	va_start(args, oflag);

	if (oflag & O_CREAT)
		pmode = va_arg(args, int);

	va_end(args);

	return _std_open(path, oflag, pmode);
}

FILE *_std_fopen(const char *, const char *);

#undef fopen
FILE *fopen(const char *upath, const char *mode)
{
	char path[PATH_MAX];

	if (fromutf8(path, sizeof(path), upath) == -1)
		return NULL;

	return _std_fopen(upath, mode);
}

FILE *_std_freopen(const char *, const char *, FILE *);

FILE *freopen(const char *upath, const char *mode, FILE *stream)
{
	char path[PATH_MAX];

	if (fromutf8(path, sizeof(path), upath) == -1)
		return NULL;

	return _std_freopen(upath, mode, stream);
}

int _std_access(const char *, int);

int access(const char *upath, int mode)
{
	char path[PATH_MAX];

	if (fromutf8(path, sizeof(path), upath) == -1)
		return -1;

	return _std_access(upath, mode);
}

int _std_chdir(const char *);

int chdir(const char *upath)
{
	char path[PATH_MAX];

	if (fromutf8(path, sizeof(path), upath) == -1)
		return -1;

	return _std_chdir(path);
}

#include <InnoTekLIBC/backend.h>

int _std_chmod(const char *, int);

int chmod(const char *upath, int pmode)
{
	char path[PATH_MAX];
	int ret;

	if (fromutf8(path, sizeof(path), upath) == -1)
		return -1;

	ret = _std_chmod(path, pmode);
	if (ret < 0 && errno == EACCES) {
		/* try fchmod() if path is opened by us */
		char fullpath[PATH_MAX];
		int fd;
		char fdpath[PATH_MAX];

		if (_fullpath(fullpath, path, sizeof(fullpath)) == 0) {
			_fnslashify(fullpath);

			for (fd = 0; fd < FD_SETSIZE; fd++) {
				if (__libc_Back_ioFHToPath(fd, fdpath, sizeof(fdpath)) == 0 &&
				    !stricmp(fdpath, fullpath))
					return fchmod(fd, pmode);
			}
		}

		errno = EACCES;
		return -1;
	}

	return ret;
}

int _std_lstat(const char *, struct stat *);

int lstat(const char *upath, struct stat *buffer)
{
	char path[PATH_MAX];

	if (fromutf8(path, sizeof(path), upath) == -1)
		return -1;

	return _std_lstat(path, buffer);
}

int _std_stat(const char *, struct stat *);

int stat(const char *upath, struct stat *buffer)
{
	char path[PATH_MAX];

	if (fromutf8(path, sizeof(path), upath) == -1)
		return -1;

	return _std_stat(path, buffer);
}

int _std_utime(const char *, const struct utimbuf *);

int utime(const char *upath, const struct utimbuf *times)
{
	char path[PATH_MAX];

	if (fromutf8(path, sizeof(path), upath) == -1)
		return -1;

	return _std_utime(path, times);
}

char *_std_mktemp(char *);

char *mktemp(char *ustr)
{
	static char upath[PATH_MAX * 4];

	char str[PATH_MAX];
	char *path;

	if (fromutf8(str, sizeof(str), ustr) == -1)
		return NULL;

	path = _std_mktemp(str);
	if (!path)
		return NULL;

	if (toutf8(upath, sizeof(upath), path) == -1)
		return NULL;

	return upath;
}

int _std_mkstemp(char *);

int mkstemp(char *ustr)
{
	char str[PATH_MAX];

	if (fromutf8(str, sizeof(str), ustr) == -1)
		return -1;

	return _std_mkstemp(str);
}

char *_std_getcwd(char *, size_t);

char *getcwd(char *ubuf, size_t size)
{
	char buf[PATH_MAX];

	/* do not consider ubuf == NULL */
	if (!_std_getcwd(buf, sizeof(buf)))
		return NULL;

	if (toutf8(ubuf, size, buf) == -1)
		return NULL;

	return ubuf;
}

int _std_rename(const char *, const char *);

int rename(const char *uold, const char *unew)
{
	char oldname[PATH_MAX];
	char newname[PATH_MAX];

	if (fromutf8(oldname, sizeof(oldname), uold) == -1 ||
	    fromutf8(newname, sizeof(newname), unew) == -1)
		return -1;

	return _std_rename(oldname, newname);
}

int _std_link(const char *, const char *);

int link(const char *upath1, const char *upath2)
{
	char path1[PATH_MAX];
	char path2[PATH_MAX];

	if (fromutf8(path1, sizeof(path1), upath1) == -1 ||
	    fromutf8(path2, sizeof(path2), upath2) == -1)
		return -1;

	return _std_link(path1, path2);
}

/* UTF-8 version getenv()/putenv() */
char *_std_getenv(const char *);

char *getenv(const char *uname)
{
#define GETENV_MAX_RETAIN 64
	static char *uvalues[GETENV_MAX_RETAIN];
	static int index;

	char *name;
	char *value;
	char *uvalue;

	name = fromutf8dup(uname);
	if (!name)
		return NULL;

	value = _std_getenv(name);
	free(name);
	if (!value)
		return NULL;

	uvalue = toutf8dup(value);

	free(uvalues[index]);
	uvalues[index++] = uvalue;

	index %= GETENV_MAX_RETAIN;

	return uvalue;
}

int _std_putenv(const char *);

int putenv(const char *ustr)
{
	char *str;

	str = fromutf8dup(ustr);
	if (!str)
		return -1;

	return _std_putenv(str);
}

/*
todo:
  + test server commands.
  + check klibc implementation of functions/syscalls
  + cleanup/refactoring...
*/
