/**
 * @file joining.cpp
 * @date 2026-08-03
 * @license{LicenseRef-AWH-1.0}
 *
 * @brief Выгрузка свойства вида соединения символов из состава эталонной реализации
 *        GNU libunistring в файл «tools/encoding/idna/joining.txt». Свойство требуется правилами
 *        сочетания CONTEXTJ приведения доменных имён, а отдельным файлом базы данных
 *        символов Юникода в составе подмодулей не поставляется.
 *
 * @copyright Copyright © 2026
 */

#include <cstdio>
#include <cstdint>

extern "C" {
	extern int uc_joining_type(uint32_t uc);
}

int main(){
	// Виды соединения символов в порядке их обозначений эталонной реализацией
	static const char * names[] = {"U", "T", "C", "L", "R", "D"};
	int32_t previous = -2;
	uint32_t begin = 0;
	::printf("# Свойство вида соединения символов\n");
	::printf("# Файл выгружен средством «tools/encoding/idna/joining.cpp» из состава эталонной\n");
	::printf("# реализации GNU libunistring. Правки, внесённые вручную, теряются.\n");
	for(uint32_t code = 0; code <= 0x110000; code++){
		const int32_t value = ((code > 0x10FFFF) ? -1 : uc_joining_type(code));
		if(value == previous) continue;
		if((previous > 0) && (previous < 6))
			::printf("%04X..%04X ; %s\n", begin, code - 1, names[previous]);
		previous = value;
		begin = code;
	}
	return 0;
}
