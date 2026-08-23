// Проверка обратного пути движка на длинных текстах: границы совпадения и групп против PCRE2
#include <cstdio>
#include <random>
#include <string>
#include <vector>
#include <regex/engine.hpp>
#include "silent.hpp"
#define PCRE2_CODE_UNIT_WIDTH 8
#include <pcre2.h>
using namespace std;
using namespace awh;
using result_t = vector<pair<size_t,size_t>>;
static bool oracle(const string & p, const string & t, result_t & out){
	int32_t e=0; PCRE2_SIZE o=0; out.clear();
	pcre2_code * re = pcre2_compile((PCRE2_SPTR)p.c_str(), p.size(), 0, &e, &o, nullptr);
	if(re == nullptr) return false;
	pcre2_match_data * md = pcre2_match_data_create_from_pattern(re, nullptr);
	const int32_t c = pcre2_match(re, (PCRE2_SPTR)t.c_str(), t.size(), 0, 0, md, nullptr);
	bool r = false;
	if(c > 0){ r = true;
		const PCRE2_SIZE * v = pcre2_get_ovector_pointer(md);
		for(uint32_t i = 0; i < pcre2_get_ovector_count(md); i++) out.emplace_back(v[i*2], v[i*2+1]);
	}
	pcre2_match_data_free(md); pcre2_code_free(re);
	return r;
}
static bool same(const result_t & a, const result_t & b){
	const size_t n = ((a.size() < b.size()) ? a.size() : b.size());
	for(size_t i = 0; i < n; i++) if((a[i].first != b[i].first) || (a[i].second != b[i].second)) return false;
	const result_t & rest = ((a.size() > b.size()) ? a : b);
	for(size_t i = n; i < rest.size(); i++) if(rest[i].first != PCRE2_UNSET) return false;
	return true;
}
int main(){
	mt19937 engine(20260731);
	// Основа текста и позиции вставки совпадения
	const string filler = "the quick brown fox jumps over the lazy dog 12345 ";
	struct Case { const char * pattern; const char * needle; };
	const vector<Case> cases = {
		{"(\\w+)@(\\w+)\\.(\\w+)", "user@example.com"},
		{"^.*?(ERROR|WARN) ([0-9]{3,5}):", "ERROR 4041:"},
		{"([a-z]+)ing\\b", "running"},
		{"(?i)(HTTP)/(\\d)\\.(\\d)", "http/1.1"},
		{"\\b([A-Z][a-z]+) ([A-Z][a-z]+)\\b", "John Smith"},
		{"(a+)(b+)(c+)", "aaabbbccc"},
		{"([0-9]{1,3})\\.([0-9]{1,3})\\.([0-9]{1,3})\\.([0-9]{1,3})", "192.168.001.100"},
		{"<(\\w+)>([^<]*)</\\1>", "<tag>value</tag>"}
	};
	// Длины текста, на которых проверяется выбор способа сопоставления
	const vector<size_t> sizes = {0, 1, 40, 600, 5000, 100000, 1000000};
	size_t compared = 0, diverged = 0, unsupported = 0;
	for(const auto & c : cases){
		for(const size_t target : sizes){
			// Собираем текст заданной длины
			string base;
			while(base.size() < target) base += filler;
			base.resize(target);
			// Размещаем искомое в трёх положениях: начало, середина, конец
			for(int place = 0; place < 3; place++){
				string text = base;
				const size_t at = ((place == 0) ? 0 : ((place == 1) ? (text.size() / 2) : text.size()));
				text.insert(at, c.needle);
				regex::engine_t eng(verify::logger());
				if(!eng.build(c.pattern, 0)){ unsupported++; continue; }
				result_t ours, theirs;
				const bool a = eng.exec(text, 0, ours);
				const bool b = oracle(c.pattern, text, theirs);
				compared++;
				if((a == b) && (!a || same(ours, theirs))) continue;
				diverged++;
				::printf("РАСХОЖДЕНИЕ шаблон «%s» длина %zu положение %d: AWH=%s[%zd,%zd] PCRE2=%s[%zd,%zd]\n",
					c.pattern, text.size(), place, (a?"да":"нет"),
					(ssize_t)(ours.empty()?-1:(ssize_t)ours[0].first), (ssize_t)(ours.empty()?-1:(ssize_t)ours[0].second),
					(b?"да":"нет"),
					(ssize_t)(theirs.empty()?-1:(ssize_t)theirs[0].first), (ssize_t)(theirs.empty()?-1:(ssize_t)theirs[0].second));
			}
		}
	}
	::printf("\nСличено: %zu, вне подмножества: %zu, расхождений: %zu\n", compared, unsupported, diverged);
	return (diverged > 0 ? 1 : 0);
}
