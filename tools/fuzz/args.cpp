/**
 * @file args.cpp
 * @date 2026-09-03
 *
 * @license{LicenseRef-AWH-1.0}
 *
 * @author Yuriy Lobarev
 *
 * @telegram{forman}
 * @phone{+7 (910) 983-95-90}
 *
 * @email forman@anyks.com
 * @site https://anyks.com
 *
 * @brief Инструмент фаззинга разбора параметров запуска — построение наборов доводов
 *        с точечной порчей, подача их разборщику и проверка обратимости разреза текста,
 *        совпадения разбора набора с разбором текстового потока и полноты учёта доводов
 *
 * @details Проверяются три договора, обязанные держаться на ЛЮБОМ вводе, включая
 *          испорченный, - оттого находка на испорченной записи здесь дефектом служит
 *          наравне с находкой на правильной, в отличие от ворошителей кодеков:
 *
 *          1. ОБРАТИМОСТЬ РАЗРЕЗА. Слова, собранные в текст с оградою, разрезаются
 *             обратно в те же самые слова. Ограда одинарными кавычками с обратной
 *             косой перед кавычкою и косою - единственная запись, годная всякому слову
 *
 *          2. СОВПАДЕНИЕ ВХОДОВ. Набор доводов и тот же набор, поданный текстом,
 *             дают одни и те же лексемы - вид, признак наличия значения, имя и
 *             значение. Договор этот держится устройством: текст режется на слова и
 *             подаётся ТОМУ ЖЕ разборщику, - и ворошитель стережёт устройство
 *
 *          3. ПОЛНОТА УЧЁТА. Всякий довод набора учтён ровно раз: он либо стал
 *             лексемой, либо съеден значением предыдущей, либо отвечен отказом.
 *             Довод, пропавший молча, есть худший вид дефекта разбора
 *
 *          4. СОГЛАСИЕ СКЛЕЙКИ С ОПИСАНИЕМ. Разбор склейки коротких имён либо
 *             отвечает отказом, оставляя выдачу пустой, либо выдаёт РОВНО столько
 *             длинных имён, сколько знаков в склейке, и каждое из них разыскивается
 *             по своему знаку. Разбор наполовину здесь хуже отказа: запись «-abc»
 *             неотличима от длинного имени под одним тире
 *
 * @warning Порча здесь бьёт по СЛОВАМ, а не по собранному тексту: испорченный текст
 *          проверял бы разрез против самого себя, и обратимость на нём не обещана
 *          вовсе - незакрытая кавычка есть законный отказ, а не находка
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Стандартные заголовочные файлы
 */
#include <string>
#include <vector>
#include <random>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <cstdint>

/**
 * Подключаем заголовочные файлы проекта
 */
#include <sys/fmk.hpp>
#include <sys/log.hpp>
#include <args/lexer.hpp>
#include <args/schema.hpp>

/**
 * @brief Средства заведения молчащего журнала работы (внутренняя компоновка)
 *
 */
namespace {
	/**
	 * @brief Объект молчащего журнала работы
	 *
	 */
	struct Silent {
		/**
		 * @brief Функция получения объекта фреймворка
		 *
		 * @return объект фреймворка
		 *
		 */
		static awh::fmk_t & framework() noexcept {
			// Объект фреймворка
			static awh::fmk_t fmk;
			// Выводим объект фреймворка
			return fmk;
		}
		// Объект журнала работы
		awh::log_t log;
		/**
		 * @brief Конструктор
		 *
		 */
		Silent() noexcept : log(&Silent::framework()) {
			// Выполняем отключение вывода журнала
			this->log.mode({});
		}
	};
	/**
	 * @brief Функция получения объекта фреймворка
	 *
	 * @return объект фреймворка
	 *
	 */
	awh::fmk_t * framework() noexcept {
		// Выводим объект фреймворка
		return &Silent::framework();
	}
	/**
	 * @brief Функция получения объекта журнала работы
	 *
	 * @return объект журнала работы
	 *
	 */
	awh::log_t * logger() noexcept {
		// Объект журнала работы
		static Silent silent;
		// Выводим объект журнала работы
		return &silent.log;
	}
}

/**
 * Используем стандартное пространство имён
 */
using namespace std;

/**
 * @brief Внутренние вспомогательные средства генератора (внутренняя компоновка)
 *
 */
namespace {
	/**
	 * @brief Учёт проделанной работы
	 *
	 */
	struct Statistic {
		// Количество построенных наборов доводов
		uint64_t records;
		// Количество испорченных наборов доводов
		uint64_t corrupted;
		// Число найденных расхождений договора
		uint64_t findings;
		// Количество наборов, разбор переживших
		uint64_t parsed;
		// Количество проверок обратимости разреза текста
		uint64_t splits;
		// Количество проверок совпадения разбора набора с разбором текста
		uint64_t matched;
		// Количество проверок полноты учёта доводов
		uint64_t counted;
		// Количество выданных лексем разбора
		uint64_t lexemes;
		// Количество проверок согласия склейки коротких имён с описанием
		uint64_t clusters;
		// Количество склеек, разобранных описанием успешно
		uint64_t clustered;
		/**
		 * @brief Конструктор
		 *
		 */
		Statistic() noexcept :
		 records(0), corrupted(0), findings(0), parsed(0),
		 splits(0), matched(0), counted(0), lexemes(0),
		 clusters(0), clustered(0) {}
	};

	/**
	 * @brief Снимок разобранной лексемы, содержимое копирующий
	 *
	 * @details Лексема разбора ссылается на поданный набор и живёт не дольше его,
	 *          оттого сличение ведётся снимками, а не самими лексемами
	 *
	 */
	struct Shot {
		// Вид лексемы разбора
		awh::args::token_t type;
		// Признак наличия значения у именованного параметра
		bool assigned;
		// Имя параметра без ведущих тире
		string key;
		// Значение параметра либо содержимое позиционного довода
		string value;
		/**
		 * @brief Конструктор
		 *
		 * @param lexeme лексема разбора для снятия снимка
		 *
		 */
		Shot(const awh::args::lexeme_t & lexeme) noexcept :
		 type(lexeme.type), assigned(lexeme.assigned), key(lexeme.key), value(lexeme.value) {}
		/**
		 * @brief Оператор сличения снимков лексем
		 *
		 * @param shot снимок для сличения
		 * @return     результат сличения
		 *
		 */
		bool operator != (const Shot & shot) const noexcept {
			// Выводим признак расхождения снимков лексем
			return ((this->type != shot.type) || (this->assigned != shot.assigned) ||
			        (this->key != shot.key) || (this->value != shot.value));
		}
	};

	/**
	 * @brief Функция сборки одного довода набора запуска
	 *
	 * @param engine источник случайных чисел
	 * @return       собранный довод набора запуска
	 *
	 */
	string buildWord(mt19937_64 & engine) noexcept {
		// Собираемый довод набора запуска
		string result = "";
		// Определяем вид собираемого довода
		switch(static_cast <uint32_t> (engine() % 8)){
			// Собираем именованный параметр со значением через знак равенства
			case 0: result.append("--"); break;
			// Собираем именованный параметр под одним тире
			case 1: result.append("-"); break;
			// Собираем признак конца именованных параметров
			case 2: return "--";
			// Собираем довод, видом схожий с числом
			case 3: return "-" + to_string(static_cast <int64_t> (engine() % 1000));
			// Собираем пустой довод
			case 4: return "";
		}
		// Получаем длину собираемой части довода
		const size_t length = (1 + (engine() % 12));
		// Выполняем сборку части довода
		for(size_t i = 0; i < length; i++){
			/**
			 * Знаки берутся из набора, где нарочно есть разделитель имени со
			 * значением, разделитель звеньев пути, кавычки, пробел и обратная косая:
			 * ими и ломается разбор, если он где-то их не ждёт
			 */
			static const char alphabet[] = "abcXYZ019.=_-/ '\"\\\t";
			// Добавляем к доводу очередной знак
			result.append(1, alphabet[engine() % (sizeof(alphabet) - 1)]);
		}
		// Если довод собирается со значением
		if((engine() % 2) == 0){
			// Добавляем к доводу разделитель имени со значением
			result.append(1, '=');
			// Получаем длину собираемого значения
			const size_t size = (engine() % 8);
			// Выполняем сборку значения довода
			for(size_t i = 0; i < size; i++){
				// Знаки значения берутся из того же набора
				static const char alphabet[] = "abc019 '\"\\=";
				// Добавляем к значению очередной знак
				result.append(1, alphabet[engine() % (sizeof(alphabet) - 1)]);
			}
		}
		// Выводим собранный довод набора запуска
		return result;
	}

	/**
	 * @brief Функция порчи собранного набора доводов
	 *
	 * @details Порча бьёт по СЛОВАМ, а не по собранному тексту: испорченный текст
	 *          проверял бы разрез против самого себя
	 *
	 * @param engine источник случайных чисел
	 * @param items  набор доводов для порчи
	 * @param damage доля порчи, нуль запрещает порчу вовсе
	 * @return       признак того, что набор был испорчен
	 *
	 */
	bool corrupt(mt19937_64 & engine, vector <string> & items, const uint32_t damage) noexcept {
		// Если порча запрещена либо портить нечего
		if((damage == 0) || items.empty() || ((engine() % damage) != 0))
			// Выводим признак того, что набор не портился
			return false;
		// Получаем номер портимого довода набора
		const size_t index = (engine() % items.size());
		// Получаем портимый довод набора
		string & item = items.at(index);
		// Определяем вид порчи довода
		switch(static_cast <uint32_t> (engine() % 4)){
			// Добавляем к доводу знак, взятый наугад
			case 0: item.append(1, static_cast <char> (1 + (engine() % 255))); break;
			// Снимаем у довода последний знак
			case 1: if(!item.empty()) item.pop_back(); break;
			// Заменяем довод одними лишь тире
			case 2: item.assign(1 + (engine() % 4), '-'); break;
			// Вставляем в довод восьмеричный нуль
			case 3: item.insert(item.empty() ? 0 : (engine() % item.size()), 1, '\0'); break;
		}
		// Выводим признак того, что набор был испорчен
		return true;
	}

	/**
	 * @brief Функция сборки описания ожидаемых параметров
	 *
	 * @details Описание собирается из знаков заведомо известного набора, а склейка -
	 *          из знаков как известных, так и посторонних: разбор обязан отвергать
	 *          вторые целиком, а не разбирать запись наполовину
	 *
	 * @param engine источник случайных чисел
	 * @param schema собираемое описание ожидаемых параметров
	 *
	 */
	void buildSchema(mt19937_64 & engine, awh::args::schema_t & schema) noexcept {
		// Выполняем очистку собираемого описания
		schema.clear();
		// Набор знаков, отводимых коротким именам описания
		static const char alphabet[] = "abcdef";
		// Выполняем перебор всех знаков набора
		for(size_t i = 0; i < (sizeof(alphabet) - 1); i++){
			/**
			 * Потребность в значении берётся наугад: склейка обязана отвергаться
			 * целиком, если хоть один знак её значения требует
			 */
			const awh::args::schema_t::value_t value = (((engine() % 4) == 0) ?
			 awh::args::schema_t::value_t::REQUIRED : awh::args::schema_t::value_t::NONE);
			// Выполняем заведение описания ожидаемого параметра
			static_cast <void> (schema.add(string("имя-") + alphabet[i], alphabet[i], value));
		}
	}

	/**
	 * @brief Функция сборки склейки коротких имён
	 *
	 * @param engine источник случайных чисел
	 * @return       собранная склейка коротких имён
	 *
	 */
	string buildCluster(mt19937_64 & engine) noexcept {
		// Собираемая склейка коротких имён
		string result = "";
		// Получаем длину собираемой склейки
		const size_t length = (1 + (engine() % 6));
		// Выполняем сборку склейки коротких имён
		for(size_t i = 0; i < length; i++){
			// Знаки берутся как известные описанию, так и посторонние
			static const char alphabet[] = "abcdefxyz09-";
			// Добавляем к склейке очередной знак
			result.append(1, alphabet[engine() % (sizeof(alphabet) - 1)]);
		}
		// Выводим собранную склейку коротких имён
		return result;
	}

	/**
	 * @brief Функция сборки текста из набора доводов
	 *
	 * @details Всякое слово берётся в ограду одинарными кавычками, а кавычка и
	 *          обратная косая внутри его предваряются косою: запись эта годна всякому
	 *          слову, включая пустое, и разрез обязан вернуть слова в точности
	 *
	 * @param items набор доводов для сборки
	 * @return      собранный текст
	 *
	 */
	string buildText(const vector <string> & items) noexcept {
		// Собираемый текст набора доводов
		string result = "";
		// Выполняем перебор всех доводов набора
		for(size_t i = 0; i < items.size(); i++){
			// Если довод не первый в наборе
			if(i > 0)
				// Добавляем к тексту разделитель слов
				result.append(1, ' ');
			// Добавляем к тексту открывающую ограду слова
			result.append(1, '\'');
			// Выполняем перебор всех знаков довода
			for(auto & letter : items.at(i)){
				// Если знаком является кавычка ограды либо обратная косая
				if((letter == '\'') || (letter == '\\'))
					// Добавляем к тексту обратную косую перед знаком
					result.append(1, '\\');
				// Добавляем к тексту знак довода
				result.append(1, letter);
			}
			// Добавляем к тексту закрывающую ограду слова
			result.append(1, '\'');
		}
		// Выводим собранный текст набора доводов
		return result;
	}
}

/**
 * @brief Точка входа в приложение
 *
 * @param argc количество доводов запуска
 * @param argv набор доводов запуска
 * @return     код выхода из приложения
 *
 */
int main(int argc, char * argv[]) noexcept {
	// Получаем количество проходов генератора
	const uint64_t count = ((argc > 1) ? static_cast <uint64_t> (::atoll(argv[1])) : 3000);
	// Получаем зерно источника случайных чисел
	const uint64_t seed = ((argc > 2) ? static_cast <uint64_t> (::atoll(argv[2])) : 20260903);
	// Доля порчи наборов: единица из скольких наборов портится, нуль запрещает порчу
	const uint32_t damage = ((argc > 3) ? static_cast <uint32_t> (::atoll(argv[3])) : 3);
	// Признак остановки на первой же находке
	const bool halt = ((argc > 4) ? (::atoll(argv[4]) != 0) : true);
	// Источник случайных чисел
	mt19937_64 engine(seed);
	// Учёт проделанной работы
	Statistic totals;
	// Создаём разборщик параметров запуска
	const awh::args::lexer_t lexer(::framework(), ::logger());
	// Создаём описание ожидаемых параметров запуска
	awh::args::schema_t schema(::framework(), ::logger());
	/**
	 * Выполняем проходы генератора
	 */
	for(uint64_t pass = 0; pass < count; pass++){
		// Собираемый набор доводов запуска
		vector <string> items;
		// Получаем число доводов собираемого набора
		const size_t length = (engine() % 10);
		// Выполняем сборку набора доводов запуска
		for(size_t i = 0; i < length; i++)
			// Добавляем в набор очередной собранный довод
			items.push_back(::buildWord(engine));
		// Увеличиваем счёт построенных наборов доводов
		totals.records++;
		// Если набор доводов был испорчен
		if(::corrupt(engine, items, damage))
			// Увеличиваем счёт испорченных наборов доводов
			totals.corrupted++;
		// Снимки лексем, разобранных из набора доводов
		vector <Shot> shots;
		// Число отказов разбора набора доводов
		size_t refusals = 0;
		// Число доводов, съеденных значениями предыдущих лексем
		size_t eaten = 0;
		// Выполняем разбор собранного набора доводов
		static_cast <void> (lexer.parse(items, [&shots, &eaten](const awh::args::lexeme_t & lexeme) noexcept -> bool {
			// Выполняем снятие снимка разобранной лексемы
			shots.emplace_back(lexeme);
			/**
			 * Если значение параметра пришло СЛЕДУЮЩИМ доводом, оно съело довод набора
			 *
			 * @note Признаком того служит отсутствие разделителя в имени вместе с
			 *       поданным значением: запись «--name=value» съедает один довод, а
			 *       запись «--name value» - два
			 */
			if((lexeme.type == awh::args::token_t::PARAM) && lexeme.assigned &&
			   (lexeme.key.length() + lexeme.value.length() + 1) > 0){
				// Выполняем поиск разделителя имени со значением в исходном доводе
				const bool inside = (lexeme.value.data() >= lexeme.key.data()) &&
				                    (lexeme.value.data() <= (lexeme.key.data() + lexeme.key.length() + 1));
				// Если значение стояло отдельным доводом набора
				if(!inside)
					// Увеличиваем счёт съеденных доводов
					eaten++;
			}
			// Сообщаем, что разбор следует продолжить
			return true;
		}, [&refusals](const awh::args::error_t, const awh::args::location_t &) noexcept -> bool {
			// Увеличиваем счёт отказов разбора
			refusals++;
			// Сообщаем, что разбор следует продолжить
			return true;
		}));
		// Увеличиваем счёт наборов, разбор переживших
		totals.parsed++;
		// Увеличиваем счёт выданных лексем разбора
		totals.lexemes += shots.size();
		/**
		 * ДОГОВОР 1: полнота учёта доводов
		 *
		 * @details Всякий довод набора учтён ровно раз: он стал лексемой, был съеден
		 *          значением предыдущей либо отвечен отказом. Довод, пропавший молча,
		 *          есть худший вид дефекта разбора
		 */
		if((shots.size() + eaten + refusals) != items.size()){
			// Печатаем сведения о находке
			::fprintf(stderr,
				"НАХОДКА: учёт доводов не сошёлся\n  доводов=%zu лексем=%zu съедено=%zu отказов=%zu\n",
				items.size(), shots.size(), eaten, refusals);
			// Считаем находку
			totals.findings++;
			// Выходим из приложения с кодом ошибки, если остановка затребована
			if(halt)
				return EXIT_FAILURE;
		}
		// Увеличиваем счёт проверок полноты учёта доводов
		totals.counted++;
		// Выполняем сборку текста из набора доводов
		const string text = ::buildText(items);
		// Слова, собранные разрезом текста
		vector <string> words;
		// Выполняем разрез собранного текста на слова
		static_cast <void> (lexer.split(text, words));
		/**
		 * ДОГОВОР 2: обратимость разреза текста
		 *
		 * @details Слова, собранные в текст с оградою, разрезаются обратно в те же
		 *          самые слова - иначе подача настроек текстом означала бы не то, что
		 *          подача их набором
		 */
		if(words != items){
			// Печатаем сведения о находке
			::fprintf(stderr, "НАХОДКА: разрез текста необратим\n  текст=[%s]\n  слов=%zu доводов=%zu\n",
				text.c_str(), words.size(), items.size());
			// Считаем находку
			totals.findings++;
			// Выходим из приложения с кодом ошибки, если остановка затребована
			if(halt)
				return EXIT_FAILURE;
		}
		// Увеличиваем счёт проверок обратимости разреза текста
		totals.splits++;
		// Снимки лексем, разобранных из текстового потока
		vector <Shot> stream;
		// Выполняем разбор собранного текста разборщиком
		static_cast <void> (lexer.parse(text, [&stream](const awh::args::lexeme_t & lexeme) noexcept -> bool {
			// Выполняем снятие снимка разобранной лексемы
			stream.emplace_back(lexeme);
			// Сообщаем, что разбор следует продолжить
			return true;
		}));
		/**
		 * ДОГОВОР 3: совпадение обоих входов
		 *
		 * @details Набор доводов и тот же набор, поданный текстом, дают одни и те же
		 *          лексемы. Договор этот держится УСТРОЙСТВОМ - текст режется на слова
		 *          и подаётся тому же разборщику, - и ворошитель стережёт устройство
		 */
		{
			// Признак расхождения разбора обоих входов
			bool differs = (shots.size() != stream.size());
			// Выполняем перебор всех разобранных лексем
			for(size_t i = 0; !differs && (i < shots.size()); i++)
				// Выполняем сличение снимков лексем обоих входов
				differs = (shots.at(i) != stream.at(i));
			// Если разбор обоих входов разошёлся
			if(differs){
				// Печатаем сведения о находке
				::fprintf(stderr, "НАХОДКА: разбор набора разошёлся с разбором текста\n  текст=[%s]\n  лексем набора=%zu текста=%zu\n",
					text.c_str(), shots.size(), stream.size());
				// Считаем находку
				totals.findings++;
				// Выходим из приложения с кодом ошибки, если остановка затребована
				if(halt)
					return EXIT_FAILURE;
			}
		}
		// Увеличиваем счёт проверок совпадения разбора обоих входов
		totals.matched++;
		/**
		 * ДОГОВОР 4: согласие разбора склейки с описанием ожидаемых
		 */
		{
			// Выполняем сборку описания ожидаемых параметров
			::buildSchema(engine, schema);
			// Выполняем сборку склейки коротких имён
			const string cluster = ::buildCluster(engine);
			// Контейнер разобранных длинных имён
			vector <string> names;
			// Выполняем разбор собранной склейки коротких имён
			const bool parsed = schema.cluster(cluster, names);
			// Признак расхождения разбора склейки с описанием
			bool differs = false;
			// Если разбор склейки отвечен отказом
			if(!parsed)
				/**
				 * Выдача обязана быть ПУСТОЙ: разбор наполовину оставил бы потребителю
				 * часть имён от записи, какую разобрать не удалось
				 */
				differs = !names.empty();
			// Если разбор склейки выполнен
			else {
				// Увеличиваем счёт склеек, разобранных описанием успешно
				totals.clustered++;
				// Выдача обязана нести ровно столько имён, сколько знаков в склейке
				differs = (names.size() != cluster.length());
				// Выполняем перебор всех разобранных длинных имён
				for(size_t i = 0; !differs && (i < names.size()); i++){
					// Выполняем розыск описания по знаку склейки
					const awh::args::schema_t::param_t * param = schema.get(cluster.at(i));
					/**
					 * Имя обязано разыскиваться по СВОЕМУ знаку, а значения описание его
					 * требовать не должно: иначе значение досталось бы одному знаку из
					 * нескольких
					 */
					differs = ((param == nullptr) || (param->name != names.at(i)) ||
					           (param->value == awh::args::schema_t::value_t::REQUIRED));
				}
			}
			// Если разбор склейки разошёлся с описанием
			if(differs){
				// Печатаем сведения о находке
				::fprintf(stderr, "НАХОДКА: разбор склейки разошёлся с описанием\n  склейка=[%s] разобрано=%s имён=%zu\n",
					cluster.c_str(), (parsed ? "да" : "нет"), names.size());
				// Считаем находку
				totals.findings++;
				// Выходим из приложения с кодом ошибки, если остановка затребована
				if(halt)
					return EXIT_FAILURE;
			}
			// Увеличиваем счёт проверок согласия склейки с описанием
			totals.clusters++;
		}
	}
	// Выводим итоги проделанной работы
	::fprintf(stdout,
		"ЗЕРНО=%llu ПРОХОДОВ=%llu\n"
		"  наборов построено: %llu, из них испорчено: %llu\n"
		"  находок договора: %llu\n"
		"  разбор пережили: %llu\n"
		"  лексем выдано: %llu\n"
		"  проверок полноты учёта доводов: %llu\n"
		"  проверок обратимости разреза: %llu\n"
		"  проверок совпадения обоих входов: %llu\n"
		"  проверок согласия склейки с описанием: %llu, из них разобрано: %llu\n",
		static_cast <unsigned long long> (seed), static_cast <unsigned long long> (count),
		static_cast <unsigned long long> (totals.records), static_cast <unsigned long long> (totals.corrupted),
		static_cast <unsigned long long> (totals.findings),
		static_cast <unsigned long long> (totals.parsed), static_cast <unsigned long long> (totals.lexemes),
		static_cast <unsigned long long> (totals.counted), static_cast <unsigned long long> (totals.splits),
		static_cast <unsigned long long> (totals.matched),
		static_cast <unsigned long long> (totals.clusters), static_cast <unsigned long long> (totals.clustered));
	// Выводим успешный код выхода из приложения
	return EXIT_SUCCESS;
}
