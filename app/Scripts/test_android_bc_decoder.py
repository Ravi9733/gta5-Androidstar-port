#!/usr/bin/env python3
"""Compile and exercise the Android BC texture decoder on a host compiler."""
import argparse
import pathlib
import subprocess
import tempfile

HARNESS = r"""
#include "BCTextureDecoder.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(void) {
    const int formats[] = {1,2,3,4,5,6,7};
    const size_t source_sizes[] = {8,16,16,8,16,16,16};
    const size_t output_sizes[] = {64,64,64,16,32,128,64};
    uint8_t block[16] = {0};
    for (size_t i=0; i<sizeof(formats)/sizeof(formats[0]); ++i) {
        size_t decoded_size=0;
        void *pixels=PGDecodeBC(formats[i],4,4,1,0,0,block,source_sizes[i],&decoded_size);
        if (!pixels || decoded_size != output_sizes[i]) {
            fprintf(stderr,"BC format %d failed (ptr=%p size=%zu expected=%zu)\\n",
                    formats[i],pixels,decoded_size,output_sizes[i]);
            free(pixels);
            return 1;
        }
        free(pixels);
    }
    size_t decoded_size=0;
    if (PGDecodeBC(1,4,4,1,0,0,block,7,&decoded_size) != NULL) return 2;
    if (PGDecodeBC(1,0,4,1,0,0,block,sizeof(block),&decoded_size) != NULL) return 3;
    if (PGDecodeBC(99,4,4,1,0,0,block,sizeof(block),&decoded_size) != NULL) return 4;
    puts("Android BC1-BC7 decoder smoke tests passed");
    return 0;
}
"""

def main():
    parser=argparse.ArgumentParser()
    parser.add_argument("--header",required=True)
    parser.add_argument("--source",required=True)
    args=parser.parse_args()
    source=pathlib.Path(args.source)
    with tempfile.TemporaryDirectory(prefix="pg-bc-test-") as tmp:
        tmp_path=pathlib.Path(tmp)
        harness=tmp_path/"bc_decoder_test.c"
        binary=tmp_path/"bc_decoder_test"
        harness.write_text(HARNESS)
        subprocess.run([
            "cc","-std=c11","-Wall","-Wextra","-Werror",
            "-DPG_ANDROID_BUILD=1",
            "-I",str(pathlib.Path(args.header).resolve().parent),
            "-I",str(source.resolve()),
            str(source/"BCTextureDecoder.c"),str(harness),
            "-o",str(binary)
        ],check=True)
        subprocess.run([str(binary)],check=True)

if __name__=="__main__":
    main()
