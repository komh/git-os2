extproc sh

# enable curl with a static lib, NO_CURL=
ac_cv_lib_curl_curl_global_init=yes

# -lintl not in libc, LIBC_CONTAINS_LIBINTL=
ac_cv_lib_c_gettext=no

# disable NLS, NO_GETTEXT=YesPlease
ac_cv_header_libintl_h=no

./configure. --enable-pthreads \
             --with-shell=sh \
             --with-perl=perl \
             --with-python=python \
             --with-curl \
             --with-openssl \
             --with-expat \
             --without-tcltk \
             LDFLAGS="-Zomf -Zbin-files -Zhigh-mem" \
             ac_cv_lib_curl_curl_global_init=$ac_cv_lib_curl_curl_global_init \
             ac_cv_lib_c_gettext=$ac_cv_lib_c_gettext \
             ac_cv_header_libintl_h=$ac_cv_header_libintl_h \
             "$@"
