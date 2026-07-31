// Сличение разбиения на графемные кластеры с эталоном по набору правил эталона
#include <cstdio>
#include <string>
#include <vector>
#include <regex/unicode.hpp>
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
// Порядок классов разбиения, принятый эталоном
enum { CR = 0, LF, CTL, EXT, PRE, SPM, HL, HV, HT, HLV, HLVT, RI, OTH, ZWJ, EPI };
static int classify(const uint32_t code){
	switch((int) regex::cluster(code)){
		case (int) regex::cluster_t::CARRIAGE:    return CR;
		case (int) regex::cluster_t::LINEFEED:    return LF;
		case (int) regex::cluster_t::CONTROL:     return CTL;
		case (int) regex::cluster_t::EXTEND:      return EXT;
		case (int) regex::cluster_t::PREPEND:     return PRE;
		case (int) regex::cluster_t::SPACING:     return SPM;
		case (int) regex::cluster_t::HANGUL_L:    return HL;
		case (int) regex::cluster_t::HANGUL_V:    return HV;
		case (int) regex::cluster_t::HANGUL_T:    return HT;
		case (int) regex::cluster_t::HANGUL_LV:   return HLV;
		case (int) regex::cluster_t::HANGUL_LVT:  return HLVT;
		case (int) regex::cluster_t::REGIONAL:    return RI;
		case (int) regex::cluster_t::JOINER:      return ZWJ;
		case (int) regex::cluster_t::PICTORIAL:   return EPI;
	}
	return OTH;
}
#define ESZ ((1u<<EXT)|(1u<<SPM)|(1u<<ZWJ))
static const uint32_t table[] = {
	(1u<<LF), 0, 0, ESZ,
	ESZ|(1u<<PRE)|(1u<<HL)|(1u<<HV)|(1u<<HT)|(1u<<HLV)|(1u<<HLVT)|(1u<<OTH)|(1u<<RI),
	ESZ, ESZ|(1u<<HL)|(1u<<HV)|(1u<<HLV)|(1u<<HLVT), ESZ|(1u<<HV)|(1u<<HT), ESZ|(1u<<HT),
	ESZ|(1u<<HV)|(1u<<HT), ESZ|(1u<<HT), (1u<<RI), ESZ, ESZ|(1u<<EPI), ESZ
};
// Длина кластера по правилам эталона, разбираемая по кодовым значениям
static size_t reference(const vector <uint32_t> & codes){
	if(codes.empty()) return 0;
	int left = classify(codes.at(0));
	bool joined = false;
	size_t count = 1, ricount = 0;
	while(count < codes.size()){
		const int right = classify(codes.at(count));
		if((table[left] & (1u << right)) == 0) break;
		if((left == ZWJ) && (right == EPI) && !joined) break;
		if((left == RI) && (right == RI)){
			ricount = 0;
			for(size_t i = count; i > 0; i--){
				if(classify(codes.at(i - 1)) != RI) break;
				ricount++;
			}
			if((ricount & 1) == 0) break;
		}
		joined = ((left == EPI) && (right == ZWJ));
		if((right != EXT) || (left != EPI)) left = right;
		count++;
	}
	size_t length = 0; char buffer[8];
	for(size_t i = 0; i < count; i++) length += encode(codes.at(i), buffer);
	return length;
}
static pcre2_code * re = nullptr;
static pcre2_match_data * md = nullptr;
static size_t checked = 0, diverged = 0;
static string first;
static void probe(const vector <uint32_t> & codes){
	char buffer[64]; size_t length = 0;
	for(const uint32_t cp : codes) length += encode(cp, buffer + length);
	size_t theirs = 0;
	if(pcre2_match(re, (PCRE2_SPTR) buffer, length, 0, PCRE2_ANCHORED, md, nullptr) > 0){
		PCRE2_SIZE * ov = pcre2_get_ovector_pointer(md);
		theirs = (ov[1] - ov[0]);
	}
	const size_t ours = reference(codes);
	checked++;
	if(ours != theirs){
		diverged++;
		if(first.empty()){
			string text;
			for(const uint32_t cp : codes){ char tmp[16]; snprintf(tmp, sizeof(tmp), "U+%04X ", cp); text += tmp; }
			char tmp[256];
			snprintf(tmp, sizeof(tmp), "последовательность %s: таблицы AWH=%zu, PCRE2=%zu", text.c_str(), ours, theirs);
			first = tmp;
		}
	}
}
int main(){
	const string pattern = "\\X";
	int32_t code = 0; PCRE2_SIZE off = 0;
	re = pcre2_compile((PCRE2_SPTR) pattern.c_str(), pattern.size(), PCRE2_UTF | PCRE2_UCP, &code, &off, nullptr);
	if(re == nullptr){ printf("эталон не собран\n"); return 1; }
	md = pcre2_match_data_create_from_pattern(re, nullptr);
	const vector <uint32_t> reps = {
		0x0041, 0x000D, 0x000A, 0x0009, 0x0000, 0x0300, 0x200D, 0x1F1E6, 0x1F1E7, 0x0600,
		0x0903, 0x1100, 0x1160, 0x11A8, 0xAC00, 0xAC01, 0x1F600, 0x261D, 0x0BBE, 0x0E33,
		0x0915, 0x094D, 0x200C, 0x09BC, 0x0C3C, 0x0020, 0x00AD, 0x1F3FB, 0xE0020, 0x0483,
		0x1F9B0, 0x2764, 0xFE0F, 0x0930, 0x093C, 0x0D4D, 0x0D15, 0x1B44, 0x0F84, 0x00C5
	};
	for(uint32_t cp = 0; cp <= 0x10FFFF; cp++){
		if((cp >= 0xD800) && (cp <= 0xDFFF)) continue;
		probe({cp});
		for(const uint32_t next : reps) probe({cp, next});
	}
	for(const uint32_t a : reps)
		for(const uint32_t b : reps)
			for(const uint32_t c : reps){
				probe({a, b, c});
				for(const uint32_t d : reps) probe({a, b, c, d});
			}
	if(!first.empty()) printf("%s\n", first.c_str());
	printf("сличений таблиц разбиения: %zu, расхождений: %zu\n", checked, diverged);
	return (diverged > 0 ? 1 : 0);
}
