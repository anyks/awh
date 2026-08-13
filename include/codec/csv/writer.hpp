/**
 * @file: writer.hpp
 * @date: 2026-08-12
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @brief Заголовочный файл записи текста CSV — класс Writer, собирающий текст полем за
 *        полем и отдающий собранное как целиком, так и по мере накопления
 *
 * @copyright: Copyright © 2026
 *
 */

/**
 * Экранируем повторную инициализацию модуля
 */
#ifndef __AWH_CODEC_CSV_WRITER__
#define __AWH_CODEC_CSV_WRITER__

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
#include "common.hpp"

/**
 * Снимаем на время объявлений макросы, чьи имена заняты
 * членами перечислений, применяемых ниже (возвращает их macro_pop.hpp в конце файла)
 */
#include "../../sys/macro_push.hpp"

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
		 * @brief Пространство имён контейнера CSV
		 *
		 */
		namespace csv {
			/**
			 * @brief Класс записи текста CSV
			 *
			 * @details Текст собирается полем за полем, а отдаётся либо целиком по
			 * окончании, либо кусками по мере накопления - смотря по тому, держит ли
			 * потребитель собранное в памяти или отправляет его дальше
			 *
			 * @par Порядок работы
			 *
			 * @code{.cpp}
			 * writer_t writer;
			 *
			 * writer.field("имя");
			 * writer.field("значение");
			 * writer.record();
			 *
			 * const string & text = writer.text();
			 * @endcode
			 *
			 * @par Потоковая запись
			 *
			 * @code{.cpp}
			 * while(!done){
			 *     writer.record(fields);
			 *
			 *     if(writer.size() >= 0x10000)
			 *         send(writer.take());
			 * }
			 *
			 * send(writer.take());
			 * @endcode
			 *
			 * @note Запись пределов не проверяет и ошибок не заводит: собираемый текст
			 * ошибочным не бывает, а поле, содержащее что угодно, всегда записывается
			 * так, чтобы разобраться обратно неизменным. Договор этот закреплён
			 * проверкой кругового прохода
			 *
			 */
			typedef class __AWH_SHARED_EXPORT__ Writer {
				public:
					/**
					 * @brief Настройки записи текста
					 *
					 */
					typedef struct __AWH_SHARED_EXPORT__ Settings {
						// Знак-разделитель полей
						char separator;
						// Знак кавычек, обрамляющих поле
						char quote;
						/**
						 * Знак начала строки примечания, признаваемый разбором
						 *
						 * @note Знак этот записью не ставится, а лишь учитывается: поле,
						 * начинающее запись этим знаком, берётся в кавычки, иначе разбор
						 * прочтёт всю запись строкой примечания и отбросит её. Умолчанием
						 * знака нет вовсе - примечания договором не описаны
						 */
						char comment;
						// Правило заключения поля в кавычки
						quoting_t quoting;
						// Способ записи кавычки внутри поля, заключённого в кавычки
						escape_t escape;
						// Знак конца строки
						newline_t newline;
						/**
						 * Признак записи метки порядка байтов в начале текста
						 *
						 * @note Метка эта содержимым текста не является, но без неё
						 * табличные редакторы MS Windows читают UTF-8 как местную
						 * однобайтовую кодировку и портят всё, кроме US-ASCII
						 */
						bool signature;
						/**
						 * @brief Конструктор
						 *
						 */
						Settings() noexcept;
					} settings_t;
				private:
					// Настройки записи текста
					settings_t _settings;
				private:
					// Собранный текст
					string _text;
				private:
					// Признак того, что запись уже содержит поля
					bool _started;
					// Признак того, что метка порядка байтов уже записана
					bool _marked;
				private:
					/**
					 * @brief Метод записи метки порядка байтов
					 *
					 * @details Метка записывается однажды и лишь перед первым полем: текст,
					 * отданный по частям, метки посреди себя иметь не должен
					 *
					 */
					void mark() noexcept;
					/**
					 * @brief Метод записи содержимого поля с обрамлением кавычками
					 *
					 * @param text содержимое записываемого поля
					 *
					 */
					void quoted(const string_view text) noexcept;
				public:
					/**
					 * @brief Метод записи очередного поля записи
					 *
					 * @param text содержимое записываемого поля
					 *
					 */
					void field(const string_view text) noexcept;
					/**
					 * @brief Метод записи числового поля записи
					 *
					 * @tparam T тип записываемого значения
					 * @param value записываемое значение
					 *
					 */
					template <typename T>
					void number(const T value) noexcept;
				public:
					/**
					 * @brief Метод завершения текущей записи
					 *
					 * @details Запись без полей даёт пустую строку, а разбор пустые строки
					 * пропускает: круговой проход такой записи не сохраняет, и это не
					 * упущение, а свойство самой записи CSV
					 *
					 */
					void record() noexcept;
					/**
					 * @brief Метод записи целой записи полем за полем
					 *
					 * @param fields поля записываемой записи
					 *
					 */
					void record(const vector <string> & fields) noexcept;
					/**
					 * @brief Метод записи целой записи полем за полем
					 *
					 * @param fields поля записываемой записи
					 *
					 */
					void record(const vector <string_view> & fields) noexcept;
				public:
					/**
					 * @brief Метод записи текста целиком
					 *
					 * @details Собранное прежде не отбрасывается: записи дописываются к
					 * нему, что позволяет предпослать таблице заголовок, собранный
					 * отдельно
					 *
					 * @param records записываемые записи
					 *
					 */
					void write(const vector <vector <string>> & records) noexcept;
				public:
					/**
					 * @brief Метод получения собранного текста
					 *
					 * @return собранный текст
					 *
					 */
					const string & text() const noexcept;
					/**
					 * @brief Метод получения размера собранного текста
					 *
					 * @return размер собранного текста в байтах
					 *
					 */
					size_t size() const noexcept;
				public:
					/**
					 * @brief Метод изъятия собранного текста
					 *
					 * @details Собранное изымается целиком, а сборщик остаётся готовым
					 * продолжать: тем и ведётся потоковая запись, когда держать текст в
					 * памяти целиком незачем
					 *
					 * @warning Изымать текст посреди записи нельзя: изъятое окончится
					 * полем без завершения записи, и склеенное обратно даст запись,
					 * разорванную надвое. Изъятие посреди записи потому отдаёт пустоту
					 *
					 * @return изъятый текст
					 *
					 */
					string take() noexcept;
					/**
					 * @brief Метод очистки собранного текста
					 *
					 */
					void clear() noexcept;
				public:
					/**
					 * @brief Метод получения настроек записи текста
					 *
					 * @return настройки записи текста
					 *
					 */
					const settings_t & settings() const noexcept;
					/**
					 * @brief Метод установки настроек записи текста
					 *
					 * @param settings настройки записи текста
					 *
					 */
					void settings(const settings_t & settings) noexcept;
				public:
					/**
					 * @brief Конструктор
					 *
					 */
					Writer() noexcept;
					/**
					 * @brief Конструктор
					 *
					 * @param settings настройки записи текста
					 *
					 */
					Writer(const settings_t & settings) noexcept;
					/**
					 * @brief Деструктор
					 *
					 */
					~Writer() noexcept {}
			} writer_t;
		}
	}
}

/**
 * Возвращаем снятые ранее макросы
 */
#include "../../sys/macro_pop.hpp"

#endif // __AWH_CODEC_CSV_WRITER__
