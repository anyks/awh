// Исчерпывающее сличение сокращённых классов с эталоном по всем кодовым значениям
#include <cstdio>
#include <string>
#include <vector>
#include <regex/parser.hpp>
#include <regex/compiler.hpp>
#include <regex/backtrack.hpp>
#define PCRE2_CODE_UNIT_WIDTH 8
#include <pcre2.h>
using namespace std;
using namespace awh;
static size_t encode(uint32_t code, char * out){
	if(code < 0x80){ out[0] = (char) code; return 1; }
	if(code < 0x800){ out[0] = (char)(0xC0 | (code >> 6)); out[1] = (char)(0x80 | (code & 0x3F)); return 2; }
	if(code < 0x10000){ out[0] = (char)(0xE0 | (code >> 12)); out[1] = (char)(0x80 | ((code >> 6) & 0x3F)); out[2] = (char)(0x80 | (code & 0x3F)); return 3; }
	out[0] = (char)(0xF0 | (code >> 18)); out[1] = (char)(0x80 | ((code >> 12) & 0x3F));
	out[2] = (char)(0x80 | ((code >> 6) & 0x3F)); out[3] = (char)(0x80 | (code & 0x3F)); return 4;
}
int main(){
	const vector <string> classes = {"\\d", "\\D", "\\w", "\\W", "\\s", "\\S", "\\h", "\\H", "\\v", "\\V", "[\\w\\d]", "[^\\s]"};
	size_t checked = 0, diverged = 0; string first;
	for(const auto & item : classes){
		for(int ucp = 0; ucp < 2; ucp++){
			const string pattern = "^" + item + "$";
			const uint32_t options = PCRE2_UTF | (ucp ? PCRE2_UCP : 0);
			int32_t code = 0; PCRE2_SIZE off = 0;
			pcre2_code * re = pcre2_compile((PCRE2_SPTR) pattern.c_str(), pattern.size(), options, &code, &off, nullptr);
			if(re == nullptr) continue;
			pcre2_match_data * d = pcre2_match_data_create_from_pattern(re, nullptr);
			// Собираем то же выражение нашим модулем
			uint32_t flags = (uint32_t) regex::flag_t::UTF;
			if(ucp) flags |= (uint32_t) regex::flag_t::UCP;
			regex::parser_t parser; regex::compiler_t compiler; regex::program_t prog; regex::backtrack_t bt;
			if(!parser.parse(pattern, flags)){ printf("разбор отказ: %s\n", pattern.c_str()); pcre2_match_data_free(d); pcre2_code_free(re); continue; }
			if(!compiler.compileFull(parser, prog)){ printf("компиляция отказ: %s\n", pattern.c_str()); pcre2_match_data_free(d); pcre2_code_free(re); continue; }
			vector <pair <size_t, size_t>> out;
			for(uint32_t cp = 0; cp <= 0x10FFFF; cp++){
				if((cp >= 0xD800) && (cp <= 0xDFFF)) continue;
				char buffer[8]; const size_t length = encode(cp, buffer);
				const bool theirs = (pcre2_match(re, (PCRE2_SPTR) buffer, length, 0, 0, d, nullptr) > 0);
				const bool ours = bt.exec(prog, string_view(buffer, length), 0, out);
				checked++;
				if(ours != theirs){
					diverged++;
					if(first.empty()){
						char tmp[256];
						snprintf(tmp, sizeof(tmp), "класс «%s»%s, символ U+%04X: AWH=%s, PCRE2=%s",
							item.c_str(), (ucp ? " в режиме UCP" : ""), cp, (ours?"да":"нет"), (theirs?"да":"нет"));
						first = tmp;
					}
				}
			}
			pcre2_match_data_free(d); pcre2_code_free(re);
		}
	}
	if(!first.empty()) printf("%s\n", first.c_str());
	printf("сличений сокращённых классов: %zu, расхождений: %zu\n", checked, diverged);
	return (diverged > 0 ? 1 : 0);
}
