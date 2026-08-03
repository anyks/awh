/**
 * @file: fmk.hpp
 * @date: 2025-10-25
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @brief Заголовочный файл ядра фреймворка — класс Framework с базовыми утилитами библиотеки:
 *        работа со строками и кодировками, регистр символов, форматирование, конвертация типов,
 *        проверка форматов данных, разбор чисел и вспомогательные операции над контейнерами
 *
 * @copyright: Copyright © 2025
 *
 */

/**
 * Экранируем повторную инициализацию модуля
 */
#ifndef __AWH_FRAMEWORK__
#define __AWH_FRAMEWORK__

/**
 * Стандартные заголовочные файлы
 */
#include <locale>
#include <string>
#include <vector>
#include <cctype>
#include <cstdint>
#include <cwctype>
#include <cstdarg>
#include <cstdlib>
#include <functional>
#include <type_traits>
#include <unordered_set>
#include <unordered_map>

/**
 * Подключаем заголовочные файлы проекта
 */
#include "lib.hpp"
#include "../net/nwt.hpp"
#include "../charset/charset.hpp"

/**
 * @brief Основное пространство имён
 *
 */
namespace awh {
	/**
	 * @brief Прототип класса работы с логами
	 *
	 */
	class Logging;

	/**
	 * Используем стандартное пространство имён
	 */
	using namespace std;

	/**
	 * @brief Класс фреймворка
	 *
	 * @details Ядро библиотеки: набор действий над строками, числами и двоичными
	 *          данными, которыми пользуются все прочие модули. Объект заводится один
	 *          на приложение и передаётся модулям указателем.
	 *
	 *          @code{.cpp}
	 *          awh::fmk_t fmk;
	 *          awh::log_t log(&fmk);
	 *          fmk.setLogger(&log);
	 *          @endcode
	 *
	 * @note Ошибок методы не выбрасывают: все они помечены noexcept, а возникшая
	 *       ошибка записывается в объект логирования, установленный методом
	 *       «setLogger». Отказ выводится пустым результатом либо нулевым значением,
	 *       смотря по методу
	 *
	 * @note Разбор протокольных данных выполняется по таблице ASCII, а не по
	 *       установленной локали: сличение строк, проверка символов и разбор чисел
	 *       ведут себя одинаково, какой бы ни была локаль приложения. Локаль влияет
	 *       на действия над текстом широких символов и на вывод в консоль
	 *
	 * @warning Методы, помеченные const, обращения из нескольких потоков допускают,
	 *          тогда как «setLocale», «setLogger», «domainZone» и «domainZones»
	 *          меняют состояние объекта и одновременного обращения не допускают:
	 *          вызывать их следует до начала работы модулей
	 *
	 */
	typedef class __AWH_SHARED_EXPORT__ Framework {
		public:
			/**
			 * @brief Порядок обращения с символами, кодировке не представимыми
			 *
			 */
			using replace_t = awh::charset::replace_t;
			/**
			 * @brief Кодировка текста
			 *
			 * @details Обозначение заведено затем, чтобы Framework задавал кодировки
			 *          собственным именем, не оговаривая их размещения. Набор кодировок
			 *          задан модулем перекодировки и стандартом кодировок консорциума
			 *          WHATWG, которого модуль держится.
			 *
			 */
			using codepage_t = awh::charset::encoding_t;
			/**
			 * @brief Шаблон метода формирования форматированной строки
			 *
			 * @tparam C    тип символа строки
			 * @tparam Args типы аргументов подстановки
			 *
			 */
			template <typename C, typename... Args>
			/**
			 * @brief Условие пригодности перекрытия по записи «printf»
			 *
			 * @details Перекрытие исключается из разбора, если среди доводов встречается
			 *          список записей подстановки — этот вызов обслуживает подстановка
			 *          по «$N»
			 *
			 */
			using printable_t = enable_if_t <!(false || ... || is_same <decay_t <Args>, vector <basic_string <C>>>::value), basic_string <C>>;
		public:
			/**
			 * @brief Флаги трансформации строк
			 *
			 * @details Приведение регистра выполняется по таблице ASCII для узких строк
			 *          и по установленной локали для строк широких символов.
			 *
			 * @note Флаг «SMART_CASE» переводит в верхний регистр первую букву каждого
			 *       слова, а прочие буквы — в нижний. Границей слова служат пробельный
			 *       символ, знак переноса и знак подчёркивания
			 *
			 */
			enum class transform_t : uint8_t {
				NONE       = 0x00, // Флаг не установлен
				TRIM       = 0x01, // Флаг удаления пробелов
				UPPER_CASE = 0x02, // Флаг перевода в верхний регистр
				LOWER_CASE = 0x03, // Флаг перевода в нижний регистр
				SMART_CASE = 0x04  // Флаг умного перевода начальных символов в верхний режим
			};
			/**
			 * @brief Тип штампа времени
			 *
			 * @details Значения задают единицу измерения, в которой выводится штамп
			 *          времени, а для текстового вывода — разряд, до которого штамп
			 *          записывается.
			 *
			 */
			enum class chrono_t : uint8_t {
				NONE         = 0x00, // Не установлено
				YEAR         = 0x01, // Год
				MONTH        = 0x02, // Месяц
				WEEK         = 0x03, // Неделя
				DAY          = 0x04, // День
				HOUR         = 0x05, // Час
				MINUTES      = 0x06, // Минуты
				SECONDS      = 0x07, // Секунды
				MILLISECONDS = 0x08, // Миллисекунды
				MICROSECONDS = 0x09, // Микросекунды
				NANOSECONDS  = 0x0A  // Наносекунды
			};
			/**
			 * @brief Флаги проверки текстовых данных
			 *
			 * @details Проверки выполняются по таблице ASCII и от установленной локали
			 *          не зависят. Проверка на адрес выполняется разборщиком адресов,
			 *          проверка на запись UTF-8 — модулем Юникода.
			 *
			 * @note Флаг «PSEUDO_NUMBER» принимает записи, числом не являющиеся, но
			 *       начинающиеся либо завершающиеся цифрой: такие записи встречаются
			 *       в журналах и в заголовках протоколов
			 *
			 */
			enum class check_t : uint8_t {
				NONE            = 0x00, // Флаг не установлен
				URL             = 0x01, // Флаг проверки на URL-адрес
				UTF8            = 0x02, // Флаг проверки на UTF-8
				PRINT           = 0x03, // Флаг проверки на печатаемый символ
				UPPER           = 0x04, // Флаг проверки на верхний регистр
				LOWER           = 0x05, // Флаг проверки на нижний регистр
				SPACE           = 0x06, // Флаг проверки на пробел
				LATIAN          = 0x07, // Флаг проверки на латинские символы
				NUMBER          = 0x08, // Флаг проверки на число
				DECIMAL         = 0x09, // Флаг проверки на число с плавающей точкой
				PSEUDO_NUMBER   = 0x0A, // Флаг проверки на псевдо-число
				PRESENCE_LATIAN = 0x0B  // Флаг проверки наличия латинских символов в строке
			};
		private:
			// Объект парсинга nwt адреса
			nwt_t _nwt;
		private:
			// Устанавливаем локаль по умолчанию
			std::locale _locale;
		private:
			// Объект работы с логами
			const Logging * _log;
		public:
			/**
			 * @brief Шаблон метода поиска в контейнере map указанного значения
			 *
			 * @tparam A тип контейнера
			 * @tparam B тип искомого значения
			 *
			 */
			template <typename A, typename B>
			/**
			 * @brief Метод поиска в контейнере map указанного значения
			 *
			 * @details Поиск ведётся по значению записи, а не по её ключу. Значения
			 *          строковых типов сличаются без учёта регистра, прочие — на равенство.
			 *
			 *          @code{.cpp}
			 *          unordered_map <uint16_t, string> codes = {{200, "OK"}, {404, "Not Found"}};
			 *          auto it = fmk.findInMap(string{"not found"}, codes);
			 *          if(it != codes.end())
			 *              // Обнаружен код 404: регистр значения роли не играет
			 *          @endcode
			 *
			 * @note Отсутствие записи выводится итератором конца контейнера, как и у
			 *       поиска стандартной библиотеки
			 *
			 * @note Просмотр идёт по всем записям подряд: время поиска растёт с размером
			 *       контейнера, тогда как поиск по ключу выполняется за постоянное время
			 *
			 * @note Вид сличения выбирается на этапе сборки по типу искомого значения,
			 *       отчего подстановка в поиск типов, сличения не допускающих, ошибкой
			 *       сборки не оборачивается
			 *
			 * @param val значение которое необходимо найти
			 * @param map контейнер в котором нужно произвести поиск
			 * @return    итератор найденного элемента в контейнере
			 *
			 */
			typename A::const_iterator findInMap(const B & val, const A & map) const noexcept {
				/**
				 * @brief Структура проверки значения записи контейнера
				 *
				 */
				struct Check {
					private:
						// Значение, с которым сличаются записи контейнера
						B _value;
					private:
						// Объект фреймворка
						const Framework * _fmk;
					public:
						/**
						 * @brief Оператор [()] выполнения сравнения полученных данных
						 *
						 * @param item текущее проверяемое значение
						 * @return     результат проверки
						 *
						 */
						bool operator () (const typename A::value_type & item) const noexcept {
							/**
							 * Если сличаются строковые значения
							 *
							 * @details Вид сличения выбирается на этапе сборки: сличение
							 *          строк к прочим типам данных неприменимо, и подстановка
							 *          их в него собраться не могла бы.
							 *
							 */
							if constexpr (is_same <B, string>::value || is_same <B, wstring>::value)
								// Выводим результат сличения строк без учёта регистра
								return !this->_fmk->compare(this->_value, item.second);
							/**
							 * Если сличаются значения прочих типов
							 */
							else
								// Выводим результат сличения значений на равенство
								return (this->_value != item.second);
						}
					public:
						/**
						 * @brief Конструктор
						 *
						 * @param value эталонное значение для сравнения
						 * @param fmk   объект фреймворка
						 *
						 */
						Check(const B & value, const Framework * fmk) noexcept : _value(value), _fmk(fmk) {}
				} callback(val, this);
				// Выполняем поиск искомого значения в контейнере map
				return std::find_if_not(map.cbegin(), map.cend(), callback);
			}
		public:
			/**
			 * @brief Метод генерации уникального идентификатора
			 * 
			 * @return уникальный идентификатор
			 *
			 */
			uint32_t identifier() const noexcept;
		public:
			/**
			 * @brief Метод проверки текста на соответствие флагу
			 *
			 * @param letter текст для проверки
			 * @param flag   флаг проверки
			 * @return       результат проверки
			 *
			 */
			bool is(const char letter, const check_t flag) const noexcept;
			/**
			 * @brief Метод проверки текста на соответствие флагу
			 *
			 * @param letter текст для проверки
			 * @param flag   флаг проверки
			 * @return       результат проверки
			 *
			 */
			bool is(const wchar_t letter, const check_t flag) const noexcept;
		public:
			/**
			 * @brief Метод проверки текста на соответствие флагу
			 *
			 * @details Проверка выполняется над всем текстом целиком: результат выводится
			 *          истиной, лишь когда флагу отвечает каждый символ текста. Исключение
			 *          составляют флаги «URL», «UTF8», «PSEUDO_NUMBER» и «PRESENCE_LATIAN»,
			 *          рассматривающие текст целиком, а не посимвольно.
			 *
			 *          @code{.cpp}
			 *          fmk.is("12345", awh::fmk_t::check_t::NUMBER);          // истина
			 *          fmk.is("12.45", awh::fmk_t::check_t::DECIMAL);         // истина
			 *          fmk.is("v1.2", awh::fmk_t::check_t::PSEUDO_NUMBER);    // истина
			 *          fmk.is("Привет", awh::fmk_t::check_t::UTF8);           // истина
			 *          fmk.is("https://anyks.com", awh::fmk_t::check_t::URL); // истина
			 *          @endcode
			 *
			 * @note Пустой текст флагу не отвечает: проверка выводит ложь
			 *
			 * @param text текст для проверки
			 * @param flag флаг проверки
			 * @return     результат проверки
			 *
			 */
			bool is(string_view text, const check_t flag) const noexcept;
			/**
			 * @brief Метод проверки текста на соответствие флагу
			 *
			 * @param text текст для проверки
			 * @param flag флаг проверки
			 * @return     результат проверки
			 *
			 */
			bool is(wstring_view text, const check_t flag) const noexcept;
		public:
			/**
			 * @brief Метод сравнения двух строк без учёта регистра
			 *
			 * @details Регистр приводится по таблице ASCII, отчего сличение не зависит
			 *          от установленной локали и годится для заголовков протоколов, где
			 *          регистр значения не имеет.
			 *
			 *          @code{.cpp}
			 *          fmk.compare("Content-Type", "content-type");  // истина
			 *          fmk.compare("Привет", "ПРИВЕТ");              // ложь
			 *          @endcode
			 *
			 * @note Строки разной длины сличаются ложью сразу, длину символа в кодировке
			 *       UTF-8 сличение не учитывает: буквы вне набора ASCII сличаются побайтно
			 *       и потому с учётом регистра
			 *
			 * @note Две пустые строки признаются равными
			 *
			 * @param first  первое слово
			 * @param second второе слово
			 * @return       результат сравнения
			 *
			 */
			bool compare(string_view first, string_view second) const noexcept;
			/**
			 * @brief Метод сравнения двух строк без учёта регистра
			 *
			 * @param first  первое слово
			 * @param second второе слово
			 * @return       результат сравнения
			 *
			 */
			bool compare(const char * first, const char * second) const noexcept;
		public:
			/**
			 * @brief Метод сравнения двух строк без учёта регистра
			 *
			 * @param first  первое слово
			 * @param second второе слово
			 * @return       результат сравнения
			 *
			 */
			bool compare(wstring_view first, wstring_view second) const noexcept;
			/**
			 * @brief Метод сравнения двух строк без учёта регистра
			 *
			 * @param first  первое слово
			 * @param second второе слово
			 * @return       результат сравнения
			 *
			 */
			bool compare(const wchar_t * first, const wchar_t * second) const noexcept;
		private:
			/**
			 * @brief Метод получения штампа времени в указанных единицах измерения
			 *
			 * @param buffer буфер бинарных данных для установки штампа времени
			 * @param size   размер бинарных данных штампа времени
			 * @param type   тип формируемого штампа времени
			 * @param text   флаг извлечения данных в текстовом виде
			 *
			 */
			void timestamp(void * buffer, const size_t size, const chrono_t type, const bool text) const noexcept;
		public:
			/**
			 * @brief Шаблон метода получения штампа времени в указанных единицах измерения
			 *
			 * @tparam T тип данных в котором извлекаются данные
			 *
			 */
			template <typename T>
			/**
			 * @brief Метод получения штампа времени в указанных единицах измерения
			 *
			 * @details Штамп снимается с системных часов. Числовой тип получает время,
			 *          прошедшее с начала эпохи в заданных единицах измерения, тогда как
			 *          строковый тип получает запись даты и времени, доведённую до
			 *          заданного разряда.
			 *
			 *          @code{.cpp}
			 *          fmk.timestamp <uint64_t> (awh::fmk_t::chrono_t::MILLISECONDS);
			 *          fmk.timestamp <string> (awh::fmk_t::chrono_t::SECONDS);
			 *          @endcode
			 *
			 * @param type тип формируемого штампа времени
			 * @return     сгенерированный штамп времени
			 *
			 */
			T timestamp(const chrono_t type) const noexcept;
		public:
			/**
			 * @brief Метод конвертирования текста из одной кодировки в другую
			 *
			 * @details Отказ конвертирования выводится пустым текстом и записывается
			 *          в лог. Кодировки задаются обозначением, полученным разбором имени
			 *          методом «codepage» либо определением кодировки методом «detect».
			 *
			 *          @code{.cpp}
			 *          // Приведение тела ответа к UTF-8 по заголовку Content-Type
			 *          const auto codepage = fmk.codepage("windows-1251");
			 *          const string body = fmk.transcode(payload, codepage, awh::fmk_t::codepage_t::UTF8);
			 *
			 *          // Замена символов, кодировке не представимых, знаком вопроса
			 *          fmk.transcode("Привет", awh::fmk_t::codepage_t::UTF8,
			 *                        awh::fmk_t::codepage_t::ISO8859_1,
			 *                        awh::fmk_t::replace_t::REPLACE);           // «??????»
			 *          @endcode
			 *
			 * @note Совпадение кодировок отказом не является: текст выводится без изменений
			 *
			 * @param text    текст для конвертирования
			 * @param from    кодировка, в которой записан текст
			 * @param to      кодировка, в которую требуется сконвертировать текст
			 * @param replace порядок обращения с символами, кодировке не представимыми
			 * @return        сконвертированный текст в требуемой кодировке
			 *
			 */
			string transcode(string_view text, const codepage_t from, const codepage_t to, const replace_t replace = replace_t::STRICT) const noexcept;
		public:
			/**
			 * @brief Метод разбора имени кодировки
			 *
			 * @details Имя приводится к нормальному виду: буквы записываются в нижнем
			 *          регистре, окружающие пробельные символы опускаются. Распознаются
			 *          все имена, которыми кодировки обозначает стандарт кодировок
			 *          консорциума WHATWG, а не одни лишь канонические.
			 *
			 *          @code{.cpp}
			 *          fmk.codepage("windows-1251");  // codepage_t::CP1251
			 *          fmk.codepage(" CP1251 ");      // codepage_t::CP1251
			 *          fmk.codepage("koi8-r");        // codepage_t::KOI8_R
			 *          @endcode
			 *
			 * @note Нераспознанное имя выводится значением «NONE» и ошибкой не считается:
			 *       имя задаётся отправителем произвольно
			 *
			 * @param name имя кодировки, заданное заголовком протокола
			 * @return     обозначение кодировки либо признак нераспознанного имени
			 *
			 */
			codepage_t codepage(string_view name) const noexcept;
			/**
			 * @brief Метод извлечения имени кодировки по её обозначению
			 *
			 * @param codepage обозначение кодировки текста
			 * @return         каноническое имя кодировки
			 *
			 */
			string codepage(const codepage_t codepage) const noexcept;
			/**
			 * @brief Метод определения кодировки текста
			 *
			 * @details Определение выполняется проверкой правильности записи текста
			 *          в кодировке UTF-8: текст, ей отвечающий, признаётся записанным
			 *          в UTF-8, а не отвечающий — записанным в заданной кодировке.
			 *
			 *          @code{.cpp}
			 *          fmk.detect("Привет");                   // codepage_t::UTF8
			 *          fmk.detect("\xCF\xF0\xE8\xE2\xE5\xF2");   // codepage_t::CP1251
			 *          @endcode
			 *
			 * @note Кодировку однобайтового текста определить нельзя: любая
			 *       последовательность байтов записана в любой однобайтовой кодировке.
			 *       Предполагаемая кодировка задаётся доводом и по умолчанию равна CP1251
			 *
			 * @param text     текст, кодировку которого требуется определить
			 * @param fallback кодировка, предполагаемая для текста, записью UTF-8 не являющегося
			 * @return         обозначение определённой кодировки текста
			 *
			 */
			codepage_t detect(string_view text, const codepage_t fallback = codepage_t::CP1251) const noexcept;
		public:
			/**
			 * @brief Метод трансформации одного символа
			 *
			 * @param letter символ для трансформации
			 * @param flag   флаг трансформации
			 * @return       трансформированный символ
			 *
			 */
			char transform(const char letter, const transform_t flag) const noexcept;
			/**
			 * @brief Метод трансформации одного символа
			 *
			 * @param letter символ для трансформации
			 * @param flag   флаг трансформации
			 * @return       трансформированный символ
			 *
			 */
			wchar_t transform(const wchar_t letter, const transform_t flag) const noexcept;
		public:
			/**
			 * @brief Метод трансформации строки
			 *
			 * @details Строка изменяется на месте и выводится ссылкой на неё же.
			 *
			 *          @code{.cpp}
			 *          string text = "  hello-world  ";
			 *          fmk.transform(text, awh::fmk_t::transform_t::TRIM);        // «hello-world»
			 *          fmk.transform(text, awh::fmk_t::transform_t::SMART_CASE);  // «Hello-World»
			 *          @endcode
			 *
			 * @note Перегрузки, принимающие строку доводом-значением либо ссылкой на
			 *       постоянную строку, исходную строку не меняют, а выводят изменённую
			 *
			 * @param text текст для трансформации
			 * @param flag флаг трансформации
			 * @return     трансформированная строка
			 *
			 */
			string & transform(string & text, const transform_t flag) const noexcept;
			/**
			 * @brief Метод трансформации строки
			 *
			 * @param text текст для трансформации
			 * @param flag флаг трансформации
			 * @return     трансформированная строка
			 *
			 */
			wstring & transform(wstring & text, const transform_t flag) const noexcept;
		public:
			/**
			 * @brief Метод трансформации строки
			 *
			 * @param text текст для трансформации
			 * @param flag флаг трансформации
			 * @return     трансформированная строка
			 *
			 */
			const string & transform(const string & text, const transform_t flag) const noexcept;
			/**
			 * @brief Метод трансформации строки
			 *
			 * @param text текст для трансформации
			 * @param flag флаг трансформации
			 * @return     трансформированная строка
			 *
			 */
			const wstring & transform(const wstring & text, const transform_t flag) const noexcept;
		public:
			/**
			 * @brief Метод трансформации строки
			 *
			 * @param text текст для трансформации
			 * @param flag флаг трансформации
			 * @return     трансформированная строка
			 *
			 */
			string transform(string_view text, const transform_t flag) const noexcept;
			/**
			 * @brief Метод трансформации строки
			 *
			 * @param text текст для трансформации
			 * @param flag флаг трансформации
			 * @return     трансформированная строка
			 *
			 */
			wstring transform(wstring_view text, const transform_t flag) const noexcept;
		public:
			/**
			 * @brief Метод объединения списка строк в одну строку
			 *
			 * @details Разделитель размещается между записями списка, а перед первой
			 *          записью и после последней не размещается.
			 *
			 *          @code{.cpp}
			 *          fmk.join({"gzip", "deflate", "br"}, ", ");  // «gzip, deflate, br»
			 *          @endcode
			 *
			 * @note Пустой список даёт пустую строку, список из одной записи — саму запись
			 *
			 * @see split
			 *
			 * @param items список строк которые необходимо объединить
			 * @param delim разделитель
			 * @return      строка полученная после объединения
			 *
			 */
			string join(const vector <string> & items, string_view delim) const noexcept;
			/**
			 * @brief Метод объединения списка строк в одну строку
			 *
			 * @param items список строк которые необходимо объединить
			 * @param delim разделитель
			 * @return      строка полученная после объединения
			 *
			 */
			wstring join(const vector <wstring> & items, wstring_view delim) const noexcept;
		public:
			/**
			 * @brief Метод разделения строк на токены
			 *
			 * @details Разделителем служит вся переданная строка целиком, а не любой из
			 *          её символов. Полученный контейнер очищается перед заполнением.
			 *
			 *          @code{.cpp}
			 *          vector <string> items;
			 *          fmk.split("gzip, deflate, br", ", ", items);  // {"gzip", "deflate", "br"}
			 *          @endcode
			 *
			 * @note Контейнер выводится ссылкой на переданный, что позволяет
			 *       переиспользовать выделенную им память при разборе многих строк
			 *
			 * @see join
			 *
			 * @param text      строка для парсинга
			 * @param delim     разделитель
			 * @param container результирующий вектор
			 *
			 */
			vector <string> & split(string_view text, string_view delim, vector <string> & container) const noexcept;
			/**
			 * @brief Метод разделения строк на токены
			 *
			 * @param text      строка для парсинга
			 * @param delim     разделитель
			 * @param container результирующий вектор
			 *
			 */
			vector <wstring> & split(wstring_view text, wstring_view delim, vector <wstring> & container) const noexcept;
		public:
			/**
			 * @brief Метод конвертирования строки в строку utf-8
			 *
			 * @details Узкая строка разбирается как запись в кодировке UTF-8 и выводится
			 *          строкой широких символов. Обратное действие выполняется одноимённым
			 *          методом, принимающим строку широких символов.
			 *
			 *          @code{.cpp}
			 *          const wstring wide = fmk.convert(string{"Привет"});
			 *          const string text = fmk.convert(wide);
			 *          @endcode
			 *
			 * @note Разрядность широкого символа задаётся операционной системой: на
			 *       MS Windows он двухбайтовый, и символы за пределами основной плоскости
			 *       записываются суррогатной парой, тогда как на прочих системах он
			 *       четырёхбайтовый и хранит кодовое значение целиком
			 *
			 * @note Текст, записью UTF-8 не являющийся, выводится пустой строкой,
			 *       а ошибка записывается в лог
			 *
			 * @param str строка для конвертирования
			 * @return    строка в utf-8
			 *
			 */
			wstring convert(string_view str) const noexcept;
			/**
			 * @brief Метод конвертирования строки utf-8 в строку
			 *
			 * @param str строка utf-8 для конвертирования
			 * @return    обычная строка
			 *
			 */
			string convert(wstring_view str) const noexcept;
			/**
			 * @brief Метод конвертирования строки в строку utf-8
			 *
			 * @param str строка для конвертирования
			 * @return    строка в utf-8
			 *
			 */
			wstring convert(const char * str) const noexcept;
			/**
			 * @brief Метод конвертирования строки utf-8 в строку
			 *
			 * @param str строка utf-8 для конвертирования
			 * @return    обычная строка
			 *
			 */
			string convert(const wchar_t * str) const noexcept;
			/**
			 * @brief Метод конвертирования строки в строку utf-8
			 *
			 * @param str строка для конвертирования
			 * @return    строка в utf-8
			 *
			 */
			wstring convert(const string & str) const noexcept;
			/**
			 * @brief Метод конвертирования строки utf-8 в строку
			 *
			 * @param str строка utf-8 для конвертирования
			 * @return    обычная строка
			 *
			 */
			string convert(const wstring & str) const noexcept;
		public:
			/**
			 * @brief функции определения точного размера, сколько занимает число байт
			 *
			 * @tparam T тип данных с которым работает функция
			 *
			 */
			template <typename T>
			/**
			 * @brief Метод определения точного размера, сколько занимает число байт
			 *
			 * @details Выводится количество байт, которым число записывается без потери
			 *          значения, а не размер его типа. Метод служит сжатию записи чисел
			 *          в двоичных протоколах.
			 *
			 *          @code{.cpp}
			 *          fmk.size <uint64_t> (255);    // 1
			 *          fmk.size <uint64_t> (256);    // 2
			 *          fmk.size <uint64_t> (0);      // 0
			 *          @endcode
			 *
			 * @param num число для проверки
			 * @return    фактический размер занимаемым числом байт
			 *
			 */
			size_t size(const T num) const noexcept;
			/**
			 * @brief Метод определения точного размера, сколько занимают данные (в байтах) в буфере
			 *
			 * @details Выводится размер буфера за вычетом нулевых байтов, лежащих в его
			 *          конце. Буфер, заполненный нулями целиком, даёт нулевой размер.
			 *
			 * @warning Порядок байт числа при этом не учитывается: на машине с прямым
			 *          порядком байт старший разряд числа лежит в конце буфера, и метод
			 *          пригоден лишь для буферов, записанных обратным порядком
			 *
			 * @param value значение бинарного буфера для проверки
			 * @param size  общий размер бинарного буфера
			 * @return      фактический размер буфера занимаемый данными
			 *
			 */
			size_t size(const void * value, const size_t size) const noexcept;
		public:
			/**
			 * @brief Шаблон функции проверки больше первое число второго или нет (бинарным методом)
			 *
			 * @tparam T тип данных с которым работает функция
			 *
			 */
			template <typename T>
			/**
			 * @brief Метод проверки больше первое число второго или нет (бинарным методом)
			 *
			 * @param num1 значение первого числа в бинарном виде
			 * @param num2 значение второго числа в бинарном виде
			 * @return     результат проверки
			 *
			 */
			bool isGreater(const T num1, const T num2) const noexcept;
			/**
			 * @brief Метод проверки больше первое число второго или нет (бинарным методом)
			 *
			 * @details Сличаются **числа**, лежащие в памяти так, как их кладёт сама
			 *          машина. Перебор идёт с последнего байта: на машине с обратным
			 *          порядком байт старший разряд числа лежит именно там. Так и
			 *          задумано, и разворачивать перебор не следует - иначе метод
			 *          перестанет сличать числа
			 *
			 * @warning Двоичный буфер, числом не являющийся, сличать этим методом
			 *          **нельзя**: сетевой адрес, аппаратный адрес, отпечаток - всё
			 *          это лежит в своём порядке, старшим байтом вперёд, и порядок их
			 *          даёт побайтное сличение `memcmp`, а не этот метод
			 *
			 * @param value1 значение первого числа в бинарном виде
			 * @param value2 значение второго числа в бинарном виде
			 * @param size   размер бинарного буфера числа
			 * @return       результат проверки
			 *
			 */
			bool isGreater(const void * value1, const void * value2, const size_t size) const noexcept;
		public:
			/**
			 * @brief Шаблон функции конвертации чисел в указанную систему счисления
			 *
			 * @tparam T тип данных с которым работает функция
			 *
			 */
			template <typename T>
			/**
			 * @brief Метод конвертации чисел в указанную систему счисления
			 *
			 * @details Разряды свыше девятого записываются прописными буквами латиницы.
			 *          Запись по основанию 2 дополняется нулями до разрядности типа,
			 *          прочие основания записи не дополняют.
			 *
			 *          @code{.cpp}
			 *          fmk.itoa <uint32_t> (255, 16);  // «FF»
			 *          fmk.itoa <uint8_t> (255, 2);    // «11111111»
			 *          fmk.itoa <uint32_t> (255, 2);   // «00000000000000000000000011111111»
			 *          @endcode
			 *
			 * @note Основание системы счисления принимается в пределах от 2 до 36:
			 *       при ином основании выводится пустая строка
			 *
			 * @warning Знак числа записью не сохраняется: число со знаком записывается
			 *          так, как лежит в памяти, отчего значение -42 типа int32_t даёт
			 *          запись «4294967254»
			 *
			 * @see atoi
			 *
			 * @param value число для конвертации
			 * @param radix система счисления
			 * @return      полученная строка в указанной системе счисления
			 *
			 */
			string itoa(const T value, const uint8_t radix) const noexcept;
			/**
			 * @brief Метод конвертации чисел в указанную систему счисления
			 *
			 * @param value бинарный буфер числа для конвертации
			 * @param size  размер бинарного буфера
			 * @param radix система счисления
			 * @return      полученная строка в указанной системе счисления
			 *
			 */
			string itoa(const void * value, const size_t size, const uint8_t radix) const noexcept;
		public:
			/**
			 * @brief Шаблон функции конвертации строковых чисел в десятичную систему счисления
			 *
			 * @tparam T тип данных с которым работает функция
			 *
			 */
			template <typename T>
			/**
			 * @brief Метод конвертации строковых чисел в десятичную систему счисления
			 *
			 * @details Разбор выполняется по таблице ASCII и от установленной локали
			 *          не зависит: разделителем дробной части всегда служит точка.
			 *          Тип разбираемого числа задаётся доводом шаблона.
			 *
			 *          @code{.cpp}
			 *          fmk.atoi <uint32_t> ("12345");     // 12345
			 *          fmk.atoi <double> ("3.14159");     // 3.14159
			 *          fmk.atoi <int32_t> ("-42");        // -42
			 *          fmk.atoi <uint32_t> ("ff", 16);    // 255
			 *          @endcode
			 *
			 * @note Запись, числом не являющаяся, выводится нулевым значением; запись,
			 *       выходящую за пределы типа, метод обрезает по его границе
			 *
			 * @note Часть строки разбирается её представлением, а не парой из указателя
			 *       и длины:
			 *
			 *       @code{.cpp}
			 *       fmk.atoi <uint32_t> (string_view{text.data() + begin, length});
			 *       @endcode
			 *
			 * @see itoa
			 *
			 * @param value строковое представление числа
			 * @return      числовое значение в десятичной системе счисления
			 *
			 */
			T atoi(string_view value) const noexcept;
			/**
			 * @brief Шаблон функции конвертации строковых чисел в десятичную систему счисления
			 *
			 * @tparam T тип данных с которым работает функция
			 *
			 */
			template <typename T>
			/**
			 * @brief Метод конвертации строковых чисел в десятичную систему счисления
			 *
			 * @param value число в бинарном виде для конвертации в 10-ю систему
			 * @param radix система счисления
			 * @return      полученное значение в десятичной системе счисления
			 *
			 */
			T atoi(string_view value, const uint8_t radix) const noexcept;
			/**
			 * @brief Метод конвертации строковых чисел в десятичную систему счисления
			 *
			 * @param value  число в бинарном виде для конвертации в 10-ю систему
			 * @param radix  система счисления
			 * @param buffer бинарный буфер куда следует положить результат
			 * @param size   размер бинарного буфера куда следует положить результат
			 *
			 */
			void atoi(string_view value, const uint8_t radix, void * buffer, const size_t size) const noexcept;
		public:
			public:
			/**
			 * @brief Шаблон функции конвертации строковых чисел в десятичную систему счисления
			 *
			 * @tparam T тип данных с которым работает функция
			 *
			 */
			template <typename T>
			/**
			 * @brief Метод конвертации строковых чисел в десятичную систему счисления
			 *
			 * @param value строковое представление числа
			 * @return      числовое значение в десятичной системе счисления
			 *
			 */
			T atoi(wstring_view value) const noexcept;
			/**
			 * @brief Шаблон функции конвертации строковых чисел в десятичную систему счисления
			 *
			 * @tparam T тип данных с которым работает функция
			 *
			 */
			template <typename T>
			/**
			 * @brief Метод конвертации строковых чисел в десятичную систему счисления
			 *
			 * @param value число в бинарном виде для конвертации в 10-ю систему
			 * @param radix система счисления
			 * @return      полученное значение в десятичной системе счисления
			 *
			 */
			T atoi(wstring_view value, const uint8_t radix) const noexcept;
			/**
			 * @brief Метод конвертации строковых чисел в десятичную систему счисления
			 *
			 * @param value  число в бинарном виде для конвертации в 10-ю систему
			 * @param radix  система счисления
			 * @param buffer бинарный буфер куда следует положить результат
			 * @param size   размер бинарного буфера куда следует положить результат
			 *
			 */
			void atoi(wstring_view value, const uint8_t radix, void * buffer, const size_t size) const noexcept;
		public:
			/**
			 * @brief Метод перевода числа в безэкспоненциальную форму
			 *
			 * @details Количество знаков после запятой задаётся размером шага, дробная
			 *          часть при этом округляется. Целое число записывается без дробной
			 *          части вовсе, каким бы ни был размер шага.
			 *
			 *          @code{.cpp}
			 *          fmk.noexp(2986.808299, static_cast <uint8_t> (3));  // «2986.808»
			 *          fmk.noexp(2986.808299, static_cast <uint8_t> (4));  // «2986.8083»
			 *          fmk.noexp(2986., static_cast <uint8_t> (4));        // «2986»
			 *          @endcode
			 *
			 * @note Запись не зависит от установленной локали: разделителем дробной части
			 *       всегда служит точка, разделителей разрядов запись не содержит
			 *
			 * @note Нулевой размер шага даёт запись «0»: округлять до нуля знаков после
			 *       запятой следует перегрузкой с подбором точности
			 *
			 * @param number число для перевода
			 * @param step   размер шага после запятой
			 * @return       число в безэкспоненциальной форме
			 *
			 * @see noexp(const double, const bool)
			 *
			 */
			string noexp(const double number, const uint8_t step) const noexcept;
			/**
			 * @brief Метод перевода числа в безэкспоненциальную форму
			 *
			 * @details Количество знаков после запятой подбирается наименьшим из тех, при
			 *          котором запись читается обратно ровно тем же числом. Запись выходит
			 *          краткой, не теряя при этом ни одного значащего разряда.
			 *
			 *          @code{.cpp}
			 *          fmk.noexp(1e+19);                 // «10000000000000000000»
			 *          fmk.noexp(1e-5);                  // «0.00001»
			 *          fmk.noexp(1536. / 1024.);         // «1.5»
			 *          fmk.noexp(0.111);                 // «0.111»
			 *          fmk.noexp(-2986.808299);          // «-2986.808299»
			 *          @endcode
			 *
			 * @note Запись не зависит от установленной локали: разделителем дробной части
			 *       всегда служит точка, разделителей разрядов запись не содержит
			 *
			 * @note Округления запись не выполняет: число, требующее семнадцати значащих
			 *       разрядов, все семнадцать и получит. Для краткой записи с потерей
			 *       точности следует пользоваться перегрузкой с размером шага
			 *
			 * @note Довод вывода одних лишь разрядов сохранён ради совместимости вызовов
			 *       и на запись не влияет: посторонних символов она не содержит
			 *
			 * @param number  число для перевода
			 * @param onlyNum выводить только числа
			 * @return        число в безэкспоненциальной форме
			 *
			 * @see noexp(const double, const uint8_t)
			 *
			 */
			string noexp(const double number, const bool onlyNum = false) const noexcept;
		public:
			/**
			 * @brief Метод порверки на сколько процентов (A > B) или (A < B)
			 *
			 * @details Выводится отклонение первого числа от второго в процентах от
			 *          второго. Превышение выводится положительным значением, недостача —
			 *          отрицательным.
			 *
			 *          @code{.cpp}
			 *          fmk.rate(150.f, 100.f);  // 50
			 *          fmk.rate(50.f, 100.f);   // -50
			 *          @endcode
			 *
			 * @note Нулевое второе число даёт нулевой результат: делить на него нельзя
			 *
			 * @param a первое число
			 * @param b второе число
			 * @return  результат расчёта
			 *
			 */
			float rate(const float a, const float b) const noexcept;
			/**
			 * @brief Метод приведения количества символов после запятой к указанному количества
			 *
			 * @details Дробная часть отсекается, а не округляется: число приводится
			 *          к ближайшему меньшему.
			 *
			 *          @code{.cpp}
			 *          fmk.floor(3.14159, 2);  // 3.14
			 *          fmk.floor(3.999, 2);    // 3.99
			 *          @endcode
			 *
			 * @note Округление выполняется методом «noexp», выводящим запись числа
			 *
			 * @see noexp
			 *
			 * @param x число для приведения
			 * @param n количество символов после запятой
			 * @return  сформированное число
			 *
			 */
			double floor(const double x, const uint8_t n) const noexcept;
		public:
			/**
			 * @brief Метод перевода римских цифр в арабские
			 *
			 * @details Запись разбирается без учёта регистра.
			 *
			 *          @code{.cpp}
			 *          fmk.rome2arabic("XIV");   // 14
			 *          fmk.rome2arabic("mcmxc"); // 1990
			 *          @endcode
			 *
			 * @note Запись, римским числом не являющаяся, выводится нулевым значением
			 *
			 * @see arabic2rome
			 *
			 * @param word римское число
			 * @return     арабское число
			 *
			 */
			uint16_t rome2arabic(string_view word) const noexcept;
			/**
			 * @brief Метод перевода римских цифр в арабские
			 *
			 * @param word римское число
			 * @return     арабское число
			 *
			 */
			uint16_t rome2arabic(wstring_view word) const noexcept;
		public:
			/**
			 * @brief Метод перевода арабских чисел в римские
			 *
			 * @details Запись выводится прописными буквами.
			 *
			 *          @code{.cpp}
			 *          fmk.arabic2rome(14);    // «XIV»
			 *          fmk.arabic2rome(1990);  // «MCMXC»
			 *          @endcode
			 *
			 * @note Число вне пределов от 1 до 4999 выводится пустой строкой: римская
			 *       запись больших чисел требует надчёркивания, записи не имеющего
			 *
			 * @see rome2arabic
			 *
			 * @param number арабское число от 1 до 4999
			 * @return       римское число
			 *
			 */
			wstring arabic2rome(const uint32_t number) const noexcept;
			/**
			 * @brief Метод перевода арабских чисел в римские
			 *
			 * @param word арабское число от 1 до 4999
			 * @return     римское число
			 *
			 */
			string arabic2rome(string_view word) const noexcept;
			/**
			 * @brief Метод перевода арабских чисел в римские
			 *
			 * @param word арабское число от 1 до 4999
			 * @return     римское число
			 *
			 */
			wstring arabic2rome(wstring_view word) const noexcept;
		public:
			/**
			 * @brief Метод подсчёта количества указанной буквы в слове
			 *
			 * @details Подсчёт ведётся с учётом регистра.
			 *
			 *          @code{.cpp}
			 *          fmk.countLetter("example.com", L'.');  // 1
			 *          @endcode
			 *
			 * @param word   слово в котором нужно подсчитать букву
			 * @param letter букву которую нужно подсчитать
			 * @return       результат подсчёта
			 *
			 */
			size_t countLetter(string_view word, const wchar_t letter) const noexcept;
			/**
			 * @brief Метод подсчёта количества указанной буквы в слове
			 *
			 * @param word   слово в котором нужно подсчитать букву
			 * @param letter букву которую нужно подсчитать
			 * @return       результат подсчёта
			 *
			 */
			size_t countLetter(wstring_view word, const wchar_t letter) const noexcept;
		public:
			/**
			 * @brief Шаблон функции проверки установлен ли бит в указанной позиции
			 *
			 * @tparam T тип данных с которым работает функция
			 *
			 */
			template <typename T>
			/**
			 * @brief Метод проверки установлен ли бит в указанной позиции
			 *
			 * @details Разряды отсчитываются от младшего, начиная с нуля.
			 *
			 *          @code{.cpp}
			 *          fmk.isBit <uint8_t> (0, 0b00000101);   // истина
			 *          fmk.setBit <uint8_t> (3, 0b00000001);  // 0b00001001
			 *          fmk.resetBit <uint8_t> (0, 0b00000101); // 0b00000100
			 *          fmk.flipBit <uint8_t> (1, 0b00000101);  // 0b00000111
			 *          @endcode
			 *
			 * @warning Позиция, разрядность типа превышающая, поведения не задаёт:
			 *          проверять её следует вызывающей стороне
			 *
			 * @param pos позиция для проверки
			 * @param num число в бинарном виде для проверки бита
			 * @return    результат проверки
			 *
			 */
			bool isBit(const T pos, const T num) const noexcept;
			/**
			 * @brief Шаблон функции инверсии бита в указанной позиции
			 *
			 * @tparam T тип данных с которым работает функция
			 *
			 */
			template <typename T>
			/**
			 * @brief Метод инверсии бита в указанной позиции
			 *
			 * @param pos позиция для инверсии
			 * @param num число в бинарном виде для инверсии бита
			 * @return    итоговое значение числа после инверсии
			 *
			 */
			T flipBit(const T pos, const T num) const noexcept;
			/**
			 * @brief Шаблон функции сброса бита в указанной позиции
			 *
			 * @tparam T тип данных с которым работает функция
			 *
			 */
			template <typename T>
			/**
			 * @brief Метод сброса бита в указанной позиции
			 *
			 * @param pos позиция для сброса
			 * @param num число в бинарном виде для сброса бита
			 * @return    итоговое значение числа после сброса бита
			 *
			 */
			T resetBit(const T pos, const T num) const noexcept;
			/**
			 * @brief Шаблон функции устанвки бита в указанную позицию
			 *
			 * @tparam T тип данных с которым работает функция
			 *
			 */
			template <typename T>
			/**
			 * @brief Метод устанвки бита в указанную позицию
			 *
			 * @param pos позиция для установки бита
			 * @param num начальное значение бита
			 * @return    итоговое значение числа после установки бита
			 *
			 */
			T setBit(const T pos, const T num = 0) const noexcept;
		private:
			/**
			 * @brief Метод формирования форматированной строки по записи «printf»
			 *
			 * @details Здесь выполняется само построение строки. Наружу метод не
			 *          выведен: обращение к нему идёт через перекрытие «format»,
			 *          отчего перечень доводов с переменным числом остаётся вне
			 *          разбора перекрытий и не сталкивается с подстановкой по «$N»
			 *
			 * @param format формат строки вывода
			 * @param args   передаваемые аргументы
			 * @return       сформированная строка
			 *
			 */
			string formatted(const char * format, ...) const noexcept;
			/**
			 * @brief Метод формирования форматированной строки по записи «printf»
			 *
			 * @param format формат строки вывода
			 * @param args   передаваемые аргументы
			 * @return       сформированная строка
			 *
			 */
			wstring formatted(const wchar_t * format, ...) const noexcept;
		public:
			/**
			 * @brief Шаблон функции реализации функции формирования форматированной строки
			 *
			 * @tparam Args типы доводов функции
			 *
			 */
			template <typename... Args>
			/**
			 * @brief Метод реализации функции формирования форматированной строки
			 *
			 * @details Запись формата задана стандартной библиотекой и совпадает с
			 *          записью, принимаемой функцией «printf».
			 *
			 *          @code{.cpp}
			 *          fmk.format("%s:%u", "127.0.0.1", 8080);  // «127.0.0.1:8080»
			 *          @endcode
			 *
			 * @warning Проверки соответствия доводов записи формата не выполняется:
			 *          несоответствие ведёт к незаданному поведению, как и у «printf»
			 *
			 * @param format формат строки вывода
			 * @param args   передаваемые аргументы
			 * @return       сформированная строка
			 *
			 */
			printable_t <char, Args...> format(const char * format, Args... args) const noexcept {
				// Выполняем формирование строки по записи «printf»
				return this->formatted(format, args...);
			}
			/**
			 * @brief Шаблон функции реализации функции формирования форматированной строки
			 *
			 * @tparam Args типы доводов функции
			 *
			 */
			template <typename... Args>
			/**
			 * @brief Метод реализации функции формирования форматированной строки
			 *
			 * @param format формат строки вывода
			 * @param args   передаваемые аргументы
			 * @return       сформированная строка
			 *
			 */
			printable_t <wchar_t, Args...> format(const wchar_t * format, Args... args) const noexcept {
				// Выполняем формирование строки по записи «printf»
				return this->formatted(format, args...);
			}
		public:
			/**
			 * @brief Метод реализации функции формирования форматированной строки
			 *
			 * @details Места подстановки обозначаются знаком доллара с номером записи
			 *          списка, начиная с единицы. Записи подставляются по всему тексту,
			 *          сколько бы раз обозначение ни встретилось. Записи «\r», «\n»
			 *          и «\t» при этом раскрываются в соответствующие им символы.
			 *
			 *          @code{.cpp}
			 *          fmk.format("$1 -> $2 ($1)", vector <string> {"a", "b"});
			 *          // «a -> b (a)»
			 *          @endcode
			 *
			 * @note Типов доводов запись не задаёт, отчего несоответствия доводов
			 *       записи, свойственного «printf», здесь не возникает
			 *
			 * @note Перекрытие с тем же именем выполняет подстановку по записи «printf».
			 *       Столкновения на строковом литерале не возникает: то перекрытие
			 *       исключается из разбора, когда среди доводов встречается список
			 *       записей подстановки
			 *
			 * @param format формат строки вывода
			 * @param items  список аргументов строки
			 * @return       сформированная строка
			 *
			 */
			string format(string_view format, const vector <string> & items) const noexcept;
			/**
			 * @brief Метод реализации функции формирования форматированной строки
			 *
			 * @param format формат строки вывода
			 * @param items  список аргументов строки
			 * @return       сформированная строка
			 *
			 */
			wstring format(wstring_view format, const vector <wstring> & items) const noexcept;
		public:
			/**
			 * @brief Метод проверки существования слова в тексте
			 *
			 * @details Слово ищется как часть текста, границы слова при этом не
			 *          проверяются: слово «код» обнаруживается в тексте «кодировка».
			 *
			 * @param word слово для проверки
			 * @param text текст в котором выполнения проверка
			 * @return     результат выполнения проверки
			 *
			 */
			bool exists(string_view word, string_view text) const noexcept;
			/**
			 * @brief Метод проверки существования слова в тексте
			 *
			 * @param word слово для проверки
			 * @param text текст в котором выполнения проверка
			 * @return     результат выполнения проверки
			 *
			 */
			bool exists(wstring_view word, wstring_view text) const noexcept;
		public:
			/**
			 * @brief Метод замены в тексте слово на другое слово
			 *
			 * @details Заменяются все вхождения слова, а не одно лишь первое. Текст
			 *          изменяется на месте и выводится ссылкой на него же.
			 *
			 *          @code{.cpp}
			 *          string text = "a-b-c";
			 *          fmk.replace(text, "-", "/");  // «a/b/c»
			 *          fmk.replace(text, "/");       // «abc», слово опускается
			 *          @endcode
			 *
			 * @note Опущенное слово замены задаёт удаление вхождений
			 *
			 * @param text текст в котором нужно произвести замену
			 * @param word слово для поиска
			 * @param alt  слово на которое нужно произвести замену
			 * @return     результирующий текст
			 *
			 */
			string & replace(string & text, const string & word, const string & alt = "") const noexcept;
			/**
			 * @brief Метод замены в тексте слово на другое слово
			 *
			 * @param text текст в котором нужно произвести замену
			 * @param word слово для поиска
			 * @param alt  слово на которое нужно произвести замену
			 * @return     результирующий текст
			 *
			 */
			wstring & replace(wstring & text, const wstring & word, const wstring & alt = L"") const noexcept;
		public:
			/**
			 * @brief Метод замены в тексте слово на другое слово
			 *
			 * @param text текст в котором нужно произвести замену
			 * @param word слово для поиска
			 * @param alt  слово на которое нужно произвести замену
			 * @return     результирующий текст
			 *
			 */
			const string & replace(const string & text, const string & word, const string & alt = "") const noexcept;
			/**
			 * @brief Метод замены в тексте слово на другое слово
			 *
			 * @param text текст в котором нужно произвести замену
			 * @param word слово для поиска
			 * @param alt  слово на которое нужно произвести замену
			 * @return     результирующий текст
			 *
			 */
			const wstring & replace(const wstring & text, const wstring & word, const wstring & alt = L"") const noexcept;
		private:
			/**
			 * @brief Метод извлечения списка символов экранирования по умолчанию
			 *
			 * @return список символов экранирования по умолчанию
			 *
			 */
			static const vector <string> & escapingText() noexcept;
			/**
			 * @brief Метод извлечения списка символов экранирования по умолчанию
			 *
			 * @return список символов экранирования по умолчанию
			 *
			 */
			static const vector <wstring> & escapingWide() noexcept;
		public:
			/**
			 * @brief Намеренные решения разбора записей ключ-значение
			 *
			 * @details Разбор рассчитан на форматы вида CEF, где значение записи не отделено от следующего ключа
			 *          ничем, кроме разделителя записей, поэтому приняты следующие решения:
			 *
			 *          1. Значение может состоять из нескольких слов и содержать разделитель записей: концом
			 *             значения считается последний разделитель, встреченный перед разделителем ключа и
			 *             значения следующей записи (rt=Feb 17 2023 23:30:15.734 YEKT smac=...).
			 *          2. Если следующей записи нет, значение занимает весь остаток текста, а не обрывается на
			 *             первом разделителе, иначе последняя запись вела бы себя иначе, чем все остальные.
			 *          3. Пустое значение является полноценной записью и возвращается с пустой строкой: в CEF
			 *             запись вида cs3= осмысленна и образует пару с cs3Label=CVEID.
			 *          4. Разделители записей в начале текста и между записями пропускаются.
			 *          5. Символ считается экранированным при нечётном количестве предшествующих обратных слэшей,
			 *             что позволяет разбирать значения вида originsicname=CN\=chr-cpsg-01,O\=stal.
			 *          6. Повторяющиеся ключи сохраняются полностью, поэтому результатом является multimap:
			 *             в реальных журналах ключ повторяется (ad.prog-id, deviceExternalId в auditd).
			 *          7. Относительный порядок одинаковых ключей в контейнере зависит от реализации: если важен
			 *             порядок следования записей в исходном тексте, следует использовать потоковый разбор,
			 *             который отдаёт записи строго в порядке их появления.
			 *
			 */
			/**
			 * @brief Метод извлечения ключей и значений из текста
			 *
			 * @details Разбор рассчитан на записи журналов и заголовки протоколов, где
			 *          значение от следующего ключа ничем, кроме разделителя записей,
			 *          не отделено. Устройство разбора описано выше.
			 *
			 *          @code{.cpp}
			 *          const auto items = fmk.kv("cs1=a cs2=b cs3=", " ");
			 *          // {"cs1": "a", "cs2": "b", "cs3": ""}
			 *          @endcode
			 *
			 * @note Повторяющиеся ключи сохраняются полностью, отчего результатом
			 *       является multimap. Порядок следования записей в исходном тексте
			 *       контейнером не сохраняется: он снимается потоковым разбором
			 *
			 * @see kv(const uint64_t, string_view, string_view, function <void (const uint64_t, const string_view, const string_view)>, string_view, const vector <string> &)
			 *
			 * @param text      текст из которого извлекаются записи
			 * @param delim     разделитель записей
			 * @param separator разделитель ключа и значения
			 * @param escaping  символы экранирования
			 * @return          список найденных элементов
			 *
			 */
			unordered_multimap <string, string> kv(string_view text, string_view delim, string_view separator = "=", const vector <string> & escaping = escapingText()) const noexcept;
			/**
			 * @brief Метод извлечения ключей и значений из текста
			 *
			 * @param text      текст из которого извлекаются записи
			 * @param delim     разделитель записей
			 * @param separator разделитель ключа и значения
			 * @param escaping  символы экранирования
			 * @return          список найденных элементов
			 *
			 */
			unordered_multimap <wstring, wstring> kv(wstring_view text, wstring_view delim, wstring_view separator = L"=", const vector <wstring> & escaping = escapingWide()) const noexcept;
		public:
			/**
			 * @brief Метод потокового извлечения ключей и значений из текста
			 *
			 * @details Записи отдаются функции обратного вызова строго в порядке их
			 *          появления в тексте, а контейнер под них не заводится вовсе.
			 *          Идентификатор потока разбора передаётся функции без изменений
			 *          и позволяет разбирать несколько текстов одним обработчиком.
			 *
			 *          @code{.cpp}
			 *          fmk.kv(1, "cs1=a cs2=b", " ", [](const uint64_t sid,
			 *           const string_view key, const string_view value) noexcept {
			 *              // Записи придут в порядке «cs1=a», затем «cs2=b»
			 *          });
			 *          @endcode
			 *
			 * @param sid       идентификатор потока разбора
			 * @param text      текст из которого извлекаются записи
			 * @param delim     разделитель записей
			 * @param callback  функция обратного вызова для каждой найденной записи
			 * @param separator разделитель ключа и значения
			 * @param escaping  символы экранирования
			 *
			 */
			void kv(const uint64_t sid, string_view text, string_view delim, function <void (const uint64_t, const string_view, const string_view)> callback, string_view separator = "=", const vector <string> & escaping = escapingText()) const noexcept;
			/**
			 * @brief Метод потокового извлечения ключей и значений из текста
			 *
			 * @param sid       идентификатор потока разбора
			 * @param text      текст из которого извлекаются записи
			 * @param delim     разделитель записей
			 * @param callback  функция обратного вызова для каждой найденной записи
			 * @param separator разделитель ключа и значения
			 * @param escaping  символы экранирования
			 *
			 */
			void kv(const uint64_t sid, wstring_view text, wstring_view delim, function <void (const uint64_t, const wstring_view, const wstring_view)> callback, wstring_view separator = L"=", const vector <wstring> & escaping = escapingWide()) const noexcept;
		public:
			/**
			 * @brief Метод установки пользовательской зоны
			 *
			 * @details Набор доменных зон применяется разбором адресов: зона, набору не
			 *          принадлежащая, адресом не признаётся. Framework везёт набор зон
			 *          общего пользования, а метод пополняет его зонами частными.
			 *
			 *          @code{.cpp}
			 *          fmk.domainZone("local");
			 *          fmk.is("http://server.local", awh::fmk_t::check_t::URL);  // истина
			 *          @endcode
			 *
			 * @warning Метод меняет состояние объекта: вызывать его следует до начала
			 *          работы модулей, обращающихся к разбору адресов
			 *
			 * @param zone пользовательская зона
			 *
			 */
			void domainZone(const string_view zone) noexcept;
			/**
			 * @brief Метод установки списка пользовательских зон
			 *
			 * @param zones список доменных зон интернета
			 *
			 */
			void domainZones(const unordered_set <string> & zones) noexcept;
			/**
			 * @brief Метод извлечения списка пользовательских зон интернета
			 *
			 * @return список доменных зон
			 *
			 */
			const unordered_set <string> & domainZones() const noexcept;
		public:
			/**
			 * @brief Метод установки системной локали
			 *
			 * @details Локаль ставится всему приложению, а не одному объекту Framework:
			 *          метод обращается к системной установке локали. Locale по умолчанию
			 *          ставится конструктором и задана значением «AWH_LOCALE».
			 *
			 * @note Разбор протокольных данных от локали не зависит: она влияет на
			 *       действия над текстом широких символов и на вывод в консоль
			 *
			 * @warning Метод меняет состояние приложения целиком: вызывать его следует
			 *          однажды при запуске, до начала работы модулей
			 *
			 * @param locale локализация приложения
			 *
			 */
			void setLocale(string_view locale = AWH_LOCALE) noexcept;
		public:
			/**
			 * @brief Метод извлечения координат url адресов в строке
			 *
			 * @details Выводится набор пар «начало-конец» размещения каждого обнаруженного
			 *          адреса, а не сами адреса: по ним текст размечается либо разбирается
			 *          дальше без повторного поиска.
			 *
			 *          @code{.cpp}
			 *          for(auto & item : fmk.urls("см. https://anyks.com и ftp://a.b"))
			 *              const string url = text.substr(item.first, item.second - item.first);
			 *          @endcode
			 *
			 * @note Распознание адреса опирается на набор доменных зон, пополняемый
			 *       методом «domainZone»
			 *
			 * @see domainZone
			 *
			 * @param text текст для извлечения url адресов
			 * @return     список координат с url адресами
			 *
			 */
			unordered_map <size_t, size_t> urls(string_view text) const noexcept;
		public:
			/**
			 * @brief Метод получения иконки
			 *
			 * @details Выводится случайно выбранный знак из набора: один набор отведён
			 *          началу работы, другой — её завершению. Метод служит выводу в
			 *          консоль и содержательной нагрузки не несёт.
			 *
			 * @param end флаг завершения работы
			 * @return    иконка напутствия работы
			 *
			 */
			string icon(const bool end = false) const noexcept;
		public:
			/**
			 * @brief Метод получения размера в байтах из строки
			 *
			 * @details Единица измерения сличается без учёта регистра и может отделяться
			 *          от числа пробелом. Приставки задают степени числа 1024, а не 1000.
			 *
			 *          @code{.cpp}
			 *          fmk.bytes("1Kb");        // 1024
			 *          fmk.bytes("1 Kb");       // 1024
			 *          fmk.bytes("1.5 Mb");     // 1572864
			 *          fmk.bytes("100 Gb");     // 107374182400
			 *          fmk.bytes("1024 bytes"); // 1024
			 *          @endcode
			 *
			 * @note Запись, не начинающаяся цифрой, выводится нулевым значением
			 *
			 * @note Единица измерения обязательна: задача метода — получить точное число
			 *       байт из записи размерности, а не разобрать число. Запись из одних
			 *       цифр выводится нулевым значением, и разбирать её следует модулем
			 *       лексического разбора чисел
			 *
			 * @see bytes(const double, const bool)
			 *
			 * @param str строка обозначения размерности (b, Kb, Mb, Gb, Tb)
			 * @return    размер в байтах
			 *
			 */
			double bytes(const string_view str) const noexcept;
			/**
			 * @brief Метод конвертации байт в строку
			 *
			 * @details Единица измерения подбирается наибольшей из тех, при которой число
			 *          остаётся не меньше единицы. Запись числа выполняется методом «noexp»
			 *          и от установленной локали не зависит.
			 *
			 *          @code{.cpp}
			 *          fmk.bytes(1024.);     // «1 Kb»
			 *          fmk.bytes(1572864.);  // «1.5 Mb»
			 *          fmk.bytes(512.);      // «512 bytes»
			 *          fmk.bytes(0.);        // «0 bytes»
			 *          @endcode
			 *
			 * @note Запись, выводимая этим методом, разбирается обратно одноимённым
			 *       методом до того же значения
			 *
			 * @see bytes(const string_view)
			 *
			 * @param value   количество байт
			 * @param onlyNum выводить только числа
			 * @return        полученная строка
			 *
			 */
			string bytes(const double value, const bool onlyNum = false) const noexcept;
		public:
			/**
			 * @brief Метод получения количества байт в секунду из строки
			 *
			 * @details Пропускная способность сети задаётся в битах, а выводится в байтах:
			 *          разобранное значение делится на восемь. Приставки при этом задают
			 *          степени числа 1000, а не 1024, как принято для пропускной
			 *          способности сети.
			 *
			 *          @code{.cpp}
			 *          fmk.bpsSize("8bps");     // 1
			 *          fmk.bpsSize("1Kbps");    // 125
			 *          fmk.bpsSize("1.5Mbps");  // 187500
			 *          fmk.bpsSize("100Mbps");  // 12500000
			 *          @endcode
			 *
			 * @note Приставки размера буфера, выводимого методом «bytes», задают степени
			 *       числа 1024: единицы измерения этих двух методов не совпадают намеренно
			 *
			 * @see bpsBuffer
			 *
			 * @param str пропускная способность сети (bps, kbps, Mbps, Gbps)
			 * @return    количество байт в секунду
			 *
			 */
			size_t bpsSize(const string_view str) const noexcept;
			/**
			 * @brief Метод получения размера буфера в байтах
			 *
			 * @details Выводится размер приёмного либо передающего буфера сокета,
			 *          отвечающий заданной пропускной способности сети.
			 *
			 * @see bpsSize
			 *
			 * @param str пропускная способность сети (bps, kbps, Mbps, Gbps)
			 * @return    размер буфера в байтах
			 *
			 */
			size_t bpsBuffer(const string_view str) const noexcept;
		public:
			/**
			 * @brief Метод установки объекта логирования
			 *
			 * @details Объект логирования принимает ошибки, возникающие в методах.
			 *          Пока он не установлен, ошибки выводятся в поток ошибок.
			 *
			 * @warning Метод меняет состояние объекта: вызывать его следует однажды
			 *          при запуске, до начала работы модулей
			 *
			 * @param log объект работы с логами
			 *
			 */
			void setLogger(const Logging * log) noexcept;
		public:
			/**
			 * @brief Конструктор
			 *
			 */
			explicit Framework() noexcept;
			/**
			 * @brief Конструктор
			 *
			 * @details Локаль ставится всему приложению. Конструктор без довода ставит
			 *          локаль, заданную значением «AWH_LOCALE».
			 *
			 * @see setLocale
			 *
			 * @param locale локализация приложения
			 *
			 */
			explicit Framework(string_view locale) noexcept;
			/**
			 * @brief Деструктор
			 *
			 */
			~Framework() noexcept {}
	} fmk_t;
};

#endif // __AWH_FRAMEWORK__
