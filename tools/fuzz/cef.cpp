/**
 * @file cef.cpp
 * @date 2026-09-04
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
 * @brief Инструмент фаззинга кодека событий CEF — построение полуструктурированных записей
 *        с точечной порчей, подача их чтению целиком и кусками произвольного размера, укладка
 *        в дерево контейнера ABC и обратная сборка для поиска аварийных завершений, выходов
 *        за границы буфера и расхождений разбора
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
#include <cstdlib>
#include <cstdint>

/**
 * Подключаем заголовочный файл проекта
 */
#include <codec/cef/cef.hpp>
#include <sys/log.hpp>

/**
 * Используем стандартное пространство имён
 */
using namespace std;
using namespace awh::codec;

/**
 * @brief Пространство имён построений ворошителя
 *
 * @note Держится оно безымянным намеренно: имена свободных функций иначе сходятся с
 *       именами POSIX - так свободная truncate() валила сборку ворошителя на всех трёх
 *       BSD, оставаясь незамеченной на рабочей машине
 *
 */
namespace {
	/**
	 * @brief Событие разбора, ворошителем удерживаемое
	 *
	 * @details Удерживается копия содержимого, а не вид на хранилище разбора: вид
	 *          живёт лишь до следующего события, и сличение двух прогонов по видам
	 *          отдавало бы содержимое чужой записи
	 *
	 */
	struct Event {
		// Вид события разбора
		uint8_t kind;
		// Имя ключа пары расширения
		string key;
		// Значение события разбора
		string value;
	};

	/**
	 * @brief Итоги работы ворошителя
	 *
	 */
	struct Totals {
		// Количество построенных записей
		uint64_t records;
		// Количество испорченных записей
		uint64_t corrupted;
		// Количество выданных событий разбора
		uint64_t events;
		// Количество записей, разобранных до конца
		uint64_t survived;
		// Количество записей, уложенных в дерево
		uint64_t trees;
		// Количество записей, собранных обратно
		uint64_t rewrites;
		// Количество записей, обратный оборот выдержавших
		uint64_t mirrored;
		/**
		 * @brief Конструктор
		 *
		 */
		Totals() noexcept :
		 records(0), corrupted(0), events(0), survived(0), trees(0), rewrites(0), mirrored(0) {}
	};

	// Итоги работы ворошителя
	Totals totals;

	/**
	 * @brief Объект журнала ворошителя с отключённым выводом
	 *
	 */
	struct Silent {
		// Объект фреймворка ворошителя
		awh::fmk_t fmk;
		// Объект журнала ворошителя
		awh::log_t log;
		/**
		 * @brief Конструктор
		 *
		 */
		Silent() noexcept : log(&this->fmk) {
			// Выполняем отключение вывода логов
			this->log.mode({});
		}
	};

	/**
	 * @brief Функция получения объекта окружения ворошителя
	 *
	 * @return объект окружения ворошителя
	 *
	 */
	Silent & environment() noexcept {
		// Объект окружения ворошителя
		static Silent silent;
		// Выводим объект окружения ворошителя
		return silent;
	}

	/**
	 * @brief Функция построения куска текста произвольного вида
	 *
	 * @param engine источник псевдослучайных чисел
	 * @param length длина строимого куска текста
	 * @return       построенный кусок текста
	 *
	 */
	string chunk(mt19937 & engine, const size_t length) noexcept {
		// Набор знаков, из которых строится кусок текста
		static const char alphabet[] = "abcXYZ0129._-:/ \\|=\"'\t\n\r@%+~[]{}(),;&$#!?*";
		// Строимый кусок текста
		string result;
		// Выделяем память под строимый кусок текста
		result.reserve(length);
		/**
		 * Выполняем построение куска текста нужной длины
		 */
		for(size_t i = 0; i < length; i++)
			// Добавляем очередной знак в строимый кусок текста
			result.append(1, alphabet[engine() % (sizeof(alphabet) - 1)]);
		// Выводим построенный кусок текста
		return result;
	}

	/**
	 * @brief Функция построения записи CEF полуструктурированного вида
	 *
	 * @details Запись строится ПРАВИЛЬНОЙ, а порча наводится следом отдельным ходом:
	 *          построение сразу испорченного даёт текст, до разбора не доходящий вовсе,
	 *          и пути разбора остаются нехожеными
	 *
	 * @param engine источник псевдослучайных чисел
	 * @return       построенная запись CEF
	 *
	 */
	string build(mt19937 & engine) noexcept {
		// Строимая запись CEF
		string result;
		/**
		 * Если записи предшествует приставка syslog
		 */
		if((engine() % 3) == 0){
			// Добавляем приставку syslog в строимую запись
			result.append(chunk(engine, 1 + (engine() % 24)));
			// Отделяем приставку syslog от заголовка пробелом
			result.append(1, ' ');
		}
		// Добавляем слово, заголовок записи открывающее
		result.append("CEF:");
		// Добавляем номер редакции записи
		result.append(::std::to_string(engine() % 3));
		// Отделяем номер редакции от следующего поля прямой чертой
		result.append(1, '|');
		/**
		 * Выполняем построение полей заголовка, за номером редакции следующих
		 */
		for(uint32_t i = 1; i < cef::HEADER_FIELDS; i++){
			// Добавляем очередное поле заголовка в строимую запись
			result.append(chunk(engine, engine() % 12));
			// Отделяем поле заголовка от следующего прямой чертой
			result.append(1, '|');
		}
		// Получаем количество пар расширения строимой записи
		const size_t count = (engine() % 12);
		/**
		 * Выполняем построение пар расширения записи
		 */
		for(size_t i = 0; i < count; i++){
			/**
			 * Если пара расширения не первая
			 */
			if(i > 0)
				// Отделяем пару расширения от предыдущей пробелом
				result.append(1, ' ');
			/**
			 * Определяем вид строимой пары расширения
			 */
			switch(engine() % 5){
				// Если строится пара ключа, словарю известного
				case 0: {
					// Получаем запись словаря расширений по её порядковому номеру
					const cef::entry_t * entry = cef::dictionary::at(engine() % cef::dictionary::size());
					// Добавляем ключ, словарю известный, в строимую запись
					result.append(entry->key);
				} break;
				// Если строится пара ключа с меткой имени
				case 1: {
					// Добавляем ключ с меткой имени в строимую запись
					result.append("cs").append(::std::to_string(1 + (engine() % 6)));
					/**
					 * Если ключ несёт метку имени
					 */
					if((engine() % 2) == 0)
						// Добавляем окончание метки имени в строимую запись
						result.append(cef::LABEL_SUFFIX);
				} break;
				// Если строится пара ключа с точкой в имени
				case 2: result.append("ad.").append(chunk(engine, 1 + (engine() % 6))); break;
				// Если строится пара ключа произвольного вида
				default: result.append(chunk(engine, 1 + (engine() % 8)));
			}
			// Отделяем имя ключа от значения знаком равенства
			result.append(1, '=');
			/**
			 * Если пара расширения несёт значение
			 */
			if((engine() % 4) > 0)
				// Добавляем значение пары расширения в строимую запись
				result.append(chunk(engine, engine() % 24));
		}
		// Выводим построенную запись CEF
		return result;
	}

	/**
	 * @brief Функция наведения точечной порчи на записи
	 *
	 * @param engine источник псевдослучайных чисел
	 * @param text   запись, порче подлежащая
	 * @return       признак наведения порчи
	 *
	 */
	bool corrupt(mt19937 & engine, string & text) noexcept {
		/**
		 * Если порча на запись не наводится
		 */
		if(text.empty() || ((engine() % 3) == 0))
			// Выводим отсутствие наведённой порчи
			return false;
		// Получаем количество наводимых порч
		const size_t count = (1 + (engine() % 3));
		/**
		 * Выполняем наведение порчи на запись
		 */
		for(size_t i = 0; i < count; i++){
			// Получаем место наводимой порчи в записи
			const size_t place = (engine() % text.size());
			/**
			 * Определяем вид наводимой порчи
			 */
			switch(engine() % 6){
				// Если знак записи замещается иным
				case 0: text[place] = static_cast <char> (engine() % 256); break;
				// Если знак записи сносится
				case 1: text.erase(place, 1); break;
				// Если в запись вставляется знак отмены
				case 2: text.insert(place, 1, '\\'); break;
				// Если в запись вставляется разделитель поля заголовка
				case 3: text.insert(place, 1, '|'); break;
				// Если в запись вставляется разделитель ключа и значения
				case 4: text.insert(place, 1, '='); break;
				// Если запись обрывается на середине
				default: text.erase(place);
			}
			/**
			 * Если запись порчей исчерпана
			 */
			if(text.empty())
				// Выходим из цикла наведения порчи
				break;
		}
		// Выводим признак наведения порчи
		return true;
	}

	/**
	 * @brief Функция подачи записи потоковому чтению
	 *
	 * @param text   подаваемая запись CEF
	 * @param step   размер куска подаваемой записи, нулевой для подачи целиком
	 * @param events перечень выданных разбором событий
	 * @param code   код отказа разбора
	 * @return       состояние чтения по исчерпании записи
	 *
	 */
	cef::state_t consume(const string & text, const size_t step, vector <Event> & events, cef::error_t & code) noexcept {
		// Объект потокового чтения записей
		cef::reader_t reader(&environment().fmk, &environment().log);
		// Смещение подачи записи
		size_t offset = 0;
		/**
		 * Выполняем подачу записи, пока она не исчерпана
		 */
		do {
			// Получаем размер очередного куска подаваемой записи
			const size_t size = ((step == 0) ? (text.size() - offset) : ::std::min(step, text.size() - offset));
			// Выполняем подачу очередного куска записи
			reader.feed(text.data() + offset, size, (offset + size) >= text.size());
			// Сдвигаем смещение подачи записи
			offset += size;
			/**
			 * Выполняем перебор всех событий разбора
			 */
			while(reader.next()){
				// Создаём очередное событие разбора
				events.emplace_back();
				// Запоминаем вид события разбора
				events.back().kind = static_cast <uint8_t> (reader.event());
				// Запоминаем имя ключа пары расширения
				events.back().key = reader.key();
				// Запоминаем значение события разбора
				events.back().value = reader.value();
				// Наращиваем количество выданных событий разбора
				totals.events++;
			}
			/**
			 * Если разбор прекращён ошибкой
			 */
			if(reader.state() == cef::state_t::FAILED)
				// Выходим из цикла подачи записи
				break;
		} while(offset < text.size());
		// Запоминаем код отказа разбора
		code = reader.error();
		// Выводим состояние чтения по исчерпании записи
		return reader.state();
	}

	/**
	 * @brief Функция сличения перечней выданных разбором событий
	 *
	 * @param narrow перечень событий подачи записи целиком
	 * @param broad  перечень событий подачи записи кусками
	 * @param step   размер куска подаваемой записи
	 * @param text   разбираемая запись CEF
	 * @return       признак совпадения перечней событий
	 *
	 */
	bool compare(const vector <Event> & narrow, const vector <Event> & broad, const size_t step, const string & text) noexcept {
		/**
		 * Если количества выданных событий разошлись
		 */
		if(narrow.size() != broad.size()){
			// Выводим сообщение о расхождении количества событий
			::fprintf(
				stderr, "cef fuzz: chunk=%zu events differ: %zu/%zu\n",
				step, narrow.size(), broad.size()
			);
			// Выводим разбираемую запись
			::fprintf(stderr, "cef fuzz: record: %s\n", text.c_str());
			// Выводим отсутствие совпадения перечней событий
			return false;
		}
		/**
		 * Выполняем перебор всех выданных разбором событий
		 */
		for(size_t i = 0; i < narrow.size(); i++){
			/**
			 * Если очередные события разошлись
			 */
			if((narrow.at(i).kind != broad.at(i).kind) ||
			   (narrow.at(i).key != broad.at(i).key) ||
			   (narrow.at(i).value != broad.at(i).value)){
				// Выводим сообщение о расхождении события разбора
				::fprintf(
					stderr, "cef fuzz: chunk=%zu event %zu differs: %u{%s=%s} / %u{%s=%s}\n",
					step, i, (unsigned) narrow.at(i).kind, narrow.at(i).key.c_str(), narrow.at(i).value.c_str(),
					(unsigned) broad.at(i).kind, broad.at(i).key.c_str(), broad.at(i).value.c_str()
				);
				// Выводим разбираемую запись
				::fprintf(stderr, "cef fuzz: record: %s\n", text.c_str());
				// Выводим отсутствие совпадения перечней событий
				return false;
			}
		}
		// Выводим совпадение перечней событий
		return true;
	}

	/**
	 * @brief Функция проверки укладки записи в дерево и обратной сборки
	 *
	 * @details Обратимость поверяется сличением ДЕРЕВЬЕВ, а не текстов: дословного
	 *          совпадения записи перевод не обещает - обещает значение
	 *
	 * @param text     разбираемая запись CEF
	 * @param settings настройки разбора записей
	 * @return         признак успешности проверки
	 *
	 */
	bool tree(const string & text, const cef::reader_t::settings_t & settings) noexcept {
		// Объект события CEF
		cef::document_t doc(&environment().fmk, &environment().log);
		// Устанавливаем настройки разбора записей
		doc.settings(settings);
		/**
		 * Если укладка записи в дерево отказом завершилась
		 */
		if(!doc.parse(text))
			// Выводим успешность проверки: отказ разбора есть законный исход
			return true;
		// Наращиваем количество записей, уложенных в дерево
		totals.trees++;
		/**
		 * Выполняем обход всех звеньев пути расширения записи
		 */
		for(const auto & key : doc.keys("/extension")){
			/**
			 * Если звено пути к потомку не ведёт
			 */
			if(!doc.has("/extension/" + key)){
				// Выводим сообщение о разомкнутости обхода дерева
				::fprintf(stderr, "cef fuzz: traversal broken at link \"%s\"\n", key.c_str());
				// Выводим разбираемую запись
				::fprintf(stderr, "cef fuzz: record: %s\n", text.c_str());
				// Выводим неуспешность проверки
				return false;
			}
		}
		// Выполняем сборку записи CEF из дерева события
		const string rewritten = doc.dump();
		/**
		 * Если сборка записи отказом завершилась
		 */
		if(rewritten.empty())
			// Выводим успешность проверки: отказ сборки есть законный исход
			return true;
		// Наращиваем количество записей, собранных обратно
		totals.rewrites++;
		// Объект события CEF повторного разбора
		cef::document_t again(&environment().fmk, &environment().log);
		// Устанавливаем настройки разбора записей
		again.settings(settings);
		/**
		 * Если повторный разбор собранной записи отказом завершился
		 */
		if(!again.parse(rewritten)){
			// Выводим сообщение о неразбираемости собранной записи
			::fprintf(stderr, "cef fuzz: rewritten record is not parsable: %s\n", cef::message(again.error()));
			// Выводим собранную запись
			::fprintf(stderr, "cef fuzz: rewritten: %s\n", rewritten.c_str());
			// Выводим исходную запись
			::fprintf(stderr, "cef fuzz: record: %s\n", text.c_str());
			// Выводим неуспешность проверки
			return false;
		}
		/**
		 * Если деревья разбора разошлись
		 */
		if(doc.root().dump() != again.root().dump()){
			// Выводим сообщение о расхождении деревьев разбора
			::fprintf(stderr, "cef fuzz: trees differ after the round trip\n");
			// Выводим собранную запись
			::fprintf(stderr, "cef fuzz: rewritten: %s\n", rewritten.c_str());
			// Выводим исходную запись
			::fprintf(stderr, "cef fuzz: record: %s\n", text.c_str());
			// Выводим неуспешность проверки
			return false;
		}
		// Наращиваем количество записей, обратный оборот выдержавших
		totals.mirrored++;
		// Выводим успешность проверки
		return true;
	}
}

/**
 * @brief Признак сборки под надзирателями памяти
 *
 * @details Чистая кампания без надзирателей доказывает равенство поведения, а не
 *          чистоту памяти, и разницу эту надо видеть в самой строке отчёта
 *
 */
#if defined(__has_feature)
	#if __has_feature(address_sanitizer)
		#define AWH_FUZZ_SANITIZED 1
	#endif
#endif
#if !defined(AWH_FUZZ_SANITIZED) && defined(__SANITIZE_ADDRESS__)
	#define AWH_FUZZ_SANITIZED 1
#endif
#if !defined(AWH_FUZZ_SANITIZED)
	#define AWH_FUZZ_SANITIZED 0
#endif

/**
 * @brief Функция запуска приложения
 *
 * @param argc длина массива параметров
 * @param argv массив параметров
 * @return     код выхода из приложения
 *
 */
int32_t main(int32_t argc, char * argv[]) noexcept {
	// Количество выполняемых проходов генератора
	uint64_t count = 3000;
	/**
	 * Если количество проходов задано параметром командной строки
	 */
	if(argc > 1)
		// Выполняем чтение количества проходов из параметра командной строки
		count = static_cast <uint64_t> (::strtoull(argv[1], nullptr, 10));
	// Зерно источника псевдослучайных чисел
	uint32_t seed = 0x5A1CE;
	/**
	 * Если зерно задано вторым параметром командной строки
	 *
	 * @note Зерно закреплено по умолчанию: прогон обязан воспроизводиться
	 */
	if(argc > 2)
		// Выполняем чтение зерна из параметра командной строки
		seed = static_cast <uint32_t> (::strtoul(argv[2], nullptr, 10));
	// Создаём источник псевдослучайных чисел с закреплённым зерном
	mt19937 engine(seed);
	/**
	 * Выполняем проходы генератора
	 */
	for(uint64_t i = 0; i < count; i++){
		// Выполняем построение очередной записи CEF
		string text = build(engine);
		// Наращиваем количество построенных записей
		totals.records++;
		/**
		 * Если на запись наведена порча
		 */
		if(corrupt(engine, text))
			// Наращиваем количество испорченных записей
			totals.corrupted++;
		// Настройки разбора записей
		cef::reader_t::settings_t settings;
		// Выбираем строгость сличения ключей расширения со словарём
		settings.mode = static_cast <cef::mode_t> (engine() % 4);
		// Выбираем обращение с пустым значением расширения
		settings.empty = static_cast <cef::empty_t> (engine() % 4);
		// Выбираем признание приставки syslog перед словом «CEF:»
		settings.syslog = ((engine() % 4) > 0);
		// Выбираем снятие отмены знаков со значений
		settings.unescape = ((engine() % 4) > 0);
		// Перечень событий подачи записи целиком
		vector <Event> narrow;
		// Код отказа разбора записи, поданной целиком
		cef::error_t first = cef::error_t::NONE;
		// Выполняем подачу записи целиком
		const cef::state_t reached = consume(text, 0, narrow, first);
		/**
		 * Если запись разобрана до конца
		 */
		if(reached == cef::state_t::FINISHED)
			// Наращиваем количество записей, разобранных до конца
			totals.survived++;
		/**
		 * Выполняем перебор размеров куска подачи
		 */
		for(const size_t step : {static_cast <size_t> (1), static_cast <size_t> (2), static_cast <size_t> (3), static_cast <size_t> (7)}){
			// Перечень событий подачи записи кусками
			vector <Event> broad;
			// Код отказа разбора записи, поданной кусками
			cef::error_t second = cef::error_t::NONE;
			// Выполняем подачу записи кусками
			const cef::state_t arrived = consume(text, step, broad, second);
			/**
			 * Если перечни выданных разбором событий разошлись
			 */
			if(!compare(narrow, broad, step, text))
				// Выходим из приложения с кодом ошибки
				return EXIT_FAILURE;
			/**
			 * Если итоги разбора разошлись
			 */
			if((arrived != reached) || (first != second)){
				// Выводим сообщение о расхождении итога разбора
				::fprintf(
					stderr, "cef fuzz: chunk=%zu outcome differs: state %u/%u error %u/%u\n",
					step, (unsigned) reached, (unsigned) arrived, (unsigned) first, (unsigned) second
				);
				// Выводим разбираемую запись
				::fprintf(stderr, "cef fuzz: record: %s\n", text.c_str());
				// Выходим из приложения с кодом ошибки
				return EXIT_FAILURE;
			}
		}
		/**
		 * Если проверка дерева события не удалась
		 */
		if(!tree(text, settings))
			// Выходим из приложения с кодом ошибки
			return EXIT_FAILURE;
	}
	// Выводим статистику работы генератора
	::fprintf(
		stdout,
		"cef fuzz: %llu records (%llu corrupted), %llu events, %llu parsed to the end, %llu trees, %llu rewrites, %llu mirrored%s\n",
		static_cast <unsigned long long> (totals.records),
		static_cast <unsigned long long> (totals.corrupted),
		static_cast <unsigned long long> (totals.events),
		static_cast <unsigned long long> (totals.survived),
		static_cast <unsigned long long> (totals.trees),
		static_cast <unsigned long long> (totals.rewrites),
		static_cast <unsigned long long> (totals.mirrored),
		(AWH_FUZZ_SANITIZED ? "" : " [БЕЗ НАДЗИРАТЕЛЕЙ]")
	);
	// Выходим из приложения
	return EXIT_SUCCESS;
}
