/**
 * @file normalize.cpp
 * @date 2026-08-03
 * @license{LicenseRef-AWH-1.0}
 *
 * @brief Стенд сверки нормализации текста с эталонной реализацией GNU libunistring.
 *        Сверяются все четыре нормальных представления для каждого кодового значения
 *        Юникода по отдельности и для пар «символ со знаком», на которых проверяются
 *        каноническое упорядочение и сочетание.
 *
 * @copyright Copyright © 2026
 */

#include <cstdio>
#include <cstring>
#include <vector>
#include <string>
#include <encoding/unicode/normalize.hpp>

extern "C" {
	struct unicode_normalization_form;
	typedef const struct unicode_normalization_form * uninorm_t;
	// Виды нормального представления заданы эталоном переменными, а не функциями
	extern const struct unicode_normalization_form uninorm_nfd;
	extern const struct unicode_normalization_form uninorm_nfc;
	extern uint32_t * u32_normalize(uninorm_t nf, const uint32_t * s, size_t n, uint32_t * resultbuf, size_t * lengthp);
	extern int uc_combining_class(uint32_t uc);
	extern int uc_canonical_decomposition(uint32_t uc, uint32_t * decomposition);
}

using namespace std;
using namespace awh;

// Соответствие видов нормального представления их эталонным обозначениям
static uninorm_t oracle(const unicode::form_t form){
	// Эталонная реализация в составе сборки задаёт лишь канонические представления:
	// представления совместимости сверяются отдельным стендом
	return ((form == unicode::form_t::NFD) ? &uninorm_nfd : &uninorm_nfc);
}

// Приведение текста эталонной реализацией
static bool reference(const unicode::form_t form, const vector <uint32_t> & text, vector <uint32_t> & result){
	size_t length = 0;
	uint32_t * output = u32_normalize(oracle(form), text.data(), text.size(), nullptr, &length);
	if(output == nullptr) return false;
	result.assign(output, output + length);
	::free(output);
	return true;
}

// Имя вида нормального представления
static const char * naming(const unicode::form_t form){
	switch((int) form){
		case (int) unicode::form_t::NFD:  return "NFD";
		case (int) unicode::form_t::NFC:  return "NFC";
	}
	return "NFKC";
}

/**
 * Признак того, что символ эталонному изданию стандарта неизвестен
 *
 * Издание стандарта эталонной реализации отстаёт от издания таблиц модуля, и
 * символы, добавленные позднее, эталон разбирает как неназначенные: их класс
 * сочетания нулевой, а разложения нет. Сверка на них бессмысленна.
 */
static bool unknown(const uint32_t code){
	uint32_t buffer[32];
	if((uc_combining_class(code) != 0) || (uc_canonical_decomposition(code, buffer) > 0)) return false;
	// Свой класс сочетания либо своё разложение при их отсутствии у эталона
	// означают, что символ добавлен изданием, эталону неизвестным
	vector <uint32_t> ours;
	awh::unicode::decompose(code, true, ours);
	return ((awh::unicode::combining(code) != 0) || (ours.size() != 1) || (ours.at(0) != code));
}

// Сверка приведения одного текста
static bool compare(const unicode::form_t form, const vector <uint32_t> & text, size_t & diverged){
	// Пропускаем тексты с символами, эталонному изданию стандарта неизвестными
	for(auto & code : text){
		if(unknown(code)) return true;
	}
	vector <uint32_t> ours, theirs;
	unicode::normalize(text, form, ours);
	if(!reference(form, text, theirs)) return true;
	if(ours == theirs) return true;
	diverged++;
	if(diverged <= 20){
		::printf("%s, текст", naming(form));
		for(auto & code : text) ::printf(" U+%04X", code);
		::printf(": AWH");
		for(auto & code : ours) ::printf(" U+%04X", code);
		::printf(", libunistring");
		for(auto & code : theirs) ::printf(" U+%04X", code);
		::printf("\n");
	}
	return false;
}

/**
 * Выгрузка приведения текстов набора сверки для стороннего эталона
 *
 * Эталонная реализация в составе сборки задаёт лишь канонические представления,
 * поэтому представления совместимости сверяются стендом normalize.py, которому
 * набор сверки выгружается этим режимом.
 */
static void dump(){
	const unicode::form_t forms[] = {
		unicode::form_t::NFD, unicode::form_t::NFC,
		unicode::form_t::NFKD, unicode::form_t::NFKC
	};
	vector <uint32_t> text, result;
	for(uint32_t code = 0; code <= 0x10FFFF; code++){
		if((code >= 0xD800) && (code <= 0xDFFF)) continue;
		text = {code};
		for(int32_t i = 0; i < 4; i++){
			unicode::normalize(text, forms[i], result);
			::printf("%d %X |", i, code);
			for(auto & value : result) ::printf(" %X", value);
			::printf("\n");
		}
	}
	const uint32_t marks[] = {
		0x0300, 0x0301, 0x0302, 0x0303, 0x0308, 0x030A, 0x0327, 0x0328,
		0x031B, 0x0323, 0x0334, 0x05B0, 0x093C, 0x0E38, 0x3099, 0x309A
	};
	for(uint32_t code = 0; code <= 0x2FFF; code++){
		if((code >= 0xD800) && (code <= 0xDFFF)) continue;
		for(auto & first : marks){
			for(auto & second : marks){
				text = {code, first, second};
				for(int32_t i = 0; i < 4; i++){
					unicode::normalize(text, forms[i], result);
					::printf("%d %X %X %X |", i, code, first, second);
					for(auto & value : result) ::printf(" %X", value);
					::printf("\n");
				}
			}
		}
	}
}

int main(int argc, char ** argv){
	// Выполняем выгрузку набора сверки для стороннего эталона
	if((argc > 1) && (::strcmp(argv[1], "--dump") == 0)){
		dump();
		return 0;
	}
	const unicode::form_t forms[] = {unicode::form_t::NFD, unicode::form_t::NFC};
	size_t checked = 0, diverged = 0;
	// Сверяем приведение каждого кодового значения Юникода по отдельности
	for(uint32_t code = 0; code <= 0x10FFFF; code++){
		if((code >= 0xD800) && (code <= 0xDFFF)) continue;
		const vector <uint32_t> text = {code};
		for(auto & form : forms){
			compare(form, text, diverged);
			checked++;
		}
	}
	// Сверяем приведение пар «символ со знаком»: на них проверяются каноническое
	// упорядочение знаков и последующее их сочетание с начальным символом
	const uint32_t marks[] = {
		0x0300, 0x0301, 0x0302, 0x0303, 0x0308, 0x030A, 0x0327, 0x0328,
		0x031B, 0x0323, 0x0334, 0x05B0, 0x093C, 0x0E38, 0x3099, 0x309A
	};
	for(uint32_t code = 0; code <= 0x2FFF; code++){
		if((code >= 0xD800) && (code <= 0xDFFF)) continue;
		for(auto & first : marks){
			for(auto & second : marks){
				const vector <uint32_t> text = {code, first, second};
				for(auto & form : forms){
					compare(form, text, diverged);
					checked++;
				}
			}
		}
	}
	// Сверяем приведение слогов хангыля вместе с их частями
	for(uint32_t code = 0xAC00; code < 0xD7A4; code++){
		const vector <uint32_t> text = {code, 0x0300};
		for(auto & form : forms){
			compare(form, text, diverged);
			checked++;
		}
	}
	::printf("сличений нормализации: %zu, расхождений: %zu\n", checked, diverged);
	return (diverged > 0);
}
