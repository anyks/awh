// Дифференциальная проверка сопоставления AWH против PCRE2: сравнение границ совпадения и групп
#include <cstdio>
#include <random>
#include <string>
#include <vector>
#include <map>
#include <regex/engine.hpp>



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
	regex::engine_t engine;
	if(!engine.build(pattern, 0)) { supported = false; error = engine.message(); return false; }
	return engine.exec(text, 0, out);
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
	mt19937 engine(20260731);
	// Фрагменты шаблонов регулярного подмножества
	const vector <string> pieces = {
		"a", "b", "c", "x", "0", "1", ".", "^", "$", "|", "*", "+", "?",
		"(", ")", "[ab]", "[^a]", "[a-c]", "\\d", "\\w", "\\s", "\\b", "\\B",
		"(?:", "(", "a*", "b+", "c?", "a*?", "b+?", "c??", "{2}", "{1,3}", "{0,2}",
		"(?i)", "(?s)", "(?m)", "\\A", "\\z", "\\Z", "ab", "abc", "a|b", "(a)", "(a|b)",
		"(?i:", "[0-9]", "[^0-9]", "\\D", "\\W", "\\S", "A", "B", "-", "_", "\\.", "\\*",
		"(a)(b)", "((a))", "(a|)", "a{2,}", "[abc]+", "[^abc]*", "\\bx", "x\\b", ".*", ".+", ".?"
	};
	// Тексты сопоставления
	const vector <string> texts = {
		"", "a", "ab", "abc", "aab", "abcabc", "xyz", "aaa", "a\nb", "0a1b",
		" a b ", "abcabcabc", "cba", "aXbXc", "\n", "aaaa", "ab\ncd",
		"AbC", "A_B-C", "0123", "a.b*c", "  ", "aaaaaaaaaaaa", "xAx", "\n\n", "abc\n",
		"The quick brown fox", "a1b2c3", "___", "aBcDeF", "..."
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
