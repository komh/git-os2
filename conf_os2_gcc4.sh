#!/bin/sh

# * build prerequesticies *
#
# symbols:
# shell_for_git     sh (sh.exe) pathname (=/bin/sh)
# perl_for_git      perl pathname
# python_for_git    python (2.6) pathname
#
# symbols (environment variables):
# ASCIIDOC          asciidoc pathname
#
# install-path:
# openssl           /extras
# curl              /curl
#                   (openssl: /extras)
#                   (libssh2: /usr/local)
#                   not used (libidn: /extras)
# expat             /usr/local
# zlib              /usr/local
# ln4exe.exe        anywhere in %PATH%
#
# * quick'n dirty build (and install) step *
#
#   ./conf_os2_gcc4.sh
#   make -f Makefile.klibc
#   make -f Makefile.klibc install
#
# or under OS/2 command prompt (cmd/4os2):
#
#   \bin\sh ./conf_os2_gcc4.sh
#   .\makeos2.cmd install
#

#export CONFIG_SHELL=sh
#export SHELL=$CONFIG_SHELL

shell_for_git="/bin/sh"
perl_for_git="perl"
python_for_git="python"

#export SHELL_PATH="/bin/sh"
#export SHELL_PATH="$SHELL"
#export PERL_PATH="perl"
#export PYTHON_PATH="python"

export ASCIIDOC="$python_for_git /asciidoc/asciidoc.py"
#export XSLTPROC=xsltproc
#export XSLTPROC="echo xslt_is_not_available"

#gitbase="/usr"
#gitbase="/git"
#gitbase="/git-1.7.3.5"
#gitbase="/git-1.7.3.4"
#gitbase="/git-1.7.3.3"
#gitbase="/git-1.7.3.2"
#gitbase="/git-1.7.3.1"
#gitbase="/git-1.7.3"
#gitbase="/git-1.7.2.5"
#gitbase="/git-1.7.2.4"
#gitbase="/git-1.7.2.3"
#gitbase="/git-1.7.2.2"
#gitbase="/git-1.7.2.1"
#gitbase="/git-1.7.2"
#gitbase="/git-1.7.2.rc4"
#gitbase="/git-1.7.2.rc3"
#gitbase="/git-1.7.2.rc2"
#gitbase="/git-1.7.2.rc1"
#gitbase="/git-1.7.1.2"
#gitbase="/git-1.7.1.1"
#gitbase="/git-1.7.1"
#gitbase="/git-1.7.1.rc3"
#gitbase="/git-1.7.1.rc2"
#gitbase="/git-1.7.1.rc1"
#gitbase="/git-1.7.1.rc0"
#gitbase="/git-1.7.0.6"
#gitbase="/git-1.7.0.5"
#gitbase="/git-1.7.0.4"
#gitbase="/git-1.7.0.3"
#gitbase="/git-1.7.0.2"
#gitbase="/git-1.7.0.1"
#gitbase="/git-1.7.0"
#gitbase="/git-1.6.6.3"
#gitbase="/git-1.6.6.2"
#gitbase="/git-1.6.6.1"
#gitbase="/git-1.6.6"
#gitbase="/git-1.6.6.rc4"
#gitbase="/git-1.6.6.rc3"
#gitbase="/git-1.6.6.rc2"
#gitbase="/git-1.6.5.8"
#gitbase="/git-1.6.5.7"
#gitbase="/git-1.6.5.6"
#gitbase="/git-1.6.5.5"
#gitbase="/git-1.6.5.4"
#gitbase="/git-1.6.5.3"
#gitbase="/git-1.6.5.2"
#gitbase="/git-1.6.5.1"
#gitbase="/git-1.6.5"
#gitbase="/git-1.6.5.rc3"
#gitbase="/git-1.6.5.rc2"
#gitbase="/git-1.6.5.rc1"
#gitbase="/git-1.6.4.5"
#gitbase="/git-1.6.4.4"
#gitbase="/git-1.6.4.3"
#gitbase="/git-1.6.4.2"
#gitbase="/git-1.6.4.1"
#gitbase="/git-1.6.4"
#gitbase="/git-1.6.4.rc1"
#gitbase="/git-1.6.3.4"
#gitbase="/git-1.6.3.3"
#gitbase="/git-1.6.3.2"
#gitbase="/git-1.6.3.1"
#gitbase="/git-1.6.3"
#gitbase="/git-1.6.2.4"
#gitbase="/git-1.6.1.3"
#gitbase="/git-1.6.2"
#gitbase="/usr/local"

#curlbase="/curl"
curlbase="/usr"
#use_curl="--with-curl"
use_curl="--with-curl=$curlbase"
#use_curl="--without-curl"
#curlinc="-I$curlbase/include"
#curllib="-L$curlbase/lib"
#curlconfig="$curlbase/bin/curl-config"
#curlinc=`$curlconfig --cflags | sed -e s/-Z[A-Za-z_\-]*//g`
#curllib=`$curlconfig --libs | sed -e s/-Z[A-Za-z_\-]*//g`
#curlver=`$curlconfig --vernum`

#extrabase="/extras"
extrabase="/usr"
extrainc="-I$extrabase/include"
extralib="-L$extrabase/lib"

use_openssl="--with-openssl"
#use_openssl="--with-openssl=$extrabase"
#use_openssl="--without-openssl"

use_expat="--with-expat"
#use_expat="--without-expat"

#use_tcltk="--with-tcltk"
use_tcltk="--without-tcltk"

# gcc 3.4.x and higher
c_tune="-O3 -mtune=pentium4 -march=pentium"
# gcc 3.3.x and older
#c_tune="-O3 -mcpu=pentium2 -march=pentium"
c_warn="-Wall"
#c_warn="-Wall -pedantic -Wno-long-long"

CC="gcc"
CXX="g++"
CFLAGS="-s -std=gnu99 $c_warn $c_tune"
CXXFLAGS="$CFLAGS"
#CPPFLAGS="-I/usr/local/include $curlinc $extrainc $expatinc"
CPPFLAGS="-I/usr/include $curlinc $extrainc $expatinc"
#LIBS="-L/usr/local/lib $curllib $extralib $expatlib"
LIBS="-L/usr/lib -lintl $curllib $extralib $expatlib"
#LDFLAGS="-Zomf"
#LDFLAGS="-Zomf -Zbin-files"
LDFLAGS="-Zomf -Zbin-files -Zhigh-mem"

# always use default browser on "git help --web"
#CPPFLAGS += -DGIT_OS2_USE_DEFAULT_BROWSER

export CC
export CXX
export CFLAGS
export CXXFLAGS
export CPPFLAGS
export LIBS
export LDFLAGS


export ac_cv_lib_curl_curl_global_init=yes
export ac_cv_prog_TAR=tar

# dike '-lgen' and '-lresolv'
export ac_cv_lib_gen_basename=yes
export ac_cv_lib_resolv_hstrerror=yes

export lt_cv_path_SED="sed"
export LN="cp -p"
export lt_cv_path_NM="nm -B"
export lt_cv_sys_max_cmd_len="8192"


# bypass ld checks (to keep LDFLAGS)
export ld_dashr=no
export ld_wl_rpath=no
export ld_rpath=no
#export SAVE_LD_FLAGS="$LDFLAGS"

./configure \
 --enable-pthreads=no \
 --with-shell="$shell_for_git" \
 --with-perl="$perl_for_git" \
 --with-python="$python_for_git" \
 $use_openssl \
 $use_curl \
 $use_expat \
 $use_tcltk \
 "$@"

cp -p config.status config.status.backup
cp -p config.mak.autogen config.mak.backup

sed -e "s;@SHELL@;$SHELL;g" -e "s;@SHELL_PATH@;$shell_for_git;g" -e "s;@PERL_PATH@;$perl_for_git;g" -e "s;@PYTHON_PATH@;$python_for_git;g" -e "s;@LN_X@;ln4exe;g" -e "s;@CURL_CONFIG@;$curlconfig;g" -e "s;@LIBS@;$LIBS;g" < Makefile.klibc.in > Makefile.klibc
echo "make V=1 MAKESHELL=$SHELL $curl"' -f Makefile.klibc %1 %2 %3 %4 %5 %6 %7 %8 %9 2>&1 | tee makeos2.log' > makeos2.cmd
