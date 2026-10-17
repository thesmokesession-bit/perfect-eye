/* Host check of ps5/ps5_rom.c: a byteswapped (.v64) Perfect Dark ROM dropped in the title
 * folder comes out as data/pd.ntsc-final.z64 in big-endian order, and a GoldenEye ROM is
 * moved to added-content/.  cc -DPS5_TITLE_DIR='"/tmp/x"' tests/test_ps5_rom.c ps5/ps5_rom.c */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

void ps5ImportRoms(void);

static uint8_t *make_rom(const char *code, size_t size)
{
	uint8_t *r = malloc(size);
	for (size_t i = 0; i < size; i++) r[i] = (uint8_t)(i * 7 + 3);
	r[0] = 0x80; r[1] = 0x37; r[2] = 0x12; r[3] = 0x40;
	memcpy(r + 0x3B, code, 4);
	return r;
}

static void write_swapped(const char *path, const uint8_t *be, size_t size)
{
	FILE *f = fopen(path, "wb");
	for (size_t i = 0; i < size; i += 2) { fputc(be[i + 1], f); fputc(be[i], f); }
	fclose(f);
}

int main(void)
{
	const size_t size = 32u * 1024 * 1024;
	mkdir(PS5_TITLE_DIR, 0777);
	uint8_t *pd = make_rom("NPDE", size);
	write_swapped(PS5_TITLE_DIR "/Perfect Dark (USA) (Rev 1).v64", pd, size);
	uint8_t *ge = make_rom("NGEE", 4096);
	FILE *f = fopen(PS5_TITLE_DIR "/GoldenEye 007 (USA).z64", "wb");
	fwrite(ge, 1, 4096, f);
	fclose(f);

	ps5ImportRoms();

	uint8_t *out = malloc(size);
	f = fopen(PS5_TITLE_DIR "/data/pd.ntsc-final.z64", "rb");
	assert(f && fread(out, 1, size, f) == size);
	fclose(f);
	assert(memcmp(out, pd, size) == 0);
	struct stat st;
	assert(stat(PS5_TITLE_DIR "/added-content/GoldenEye 007 (USA).z64", &st) == 0);
	assert(stat(PS5_TITLE_DIR "/GoldenEye 007 (USA).z64", &st) != 0);
	puts("test_ps5_rom: ok");
	return 0;
}
