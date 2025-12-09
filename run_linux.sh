#!/bin/sh

cc -o test_original $@
cc -o test -D MALLOC_LOGGING -I include $@ -L . -lft_malloc

echo "Original"
/usr/bin/time -v ./test_original
echo "\nMine"
export LD_LIBRARY_PATH=.
export LD_PRELOAD=libft_malloc.so
/usr/bin/time -v ./test
echo "\nDone Cleaning up..."
# rm test test_original
