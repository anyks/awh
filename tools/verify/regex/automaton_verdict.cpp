// Дифференциальная проверка вердикта DFA против PCRE2 и против Pike VM
#include <cstdio>
#include <random>
#include <string>
#include <vector>
#include <map>
#include <regex/dfa.hpp>
#include <regex/pike.hpp>
#include <regex/parser.hpp>
#include <regex/compiler.hpp>
#define PCRE2_CODE_UNIT_WIDTH 8
#include <pcre2.h>
using namespace std;
using namespace awh;
static bool reference(const string & p, const string & t){
	int32_t e=0; PCRE2_SIZE o=0;
	pcre2_code * re = pcre2_compile((PCRE2_SPTR)p.c_str(), p.size(), 0, &e, &o, nullptr);
	if(re == nullptr) return false;
	pcre2_match_data * md = pcre2_match_data_create_from_pattern(re, nullptr);
	const int32_t c = pcre2_match(re, (PCRE2_SPTR)t.c_str(), t.size(), 0, 0, md, nullptr);
	pcre2_match_data_free(md); pcre2_code_free(re);
	return (c > 0);
}
int main(int argc, char * argv[]){
	const size_t total = ((argc > 1) ? (size_t) ::atoll(argv[1]) : 200000);
	mt19937 engine(20260731);
	const vector<string> pieces = {
		"a","b","c","x","0","1",".","^","$","|","*","+","?","(",")","[ab]","[^a]","[a-c]",
		"\\d","\\w","\\s","\\b","\\B","(?:","a*","b+","c?","a*?","b+?","c??","{2}","{1,3}","{0,2}",
		"(?i)","(?s)","(?m)","\\A","\\z","\\Z","ab","abc","a|b","(a)","(a|b)","(?i:","[0-9]","[^0-9]",
		"\\D","\\W","\\S","A","B","-","_","\\.","\\*","(a)(b)","((a))","(a|)","a{2,}","[abc]+","[^abc]*",
		"\\bx","x\\b",".*",".+",".?"
	};
	const vector<string> texts = {
		"","a","ab","abc","aab","abcabc","xyz","aaa","a\nb","0a1b"," a b ","abcabcabc","cba","aXbXc",
		"\n","aaaa","ab\ncd","AbC","A_B-C","0123","a.b*c","  ","aaaaaaaaaaaa","xAx","\n\n","abc\n",
		"The quick brown fox","a1b2c3","___","aBcDeF","..."
	};
	uniform_int_distribution<size_t> lengths(1,9), indexes(0,pieces.size()-1), samples(0,texts.size()-1);
	size_t compared = 0, skipped = 0, diverged = 0;
	map<string,pair<size_t,string>> groups;
	for(size_t i = 0; i < total; i++){
		string pattern;
		const size_t n = lengths(engine);
		for(size_t j = 0; j < n; j++) pattern.append(pieces.at(indexes(engine)));
		const string & text = texts.at(samples(engine));
		regex::parser_t parser;
		if(!parser.parse(pattern, 0)) continue;
		regex::compiler_t compiler; regex::program_t program;
		if(!compiler.compile(parser, program)){ skipped++; continue; }
		regex::dfa_t dfa;
		if(!dfa.available(program)){ skipped++; continue; }
		const bool a = dfa.test(program, text, 0);
		const bool b = reference(pattern, text);
		regex::pike_t pike; vector<pair<size_t,size_t>> caps;
		const bool c = pike.exec(program, text, 0, caps);
		compared++;
		if((a == b) && (a == c)) continue;
		diverged++;
		const string key = ((a != b) ? "DFA против PCRE2" : "DFA против Pike VM");
		auto & s = groups[key];
		s.first++;
		if(s.second.empty()){
			char buf[512];
			::snprintf(buf, sizeof(buf), "шаблон «%s», текст «%s», DFA=%s PCRE2=%s Pike=%s",
				pattern.c_str(), text.c_str(), (a?"да":"нет"), (b?"да":"нет"), (c?"да":"нет"));
			s.second = buf;
		}
	}
	for(const auto & it : groups) ::printf("%7zu  %-22s  %s\n", it.second.first, it.first.c_str(), it.second.second.c_str());
	::printf("\nСличено: %zu, пропущено: %zu, расхождений: %zu\n", compared, skipped, diverged);
	return (diverged > 0 ? 1 : 0);
}
