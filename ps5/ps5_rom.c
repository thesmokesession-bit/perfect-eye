#include <dirent.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/stat.h>

#ifndef PS5_TITLE_DIR
#define PS5_TITLE_DIR "/app0"
#endif

/* The game wants its ROM at data/pd.ntsc-final.z64 in big-endian (.z64) order. A player
 * drops their dump into the title folder under whatever name it came with, so on start-up
 * a Perfect Dark (USA) ROM found there or in data/ is copied into place in the right byte
 * order, once. A GoldenEye (USA) ROM goes to added-content/, where the game finds it by its
 * contents. The rest of the checking (the md5) is the game's own. */
#define ROM_SIZE (32u * 1024 * 1024)

/* 0: not an N64 ROM; 1: big-endian; 2: byteswapped (.v64); 3: little-endian (.n64) */
static int rom_order(const uint8_t *h)
{
	if (h[0] == 0x80 && h[1] == 0x37 && h[2] == 0x12 && h[3] == 0x40) return 1;
	if (h[0] == 0x37 && h[1] == 0x80 && h[2] == 0x40 && h[3] == 0x12) return 2;
	if (h[0] == 0x40 && h[1] == 0x12 && h[2] == 0x37 && h[3] == 0x80) return 3;
	return 0;
}

static void rom_to_big_endian(uint8_t *data, size_t len, int order)
{
	for (size_t i = 0; i + 3 < len; i += 4) {
		uint8_t a = data[i], b = data[i + 1], c = data[i + 2], d = data[i + 3];
		if (order == 2) {
			data[i] = b; data[i + 1] = a; data[i + 2] = d; data[i + 3] = c;
		} else if (order == 3) {
			data[i] = d; data[i + 1] = c; data[i + 2] = b; data[i + 3] = a;
		}
	}
}

static int has_rom_extension(const char *name)
{
	const char *dot = strrchr(name, '.');
	if (!dot) return 0;
	return !strcasecmp(dot, ".z64") || !strcasecmp(dot, ".n64") || !strcasecmp(dot, ".v64");
}

static int file_exists(const char *path)
{
	struct stat st;
	return stat(path, &st) == 0;
}

/* Reads the header of path and gives its game code (e.g. "NPDE") and order; 0 if not a ROM. */
static int rom_identify(const char *path, char code[5])
{
	uint8_t h[0x40];
	FILE *f = fopen(path, "rb");
	if (!f) return 0;
	size_t got = fread(h, 1, sizeof(h), f);
	fclose(f);
	if (got != sizeof(h)) return 0;
	int order = rom_order(h);
	if (!order) return 0;
	rom_to_big_endian(h, sizeof(h), order);
	memcpy(code, h + 0x3B, 4);
	code[4] = '\0';
	return order;
}

void ps5Notify(const char *fmt, ...);

static void import_pd_rom(const char *src, int order)
{
	ps5Notify("Perfect Dark: first start, setting up your ROM. This takes a moment.");
	const char *dst = PS5_TITLE_DIR "/data/pd.ntsc-final.z64";
	uint8_t *data = malloc(ROM_SIZE);
	FILE *f = data ? fopen(src, "rb") : NULL;
	size_t got = f ? fread(data, 1, ROM_SIZE, f) : 0;
	if (f) fclose(f);
	if (got != ROM_SIZE) {
		printf("PS5: %s is not a whole Perfect Dark ROM (%zu bytes), not imported\n", src, got);
		free(data);
		return;
	}
	rom_to_big_endian(data, ROM_SIZE, order);
	mkdir(PS5_TITLE_DIR "/data", 0777);
	FILE *out = fopen(dst, "wb");
	size_t put = out ? fwrite(data, 1, ROM_SIZE, out) : 0;
	if (out) fclose(out);
	free(data);
	if (put != ROM_SIZE) {
		printf("PS5: could not write %s\n", dst);
		remove(dst);
		return;
	}
	printf("PS5: imported %s as %s\n", src, dst);
}

static void import_roms_in(const char *dir)
{
	DIR *d = opendir(dir);
	if (!d) return;
	struct dirent *e;
	while ((e = readdir(d)) != NULL) {
		if (e->d_name[0] == '.' || !has_rom_extension(e->d_name)) continue;
		if (!strcmp(e->d_name, "pd.ntsc-final.z64")) continue;
		char path[512], code[5];
		snprintf(path, sizeof(path), "%s/%s", dir, e->d_name);
		int order = rom_identify(path, code);
		if (!order) continue;
		if (!strcmp(code, "NPDE") && !file_exists(PS5_TITLE_DIR "/data/pd.ntsc-final.z64")) {
			import_pd_rom(path, order);
		} else if (!strcmp(code, "NGEE") && strcmp(dir, PS5_TITLE_DIR "/added-content")) {
			char to[512];
			mkdir(PS5_TITLE_DIR "/added-content", 0777);
			snprintf(to, sizeof(to), PS5_TITLE_DIR "/added-content/%s", e->d_name);
			if (!file_exists(to) && rename(path, to) == 0) {
				printf("PS5: moved the GoldenEye ROM %s to added-content/\n", e->d_name);
			}
		}
	}
	closedir(d);
}

void ps5ImportRoms(void)
{
	import_roms_in(PS5_TITLE_DIR);
	import_roms_in(PS5_TITLE_DIR "/data");
}
