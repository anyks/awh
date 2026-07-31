/**
 * @file: headers.hpp
 * @date: 2026-07-08
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @brief Заголовочный файл контейнера HTTP-заголовков — класс http::Headers с регистронезависимым хранением,
 *        итераторами, специализациями хеш-функций,
 *        лимитами на количество и размер заголовков и поддержкой множественных значений одного поля
 *
 * @copyright: Copyright © 2026
 *
 */

#ifndef __AWH_HTTP_HEADERS__
#define __AWH_HTTP_HEADERS__

/**
 * Если максимальное количество заголовков не указано
 */
#ifndef AWH_MAX_COUNT_HTTP_HEADERS
	/**
	 * Устанавливаем максимальное количество заголовков в 100
	 */
	#define AWH_MAX_COUNT_HTTP_HEADERS 0x64
#endif

/**
 * Если максимальное значение потребляемой памяти не указано
 */
#ifndef AWH_MAX_MEMORY_HTTP_HEADERS
	/**
	 * Устанавливаем максимальное значение потребляемой памяти 16 КБ
	 */
	#define AWH_MAX_MEMORY_HTTP_HEADERS 0x4000
#endif

/**
 * Стандартные заголовочные файлы
 */
#include <string>
#include <vector>
#include <unordered_set>
#include <unordered_map>
#include <initializer_list>

/**
 * Подключаем заголовочные файлы проекта
 */
#include "provider.hpp"
#include "../../sys/fmk.hpp"
#include "../../sys/log.hpp"

/**
 * @brief основное пространство имён
 *
 */
namespace awh {
	/**
	 * Подписываемся на стандартное пространство имён
	 */
	using namespace std;

	/**
	 * @brief Пространство имён HTTP-протокола
	 *
	 */
	namespace http {
		/**
		 * @brief Класс контейнера HTTP-заголовков
		 *
		 */
		typedef class __AWH_SHARED_EXPORT__ Headers {
			public:
				/**
				 * @brief Режим добавления заголовка
				 *
				 * @note Режима «не установлен» здесь нет намеренно: он вёл бы себя в точности
				 *       как APPEND, обещая названием третье поведение, которого не существует.
				 *       Режим по умолчанию задаётся значением аргумента, а не членом набора
				 *
				 */
				enum class mode_t : uint8_t {
					APPEND  = 0x01, // Добавить новый заголовок, сохранив существующие одноимённые
					REPLACE = 0x02  // Заменить все существующие одноимённые заголовки новым значением
				};
			public:
				/**
				 * @brief Класс HTTP-заголовка
				 *
				 */
				typedef class __AWH_SHARED_EXPORT__ Header {
					public:
						// Название заголовка
						string name = "";
						// Значение заголовка
						string value = "";
					public:
						/**
						 * @brief Фабричный метод создания HTTP-заголовка
						 *
						 * @param name  название HTTP-заголовка
						 * @param value значение HTTP-заголовка
						 * @return      ссылка на текущий объект заголовка
						 *
						 */
						Header & from(string_view name, string_view value) noexcept;
					public:
						/**
						 * @brief Оператор сравнения
						 *
						 * @param other другой объект для сравнения
						 * @return      результат сравнения
						 *
						 */
						bool operator == (const Header & other) const noexcept;
					public:
						/**
						 * @brief Конструктор
						 *
						 */
						explicit Header() noexcept = default;
				} header_t;
			public:
				/**
				 * @brief Специализация хеш-функции для структуры HTTP-заголовка
				 *
				 */
				typedef class __AWH_SHARED_EXPORT__ Header_Hash {
					public:
						/**
						 * @brief Оператор вычисления хеш-кода
						 *
						 * @param header объект для вычисления хеш-кода
						 * @return       хеш-код объекта
						 *
						 */
						size_t operator()(const header_t & header) const noexcept;
					public:
						/**
						 * @brief Конструктор
						 *
						 */
						explicit Header_Hash() noexcept = default;
				} header_hash_t;
			public:
				/**
				 * @brief Специализация хеш-функции для названия HTTP-заголовка
				 *
				 */
				typedef class __AWH_SHARED_EXPORT__ Header_Name_Hash {
					public:
						/**
						 * @brief Оператор вычисления хеш-кода
						 *
						 * @param name название заголовка для вычисления хеш-кода
						 * @return     хеш-код названия заголовка
						 *
						 */
						size_t operator()(const string & name) const noexcept;
					public:
						/**
						 * @brief Конструктор
						 *
						 */
						explicit Header_Name_Hash() noexcept = default;
				} header_name_hash_t;
			public:
				/**
				 * @brief Специализация предиката равенства для названия HTTP-заголовка
				 *
				 */
				typedef class __AWH_SHARED_EXPORT__ Header_Name_Equal {
					public:
						/**
						 * @brief Оператор сравнения названий заголовков
						 *
						 * @param first  первое название заголовка
						 * @param second второе название заголовка
						 * @return       результат сравнения без учёта регистра
						 *
						 */
						bool operator()(const string & first, const string & second) const noexcept;
					public:
						/**
						 * @brief Конструктор
						 *
						 */
						explicit Header_Name_Equal() noexcept = default;
				} header_name_equal_t;
			public:
				/**
				 * @brief Тип списка HTTP-заголовков
				 *
				 * @details В отличие от карты, список позволяет хранить несколько одноимённых заголовков,
				 *          что необходимо для корректной работы с протоколами HTTP/1.1 и HTTP/2.
				 *          Список также содержит псевдо-заголовки, которые используются в протоколе HTTP/2 для передачи служебной информации.
				 *
				 */
				using fields_t = vector <header_t>;
				/**
				 * @brief Тип набора HTTP-заголовков
				 *
				 * @details Набор позволяет хранить несколько одноимённых заголовков,
				 *          что необходимо для корректной работы с протоколами HTTP/1.1 и HTTP/2.
				 *          Набор также содержит псевдо-заголовки, которые используются в протоколе HTTP/2 для передачи служебной информации.
				 *
				 */
				using entries_t = unordered_multiset <header_t, header_hash_t>;
				/**
				 * @brief Тип карты HTTP-заголовков
				 *
				 * @details Карта позволяет хранить только уникальные заголовки,
				 *          что необходимо для корректной работы с протоколами HTTP/1.1 и HTTP/2.
				 *          Карта не содержит псевдо-заголовков, так-как они предназначены только для протокола HTTP/2.
				 *
				 */
				using map_t = unordered_map <string, string, header_name_hash_t, header_name_equal_t>;
				/**
				 * @brief Тип мультикарты HTTP-заголовков
				 *
				 * @details Мультикарта позволяет хранить несколько одноимённых заголовков,
				 *          что необходимо для корректной работы с протоколами HTTP/1.1 и HTTP/2.
				 *          Мультикарта не содержит псевдо-заголовков, так-как они предназначены только для протокола HTTP/2.
				 *
				 */
				using multimap_t = unordered_multimap <string, string, header_name_hash_t, header_name_equal_t>;
			public:
				/**
				 * @brief Предварительное объявление константного итератора
				 *
				 */
				class Const_Iterator;
				/**
				 * @brief Итератор как вложенный класс
				 *
				 */
				typedef class __AWH_SHARED_EXPORT__ Iterator {
					private:
						/**
						 * @brief Разрешаем доступ к позиции константному итератору
						 *
						 */
						friend class Const_Iterator;
					public:
						/**
						 * @brief Объект указателя заголовка
						 *
						 * @note Доступ только для чтения: правка названия либо значения по месту
						 *       нарушила бы учёт потребляемой памяти контейнера и каноническую
						 *       форму регистра названий, а восстановить их извне нечем.
						 *       Изменение заголовка выполняется методами контейнера -
						 *       emplace с режимом REPLACE либо erase с последующим добавлением
						 *
						 */
						using pointer = const header_t *;
						/**
						 * @brief Объект референса заголовка
						 *
						 */
						using reference = const header_t &;
					public:
						/**
						 * @brief Тип итератора заголовков
						 *
						 */
						using iterator = fields_t::iterator;
					private:
						// Текущее значение итератора
						iterator _it;
					private:
						// Объект работы с логами
						const log_t * _log;
					private:
						/**
						 * @brief Метод вывода сообщения об ошибке в лог
						 *
						 * @param func    название функции, в которой произошла ошибка
						 * @param message текст сообщения об ошибке
						 * @param flag    флаг важности сообщения
						 *
						 */
						void _error(const char * func, const char * message, const log_t::flag_t flag = log_t::flag_t::CRITICAL) const noexcept;
					public:
						/**
						 * @brief Оператор преобразования в сырой итератор
						 *
						 * @note Сырой итератор доступ к заголовку не ограничивает: он существует
						 *       для передачи позиции в алгоритмы стандартной библиотеки. Правка
						 *       заголовка через него оставит учёт потребляемой памяти контейнера
						 *       и регистр названия рассогласованными - это ответственность
						 *       вызывающей стороны, сознательно взявшей сырую позицию
						 *
						 * @note Приведение объявлено явным именно поэтому: неявное отдавало бы
						 *       сырую позицию простым присваиванием обёртки, и правка заголовка
						 *       через неё обходила бы учёт потребляемой памяти без всякого
						 *       признака сознательного выбора - тогда как указатель и референс
						 *       обёртки намеренно объявлены доступными только для чтения
						 *
						 * @return iterator итератор для преобразования
						 *
						 */
						explicit operator iterator() noexcept;
					public:
						/**
						 * @brief Оператор извлечения указателя заголовка
						 *
						 * @return указатель заголовка
						 *
						 */
						pointer operator -> () noexcept;
						/**
						 * @brief Оператор разыменования заголовка
						 *
						 * @return значение заголовка
						 *
						 */
						reference operator * () noexcept;
					public:
						/**
						 * @brief Оператор смещения вперед
						 *
						 * @return значение текущего итератора
						 *
						 */
						Iterator & operator ++ () noexcept;
					public:
						/**
						 * @brief Оператор сравнения соответствия итератора
						 *
						 * @param other итератор для сравнения
						 * @return      результат сравнения
						 *
						 */
						bool operator == (const Iterator & other) const noexcept;
						/**
						 * @brief Оператора сравнения несоответствия итератора
						 *
						 * @param other итератор для сравнения
						 * @return      результат сравнения
						 *
						 */
						bool operator != (const Iterator & other) const noexcept;
					public:
						/**
						 * @brief Оператор сравнения соответствия итератора
						 *
						 * @param other константный итератор для сравнения
						 * @return      результат сравнения
						 *
						 */
						bool operator == (const Const_Iterator & other) const noexcept;
						/**
						 * @brief Оператора сравнения несоответствия итератора
						 *
						 * @param other константный итератор для сравнения
						 * @return      результат сравнения
						 *
						 */
						bool operator != (const Const_Iterator & other) const noexcept;
					public:
						/**
						 * @brief Конструктор
						 *
						 * @param it  итератор для установки
						 * @param log объект для работы с логами
						 *
						 */
						explicit Iterator(iterator it, const log_t * log) noexcept;
				} iterator_t;
				/**
				 * @brief Константный итератор как вложенный класс
				 *
				 */
				typedef class __AWH_SHARED_EXPORT__ Const_Iterator {
					private:
						/**
						 * @brief Разрешаем доступ к позиции обычному итератору
						 *
						 */
						friend class Iterator;
					public:
						/**
						 * @brief Объект указателя заголовка
						 *
						 */
						using pointer = const header_t *;
						/**
						 * @brief Объект референса заголовка
						 *
						 */
						using reference = const header_t &;
					public:
						/**
						 * @brief Тип константного итератора заголовков
						 *
						 */
						using const_iterator = fields_t::const_iterator;
					private:
						// Текущее значение итератора
						const_iterator _it;
					private:
						// Объект работы с логами
						const log_t * _log;
					private:
						/**
						 * @brief Метод вывода сообщения об ошибке в лог
						 *
						 * @param func    название функции, в которой произошла ошибка
						 * @param message текст сообщения об ошибке
						 * @param flag    флаг важности сообщения
						 *
						 */
						void _error(const char * func, const char * message, const log_t::flag_t flag = log_t::flag_t::CRITICAL) const noexcept;
					public:
						/**
						 * @brief Оператор преобразования в сырой константный итератор
						 *
						 * @return const_iterator итератор для преобразования
						 *
						 */
						operator const_iterator() const noexcept;
					public:
						/**
						 * @brief Оператор извлечения указателя заголовка
						 *
						 * @return указатель заголовка
						 *
						 */
						pointer operator -> () const noexcept;
						/**
						 * @brief Оператор разыменования заголовка
						 *
						 * @return значение заголовка
						 *
						 */
						reference operator * () const noexcept;
					public:
						/**
						 * @brief Оператор смещения вперед
						 *
						 * @return значение текущего итератора
						 *
						 */
						Const_Iterator & operator ++ () noexcept;
					public:
						/**
						 * @brief Оператор сравнения соответствия итератора
						 *
						 * @param other итератор для сравнения
						 * @return      результат сравнения
						 *
						 */
						bool operator == (const Iterator & other) const noexcept;
						/**
						 * @brief Оператора сравнения несоответствия итератора
						 *
						 * @param other итератор для сравнения
						 * @return      результат сравнения
						 *
						 */
						bool operator != (const Iterator & other) const noexcept;
					public:
						/**
						 * @brief Оператор сравнения соответствия итератора
						 *
						 * @param other итератор для сравнения
						 * @return      результат сравнения
						 *
						 */
						bool operator == (const Const_Iterator & other) const noexcept;
						/**
						 * @brief Оператора сравнения несоответствия итератора
						 *
						 * @param other итератор для сравнения
						 * @return      результат сравнения
						 *
						 */
						bool operator != (const Const_Iterator & other) const noexcept;
					public:
						/**
						 * @brief Конструктор
						 *
						 * @param it  итератор для установки
						 * @param log объект для работы с логами
						 *
						 */
						explicit Const_Iterator(const_iterator it, const log_t * log) noexcept;
				} const_iterator_t;
			private:
				/**
				 * @brief Структура идентификации сервиса
				 *
				 */
				typedef struct __AWH_SHARED_EXPORT__ Ident {
					// Идентификатор сервиса
					string id;
					// Название сервиса
					string name;
					// Версия модуля приложения
					string version;
					/**
					 * @brief Конструктор
					 *
					 */
					explicit Ident() noexcept;
				} ident_t;
				/**
				 * @brief Структура параметров максимальных значений
				 *
				 */
				typedef struct __AWH_SHARED_EXPORT__ Max {
					// Максимальный размер выделения памяти
					size_t memory;
					// Максимальное количество добавляемых записей
					size_t records;
					/**
					 * @brief Конструктор
					 *
					 */
					explicit Max() noexcept;
				} max_t;
			private:
				// Размеры максимальныйх ограничений
				max_t _max;
			private:
				// Идентификация сервиса
				ident_t _ident;
			private:
				// Набор установленных HTTP-заголовков
				fields_t _headers;
			private:
				// Текущий размер потребляемой памяти (полезная нагрузка всех заголовков)
				size_t _memory = 0;
			private:
				// Протокол HTTP-запроса/ответа
				proto_t _proto = proto_t::NONE;
			private:
				// Объект провайдера HTTP-запроса/ответа
				unique_ptr <provider_t> _provider = nullptr;
			private:
				// Объект фреймворка
				const fmk_t * _fmk = nullptr;
				// Объект работы с логами
				const log_t * _log = nullptr;
			private:
				/**
				 * @brief Метод приведения названий всех заголовков к канонической форме текущего протокола
				 *
				 * @details Для протоколов семейства HTTP/2 названия приводятся к нижнему регистру, для остальных -
				 *          к «умному» регистру. Вызывается при изменении протокола, чтобы единая семантика
				 *          регистра соблюдалась при любом способе доступа к заголовкам (не только при печати).
				 *
				 */
				void _recase() noexcept;
				/**
				 * @brief Метод проверки, поместится ли заголовок при замене одноимённых
				 *
				 * @details Замена состоит из удаления прежних вхождений и добавления нового значения.
				 *          Если новое значение в ограничения не помещается, удаление уже произошло,
				 *          и заголовок исчезает из набора целиком. Проверка выполняется до удаления
				 *          по состоянию, каким оно будет после него, и в таком случае замена не начинается.
				 *
				 * @param name    название заменяемого заголовка
				 * @param payload объём полезной нагрузки нового значения (название и значение)
				 * @return        результат проверки
				 *
				 */
				bool _fits(string_view name, const size_t payload) const noexcept;
				/**
				 * @brief Метод приведения набора заголовков в соответствие ограничениям
				 *
				 * @details Отбрасывает заголовки с конца набора, пока он не уложится
				 *          в ограничения по количеству записей и объёму памяти. Вызывается
				 *          при понижении ограничений: без этого набор, собранный до их
				 *          установки, остался бы сверх предела, а сам предел - невыполненным
				 *
				 */
				void _trim() noexcept;
				/**
				 * @brief Метод сборки набора заголовков в виде, пригодном для указанного протокола
				 *
				 * @details Для протоколов семейства HTTP/2 к набору добавляются псевдозаголовки,
				 *          заголовок адресата переносится в [:authority], а поля, которых
				 *          в сообщении быть не может, отсеиваются: управляющие соединением
				 *          (RFC 9113 §8.2.2, RFC 9114 §4.2), перечисленные в значении заголовка
				 *          Connection, [TE] с недопустимым значением и псевдозаголовки, уже
				 *          собранные из провайдера. Для остальных протоколов набор отдаётся как есть.
				 *
				 * @note Метод един для печати и для операторов преобразования: расхождение между
				 *       ними означало бы, что вид набора зависит от способа его получения,
				 *       и собранное из одного отвергалось бы проверкой другого
				 *
				 * @param proto протокол, под который собирается набор
				 * @return      собранный набор заголовков
				 *
				 */
				fields_t _compose(const http::proto_t proto) const noexcept;
			private:
				/**
				 * @brief Шаблон добавления нового заголовка
				 *
				 * @tparam Name    тип названия добавляемого заголовка
				 * @tparam Content тип содержимого добавляемого заголовка
				 *
				 */
				template <typename Name, typename Content>
				/**
				 * @brief Метод добавления нового заголовка
				 *
				 * @param name    название заголовка
				 * @param content содержимое заголовка
				 * @return        общее количество заголовков
				 *
				 */
				size_t _emplace(Name && name, Content && content) noexcept;
			private:
				/**
				 * @brief Метод вывода сообщения об ошибке в лог
				 *
				 * @param func    название функции, в которой произошла ошибка
				 * @param message текст сообщения об ошибке
				 * @param flag    флаг важности сообщения
				 *
				 */
				void _error(const char * func, const char * message, const log_t::flag_t flag = log_t::flag_t::CRITICAL) const noexcept;
			public:
				/**
				 * @brief Метод очистки всех данных заголовков
				 *
				 */
				void clear() noexcept;
			public:
				/**
				 * @brief Метод полной очистки памяти
				 *
				 */
				void reset() noexcept;
			public:
				/**
				 * @brief Метод проверки на заполненность заголовков
				 *
				 * @note Отвечает о наборе полей и только о нём: контейнер с установленным
				 *       провайдером, но без полей считается пустым. Провайдер описывает
				 *       стартовую строку либо псевдозаголовки, а не поля, и наличие
				 *       сообщения проверяется методом provider()
				 *
				 * @return результат проверки
				 *
				 */
				bool empty() const noexcept;
			public:
				/**
				 * @brief Метод добавления стандартных заголовков по умолчанию
				 *
				 * @details Добавляет недостающие заголовки: User-Agent для запроса клиента,
				 *          Server, X-Powered-By и Date для ответа сервера. Уже установленные заголовки не изменяются.
				 *
				 * @return результат выполнения операции
				 *
				 */
				bool addDefaultHeaders() noexcept;
			public:
				/**
				 * @brief Метод получения текущей даты для HTTP-запроса
				 *
				 * @note Unix Timestamp - количество секунд с 1 января 1970 года
				 *
				 * @param date дата в формате Unix Timestamp
				 * @return     штамп времени в текстовом виде
				 *
				 */
				string date(const uint64_t date = 0) const noexcept;
			public:
				/**
				 * @brief Метод получения протокола HTTP-запроса/ответа
				 *
				 * @return протокол HTTP-запроса/ответа
				 *
				 */
				proto_t proto() const noexcept;
				/**
				 * @brief Метод установки протокола HTTP-запроса/ответа
				 *
				 * @param proto протокол HTTP-запроса/ответа
				 *
				 */
				void proto(const proto_t proto) noexcept;
			public:
				/**
				 * @brief Метод получения идентификации сервиса
				 *
				 * @return сформированный агент
				 *
				 */
				string ident() const noexcept;
				/**
				 * @brief Метод установки идентификации сервиса
				 *
				 * @details Составляющие попадают в значение поля целиком: у запроса - в
				 *          User-Agent, у ответа - в X-Powered-By. Составляющая, содержащая
				 *          октеты, недопустимые в значении поля (управляющие символы и DEL),
				 *          не применяется вовсе и записывается в лог: приложение берёт эти
				 *          строки извне, а возврат каретки либо перевод строки внутри них
				 *          расщепил бы сообщение на два. Пустая составляющая оставляет
				 *          прежнее значение, а непригодная одна не отменяет остальных.
				 *
				 * @param id      идентификатор сервиса
				 * @param name    название сервиса
				 * @param version версия сервиса
				 *
				 */
				void ident(string_view id, string_view name, string_view version) noexcept;
			public:
				/**
				 * @brief Метод получения объекта провайдера HTTP-запроса/ответа
				 *
				 * @return объект провайдера HTTP-запроса/ответа
				 *
				 */
				const provider_t * provider() const noexcept;
				/**
				 * @brief Метод получения объекта провайдера HTTP-запроса/ответа
				 *
				 * @param provider объект провайдера HTTP-запроса/ответа
				 * @return         результат выполнения операции
				 *
				 */
				bool provider(unique_ptr <provider_t> & provider) const noexcept;
			public:
				/**
				 * @brief Метод установки объекта провайдера HTTP-запроса/ответа
				 *
				 * @param provider объект провайдера HTTP-запроса/ответа
				 *
				 */
				void provider(const provider_t * provider) noexcept;
				/**
				 * @brief Метод установки объекта провайдера HTTP-запроса/ответа
				 *
				 * @param provider объект провайдера HTTP-запроса/ответа
				 *
				 */
				void provider(unique_ptr <provider_t> && provider) noexcept;
			public:
				/**
				 * @brief Метод получения стартовой строки HTTP-запроса/ответа
				 *
				 * @return стартовая строка HTTP-запроса/ответа
				 *
				 */
				string startline() const noexcept;
				/**
				 * @brief Метод установки стартовой строки HTTP-запроса/ответа
				 *
				 * @param startline стартовая строка HTTP-запроса/ответа
				 *
				 */
				void startline(const string_view startline) noexcept;
			public:
				/**
				 * @brief Метод удаления заголовка
				 *
				 * @param name название удаляемого заголовка
				 *
				 */
				void erase(string_view name) noexcept;
				/**
				 * @brief erase Метод удаления заголовка
				 *
				 * @param it идетартор заголовка для удаления
				 * @return   следующий итератор
				 *
				 */
				iterator_t erase(const iterator_t & it) noexcept;
			public:
				/**
				 * @brief Метод проверки существования заголовка
				 *
				 * @param name название заголовка для проверки
				 * @return     результат выполнения проверки
				 *
				 */
				bool has(string_view name) const noexcept;
			public:
				/**
				 * @brief Метод получения общего количества заголовков
				 *
				 * @return общее количество заголовков
				 *
				 */
				size_t size() const noexcept;
			public:
				/**
				 * @brief Количество добавленных заголовков
				 *
				 * @param name название заголовка количество которых нужно определить
				 * @return     количество добавленных заголовков
				 *
				 */
				size_t count(string_view name = "") const noexcept;
			public:
				/**
				 * @brief Метод извлечения содержимого заголовка
				 *
				 * @param name название заголовка
				 * @return     содержимое заголовка
				 *
				 */
				const string & at(string_view name) const noexcept;
			public:
				/**
				 * @brief Метод извлечения названий заголовков
				 *
				 * @return список названий заголовков
				 *
				 */
				vector <string> names() const noexcept;
			public:
				/**
				 * @brief Метод вывода списка значений одинаковых заголовков
				 *
				 * @param name название заголовка
				 * @return     список значений одинаковых заголовков
				 *
				 */
				vector <string> range(string_view name) const noexcept;
			public:
				/**
				 * @brief Метод добавления или замены заголовка
				 *
				 * @note В режиме REPLACE (по умолчанию) все прежние вхождения заголовка с указанным
				 *       именем заменяются новым значением. В режиме APPEND новый заголовок добавляется,
				 *       сохраняя существующие одноимённые заголовки.
				 *
				 * @param name    название заголовка
				 * @param content содержимое заголовка
				 * @param mode    режим добавления заголовка (добавить/заменить)
				 * @return        общее количество заголовков
				 *
				 */
				size_t emplace(string && name, string && content, const mode_t mode = mode_t::REPLACE) noexcept;
				/**
				 * @brief Метод добавления или замены заголовка
				 *
				 * @note В режиме REPLACE (по умолчанию) все прежние вхождения заголовка с указанным
				 *       именем заменяются новым значением. В режиме APPEND новый заголовок добавляется,
				 *       сохраняя существующие одноимённые заголовки.
				 *
				 * @param name    название заголовка (C-строка)
				 * @param content содержимое заголовка (переносится)
				 * @param mode    режим добавления заголовка (добавить/заменить)
				 * @return        общее количество заголовков
				 *
				 */
				size_t emplace(const char * name, string && content, const mode_t mode = mode_t::REPLACE) noexcept;
				/**
				 * @brief Метод добавления или замены заголовка
				 *
				 * @note В режиме REPLACE (по умолчанию) все прежние вхождения заголовка с указанным
				 *       именем заменяются новым значением. В режиме APPEND новый заголовок добавляется,
				 *       сохраняя существующие одноимённые заголовки.
				 *
				 * @param name    название заголовка (переносится)
				 * @param content содержимое заголовка (C-строка)
				 * @param mode    режим добавления заголовка (добавить/заменить)
				 * @return        общее количество заголовков
				 *
				 */
				size_t emplace(string && name, const char * content, const mode_t mode = mode_t::REPLACE) noexcept;
				/**
				 * @brief Метод добавления или замены заголовка
				 *
				 * @note В режиме REPLACE (по умолчанию) все прежние вхождения заголовка с указанным
				 *       именем заменяются новым значением. В режиме APPEND новый заголовок добавляется,
				 *       сохраняя существующие одноимённые заголовки.
				 *
				 * @param name    название заголовка
				 * @param content содержимое заголовка
				 * @param mode    режим добавления заголовка (добавить/заменить)
				 * @return        общее количество заголовков
				 *
				 */
				size_t emplace(string_view name, string_view content, const mode_t mode = mode_t::REPLACE) noexcept;
				/**
				 * @brief Метод добавления или замены заголовка
				 *
				 * @note В режиме REPLACE (по умолчанию) все прежние вхождения заголовка с указанным
				 *       именем заменяются новым значением. В режиме APPEND новый заголовок добавляется,
				 *       сохраняя существующие одноимённые заголовки.
				 *
				 * @param name    название заголовка (переносится)
				 * @param content содержимое заголовка (копируется)
				 * @param mode    режим добавления заголовка (добавить/заменить)
				 * @return        общее количество заголовков
				 *
				 */
				size_t emplace(string && name, const string & content, const mode_t mode = mode_t::REPLACE) noexcept;
				/**
				 * @brief Метод добавления или замены заголовка
				 *
				 * @note В режиме REPLACE (по умолчанию) все прежние вхождения заголовка с указанным
				 *       именем заменяются новым значением. В режиме APPEND новый заголовок добавляется,
				 *       сохраняя существующие одноимённые заголовки.
				 *
				 * @param name    название заголовка (копируется)
				 * @param content содержимое заголовка (переносится)
				 * @param mode    режим добавления заголовка (добавить/заменить)
				 * @return        общее количество заголовков
				 *
				 */
				size_t emplace(const string & name, string && content, const mode_t mode = mode_t::REPLACE) noexcept;
				/**
				 * @brief Метод добавления или замены заголовка
				 *
				 * @note В режиме REPLACE (по умолчанию) все прежние вхождения заголовка с указанным
				 *       именем заменяются новым значением. В режиме APPEND новый заголовок добавляется,
				 *       сохраняя существующие одноимённые заголовки.
				 *
				 * @param name    название заголовка (C-строка)
				 * @param content содержимое заголовка (C-строка)
				 * @param mode    режим добавления заголовка (добавить/заменить)
				 * @return        общее количество заголовков
				 *
				 */
				size_t emplace(const char * name, const char * content, const mode_t mode = mode_t::REPLACE) noexcept;
				/**
				 * @brief Метод добавления или замены заголовка
				 *
				 * @note В режиме REPLACE (по умолчанию) все прежние вхождения заголовка с указанным
				 *       именем заменяются новым значением. В режиме APPEND новый заголовок добавляется,
				 *       сохраняя существующие одноимённые заголовки.
				 *
				 * @param name    название заголовка (C-строка)
				 * @param content содержимое заголовка (копируется)
				 * @param mode    режим добавления заголовка (добавить/заменить)
				 * @return        общее количество заголовков
				 *
				 */
				size_t emplace(const char * name, const string & content, const mode_t mode = mode_t::REPLACE) noexcept;
				/**
				 * @brief Метод добавления или замены заголовка
				 *
				 * @note В режиме REPLACE (по умолчанию) все прежние вхождения заголовка с указанным
				 *       именем заменяются новым значением. В режиме APPEND новый заголовок добавляется,
				 *       сохраняя существующие одноимённые заголовки.
				 *
				 * @param name    название заголовка (копируется)
				 * @param content содержимое заголовка (C-строка)
				 * @param mode    режим добавления заголовка (добавить/заменить)
				 * @return        общее количество заголовков
				 *
				 */
				size_t emplace(const string & name, const char * content, const mode_t mode = mode_t::REPLACE) noexcept;
				/**
				 * @brief Метод добавления или замены заголовка
				 *
				 * @note В режиме REPLACE (по умолчанию) все прежние вхождения заголовка с указанным
				 *       именем заменяются новым значением. В режиме APPEND новый заголовок добавляется,
				 *       сохраняя существующие одноимённые заголовки.
				 *
				 * @param name    название заголовка (копируется)
				 * @param content содержимое заголовка (копируется)
				 * @param mode    режим добавления заголовка (добавить/заменить)
				 * @return        общее количество заголовков
				 *
				 */
				size_t emplace(const string & name, const string & content, const mode_t mode = mode_t::REPLACE) noexcept;
			public:
				/**
				 * @brief Метод печати содержимого заголовков в формате HTTP
				 *
				 * @return заголовки в формате HTTP
				 *
				 */
				string print(const http::proto_t proto) const noexcept;
				/**
				 * @brief Метод печати содержимого заголовков в формате протокола контейнера
				 *
				 * @note Протокол берётся у самого контейнера, а не подставляется значением
				 *       по умолчанию: умолчание HTTP/1 давало бы для контейнера HTTP/2
				 *       вид другого протокола, расходясь с оператором преобразования в строку
				 *
				 * @return заголовки в формате HTTP
				 *
				 */
				string print() const noexcept;
				/**
				 * @brief Метод печати содержимого заголовка
				 *
				 * @param name  печать заголовка в формате HTTP
				 * @param proto версия протокола
				 * @return      распечатанный заголовок
				 *
				 */
				string print(string_view name, const http::proto_t proto) const noexcept;
				/**
				 * @brief Метод печати содержимого заголовка в формате протокола контейнера
				 *
				 * @param name название печатаемого заголовка
				 * @return     распечатанный заголовок
				 *
				 */
				string print(string_view name) const noexcept;
			public:
				/**
				 * @brief Метод получения текущего размера потребляемой памяти
				 *
				 * @return текущий размер потребляемой памяти
				 *
				 */
				size_t memory() const noexcept;
			public:
				/**
				 * @brief Метод получения максимального размера потребления памяти
				 *
				 * @return максимальный размер потребления памяти
				 *
				 */
				size_t maxMemory() const noexcept;
				/**
				 * @brief Метод установки максимального размера потребления памяти
				 *
				 * @details Если новое ограничение меньше уже занятого объёма, лишние заголовки
				 *          отбрасываются с конца набора, а в лог записывается предупреждение
				 *          с их количеством. Иначе ограничение не выполнялось бы на наборе,
				 *          собранном до его установки, и контейнер оставался бы сверх предела
				 *          вплоть до полной очистки
				 *
				 * @param size максимальный размер потребления памяти
				 *
				 */
				void maxMemory(const size_t size) noexcept;
			public:
				/**
				 * @brief Метод получения максимального количества заголовков
				 *
				 * @return максимальное количество заголовков
				 *
				 */
				size_t maxRecords() const noexcept;
				/**
				 * @brief Метод установки максимального количества заголовков
				 *
				 * @details Если новое ограничение меньше числа уже добавленных заголовков,
				 *          лишние отбрасываются с конца набора, а в лог записывается
				 *          предупреждение с их количеством
				 *
				 * @param count максимальное количество заголовков
				 *
				 */
				void maxRecords(const size_t count) noexcept;
			public:
				/**
				 * @brief Метод обмена заголовками
				 *
				 * @param headers заголовки для обмена
				 *
				 */
				void swap(Headers & headers) noexcept;
				/**
				 * @brief Метод слияния заголовков
				 *
				 * @details В режиме APPEND заголовки переданного контейнера дописываются
				 *          к текущему набору с сохранением одноимённых, в режиме REPLACE
				 *          одноимённые заменяются переданными. Режим по умолчанию - APPEND:
				 *          у полей, законно встречающихся несколько раз, замена потеряла бы
				 *          прежние значения, а слияние по своему смыслу их сохраняет
				 *
				 * @note Полю, встречающемуся в сообщении единожды - Content-Length, Host, -
				 *       умолчание даёт дубликат: слияние не знает, какие поля одиночные.
				 *       Для таких полей передаётся режим REPLACE
				 *
				 * @param headers заголовки для слияния
				 * @param mode    режим слияния заголовков (добавить/заменить)
				 *
				 */
				void merge(const Headers & headers, const mode_t mode = mode_t::APPEND) noexcept;
			public:
				/**
				 * @brief Метод получения конечного итератора
				 *
				 * @return конечный итератор
				 *
				 */
				iterator_t end() noexcept;
				/**
				 * @brief Метод получения конечного константного итератора
				 *
				 * @return конечный константный итератор
				 *
				 */
				const_iterator_t end() const noexcept;
				/**
				 * @brief Метод получения конечного константного итератора
				 *
				 * @return конечный константный итератор
				 *
				 */
				const_iterator_t cend() const noexcept;
			public:
				/**
				 * @brief Метод получение начального итератора
				 *
				 * @return начальный итератор
				 *
				 */
				iterator_t begin() noexcept;
				/**
				 * @brief Метод получения начального константного итератора
				 *
				 * @return начальный константный итератор
				 *
				 */
				const_iterator_t begin() const noexcept;
				/**
				 * @brief Метод получения начального константного итератора
				 *
				 * @return начальный константный итератор
				 *
				 */
				const_iterator_t cbegin() const noexcept;
			public:
				/**
				 * @brief Метод поиска указанного заголовка
				 *
				 * @param name название заголовка для поиска
				 * @return     итератор указанного заголовка
				 *
				 */
				iterator_t find(string_view name) noexcept;
				/**
				 * @brief Метод поиска указанного заголовка
				 *
				 * @param name название заголовка для поиска
				 * @return     константный итератор указанного заголовка
				 *
				 */
				const_iterator_t find(string_view name) const noexcept;
			public:
				/**
				 * @brief Оператор получения количество заголовков
				 *
				 * @return количество заголовков
				 *
				 */
				explicit operator size_t() const noexcept;
				/**
				 * @brief Оператор печати содержимого заголовков в формате HTTP
				 *
				 * @return заголовки в формате HTTP
				 *
				 */
				explicit operator string() const noexcept;
			public:
				/**
				 * @brief Оператор получения протокола HTTP-запроса/ответа
				 *
				 * @return протокол HTTP-запроса/ответа
				 *
				 */
				explicit operator proto_t() const noexcept;
			public:
				/**
				 * @brief Оператор получения объекта провайдера HTTP-запроса/ответа
				 *
				 * @return объект провайдера HTTP-запроса/ответа
				 *
				 */
				explicit operator const provider_t * () const noexcept;
				/**
				 * @brief Оператор получения объекта провайдера HTTP-запроса/ответа
				 *
				 * @return объект провайдера HTTP-запроса/ответа
				 *
				 */
				explicit operator unique_ptr <provider_t> () const noexcept;
			public:
				/**
				 * @brief Оператор получения списка заголовков в виде сообщения
				 *
				 * @details Отдаёт набор таким, каким он уйдёт на провод: для протоколов
				 *          семейства HTTP/2 добавляются псевдозаголовки, заголовок адресата
				 *          переносится в [:authority], отсеиваются поля, которых в сообщении
				 *          быть не может. Тот же вид даёт печать
				 *
				 * @note Отличие от map_t и multimap_t намеренно: те отдают вид хранилища,
				 *       содержимое как есть. Подробности и причина - в README модуля,
				 *       раздел «Два вида одного контейнера»
				 *
				 * @return список заголовков в виде сообщения
				 *
				 */
				explicit operator fields_t() const noexcept;
				/**
				 * @brief Оператор получения набора заголовков в виде сообщения
				 *
				 * @details Состав собирается тем же способом, что и список заголовков:
				 *          с псевдозаголовками и с отсевом полей, которых в сообщении
				 *          быть не может
				 *
				 * @note Порядок при этом теряется: набор неупорядоченный. Для полей
				 *       одного названия порядок несёт смысл (RFC 9110 §5.3), поэтому
				 *       восстановить по нему сообщение нельзя - он отвечает на вопрос
				 *       о составе и кратности, а не о последовательности. Сообщение
				 *       целиком отдаёт список полей либо печать
				 *
				 * @return набор заголовков в виде сообщения
				 *
				 */
				explicit operator entries_t() const noexcept;
			public:
				/**
				 * @brief Оператор получения карты заголовков в виде хранилища
				 *
				 * @details Отдаёт содержимое как есть, без псевдозаголовков и без отсева:
				 *          по нему вызывающая сторона находит то, что сама же и положила.
				 *          Карта хранит единственное значение на название
				 *
				 * @note Расхождение с fields_t и entries_t намеренно - см. README модуля,
				 *       раздел «Два вида одного контейнера»
				 *
				 * @return карта заголовков в виде хранилища
				 *
				 */
				explicit operator map_t() const noexcept;
				/**
				 * @brief Оператор получения мультикарты заголовков в виде хранилища
				 *
				 * @details Отдаёт содержимое как есть, сохраняя одноимённые заголовки
				 *
				 * @note Расхождение с fields_t и entries_t намеренно - см. README модуля,
				 *       раздел «Два вида одного контейнера»
				 *
				 * @return мультикарта заголовков в виде хранилища
				 *
				 */
				explicit operator multimap_t() const noexcept;
			public:
				/**
				 * @brief Оператор извлечения содержимого заголовка
				 *
				 * @details Намеренно только для чтения: запись через operator[] не поддерживается,
				 *          так как контейнер допускает несколько заголовков с одним именем
				 *          (unordered_multiset) и однозначная семантика записи невозможна.
				 *          Для добавления/замены используйте emplace().
				 *
				 * @param name название заголовка для извлечения
				 * @return     содержимое заголовка
				 *
				 */
				const string & operator[](string_view name) const noexcept;
			public:
				/**
				 * @brief Оператор слияния заголовков
				 *
				 * @param headers заголовки для слияния
				 * @return        текущий контейнер заголовков
				 *
				 */
				Headers & operator += (const Headers & headers) noexcept;
			public:
				/**
				 * @brief Оператор сравнения двух заголовков
				 *
				 * @param headers заголовки для сравнения
				 * @return        результат сравнения
				 *
				 */
				bool operator == (const Headers & headers) const noexcept;
				/**
				 * @brief Оператор несравнения двух заголовков
				 *
				 * @param headers заголовки для сравнения
				 * @return        результат сравнения
				 *
				 */
				bool operator != (const Headers & headers) const noexcept;
			public:
				/**
				 * @brief Оператор перемещения
				 *
				 * @param headers заголовки для перемещения
				 * @return        текущий контейнер заголовков
				 *
				 */
				Headers & operator = (Headers && headers) noexcept;
				/**
				 * @brief Оператор копирования
				 *
				 * @param headers заголовки для копирования
				 * @return        текущий контейнер заголовков
				 *
				 */
				Headers & operator = (const Headers & headers) noexcept;
			public:
				/**
				 * @brief Оператор установки протокола HTTP-запроса/ответа
				 *
				 * @param proto протокол HTTP-запроса/ответа
				 * @return      текущий контейнер заголовков
				 *
				 */
				Headers & operator = (const proto_t proto) noexcept;
				/**
				 * @brief Оператор установки объекта провайдера HTTP-запроса/ответа
				 *
				 * @param provider объект провайдера HTTP-запроса/ответа
				 * @return         текущий контейнер заголовков
				 *
				 */
				Headers & operator = (const provider_t * provider) noexcept;
				/**
				 * @brief Оператор установки объекта провайдера HTTP-запроса/ответа
				 *
				 * @param provider объект провайдера HTTP-запроса/ответа
				 * @return         текущий контейнер заголовков
				 *
				 */
				Headers & operator = (unique_ptr <provider_t> && provider) noexcept;
			public:
				/**
				 * @brief Оператор копирования
				 *
				 * @param headers заголовки для копирования
				 * @return        текущий контейнер заголовков
				 *
				 */
				Headers & operator = (const fields_t & headers) noexcept;
				/**
				 * @brief Оператор копирования
				 *
				 * @param headers заголовки для копирования
				 * @return        текущий контейнер заголовков
				 *
				 */
				Headers & operator = (const entries_t & headers) noexcept;
				/**
				 * @brief Оператор копирования
				 *
				 * @param headers заголовки для копирования
				 * @return        текущий контейнер заголовков
				 *
				 */
				Headers & operator = (const multimap_t & headers) noexcept;
				/**
				 * @brief Оператор копирования
				 *
				 * @param headers заголовки для копирования
				 * @return        текущий контейнер заголовков
				 *
				 */
				Headers & operator = (initializer_list <header_t> headers) noexcept;
			public:
				/**
				 * @brief Разрешаем пустое значение объекта
				 *
				 */
				explicit Headers() = default;
			public:
				/**
				 * @brief Конструктор перемещения
				 *
				 * @param headers заголовки для перемещения
				 *
				 */
				Headers(Headers && headers) noexcept;
				/**
				 * @brief Конструктор копирования
				 *
				 * @param headers заголовки для копирования
				 *
				 */
				Headers(const Headers & headers) noexcept;
			public:
				/**
				 * @brief Конструктор
				 *
				 * @param proto протокол HTTP-запроса/ответа
				 *
				 */
				explicit Headers(const proto_t proto) noexcept;
				/**
				 * @brief Конструктор
				 *
				 * @param provider объект провайдера HTTP-запроса/ответа
				 *
				 */
				explicit Headers(const provider_t * provider) noexcept;
				/**
				 * @brief Конструктор
				 *
				 * @param provider объект провайдера HTTP-запроса/ответа
				 *
				 */
				explicit Headers(unique_ptr <provider_t> && provider) noexcept;
			public:
				/**
				 * @brief Конструктор
				 *
				 * @param headers список заголовков инициализации
				 *
				 */
				explicit Headers(const fields_t & headers) noexcept;
				/**
				 * @brief Конструктор
				 *
				 * @param headers список заголовков инициализации
				 *
				 */
				explicit Headers(const entries_t & headers) noexcept;
				/**
				 * @brief Конструктор
				 *
				 * @param headers список заголовков инициализации
				 *
				 */
				explicit Headers(const multimap_t & headers) noexcept;
				/**
				 * @brief Конструктор
				 *
				 * @param headers список заголовков инициализации
				 *
				 */
				explicit Headers(initializer_list <header_t> headers) noexcept;
			public:
				/**
				 * @brief Конструктор
				 *
				 * @param proto   протокол HTTP-запроса/ответа
				 * @param headers список заголовков инициализации
				 *
				 */
				explicit Headers(const proto_t proto, const fields_t & headers) noexcept;
				/**
				 * @brief Конструктор
				 *
				 * @param proto   протокол HTTP-запроса/ответа
				 * @param headers список заголовков инициализации
				 *
				 */
				explicit Headers(const proto_t proto, const entries_t & headers) noexcept;
				/**
				 * @brief Конструктор
				 *
				 * @param proto   протокол HTTP-запроса/ответа
				 * @param headers список заголовков инициализации
				 *
				 */
				explicit Headers(const proto_t proto, const multimap_t & headers) noexcept;
				/**
				 * @brief Конструктор
				 *
				 * @param proto   протокол HTTP-запроса/ответа
				 * @param headers список заголовков инициализации
				 *
				 */
				explicit Headers(const proto_t proto, initializer_list <header_t> headers) noexcept;
			public:
				/**
				 * @brief Конструктор
				 *
				 * @param provider объект провайдера HTTP-запроса/ответа
				 * @param headers  список заголовков инициализации
				 *
				 */
				explicit Headers(const provider_t * provider, const fields_t & headers) noexcept;
				/**
				 * @brief Конструктор
				 *
				 * @param provider объект провайдера HTTP-запроса/ответа
				 * @param headers  список заголовков инициализации
				 *
				 */
				explicit Headers(const provider_t * provider, const entries_t & headers) noexcept;
				/**
				 * @brief Конструктор
				 *
				 * @param provider объект провайдера HTTP-запроса/ответа
				 * @param headers  список заголовков инициализации
				 *
				 */
				explicit Headers(const provider_t * provider, const multimap_t & headers) noexcept;
				/**
				 * @brief Конструктор
				 *
				 * @param provider объект провайдера HTTP-запроса/ответа
				 * @param headers  список заголовков инициализации
				 *
				 */
				explicit Headers(const provider_t * provider, initializer_list <header_t> headers) noexcept;
			public:
				/**
				 * @brief Конструктор
				 *
				 * @param provider объект провайдера HTTP-запроса/ответа
				 * @param headers  список заголовков инициализации
				 *
				 */
				explicit Headers(unique_ptr <provider_t> && provider, const fields_t & headers) noexcept;
				/**
				 * @brief Конструктор
				 *
				 * @param provider объект провайдера HTTP-запроса/ответа
				 * @param headers  список заголовков инициализации
				 *
				 */
				explicit Headers(unique_ptr <provider_t> && provider, const entries_t & headers) noexcept;
				/**
				 * @brief Конструктор
				 *
				 * @param provider объект провайдера HTTP-запроса/ответа
				 * @param headers  список заголовков инициализации
				 *
				 */
				explicit Headers(unique_ptr <provider_t> && provider, const multimap_t & headers) noexcept;
				/**
				 * @brief Конструктор
				 *
				 * @param provider объект провайдера HTTP-запроса/ответа
				 * @param headers  список заголовков инициализации
				 *
				 */
				explicit Headers(unique_ptr <provider_t> && provider, initializer_list <header_t> headers) noexcept;
			public:
				/**
				 * @brief Конструктор
				 *
				 * @param proto    протокол HTTP-запроса/ответа
				 * @param provider объект провайдера HTTP-запроса/ответа
				 * @param headers  список заголовков инициализации
				 *
				 */
				explicit Headers(const proto_t proto, const provider_t * provider, const fields_t & headers) noexcept;
				/**
				 * @brief Конструктор
				 *
				 * @param proto    протокол HTTP-запроса/ответа
				 * @param provider объект провайдера HTTP-запроса/ответа
				 * @param headers  список заголовков инициализации
				 *
				 */
				explicit Headers(const proto_t proto, const provider_t * provider, const entries_t & headers) noexcept;
				/**
				 * @brief Конструктор
				 *
				 * @param proto    протокол HTTP-запроса/ответа
				 * @param provider объект провайдера HTTP-запроса/ответа
				 * @param headers  список заголовков инициализации
				 *
				 */
				explicit Headers(const proto_t proto, const provider_t * provider, const multimap_t & headers) noexcept;
				/**
				 * @brief Конструктор
				 *
				 * @param proto    протокол HTTP-запроса/ответа
				 * @param provider объект провайдера HTTP-запроса/ответа
				 * @param headers  список заголовков инициализации
				 *
				 */
				explicit Headers(const proto_t proto, const provider_t * provider, initializer_list <header_t> headers) noexcept;
			public:
				/**
				 * @brief Конструктор
				 *
				 * @param proto    протокол HTTP-запроса/ответа
				 * @param provider объект провайдера HTTP-запроса/ответа
				 * @param headers  список заголовков инициализации
				 *
				 */
				explicit Headers(const proto_t proto, unique_ptr <provider_t> && provider, const fields_t & headers) noexcept;
				/**
				 * @brief Конструктор
				 *
				 * @param proto    протокол HTTP-запроса/ответа
				 * @param provider объект провайдера HTTP-запроса/ответа
				 * @param headers  список заголовков инициализации
				 *
				 */
				explicit Headers(const proto_t proto, unique_ptr <provider_t> && provider, const entries_t & headers) noexcept;
				/**
				 * @brief Конструктор
				 *
				 * @param proto    протокол HTTP-запроса/ответа
				 * @param provider объект провайдера HTTP-запроса/ответа
				 * @param headers  список заголовков инициализации
				 *
				 */
				explicit Headers(const proto_t proto, unique_ptr <provider_t> && provider, const multimap_t & headers) noexcept;
				/**
				 * @brief Конструктор
				 *
				 * @param proto    протокол HTTP-запроса/ответа
				 * @param provider объект провайдера HTTP-запроса/ответа
				 * @param headers  список заголовков инициализации
				 *
				 */
				explicit Headers(const proto_t proto, unique_ptr <provider_t> && provider, initializer_list <header_t> headers) noexcept;
			public:
				/**
				 * @brief Конструктор
				 *
				 * @param fmk объект фреймворка
				 * @param log объект для работы с логами
				 *
				 */
				explicit Headers(const fmk_t * fmk, const log_t * log) noexcept;
				/**
				 * @brief Конструктор
				 *
				 * @param proto протокол HTTP-запроса/ответа
				 *
				 */
				explicit Headers(const proto_t proto, const fmk_t * fmk, const log_t * log) noexcept;
				/**
				 * @brief Конструктор
				 *
				 * @param provider объект провайдера HTTP-запроса/ответа
				 *
				 */
				explicit Headers(const provider_t * provider, const fmk_t * fmk, const log_t * log) noexcept;
				/**
				 * @brief Конструктор
				 *
				 * @param provider объект провайдера HTTP-запроса/ответа
				 *
				 */
				explicit Headers(unique_ptr <provider_t> && provider, const fmk_t * fmk, const log_t * log) noexcept;
			public:
				/**
				 * @brief Конструктор
				 *
				 * @param headers список заголовков инициализации
				 * @param fmk     объект фреймворка
				 * @param log     объект для работы с логами
				 *
				 */
				explicit Headers(const fields_t & headers, const fmk_t * fmk, const log_t * log) noexcept;
				/**
				 * @brief Конструктор
				 *
				 * @param headers список заголовков инициализации
				 * @param fmk     объект фреймворка
				 * @param log     объект для работы с логами
				 *
				 */
				explicit Headers(const entries_t & headers, const fmk_t * fmk, const log_t * log) noexcept;
				/**
				 * @brief Конструктор
				 *
				 * @param headers список заголовков инициализации
				 * @param fmk     объект фреймворка
				 * @param log     объект для работы с логами
				 *
				 */
				explicit Headers(const multimap_t & headers, const fmk_t * fmk, const log_t * log) noexcept;
				/**
				 * @brief Конструктор
				 *
				 * @param headers список заголовков инициализации
				 * @param fmk     объект фреймворка
				 * @param log     объект для работы с логами
				 *
				 */
				explicit Headers(initializer_list <header_t> headers, const fmk_t * fmk, const log_t * log) noexcept;
			public:
				/**
				 * @brief Конструктор
				 *
				 * @param proto   протокол HTTP-запроса/ответа
				 * @param headers список заголовков инициализации
				 * @param fmk     объект фреймворка
				 * @param log     объект для работы с логами
				 *
				 */
				explicit Headers(const proto_t proto, const fields_t & headers, const fmk_t * fmk, const log_t * log) noexcept;
				/**
				 * @brief Конструктор
				 *
				 * @param proto   протокол HTTP-запроса/ответа
				 * @param headers список заголовков инициализации
				 * @param fmk     объект фреймворка
				 * @param log     объект для работы с логами
				 *
				 */
				explicit Headers(const proto_t proto, const entries_t & headers, const fmk_t * fmk, const log_t * log) noexcept;
				/**
				 * @brief Конструктор
				 *
				 * @param proto   протокол HTTP-запроса/ответа
				 * @param headers список заголовков инициализации
				 * @param fmk     объект фреймворка
				 * @param log     объект для работы с логами
				 *
				 */
				explicit Headers(const proto_t proto, const multimap_t & headers, const fmk_t * fmk, const log_t * log) noexcept;
				/**
				 * @brief Конструктор
				 *
				 * @param proto   протокол HTTP-запроса/ответа
				 * @param headers список заголовков инициализации
				 * @param fmk     объект фреймворка
				 * @param log     объект для работы с логами
				 *
				 */
				explicit Headers(const proto_t proto, initializer_list <header_t> headers, const fmk_t * fmk, const log_t * log) noexcept;
			public:
				/**
				 * @brief Конструктор
				 *
				 * @param provider объект провайдера HTTP-запроса/ответа
				 * @param headers  список заголовков инициализации
				 * @param fmk      объект фреймворка
				 * @param log      объект для работы с логами
				 *
				 */
				explicit Headers(const provider_t * provider, const fields_t & headers, const fmk_t * fmk, const log_t * log) noexcept;
				/**
				 * @brief Конструктор
				 *
				 * @param provider объект провайдера HTTP-запроса/ответа
				 * @param headers  список заголовков инициализации
				 * @param fmk      объект фреймворка
				 * @param log      объект для работы с логами
				 *
				 */
				explicit Headers(const provider_t * provider, const entries_t & headers, const fmk_t * fmk, const log_t * log) noexcept;
				/**
				 * @brief Конструктор
				 *
				 * @param provider объект провайдера HTTP-запроса/ответа
				 * @param headers  список заголовков инициализации
				 * @param fmk      объект фреймворка
				 * @param log      объект для работы с логами
				 *
				 */
				explicit Headers(const provider_t * provider, const multimap_t & headers, const fmk_t * fmk, const log_t * log) noexcept;
				/**
				 * @brief Конструктор
				 *
				 * @param provider объект провайдера HTTP-запроса/ответа
				 * @param headers  список заголовков инициализации
				 * @param fmk      объект фреймворка
				 * @param log      объект для работы с логами
				 *
				 */
				explicit Headers(const provider_t * provider, initializer_list <header_t> headers, const fmk_t * fmk, const log_t * log) noexcept;
			public:
				/**
				 * @brief Конструктор
				 *
				 * @param provider объект провайдера HTTP-запроса/ответа
				 * @param headers  список заголовков инициализации
				 * @param fmk      объект фреймворка
				 * @param log      объект для работы с логами
				 *
				 */
				explicit Headers(unique_ptr <provider_t> && provider, const fields_t & headers, const fmk_t * fmk, const log_t * log) noexcept;
				/**
				 * @brief Конструктор
				 *
				 * @param provider объект провайдера HTTP-запроса/ответа
				 * @param headers  список заголовков инициализации
				 * @param fmk      объект фреймворка
				 * @param log      объект для работы с логами
				 *
				 */
				explicit Headers(unique_ptr <provider_t> && provider, const entries_t & headers, const fmk_t * fmk, const log_t * log) noexcept;
				/**
				 * @brief Конструктор
				 *
				 * @param provider объект провайдера HTTP-запроса/ответа
				 * @param headers  список заголовков инициализации
				 * @param fmk      объект фреймворка
				 * @param log      объект для работы с логами
				 *
				 */
				explicit Headers(unique_ptr <provider_t> && provider, const multimap_t & headers, const fmk_t * fmk, const log_t * log) noexcept;
				/**
				 * @brief Конструктор
				 *
				 * @param provider объект провайдера HTTP-запроса/ответа
				 * @param headers  список заголовков инициализации
				 * @param fmk      объект фреймворка
				 * @param log      объект для работы с логами
				 *
				 */
				explicit Headers(unique_ptr <provider_t> && provider, initializer_list <header_t> headers, const fmk_t * fmk, const log_t * log) noexcept;
			public:
				/**
				 * @brief Конструктор
				 *
				 * @param proto    протокол HTTP-запроса/ответа
				 * @param provider объект провайдера HTTP-запроса/ответа
				 * @param headers  список заголовков инициализации
				 * @param fmk      объект фреймворка
				 * @param log      объект для работы с логами
				 *
				 */
				explicit Headers(const proto_t proto, const provider_t * provider, const fields_t & headers, const fmk_t * fmk, const log_t * log) noexcept;
				/**
				 * @brief Конструктор
				 *
				 * @param proto    протокол HTTP-запроса/ответа
				 * @param provider объект провайдера HTTP-запроса/ответа
				 * @param headers  список заголовков инициализации
				 * @param fmk      объект фреймворка
				 * @param log      объект для работы с логами
				 *
				 */
				explicit Headers(const proto_t proto, const provider_t * provider, const entries_t & headers, const fmk_t * fmk, const log_t * log) noexcept;
				/**
				 * @brief Конструктор
				 *
				 * @param proto    протокол HTTP-запроса/ответа
				 * @param provider объект провайдера HTTP-запроса/ответа
				 * @param headers  список заголовков инициализации
				 * @param fmk      объект фреймворка
				 * @param log      объект для работы с логами
				 *
				 */
				explicit Headers(const proto_t proto, const provider_t * provider, const multimap_t & headers, const fmk_t * fmk, const log_t * log) noexcept;
				/**
				 * @brief Конструктор
				 *
				 * @param proto    протокол HTTP-запроса/ответа
				 * @param provider объект провайдера HTTP-запроса/ответа
				 * @param headers  список заголовков инициализации
				 * @param fmk      объект фреймворка
				 * @param log      объект для работы с логами
				 *
				 */
				explicit Headers(const proto_t proto, const provider_t * provider, initializer_list <header_t> headers, const fmk_t * fmk, const log_t * log) noexcept;
			public:
				/**
				 * @brief Конструктор
				 *
				 * @param proto    протокол HTTP-запроса/ответа
				 * @param provider объект провайдера HTTP-запроса/ответа
				 * @param headers  список заголовков инициализации
				 * @param fmk      объект фреймворка
				 * @param log      объект для работы с логами
				 *
				 */
				explicit Headers(const proto_t proto, unique_ptr <provider_t> && provider, const fields_t & headers, const fmk_t * fmk, const log_t * log) noexcept;
				/**
				 * @brief Конструктор
				 *
				 * @param proto    протокол HTTP-запроса/ответа
				 * @param provider объект провайдера HTTP-запроса/ответа
				 * @param headers  список заголовков инициализации
				 * @param fmk      объект фреймворка
				 * @param log      объект для работы с логами
				 *
				 */
				explicit Headers(const proto_t proto, unique_ptr <provider_t> && provider, const entries_t & headers, const fmk_t * fmk, const log_t * log) noexcept;
				/**
				 * @brief Конструктор
				 *
				 * @param proto    протокол HTTP-запроса/ответа
				 * @param provider объект провайдера HTTP-запроса/ответа
				 * @param headers  список заголовков инициализации
				 * @param fmk      объект фреймворка
				 * @param log      объект для работы с логами
				 *
				 */
				explicit Headers(const proto_t proto, unique_ptr <provider_t> && provider, const multimap_t & headers, const fmk_t * fmk, const log_t * log) noexcept;
				/**
				 * @brief Конструктор
				 *
				 * @param proto    протокол HTTP-запроса/ответа
				 * @param provider объект провайдера HTTP-запроса/ответа
				 * @param headers  список заголовков инициализации
				 * @param fmk      объект фреймворка
				 * @param log      объект для работы с логами
				 *
				 */
				explicit Headers(const proto_t proto, unique_ptr <provider_t> && provider, initializer_list <header_t> headers, const fmk_t * fmk, const log_t * log) noexcept;
			public:
				/**
				 * @brief Деструктор
				 *
				 */
				~Headers() noexcept;
		} headers_t;
	};
	/**
	 * @brief Оператор [<<] вывода в поток буфера
	 *
	 * @param os      поток куда нужно вывести данные
	 * @param headers контейнер заголовков
	 *
	 */
	__AWH_SHARED_EXPORT__ ostream & operator << (ostream & os, const http::headers_t & headers) noexcept;
};

#endif // __AWH_HTTP_HEADERS__
