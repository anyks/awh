/**
 * @file: conformance.cpp
 * @date: 2026-08-03
 * @license: LicenseRef-AWH-1.0
 *
 * @brief Стенд сверки приведения доменных имён с набором соответствия приложению
 *        по обработке доменных имён стандарта Юникода. Набор поставляется составом
 *        подмодуля эталонной реализации файлом «tests/IdnaTest.txt» и задаёт
 *        ожидаемый исход приведения в обе стороны вместе с кодами ошибок.
 *
 * @copyright: Copyright © 2026
 */

#include <cstdio>
#include <cstring>
#include <string>
#include <vector>
#include <fstream>
#include <encoding/idna/idna.hpp>
#include <encoding/unicode/utf8.hpp>

using namespace std;
using namespace awh;

// Набор режимов приведения, заданный набором соответствия
static const uint16_t MODE = (
	(uint16_t) idna::option_t::HYPHENS | (uint16_t) idna::option_t::BIDI |
	(uint16_t) idna::option_t::JOINERS | (uint16_t) idna::option_t::LENGTH |
	(uint16_t) idna::option_t::STD3
);

// Снятие пробельных символов с краёв записи
static string trim(const string & text){
	size_t begin = 0, end = text.size();
	// Набор записан с возвратом каретки: он снимается наравне с пробельными символами
	const char * spaces = " \t\r\n";
	while((begin < end) && (::strchr(spaces, text[begin]) != nullptr)) begin++;
	while((end > begin) && (::strchr(spaces, text[end - 1]) != nullptr)) end--;
	return text.substr(begin, end - begin);
}

// Раскрытие записей символов вида «\uXXXX» и «\x{XXXX}»
static string unescape(const string & text){
	string result;
	char buffer[utf8::MAX_LENGTH];
	for(size_t i = 0; i < text.size();){
		if((text[i] == '\\') && ((i + 1) < text.size()) && (text[i + 1] == 'u') && ((i + 5) < text.size())){
			const uint32_t code = (uint32_t) strtoul(text.substr(i + 2, 4).c_str(), nullptr, 16);
			const size_t length = utf8::encode(code, buffer);
			result.append(buffer, length);
			i += 6;
			continue;
		}
		if((text[i] == '\\') && ((i + 2) < text.size()) && (text[i + 1] == 'x') && (text[i + 2] == '{')){
			const size_t close = text.find('}', i);
			if(close != string::npos){
				const uint32_t code = (uint32_t) strtoul(text.substr(i + 3, close - i - 3).c_str(), nullptr, 16);
				const size_t length = utf8::encode(code, buffer);
				result.append(buffer, length);
				i = (close + 1);
				continue;
			}
		}
		result.append(1, text[i++]);
	}
	return result;
}

// Признак того, что ожидаемым исходом является отказ
static bool failing(const string & text){
	return (!text.empty() && (text.front() == '['));
}

int main(int argc, char ** argv){
	const char * path = ((argc > 1) ? argv[1] : "submodules/libidn2/tests/IdnaTest.txt");
	ifstream file(path);
	if(!file.is_open()){
		::printf("набор соответствия недоступен: %s\n", path);
		return 1;
	}
	size_t checked = 0, diverged = 0;
	string line;
	while(getline(file, line)){
		const size_t hash = line.find('#');
		if(hash != string::npos) line = line.substr(0, hash);
		line = trim(line);
		if(line.empty()) continue;
		// Разбираем строку набора на столбцы
		vector <string> fields;
		size_t begin = 0;
		for(size_t i = 0; i <= line.size(); i++){
			if((i == line.size()) || (line[i] == ';')){
				fields.push_back(trim(line.substr(begin, i - begin)));
				begin = (i + 1);
			}
		}
		if(fields.size() < 4) continue;
		const string kind = fields.at(0);
		const string source = unescape(fields.at(1));
		// Пустой столбец означает совпадение с исходной записью
		const string wanted = (fields.at(2).empty() ? source : unescape(fields.at(2)));
		const string ascii = (fields.at(3).empty() ? wanted : unescape(fields.at(3)));
		// Сверяем приведение к записи Юникода
		{
			string result; idna::error_t error = idna::error_t::NONE;
			const bool ok = idna::toUnicode(source, result, error, MODE);
			checked++;
			const bool expected = !failing(fields.at(2));
			if((ok != expected) || (expected && (result != wanted))){
				diverged++;
				if(diverged <= 15)
					::printf("toUnicode «%s»: AWH «%s» (%s), ожидалось «%s»\n",
						fields.at(1).c_str(), result.c_str(),
						(ok ? "принято" : string(idna::message(error)).c_str()),
						fields.at(2).c_str());
			}
		}
		// Сверяем приведение к записи из символов набора ASCII
		for(int32_t pass = 0; pass < 2; pass++){
			const bool transitional = (pass == 1);
			if(transitional && (kind == "N")) continue;
			if(!transitional && (kind == "T")) continue;
			const uint16_t mode = (MODE | (transitional ? (uint16_t) idna::option_t::TRANSITIONAL : 0));
			string result; idna::error_t error = idna::error_t::NONE;
			const bool ok = idna::toAscii(source, result, error, mode);
			checked++;
			const bool expected = !failing(ascii);
			if((ok != expected) || (expected && (result != ascii))){
				diverged++;
				if(diverged <= 15)
					::printf("toAscii(%s) «%s»: AWH «%s» (%s), ожидалось «%s»\n",
						(transitional ? "T" : "N"), fields.at(1).c_str(), result.c_str(),
						(ok ? "принято" : string(idna::message(error)).c_str()),
						ascii.c_str());
			}
		}
	}
	::printf("сличений соответствия: %zu, расхождений: %zu\n", checked, diverged);
	return (diverged > 0);
}
