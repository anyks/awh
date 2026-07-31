// Сличение границы слова с эталоном на парах символов различных категорий
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
	// Представители различных категорий символов
	const vector <uint32_t> points = {
		0x41, 0x61, 0x30, 0x5F, 0x20, 0x2E, 0x0A, 0x09,
		0x410, 0x430, 0x4E00, 0x3042, 0x300, 0x5D0, 0x660, 0x2018,
		0xA0, 0xC0, 0xE9, 0x1F600, 0x10400, 0x2028, 0x200B, 0x180E
	};
	const vector <string> patterns = {"\\b", "\\B", "a\\b", "\\bx", "\\w\\b\\W", "\\B\\w"};
	size_t checked = 0, diverged = 0; string first;
	for(const auto & item : patterns){
		for(int ucp = 0; ucp < 2; ucp++){
			const uint32_t options = PCRE2_UTF | (ucp ? PCRE2_UCP : 0);
			int32_t code = 0; PCRE2_SIZE off = 0;
			pcre2_code * re = pcre2_compile((PCRE2_SPTR) item.c_str(), item.size(), options, &code, &off, nullptr);
			if(re == nullptr) continue;
			pcre2_match_data * d = pcre2_match_data_create_from_pattern(re, nullptr);
			uint32_t flags = (uint32_t) regex::flag_t::UTF;
			if(ucp) flags |= (uint32_t) regex::flag_t::UCP;
			regex::parser_t parser; regex::compiler_t compiler; regex::program_t prog; regex::backtrack_t bt;
			if(!parser.parse(item, flags) || !compiler.compileFull(parser, prog)){ pcre2_match_data_free(d); pcre2_code_free(re); continue; }
			vector <pair <size_t, size_t>> out;
			for(auto & a : points){
				for(auto & b : points){
					char buffer[16]; size_t length = encode(a, buffer);
					length += encode(b, buffer + length);
					const int32_t n = pcre2_match(re, (PCRE2_SPTR) buffer, length, 0, 0, d, nullptr);
					const bool theirs = (n > 0);
					const size_t at = (theirs ? pcre2_get_ovector_pointer(d)[0] : 0);
					const bool ours = bt.exec(prog, string_view(buffer, length), 0, out);
					const size_t ourat = (ours ? out.front().first : 0);
					checked++;
					if((ours != theirs) || (ours && (at != ourat))){
						diverged++;
						if(first.empty()){
							char tmp[256];
							snprintf(tmp, sizeof(tmp), "шаблон «%s»%s, U+%04X U+%04X: AWH=%s@%zu, PCRE2=%s@%zu",
								item.c_str(), (ucp ? " в режиме UCP" : ""), a, b,
								(ours?"да":"нет"), ourat, (theirs?"да":"нет"), at);
							first = tmp;
						}
					}
				}
			}
			pcre2_match_data_free(d); pcre2_code_free(re);
		}
	}
	if(!first.empty()) printf("%s\n", first.c_str());
	printf("сличений границы слова: %zu, расхождений: %zu\n", checked, diverged);
	return (diverged > 0 ? 1 : 0);
}
