/**
 * @file density.c
 * @date 2026-10-02
 * @license{LicenseRef-AWH-1.0}
 * @author Yuriy Lobarev
 * @brief Проверка выравнивания и совместимости потока Density
 * @copyright Copyright © 2026
 */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "density_api.h"

int main(int argc, char **argv){
	const size_t sizes[] = {1, 3, 4, 7, 8, 31, 32, 63, 64, 255, 256, 1024, 16384, 1048576};
	if(argc != 3)
		return 2;
	const int only = atoi(argv[2]);
	FILE *wire = fopen(argv[1], "wb");
	if(wire == NULL)
		return 3;
	size_t runs = 0;
	for(int level = 1; level <= 3; ++level){
		if(only != 0 && level != only)
			continue;
		for(size_t index = 0; index < sizeof(sizes) / sizeof(sizes[0]); ++index){
			const size_t size = sizes[index];
			const size_t capacity = density_compress_safe_size(size);
			const size_t restored_capacity = density_decompress_safe_size(size);
			uint8_t *input = malloc(size + 8);
			uint8_t *output = malloc(capacity + 8);
			uint8_t *restored = malloc(restored_capacity + 8);
			uint8_t *reference = malloc(capacity);
			if(input == NULL || output == NULL || restored == NULL || reference == NULL)
				return 4;
			for(int pattern = 0; pattern < 3; ++pattern){
				size_t reference_size = 0;
				for(size_t offset = 0; offset < 8; ++offset){
					uint32_t random = 0x12345678;
					for(size_t byte = 0; byte < size; ++byte){
						random ^= random << 13;
						random ^= random >> 17;
						random ^= random << 5;
						input[offset + byte] = (pattern == 0 ? 'a' : (pattern == 1 ? byte % 17 : random & 0xFF));
					}
					const density_processing_result encoded = density_compress(input + offset, size, output + offset, capacity, (DENSITY_ALGORITHM) level);
					if(encoded.state != DENSITY_STATE_OK || encoded.bytesRead != size || encoded.bytesWritten > capacity)
						return 5;
					if(offset == 0){
						reference_size = encoded.bytesWritten;
						memcpy(reference, output, reference_size);
						const uint64_t length = reference_size;
						if(fwrite(&length, sizeof(length), 1, wire) != 1 || fwrite(reference, 1, reference_size, wire) != reference_size)
							return 6;
					} else if(encoded.bytesWritten != reference_size || memcmp(reference, output + offset, reference_size) != 0)
						return 7;
					const density_processing_result decoded = density_decompress(output + offset, encoded.bytesWritten, restored + offset, restored_capacity);
					if(decoded.state != DENSITY_STATE_OK || decoded.bytesRead > encoded.bytesWritten || decoded.bytesWritten != size || memcmp(input + offset, restored + offset, size) != 0){
						fprintf(stderr, "level=%d size=%zu pattern=%d offset=%zu state=%u read=%llu/%llu written=%llu\n", level, size, pattern, offset, decoded.state, (unsigned long long) decoded.bytesRead, (unsigned long long) encoded.bytesWritten, (unsigned long long) decoded.bytesWritten);
						return 8;
					}
					++runs;
				}
			}
			free(reference);
			free(restored);
			free(output);
			free(input);
		}
	}
	if(fclose(wire) != 0)
		return 9;
	printf("RESULT runs=%zu\n", runs);
	return 0;
}
