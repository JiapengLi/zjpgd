


# build test executable
```bash
gcc -DZJD_DEBUG=1 -I zjpgd test.c  zjpgd/zjpgd.c -o zjdtest_debug
gcc -DZJD_DEBUG=0 -I zjpgd test.c  zjpgd/zjpgd.c -o zjdtest
```

