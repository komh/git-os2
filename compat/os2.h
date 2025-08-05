#include "os2-posix.h"

extern const char *git_os2_default_template_dir (void);
extern const char *git_os2_default_html_path (void);
extern const char *git_os2_default_info_path (void);
extern const char *git_os2_default_man_path (void);

#undef DEFAULT_GIT_TEMPLATE_DIR
#define DEFAULT_GIT_TEMPLATE_DIR (git_os2_default_template_dir ())
#undef GIT_HTML_PATH
#define GIT_HTML_PATH (git_os2_default_html_path ())
#undef GIT_INFO_PATH
#define GIT_INFO_PATH (git_os2_default_info_path ())
#undef GIT_MAN_PATH
#define GIT_MAN_PATH (git_os2_default_man_path ())

#define PATH_SEP ';'

int os2_spawnvpe(const char *cmd, const char **argv, char **deltaenv,
		 const char *dir, int fhin, int fhout, int fherr);

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
