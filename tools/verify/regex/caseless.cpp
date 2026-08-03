// Исчерпывающее сличение сопоставления без учёта регистра с эталоном
#include <cstdio>
#include <string>
#include <vector>
#include <regex/engine.hpp>
#include <unicode/unicode.hpp>
#define PCRE2_CODE_UNIT_WIDTH 8
#include <pcre2.h>
using namespace std; using namespace awh;
static size_t encode(uint32_t code, char * out){
	if(code < 0x80){ out[0]=(char)code; return 1; }
	if(code < 0x800){ out[0]=(char)(0xC0|(code>>6)); out[1]=(char)(0x80|(code&0x3F)); return 2; }
	if(code < 0x10000){ out[0]=(char)(0xE0|(code>>12)); out[1]=(char)(0x80|((code>>6)&0x3F)); out[2]=(char)(0x80|(code&0x3F)); return 3; }
	out[0]=(char)(0xF0|(code>>18)); out[1]=(char)(0x80|((code>>12)&0x3F));
	out[2]=(char)(0x80|((code>>6)&0x3F)); out[3]=(char)(0x80|(code&0x3F)); return 4;
}
int main(){
	size_t checked = 0, diverged = 0; string first;
	// Сличаем сопоставление одиночного символа с каждым символом иного регистра
	for(uint32_t cp = 1; cp <= 0x10FFFF; cp++){
		if((cp >= 0xD800) && (cp <= 0xDFFF)) continue;
		char pat[8]; const size_t plen = encode(cp, pat);
		const string pattern(pat, plen);
		int32_t code = 0; PCRE2_SIZE off = 0;
		pcre2_code * re = pcre2_compile((PCRE2_SPTR) pattern.c_str(), pattern.size(), PCRE2_UTF|PCRE2_CASELESS|PCRE2_ANCHORED, &code, &off, nullptr);
		if(re == nullptr) continue;
		pcre2_match_data * d = pcre2_match_data_create_from_pattern(re, nullptr);
		regex::engine_t engine;
		const uint32_t flags = ((uint32_t) regex::flag_t::UTF | (uint32_t) regex::flag_t::CASELESS | (uint32_t) regex::flag_t::ANCHORED);
		if(!engine.build(pattern, flags)){ pcre2_match_data_free(d); pcre2_code_free(re); printf("сборка отказ U+%04X\n", cp); continue; }
		// Проверяем сам символ и все символы, приводимые к тому же значению
		vector <uint32_t> members;
		members.push_back(cp);
		unicode::variants(cp, members);
		for(const uint32_t other : members){
			char buf[8]; const size_t len = encode(other, buf);
			const bool theirs = (pcre2_match(re, (PCRE2_SPTR) buf, len, 0, 0, d, nullptr) > 0);
			const bool ours = engine.test(string_view(buf, len), 0);
			checked++;
			if(ours != theirs){
				diverged++;
				if(first.empty()){
					char tmp[256];
					snprintf(tmp, sizeof(tmp), "шаблон U+%04X, текст U+%04X: AWH=%s, PCRE2=%s", cp, other, (ours?"да":"нет"), (theirs?"да":"нет"));
					first = tmp;
				}
			}
		}
		pcre2_match_data_free(d); pcre2_code_free(re);
	}
	if(!first.empty()) printf("%s\n", first.c_str());
	printf("сличений приведения регистра: %zu, расхождений: %zu\n", checked, diverged);
	return (diverged > 0 ? 1 : 0);
}
