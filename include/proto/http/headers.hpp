/**
 * @file headers.hpp
 * @date 2026-07-08
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
 * \~russian
 * @brief Заголовочный файл контейнера HTTP-заголовков — класс http::Headers с регистронезависимым хранением,
 *        итераторами, специализациями хеш-функций,
 *        лимитами на количество и размер заголовков и поддержкой множественных значений одного поля
 *
 * \~english
 * @brief Header file of the container of the HTTP headers — the class http::Headers with a case-independent storing,
 *        the iterators, the specializations of the hash functions,
 *        the limits on the number and the size of the headers and a support of the multiple values of one field
 *
 * \~
 *
 * @copyright Copyright © 2026
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
 * \~russian
 * @brief основное пространство имён
 *
 *
 * \~english
 * @brief main namespace
 *
 * \~
 */
namespace awh {
	/**
	 * Подписываемся на стандартное пространство имён
	 */
	using namespace std;

	/**
	 * \~russian
	 * @brief Пространство имён HTTP-протокола
	 *
	 *
	 * \~english
	 * @brief HTTP protocol namespace
	 *
	 * \~
	 */
	namespace http {
		/**
		 * \~russian
		 * @brief Класс контейнера HTTP-заголовков
		 *
		 * \~english
		 * @brief Class of the container of the HTTP headers
		 *
		 * \~
		 */
		typedef class __AWH_SHARED_EXPORT__ Headers {
			public:
				/**
				 * \~russian
				 * @brief Режим добавления заголовка
				 *
				 * @note Режима «не установлен» здесь нет намеренно: он вёл бы себя в точности
				 *       как APPEND, обещая названием третье поведение, которого не существует.
				 *       Режим по умолчанию задаётся значением аргумента, а не членом набора
				 *
				 * \~english
				 * @brief Mode of the addition of a header
				 * @note There is no mode «not set» here deliberately: it would behave exactly
				 *       as APPEND, promising by the name a third behaviour which does not exist.
				 *       The mode by default is set by the value of an argument rather than by a member of the collection
				 *
				 * \~
				 */
				enum class mode_t : uint8_t {
					APPEND  = 0x01, // Добавить новый заголовок, сохранив существующие одноимённые
					REPLACE = 0x02  // Заменить все существующие одноимённые заголовки новым значением
				};
			public:
				/**
				 * \~russian
				 * @brief Класс HTTP-заголовка
				 *
				 * \~english
				 * @brief Class of an HTTP header
				 *
				 * \~
				 */
				typedef class __AWH_SHARED_EXPORT__ Header {
					public:
						// Название заголовка
						string name = "";
						// Значение заголовка
						string value = "";
					public:
						/**
						 * \~russian
						 * @brief Фабричный метод создания HTTP-заголовка
						 *
						 * @param name  название HTTP-заголовка
						 * @param value значение HTTP-заголовка
						 * @return      ссылка на текущий объект заголовка
						 *
						 * \~english
						 * @brief Factory method of the creation of an HTTP header
						 * @param name  name of the HTTP header
						 * @param value value of the HTTP header
						 * @return      reference to the current object of the header
						 *
						 * \~
						 */
						Header & from(string_view name, string_view value) noexcept;
					public:
						/**
						 * \~russian
						 * @brief Оператор сравнения
						 *
						 * @param other другой объект для сравнения
						 * @return      результат сравнения
						 *
						 * \~english
						 * @brief Operator of a comparison
						 * @param other another object for the comparison
						 * @return      result of the comparison
						 *
						 * \~
						 */
						bool operator == (const Header & other) const noexcept;
					public:
						/**
						 * \~russian
						 * @brief Конструктор
						 *
						 *
						 * \~english
						 * @brief Constructor
						 *
						 * \~
						 */
						explicit Header() noexcept = default;
				} header_t;
			public:
				/**
				 * \~russian
				 * @brief Специализация хеш-функции для структуры HTTP-заголовка
				 *
				 * \~english
				 * @brief Specialization of the hash function for the structure of an HTTP header
				 *
				 * \~
				 */
				typedef class __AWH_SHARED_EXPORT__ Header_Hash {
					public:
						/**
						 * \~russian
						 * @brief Оператор вычисления хеш-кода
						 *
						 * @param header объект для вычисления хеш-кода
						 * @return       хеш-код объекта
						 *
						 * \~english
						 * @brief Operator of the calculation of a hash code
						 * @param header object for the calculation of the hash code
						 * @return       hash code of the object
						 *
						 * \~
						 */
						size_t operator()(const header_t & header) const noexcept;
					public:
						/**
						 * \~russian
						 * @brief Конструктор
						 *
						 * @note Ключевого слова explicit здесь быть не должно: unordered_map
						 *       создаёт объекты хеш-функции и предиката равенства списочной
						 *       инициализацией, а та явный конструктор не берёт
						 *
						 * \~english
						 * @brief Constructor
						 * @note There is not obliged to be the keyword explicit here: unordered_map
						 *       creates the objects of the hash function and of the predicate of the equality by a list
						 *       initialization, while that one does not take an explicit constructor
						 *
						 * \~
						 */
						Header_Hash() noexcept = default;
				} header_hash_t;
			public:
				/**
				 * \~russian
				 * @brief Специализация хеш-функции для названия HTTP-заголовка
				 *
				 * \~english
				 * @brief Specialization of the hash function for the name of an HTTP header
				 *
				 * \~
				 */
				typedef class __AWH_SHARED_EXPORT__ Header_Name_Hash {
					public:
						/**
						 * \~russian
						 * @brief Оператор вычисления хеш-кода
						 *
						 * @param name название заголовка для вычисления хеш-кода
						 * @return     хеш-код названия заголовка
						 *
						 * \~english
						 * @brief Operator of the calculation of a hash code
						 * @param name name of the header for the calculation of the hash code
						 * @return     hash code of the name of the header
						 *
						 * \~
						 */
						size_t operator()(const string & name) const noexcept;
					public:
						/**
						 * \~russian
						 * @brief Конструктор
						 *
						 * @note Ключевого слова explicit здесь быть не должно: unordered_map
						 *       создаёт объекты хеш-функции и предиката равенства списочной
						 *       инициализацией, а та явный конструктор не берёт
						 *
						 * \~english
						 * @brief Constructor
						 * @note There is not obliged to be the keyword explicit here: unordered_map
						 *       creates the objects of the hash function and of the predicate of the equality by a list
						 *       initialization, while that one does not take an explicit constructor
						 *
						 * \~
						 */
						Header_Name_Hash() noexcept = default;
				} header_name_hash_t;
			public:
				/**
				 * \~russian
				 * @brief Специализация предиката равенства для названия HTTP-заголовка
				 *
				 * \~english
				 * @brief Specialization of the predicate of the equality for the name of an HTTP header
				 *
				 * \~
				 */
				typedef class __AWH_SHARED_EXPORT__ Header_Name_Equal {
					public:
						/**
						 * \~russian
						 * @brief Оператор сравнения названий заголовков
						 *
						 * @param first  первое название заголовка
						 * @param second второе название заголовка
						 * @return       результат сравнения без учёта регистра
						 *
						 * \~english
						 * @brief Operator of the comparison of the names of the headers
						 * @param first  first name of a header
						 * @param second second name of a header
						 * @return       result of the comparison without the account of the case
						 *
						 * \~
						 */
						bool operator()(const string & first, const string & second) const noexcept;
					public:
						/**
						 * \~russian
						 * @brief Конструктор
						 *
						 * @note Ключевого слова explicit здесь быть не должно: unordered_map
						 *       создаёт объекты хеш-функции и предиката равенства списочной
						 *       инициализацией, а та явный конструктор не берёт
						 *
						 * \~english
						 * @brief Constructor
						 * @note There is not obliged to be the keyword explicit here: unordered_map
						 *       creates the objects of the hash function and of the predicate of the equality by a list
						 *       initialization, while that one does not take an explicit constructor
						 *
						 * \~
						 */
						Header_Name_Equal() noexcept = default;
				} header_name_equal_t;
			public:
				/**
				 * \~russian
				 * @brief Тип списка HTTP-заголовков
				 *
				 * @details В отличие от карты, список позволяет хранить несколько одноимённых заголовков,
				 *          что необходимо для корректной работы с протоколами HTTP/1.1 и HTTP/2.
				 *          Список также содержит псевдо-заголовки, которые используются в протоколе HTTP/2 для передачи служебной информации.
				 *
				 * \~english
				 * @brief Type of the list of the HTTP headers
				 * @details Unlike a map, a list allows to store several headers of the same name,
				 *          which is necessary for a correct work with the protocols HTTP/1.1 and HTTP/2.
				 *          The list also contains the pseudo headers which are used in the HTTP/2 protocol for the transmission of the service information.
				 *
				 * \~
				 */
				using fields_t = vector <header_t>;
				/**
				 * \~russian
				 * @brief Тип набора HTTP-заголовков
				 *
				 * @details Набор позволяет хранить несколько одноимённых заголовков,
				 *          что необходимо для корректной работы с протоколами HTTP/1.1 и HTTP/2.
				 *          Набор также содержит псевдо-заголовки, которые используются в протоколе HTTP/2 для передачи служебной информации.
				 *
				 * \~english
				 * @brief Type of the collection of the HTTP headers
				 * @details A collection allows to store several headers of the same name,
				 *          which is necessary for a correct work with the protocols HTTP/1.1 and HTTP/2.
				 *          The collection also contains the pseudo headers which are used in the HTTP/2 protocol for the transmission of the service information.
				 *
				 * \~
				 */
				using entries_t = unordered_multiset <header_t, header_hash_t>;
				/**
				 * \~russian
				 * @brief Тип карты HTTP-заголовков
				 *
				 * @details Карта позволяет хранить только уникальные заголовки,
				 *          что необходимо для корректной работы с протоколами HTTP/1.1 и HTTP/2.
				 *          Карта не содержит псевдо-заголовков, так-как они предназначены только для протокола HTTP/2.
				 *
				 * \~english
				 * @brief Type of the map of the HTTP headers
				 * @details A map allows to store only the unique headers,
				 *          which is necessary for a correct work with the protocols HTTP/1.1 and HTTP/2.
				 *          The map does not contain the pseudo headers, since they are intended only for the HTTP/2 protocol.
				 *
				 * \~
				 */
				using map_t = unordered_map <string, string, header_name_hash_t, header_name_equal_t>;
				/**
				 * \~russian
				 * @brief Тип мультикарты HTTP-заголовков
				 *
				 * @details Мультикарта позволяет хранить несколько одноимённых заголовков,
				 *          что необходимо для корректной работы с протоколами HTTP/1.1 и HTTP/2.
				 *          Мультикарта не содержит псевдо-заголовков, так-как они предназначены только для протокола HTTP/2.
				 *
				 * \~english
				 * @brief Type of the multimap of the HTTP headers
				 * @details A multimap allows to store several headers of the same name,
				 *          which is necessary for a correct work with the protocols HTTP/1.1 and HTTP/2.
				 *          The multimap does not contain the pseudo headers, since they are intended only for the HTTP/2 protocol.
				 *
				 * \~
				 */
				using multimap_t = unordered_multimap <string, string, header_name_hash_t, header_name_equal_t>;
			public:
				/**
				 * \~russian
				 * @brief Предварительное объявление константного итератора
				 *
				 * \~english
				 * @brief Preliminary declaration of the constant iterator
				 *
				 * \~
				 */
				class Const_Iterator;
				/**
				 * \~russian
				 * @brief Итератор как вложенный класс
				 *
				 * \~english
				 * @brief Iterator as a nested class
				 *
				 * \~
				 */
				typedef class __AWH_SHARED_EXPORT__ Iterator {
					private:
						/**
						 * \~russian
						 * @brief Разрешаем доступ к позиции константному итератору
						 *
						 * \~english
						 * @brief We permit the access to the position to the constant iterator
						 *
						 * \~
						 */
						friend class Const_Iterator;
					public:
						/**
						 * \~russian
						 * @brief Объект указателя заголовка
						 *
						 * @note Доступ только для чтения: правка названия либо значения по месту
						 *       нарушила бы учёт потребляемой памяти контейнера и каноническую
						 *       форму регистра названий, а восстановить их извне нечем.
						 *       Изменение заголовка выполняется методами контейнера -
						 *       emplace с режимом REPLACE либо erase с последующим добавлением
						 *
						 * \~english
						 * @brief Object of the pointer of a header
						 * @note The access is only for the reading: an editing of the name or of the value in place
						 *       would violate the account of the consumed memory of the container and the canonical
						 *       form of the case of the names, while there is nothing to restore them from the outside with.
						 *       A change of a header is performed by the methods of the container -
						 *       emplace with the mode REPLACE or erase with a subsequent addition
						 *
						 * \~
						 */
						using pointer = const header_t *;
						/**
						 * \~russian
						 * @brief Объект референса заголовка
						 *
						 * \~english
						 * @brief Object of the reference of a header
						 *
						 * \~
						 */
						using reference = const header_t &;
					public:
						/**
						 * \~russian
						 * @brief Тип итератора заголовков
						 *
						 * \~english
						 * @brief Type of the iterator of the headers
						 *
						 * \~
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
						 * \~russian
						 * @brief Метод вывода сообщения об ошибке в лог
						 *
						 * @param func    название функции, в которой произошла ошибка
						 * @param message текст сообщения об ошибке
						 * @param flag    флаг важности сообщения
						 *
						 * \~english
						 * @brief Method of the output of a message about an error into the log
						 * @param func    name of the function in which the error has occurred
						 * @param message text of the message about the error
						 * @param flag    flag of the importance of the message
						 *
						 * \~
						 */
						void _error(const char * func, const char * message, const log_t::flag_t flag = log_t::flag_t::CRITICAL) const noexcept;
					public:
						/**
						 * \~russian
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
						 * \~english
						 * @brief Operator of the conversion into a raw iterator
						 * @note A raw iterator does not limit the access to a header: it exists
						 *       for the transmission of a position into the algorithms of the standard library. An editing
						 *       of a header through it will leave the account of the consumed memory of the container
						 *       and the case of the name unconcordant - this is a responsibility
						 *       of the calling side which has consciously taken a raw position
						 * @note The conversion is declared explicit exactly for this reason: an implicit one would issue
						 *       a raw position by a simple assignment of the wrapper, and an editing of a header
						 *       through it would bypass the account of the consumed memory without any
						 *       flag of a conscious choice - whereas the pointer and the reference
						 *       of the wrapper are deliberately declared accessible only for the reading
						 * @return iterator iterator for the conversion
						 *
						 * \~
						 */
						explicit operator iterator() noexcept;
					public:
						/**
						 * \~russian
						 * @brief Оператор извлечения указателя заголовка
						 *
						 * @return указатель заголовка
						 *
						 * \~english
						 * @brief Operator of the extraction of the pointer of a header
						 * @return pointer of the header
						 *
						 * \~
						 */
						pointer operator -> () noexcept;
						/**
						 * \~russian
						 * @brief Оператор разыменования заголовка
						 *
						 * @return значение заголовка
						 *
						 * \~english
						 * @brief Operator of the dereferencing of a header
						 * @return value of the header
						 *
						 * \~
						 */
						reference operator * () noexcept;
					public:
						/**
						 * \~russian
						 * @brief Оператор смещения вперед
						 *
						 * @return значение текущего итератора
						 *
						 * \~english
						 * @brief Operator of the displacement forward
						 * @return value of the current iterator
						 *
						 * \~
						 */
						Iterator & operator ++ () noexcept;
					public:
						/**
						 * \~russian
						 * @brief Оператор сравнения соответствия итератора
						 *
						 * @param other итератор для сравнения
						 * @return      результат сравнения
						 *
						 * \~english
						 * @brief Operator of the comparison of the correspondence of an iterator
						 * @param other iterator for the comparison
						 * @return      result of the comparison
						 *
						 * \~
						 */
						bool operator == (const Iterator & other) const noexcept;
						/**
						 * \~russian
						 * @brief Оператора сравнения несоответствия итератора
						 *
						 * @param other итератор для сравнения
						 * @return      результат сравнения
						 *
						 * \~english
						 * @brief Operator of the comparison of the non-correspondence of an iterator
						 * @param other iterator for the comparison
						 * @return      result of the comparison
						 *
						 * \~
						 */
						bool operator != (const Iterator & other) const noexcept;
					public:
						/**
						 * \~russian
						 * @brief Оператор сравнения соответствия итератора
						 *
						 * @param other константный итератор для сравнения
						 * @return      результат сравнения
						 *
						 * \~english
						 * @brief Operator of the comparison of the correspondence of an iterator
						 * @param other constant iterator for the comparison
						 * @return      result of the comparison
						 *
						 * \~
						 */
						bool operator == (const Const_Iterator & other) const noexcept;
						/**
						 * \~russian
						 * @brief Оператора сравнения несоответствия итератора
						 *
						 * @param other константный итератор для сравнения
						 * @return      результат сравнения
						 *
						 * \~english
						 * @brief Operator of the comparison of the non-correspondence of an iterator
						 * @param other constant iterator for the comparison
						 * @return      result of the comparison
						 *
						 * \~
						 */
						bool operator != (const Const_Iterator & other) const noexcept;
					public:
						/**
						 * \~russian
						 * @brief Конструктор
						 *
						 * @param it  итератор для установки
						 * @param log объект для работы с логами
						 *
						 * \~english
						 * @brief Constructor
						 * @param it  iterator for the setting
						 * @param log object for the work with the logs
						 *
						 * \~
						 */
						explicit Iterator(iterator it, const log_t * log) noexcept;
				} iterator_t;
				/**
				 * \~russian
				 * @brief Константный итератор как вложенный класс
				 *
				 * \~english
				 * @brief Constant iterator as a nested class
				 *
				 * \~
				 */
				typedef class __AWH_SHARED_EXPORT__ Const_Iterator {
					private:
						/**
						 * \~russian
						 * @brief Разрешаем доступ к позиции обычному итератору
						 *
						 * \~english
						 * @brief We permit the access to the position to the ordinary iterator
						 *
						 * \~
						 */
						friend class Iterator;
					public:
						/**
						 * \~russian
						 * @brief Объект указателя заголовка
						 *
						 * \~english
						 * @brief Object of the pointer of a header
						 *
						 * \~
						 */
						using pointer = const header_t *;
						/**
						 * \~russian
						 * @brief Объект референса заголовка
						 *
						 * \~english
						 * @brief Object of the reference of a header
						 *
						 * \~
						 */
						using reference = const header_t &;
					public:
						/**
						 * \~russian
						 * @brief Тип константного итератора заголовков
						 *
						 * \~english
						 * @brief Type of the constant iterator of the headers
						 *
						 * \~
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
						 * \~russian
						 * @brief Метод вывода сообщения об ошибке в лог
						 *
						 * @param func    название функции, в которой произошла ошибка
						 * @param message текст сообщения об ошибке
						 * @param flag    флаг важности сообщения
						 *
						 * \~english
						 * @brief Method of the output of a message about an error into the log
						 * @param func    name of the function in which the error has occurred
						 * @param message text of the message about the error
						 * @param flag    flag of the importance of the message
						 *
						 * \~
						 */
						void _error(const char * func, const char * message, const log_t::flag_t flag = log_t::flag_t::CRITICAL) const noexcept;
					public:
						/**
						 * \~russian
						 * @brief Оператор преобразования в сырой константный итератор
						 *
						 * @return const_iterator итератор для преобразования
						 *
						 * \~english
						 * @brief Operator of the conversion into a raw constant iterator
						 * @return const_iterator iterator for the conversion
						 *
						 * \~
						 */
						operator const_iterator() const noexcept;
					public:
						/**
						 * \~russian
						 * @brief Оператор извлечения указателя заголовка
						 *
						 * @return указатель заголовка
						 *
						 * \~english
						 * @brief Operator of the extraction of the pointer of a header
						 * @return pointer of the header
						 *
						 * \~
						 */
						pointer operator -> () const noexcept;
						/**
						 * \~russian
						 * @brief Оператор разыменования заголовка
						 *
						 * @return значение заголовка
						 *
						 * \~english
						 * @brief Operator of the dereferencing of a header
						 * @return value of the header
						 *
						 * \~
						 */
						reference operator * () const noexcept;
					public:
						/**
						 * \~russian
						 * @brief Оператор смещения вперед
						 *
						 * @return значение текущего итератора
						 *
						 * \~english
						 * @brief Operator of the displacement forward
						 * @return value of the current iterator
						 *
						 * \~
						 */
						Const_Iterator & operator ++ () noexcept;
					public:
						/**
						 * \~russian
						 * @brief Оператор сравнения соответствия итератора
						 *
						 * @param other итератор для сравнения
						 * @return      результат сравнения
						 *
						 * \~english
						 * @brief Operator of the comparison of the correspondence of an iterator
						 * @param other iterator for the comparison
						 * @return      result of the comparison
						 *
						 * \~
						 */
						bool operator == (const Iterator & other) const noexcept;
						/**
						 * \~russian
						 * @brief Оператора сравнения несоответствия итератора
						 *
						 * @param other итератор для сравнения
						 * @return      результат сравнения
						 *
						 * \~english
						 * @brief Operator of the comparison of the non-correspondence of an iterator
						 * @param other iterator for the comparison
						 * @return      result of the comparison
						 *
						 * \~
						 */
						bool operator != (const Iterator & other) const noexcept;
					public:
						/**
						 * \~russian
						 * @brief Оператор сравнения соответствия итератора
						 *
						 * @param other итератор для сравнения
						 * @return      результат сравнения
						 *
						 * \~english
						 * @brief Operator of the comparison of the correspondence of an iterator
						 * @param other iterator for the comparison
						 * @return      result of the comparison
						 *
						 * \~
						 */
						bool operator == (const Const_Iterator & other) const noexcept;
						/**
						 * \~russian
						 * @brief Оператора сравнения несоответствия итератора
						 *
						 * @param other итератор для сравнения
						 * @return      результат сравнения
						 *
						 * \~english
						 * @brief Operator of the comparison of the non-correspondence of an iterator
						 * @param other iterator for the comparison
						 * @return      result of the comparison
						 *
						 * \~
						 */
						bool operator != (const Const_Iterator & other) const noexcept;
					public:
						/**
						 * \~russian
						 * @brief Конструктор
						 *
						 * @param it  итератор для установки
						 * @param log объект для работы с логами
						 *
						 * \~english
						 * @brief Constructor
						 * @param it  iterator for the setting
						 * @param log object for the work with the logs
						 *
						 * \~
						 */
						explicit Const_Iterator(const_iterator it, const log_t * log) noexcept;
				} const_iterator_t;
			private:
				/**
				 * \~russian
				 * @brief Структура идентификации сервиса
				 *
				 * \~english
				 * @brief Structure of the identification of the service
				 *
				 * \~
				 */
				typedef struct __AWH_SHARED_EXPORT__ Ident {
					// Идентификатор сервиса
					string id;
					// Название сервиса
					string name;
					// Версия модуля приложения
					string version;
					/**
					 * \~russian
					 * @brief Конструктор
					 *
					 *
					 * \~english
					 * @brief Constructor
					 *
					 * \~
					 */
					explicit Ident() noexcept;
				} ident_t;
				/**
				 * \~russian
				 * @brief Структура параметров максимальных значений
				 *
				 * \~english
				 * @brief Structure of the parameters of the largest values
				 *
				 * \~
				 */
				typedef struct __AWH_SHARED_EXPORT__ Max {
					// Максимальный размер выделения памяти
					size_t memory;
					// Максимальное количество добавляемых записей
					size_t records;
					/**
					 * \~russian
					 * @brief Конструктор
					 *
					 *
					 * \~english
					 * @brief Constructor
					 *
					 * \~
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
				 * \~russian
				 * @brief Метод приведения названий всех заголовков к канонической форме текущего протокола
				 *
				 * @details Для протоколов семейства HTTP/2 названия приводятся к нижнему регистру, для остальных -
				 *          к «умному» регистру. Вызывается при изменении протокола, чтобы единая семантика
				 *          регистра соблюдалась при любом способе доступа к заголовкам (не только при печати).
				 *
				 * \~english
				 * @brief Method of the bringing of the names of all the headers to the canonical form of the current protocol
				 * @details For the protocols of the family HTTP/2 the names are brought to the lower case, for the rest -
				 *          to a «smart» case. It is called at a change of the protocol, so that a single semantics
				 *          of the case would be observed at any way of the access to the headers (not only at the printing).
				 *
				 * \~
				 */
				void _recase() noexcept;
				/**
				 * \~russian
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
				 * \~english
				 * @brief Method of checking whether a header will fit at a replacement of those of the same name
				 * @details A replacement consists of a removal of the previous occurrences and of an addition of a new value.
				 *          If the new value does not fit into the limitations, the removal has already happened,
				 *          and the header disappears from the collection entirely. The checking is performed before the removal
				 *          by the state as it will be after it, and in such a case the replacement does not begin.
				 * @param name    name of the header being replaced
				 * @param payload volume of the payload of the new value (the name and the value)
				 * @return        result of the checking
				 *
				 * \~
				 */
				bool _fits(string_view name, const size_t payload) const noexcept;
				/**
				 * \~russian
				 * @brief Метод приведения набора заголовков в соответствие ограничениям
				 *
				 * @details Отбрасывает заголовки с конца набора, пока он не уложится
				 *          в ограничения по количеству записей и объёму памяти. Вызывается
				 *          при понижении ограничений: без этого набор, собранный до их
				 *          установки, остался бы сверх предела, а сам предел - невыполненным
				 *
				 * \~english
				 * @brief Method of the bringing of the collection of the headers into a correspondence with the limitations
				 * @details It discards the headers from the end of the collection until it fits
				 *          into the limitations by the number of the records and by the volume of the memory. It is called
				 *          at a lowering of the limitations: without this a collection assembled before their
				 *          setting would remain above the limit, while the limit itself - unfulfilled
				 *
				 * \~
				 */
				void _trim() noexcept;
				/**
				 * \~russian
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
				 * \~english
				 * @brief Method of the assembly of the collection of the headers in the form suitable for the indicated protocol
				 * @details For the protocols of the family HTTP/2 the pseudo headers are added to the collection,
				 *          the header of the addressee is carried over into [:authority], while the fields which
				 *          cannot be in a message are sifted out: those controlling the connection
				 *          (RFC 9113 §8.2.2, RFC 9114 §4.2), those listed in the value of the header
				 *          Connection, a [TE] with an inadmissible value and the pseudo headers already
				 *          assembled from the provider. For the rest of the protocols the collection is issued as it is.
				 * @note The method is single for the printing and for the operators of the conversion: a divergence between
				 *       them would mean that the form of the collection depends on the way of its getting,
				 *       and that assembled from one would be rejected by the check of the other
				 * @param proto protocol for which the collection is assembled
				 * @return      assembled collection of the headers
				 *
				 * \~
				 */
				fields_t _compose(const http::proto_t proto) const noexcept;
			private:
				/**
				 * \~russian
				 * @brief Шаблон добавления нового заголовка
				 *
				 * @tparam Name    тип названия добавляемого заголовка
				 * @tparam Content тип содержимого добавляемого заголовка
				 *
				 * \~english
				 * @brief Template of the addition of a new header
				 * @tparam Name    type of the name of the header being added
				 * @tparam Content type of the content of the header being added
				 *
				 * \~
				 */
				template <typename Name, typename Content>
				/**
				 * \~russian
				 * @brief Метод добавления нового заголовка
				 *
				 * @param name    название заголовка
				 * @param content содержимое заголовка
				 * @return        общее количество заголовков
				 *
				 * \~english
				 * @brief Method of the addition of a new header
				 * @param name    name of the header
				 * @param content content of the header
				 * @return        total number of the headers
				 *
				 * \~
				 */
				size_t _emplace(Name && name, Content && content) noexcept;
			private:
				/**
				 * \~russian
				 * @brief Метод вывода сообщения об ошибке в лог
				 *
				 * @param func    название функции, в которой произошла ошибка
				 * @param message текст сообщения об ошибке
				 * @param flag    флаг важности сообщения
				 *
				 * \~english
				 * @brief Method of the output of a message about an error into the log
				 * @param func    name of the function in which the error has occurred
				 * @param message text of the message about the error
				 * @param flag    flag of the importance of the message
				 *
				 * \~
				 */
				void _error(const char * func, const char * message, const log_t::flag_t flag = log_t::flag_t::CRITICAL) const noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод очистки всех данных заголовков
				 *
				 * \~english
				 * @brief Method of the clearing of all the data of the headers
				 *
				 * \~
				 */
				void clear() noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод полной очистки памяти
				 *
				 * \~english
				 * @brief Method of a full clearing of the memory
				 *
				 * \~
				 */
				void reset() noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод проверки на заполненность заголовков
				 *
				 * @note Отвечает о наборе полей и только о нём: контейнер с установленным
				 *       провайдером, но без полей считается пустым. Провайдер описывает
				 *       стартовую строку либо псевдозаголовки, а не поля, и наличие
				 *       сообщения проверяется методом provider()
				 *
				 * @return результат проверки
				 *
				 * \~english
				 * @brief Method of checking the filledness of the headers
				 * @note It answers about the collection of the fields and only about it: a container with a set
				 *       provider but without the fields is considered empty. The provider describes
				 *       the starting line or the pseudo headers rather than the fields, and the presence
				 *       of a message is checked by the method provider()
				 * @return result of the checking
				 *
				 * \~
				 */
				bool empty() const noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод добавления стандартных заголовков по умолчанию
				 *
				 * @details Добавляет недостающие заголовки: User-Agent для запроса клиента,
				 *          Server, X-Powered-By и Date для ответа сервера. Уже установленные заголовки не изменяются.
				 *
				 * @return результат выполнения операции
				 *
				 * \~english
				 * @brief Method of the addition of the standard headers by default
				 * @details It adds the lacking headers: a User-Agent for a request of a client,
				 *          a Server, an X-Powered-By and a Date for an answer of a server. The already set headers are not changed.
				 * @return result of the performance of the operation
				 *
				 * \~
				 */
				bool addDefaultHeaders() noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод получения текущей даты для HTTP-запроса
				 *
				 * @note Unix Timestamp - количество секунд с 1 января 1970 года
				 *
				 * @param date дата в формате Unix Timestamp
				 * @return     штамп времени в текстовом виде
				 *
				 * \~english
				 * @brief Method of getting the current date for an HTTP request
				 * @note A Unix Timestamp is the number of the seconds since the 1st of January 1970
				 * @param date date in the format Unix Timestamp
				 * @return     time stamp in a text form
				 *
				 * \~
				 */
				string date(const uint64_t date = 0) const noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод получения протокола HTTP-запроса/ответа
				 *
				 * @return протокол HTTP-запроса/ответа
				 *
				 * \~english
				 * @brief Method of getting the protocol of the HTTP request/answer
				 * @return protocol of the HTTP request/answer
				 *
				 * \~
				 */
				proto_t proto() const noexcept;
				/**
				 * \~russian
				 * @brief Метод установки протокола HTTP-запроса/ответа
				 *
				 * @param proto протокол HTTP-запроса/ответа
				 *
				 * \~english
				 * @brief Method of setting the protocol of the HTTP request/answer
				 * @param proto protocol of the HTTP request/answer
				 *
				 * \~
				 */
				void proto(const proto_t proto) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод получения идентификации сервиса
				 *
				 * @return сформированный агент
				 *
				 * \~english
				 * @brief Method of getting the identification of the service
				 * @return formed agent
				 *
				 * \~
				 */
				string ident() const noexcept;
				/**
				 * \~russian
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
				 * \~english
				 * @brief Method of setting the identification of the service
				 * @details The constituents get into the value of the field entirely: at a request - into the
				 *          User-Agent, at an answer - into the X-Powered-By. A constituent containing
				 *          the octets inadmissible in the value of a field (the control characters and a DEL),
				 *          is not applied at all and is written into the log: the application takes these
				 *          strings from the outside, while a carriage return or a line feed inside them
				 *          would split the message into two. An empty constituent leaves
				 *          the previous value, while one unsuitable does not cancel the rest.
				 * @param id      identifier of the service
				 * @param name    name of the service
				 * @param version version of the service
				 *
				 * \~
				 */
				void ident(string_view id, string_view name, string_view version) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод получения объекта провайдера HTTP-запроса/ответа
				 *
				 * @return объект провайдера HTTP-запроса/ответа
				 *
				 * \~english
				 * @brief Method of getting the object of the provider of the HTTP request/answer
				 * @return object of the provider of the HTTP request/answer
				 *
				 * \~
				 */
				const provider_t * provider() const noexcept;
				/**
				 * \~russian
				 * @brief Метод получения объекта провайдера HTTP-запроса/ответа
				 *
				 * @param provider объект провайдера HTTP-запроса/ответа
				 * @return         результат выполнения операции
				 *
				 * \~english
				 * @brief Method of getting the object of the provider of the HTTP request/answer
				 * @param provider object of the provider of the HTTP request/answer
				 * @return         result of the performance of the operation
				 *
				 * \~
				 */
				bool provider(unique_ptr <provider_t> & provider) const noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод установки объекта провайдера HTTP-запроса/ответа
				 *
				 * @param provider объект провайдера HTTP-запроса/ответа
				 *
				 * \~english
				 * @brief Method of setting the object of the provider of the HTTP request/answer
				 * @param provider object of the provider of the HTTP request/answer
				 *
				 * \~
				 */
				void provider(const provider_t * provider) noexcept;
				/**
				 * \~russian
				 * @brief Метод установки объекта провайдера HTTP-запроса/ответа
				 *
				 * @param provider объект провайдера HTTP-запроса/ответа
				 *
				 * \~english
				 * @brief Method of setting the object of the provider of the HTTP request/answer
				 * @param provider object of the provider of the HTTP request/answer
				 *
				 * \~
				 */
				void provider(unique_ptr <provider_t> && provider) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод получения стартовой строки HTTP-запроса/ответа
				 *
				 * @return стартовая строка HTTP-запроса/ответа
				 *
				 * \~english
				 * @brief Method of getting the starting line of the HTTP request/answer
				 * @return starting line of the HTTP request/answer
				 *
				 * \~
				 */
				string startline() const noexcept;
				/**
				 * \~russian
				 * @brief Метод установки стартовой строки HTTP-запроса/ответа
				 *
				 * @param startline стартовая строка HTTP-запроса/ответа
				 *
				 * \~english
				 * @brief Method of setting the starting line of the HTTP request/answer
				 * @param startline starting line of the HTTP request/answer
				 *
				 * \~
				 */
				void startline(const string_view startline) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод удаления заголовка
				 *
				 * @param name название удаляемого заголовка
				 *
				 * \~english
				 * @brief Method of the removal of a header
				 * @param name name of the header being removed
				 *
				 * \~
				 */
				void erase(string_view name) noexcept;
				/**
				 * \~russian
				 * @brief erase Метод удаления заголовка
				 *
				 * @param it идетартор заголовка для удаления
				 * @return   следующий итератор
				 *
				 * \~english
				 * @brief erase Method of the removal of a header
				 * @param it iterator of the header for the removal
				 * @return   next iterator
				 *
				 * \~
				 */
				iterator_t erase(const iterator_t & it) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод проверки существования заголовка
				 *
				 * @param name название заголовка для проверки
				 * @return     результат выполнения проверки
				 *
				 * \~english
				 * @brief Method of checking the existence of a header
				 * @param name name of the header for the checking
				 * @return     result of the performance of the checking
				 *
				 * \~
				 */
				bool has(string_view name) const noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод получения общего количества заголовков
				 *
				 * @return общее количество заголовков
				 *
				 * \~english
				 * @brief Method of getting the total number of the headers
				 * @return total number of the headers
				 *
				 * \~
				 */
				size_t size() const noexcept;
			public:
				/**
				 * \~russian
				 * @brief Количество добавленных заголовков
				 *
				 * @param name название заголовка количество которых нужно определить
				 * @return     количество добавленных заголовков
				 *
				 * \~english
				 * @brief Number of the added headers
				 * @param name name of the header the number of which is needed to be determined
				 * @return     number of the added headers
				 *
				 * \~
				 */
				size_t count(string_view name = "") const noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод извлечения содержимого заголовка
				 *
				 * @param name название заголовка
				 * @return     содержимое заголовка
				 *
				 * \~english
				 * @brief Method of the extraction of the content of a header
				 * @param name name of the header
				 * @return     content of the header
				 *
				 * \~
				 */
				const string & at(string_view name) const noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод извлечения названий заголовков
				 *
				 * @return список названий заголовков
				 *
				 * \~english
				 * @brief Method of the extraction of the names of the headers
				 * @return list of the names of the headers
				 *
				 * \~
				 */
				vector <string> names() const noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод вывода списка значений одинаковых заголовков
				 *
				 * @param name название заголовка
				 * @return     список значений одинаковых заголовков
				 *
				 * \~english
				 * @brief Method of the output of the list of the values of the identical headers
				 * @param name name of the header
				 * @return     list of the values of the identical headers
				 *
				 * \~
				 */
				vector <string> range(string_view name) const noexcept;
			public:
				/**
				 * \~russian
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
				 * \~english
				 * @brief Method of the addition or of the replacement of a header
				 * @note In the mode REPLACE (by default) all the previous occurrences of the header with the indicated
				 *       name are replaced by the new value. In the mode APPEND a new header is added,
				 *       preserving the existing headers of the same name.
				 * @param name    name of the header
				 * @param content content of the header
				 * @param mode    mode of the addition of the header (to add/to replace)
				 * @return        total number of the headers
				 *
				 * \~
				 */
				size_t emplace(string && name, string && content, const mode_t mode = mode_t::REPLACE) noexcept;
				/**
				 * \~russian
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
				 * \~english
				 * @brief Method of the addition or of the replacement of a header
				 * @note In the mode REPLACE (by default) all the previous occurrences of the header with the indicated
				 *       name are replaced by the new value. In the mode APPEND a new header is added,
				 *       preserving the existing headers of the same name.
				 * @param name    name of the header (a C string)
				 * @param content content of the header (is moved)
				 * @param mode    mode of the addition of the header (to add/to replace)
				 * @return        total number of the headers
				 *
				 * \~
				 */
				size_t emplace(const char * name, string && content, const mode_t mode = mode_t::REPLACE) noexcept;
				/**
				 * \~russian
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
				 * \~english
				 * @brief Method of the addition or of the replacement of a header
				 * @note In the mode REPLACE (by default) all the previous occurrences of the header with the indicated
				 *       name are replaced by the new value. In the mode APPEND a new header is added,
				 *       preserving the existing headers of the same name.
				 * @param name    name of the header (is moved)
				 * @param content content of the header (a C string)
				 * @param mode    mode of the addition of the header (to add/to replace)
				 * @return        total number of the headers
				 *
				 * \~
				 */
				size_t emplace(string && name, const char * content, const mode_t mode = mode_t::REPLACE) noexcept;
				/**
				 * \~russian
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
				 * \~english
				 * @brief Method of the addition or of the replacement of a header
				 * @note In the mode REPLACE (by default) all the previous occurrences of the header with the indicated
				 *       name are replaced by the new value. In the mode APPEND a new header is added,
				 *       preserving the existing headers of the same name.
				 * @param name    name of the header
				 * @param content content of the header
				 * @param mode    mode of the addition of the header (to add/to replace)
				 * @return        total number of the headers
				 *
				 * \~
				 */
				size_t emplace(string_view name, string_view content, const mode_t mode = mode_t::REPLACE) noexcept;
				/**
				 * \~russian
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
				 * \~english
				 * @brief Method of the addition or of the replacement of a header
				 * @note In the mode REPLACE (by default) all the previous occurrences of the header with the indicated
				 *       name are replaced by the new value. In the mode APPEND a new header is added,
				 *       preserving the existing headers of the same name.
				 * @param name    name of the header (is moved)
				 * @param content content of the header (is copied)
				 * @param mode    mode of the addition of the header (to add/to replace)
				 * @return        total number of the headers
				 *
				 * \~
				 */
				size_t emplace(string && name, const string & content, const mode_t mode = mode_t::REPLACE) noexcept;
				/**
				 * \~russian
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
				 * \~english
				 * @brief Method of the addition or of the replacement of a header
				 * @note In the mode REPLACE (by default) all the previous occurrences of the header with the indicated
				 *       name are replaced by the new value. In the mode APPEND a new header is added,
				 *       preserving the existing headers of the same name.
				 * @param name    name of the header (is copied)
				 * @param content content of the header (is moved)
				 * @param mode    mode of the addition of the header (to add/to replace)
				 * @return        total number of the headers
				 *
				 * \~
				 */
				size_t emplace(const string & name, string && content, const mode_t mode = mode_t::REPLACE) noexcept;
				/**
				 * \~russian
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
				 * \~english
				 * @brief Method of the addition or of the replacement of a header
				 * @note In the mode REPLACE (by default) all the previous occurrences of the header with the indicated
				 *       name are replaced by the new value. In the mode APPEND a new header is added,
				 *       preserving the existing headers of the same name.
				 * @param name    name of the header (a C string)
				 * @param content content of the header (a C string)
				 * @param mode    mode of the addition of the header (to add/to replace)
				 * @return        total number of the headers
				 *
				 * \~
				 */
				size_t emplace(const char * name, const char * content, const mode_t mode = mode_t::REPLACE) noexcept;
				/**
				 * \~russian
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
				 * \~english
				 * @brief Method of the addition or of the replacement of a header
				 * @note In the mode REPLACE (by default) all the previous occurrences of the header with the indicated
				 *       name are replaced by the new value. In the mode APPEND a new header is added,
				 *       preserving the existing headers of the same name.
				 * @param name    name of the header (a C string)
				 * @param content content of the header (is copied)
				 * @param mode    mode of the addition of the header (to add/to replace)
				 * @return        total number of the headers
				 *
				 * \~
				 */
				size_t emplace(const char * name, const string & content, const mode_t mode = mode_t::REPLACE) noexcept;
				/**
				 * \~russian
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
				 * \~english
				 * @brief Method of the addition or of the replacement of a header
				 * @note In the mode REPLACE (by default) all the previous occurrences of the header with the indicated
				 *       name are replaced by the new value. In the mode APPEND a new header is added,
				 *       preserving the existing headers of the same name.
				 * @param name    name of the header (is copied)
				 * @param content content of the header (a C string)
				 * @param mode    mode of the addition of the header (to add/to replace)
				 * @return        total number of the headers
				 *
				 * \~
				 */
				size_t emplace(const string & name, const char * content, const mode_t mode = mode_t::REPLACE) noexcept;
				/**
				 * \~russian
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
				 * \~english
				 * @brief Method of the addition or of the replacement of a header
				 * @note In the mode REPLACE (by default) all the previous occurrences of the header with the indicated
				 *       name are replaced by the new value. In the mode APPEND a new header is added,
				 *       preserving the existing headers of the same name.
				 * @param name    name of the header (is copied)
				 * @param content content of the header (is copied)
				 * @param mode    mode of the addition of the header (to add/to replace)
				 * @return        total number of the headers
				 *
				 * \~
				 */
				size_t emplace(const string & name, const string & content, const mode_t mode = mode_t::REPLACE) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод печати содержимого заголовков в формате HTTP
				 *
				 * @return заголовки в формате HTTP
				 *
				 * \~english
				 * @brief Method of the printing of the content of the headers in the format of HTTP
				 * @return headers in the format of HTTP
				 *
				 * \~
				 */
				string print(const http::proto_t proto) const noexcept;
				/**
				 * \~russian
				 * @brief Метод печати содержимого заголовков в формате протокола контейнера
				 *
				 * @note Протокол берётся у самого контейнера, а не подставляется значением
				 *       по умолчанию: умолчание HTTP/1 давало бы для контейнера HTTP/2
				 *       вид другого протокола, расходясь с оператором преобразования в строку
				 *
				 * @return заголовки в формате HTTP
				 *
				 * \~english
				 * @brief Method of the printing of the content of the headers in the format of the protocol of the container
				 * @note The protocol is taken at the container itself rather than substituted by a value
				 *       by default: a default of HTTP/1 would give for a container of HTTP/2
				 *       the form of another protocol, diverging with the operator of the conversion into a string
				 * @return headers in the format of HTTP
				 *
				 * \~
				 */
				string print() const noexcept;
				/**
				 * \~russian
				 * @brief Метод печати содержимого заголовка
				 *
				 * @param name  печать заголовка в формате HTTP
				 * @param proto версия протокола
				 * @return      распечатанный заголовок
				 *
				 * \~english
				 * @brief Method of the printing of the content of a header
				 * @param name  printing of the header in the format of HTTP
				 * @param proto version of the protocol
				 * @return      printed header
				 *
				 * \~
				 */
				string print(string_view name, const http::proto_t proto) const noexcept;
				/**
				 * \~russian
				 * @brief Метод печати содержимого заголовка в формате протокола контейнера
				 *
				 * @param name название печатаемого заголовка
				 * @return     распечатанный заголовок
				 *
				 * \~english
				 * @brief Method of the printing of the content of a header in the format of the protocol of the container
				 * @param name name of the header being printed
				 * @return     printed header
				 *
				 * \~
				 */
				string print(string_view name) const noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод получения текущего размера потребляемой памяти
				 *
				 * @return текущий размер потребляемой памяти
				 *
				 * \~english
				 * @brief Method of getting the current size of the consumed memory
				 * @return current size of the consumed memory
				 *
				 * \~
				 */
				size_t memory() const noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод получения максимального размера потребления памяти
				 *
				 * @return максимальный размер потребления памяти
				 *
				 * \~english
				 * @brief Method of getting the largest size of the consumption of the memory
				 * @return largest size of the consumption of the memory
				 *
				 * \~
				 */
				size_t maxMemory() const noexcept;
				/**
				 * \~russian
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
				 * \~english
				 * @brief Method of setting the largest size of the consumption of the memory
				 * @details If the new limitation is smaller than the already occupied volume, the superfluous headers
				 *          are discarded from the end of the collection, while into the log a warning is written
				 *          with their number. Otherwise the limitation would not be fulfilled on a collection
				 *          assembled before its setting, and the container would remain above the limit
				 *          up to a full clearing
				 * @param size largest size of the consumption of the memory
				 *
				 * \~
				 */
				void maxMemory(const size_t size) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод получения максимального количества заголовков
				 *
				 * @return максимальное количество заголовков
				 *
				 * \~english
				 * @brief Method of getting the largest number of the headers
				 * @return largest number of the headers
				 *
				 * \~
				 */
				size_t maxRecords() const noexcept;
				/**
				 * \~russian
				 * @brief Метод установки максимального количества заголовков
				 *
				 * @details Если новое ограничение меньше числа уже добавленных заголовков,
				 *          лишние отбрасываются с конца набора, а в лог записывается
				 *          предупреждение с их количеством
				 *
				 * @param count максимальное количество заголовков
				 *
				 * \~english
				 * @brief Method of setting the largest number of the headers
				 * @details If the new limitation is smaller than the number of the already added headers,
				 *          the superfluous ones are discarded from the end of the collection, while into the log
				 *          a warning is written with their number
				 * @param count largest number of the headers
				 *
				 * \~
				 */
				void maxRecords(const size_t count) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод обмена заголовками
				 *
				 * @param headers заголовки для обмена
				 *
				 * \~english
				 * @brief Method of the exchange of the headers
				 * @param headers headers for the exchange
				 *
				 * \~
				 */
				void swap(Headers & headers) noexcept;
				/**
				 * \~russian
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
				 * \~english
				 * @brief Method of the merging of the headers
				 * @details In the mode APPEND the headers of the transmitted container are appended
				 *          to the current collection with the preservation of those of the same name, in the mode REPLACE
				 *          those of the same name are replaced by the transmitted ones. The mode by default is APPEND:
				 *          at the fields lawfully met several times a replacement would lose
				 *          the previous values, while a merging by its sense preserves them
				 * @note To a field met in a message once - a Content-Length, a Host, -
				 *       the default gives a duplicate: the merging does not know which fields are single.
				 *       For such fields the mode REPLACE is transmitted
				 * @param headers headers for the merging
				 * @param mode    mode of the merging of the headers (to add/to replace)
				 *
				 * \~
				 */
				void merge(const Headers & headers, const mode_t mode = mode_t::APPEND) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод получения конечного итератора
				 *
				 * @return конечный итератор
				 *
				 * \~english
				 * @brief Method of getting the final iterator
				 * @return final iterator
				 *
				 * \~
				 */
				iterator_t end() noexcept;
				/**
				 * \~russian
				 * @brief Метод получения конечного константного итератора
				 *
				 * @return конечный константный итератор
				 *
				 * \~english
				 * @brief Method of getting the final constant iterator
				 * @return final constant iterator
				 *
				 * \~
				 */
				const_iterator_t end() const noexcept;
				/**
				 * \~russian
				 * @brief Метод получения конечного константного итератора
				 *
				 * @return конечный константный итератор
				 *
				 * \~english
				 * @brief Method of getting the final constant iterator
				 * @return final constant iterator
				 *
				 * \~
				 */
				const_iterator_t cend() const noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод получение начального итератора
				 *
				 * @return начальный итератор
				 *
				 * \~english
				 * @brief Method of getting the initial iterator
				 * @return initial iterator
				 *
				 * \~
				 */
				iterator_t begin() noexcept;
				/**
				 * \~russian
				 * @brief Метод получения начального константного итератора
				 *
				 * @return начальный константный итератор
				 *
				 * \~english
				 * @brief Method of getting the initial constant iterator
				 * @return initial constant iterator
				 *
				 * \~
				 */
				const_iterator_t begin() const noexcept;
				/**
				 * \~russian
				 * @brief Метод получения начального константного итератора
				 *
				 * @return начальный константный итератор
				 *
				 * \~english
				 * @brief Method of getting the initial constant iterator
				 * @return initial constant iterator
				 *
				 * \~
				 */
				const_iterator_t cbegin() const noexcept;
			public:
				/**
				 * \~russian
				 * @brief Метод поиска указанного заголовка
				 *
				 * @param name название заголовка для поиска
				 * @return     итератор указанного заголовка
				 *
				 * \~english
				 * @brief Method of the search of the indicated header
				 * @param name name of the header for the search
				 * @return     iterator of the indicated header
				 *
				 * \~
				 */
				iterator_t find(string_view name) noexcept;
				/**
				 * \~russian
				 * @brief Метод поиска указанного заголовка
				 *
				 * @param name название заголовка для поиска
				 * @return     константный итератор указанного заголовка
				 *
				 * \~english
				 * @brief Method of the search of the indicated header
				 * @param name name of the header for the search
				 * @return     constant iterator of the indicated header
				 *
				 * \~
				 */
				const_iterator_t find(string_view name) const noexcept;
			public:
				/**
				 * \~russian
				 * @brief Оператор получения количество заголовков
				 *
				 * @return количество заголовков
				 *
				 * \~english
				 * @brief Operator of getting the number of the headers
				 * @return number of the headers
				 *
				 * \~
				 */
				explicit operator size_t() const noexcept;
				/**
				 * \~russian
				 * @brief Оператор печати содержимого заголовков в формате HTTP
				 *
				 * @return заголовки в формате HTTP
				 *
				 * \~english
				 * @brief Operator of the printing of the content of the headers in the format of HTTP
				 * @return headers in the format of HTTP
				 *
				 * \~
				 */
				explicit operator string() const noexcept;
			public:
				/**
				 * \~russian
				 * @brief Оператор получения протокола HTTP-запроса/ответа
				 *
				 * @return протокол HTTP-запроса/ответа
				 *
				 * \~english
				 * @brief Operator of getting the protocol of the HTTP request/answer
				 * @return protocol of the HTTP request/answer
				 *
				 * \~
				 */
				explicit operator proto_t() const noexcept;
			public:
				/**
				 * \~russian
				 * @brief Оператор получения объекта провайдера HTTP-запроса/ответа
				 *
				 * @return объект провайдера HTTP-запроса/ответа
				 *
				 * \~english
				 * @brief Operator of getting the object of the provider of the HTTP request/answer
				 * @return object of the provider of the HTTP request/answer
				 *
				 * \~
				 */
				explicit operator const provider_t * () const noexcept;
				/**
				 * \~russian
				 * @brief Оператор получения объекта провайдера HTTP-запроса/ответа
				 *
				 * @return объект провайдера HTTP-запроса/ответа
				 *
				 * \~english
				 * @brief Operator of getting the object of the provider of the HTTP request/answer
				 * @return object of the provider of the HTTP request/answer
				 *
				 * \~
				 */
				explicit operator unique_ptr <provider_t> () const noexcept;
			public:
				/**
				 * \~russian
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
				 * \~english
				 * @brief Operator of getting the list of the headers in the form of a message
				 * @details It issues the collection as it will go away onto the wire: for the protocols
				 *          of the family HTTP/2 the pseudo headers are added, the header of the addressee
				 *          is carried over into [:authority], the fields which cannot be in a message
				 *          are sifted out. The same form is given by the printing
				 * @note The difference from map_t and multimap_t is deliberate: those issue the form of the storage,
				 *       the content as it is. The details and the reason are in the README of the module,
				 *       the section «Two forms of one container»
				 * @return list of the headers in the form of a message
				 *
				 * \~
				 */
				explicit operator fields_t() const noexcept;
				/**
				 * \~russian
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
				 * \~english
				 * @brief Operator of getting the collection of the headers in the form of a message
				 * @details The composition is assembled by the same way as the list of the headers:
				 *          with the pseudo headers and with a sifting out of the fields which cannot
				 *          be in a message
				 * @note The order is thereby lost: the collection is an unordered one. For the fields
				 *       of one name the order carries a sense (RFC 9110 §5.3), therefore
				 *       to restore a message by it is impossible - it answers the question
				 *       about the composition and the multiplicity rather than about the sequence. The message
				 *       as a whole is issued by the list of the fields or by the printing
				 * @return collection of the headers in the form of a message
				 *
				 * \~
				 */
				explicit operator entries_t() const noexcept;
			public:
				/**
				 * \~russian
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
				 * \~english
				 * @brief Operator of getting the map of the headers in the form of a storage
				 * @details It issues the content as it is, without the pseudo headers and without a sifting out:
				 *          by it the calling side finds that which it has itself put in.
				 *          The map stores a single value per name
				 * @note The divergence with fields_t and entries_t is deliberate - see the README of the module,
				 *       the section «Two forms of one container»
				 * @return map of the headers in the form of a storage
				 *
				 * \~
				 */
				explicit operator map_t() const noexcept;
				/**
				 * \~russian
				 * @brief Оператор получения мультикарты заголовков в виде хранилища
				 *
				 * @details Отдаёт содержимое как есть, сохраняя одноимённые заголовки
				 *
				 * @note Расхождение с fields_t и entries_t намеренно - см. README модуля,
				 *       раздел «Два вида одного контейнера»
				 *
				 * @return мультикарта заголовков в виде хранилища
				 *
				 * \~english
				 * @brief Operator of getting the multimap of the headers in the form of a storage
				 * @details It issues the content as it is, preserving the headers of the same name
				 * @note The divergence with fields_t and entries_t is deliberate - see the README of the module,
				 *       the section «Two forms of one container»
				 * @return multimap of the headers in the form of a storage
				 *
				 * \~
				 */
				explicit operator multimap_t() const noexcept;
			public:
				/**
				 * \~russian
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
				 * \~english
				 * @brief Operator of the extraction of the content of a header
				 * @details It is deliberately only for the reading: a writing through operator[] is not supported,
				 *          since the container admits several headers with one name
				 *          (unordered_multiset) and an unambiguous semantics of the writing is impossible.
				 *          For an addition/a replacement use emplace().
				 * @param name name of the header for the extraction
				 * @return     content of the header
				 *
				 * \~
				 */
				const string & operator[](string_view name) const noexcept;
			public:
				/**
				 * \~russian
				 * @brief Оператор слияния заголовков
				 *
				 * @param headers заголовки для слияния
				 * @return        текущий контейнер заголовков
				 *
				 * \~english
				 * @brief Operator of the merging of the headers
				 * @param headers headers for the merging
				 * @return        current container of the headers
				 *
				 * \~
				 */
				Headers & operator += (const Headers & headers) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Оператор сравнения двух заголовков
				 *
				 * @param headers заголовки для сравнения
				 * @return        результат сравнения
				 *
				 * \~english
				 * @brief Operator of the comparison of two headers
				 * @param headers headers for the comparison
				 * @return        result of the comparison
				 *
				 * \~
				 */
				bool operator == (const Headers & headers) const noexcept;
				/**
				 * \~russian
				 * @brief Оператор несравнения двух заголовков
				 *
				 * @param headers заголовки для сравнения
				 * @return        результат сравнения
				 *
				 * \~english
				 * @brief Operator of the non-comparison of two headers
				 * @param headers headers for the comparison
				 * @return        result of the comparison
				 *
				 * \~
				 */
				bool operator != (const Headers & headers) const noexcept;
			public:
				/**
				 * \~russian
				 * @brief Оператор перемещения
				 *
				 * @param headers заголовки для перемещения
				 * @return        текущий контейнер заголовков
				 *
				 * \~english
				 * @brief Operator of the moving
				 * @param headers headers for the moving
				 * @return        current container of the headers
				 *
				 * \~
				 */
				Headers & operator = (Headers && headers) noexcept;
				/**
				 * \~russian
				 * @brief Оператор копирования
				 *
				 * @param headers заголовки для копирования
				 * @return        текущий контейнер заголовков
				 *
				 * \~english
				 * @brief Operator of the copying
				 * @param headers headers for the copying
				 * @return        current container of the headers
				 *
				 * \~
				 */
				Headers & operator = (const Headers & headers) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Оператор установки протокола HTTP-запроса/ответа
				 *
				 * @param proto протокол HTTP-запроса/ответа
				 * @return      текущий контейнер заголовков
				 *
				 * \~english
				 * @brief Operator of setting the protocol of the HTTP request/answer
				 * @param proto protocol of the HTTP request/answer
				 * @return      current container of the headers
				 *
				 * \~
				 */
				Headers & operator = (const proto_t proto) noexcept;
				/**
				 * \~russian
				 * @brief Оператор установки объекта провайдера HTTP-запроса/ответа
				 *
				 * @param provider объект провайдера HTTP-запроса/ответа
				 * @return         текущий контейнер заголовков
				 *
				 * \~english
				 * @brief Operator of setting the object of the provider of the HTTP request/answer
				 * @param provider object of the provider of the HTTP request/answer
				 * @return         current container of the headers
				 *
				 * \~
				 */
				Headers & operator = (const provider_t * provider) noexcept;
				/**
				 * \~russian
				 * @brief Оператор установки объекта провайдера HTTP-запроса/ответа
				 *
				 * @param provider объект провайдера HTTP-запроса/ответа
				 * @return         текущий контейнер заголовков
				 *
				 * \~english
				 * @brief Operator of setting the object of the provider of the HTTP request/answer
				 * @param provider object of the provider of the HTTP request/answer
				 * @return         current container of the headers
				 *
				 * \~
				 */
				Headers & operator = (unique_ptr <provider_t> && provider) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Оператор копирования
				 *
				 * @param headers заголовки для копирования
				 * @return        текущий контейнер заголовков
				 *
				 * \~english
				 * @brief Operator of the copying
				 * @param headers headers for the copying
				 * @return        current container of the headers
				 *
				 * \~
				 */
				Headers & operator = (const fields_t & headers) noexcept;
				/**
				 * \~russian
				 * @brief Оператор копирования
				 *
				 * @param headers заголовки для копирования
				 * @return        текущий контейнер заголовков
				 *
				 * \~english
				 * @brief Operator of the copying
				 * @param headers headers for the copying
				 * @return        current container of the headers
				 *
				 * \~
				 */
				Headers & operator = (const entries_t & headers) noexcept;
				/**
				 * \~russian
				 * @brief Оператор копирования
				 *
				 * @param headers заголовки для копирования
				 * @return        текущий контейнер заголовков
				 *
				 * \~english
				 * @brief Operator of the copying
				 * @param headers headers for the copying
				 * @return        current container of the headers
				 *
				 * \~
				 */
				Headers & operator = (const multimap_t & headers) noexcept;
				/**
				 * \~russian
				 * @brief Оператор копирования
				 *
				 * @param headers заголовки для копирования
				 * @return        текущий контейнер заголовков
				 *
				 * \~english
				 * @brief Operator of the copying
				 * @param headers headers for the copying
				 * @return        current container of the headers
				 *
				 * \~
				 */
				Headers & operator = (initializer_list <header_t> headers) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Разрешаем пустое значение объекта
				 *
				 * \~english
				 * @brief We permit an empty value of the object
				 *
				 * \~
				 */
				explicit Headers() = default;
			public:
				/**
				 * \~russian
				 * @brief Конструктор перемещения
				 *
				 * @param headers заголовки для перемещения
				 *
				 * \~english
				 * @brief Constructor of the moving
				 * @param headers headers for the moving
				 *
				 * \~
				 */
				Headers(Headers && headers) noexcept;
				/**
				 * \~russian
				 * @brief Конструктор копирования
				 *
				 * @param headers заголовки для копирования
				 *
				 * \~english
				 * @brief Constructor of the copying
				 * @param headers headers for the copying
				 *
				 * \~
				 */
				Headers(const Headers & headers) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Конструктор
				 *
				 * @param proto протокол HTTP-запроса/ответа
				 *
				 * \~english
				 * @brief Constructor
				 * @param proto protocol of the HTTP request/answer
				 *
				 * \~
				 */
				explicit Headers(const proto_t proto) noexcept;
				/**
				 * \~russian
				 * @brief Конструктор
				 *
				 * @param provider объект провайдера HTTP-запроса/ответа
				 *
				 * \~english
				 * @brief Constructor
				 * @param provider object of the provider of the HTTP request/answer
				 *
				 * \~
				 */
				explicit Headers(const provider_t * provider) noexcept;
				/**
				 * \~russian
				 * @brief Конструктор
				 *
				 * @param provider объект провайдера HTTP-запроса/ответа
				 *
				 * \~english
				 * @brief Constructor
				 * @param provider object of the provider of the HTTP request/answer
				 *
				 * \~
				 */
				explicit Headers(unique_ptr <provider_t> && provider) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Конструктор
				 *
				 * @param headers список заголовков инициализации
				 *
				 * \~english
				 * @brief Constructor
				 * @param headers list of the headers of the initialization
				 *
				 * \~
				 */
				explicit Headers(const fields_t & headers) noexcept;
				/**
				 * \~russian
				 * @brief Конструктор
				 *
				 * @param headers список заголовков инициализации
				 *
				 * \~english
				 * @brief Constructor
				 * @param headers list of the headers of the initialization
				 *
				 * \~
				 */
				explicit Headers(const entries_t & headers) noexcept;
				/**
				 * \~russian
				 * @brief Конструктор
				 *
				 * @param headers список заголовков инициализации
				 *
				 * \~english
				 * @brief Constructor
				 * @param headers list of the headers of the initialization
				 *
				 * \~
				 */
				explicit Headers(const multimap_t & headers) noexcept;
				/**
				 * \~russian
				 * @brief Конструктор
				 *
				 * @param headers список заголовков инициализации
				 *
				 * \~english
				 * @brief Constructor
				 * @param headers list of the headers of the initialization
				 *
				 * \~
				 */
				explicit Headers(initializer_list <header_t> headers) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Конструктор
				 *
				 * @param proto   протокол HTTP-запроса/ответа
				 * @param headers список заголовков инициализации
				 *
				 * \~english
				 * @brief Constructor
				 * @param proto   protocol of the HTTP request/answer
				 * @param headers list of the headers of the initialization
				 *
				 * \~
				 */
				explicit Headers(const proto_t proto, const fields_t & headers) noexcept;
				/**
				 * \~russian
				 * @brief Конструктор
				 *
				 * @param proto   протокол HTTP-запроса/ответа
				 * @param headers список заголовков инициализации
				 *
				 * \~english
				 * @brief Constructor
				 * @param proto   protocol of the HTTP request/answer
				 * @param headers list of the headers of the initialization
				 *
				 * \~
				 */
				explicit Headers(const proto_t proto, const entries_t & headers) noexcept;
				/**
				 * \~russian
				 * @brief Конструктор
				 *
				 * @param proto   протокол HTTP-запроса/ответа
				 * @param headers список заголовков инициализации
				 *
				 * \~english
				 * @brief Constructor
				 * @param proto   protocol of the HTTP request/answer
				 * @param headers list of the headers of the initialization
				 *
				 * \~
				 */
				explicit Headers(const proto_t proto, const multimap_t & headers) noexcept;
				/**
				 * \~russian
				 * @brief Конструктор
				 *
				 * @param proto   протокол HTTP-запроса/ответа
				 * @param headers список заголовков инициализации
				 *
				 * \~english
				 * @brief Constructor
				 * @param proto   protocol of the HTTP request/answer
				 * @param headers list of the headers of the initialization
				 *
				 * \~
				 */
				explicit Headers(const proto_t proto, initializer_list <header_t> headers) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Конструктор
				 *
				 * @param provider объект провайдера HTTP-запроса/ответа
				 * @param headers  список заголовков инициализации
				 *
				 * \~english
				 * @brief Constructor
				 * @param provider object of the provider of the HTTP request/answer
				 * @param headers  list of the headers of the initialization
				 *
				 * \~
				 */
				explicit Headers(const provider_t * provider, const fields_t & headers) noexcept;
				/**
				 * \~russian
				 * @brief Конструктор
				 *
				 * @param provider объект провайдера HTTP-запроса/ответа
				 * @param headers  список заголовков инициализации
				 *
				 * \~english
				 * @brief Constructor
				 * @param provider object of the provider of the HTTP request/answer
				 * @param headers  list of the headers of the initialization
				 *
				 * \~
				 */
				explicit Headers(const provider_t * provider, const entries_t & headers) noexcept;
				/**
				 * \~russian
				 * @brief Конструктор
				 *
				 * @param provider объект провайдера HTTP-запроса/ответа
				 * @param headers  список заголовков инициализации
				 *
				 * \~english
				 * @brief Constructor
				 * @param provider object of the provider of the HTTP request/answer
				 * @param headers  list of the headers of the initialization
				 *
				 * \~
				 */
				explicit Headers(const provider_t * provider, const multimap_t & headers) noexcept;
				/**
				 * \~russian
				 * @brief Конструктор
				 *
				 * @param provider объект провайдера HTTP-запроса/ответа
				 * @param headers  список заголовков инициализации
				 *
				 * \~english
				 * @brief Constructor
				 * @param provider object of the provider of the HTTP request/answer
				 * @param headers  list of the headers of the initialization
				 *
				 * \~
				 */
				explicit Headers(const provider_t * provider, initializer_list <header_t> headers) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Конструктор
				 *
				 * @param provider объект провайдера HTTP-запроса/ответа
				 * @param headers  список заголовков инициализации
				 *
				 * \~english
				 * @brief Constructor
				 * @param provider object of the provider of the HTTP request/answer
				 * @param headers  list of the headers of the initialization
				 *
				 * \~
				 */
				explicit Headers(unique_ptr <provider_t> && provider, const fields_t & headers) noexcept;
				/**
				 * \~russian
				 * @brief Конструктор
				 *
				 * @param provider объект провайдера HTTP-запроса/ответа
				 * @param headers  список заголовков инициализации
				 *
				 * \~english
				 * @brief Constructor
				 * @param provider object of the provider of the HTTP request/answer
				 * @param headers  list of the headers of the initialization
				 *
				 * \~
				 */
				explicit Headers(unique_ptr <provider_t> && provider, const entries_t & headers) noexcept;
				/**
				 * \~russian
				 * @brief Конструктор
				 *
				 * @param provider объект провайдера HTTP-запроса/ответа
				 * @param headers  список заголовков инициализации
				 *
				 * \~english
				 * @brief Constructor
				 * @param provider object of the provider of the HTTP request/answer
				 * @param headers  list of the headers of the initialization
				 *
				 * \~
				 */
				explicit Headers(unique_ptr <provider_t> && provider, const multimap_t & headers) noexcept;
				/**
				 * \~russian
				 * @brief Конструктор
				 *
				 * @param provider объект провайдера HTTP-запроса/ответа
				 * @param headers  список заголовков инициализации
				 *
				 * \~english
				 * @brief Constructor
				 * @param provider object of the provider of the HTTP request/answer
				 * @param headers  list of the headers of the initialization
				 *
				 * \~
				 */
				explicit Headers(unique_ptr <provider_t> && provider, initializer_list <header_t> headers) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Конструктор
				 *
				 * @param proto    протокол HTTP-запроса/ответа
				 * @param provider объект провайдера HTTP-запроса/ответа
				 * @param headers  список заголовков инициализации
				 *
				 * \~english
				 * @brief Constructor
				 * @param proto    protocol of the HTTP request/answer
				 * @param provider object of the provider of the HTTP request/answer
				 * @param headers  list of the headers of the initialization
				 *
				 * \~
				 */
				explicit Headers(const proto_t proto, const provider_t * provider, const fields_t & headers) noexcept;
				/**
				 * \~russian
				 * @brief Конструктор
				 *
				 * @param proto    протокол HTTP-запроса/ответа
				 * @param provider объект провайдера HTTP-запроса/ответа
				 * @param headers  список заголовков инициализации
				 *
				 * \~english
				 * @brief Constructor
				 * @param proto    protocol of the HTTP request/answer
				 * @param provider object of the provider of the HTTP request/answer
				 * @param headers  list of the headers of the initialization
				 *
				 * \~
				 */
				explicit Headers(const proto_t proto, const provider_t * provider, const entries_t & headers) noexcept;
				/**
				 * \~russian
				 * @brief Конструктор
				 *
				 * @param proto    протокол HTTP-запроса/ответа
				 * @param provider объект провайдера HTTP-запроса/ответа
				 * @param headers  список заголовков инициализации
				 *
				 * \~english
				 * @brief Constructor
				 * @param proto    protocol of the HTTP request/answer
				 * @param provider object of the provider of the HTTP request/answer
				 * @param headers  list of the headers of the initialization
				 *
				 * \~
				 */
				explicit Headers(const proto_t proto, const provider_t * provider, const multimap_t & headers) noexcept;
				/**
				 * \~russian
				 * @brief Конструктор
				 *
				 * @param proto    протокол HTTP-запроса/ответа
				 * @param provider объект провайдера HTTP-запроса/ответа
				 * @param headers  список заголовков инициализации
				 *
				 * \~english
				 * @brief Constructor
				 * @param proto    protocol of the HTTP request/answer
				 * @param provider object of the provider of the HTTP request/answer
				 * @param headers  list of the headers of the initialization
				 *
				 * \~
				 */
				explicit Headers(const proto_t proto, const provider_t * provider, initializer_list <header_t> headers) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Конструктор
				 *
				 * @param proto    протокол HTTP-запроса/ответа
				 * @param provider объект провайдера HTTP-запроса/ответа
				 * @param headers  список заголовков инициализации
				 *
				 * \~english
				 * @brief Constructor
				 * @param proto    protocol of the HTTP request/answer
				 * @param provider object of the provider of the HTTP request/answer
				 * @param headers  list of the headers of the initialization
				 *
				 * \~
				 */
				explicit Headers(const proto_t proto, unique_ptr <provider_t> && provider, const fields_t & headers) noexcept;
				/**
				 * \~russian
				 * @brief Конструктор
				 *
				 * @param proto    протокол HTTP-запроса/ответа
				 * @param provider объект провайдера HTTP-запроса/ответа
				 * @param headers  список заголовков инициализации
				 *
				 * \~english
				 * @brief Constructor
				 * @param proto    protocol of the HTTP request/answer
				 * @param provider object of the provider of the HTTP request/answer
				 * @param headers  list of the headers of the initialization
				 *
				 * \~
				 */
				explicit Headers(const proto_t proto, unique_ptr <provider_t> && provider, const entries_t & headers) noexcept;
				/**
				 * \~russian
				 * @brief Конструктор
				 *
				 * @param proto    протокол HTTP-запроса/ответа
				 * @param provider объект провайдера HTTP-запроса/ответа
				 * @param headers  список заголовков инициализации
				 *
				 * \~english
				 * @brief Constructor
				 * @param proto    protocol of the HTTP request/answer
				 * @param provider object of the provider of the HTTP request/answer
				 * @param headers  list of the headers of the initialization
				 *
				 * \~
				 */
				explicit Headers(const proto_t proto, unique_ptr <provider_t> && provider, const multimap_t & headers) noexcept;
				/**
				 * \~russian
				 * @brief Конструктор
				 *
				 * @param proto    протокол HTTP-запроса/ответа
				 * @param provider объект провайдера HTTP-запроса/ответа
				 * @param headers  список заголовков инициализации
				 *
				 * \~english
				 * @brief Constructor
				 * @param proto    protocol of the HTTP request/answer
				 * @param provider object of the provider of the HTTP request/answer
				 * @param headers  list of the headers of the initialization
				 *
				 * \~
				 */
				explicit Headers(const proto_t proto, unique_ptr <provider_t> && provider, initializer_list <header_t> headers) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Конструктор
				 *
				 * @param fmk объект фреймворка
				 * @param log объект для работы с логами
				 *
				 *
				 * \~english
				 * @brief Constructor
				 * @param fmk framework object
				 * @param log object for working with logs
				 *
				 * \~
				 */
				explicit Headers(const fmk_t * fmk, const log_t * log) noexcept;
				/**
				 * \~russian
				 * @brief Конструктор
				 *
				 * @param proto протокол HTTP-запроса/ответа
				 *
				 * \~english
				 * @brief Constructor
				 * @param proto protocol of the HTTP request/answer
				 *
				 * \~
				 */
				explicit Headers(const proto_t proto, const fmk_t * fmk, const log_t * log) noexcept;
				/**
				 * \~russian
				 * @brief Конструктор
				 *
				 * @param provider объект провайдера HTTP-запроса/ответа
				 *
				 * \~english
				 * @brief Constructor
				 * @param provider object of the provider of the HTTP request/answer
				 *
				 * \~
				 */
				explicit Headers(const provider_t * provider, const fmk_t * fmk, const log_t * log) noexcept;
				/**
				 * \~russian
				 * @brief Конструктор
				 *
				 * @param provider объект провайдера HTTP-запроса/ответа
				 *
				 * \~english
				 * @brief Constructor
				 * @param provider object of the provider of the HTTP request/answer
				 *
				 * \~
				 */
				explicit Headers(unique_ptr <provider_t> && provider, const fmk_t * fmk, const log_t * log) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Конструктор
				 *
				 * @param headers список заголовков инициализации
				 * @param fmk     объект фреймворка
				 * @param log     объект для работы с логами
				 *
				 * \~english
				 * @brief Constructor
				 * @param headers list of the headers of the initialization
				 * @param fmk     object of the framework
				 * @param log     object for the work with the logs
				 *
				 * \~
				 */
				explicit Headers(const fields_t & headers, const fmk_t * fmk, const log_t * log) noexcept;
				/**
				 * \~russian
				 * @brief Конструктор
				 *
				 * @param headers список заголовков инициализации
				 * @param fmk     объект фреймворка
				 * @param log     объект для работы с логами
				 *
				 * \~english
				 * @brief Constructor
				 * @param headers list of the headers of the initialization
				 * @param fmk     object of the framework
				 * @param log     object for the work with the logs
				 *
				 * \~
				 */
				explicit Headers(const entries_t & headers, const fmk_t * fmk, const log_t * log) noexcept;
				/**
				 * \~russian
				 * @brief Конструктор
				 *
				 * @param headers список заголовков инициализации
				 * @param fmk     объект фреймворка
				 * @param log     объект для работы с логами
				 *
				 * \~english
				 * @brief Constructor
				 * @param headers list of the headers of the initialization
				 * @param fmk     object of the framework
				 * @param log     object for the work with the logs
				 *
				 * \~
				 */
				explicit Headers(const multimap_t & headers, const fmk_t * fmk, const log_t * log) noexcept;
				/**
				 * \~russian
				 * @brief Конструктор
				 *
				 * @param headers список заголовков инициализации
				 * @param fmk     объект фреймворка
				 * @param log     объект для работы с логами
				 *
				 * \~english
				 * @brief Constructor
				 * @param headers list of the headers of the initialization
				 * @param fmk     object of the framework
				 * @param log     object for the work with the logs
				 *
				 * \~
				 */
				explicit Headers(initializer_list <header_t> headers, const fmk_t * fmk, const log_t * log) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Конструктор
				 *
				 * @param proto   протокол HTTP-запроса/ответа
				 * @param headers список заголовков инициализации
				 * @param fmk     объект фреймворка
				 * @param log     объект для работы с логами
				 *
				 * \~english
				 * @brief Constructor
				 * @param proto   protocol of the HTTP request/answer
				 * @param headers list of the headers of the initialization
				 * @param fmk     object of the framework
				 * @param log     object for the work with the logs
				 *
				 * \~
				 */
				explicit Headers(const proto_t proto, const fields_t & headers, const fmk_t * fmk, const log_t * log) noexcept;
				/**
				 * \~russian
				 * @brief Конструктор
				 *
				 * @param proto   протокол HTTP-запроса/ответа
				 * @param headers список заголовков инициализации
				 * @param fmk     объект фреймворка
				 * @param log     объект для работы с логами
				 *
				 * \~english
				 * @brief Constructor
				 * @param proto   protocol of the HTTP request/answer
				 * @param headers list of the headers of the initialization
				 * @param fmk     object of the framework
				 * @param log     object for the work with the logs
				 *
				 * \~
				 */
				explicit Headers(const proto_t proto, const entries_t & headers, const fmk_t * fmk, const log_t * log) noexcept;
				/**
				 * \~russian
				 * @brief Конструктор
				 *
				 * @param proto   протокол HTTP-запроса/ответа
				 * @param headers список заголовков инициализации
				 * @param fmk     объект фреймворка
				 * @param log     объект для работы с логами
				 *
				 * \~english
				 * @brief Constructor
				 * @param proto   protocol of the HTTP request/answer
				 * @param headers list of the headers of the initialization
				 * @param fmk     object of the framework
				 * @param log     object for the work with the logs
				 *
				 * \~
				 */
				explicit Headers(const proto_t proto, const multimap_t & headers, const fmk_t * fmk, const log_t * log) noexcept;
				/**
				 * \~russian
				 * @brief Конструктор
				 *
				 * @param proto   протокол HTTP-запроса/ответа
				 * @param headers список заголовков инициализации
				 * @param fmk     объект фреймворка
				 * @param log     объект для работы с логами
				 *
				 * \~english
				 * @brief Constructor
				 * @param proto   protocol of the HTTP request/answer
				 * @param headers list of the headers of the initialization
				 * @param fmk     object of the framework
				 * @param log     object for the work with the logs
				 *
				 * \~
				 */
				explicit Headers(const proto_t proto, initializer_list <header_t> headers, const fmk_t * fmk, const log_t * log) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Конструктор
				 *
				 * @param provider объект провайдера HTTP-запроса/ответа
				 * @param headers  список заголовков инициализации
				 * @param fmk      объект фреймворка
				 * @param log      объект для работы с логами
				 *
				 * \~english
				 * @brief Constructor
				 * @param provider object of the provider of the HTTP request/answer
				 * @param headers  list of the headers of the initialization
				 * @param fmk      object of the framework
				 * @param log      object for the work with the logs
				 *
				 * \~
				 */
				explicit Headers(const provider_t * provider, const fields_t & headers, const fmk_t * fmk, const log_t * log) noexcept;
				/**
				 * \~russian
				 * @brief Конструктор
				 *
				 * @param provider объект провайдера HTTP-запроса/ответа
				 * @param headers  список заголовков инициализации
				 * @param fmk      объект фреймворка
				 * @param log      объект для работы с логами
				 *
				 * \~english
				 * @brief Constructor
				 * @param provider object of the provider of the HTTP request/answer
				 * @param headers  list of the headers of the initialization
				 * @param fmk      object of the framework
				 * @param log      object for the work with the logs
				 *
				 * \~
				 */
				explicit Headers(const provider_t * provider, const entries_t & headers, const fmk_t * fmk, const log_t * log) noexcept;
				/**
				 * \~russian
				 * @brief Конструктор
				 *
				 * @param provider объект провайдера HTTP-запроса/ответа
				 * @param headers  список заголовков инициализации
				 * @param fmk      объект фреймворка
				 * @param log      объект для работы с логами
				 *
				 * \~english
				 * @brief Constructor
				 * @param provider object of the provider of the HTTP request/answer
				 * @param headers  list of the headers of the initialization
				 * @param fmk      object of the framework
				 * @param log      object for the work with the logs
				 *
				 * \~
				 */
				explicit Headers(const provider_t * provider, const multimap_t & headers, const fmk_t * fmk, const log_t * log) noexcept;
				/**
				 * \~russian
				 * @brief Конструктор
				 *
				 * @param provider объект провайдера HTTP-запроса/ответа
				 * @param headers  список заголовков инициализации
				 * @param fmk      объект фреймворка
				 * @param log      объект для работы с логами
				 *
				 * \~english
				 * @brief Constructor
				 * @param provider object of the provider of the HTTP request/answer
				 * @param headers  list of the headers of the initialization
				 * @param fmk      object of the framework
				 * @param log      object for the work with the logs
				 *
				 * \~
				 */
				explicit Headers(const provider_t * provider, initializer_list <header_t> headers, const fmk_t * fmk, const log_t * log) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Конструктор
				 *
				 * @param provider объект провайдера HTTP-запроса/ответа
				 * @param headers  список заголовков инициализации
				 * @param fmk      объект фреймворка
				 * @param log      объект для работы с логами
				 *
				 * \~english
				 * @brief Constructor
				 * @param provider object of the provider of the HTTP request/answer
				 * @param headers  list of the headers of the initialization
				 * @param fmk      object of the framework
				 * @param log      object for the work with the logs
				 *
				 * \~
				 */
				explicit Headers(unique_ptr <provider_t> && provider, const fields_t & headers, const fmk_t * fmk, const log_t * log) noexcept;
				/**
				 * \~russian
				 * @brief Конструктор
				 *
				 * @param provider объект провайдера HTTP-запроса/ответа
				 * @param headers  список заголовков инициализации
				 * @param fmk      объект фреймворка
				 * @param log      объект для работы с логами
				 *
				 * \~english
				 * @brief Constructor
				 * @param provider object of the provider of the HTTP request/answer
				 * @param headers  list of the headers of the initialization
				 * @param fmk      object of the framework
				 * @param log      object for the work with the logs
				 *
				 * \~
				 */
				explicit Headers(unique_ptr <provider_t> && provider, const entries_t & headers, const fmk_t * fmk, const log_t * log) noexcept;
				/**
				 * \~russian
				 * @brief Конструктор
				 *
				 * @param provider объект провайдера HTTP-запроса/ответа
				 * @param headers  список заголовков инициализации
				 * @param fmk      объект фреймворка
				 * @param log      объект для работы с логами
				 *
				 * \~english
				 * @brief Constructor
				 * @param provider object of the provider of the HTTP request/answer
				 * @param headers  list of the headers of the initialization
				 * @param fmk      object of the framework
				 * @param log      object for the work with the logs
				 *
				 * \~
				 */
				explicit Headers(unique_ptr <provider_t> && provider, const multimap_t & headers, const fmk_t * fmk, const log_t * log) noexcept;
				/**
				 * \~russian
				 * @brief Конструктор
				 *
				 * @param provider объект провайдера HTTP-запроса/ответа
				 * @param headers  список заголовков инициализации
				 * @param fmk      объект фреймворка
				 * @param log      объект для работы с логами
				 *
				 * \~english
				 * @brief Constructor
				 * @param provider object of the provider of the HTTP request/answer
				 * @param headers  list of the headers of the initialization
				 * @param fmk      object of the framework
				 * @param log      object for the work with the logs
				 *
				 * \~
				 */
				explicit Headers(unique_ptr <provider_t> && provider, initializer_list <header_t> headers, const fmk_t * fmk, const log_t * log) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Конструктор
				 *
				 * @param proto    протокол HTTP-запроса/ответа
				 * @param provider объект провайдера HTTP-запроса/ответа
				 * @param headers  список заголовков инициализации
				 * @param fmk      объект фреймворка
				 * @param log      объект для работы с логами
				 *
				 * \~english
				 * @brief Constructor
				 * @param proto    protocol of the HTTP request/answer
				 * @param provider object of the provider of the HTTP request/answer
				 * @param headers  list of the headers of the initialization
				 * @param fmk      object of the framework
				 * @param log      object for the work with the logs
				 *
				 * \~
				 */
				explicit Headers(const proto_t proto, const provider_t * provider, const fields_t & headers, const fmk_t * fmk, const log_t * log) noexcept;
				/**
				 * \~russian
				 * @brief Конструктор
				 *
				 * @param proto    протокол HTTP-запроса/ответа
				 * @param provider объект провайдера HTTP-запроса/ответа
				 * @param headers  список заголовков инициализации
				 * @param fmk      объект фреймворка
				 * @param log      объект для работы с логами
				 *
				 * \~english
				 * @brief Constructor
				 * @param proto    protocol of the HTTP request/answer
				 * @param provider object of the provider of the HTTP request/answer
				 * @param headers  list of the headers of the initialization
				 * @param fmk      object of the framework
				 * @param log      object for the work with the logs
				 *
				 * \~
				 */
				explicit Headers(const proto_t proto, const provider_t * provider, const entries_t & headers, const fmk_t * fmk, const log_t * log) noexcept;
				/**
				 * \~russian
				 * @brief Конструктор
				 *
				 * @param proto    протокол HTTP-запроса/ответа
				 * @param provider объект провайдера HTTP-запроса/ответа
				 * @param headers  список заголовков инициализации
				 * @param fmk      объект фреймворка
				 * @param log      объект для работы с логами
				 *
				 * \~english
				 * @brief Constructor
				 * @param proto    protocol of the HTTP request/answer
				 * @param provider object of the provider of the HTTP request/answer
				 * @param headers  list of the headers of the initialization
				 * @param fmk      object of the framework
				 * @param log      object for the work with the logs
				 *
				 * \~
				 */
				explicit Headers(const proto_t proto, const provider_t * provider, const multimap_t & headers, const fmk_t * fmk, const log_t * log) noexcept;
				/**
				 * \~russian
				 * @brief Конструктор
				 *
				 * @param proto    протокол HTTP-запроса/ответа
				 * @param provider объект провайдера HTTP-запроса/ответа
				 * @param headers  список заголовков инициализации
				 * @param fmk      объект фреймворка
				 * @param log      объект для работы с логами
				 *
				 * \~english
				 * @brief Constructor
				 * @param proto    protocol of the HTTP request/answer
				 * @param provider object of the provider of the HTTP request/answer
				 * @param headers  list of the headers of the initialization
				 * @param fmk      object of the framework
				 * @param log      object for the work with the logs
				 *
				 * \~
				 */
				explicit Headers(const proto_t proto, const provider_t * provider, initializer_list <header_t> headers, const fmk_t * fmk, const log_t * log) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Конструктор
				 *
				 * @param proto    протокол HTTP-запроса/ответа
				 * @param provider объект провайдера HTTP-запроса/ответа
				 * @param headers  список заголовков инициализации
				 * @param fmk      объект фреймворка
				 * @param log      объект для работы с логами
				 *
				 * \~english
				 * @brief Constructor
				 * @param proto    protocol of the HTTP request/answer
				 * @param provider object of the provider of the HTTP request/answer
				 * @param headers  list of the headers of the initialization
				 * @param fmk      object of the framework
				 * @param log      object for the work with the logs
				 *
				 * \~
				 */
				explicit Headers(const proto_t proto, unique_ptr <provider_t> && provider, const fields_t & headers, const fmk_t * fmk, const log_t * log) noexcept;
				/**
				 * \~russian
				 * @brief Конструктор
				 *
				 * @param proto    протокол HTTP-запроса/ответа
				 * @param provider объект провайдера HTTP-запроса/ответа
				 * @param headers  список заголовков инициализации
				 * @param fmk      объект фреймворка
				 * @param log      объект для работы с логами
				 *
				 * \~english
				 * @brief Constructor
				 * @param proto    protocol of the HTTP request/answer
				 * @param provider object of the provider of the HTTP request/answer
				 * @param headers  list of the headers of the initialization
				 * @param fmk      object of the framework
				 * @param log      object for the work with the logs
				 *
				 * \~
				 */
				explicit Headers(const proto_t proto, unique_ptr <provider_t> && provider, const entries_t & headers, const fmk_t * fmk, const log_t * log) noexcept;
				/**
				 * \~russian
				 * @brief Конструктор
				 *
				 * @param proto    протокол HTTP-запроса/ответа
				 * @param provider объект провайдера HTTP-запроса/ответа
				 * @param headers  список заголовков инициализации
				 * @param fmk      объект фреймворка
				 * @param log      объект для работы с логами
				 *
				 * \~english
				 * @brief Constructor
				 * @param proto    protocol of the HTTP request/answer
				 * @param provider object of the provider of the HTTP request/answer
				 * @param headers  list of the headers of the initialization
				 * @param fmk      object of the framework
				 * @param log      object for the work with the logs
				 *
				 * \~
				 */
				explicit Headers(const proto_t proto, unique_ptr <provider_t> && provider, const multimap_t & headers, const fmk_t * fmk, const log_t * log) noexcept;
				/**
				 * \~russian
				 * @brief Конструктор
				 *
				 * @param proto    протокол HTTP-запроса/ответа
				 * @param provider объект провайдера HTTP-запроса/ответа
				 * @param headers  список заголовков инициализации
				 * @param fmk      объект фреймворка
				 * @param log      объект для работы с логами
				 *
				 * \~english
				 * @brief Constructor
				 * @param proto    protocol of the HTTP request/answer
				 * @param provider object of the provider of the HTTP request/answer
				 * @param headers  list of the headers of the initialization
				 * @param fmk      object of the framework
				 * @param log      object for the work with the logs
				 *
				 * \~
				 */
				explicit Headers(const proto_t proto, unique_ptr <provider_t> && provider, initializer_list <header_t> headers, const fmk_t * fmk, const log_t * log) noexcept;
			public:
				/**
				 * \~russian
				 * @brief Деструктор
				 *
				 *
				 * \~english
				 * @brief Destructor
				 *
				 * \~
				 */
				~Headers() noexcept;
		} headers_t;
	};
	/**
	 * \~russian
	 * @brief Оператор [<<] вывода в поток буфера
	 *
	 * @param os      поток куда нужно вывести данные
	 * @param headers контейнер заголовков
	 *
	 * \~english
	 * @brief Operator [<<] of the output into a stream of the buffer
	 * @param os      stream where it is needed to output the data
	 * @param headers container of the headers
	 *
	 * \~
	 */
	__AWH_SHARED_EXPORT__ ostream & operator << (ostream & os, const http::headers_t & headers) noexcept;
};

#endif // __AWH_HTTP_HEADERS__
