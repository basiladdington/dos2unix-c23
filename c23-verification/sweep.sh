#!/usr/bin/env zsh
# Code page sweep: all 256 byte values through every conversion table.
# usage: sweep.sh MODERN_DIR BASELINE_DIR WORKDIR
# Each dir must hold dos2unix, mac2unix, unix2dos and unix2mac.
# Compares stdout, stderr and exit status of 4 programs x 9 modes x 4 option sets.
P=$1; H=$2; W=$3
rm -rf $W; mkdir -p $W; cd $W
python3 -c "open('all.bin','wb').write(bytes(range(256))*2 + b'tail\r\nline\nmac\rend')"
fail=0; n=0
for prog in dos2unix mac2unix unix2dos unix2mac; do
  for mode in -ascii -7 -iso -437 -850 -860 -863 -865 -1252; do
    for extra in "" "-l" "-e" "-l -e"; do
      n=$((n+1))
      $P/$prog -f $mode ${=extra} < all.bin > a.out 2> a.err; ra=$?
      $H/$prog -f $mode ${=extra} < all.bin > b.out 2> b.err; rb=$?
      if ! cmp -s a.out b.out || ! cmp -s a.err b.err || [ $ra != $rb ]; then
        fail=$((fail+1)); echo "DIFF $prog $mode $extra"
      fi
    done
  done
done
echo "sweep: $n cases, $fail differences"
echo "sanitizer reports: $(cat a.err b.err | grep -cE 'AddressSanitizer|runtime error|LeakSanitizer')"
[ $fail = 0 ]
