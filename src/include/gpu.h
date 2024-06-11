/*
 * (C) 2024 Rhys Baker, spicyjpeg
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH
 * REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT,
 * INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
 * LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR
 * OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */

#pragma once

#include <stdint.h>
#include "ps1/gpucmd.h"

#define DMA_MAX_CHUNK_SIZE 16
#define CHAIN_BUFFER_SIZE 32768
#define ORDERING_TABLE_SIZE 720

typedef struct{
    uint32_t data[CHAIN_BUFFER_SIZE];
    uint32_t orderingTable[ORDERING_TABLE_SIZE];
    uint32_t *nextPacket;
} DMAChain;

typedef struct {
    uint8_t u, v;
    uint16_t w, h;
    uint16_t page, clut;
} TextureInfo;

#ifdef __cplusplus
extern "C" {
#endif

void setupGPU(GP1VideoMode mode, int width, int height);
void waitForGP0Ready(void);
void waitForDMADone(void);
void waitForVSync(void);

void sendLinkedList(const void *data);
void sendVRAMData(const void *data, int x, int y, int w, int h);
void clearOrderingTable(uint32_t *table, int numEntries);
uint32_t *allocatePacket(DMAChain *chain, int zIndex, int numCommands);

void uploadTexture(
    TextureInfo *info, const void *data, int x, int y, int w, int h
);
void uploadIndexedTexture(
    TextureInfo *info, const void *image, int x, int y, int w, int h,
    const void *palette, int paletteX, int paletteY, GP0ColorDepth colorDepth
);

#ifdef __cplusplus
}
#endif