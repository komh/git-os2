#! /bin/sh

config() {
  n=configure

  target=i686-pc-os2-emx

  export LDFLAGS="-Zomf -Zbin-files -Zhigh-mem -Zstack 8192"

  opts="
    --prefix=/@unixroot/usr/local
    --enable-pthreads
    --with-shell=sh
    --with-perl=perl
    --with-python=python
    --with-curl
    --with-openssl
    --with-expat
    --without-tcltk
"

  crossopts="
    --host=$target
"
}

# convert $2 to the relative path to $1
getrelpath() {
  dir1="$1"
  [ ${#dir1} -gt 1 ] && dir1=${dir1%/}
  dir2="$2"
  [ ${#dir2} -gt 1 ] && dir2=${dir2%/}

  while [ -n "$dir1" ] && [ -n "$dir2" ]; do
    d1="${dir1%%/*}"
    dir1="${dir1#$d1}"
    dir1="${dir1#/}"

    d2="${dir2%%/*}"
    dir2="${dir2#$d2}"
    dir2="${dir2#/}"

    if [ "$(echo "$d1" | tr [:upper:] [:lower])" != \
         "$(echo "$d2" | tr [:upper:] [:lower])" ]; then
      dir1="$d1/$dir1"
      dir1="${dir1%/}"

      dir2="$d2/$dir2"
      dir2="${dir2%/}"

      break
    fi
  done

  while [ -n "$dir1" ]; do
    d1="${dir1%%/*}"
    dir1="${dir1#$d1}"
    dir1="${dir1#/}"
    [ -z "$d1" ] && continue

    dir2="../$dir2"
  done

  getrelpath_result="${dir2%/}"
  getrelpath_result="${getrelpath_result:-.}"
}

run() {
  d="$(dirname "$0")"
  test -f "$d/$n" || { echo "\`$d/$n' not found !!!"; exit 1; }

  blddir="$d"

  # get the absolute path of configure
  srcdir="$(cd "$d"; pwd)"

  [ -f "$d/$n" ] || { echo "\`$d/$n' not found!!!"; exit 1; }

  [ -z "$OS2_SHELL" ] && opts="$opts $crossopts"

  mkdir -p "$blddir" && cd "$blddir" || exit 1

  # convrt the path of configure to the relative path to the build dir
  getrelpath "$(pwd)" "$srcdir"

  eval '"$getrelpath_result/$n"' $opts '"$@"'
}

config
run "$@"
