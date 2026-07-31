// Дифференциальный фаззинг парсера AWH против PCRE2 на случайно порождаемых шаблонах
#include <cstdio>
#include <random>
#include <string>
#include <vector>
#include <map>
#include <regex/parser.hpp>

#define PCRE2_CODE_UNIT_WIDTH 8
#include <pcre2.h>

using namespace std;
using namespace awh;

int main(int argc, char * argv[]) {
	// Получаем количество порождаемых шаблонов
	const size_t total = ((argc > 1) ? static_cast <size_t> (::atoll(argv[1])) : 200000);
	// Создаём генератор псевдослучайных чисел с фиксированным зерном
	mt19937 engine(20260731);
	// Создаём набор фрагментов, из которых собираются шаблоны
	const vector <string> pieces = {
		"a", "b", "z", "0", "9", ".", "^", "$", "|", "*", "+", "?", "-", ":", "=", "!", ",",
		"(", ")", "[", "]", "{", "}", "<", ">", "'", "&", "#", "\\", "/",
		"\\d", "\\w", "\\s", "\\D", "\\W", "\\S", "\\b", "\\B", "\\A", "\\z", "\\Z", "\\G",
		"\\K", "\\R", "\\N", "\\Q", "\\E", "\\1", "\\2", "\\g", "\\k", "\\p", "\\P", "\\x",
		"\\o", "\\c", "\\n", "\\t", "\\0",
		"(?", "(?:", "(?>", "(?=", "(?!", "(?<", "(?<=", "(?<!", "(?'", "(?|", "(?#", "(?P",
		"(?R)", "(?1)", "(?-1)", "(?+1)", "(?&", "(?(", "(?i)", "(?-i)", "(?i:", "(?im-sx:",
		"{2}", "{2,}", "{2,5}", "{,5}", "{}", "[a-z]", "[^a]", "[[:alpha:]]", "[[:^x:]]",
		"\\p{L}", "\\p{Nd}", "\\x{41}", "\\k<n>", "\\g{1}", "\\g<n>", "n", ">", "abc", "DEFINE"
	};
	// Создаём распределение количества фрагментов в шаблоне
	uniform_int_distribution <size_t> lengths(1, 8);
	// Создаём распределение индексов фрагментов
	uniform_int_distribution <size_t> indexes(0, pieces.size() - 1);
	// Счётчик расхождений вердиктов
	size_t diverged = 0;
	// Счётчик шаблонов, принятых обеими реализациями
	size_t accepted = 0;
	// Набор образцов расхождений, сгруппированных по тексту ошибки
	map <string, pair <size_t, string>> samples;
	// Выполняем порождение и сравнение шаблонов
	for(size_t i = 0; i < total; i++) {
		// Собираем очередной шаблон из случайных фрагментов
		string pattern;
		// Получаем количество фрагментов очередного шаблона
		const size_t count = lengths(engine);
		// Выполняем сборку шаблона из фрагментов
		for(size_t j = 0; j < count; j++) pattern.append(pieces.at(indexes(engine)));
		// Разбираем шаблон парсером AWH
		regex::parser_t parser;
		// Получаем вердикт парсера AWH
		const bool ours = parser.parse(pattern, 0);
		// Компилируем шаблон эталонной реализацией
		int32_t code = 0;
		PCRE2_SIZE offset = 0;
		pcre2_code * result = pcre2_compile(
			reinterpret_cast <PCRE2_SPTR> (pattern.c_str()), pattern.size(),
			0, &code, &offset, nullptr
		);
		// Получаем вердикт эталонной реализации
		const bool theirs = (result != nullptr);
		// Освобождаем скомпилированный шаблон эталонной реализации
		if(result != nullptr) pcre2_code_free(result);
		// Учитываем шаблон, принятый обеими реализациями
		if(ours && theirs) accepted++;
		// Пропускаем шаблоны с совпавшими вердиктами
		if(ours == theirs) continue;
		// Учитываем расхождение вердиктов
		diverged++;
		// Получаем текст ошибки отклонившей реализации
		char buffer[256] = {0};
		if(!theirs) pcre2_get_error_message(code, reinterpret_cast <PCRE2_UCHAR *> (buffer), sizeof(buffer));
		// Формируем ключ группировки расхождений
		const string key(string(ours ? "AWH принял, PCRE2: " : "PCRE2 принял, AWH: ") + (ours ? buffer : parser.message()));
		// Учитываем образец расхождения
		auto & sample = samples[key];
		// Увеличиваем счётчик расхождений группы
		sample.first++;
		// Сохраняем первый образец шаблона группы
		if(sample.second.empty()) sample.second = pattern;
	}
	// Выводим сводку расхождений по группам
	for(const auto & item : samples)
		::printf("%7zu  %-58s  пример: %s\n", item.second.first, item.first.c_str(), item.second.second.c_str());
	// Выводим итог сравнения вердиктов
	::printf("\nШаблонов: %zu, принято обеими: %zu, расхождений: %zu (%.3f%%)\n",
	 total, accepted, diverged, ((100.0 * static_cast <double> (diverged)) / static_cast <double> (total)));
	// Выводим результат выполнения проверки
	return 0;
}
