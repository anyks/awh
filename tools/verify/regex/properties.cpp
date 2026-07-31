// Исчерпывающее сличение свойств Юникода с эталоном по всем кодовым значениям
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>
#include <fstream>
#include <regex/unicode.hpp>
#define PCRE2_CODE_UNIT_WIDTH 8
#include <pcre2.h>
using namespace std;
using namespace awh;
// Кодирование кодового значения последовательностью UTF-8
static size_t encode(uint32_t code, char * out){
	if(code < 0x80){ out[0] = (char) code; return 1; }
	if(code < 0x800){ out[0] = (char)(0xC0 | (code >> 6)); out[1] = (char)(0x80 | (code & 0x3F)); return 2; }
	if(code < 0x10000){ out[0] = (char)(0xE0 | (code >> 12)); out[1] = (char)(0x80 | ((code >> 6) & 0x3F)); out[2] = (char)(0x80 | (code & 0x3F)); return 3; }
	out[0] = (char)(0xF0 | (code >> 18)); out[1] = (char)(0x80 | ((code >> 12) & 0x3F));
	out[2] = (char)(0x80 | ((code >> 6) & 0x3F)); out[3] = (char)(0x80 | (code & 0x3F)); return 4;
}
int main(int argc, char ** argv){
	// Путь набора имён свойств, принимаемых эталонной реализацией
	const char * path = ((argc > 1) ? argv[1] : "sh/unicode.accepted");
	// Выполняем открытие набора имён свойств
	ifstream file(path);
	string name;
	size_t checked = 0, diverged = 0, properties = 0;
	string first;
	while(getline(file, name)){
		if(name.empty()) continue;
		const string pattern = "^\\p{" + name + "}$";
		int32_t code = 0; PCRE2_SIZE offset = 0;
		pcre2_code * re = pcre2_compile((PCRE2_SPTR) pattern.c_str(), pattern.size(), PCRE2_UTF | PCRE2_UCP, &code, &offset, nullptr);
		if(re == nullptr) continue;
		// Разбираем имя свойства нашим модулем
		string key = name;
		const uint16_t id = regex::property(key);
		properties++;
		if(id == (uint16_t) regex::property_id_t::UNKNOWN){
			printf("нераспознано имя: %s\n", name.c_str());
			pcre2_code_free(re); diverged++; continue;
		}
		pcre2_match_data * data = pcre2_match_data_create_from_pattern(re, nullptr);
		for(uint32_t cp = 0; cp <= 0x10FFFF; cp++){
			if((cp >= 0xD800) && (cp <= 0xDFFF)) continue;
			char buffer[8]; const size_t length = encode(cp, buffer);
			const int32_t n = pcre2_match(re, (PCRE2_SPTR) buffer, length, 0, 0, data, nullptr);
			const bool theirs = (n > 0);
			const bool ours = regex::holds(cp, id);
			checked++;
			if(ours != theirs){
				diverged++;
				if(first.empty()){
					char tmp[256];
					snprintf(tmp, sizeof(tmp), "свойство «%s», символ U+%04X: AWH=%s, PCRE2=%s",
						name.c_str(), cp, (ours ? "да" : "нет"), (theirs ? "да" : "нет"));
					first = tmp;
				}
			}
		}
		pcre2_match_data_free(data); pcre2_code_free(re);
	}
	if(!first.empty()) printf("%s\n", first.c_str());
	printf("свойств: %zu, сличений: %zu, расхождений: %zu\n", properties, checked, diverged);
	return (diverged > 0 ? 1 : 0);
}
