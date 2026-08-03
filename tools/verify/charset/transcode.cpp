/**
 * @file: transcode.cpp
 * @date: 2026-08-03
 * @license: LicenseRef-AWH-1.0
 *
 * @brief Стенд сверки перекодировки текста с эталонной реализацией GNU libiconv.
 *        Сверяются оба направления перекодировки: разбор каждого байта каждой
 *        заданной кодировки и запись каждого кодового значения, кодировке
 *        представимого, обратно в её байт.
 *
 * @copyright: Copyright © 2026
 */

#include <cstdio>
#include <cstring>
#include <string>
#include <charset/charset.hpp>
#include <iconv/iconv.h>

using namespace std;
using namespace awh;

// Соответствие обозначений кодировок их именам в эталонной реализации
static const struct { charset::encoding_t encoding; const char * name; } ORACLE[] = {
	{charset::encoding_t::ISO8859_1,    "ISO-8859-1"},
	{charset::encoding_t::ISO8859_2,    "ISO-8859-2"},
	{charset::encoding_t::ISO8859_3,    "ISO-8859-3"},
	{charset::encoding_t::ISO8859_4,    "ISO-8859-4"},
	{charset::encoding_t::ISO8859_5,    "ISO-8859-5"},
	{charset::encoding_t::ISO8859_6,    "ISO-8859-6"},
	{charset::encoding_t::ISO8859_7,    "ISO-8859-7"},
	{charset::encoding_t::ISO8859_8,    "ISO-8859-8"},
	{charset::encoding_t::ISO8859_9,    "ISO-8859-9"},
	{charset::encoding_t::ISO8859_10,   "ISO-8859-10"},
	{charset::encoding_t::ISO8859_11,   "ISO-8859-11"},
	{charset::encoding_t::ISO8859_13,   "ISO-8859-13"},
	{charset::encoding_t::ISO8859_14,   "ISO-8859-14"},
	{charset::encoding_t::ISO8859_15,   "ISO-8859-15"},
	{charset::encoding_t::ISO8859_16,   "ISO-8859-16"},
	{charset::encoding_t::CP866,        "CP866"},
	{charset::encoding_t::CP874,        "CP874"},
	{charset::encoding_t::CP1250,       "CP1250"},
	{charset::encoding_t::CP1251,       "CP1251"},
	{charset::encoding_t::CP1252,       "CP1252"},
	{charset::encoding_t::CP1253,       "CP1253"},
	{charset::encoding_t::CP1254,       "CP1254"},
	{charset::encoding_t::CP1255,       "CP1255"},
	{charset::encoding_t::CP1256,       "CP1256"},
	{charset::encoding_t::CP1257,       "CP1257"},
	{charset::encoding_t::CP1258,       "CP1258"},
	{charset::encoding_t::KOI8_R,       "KOI8-R"},
	{charset::encoding_t::KOI8_U,       "KOI8-U"},
	{charset::encoding_t::MAC_ROMAN,    "MacRoman"},
	{charset::encoding_t::MAC_CYRILLIC, "MacCyrillic"}
};

// Перекодировка эталонной реализацией, признак отказа выводится ложью
static bool reference(const char * from, const char * to, const string & text, string & result){
	result.clear();
	iconv_t handle = ::iconv_open(to, from);
	if(handle == (iconv_t) -1) return false;
	char buffer[64];
	char * input = const_cast <char *> (text.data());
	size_t left = text.size();
	char * output = buffer;
	size_t space = sizeof(buffer);
	const size_t status = ::iconv(handle, &input, &left, &output, &space);
	// Кодировки с сочетающимися знаками задерживают символ до его завершения:
	// незавершённое состояние сбрасывается вызовом с пустым входным буфером
	const size_t flush = ((status == (size_t) -1) ? status : ::iconv(handle, nullptr, nullptr, &output, &space));
	::iconv_close(handle);
	if((status == (size_t) -1) || (flush == (size_t) -1) || (left != 0)) return false;
	result.assign(buffer, sizeof(buffer) - space);
	return true;
}

int main(){
	size_t checked = 0, diverged = 0;
	string ours, theirs;
	for(auto & item : ORACLE){
		// Сверяем разбор каждого байта кодировки
		for(uint32_t byte = 0; byte <= 0xFF; byte++){
			const string source(1, (char) byte);
			const bool right = reference(item.name, "UTF-8", source, theirs);
			const bool left = charset::transcode(source, item.encoding, charset::encoding_t::UTF8, ours);
			checked++;
			if((left != right) || (left && (ours != theirs))){
				diverged++;
				if(diverged <= 10)
					::printf("кодировка %s, байт 0x%02X: AWH=%s, libiconv=%s\n", item.name, byte,
						(left ? ours.c_str() : "отказ"), (right ? theirs.c_str() : "отказ"));
			}
		}
		// Сверяем запись каждого представимого кодового значения обратно в байт
		const charset::table_t * page = charset::table(item.encoding);
		for(size_t i = 0; i < page->count; i++){
			char buffer[awh::utf8::MAX_LENGTH];
			const size_t length = awh::utf8::encode(page->mappings[i].code, buffer);
			const string source(buffer, length);
			const bool right = reference("UTF-8", item.name, source, theirs);
			const bool left = charset::transcode(source, charset::encoding_t::UTF8, item.encoding, ours);
			checked++;
			if((left != right) || (left && (ours != theirs))){
				diverged++;
				if(diverged <= 10)
					::printf("кодировка %s, символ U+%04X: AWH=0x%02X, libiconv=%s\n", item.name,
						page->mappings[i].code, (left ? (uint8_t) ours[0] : 0),
						(right ? "иной" : "отказ"));
			}
		}
	}
	::printf("сличений перекодировки: %zu, расхождений: %zu\n", checked, diverged);
	return (diverged > 0);
}
