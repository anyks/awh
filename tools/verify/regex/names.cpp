// Сличение видов записи именованных групп с эталоном
#include <cstdio>
#include <string>
#include <vector>
#include <regex/engine.hpp>
#include "silent.hpp"
#define PCRE2_CODE_UNIT_WIDTH 8
#include <pcre2.h>
using namespace std; using namespace awh;
struct Item { const char * pattern; const char * text; };
int main(){
	const vector<Item> items = {
		{"(?<w>a+)b", "aab"},
		{"(?'w'a+)b", "aab"},
		{"(?P<w>a+)b", "aab"},
		{"(?<w>a)\\k<w>", "aa"},
		{"(?<w>a)\\k'w'", "aa"},
		{"(?<w>a)\\k{w}", "aa"},
		{"(?<w>a)(?P=w)", "aa"},
		{"(?<w>a)\\g{w}", "aa"},
		{"(?<w>a+)(?&w)", "aaa"},
		{"(?<w>a+)(?P>w)", "aaa"},
		{"(?<w>a)?(?(<w>)b|c)", "ab"},
		{"(?<w>a)?(?('w')b|c)", "ab"},
		{"(?<w>a)?(?(w)b|c)", "ab"},
		{"(?(DEFINE)(?<w>a+))(?&w)b", "aaab"},
		{"(?<y>\\d{4})-(?<m>\\d{2})", "2026-07"},
		{"(?<a>x)(?<b>y)\\k<a>\\k<b>", "xyxy"},
		{"(?J)(?<n>a)|(?<n>b)", "b"},
		{"(?<n>a)(?<m>b)(?&n)", "aba"}
	};
	size_t checked = 0, diverged = 0;
	for(const auto & item : items){
		int32_t code = 0; PCRE2_SIZE off = 0;
		pcre2_code * re = pcre2_compile((PCRE2_SPTR) item.pattern, PCRE2_ZERO_TERMINATED, 0, &code, &off, nullptr);
		const bool theirsBuilt = (re != nullptr);
		bool theirs = false; size_t tb = 0, te = 0;
		if(theirsBuilt){
			pcre2_match_data * d = pcre2_match_data_create_from_pattern(re, nullptr);
			if(pcre2_match(re, (PCRE2_SPTR) item.text, PCRE2_ZERO_TERMINATED, 0, 0, d, nullptr) > 0){
				theirs = true; PCRE2_SIZE * ov = pcre2_get_ovector_pointer(d); tb = ov[0]; te = ov[1];
			}
			pcre2_match_data_free(d); pcre2_code_free(re);
		}
		regex::engine_t engine(verify::logger());
		const bool oursBuilt = engine.build(item.pattern, 0);
		bool ours = false; size_t ob = 0, oe = 0;
		vector<pair<size_t,size_t>> caps;
		if(oursBuilt && engine.exec(item.text, 0, caps)){ ours = true; ob = caps[0].first; oe = caps[0].second; }
		checked++;
		const bool same = ((oursBuilt == theirsBuilt) && (ours == theirs) && (!ours || ((ob == tb) && (oe == te))));
		if(!same){
			diverged++;
			printf("%-30s текст «%s»: AWH сборка=%d совпад=%d [%zu,%zu], PCRE2 сборка=%d совпад=%d [%zu,%zu]\n",
				item.pattern, item.text, (int) oursBuilt, (int) ours, ob, oe, (int) theirsBuilt, (int) theirs, tb, te);
		}
	}
	printf("\nвидов записи: %zu, расхождений: %zu\n", checked, diverged);
	return (diverged > 0 ? 1 : 0);
}
