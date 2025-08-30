/*
 * Copyright (c) 2025 Jiapeng Li <mail@jiapeng.me>
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <stdio.h>

#include "zjpgd.h"

static zjd_outfmt_t outfmt = ZJD_RGB888;
static zjd_ctx_t snapshot[16];

int zjd_ifunc(zjd_t *zjd, uint8_t *buf, uint32_t addr, int len)
{
    FILE *fp = (FILE *)zjd->arg;

    ZJD_LOG("rd %d %d", addr, len);

    fseek(fp, addr, SEEK_SET);
    if (buf) {
        return fread(buf, 1, (size_t)len, fp);
    } else {
        return len;
    }

    return len;
}

int zjd_ofunc(zjd_t *zjd, zjd_rect_t *rect, void *pixels)
{
    /* take snapshot for a specified MCU */
    if (rect->x == 0 && rect->y == 0) {
        zjd_save(zjd, &snapshot[0]);
    }

#if ZJD_DEBUG
    ZJD_LOG("Decoded rect: (%d,%d)-(%d,%d)", rect->x, rect->y, rect->w, rect->h);
#else
    // Output the decoded bitmap data
    uint8_t *pix = (uint8_t *)pixels;

    int x, y, l;

    switch (outfmt) {
    case ZJD_GRAYSCALE:
    default:
        // Handle grayscale output
        l = 1;
        break;
    case ZJD_RGB565:
        // Handle RGB565 output
        l = 2;
        break;
    case ZJD_BGR565:
        // Handle BGR565 output
        l = 2;
        break;
    case ZJD_RGB888:
        // Handle RGB888 output
        l = 3;
        break;
    case ZJD_BGR888:
        // Handle BGR888 output
        l = 3;
        break;
    case ZJD_RGBA8888:
        // Handle RGBA8888 output
        l = 4;
        break;
    case ZJD_BGRA8888:
        // Handle BGRA8888 output
        l = 4;
        break;
    }
    printf("(%d,%d)-(%d,%d)\n", rect->x, rect->y, rect->w, rect->h);
    for (y = rect->y; y < rect->y + rect->h; y++) {
        for (x = rect->x; x < rect->x + rect->w; x++) {
            if (l == 2) {
                uint16_t v = *(uint16_t *)pix;
                int r = (v >> 11) & 0x1F;
                int g = (v >> 5) & 0x3F;
                int b = v & 0x1F;
                r = (r << 3) | (r >> 2);
                g = (g << 2) | (g >> 4);
                b = (b << 3) | (b >> 2);
                printf("(%3d,%3d,%3d) ", r, g, b);
                pix += 2;
            } else {
                int n = l;
                printf("(");
                while (n-- > 1) {
                    printf("%3d,", *pix++);
                }
                printf("%3d", *pix++);
                printf(") ");
            }
        }
        printf("\n");
    }
#endif

    return 1;
}

int main(int argc, char **argv)
{
    zjd_t zjd;
    zjd_cfg_t cfg;
    zjd_rect_t *roi_rect = NULL, _rect;
    zjd_res_t res;

    if (argc < 2) {
        printf("Usage: %s <jpg_file>, \n", argv[0]);
        return 1;
    }

    FILE *fp = fopen(argv[1], "rb");
    if (!fp) {
        perror("Failed to open JPEG file");
        return 1;
    }

#if ZJD_HUFFMAN_OPT == 2
    uint8_t work[1024 * 5]; // Work buffer
#else
    uint8_t work[1024 * 3]; // Work buffer
#endif

    if (argc > 3) {
        int x, y, w, h;
        int ret = sscanf(argv[3], "%d,%d,%d,%d", &x, &y, &w, &h);
        if (ret == 4) {
            _rect.x = x;
            _rect.y = y;
            _rect.w = w;
            _rect.h = h;
            roi_rect = &_rect;
        } else {
            fprintf(stderr, "Invalid rectangle format: %s\n", argv[3]);
            fclose(fp);
            return 1;
        }
    }

    if (argc > 2) {
        if (strcmp(argv[2], "grayscale") == 0) {
            outfmt = ZJD_GRAYSCALE;
        } else if (strcmp(argv[2], "rgb565") == 0) {
            outfmt = ZJD_RGB565;
        } else if (strcmp(argv[2], "bgr565") == 0) {
            outfmt = ZJD_BGR565;
        } else if (strcmp(argv[2], "rgb888") == 0) {
            outfmt = ZJD_RGB888;
        } else if (strcmp(argv[2], "bgr888") == 0) {
            outfmt = ZJD_BGR888;
        } else if (strcmp(argv[2], "rgba8888") == 0) {
            outfmt = ZJD_RGBA8888;
        } else if (strcmp(argv[2], "bgra8888") == 0) {
            outfmt = ZJD_BGRA8888;
        } else {
            fprintf(stderr, "Unknown outfmt format: %s\n", argv[2]);
            fclose(fp);
            return 1;
        }
    } else {
        outfmt = ZJD_RGB888; // Default outfmt format
    }

    printf("Preparing JPEG decoder...\n");
    cfg.outfmt = outfmt;
    cfg.ifunc = zjd_ifunc;  // User-defined input function
    cfg.ofunc = zjd_ofunc;  // User-defined output function
    cfg.buf = work;        // Work buffer
    cfg.buflen = sizeof(work);
    cfg.arg = (void *)fp;
    res = zjd_init(&zjd, &cfg);
    if (res != ZJD_OK) {
        printf("Failed to prepare JPEG decoder %d\n", res);
        fclose(fp);
        return 1;
    }

    printf("\n\nStarting FULL JPEG decompression\n");
    res = zjd_scan(&zjd, NULL, NULL);
    if (res != ZJD_OK) {
        printf("Failed to start JPEG FULL decompression %d\n", res);
        fclose(fp);
        return 1;
    }

    printf("\n\nStarting ROI JPEG decompression \n");
    res = zjd_scan(&zjd, NULL, roi_rect);
    if (res != ZJD_OK) {
        printf("Failed to start JPEG ROI decompression %d\n", res);
        fclose(fp);
        return 1;
    }

    // /* Use snapshot to start decompression */
    // res = zjd_scan(&zjd, &snapshot[0], NULL);
    // if (res != ZJD_OK) {
    //     printf("Decompress with snapshot failed %d\n", res);
    //     fclose(fp);
    //     return 1;
    // }

    fclose(fp);

    printf("\n\n\n");

    printf("pool: %zd\n", sizeof(work));
    printf("sizeof(zjd_t): %zu\n", sizeof(zjd_t));
    printf("allocated: %zd\n", sizeof(work) - zjd.buflen);
    printf("cache: %zd\n", zjd.buflen);
    printf("%s minimum cost (with 4 bytes cache): %zd\n", argv[1], sizeof(zjd_t) + sizeof(work) - zjd.buflen + 4);

    return 0;
}


