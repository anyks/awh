// Сличение выборки захваченного текста по имени группы с эталоном
#include <cstdio>
#include <string>
#include <vector>
#include <initializer_list>
#include <regex/regex.hpp>
#include "silent.hpp"
#define PCRE2_CODE_UNIT_WIDTH 8
#include <pcre2.h>
using namespace std; using namespace awh;
struct Item { const char * pattern; const char * text; const char * name; };
int main(){
	const vector<Item> items = {
		{"(?<y>\\d{4})-(?<m>\\d{2})-(?<d>\\d{2})", "дата 2026-07-31 тут", "y"},
		{"(?<y>\\d{4})-(?<m>\\d{2})-(?<d>\\d{2})", "дата 2026-07-31 тут", "m"},
		{"(?<y>\\d{4})-(?<m>\\d{2})-(?<d>\\d{2})", "дата 2026-07-31 тут", "d"},
		{"(?<a>x)?(?<b>y)", "y", "a"},
		{"(?<a>x)?(?<b>y)", "y", "b"},
		{"(?<e>z*)y", "y", "e"},
		{"(?J)(?<n>a)|(?<n>b)", "a", "n"},
		{"(?J)(?<n>a)|(?<n>b)", "b", "n"},
		{"(?J)(?<n>a)(?<n>b)", "ab", "n"},
		{"(?P<host>[\\w.]+):(?P<port>\\d+)", "anyks.com:8080", "host"},
		{"(?P<host>[\\w.]+):(?P<port>\\d+)", "anyks.com:8080", "port"},
		{"(?<w>\\w+) \\k<w>", "hello hello", "w"}
	};
	size_t checked = 0, diverged = 0;
	regexp_t regexp(verify::logger());
	for(const auto & item : items){
		// Эталон
		int32_t e = 0; PCRE2_SIZE o = 0;
		pcre2_code * re = pcre2_compile((PCRE2_SPTR) item.pattern, PCRE2_ZERO_TERMINATED, 0, &e, &o, nullptr);
		string theirs; bool theirsGot = false;
		if(re != nullptr){
			pcre2_match_data * d = pcre2_match_data_create_from_pattern(re, nullptr);
			if(pcre2_match(re, (PCRE2_SPTR) item.text, PCRE2_ZERO_TERMINATED, 0, 0, d, nullptr) > 0){
				PCRE2_UCHAR * buf = nullptr; PCRE2_SIZE len = 0;
				if(pcre2_substring_get_byname(d, (PCRE2_SPTR) item.name, &buf, &len) == 0){
					theirsGot = true; theirs.assign((const char *) buf, len); pcre2_substring_free(buf);
				}
			}
			pcre2_match_data_free(d); pcre2_code_free(re);
		}
		// Модуль
		const auto exp = regexp.build(item.pattern);
		string ours; bool oursGot = false;
		if(exp){
			vector<pair<size_t,size_t>> bounds;
			if(regexp.match(item.text, exp, bounds)){
				const string_view value = regexp.capture(item.text, bounds, exp, item.name);
				if(value.data() != nullptr){ oursGot = true; ours.assign(value); }
			}
		}
		checked++;
		if((oursGot != theirsGot) || (oursGot && (ours != theirs))){
			diverged++;
			printf("%-42s текст «%s» имя «%s»: AWH=%s«%s», PCRE2=%s«%s»\n",
				item.pattern, item.text, item.name,
				(oursGot?"":"нет "), ours.c_str(), (theirsGot?"":"нет "), theirs.c_str());
		}
	}
	printf("\nвыборок по имени: %zu, расхождений: %zu\n", checked, diverged);
	return (diverged > 0 ? 1 : 0);
}
