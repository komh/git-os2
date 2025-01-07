extproc sh

d=$(dirname "$0" | tr '\\' /)

n=configure
test -f "$d/$n." || { echo "\`$d/$n' not found !!!"; exit 1; }

opts="
    --enable-pthreads
    --with-shell=sh
    --with-perl=perl
    --with-python=python
    --with-curl
    --with-openssl
    --with-expat
    --without-tcltk
    LDFLAGS='-Zomf -Zbin-files -Zhigh-mem -Zstack 8192'
"

eval "$d/$n" $opts "$@" 2>&1 | tee "$n.log"
