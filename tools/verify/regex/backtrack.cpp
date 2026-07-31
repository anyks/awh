// Дифференциальная проверка сопоставления AWH против PCRE2: сравнение границ совпадения и групп
#include <cstdio>
#include <random>
#include <string>
#include <vector>
#include <map>
#include <regex/parser.hpp>
#include <regex/compiler.hpp>
#include <regex/backtrack.hpp>

#define PCRE2_CODE_UNIT_WIDTH 8
#include <pcre2.h>

using namespace std;
using namespace awh;

// Результат сопоставления: границы совпадения и захваченных групп
using result_t = vector <pair <size_t, size_t>>;

// Сопоставление эталонной реализацией
static bool reference(const string & pattern, const string & text, result_t & out) {
	int32_t code = 0; PCRE2_SIZE offset = 0; out.clear();
	pcre2_code * re = pcre2_compile((PCRE2_SPTR) pattern.c_str(), pattern.size(), 0, &code, &offset, nullptr);
	if(re == nullptr) return false;
	pcre2_match_data * data = pcre2_match_data_create_from_pattern(re, nullptr);
	const int32_t count = pcre2_match(re, (PCRE2_SPTR) text.c_str(), text.size(), 0, 0, data, nullptr);
	bool result = false;
	if(count > 0) {
		result = true;
		const PCRE2_SIZE * vec = pcre2_get_ovector_pointer(data);
		const uint32_t total = pcre2_get_ovector_count(data);
		for(uint32_t i = 0; i < total; i++) out.emplace_back(vec[i * 2], vec[(i * 2) + 1]);
	}
	pcre2_match_data_free(data);
	pcre2_code_free(re);
	return result;
}

// Сопоставление модулем AWH
static bool actual(const string & pattern, const string & text, result_t & out, bool & supported, string & error) {
	out.clear(); supported = true;
	regex::parser_t parser;
	if(!parser.parse(pattern, 0)) { supported = false; error = parser.message(); return false; }
	regex::compiler_t compiler;
	regex::program_t program;
	if(!compiler.compileFull(parser, program)) { supported = false; error = "unsupported"; return false; }
	regex::backtrack_t pike;
	return pike.exec(program, text, 0, out);
}

// Сравнение результатов сопоставления
static bool same(const result_t & a, const result_t & b) {
	const size_t count = ((a.size() < b.size()) ? a.size() : b.size());
	for(size_t i = 0; i < count; i++) {
		if((a.at(i).first != b.at(i).first) || (a.at(i).second != b.at(i).second)) return false;
	}
	// Хвост, отсутствующий у одной из сторон, должен быть незахваченным
	const result_t & rest = ((a.size() > b.size()) ? a : b);
	for(size_t i = count; i < rest.size(); i++) {
		if(rest.at(i).first != PCRE2_UNSET) return false;
	}
	return true;
}

int main(int argc, char * argv[]) {
	const size_t total = ((argc > 1) ? (size_t) ::atoll(argv[1]) : 200000);
	mt19937 engine(20260801);
	// Фрагменты шаблонов регулярного подмножества
	const vector <string> pieces = {
		"a", "b", "c", "x", "0", ".", "^", "$", "|", "*", "+", "?",
		"(", ")", "[ab]", "[^a]", "\\d", "\\w", "\\b",
		"(?:", "(", "a*", "b+", "c?", "a*?", "{2}", "{1,3}",
		"(?i)", "ab", "a|b", "(a)", "(a|b)", "(a)(b)", "((a))",
		// конструкции вне регулярного подмножества
		"\\1", "\\2", "(a)\\1", "(\\w)\\1", "(a|b)\\1",
		"(?=", "(?!", "(?<=", "(?<!", "(?=a)", "(?!a)", "(?<=a)", "(?<!a)", "(?=ab|c)",
		"(?>", "(?>a)", "(?>a+)", "a++", "b*+", "c?+", "a{2,3}+",
		"\\K", "a\\K", "(a*)*", "(a?)*", "(|a)*", "(a*)+", "(?:a|)*",
		"(?(1)a|b)", "(?(1)a)", "(?(?=a)b|c)", "(?(R)a|b)",
		"(?R)", "(?1)", "(?2)", "(?:(?1))?",
		"(?P<n>a)", "(?P=n)", "\\k<n>", "(?&n)",
		"(?(DEFINE)(?<w>a))", "(?<x>b)", "\\g{1}", "\\g1"
	};
	// Тексты сопоставления
	const vector <string> texts = {
		"", "a", "aa", "ab", "abc", "aab", "abab", "aaa", "aaaa", "b", "ba",
		"abba", "aabb", "(a(b)c)", "((x))", "xaax", "aXa", "AA", "aA", "Aa",
		"abcabc", "a\nb", "0a1b", " a b ", "cba", "\n", "aaaaaaaa", "xyz", "ababab"
	};
	uniform_int_distribution <size_t> lengths(1, 9);
	uniform_int_distribution <size_t> indexes(0, pieces.size() - 1);
	uniform_int_distribution <size_t> samples(0, texts.size() - 1);
	size_t compared = 0, unsupported = 0, diverged = 0;
	map <string, pair <size_t, string>> groups;
	for(size_t i = 0; i < total; i++) {
		string pattern;
		const size_t count = lengths(engine);
		for(size_t j = 0; j < count; j++) pattern.append(pieces.at(indexes(engine)));
		const string & text = texts.at(samples(engine));
		result_t ours, theirs; bool supported = true; string error;
		// Шаблон должен приниматься эталоном
		int32_t code = 0; PCRE2_SIZE offset = 0;
		pcre2_code * probe = pcre2_compile((PCRE2_SPTR) pattern.c_str(), pattern.size(), 0, &code, &offset, nullptr);
		if(probe == nullptr) continue;
		pcre2_code_free(probe);
		const bool a = actual(pattern, text, ours, supported, error);
		if(!supported) { unsupported++; continue; }
		const bool b = reference(pattern, text, theirs);
		compared++;
		if((a != b) || (a && !same(ours, theirs))) {
			diverged++;
			string key = ((a != b) ? "разный вердикт сопоставления" : "разные границы совпадения");
			auto & sample = groups[key];
			sample.first++;
			if(sample.second.empty()) {
				char buffer[512];
				::snprintf(buffer, sizeof(buffer), "шаблон «%s», текст «%s», AWH=%s[%zu,%zu], PCRE2=%s[%zu,%zu]",
					pattern.c_str(), text.c_str(), (a ? "да" : "нет"),
					(ours.empty() ? 0 : ours.front().first), (ours.empty() ? 0 : ours.front().second),
					(b ? "да" : "нет"),
					(theirs.empty() ? 0 : theirs.front().first), (theirs.empty() ? 0 : theirs.front().second));
				sample.second = buffer;
			}
		}
	}
	for(const auto & item : groups)
		::printf("%7zu  %-32s  %s\n", item.second.first, item.first.c_str(), item.second.second.c_str());
	::printf("\nСличено: %zu, вне регулярного подмножества: %zu, расхождений: %zu\n", compared, unsupported, diverged);
	return (diverged > 0 ? 1 : 0);
}
