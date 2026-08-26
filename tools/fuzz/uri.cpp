/**
 * @file uri.cpp
 * @date 2026-08-26
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
 * @brief Инструмент фаззинга разбора адресов ресурсов — построение записей URI по
 *        грамматикам разных схем с точечной порчей, подача их разбору и проверка
 *        устойчивости приведения к принятому виду, согласия сличения с печатью и
 *        устойчивости порядка параметров запроса
 *
 * @details Договор здесь тоньше, чем у разбора сетевых адресов, и проверять его надо
 *          с оглядкой на записанные намеренные решения (tests/net/uri/DECISIONS.md).
 *          Круговой ход «разобрать - напечатать» дословного совпадения с ИСХОДНОЙ
 *          записью НЕ обещает: приведение к принятому виду снимает точечные сегменты,
 *          раскодирует процент-последовательности, приводит схему и хост к строчным,
 *          прячет порт по умолчанию. Оттого проверяется УСТОЙЧИВОСТЬ приведения:
 *          напечатанное, будучи разобрано снова, обязано напечататься точно так же
 *
 * @warning Составляющие объекта здесь НЕ задаются установщиками намеренно. У ряда схем
 *          место ресурса хранится хостом, а не путём, и сочетание, грамматике схемы
 *          противное, строкой невыразимо вовсе - это записанное ограничение 6.3, а не
 *          дефект. Ворошитель работает только разбором записей, где такое сочетание
 *          возникнуть не может
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
#include <net/uri.hpp>

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
		// Количество построенных записей адресов
		uint64_t records;
		// Количество испорченных записей адресов
		uint64_t corrupted;
		// Количество записей, разбор переживших
		uint64_t parsed;
		// Количество проверок устойчивости приведения к принятому виду
		uint64_t settled;
		// Количество проверок согласия сличения с печатью
		uint64_t compared;
		// Количество проверок устойчивости порядка параметров запроса
		uint64_t queries;
		// Количество проверок согласия полного и умного видов печати
		uint64_t formats;
		/**
		 * @brief Конструктор
		 *
		 */
		Statistic() noexcept :
		 records(0), corrupted(0), parsed(0), settled(0), compared(0), queries(0), formats(0) {}
	};
	/**
	 * @brief Функция получения случайного числа в заданных пределах
	 *
	 * @warning Имя ЗАНЯТО: свободная функция «random» столкнулась бы с одноимённой
	 *          функцией POSIX из «cstdlib», и сборка отказала бы доводом «too many
	 *          arguments to function call, expected 0»
	 *
	 * @param engine источник случайных чисел
	 * @param bound  верхний предел получаемого числа, не включая его самого
	 * @return       полученное случайное число
	 *
	 */
	uint32_t pick(mt19937_64 & engine, const uint32_t bound) noexcept {
		// Выводим полученное случайное число
		return static_cast <uint32_t> (engine() % bound);
	}
	/**
	 * @brief Функция вывода записи адреса
	 *
	 * @param title название выводимой записи
	 * @param text  выводимая запись адреса
	 *
	 */
	void dump(const char * title, const string & text) noexcept {
		// Выводим название записи
		::fprintf(stderr, "--- %s (%zu байт) ---\n", title, text.size());
		/**
		 * Выполняем перебор всех знаков записи
		 */
		for(const char letter : text){
			// Если знак является печатным
			if((static_cast <uint8_t> (letter) >= 0x20) && (static_cast <uint8_t> (letter) < 0x7F))
				// Выводим знак записи как есть
				::fputc(letter, stderr);
			// Выводим знак записи кодом
			else ::fprintf(stderr, "\\x%02X", static_cast <uint8_t> (letter));
		}
		// Выводим перевод строки
		::fputc('\n', stderr);
	}
	/**
	 * @brief Функция построения хоста записи адреса
	 *
	 * @param engine источник случайных чисел
	 * @return       построенный хост записи
	 *
	 */
	string buildHost(mt19937_64 & engine) noexcept {
		/**
		 * Определяем вид собираемого хоста
		 */
		switch(static_cast <uint8_t> (::pick(engine, 6))){
			// Собираем доменное имя
			case 0: {
				// Собранное доменное имя
				string result = "";
				// Получаем количество частей имени
				const uint32_t parts = (1 + ::pick(engine, 3));
				/**
				 * Выполняем перебор всех частей имени
				 */
				for(uint32_t i = 0; i < parts; i++){
					// Добавляем разделитель частей
					if(i > 0) result.append(1, '.');
					// Получаем длину части имени
					const uint32_t length = (1 + ::pick(engine, 6));
					/**
					 * Выполняем перебор всех знаков части имени
					 */
					for(uint32_t j = 0; j < length; j++)
						// Добавляем знак части имени
						result.append(1, static_cast <char> ('a' + ::pick(engine, 26)));
				}
				// Добавляем область имён
				return result.append(".com");
			}
			// Собираем имя местной машины
			case 1: return "localhost";
			// Собираем адрес IPv4
			case 2: {
				// Забираем части адреса по одной, в заданном порядке
				const uint32_t a = ::pick(engine, 256), b = ::pick(engine, 256);
				const uint32_t c = ::pick(engine, 256), d = ::pick(engine, 256);
				// Собранная запись адреса
				char buffer[32];
				// Собираем запись адреса
				::snprintf(buffer, sizeof(buffer), "%u.%u.%u.%u", a, b, c, d);
				// Выводим собранную запись адреса
				return string(buffer);
			}
			/**
			 * Собираем адрес IPv6 в скобках
			 *
			 * @note Скобки обязательны: без них двоеточия адреса неотличимы от
			 *       разделителя порта, и запись читается иначе
			 */
			case 3: {
				// Собранная запись адреса
				char buffer[32];
				// Забираем части адреса по одной, в заданном порядке
				const uint32_t a = ::pick(engine, 0x10000), b = ::pick(engine, 0x10000);
				// Собираем запись адреса
				::snprintf(buffer, sizeof(buffer), "[%x::%x]", a, b);
				// Выводим собранную запись адреса
				return string(buffer);
			}
			// Собираем имя с прописными знаками
			case 4: return "ExAmPlE.COM";
			// Собираем имя с подчёркиванием и цифрами
			default: return string("host_").append(to_string(::pick(engine, 1000)));
		}
	}
	/**
	 * @brief Функция построения записи адреса ресурса
	 *
	 * @param engine источник случайных чисел
	 * @return       построенная запись адреса
	 *
	 */
	string buildURI(mt19937_64 & engine) noexcept {
		// Собранная запись адреса
		string result = "";
		/**
		 * Определяем вид собираемой записи
		 */
		switch(static_cast <uint8_t> (::pick(engine, 8))){
			/**
			 * Собираем запись с полной авторитью
			 */
			case 0:
			case 1:
			case 2: {
				// Набор схем с полной авторитью
				static const char * schemes[] = {"http", "https", "ws", "wss", "ftp"};
				// Добавляем схему записи
				result.append(schemes[::pick(engine, 5)]).append("://");
				// Если запись несёт сведения о пользователе
				if(::pick(engine, 4) == 0){
					// Добавляем имя пользователя
					result.append("user").append(to_string(::pick(engine, 100)));
					// Если запись несёт пароль пользователя
					if(::pick(engine, 2) != 0)
						// Добавляем пароль пользователя
						result.append(1, ':').append("pass").append(to_string(::pick(engine, 100)));
					// Добавляем разделитель сведений о пользователе
					result.append(1, '@');
				}
				// Добавляем хост записи
				result.append(::buildHost(engine));
				// Если запись несёт порт
				if(::pick(engine, 3) == 0)
					// Добавляем порт записи
					result.append(1, ':').append(to_string(::pick(engine, 65536)));
				/**
				 * Добавляем путь записи
				 */
				{
					// Получаем количество сегментов пути
					const uint32_t segments = ::pick(engine, 4);
					/**
					 * Выполняем перебор всех сегментов пути
					 */
					for(uint32_t i = 0; i < segments; i++){
						// Добавляем разделитель сегментов
						result.append(1, '/');
						/**
						 * Определяем вид сегмента пути
						 */
						switch(static_cast <uint8_t> (::pick(engine, 6))){
							// Добавляем точечный сегмент
							case 0: result.append("."); break;
							// Добавляем сегмент возврата
							case 1: result.append(".."); break;
							// Добавляем сегмент с процент-последовательностью
							case 2: result.append("a%20b"); break;
							// Добавляем сегмент с негодной процент-последовательностью
							case 3: result.append("a%zz"); break;
							// Добавляем сегмент с прописными знаками
							case 4: result.append("Path").append(to_string(::pick(engine, 100))); break;
							// Добавляем обычный сегмент
							default: result.append("seg").append(to_string(::pick(engine, 100))); break;
						}
					}
				}
				// Если запись несёт параметры запроса
				if(::pick(engine, 3) == 0){
					// Добавляем разделитель параметров запроса
					result.append(1, '?');
					// Получаем количество пар параметров
					const uint32_t pairs = (1 + ::pick(engine, 3));
					/**
					 * Выполняем перебор всех пар параметров
					 */
					for(uint32_t i = 0; i < pairs; i++){
						// Добавляем разделитель пар
						if(i > 0) result.append(1, '&');
						// Добавляем имя параметра
						result.append("k").append(to_string(::pick(engine, 20)));
						// Добавляем значение параметра
						result.append(1, '=').append("v").append(to_string(::pick(engine, 1000)));
					}
				}
				// Если запись несёт якорь
				if(::pick(engine, 4) == 0)
					// Добавляем якорь записи
					result.append(1, '#').append("frag").append(to_string(::pick(engine, 100)));
			} break;
			// Собираем запись доменного сокета
			case 3: result.append("unix:///var/run/sock").append(to_string(::pick(engine, 100))).append(".sock"); break;
			// Собираем запись почты
			case 4: result.append("mailto:user").append(to_string(::pick(engine, 100))).append("@example.com"); break;
			// Собираем запись файла
			case 5: result.append("file:///tmp/file").append(to_string(::pick(engine, 100))).append(".txt"); break;
			// Собираем запись без схемы
			case 6: result.append(::buildHost(engine)).append("/path").append(to_string(::pick(engine, 100))); break;
			// Собираем запись одного лишь пути
			default: result.append("/path").append(to_string(::pick(engine, 100))).append("/leaf"); break;
		}
		// Выводим собранную запись адреса
		return result;
	}
}

/**
 * @brief Средства порчи и проверки договоров (внутренняя компоновка)
 *
 */
namespace {
	/**
	 * @brief Функция порчи записи адреса
	 *
	 * @param engine источник случайных чисел
	 * @param text   запись адреса для порчи
	 * @return       признак того, что запись была испорчена
	 *
	 */
	bool corrupt(mt19937_64 & engine, string & text, const uint32_t chance) noexcept {
		/**
		 * Если порча вовсе запрещена
		 *
		 * @note Запрет нужен доводом: находка на испорченной записи договора не нарушает,
		 *       ибо устойчивость приведения обещана лишь ПРАВИЛЬНЫМ записям. Отделить
		 *       одно от другого можно только прогоном без порчи вовсе
		 */
		if(chance == 0)
			// Выводим признак того, что запись осталась целой
			return false;
		// Если порча этой записи не выпала либо портить нечего
		if((::pick(engine, chance) != 0) || text.empty())
			// Выводим признак того, что запись осталась целой
			return false;
		/**
		 * Определяем вид порчи записи
		 */
		switch(static_cast <uint8_t> (::pick(engine, 8))){
			// Подменяем случайный знак записи
			case 0: {
				// Забираем место подмены и подставляемый знак по одному
				const size_t place = ::pick(engine, static_cast <uint32_t> (text.size()));
				const char letter = static_cast <char> (::pick(engine, 256));
				// Выполняем подмену знака
				text[place] = letter;
			} break;
			// Обрубаем запись в случайном месте
			case 1: text.resize(::pick(engine, static_cast <uint32_t> (text.size()))); break;
			// Вставляем случайный знак в случайное место
			case 2: {
				// Забираем место вставки и вставляемый знак по одному
				const size_t place = ::pick(engine, static_cast <uint32_t> (text.size()));
				const char letter = static_cast <char> (::pick(engine, 256));
				// Выполняем вставку знака
				text.insert(place, 1, letter);
			} break;
			// Удваиваем разделитель составляющих
			case 3: {
				// Ищем разделитель составляющих записи
				const size_t pos = text.find_first_of(":/?#@");
				// Если разделитель найден, удваиваем его
				if(pos != string::npos) text.insert(pos, 1, text[pos]);
			} break;
			/**
			 * Вставляем нулевой байт в середину записи
			 *
			 * @note Нулевой байт вносится намеренно: разбор принимает string_view, где
			 *       длина задана отдельно, и остановка по нулевому байту была бы дефектом
			 */
			case 4: text.insert(::pick(engine, static_cast <uint32_t> (text.size())), 1, '\0'); break;
			// Добавляем окружающие пробельные знаки
			case 5: text.insert(0, 1, ' ').append(1, '\t'); break;
			// Добавляем хвост из случайных знаков
			case 6: {
				// Получаем длину добавляемого хвоста
				const uint32_t length = (1 + ::pick(engine, 8));
				/**
				 * Выполняем перебор всех знаков хвоста
				 */
				for(uint32_t i = 0; i < length; i++)
					// Добавляем знак хвоста
					text.append(1, static_cast <char> (::pick(engine, 256)));
			} break;
			// Заменяем запись набором случайных байтов
			default: {
				// Получаем длину новой записи
				const uint32_t length = ::pick(engine, 48);
				// Очищаем прежнюю запись
				text.clear();
				/**
				 * Выполняем перебор всех знаков новой записи
				 */
				for(uint32_t i = 0; i < length; i++)
					// Добавляем знак записи
					text.append(1, static_cast <char> (::pick(engine, 256)));
			} break;
		}
		// Выводим признак того, что запись была испорчена
		return true;
	}
	/**
	 * @brief Функция проверки устойчивости приведения к принятому виду
	 *
	 * @details Круговой ход «разобрать - напечатать» дословного совпадения с ИСХОДНОЙ
	 *          записью не обещает и обещать не может: приведение снимает точечные
	 *          сегменты, раскодирует процент-последовательности, приводит схему и хост
	 *          к строчным, прячет порт по умолчанию. Обещано иное - УСТОЙЧИВОСТЬ:
	 *          приведённое, будучи разобрано снова, обязано привестись к себе же.
	 *          Неустойчивость означала бы, что вид записи зависит от числа проходов,
	 *          а такой адрес нельзя ни сличать, ни держать ключом
	 *
	 * @warning Обещание это дано ПРАВИЛЬНЫМ записям и только им. На испорченных находки
	 *          есть, и они дефектом НЕ служат: скажем, у обрубка «[6522::9732» первый
	 *          проход печатает «//[6522::9732», а второй - пустоту, ибо незакрытая скобка
	 *          разбору не поддаётся. Проверено доводом порчи «0»: шесть зёрен по двадцать
	 *          тысяч правильных записей (120 000 проходов, 240 000 проверок устойчивости)
	 *          не дали ни одной находки. Оттого прежде чем нести находку владельцу, её
	 *          обязательно повторить прогоном без порчи вовсе
	 *
	 * @param source исходная запись адреса
	 * @param format вид печати записи
	 * @param totals учёт проделанной работы
	 * @return       признак устойчивости приведения
	 *
	 */
	bool settled(const string & source, const awh::uri_t::format_t format, Statistic & totals) noexcept {
		/**
		 * Объект разбора адреса
		 *
		 * @warning Объект заводится ЗАНОВО под каждый разбор намеренно: разбор поверх
		 *          непустого объекта второй записи не заменяет первую, а разрешает её
		 *          относительно первой по RFC 3986 5.2.2 - это записанное решение 1.1,
		 *          а не дефект. Переиспользование объекта дало бы здесь ложные находки
		 */
		awh::uri_t first(::framework(), ::logger());
		// Выполняем разбор исходной записи адреса
		static_cast <void> (first.parse(source));
		// Получаем печать разобранного адреса
		const string once = first.print(awh::uri_t::item_t::URI, format);
		// Если печать пуста, проверять нечего
		if(once.empty())
			// Выводим признак устойчивости приведения
			return true;
		// Объект разбора приведённого адреса
		awh::uri_t second(::framework(), ::logger());
		// Выполняем разбор приведённой записи адреса
		static_cast <void> (second.parse(once));
		// Получаем печать повторно разобранного адреса
		const string twice = second.print(awh::uri_t::item_t::URI, format);
		/**
		 * Если приведение к принятому виду оказалось неустойчивым
		 */
		if(once != twice){
			// Выводим сообщение о неустойчивости приведения
			::fprintf(stderr, "НЕУСТОЙЧИВОЕ ПРИВЕДЕНИЕ (%s): первый проход «%s», второй «%s»\n",
				((format == awh::uri_t::format_t::FULL) ? "полный вид" : "умный вид"), once.c_str(), twice.c_str());
			// Выводим исходную запись адреса
			::dump("исходная запись", source);
			// Выводим признак неустойчивости приведения
			return false;
		}
		// Увеличиваем счёт проверок устойчивости приведения
		totals.settled++;
		/**
		 * Если сличение объектов разошлось с совпадением их печати
		 *
		 * @note Сличается ВЫПИСЫВАЕМЫЙ вид записи, а не хранимый, - это записанное
		 *       решение 4.3. Оттого равенство печати обязано означать равенство объектов
		 */
		if(!(first == second)){
			// Выводим сообщение о расхождении сличения с печатью
			::fprintf(stderr, "СЛИЧЕНИЕ: печать совпала «%s», а объекты признаны разными\n", once.c_str());
			// Выводим исходную запись адреса
			::dump("исходная запись", source);
			// Выводим признак расхождения сличения
			return false;
		}
		// Увеличиваем счёт проверок согласия сличения с печатью
		totals.compared++;
		// Выводим признак устойчивости приведения
		return true;
	}
	/**
	 * @brief Функция проверки устойчивости порядка параметров запроса
	 *
	 * @details Порядок пар при печати заявлен устойчивым, а не исходным - это записанное
	 *          решение 3.1. Устойчивость эта проверяется здесь двумя печатями подряд:
	 *          порядок, зависящий от порядка обхода хранилища, разошёлся бы между ними
	 *
	 * @param source исходная запись адреса
	 * @param totals учёт проделанной работы
	 * @return       признак устойчивости порядка
	 *
	 */
	bool ordered(const string & source, Statistic & totals) noexcept {
		// Объект разбора адреса
		awh::uri_t uri(::framework(), ::logger());
		// Выполняем разбор исходной записи адреса
		static_cast <void> (uri.parse(source));
		// Получаем первую печать параметров запроса
		const string once = uri.print(awh::uri_t::item_t::QUERY, awh::uri_t::format_t::SMART);
		// Если параметров запроса нет, проверять нечего
		if(once.empty())
			// Выводим признак устойчивости порядка
			return true;
		// Получаем вторую печать параметров запроса
		const string twice = uri.print(awh::uri_t::item_t::QUERY, awh::uri_t::format_t::SMART);
		/**
		 * Если порядок параметров разошёлся между печатями
		 */
		if(once != twice){
			// Выводим сообщение о неустойчивости порядка параметров
			::fprintf(stderr, "ПОРЯДОК ПАРАМЕТРОВ: первая печать «%s», вторая «%s»\n", once.c_str(), twice.c_str());
			// Выводим исходную запись адреса
			::dump("исходная запись", source);
			// Выводим признак неустойчивости порядка
			return false;
		}
		// Увеличиваем счёт проверок устойчивости порядка параметров
		totals.queries++;
		// Выводим признак устойчивости порядка
		return true;
	}
}

/**
 * @brief Функция запуска приложения
 *
 * @param argc длина массива параметров
 * @param argv массив параметров
 * @return     код выхода из приложения
 *
 */
int main(int argc, char * argv[]) noexcept {
	// Получаем количество проходов генератора
	const uint64_t count = ((argc > 1) ? static_cast <uint64_t> (::atoll(argv[1])) : 3000);
	// Получаем зерно источника случайных чисел
	const uint64_t seed = ((argc > 2) ? static_cast <uint64_t> (::atoll(argv[2])) : 20260826);
	/**
	 * Доля порчи записей: единица из скольких записей портится, нуль запрещает порчу
	 *
	 * @note Довод нужен для разделения находок: устойчивость приведения обещана лишь
	 *       правильным записям, и находка на испорченной сама по себе дефектом не служит
	 */
	const uint32_t damage = ((argc > 3) ? static_cast <uint32_t> (::atoll(argv[3])) : 3);
	// Источник случайных чисел
	mt19937_64 engine(seed);
	// Учёт проделанной работы
	Statistic totals;
	/**
	 * Выполняем проходы генератора
	 */
	for(uint64_t pass = 0; pass < count; pass++){
		// Выполняем сборку записи адреса ресурса
		string text = ::buildURI(engine);
		// Увеличиваем счёт построенных записей адресов
		totals.records++;
		/**
		 * Если запись адреса была испорчена
		 */
		if(::corrupt(engine, text, damage))
			// Увеличиваем счёт испорченных записей адресов
			totals.corrupted++;
		/**
		 * Объект разбора адреса для проверки переживания разбора
		 *
		 * @note Разновидность адреса признаком удачи НЕ служит - это записанное решение
		 *       1.2, - оттого итог разбора здесь не утверждается вовсе. Проверяется
		 *       переживание вызова, а дальше работают проверки договоров
		 */
		{
			// Объект разбора адреса
			awh::uri_t uri(::framework(), ::logger());
			// Выполняем разбор записи адреса
			static_cast <void> (uri.parse(text));
			// Увеличиваем счёт записей, разбор переживших
			totals.parsed++;
		}
		/**
		 * Если приведение к принятому виду в умном виде печати неустойчиво
		 */
		if(!::settled(text, awh::uri_t::format_t::SMART, totals))
			// Выходим из приложения с кодом ошибки
			return EXIT_FAILURE;
		/**
		 * Если приведение к принятому виду в полном виде печати неустойчиво
		 *
		 * @note Оба вида печати проверяются намеренно: умный прячет схему и порт при
		 *       их отсутствии, полный выписывает всегда, и пути печати у них разные
		 */
		if(!::settled(text, awh::uri_t::format_t::FULL, totals))
			// Выходим из приложения с кодом ошибки
			return EXIT_FAILURE;
		// Увеличиваем счёт проверок согласия видов печати
		totals.formats++;
		/**
		 * Если порядок параметров запроса неустойчив
		 */
		if(!::ordered(text, totals))
			// Выходим из приложения с кодом ошибки
			return EXIT_FAILURE;
	}
	// Выводим итоги проделанной работы
	::fprintf(stdout,
		"ЗЕРНО=%llu ПРОХОДОВ=%llu\n"
		"  записей построено: %llu, из них испорчено: %llu\n"
		"  разбор пережили: %llu\n"
		"  проверок устойчивости приведения: %llu\n"
		"  проверок согласия сличения с печатью: %llu\n"
		"  проверок согласия видов печати: %llu\n"
		"  проверок устойчивости порядка параметров: %llu\n",
		static_cast <unsigned long long> (seed), static_cast <unsigned long long> (count),
		static_cast <unsigned long long> (totals.records), static_cast <unsigned long long> (totals.corrupted),
		static_cast <unsigned long long> (totals.parsed), static_cast <unsigned long long> (totals.settled),
		static_cast <unsigned long long> (totals.compared), static_cast <unsigned long long> (totals.formats),
		static_cast <unsigned long long> (totals.queries));
	// Выводим успешный код выхода из приложения
	return EXIT_SUCCESS;
}
