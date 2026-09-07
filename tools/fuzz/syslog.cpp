/**
 * @file syslog.cpp
 * @date 2026-09-07
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
 * @brief Ворошитель контейнера SysLog — построения записей обоих описаний с наведением порчи,
 *        сличения разбора при разных нарезках текста и поверки оборота записи через дерево
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
 * Подключаем заголовочный файл проекта
 */
#include <codec/syslog/syslog.hpp>
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
	 * @details Удерживается КОПИЯ содержимого, а не вид на хранилище разбора: вид
	 *          живёт лишь до следующего события, и сличение двух прогонов по видам
	 *          отдавало бы содержимое чужой записи
	 *
	 */
	struct Event {
		// Вид события разбора
		uint8_t kind;
		// Поле заголовка, событием выданное
		uint8_t field;
		// Имя ключа события разбора
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
		// Количество записей, оборот выдержавших
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
	 * @brief Объект окружения ворошителя с отключённым выводом журнала
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
		Silent() noexcept : log(&fmk) {
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
		/**
		 * Набор знаков, из которых строится кусок текста
		 *
		 * @note В набор нарочно входят знаки, разбору значимые: пробел разделяет поля,
		 *       квадратные скобки ограждают блоки данных, кавычка ограждает значение,
		 *       обратная косая отменяет знаки, угловые скобки открывают приставку
		 *       приоритета, а двоеточие закрывает метку приложения
		 */
		static const char alphabet[] = "abcXYZ0129._-:/ \\|=\"'\t@%+~[]{}(),;&$#!?*<>";
		// Строимый кусок текста
		string result;
		// Выделяем память под строимый кусок текста
		result.reserve(length);
		/**
		 * Выполняем построение куска текста заданной длины
		 */
		for(size_t i = 0; i < length; i++)
			// Добавляем очередной знак в строимый кусок текста
			result.append(1, alphabet[engine() % (sizeof(alphabet) - 1)]);
		// Выводим построенный кусок текста
		return result;
	}

	/**
	 * @brief Функция построения даты сообщения произвольного вида
	 *
	 * @details Виды даты перебираются все, какие живые устройства пишут: два вида BSD,
	 *          запись asctime с годом и без, четыре вида RFC 3339 и запись с пробелом
	 *          вместо знака «T». Отделение границ даты и есть самое хрупкое место
	 *          разбора устаревшего описания
	 *
	 * @param engine источник псевдослучайных чисел
	 * @return       построенная дата сообщения
	 *
	 */
	string date(mt19937 & engine) noexcept {
		// Виды даты сообщения, живыми устройствами писанные
		static const char * FORMS[] = {
			"Oct  9 22:14:15",
			"Oct 09 22:14:15",
			"Oct 9 22:14:15",
			"Oct 22 12:34:56 2011",
			"Sat Jan  8 20:07:41 2011",
			"Sat Jan  08 20:07:41 2011",
			"Sat Jan 8 20:07:41 2011",
			"2024-10-04 13:29:47",
			"2003-10-11T22:14:15.003Z",
			"2003-10-11T22:14:15Z",
			"2003-10-11T22:14:15+03:00",
			"2023-12-25T15:29:22.000003-07:00",
			"-"
		};
		// Выводим построенную дату сообщения
		return string(FORMS[engine() % (sizeof(FORMS) / sizeof(FORMS[0]))]);
	}

	/**
	 * @brief Функция построения имени, описанием дозволенного
	 *
	 * @param engine источник псевдослучайных чисел
	 * @param length длина строимого имени
	 * @return       построенное имя
	 *
	 */
	string name(mt19937 & engine, const size_t length) noexcept {
		// Набор знаков, из которых строится имя
		static const char alphabet[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789.-_@";
		// Строимое имя
		string result;
		// Выделяем память под строимое имя
		result.reserve(length);
		/**
		 * Выполняем построение имени заданной длины
		 */
		for(size_t i = 0; i < length; i++)
			// Добавляем очередной знак в строимое имя
			result.append(1, alphabet[engine() % (sizeof(alphabet) - 1)]);
		// Выводим построенное имя
		return result;
	}

	/**
	 * @brief Функция построения записи устаревшего описания
	 *
	 * @param engine источник псевдослучайных чисел
	 * @return       построенная запись системного журнала
	 *
	 */
	string legacy(mt19937 & engine) noexcept {
		// Строимая запись системного журнала
		string result;
		/**
		 * Если приставка приоритета записью объявляется
		 *
		 * @note Записи без приставки строятся намеренно: живые устройства шлют и такие,
		 *       и разбор их отказом отвечать не должен
		 */
		if((engine() % 4) > 0){
			// Открываем приставку приоритета угловой скобкой
			result.append(1, '<');
			// Добавляем приоритет записи в приставку
			result.append(::std::to_string(engine() % 200));
			// Закрываем приставку приоритета угловой скобкой
			result.append(1, '>');
		}
		// Добавляем дату сообщения в строимую запись
		result.append(::date(engine));
		// Отделяем имя узла от даты сообщения пробелом
		result.append(1, ' ');
		// Добавляем имя узла в строимую запись
		result.append(::name(engine, 1 + (engine() % 24)));
		/**
		 * Если метка приложения записью объявляется
		 */
		if((engine() % 4) > 0){
			// Отделяем метку приложения от имени узла пробелом
			result.append(1, ' ');
			// Добавляем название приложения в строимую запись
			result.append(::name(engine, 1 + (engine() % 16)));
			/**
			 * Если опознаватель работы записью объявляется
			 */
			if((engine() % 2) > 0){
				// Открываем опознаватель работы квадратной скобкой
				result.append(1, '[');
				// Добавляем опознаватель работы в строимую запись
				result.append(::std::to_string(engine() % 100000));
				// Закрываем опознаватель работы квадратной скобкой
				result.append(1, ']');
			}
			// Закрываем метку приложения двоеточием
			result.append(1, ':');
		}
		/**
		 * Если текст сообщения записью объявляется
		 */
		if((engine() % 8) > 0){
			// Отделяем текст сообщения от заголовка пробелом
			result.append(1, ' ');
			// Добавляем текст сообщения в строимую запись
			result.append(::chunk(engine, engine() % 120));
		}
		// Выводим построенную запись системного журнала
		return result;
	}

	/**
	 * @brief Функция построения записи нынешнего описания
	 *
	 * @param engine источник псевдослучайных чисел
	 * @return       построенная запись системного журнала
	 *
	 */
	string modern(mt19937 & engine) noexcept {
		// Строимая запись системного журнала
		string result;
		/**
		 * Если приставка приоритета записью объявляется
		 */
		if((engine() % 8) > 0){
			// Открываем приставку приоритета угловой скобкой
			result.append(1, '<');
			// Добавляем приоритет записи в приставку
			result.append(::std::to_string(engine() % 200));
			// Закрываем приставку приоритета угловой скобкой
			result.append(1, '>');
		}
		// Добавляем номер описания записи
		result.append(::std::to_string(1 + ((engine() % 16) == 0 ? 1 : 0)));
		// Отделяем дату сообщения от номера описания пробелом
		result.append(1, ' ');
		// Добавляем дату сообщения в строимую запись
		result.append(::date(engine));
		/**
		 * Выполняем построение четырёх полей заголовка, знаками записываемых
		 */
		for(size_t i = 0; i < 4; i++){
			// Отделяем поле заголовка от предыдущего пробелом
			result.append(1, ' ');
			// Если поле заголовка записью не объявляется
			if((engine() % 4) == 0)
				// Ставим знак отсутствующего значения вместо поля
				result.append(1, '-');
			// Если поле заголовка записью объявляется
			else result.append(::name(engine, 1 + (engine() % 20)));
		}
		// Отделяем структурированные данные от заголовка пробелом
		result.append(1, ' ');
		/**
		 * Если блоки структурированных данных записью объявляются
		 */
		if((engine() % 4) > 0){
			// Количество строимых блоков структурированных данных
			const size_t blocks = 1 + (engine() % 4);
			/**
			 * Выполняем построение блоков структурированных данных
			 */
			for(size_t i = 0; i < blocks; i++){
				// Открываем блок структурированных данных
				result.append(1, '[');
				// Добавляем опознаватель блока структурированных данных
				result.append(::name(engine, 1 + (engine() % 16)));
				// Количество строимых полей блока структурированных данных
				const size_t params = engine() % 5;
				/**
				 * Выполняем построение полей блока структурированных данных
				 */
				for(size_t j = 0; j < params; j++){
					// Отделяем поле блока от предыдущего пробелом
					result.append(1, ' ');
					// Добавляем имя поля блока структурированных данных
					result.append(::name(engine, 1 + (engine() % 12)));
					// Отделяем имя поля блока от значения знаком равенства
					result.append(1, '=');
					// Открываем значение поля блока кавычкой
					result.append(1, '"');
					// Строимое значение поля блока структурированных данных
					const string value = ::chunk(engine, engine() % 20);
					/**
					 * Выполняем постановку отмены знаков в значении поля блока
					 *
					 * @note Отменяются ровно три знака, описанием названные: иначе
					 *       строимая запись описанию не отвечала бы, и разбор её отказом
					 *       был бы законен - а нам нужна и запись, разбираемая до конца
					 */
					for(size_t k = 0; k < value.size(); k++){
						// Если знак значения отмены требует
						if((value[k] == '"') || (value[k] == ']') || (value[k] == '\\'))
							// Ставим знак обратной косой перед отменяемым знаком
							result.append(1, '\\');
						// Добавляем знак в значение поля блока
						result.append(1, value[k]);
					}
					// Закрываем значение поля блока кавычкой
					result.append(1, '"');
				}
				// Закрываем блок структурированных данных
				result.append(1, ']');
			}
		// Если блоки структурированных данных записью не объявляются
		} else result.append(1, '-');
		/**
		 * Если текст сообщения записью объявляется
		 */
		if((engine() % 8) > 0){
			// Отделяем текст сообщения от структурированных данных пробелом
			result.append(1, ' ');
			// Если текст сообщения меткой порядка байтов открывается
			if((engine() % 4) == 0)
				// Ставим метку порядка байтов перед текстом сообщения
				result.append(syslog::BOM);
			// Добавляем текст сообщения в строимую запись
			result.append(::chunk(engine, engine() % 120));
		}
		// Выводим построенную запись системного журнала
		return result;
	}

	/**
	 * @brief Функция построения очередной записи системного журнала
	 *
	 * @param engine источник псевдослучайных чисел
	 * @return       построенная запись системного журнала
	 *
	 */
	string build(mt19937 & engine) noexcept {
		// Выводим построенную запись описания, жребием выбранного
		return (((engine() % 2) > 0) ? ::modern(engine) : ::legacy(engine));
	}

	/**
	 * @brief Функция наведения порчи на построенную запись
	 *
	 * @details Порча наводится ПОСЛЕ построения правильной записи: разбор обязан либо
	 *          разобрать её, либо отказать - но не портить память и не расходиться при
	 *          разных нарезках подачи
	 *
	 * @param engine источник псевдослучайных чисел
	 * @param text   портимая запись системного журнала
	 * @return       признак того, что порча наведена
	 *
	 */
	bool corrupt(mt19937 & engine, string & text) noexcept {
		// Если порча на запись не наводится
		if((engine() % 3) > 0)
			// Выводим отсутствие наведённой порчи
			return false;
		// Если портить нечего
		if(text.empty())
			// Выводим отсутствие наведённой порчи
			return false;
		/**
		 * Определяем вид наводимой порчи
		 */
		switch(engine() % 6){
			// Если знак записи заменяется произвольным
			case 0: text[engine() % text.size()] = ::chunk(engine, 1).front(); break;
			// Если запись усекается по произвольному месту
			case 1: text.resize(engine() % text.size()); break;
			// Если в запись вставляется произвольный кусок текста
			case 2: text.insert(engine() % text.size(), ::chunk(engine, 1 + (engine() % 8))); break;
			// Если из записи вырезается произвольный кусок текста
			case 3: text.erase(engine() % text.size(), 1 + (engine() % 8)); break;
			// Если запись открывается произвольным куском текста
			case 4: text.insert(static_cast <size_t> (0), ::chunk(engine, 1 + (engine() % 8))); break;
			// Если запись замыкается произвольным куском текста
			default: text.append(::chunk(engine, 1 + (engine() % 8)));
		}
		// Выводим признак наведённой порчи
		return true;
	}

	/**
	 * @brief Функция разбора записи с накоплением выданных событий
	 *
	 * @param text     разбираемая запись системного журнала
	 * @param step     размер куска подачи, нулевой для подачи целиком
	 * @param settings настройки разбора записей
	 * @param events   накапливаемые события разбора
	 * @param error    код отказа разбора записи
	 * @return         состояние чтения по окончании разбора
	 *
	 */
	syslog::state_t consume(const string & text, const size_t step,
	                        const syslog::reader_t::settings_t & settings,
	                        vector <Event> & events, syslog::error_t & error) noexcept {
		// Объект потокового чтения записей
		syslog::reader_t reader(&environment().fmk, &environment().log);
		// Устанавливаем настройки разбора записей
		reader.settings(settings);
		// Смещение подачи записи
		size_t offset = 0;
		/**
		 * Выполняем подачу записи, пока она не исчерпана
		 */
		do {
			// Получаем размер очередного куска подаваемой записи
			const size_t size = ((step == 0) ? (text.size() - offset) :
			                     ((offset + step) < text.size() ? step : (text.size() - offset)));
			// Выполняем подачу очередного куска записи
			reader.feed(text.data() + offset, size, (offset + size) >= text.size());
			// Сдвигаем смещение подачи записи
			offset += size;
			/**
			 * Выполняем перебор всех событий разбора
			 */
			while(reader.next()){
				// Накапливаем очередное событие разбора
				events.push_back({
					static_cast <uint8_t> (reader.event()),
					static_cast <uint8_t> (reader.field()),
					reader.key(), reader.value()
				});
				// Наращиваем количество выданных событий разбора
				totals.events++;
			}
		} while(offset < text.size());
		// Запоминаем код отказа разбора записи
		error = reader.error();
		// Выводим состояние чтения по окончании разбора
		return reader.state();
	}

	/**
	 * @brief Функция сличения перечней выданных разбором событий
	 *
	 * @details Разбор обязан давать ОДИН И ТОТ ЖЕ ряд событий при любом размере куска
	 *          подачи: расхождение означало бы, что состояние разбора между кусками
	 *          теряется - и терялось бы оно молча, ибо отказа при том нет
	 *
	 * @param narrow события подачи записи целиком
	 * @param broad  события подачи записи кусками
	 * @param step   размер куска подачи
	 * @param text   разбираемая запись системного журнала
	 * @return       признак совпадения перечней событий
	 *
	 */
	bool compare(const vector <Event> & narrow, const vector <Event> & broad,
	             const size_t step, const string & text) noexcept {
		/**
		 * Если количество выданных событий разошлось
		 */
		if(narrow.size() != broad.size()){
			// Выводим сообщение о расхождении количества событий
			::fprintf(
				stderr, "syslog fuzz: chunk=%zu event count differs: %zu/%zu\n",
				step, narrow.size(), broad.size()
			);
			// Выводим разбираемую запись
			::fprintf(stderr, "syslog fuzz: record: %s\n", text.c_str());
			// Выводим неуспешность сличения
			return false;
		}
		/**
		 * Выполняем перебор всех выданных разбором событий
		 */
		for(size_t i = 0; i < narrow.size(); i++){
			/**
			 * Если событие разбора разошлось
			 */
			if((narrow.at(i).kind != broad.at(i).kind) || (narrow.at(i).field != broad.at(i).field) ||
			   (narrow.at(i).key != broad.at(i).key) || (narrow.at(i).value != broad.at(i).value)){
				// Выводим сообщение о расхождении события разбора
				::fprintf(
					stderr, "syslog fuzz: chunk=%zu event %zu differs: kind %u/%u field %u/%u\n",
					step, i, (unsigned) narrow.at(i).kind, (unsigned) broad.at(i).kind,
					(unsigned) narrow.at(i).field, (unsigned) broad.at(i).field
				);
				// Выводим содержимое события подачи целиком
				::fprintf(stderr, "syslog fuzz: whole: [%s] = [%s]\n",
				          narrow.at(i).key.c_str(), narrow.at(i).value.c_str());
				// Выводим содержимое события подачи кусками
				::fprintf(stderr, "syslog fuzz: chunked: [%s] = [%s]\n",
				          broad.at(i).key.c_str(), broad.at(i).value.c_str());
				// Выводим разбираемую запись
				::fprintf(stderr, "syslog fuzz: record: %s\n", text.c_str());
				// Выводим неуспешность сличения
				return false;
			}
		}
		// Выводим успешность сличения
		return true;
	}

	/**
	 * @brief Функция поверки укладки записи в дерево и оборота её
	 *
	 * @details Оборот сличает ДЕРЕВЬЯ, а не тексты: дословного совпадения он не
	 *          обещает - обещает значение
	 *
	 * @param text     разбираемая запись системного журнала
	 * @param settings настройки разбора записей
	 * @return         признак успешности поверки
	 *
	 */
	bool tree(const string & text, const syslog::reader_t::settings_t & settings) noexcept {
		// Объект события, удерживаемого целиком
		syslog::document_t doc(&environment().fmk, &environment().log);
		// Если установка настроек разбора записей отказом завершилась
		if(!doc.settings(settings))
			// Выводим успешность поверки: отказ настроек есть законный исход
			return true;
		/**
		 * Если укладка записи в дерево отказом завершилась
		 */
		if(!doc.parse(text))
			// Выводим успешность поверки: отказ разбора есть законный исход
			return true;
		// Наращиваем количество записей, уложенных в дерево
		totals.trees++;
		/**
		 * Выполняем обход всех блоков структурированных данных
		 */
		for(const auto & block : doc.keys("/structures")){
			/**
			 * Если звено пути к потомку не ведёт
			 */
			if(!doc.has("/structures/" + block)){
				// Выводим сообщение о разомкнутости обхода дерева
				::fprintf(stderr, "syslog fuzz: traversal broken at block \"%s\"\n", block.c_str());
				// Выводим разбираемую запись
				::fprintf(stderr, "syslog fuzz: record: %s\n", text.c_str());
				// Выводим неуспешность поверки
				return false;
			}
			/**
			 * Выполняем обход всех полей блока структурированных данных
			 */
			for(const auto & param : doc.keys("/structures/" + block)){
				/**
				 * Если звено пути к потомку не ведёт
				 */
				if(!doc.has("/structures/" + block + "/" + param)){
					// Выводим сообщение о разомкнутости обхода дерева
					::fprintf(stderr, "syslog fuzz: traversal broken at param \"%s/%s\"\n",
					          block.c_str(), param.c_str());
					// Выводим разбираемую запись
					::fprintf(stderr, "syslog fuzz: record: %s\n", text.c_str());
					// Выводим неуспешность поверки
					return false;
				}
			}
		}
		// Выполняем сборку записи из дерева события
		const string rewritten = doc.dump();
		/**
		 * Если сборка записи отказом завершилась
		 */
		if(rewritten.empty())
			// Выводим успешность поверки: отказ сборки есть законный исход
			return true;
		// Наращиваем количество записей, собранных обратно
		totals.rewrites++;
		// Объект события повторного разбора
		syslog::document_t again(&environment().fmk, &environment().log);
		// Если установка настроек разбора записей отказом завершилась
		if(!again.settings(settings))
			// Выводим успешность поверки: отказ настроек есть законный исход
			return true;
		/**
		 * Если повторный разбор собранной записи отказом завершился
		 *
		 * @details Отказ этот законным ИСХОДОМ НЕ является: запись, кодеком собранная,
		 * обязана им же и разбираться. Собери он запись, себе неразборчивую, - и
		 * потребитель получил бы событие, которое нельзя переслать дальше
		 */
		if(!again.parse(rewritten)){
			// Выводим сообщение о неразборчивости собранной записи
			::fprintf(stderr, "syslog fuzz: rewritten record is not parseable: %s\n",
			          syslog::message(again.error()));
			// Выводим исходную запись
			::fprintf(stderr, "syslog fuzz: source: %s\n", text.c_str());
			// Выводим собранную запись
			::fprintf(stderr, "syslog fuzz: rewritten: %s\n", rewritten.c_str());
			// Выводим неуспешность поверки
			return false;
		}
		// Собранная двоичная запись дерева первого разбора
		const auto & one = doc.root().dump();
		// Собранная двоичная запись дерева повторного разбора
		const auto & two = again.root().dump();
		/**
		 * Если деревья первого и повторного разбора разошлись
		 */
		if(one != two){
			// Выводим сообщение о расхождении деревьев оборота
			::fprintf(stderr, "syslog fuzz: roundtrip trees differ\n");
			// Выводим исходную запись
			::fprintf(stderr, "syslog fuzz: source: %s\n", text.c_str());
			// Выводим собранную запись
			::fprintf(stderr, "syslog fuzz: rewritten: %s\n", rewritten.c_str());
			// Выводим неуспешность поверки
			return false;
		}
		// Наращиваем количество записей, оборот выдержавших
		totals.mirrored++;
		// Выводим успешность поверки
		return true;
	}
}

/**
 * @brief Признак сборки ворошителя под надзирателями
 *
 * @details Прогон без надзирателей ловит расхождения разбора, но НЕ ловит порчу
 *          памяти, и разницу эту надо видеть в самой строке отчёта
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
		// Выполняем построение очередной записи системного журнала
		string text = ::build(engine);
		// Наращиваем количество построенных записей
		totals.records++;
		/**
		 * Если на запись наведена порча
		 */
		if(::corrupt(engine, text))
			// Наращиваем количество испорченных записей
			totals.corrupted++;
		// Настройки разбора записей
		syslog::reader_t::settings_t settings;
		// Выбираем описание, каким надлежит читать записи
		settings.standard = static_cast <syslog::standard_t> (engine() % 3);
		// Выбираем строгость сличения разбираемой записи с описанием
		settings.mode = static_cast <syslog::mode_t> (engine() % 3);
		// Выбираем обращение с отсутствующим значением поля
		settings.nil = static_cast <syslog::nil_t> (engine() % 3);
		// Выбираем снятие метки порядка байтов с текста сообщения
		settings.bom = ((engine() % 4) > 0);
		// Выбираем снятие отмены знаков со значений структурированных данных
		settings.unescape = ((engine() % 4) > 0);
		// Перечень событий подачи записи целиком
		vector <Event> narrow;
		// Код отказа разбора записи, поданной целиком
		syslog::error_t first = syslog::error_t::NONE;
		// Выполняем подачу записи целиком
		const syslog::state_t reached = ::consume(text, 0, settings, narrow, first);
		/**
		 * Если запись разобрана до конца
		 */
		if(reached == syslog::state_t::FINISHED)
			// Наращиваем количество записей, разобранных до конца
			totals.survived++;
		/**
		 * Выполняем перебор размеров куска подачи
		 */
		for(const size_t step : {static_cast <size_t> (1), static_cast <size_t> (2),
		                          static_cast <size_t> (3), static_cast <size_t> (7)}){
			// Перечень событий подачи записи кусками
			vector <Event> broad;
			// Код отказа разбора записи, поданной кусками
			syslog::error_t second = syslog::error_t::NONE;
			// Выполняем подачу записи кусками
			const syslog::state_t arrived = ::consume(text, step, settings, broad, second);
			/**
			 * Если перечни выданных разбором событий разошлись
			 */
			if(!::compare(narrow, broad, step, text))
				// Выходим из приложения с кодом ошибки
				return EXIT_FAILURE;
			/**
			 * Если итоги разбора разошлись
			 */
			if((arrived != reached) || (first != second)){
				// Выводим сообщение о расхождении итога разбора
				::fprintf(
					stderr, "syslog fuzz: chunk=%zu outcome differs: state %u/%u error %u/%u\n",
					step, (unsigned) reached, (unsigned) arrived, (unsigned) first, (unsigned) second
				);
				// Выводим разбираемую запись
				::fprintf(stderr, "syslog fuzz: record: %s\n", text.c_str());
				// Выходим из приложения с кодом ошибки
				return EXIT_FAILURE;
			}
		}
		/**
		 * Если поверка дерева события не удалась
		 */
		if(!::tree(text, settings))
			// Выходим из приложения с кодом ошибки
			return EXIT_FAILURE;
	}
	// Выводим статистику работы генератора
	::fprintf(
		stdout,
		"syslog fuzz: %llu records (%llu corrupted), %llu events, %llu parsed to the end, "
		"%llu trees, %llu rewrites, %llu mirrored%s\n",
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
