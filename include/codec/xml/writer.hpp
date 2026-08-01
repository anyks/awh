/**
 * @file: writer.hpp
 * @date: 2026-08-01
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @brief Заголовочный файл записи текста разметки XML — класс Writer, собирающий правильно построенный
 *        текст разметки последовательными указаниями, с экранированием содержимого,
 *        объявлением пространств имён и необязательным отступом
 *
 * @copyright: Copyright © 2026
 *
 */

/**
 * Экранируем повторную инициализацию модуля
 */
#ifndef __AWH_CODEC_XML_WRITER__
#define __AWH_CODEC_XML_WRITER__

/**
 * Стандартные заголовочные файлы
 */
#include <string>
#include <vector>
#include <cstdint>
#include <string_view>

/**
 * Подключаем заголовочные файлы модуля
 */
#include "document.hpp"

/**
 * @brief Основное пространство имён
 *
 */
namespace awh {
	/**
	 * Используем стандартное пространство имён
	 */
	using namespace std;

	/**
	 * @brief Пространство имён контейнеров данных
	 *
	 */
	namespace codec {
		/**
		 * @brief Пространство имён контейнера XML
		 *
		 */
		namespace xml {
			/**
			 * @brief Класс записи текста разметки
			 *
			 * @details Собирает текст разметки последовательными указаниями об открытии и
			 * закрытии узлов. Запись сама следит за парностью меток, экранирует содержимое
			 * и не позволяет собрать неправильно построенный текст: указание, нарушающее
			 * строение, отвергается, а не записывается
			 *
			 * @par Порядок работы
			 *
			 * @code{.cpp}
			 * writer_t writer;
			 *
			 * writer.declaration();
			 * writer.open("Envelope", "http://schemas.xmlsoap.org/soap/envelope/");
			 * writer.attribute("encodingStyle", "http://schemas.xmlsoap.org/soap/encoding/");
			 * writer.open("Body");
			 * writer.close();
			 * writer.close();
			 *
			 * const string & result = writer.text();
			 * @endcode
			 *
			 * @note Записывается текст в кодировке UTF-8. Прочие кодировки при записи не
			 * применяются намеренно: они лишь сужают круг тех, кто сможет текст прочесть
			 *
			 */
			typedef class __AWH_SHARED_EXPORT__ Writer {
				public:
					/**
					 * @brief Настройки записи текста разметки
					 *
					 */
					typedef struct __AWH_SHARED_EXPORT__ Settings {
						// Вид записи собираемого текста разметки
						format_t format;
						// Флаг записи узлов без содержимого самозакрывающейся меткой
						bool collapse;
						// Флаг экранирования знаков, выходящих за пределы US-ASCII, числовыми ссылками
						bool escapeNonAscii;
						// Количество пробелов одного уровня отступа
						uint8_t indent;
						// Знак отступа, обычно пробел либо горизонтальная табуляция
						char space;
						/**
						 * @brief Конструктор
						 *
						 */
						Settings() noexcept;
					} settings_t;
				private:
					/**
					 * @brief Запись открытого узла разметки
					 *
					 */
					typedef struct Opened {
						// Имя узла в записи, принятой в исходном тексте
						string name;
						// Признак того, что метка узла ещё не завершена
						bool pending;
						// Признак того, что узел уже содержит вложенное содержимое
						bool filled;
						// Признак того, что узел содержит вложенные узлы разметки
						bool elements;
						// Количество связываний префиксов, объявленных узлом
						uint32_t bindings;
						/**
						 * Записанные имена атрибутов узла в том виде, в каком они попали
						 * в текст. Договор не допускает повторного имени атрибута у одного
						 * узла, а объявление пространства имён является таким же атрибутом:
						 * без учёта записанного запись собирала бы текст, который её же
						 * собственное чтение принять не может
						 */
						vector <string> names;
						/**
						 * @brief Конструктор
						 *
						 */
						Opened() noexcept : pending(false), filled(false), elements(false), bindings(0) {}
					} opened_t;
					/**
					 * @brief Связывание префикса с пространством имён при записи
					 *
					 */
					typedef struct Scope {
						// Префикс без разделителя
						string prefix;
						// Обозначение связанного пространства имён
						string uri;
						/**
						 * @brief Конструктор
						 *
						 */
						Scope() noexcept {}
					} scope_t;
				private:
					// Настройки записи текста разметки
					settings_t _settings;
				private:
					// Код ошибки последней операции записи
					error_t _error;
				private:
					// Признак того, что корневой узел разметки уже записан
					bool _root;
				private:
					// Собираемый текст разметки
					string _text;
				private:
					// Стек открытых узлов разметки
					vector <opened_t> _opened;
				private:
					// Действующие связывания префиксов с пространствами имён
					vector <scope_t> _scopes;
				private:
					/**
					 * @brief Метод завершения незакрытой метки узла
					 *
					 */
					void flush() noexcept;
					/**
					 * @brief Метод записи отступа перед содержимым
					 *
					 */
					void indent() noexcept;
					/**
					 * @brief Метод записи последовательности знаков с экранированием
					 *
					 * @param text      записываемая последовательность знаков
					 * @param attribute признак записи значения атрибута
					 * @return          результат выполнения операции
					 *
					 */
					bool escape(const string_view text, const bool attribute) noexcept;
					/**
					 * @brief Метод проверки допустимости знаков дословно записываемой последовательности
					 *
					 * @details Содержимое примечания, дословного раздела и указания обработчику
					 * попадает в текст без экранирования, и проверка знаков экранированием там
					 * не выполняется. Проверять их всё равно необходимо: недопустимый в
					 * разметке знак либо ошибочная последовательность кодировки делают
					 * собранный текст непрочитываемым
					 *
					 * @param text проверяемая последовательность знаков
					 * @return     результат выполнения операции
					 *
					 */
					bool verify(const string_view text) noexcept;
					/**
					 * @brief Метод получения префикса для пространства имён
					 *
					 * @details Обозначение, ещё не связанное ни с одним префиксом,
					 * объявляется при записываемом узле с самостоятельно назначенным префиксом
					 *
					 * @param uri       обозначение пространства имён
					 * @param attribute признак поиска префикса для имени атрибута
					 * @param result    найденный либо назначенный префикс
					 * @return          результат выполнения операции
					 *
					 */
					bool prefix(const string_view uri, const bool attribute, string & result) noexcept;
					/**
					 * @brief Метод занятия имени атрибута открытым узлом
					 *
					 * @details Договор не допускает повторного имени атрибута у одного узла,
					 * а объявление пространства имён является таким же атрибутом. Без учёта
					 * записанного запись собирала бы текст, который её же собственное чтение
					 * принять не может
					 *
					 * @param name имя атрибута в том виде, в каком оно попадает в текст
					 * @return     результат выполнения операции
					 *
					 */
					bool occupy(const string_view name) noexcept;
				private:
					// Счётчик самостоятельно назначенных префиксов пространств имён
					uint32_t _counter;
				public:
					/**
					 * @brief Метод получения текущих настроек записи
					 *
					 * @return текущие настройки записи текста разметки
					 *
					 */
					const settings_t & settings() const noexcept;
					/**
					 * @brief Метод установки настроек записи
					 *
					 * @param settings настройки записи текста разметки
					 *
					 */
					void settings(const settings_t & settings) noexcept;
				public:
					/**
					 * @brief Метод записи объявления разметки
					 *
					 * @details Записывается первым указанием, до всякого содержимого
					 *
					 * @param standalone признак самодостаточности текста разметки
					 * @return           результат выполнения операции
					 *
					 */
					bool declaration(const standalone_t standalone = standalone_t::NONE) noexcept;
				public:
					/**
					 * @brief Метод открытия узла разметки
					 *
					 * @details Непустое обозначение пространства имён объявляется при узле,
					 * если оно ещё не объявлено выше по стеку открытых узлов
					 *
					 * @param local местное имя открываемого узла
					 * @param uri   обозначение пространства имён открываемого узла
					 * @return      результат выполнения операции
					 *
					 */
					bool open(const string_view local, const string_view uri = "") noexcept;
					/**
					 * @brief Метод закрытия последнего открытого узла
					 *
					 * @return результат выполнения операции
					 *
					 */
					bool close() noexcept;
				public:
					/**
					 * @brief Метод записи атрибута открытого узла
					 *
					 * @warning Записывается только сразу после открытия узла, до его
					 * содержимого: дописать атрибут узлу, у которого уже есть содержимое,
					 * нельзя
					 *
					 * @param local местное имя атрибута
					 * @param value значение атрибута
					 * @param uri   обозначение пространства имён атрибута
					 * @return      результат выполнения операции
					 *
					 */
					bool attribute(const string_view local, const string_view value, const string_view uri = "") noexcept;
					/**
					 * @brief Метод объявления пространства имён при открытом узле
					 *
					 * @param prefix префикс без разделителя, пустой для объявления по умолчанию
					 * @param uri    обозначение объявляемого пространства имён
					 * @return       результат выполнения операции
					 *
					 */
					bool binding(const string_view prefix, const string_view uri) noexcept;
				public:
					/**
					 * @brief Метод записи текстового содержимого
					 *
					 * @details Знаки, имеющие в разметке особый смысл, экранируются
					 *
					 * @param text записываемое текстовое содержимое
					 * @return     результат выполнения операции
					 *
					 */
					bool text(const string_view text) noexcept;
					/**
					 * @brief Метод записи раздела дословного текста
					 *
					 * @warning Содержимое записывается без экранирования, и завершающая
					 * раздел последовательность внутри него недопустима. Такое содержимое
					 * отвергается, а не разрезается на несколько разделов
					 *
					 * @param text записываемое дословное содержимое
					 * @return     результат выполнения операции
					 *
					 */
					bool cdata(const string_view text) noexcept;
					/**
					 * @brief Метод записи примечания
					 *
					 * @param text содержимое примечания
					 * @return     результат выполнения операции
					 *
					 */
					bool comment(const string_view text) noexcept;
					/**
					 * @brief Метод записи указания обработчику
					 *
					 * @param target цель указания обработчику
					 * @param text   данные указания обработчику
					 * @return       результат выполнения операции
					 *
					 */
					bool processing(const string_view target, const string_view text) noexcept;
				public:
					/**
					 * @brief Метод записи узла с текстовым содержимым
					 *
					 * @details Равнозначен открытию узла, записи содержимого и закрытию:
					 * такое сочетание в разметке встречается чаще прочих
					 *
					 * @param local местное имя записываемого узла
					 * @param value текстовое содержимое записываемого узла
					 * @param uri   обозначение пространства имён записываемого узла
					 * @return      результат выполнения операции
					 *
					 */
					bool element(const string_view local, const string_view value, const string_view uri = "") noexcept;
					/**
					 * @brief Метод записи дерева разметки
					 *
					 * @param node записываемый узел дерева разметки вместе с вложенными
					 * @return     результат выполнения операции
					 *
					 */
					bool element(const node_t & node) noexcept;
				public:
					/**
					 * @brief Метод получения кода ошибки записи
					 *
					 * @return код ошибки последней операции записи
					 *
					 */
					error_t error() const noexcept;
					/**
					 * @brief Метод проверки завершённости собранного текста
					 *
					 * @details Текст завершён, когда записан корневой узел и все открытые
					 * узлы закрыты. Незавершённый текст выдавать наружу нельзя
					 *
					 * @return результат проверки
					 *
					 */
					bool complete() const noexcept;
				public:
					/**
					 * @brief Метод получения собранного текста разметки
					 *
					 * @return собранный текст разметки в кодировке UTF-8
					 *
					 */
					const string & text() const noexcept;
					/**
					 * @brief Метод очистки собранного текста разметки
					 *
					 */
					void clear() noexcept;
				public:
					/**
					 * @brief Конструктор
					 *
					 */
					Writer() noexcept;
					/**
					 * @brief Конструктор
					 *
					 * @param settings настройки записи текста разметки
					 *
					 */
					explicit Writer(const settings_t & settings) noexcept;
					/**
					 * @brief Деструктор
					 *
					 */
					~Writer() noexcept;
			} writer_t;
		};
	};
};

#endif // __AWH_CODEC_XML_WRITER__
