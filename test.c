/*
 * Copyright (c) 2025 Jiapeng Li <mail@jiapeng.me>
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <time.h>

#include "zjpgd.h"

typedef struct {
    uint16_t x;
    uint16_t y;
    zjd_ctx_t snapshot;
} image_roi_t;

typedef struct {
    struct {
        uint8_t *data;  // Pointer to JPEG data
        size_t size;         // Size of JPEG data
        size_t offset;       // Current offset in the data
    } ifile;
    struct {
        uint8_t *data;  // Pointer to JPEG data
        size_t size;         // Size of JPEG data
        size_t offset;       // Current offset in the data
        size_t pixels;
    } ofile;
    image_roi_t rois[3];
} image_t;

int load_jpeg(const char *filename, image_t *img) {
    FILE *file = fopen(filename, "rb");
    if (!file) {
        perror("Failed to open file");
        return -1;
    }

    fseek(file, 0, SEEK_END);
    img->ifile.size = ftell(file);
    fseek(file, 0, SEEK_SET);

    img->ifile.data = (uint8_t *)malloc(img->ifile.size);
    if (!img->ifile.data) {
        perror("Failed to allocate memory");
        fclose(file);
        return -1;
    }

    fread((void *)img->ifile.data, 1, img->ifile.size, file);
    fclose(file);
    img->ifile.offset = 0;

    img->ofile.pixels = 0;
    img->ofile.size = 4096 * 4096 * 3;
    img->ofile.data = (uint8_t *)malloc(img->ofile.size);
    if (!img->ofile.data) {
        perror("Failed to allocate memory for output");
        free(img->ifile.data);
        return -1;
    }


    memset(img->ofile.data, 0, img->ofile.size);

    return 0;
}

int save_bmp(const char *filename, image_t *img, int width, int height) {
    FILE *file = fopen(filename, "wb");
    if (!file) {
        perror("Failed to open file for writing");
        return -1;
    }

    // BMP Header
    uint8_t bmp_header[54] = {
        0x42, 0x4D,             // Signature 'BM'
        0, 0, 0, 0,             // File size in bytes
        0, 0,                   // Reserved
        0, 0,                   // Reserved
        54, 0, 0, 0,            // Offset to pixel data
        40, 0, 0, 0,            // Info header size
        0, 0, 0, 0,             // Width
        0, 0, 0, 0,             // Height
        1, 0,                   // Planes
        24, 0,                  // Bits per pixel
        0, 0, 0, 0,             // Compression (none)
        0, 0, 0, 0,             // Image size (can be zero for uncompressed)
        0x13, 0x0B, 0, 0,       // Horizontal resolution (pixels per meter)
        0x13, 0x0B, 0, 0,       // Vertical resolution (pixels per meter)
        0, 0, 0, 0,             // Number of colors in palette (none)
        0, 0, 0, 0              // Important colors (all)
    };

    int row_size = (width * 3 + 3) & ~3; // Row size aligned to multiple of 4 bytes
    int pixel_data_size = row_size * height;
    int file_size = pixel_data_size + sizeof(bmp_header);

    // Fill in file size
    bmp_header[2] = (uint8_t)(file_size & 0xFF);
    bmp_header[3] = (uint8_t)((file_size >> 8) & 0xFF);
    bmp_header[4] = (uint8_t)((file_size >> 16) & 0xFF);
    bmp_header[5] = (uint8_t)((file_size >> 24) & 0xFF);
    // Fill in width
    bmp_header[18] = (uint8_t)(width & 0xFF);
    bmp_header[19] = (uint8_t)((width >> 8) & 0xFF);
    bmp_header[20] = (uint8_t)((width >> 16) & 0xFF);
    bmp_header[21] = (uint8_t)((width >> 24) & 0xFF);
    // Fill in height
    bmp_header[22] = (uint8_t)(height & 0xFF);
    bmp_header[23] = (uint8_t)((height >> 8) & 0xFF);
    bmp_header[24] = (uint8_t)((height >> 16) & 0xFF);
    bmp_header[25] = (uint8_t)((height >> 24) & 0xFF);
    // Fill in image size
    bmp_header[34] = (uint8_t)(pixel_data_size & 0xFF);

    fwrite(bmp_header, sizeof(bmp_header), 1, file);
    // Write pixel data (BMP stores pixels in BGR format and bottom-up)
    for (int y = height - 1; y >= 0; y--) {
        for (int x = 0; x < width; x++) {
            int index = (x + y * width) * 3;
            uint8_t bgr[3] = {
                img->ofile.data[index + 2], // Blue
                img->ofile.data[index + 1], // Green
                img->ofile.data[index + 0]  // Red
            };
            fwrite(bgr, sizeof(bgr), 1, file);
        }
        // Padding for 4-byte alignment
        uint8_t padding[3] = {0, 0, 0};
        fwrite(padding, row_size - width * 3, 1, file);
    }
    fclose(file);
    return 0;
}

int zjd_ifunc(zjd_t *zjd, uint8_t *buf, uint32_t addr, int len)
{
    image_t *img = (image_t *)zjd->arg;

    size_t remaining = img->ifile.size - addr;

    if (len > remaining) {
        len = remaining;
    }

    if (buf) {
        memcpy(buf, img->ifile.data + addr, len);
    }

    return len;
}

int zjd_ofunc(zjd_t *zjd, zjd_rect_t *rect, void *pixels)
{
    image_t *img = (image_t *)zjd->arg;
    uint8_t *pix = (uint8_t *)pixels;
    int x, y, index;

    // printf("Decoded rect: (%d,%d)-(%d,%d)\n", rect->x, rect->y, rect->w, rect->h);
    for (y = rect->y; y < rect->y + rect->h; y++) {
        for (x = rect->x; x < rect->x + rect->w; x++) {
            if (x >= zjd->width || y >= zjd->height) {
                // Out of bounds, skip
                pix += 3;
                continue;
            }
            index = x + y * zjd->width;
            img->ofile.data[index * 3 + 0] = *pix++;
            img->ofile.data[index * 3 + 1] = *pix++;
            img->ofile.data[index * 3 + 2] = *pix++;

            img->ofile.pixels++;
        }
    }

    return 1;
}

int zjd_test(zjd_t *zjd, void *work, size_t worksize, image_t *img)
{
    zjd_res_t res;

    res = zjd_init(
        zjd,
        &(zjd_cfg_t){
            .outfmt = ZJD_RGB888,
            .ifunc = zjd_ifunc,
            .ofunc = zjd_ofunc,
            .buf = work,
            .buflen = worksize,
            .arg = (void *)img
        }
    );
    if (res != ZJD_OK) {
        printf("Failed to initialize zjpgd %d\n", res);
        free(img->ifile.data);
        free(img->ofile.data);
        return 1;
    }
    res = zjd_scan(zjd, NULL, NULL);
    if (res != ZJD_OK) {
        printf("Failed to decode JPEG image %d\n", res);
        free(img->ifile.data);
        free(img->ofile.data);
        return 1;
    }

    return 0;
}

uint32_t micros() {
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);  // or CLOCK_MONOTONIC for relative time
    return (uint32_t)((long long)ts.tv_sec * 1000000LL + ts.tv_nsec / 1000);
}

int main(int argc, char **argv)
{
    image_t img;
    int ret, i, j, rounds;
    uint32_t us;

    uint8_t work[4096];

    zjd_t zjd;
    zjd_res_t zjd_res;
    uint32_t zjd_cost_us;
    uint32_t zjd_roi_cost_us;
    uint32_t zjd_roi_1_4_cost_us[3];
    zjd_rect_t roi_rect[3];

    if (argc < 2) {
        printf("Usage: %s <jpg_file>\n", argv[0]);
        return 1;
    }

    ret = load_jpeg(argv[1], &img);
    if (ret != 0) {
        printf("Failed to load JPEG file\n");
        return 1;
    }

    if (argc > 2) {
        rounds = atoi(argv[2]);
        if (rounds < 1) rounds = 1;
    } else {
        rounds = 1;
    }
    printf("\nDecoding %s for %d rounds\n", argv[1], rounds);

    us = micros();
    for (i = 0; i < rounds; i++) {
        img.ifile.offset = 0;
        img.ofile.offset = 0;
        img.ofile.pixels = 0;
        memset(img.ofile.data, 0, img.ofile.size);
        ret = zjd_test(&zjd, work, sizeof(work), &img);
        if (ret != 0) {
            printf("zjd_test failed with error code %d\n", ret);
            free(img.ifile.data);
            free(img.ofile.data);
            return 1;
        }
    }
    zjd_cost_us = (micros() - us) / rounds;
    printf("zjpgd %ux%u decode time: %u us, %u pixels processed\n", zjd.width, zjd.height, zjd_cost_us, (uint32_t)img.ofile.pixels);
    save_bmp("output_zjpgd.bmp", &img, zjd.width, zjd.height);

    free(img.ifile.data);
    free(img.ofile.data);

    return 0;
}
