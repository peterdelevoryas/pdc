#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <errno.h>
#include <llvm-c/Core.h>
#include <llvm-c/TargetMachine.h>
#include <llvm-c/Analysis.h>

fn read_file(path: *const char) -> *char:
    var file = fopen(path, "rb")
    if !file:
        printf("unable to open '%s': %s\n", path, strerror(errno))
        return NULL

    fseek(file, 0, SEEK_END)
    var size = ftell(file)
    fseek(file, 0, SEEK_SET)

    var text = (char*)malloc(size + 1)
    assert(text)
    assert(fread(text, size, 1, file) == 1)
    text[size] = 0
    fclose(file)
    return text

fn main(argc: int, argv: **char) -> int:
    return 0
