/**
 * @file common.hpp
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
 * \~russian
 * @brief Заголовочный файл общих определений контейнера SysLog — коды ошибок разбора, виды событий чтения,
 *        описания записи, поля заголовка, источники и степени важности сообщения, строгость сличения,
 *        правила обращения с отсутствующим значением, пределы разбора и положение в исходном тексте
 *
 * \~english
 * @brief Header file of the common definitions of the SysLog container — the error codes of the parsing, the kinds of the events of the reading,
 *        the descriptions of a record, the fields of the header, the sources and the degrees of the importance of a message, the strictness of the matching,
 *        the rules of the treatment of an absent value, the limits of the parsing and the position in the source text
 *
 * \~
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Экранируем повторную инициализацию модуля
 */
#pragma once

/**
 * Стандартные заголовочные файлы
 */
#include <string>
#include <cstdint>
#include <string_view>

/**
 * Подключаем заголовочные файлы проекта
 */
#include "../../sys/macro/global.hpp"

/**
 * Подавляем системные макросы, занявшие имена членов перечислений ниже:
 * ERROR, ALERT и DELETE у MS Windows, NOTICE и WARNING у прочих систем.
 * Имена снимаются лишь на время объявлений - возврат в конце файла
 */
#include "../../sys/macro/suppress.hpp"

/**
 * \~russian
 * @brief Основное пространство имён
 *
 *
 * \~english
 * @brief Main namespace
 *
 * \~
 */
namespace awh {
	/**
	 * Используем стандартное пространство имён
	 */
	using namespace std;

	/**
	 * \~russian
	 * @brief Пространство имён контейнеров данных
	 *
	 *
	 * \~english
	 * @brief Data containers namespace
	 *
	 * \~
	 */
	namespace codec {
		/**
		 * \~russian
		 * @brief Пространство имён контейнера SysLog
		 *
		 * @details Разбор и запись сообщений системного журнала двух описаний: устаревшего
		 * RFC 3164, где за приоритетом следуют дата, узел и метка приложения, и нынешнего
		 * RFC 5424, где к ним добавлены номер описания, опознаватель сообщения и
		 * структурированные данные
		 *
		 * @par Намеренные решения
		 *
		 * Перечисленное ниже не является пробелом реализации: это очерченные границы
		 * задачи, и каждое из решений закреплено проверочным испытанием
		 *
		 * @li **RFC 3164 и RFC 5424 суть РАЗНЫЕ разборы, а не разновидности одного.**
		 * Совпадает у них лишь приставка приоритета; далее расходится всё - вид даты,
		 * состав полей, разделители, обращение с отсутствующим значением. Свести их к
		 * одному ходу с ветвлениями значило бы разбирать неверно оба, оттого разборы
		 * разделены, а выбор между ними отдан настройке. Умолчание - самоопределение по
		 * виду записи, ибо живой сборщик журналов принимает поток от многих устройств
		 * разом и описания в нём перемешаны
		 *
		 * @li **Приоритет хранится ЧИСЛОМ, а источник и важность выдаются вычислением.**
		 * Описание задаёт приоритет одним числом, из какого источник есть частное от
		 * деления на восемь, а важность - остаток. Хранить все три значило бы держать
		 * одно сведение в трёх местах и открывать путь к их расхождению при правке
		 *
		 * @li **Отсутствующее значение и пустая последовательность знаков РАЗЛИЧАЮТСЯ.**
		 * RFC 5424 назначает знак «-» отсутствию значения (NILVALUE), и различие это
		 * несущее: «узел неизвестен» и «узел есть пустая строка» суть разные утверждения.
		 * Оттого поле со знаком «-» в дерево НЕ кладётся вовсе, а не кладётся строкою из
		 * одного знака. Обратное толкование берётся настройкой
		 *
		 * @li **Даты разбираются ходом `awh::chrono_t`, а не своим разбором.** Оба
		 * описания дат уже известны каркасу: `standard_t::RFC3164` и `standard_t::RFC3339`.
		 * Заводить рядом второй разбор значило бы держать один договор в двух местах, а
		 * расхождение между ними всплыло бы на високосном годе либо на переходе зоны
		 *
		 * @li **Метка порядка байтов перед текстом сообщения снимается, а не хранится.**
		 * RFC 5424 позволяет предварять текст сообщения меткой UTF-8, и метка эта есть
		 * признак кодировки, а не часть сообщения. Оставить её в тексте значило бы
		 * отдать потребителю три лишних октета, каких он не писал; снятие берётся
		 * настройкой, ибо обратная сборка обязана уметь вернуть её на место
		 *
		 * @li **Структурированные данные суть дерево, а не последовательность знаков.**
		 * Отмена знаков внутри них своя: отменяются «\\"», «\\]» и «\\\\», и только они.
		 * Разбирать их позже, вторым проходом, значило бы разбирать дважды и хранить
		 * между проходами текст, уже разобранный
		 *
		 * \~english
		 * @brief Namespace of the SysLog container
		 * @details The parsing and the writing of the messages of the system log of two descriptions: the outdated
		 * RFC 3164 and the present RFC 5424
		 *
		 * \~
		 */
		namespace syslog {
			/**
			 * \~russian
			 * @brief Наибольший поддерживаемый номер описания записи
			 *
			 * @details RFC 5424 объявляет описание номером первым и оговаривает, что номер
			 * высший вправе нести иной состав полей. Разбирать его значило бы ГАДАТЬ,
			 * оттого номер выше отвергается отказом
			 *
			 * \~english
			 * @brief Largest supported number of the description of a record
			 *
			 * \~
			 */
			constexpr uint32_t MAX_VERSION = 1;

			/**
			 * \~russian
			 * @brief Наибольшее допустимое значение приоритета
			 *
			 * @details Приоритет есть источник, восьмикратно взятый, и важность: источников
			 * двадцать четыре, важностей восемь, отчего предел равен 191
			 *
			 * \~english
			 * @brief Largest admissible value of the priority
			 *
			 * \~
			 */
			constexpr uint32_t MAX_PRIORITY = 191;

			/**
			 * \~russian
			 * @brief Наибольшая допустимая длина одной записи в байтах
			 *
			 * @details RFC 5424 требует от принимающего принимать не менее 480 октетов и
			 * советует принимать 2048, верхнего же предела не назначает вовсе. Предел
			 * здесь взят с большим запасом: живые устройства шлют записи в сотни килобайт,
			 * и отвергать их значило бы терять события
			 *
			 * \~english
			 * @brief Largest admissible length of one record in bytes
			 *
			 * \~
			 */
			constexpr uint32_t MAX_RECORD = 0x100000;

			/**
			 * \~russian
			 * @brief Наибольшая допустимая длина имени узла в байтах
			 *
			 * @details Предел назначен RFC 5424, раздел 6.2.4
			 *
			 * \~english
			 * @brief Largest admissible length of the name of the host in bytes
			 *
			 * \~
			 */
			constexpr uint32_t MAX_HOSTNAME = 255;

			/**
			 * \~russian
			 * @brief Наибольшая допустимая длина названия приложения в байтах
			 *
			 * @details Предел назначен RFC 5424, раздел 6.2.5
			 *
			 * \~english
			 * @brief Largest admissible length of the name of the application in bytes
			 *
			 * \~
			 */
			constexpr uint32_t MAX_APPLICATION = 48;

			/**
			 * \~russian
			 * @brief Наибольшая допустимая длина опознавателя работы в байтах
			 *
			 * @details Предел назначен RFC 5424, раздел 6.2.6
			 *
			 * \~english
			 * @brief Largest admissible length of the identifier of the process in bytes
			 *
			 * \~
			 */
			constexpr uint32_t MAX_PROCESS = 128;

			/**
			 * \~russian
			 * @brief Наибольшая допустимая длина опознавателя сообщения в байтах
			 *
			 * @details Предел назначен RFC 5424, раздел 6.2.7
			 *
			 * \~english
			 * @brief Largest admissible length of the identifier of a message in bytes
			 *
			 * \~
			 */
			constexpr uint32_t MAX_MESSAGE_ID = 32;

			/**
			 * \~russian
			 * @brief Наибольшая допустимая длина имени внутри структурированных данных
			 *
			 * @details Предел назначен RFC 5424, раздел 6.3.2, и общий у опознавателя
			 * блока и у имени поля
			 *
			 * \~english
			 * @brief Largest admissible length of a name inside the structured data
			 *
			 * \~
			 */
			constexpr uint32_t MAX_NAME = 32;

			/**
			 * \~russian
			 * @brief Наибольшее допустимое количество блоков структурированных данных
			 *
			 * \~english
			 * @brief Largest admissible number of the blocks of the structured data
			 *
			 * \~
			 */
			constexpr uint32_t MAX_STRUCTURES = 0x1000;

			/**
			 * \~russian
			 * @brief Наибольшее допустимое количество полей одного блока данных
			 *
			 * \~english
			 * @brief Largest admissible number of the fields of one block of the data
			 *
			 * \~
			 */
			constexpr uint32_t MAX_PARAMS = 0x1000;

			/**
			 * \~russian
			 * @brief Метка порядка байтов, тексту сообщения предшествующая
			 *
			 * @details RFC 5424, раздел 6.4, позволяет предварять текст сообщения меткой
			 * UTF-8: она объявляет кодировку, но частью сообщения не является
			 *
			 * \~english
			 * @brief Byte order mark preceding the text of a message
			 *
			 * \~
			 */
			constexpr string_view BOM = "\xEF\xBB\xBF";

			/**
			 * \~russian
			 * @brief Знак, отсутствию значения назначенный
			 *
			 * @details Назначен RFC 5424, раздел 6: поле, значения не имеющее, несёт один
			 * этот знак
			 *
			 * \~english
			 * @brief Character assigned to the absence of a value
			 *
			 * \~
			 */
			constexpr string_view NIL = "-";

			/**
			 * \~russian
			 * @brief Коды отказов разбора и записи
			 *
			 * @details Перечень содержит лишь те коды, какие разбор ВЫСТАВЛЯЕТ хотя бы на
			 * одном пути: недостижимый код отказа хуже отсутствующего, ибо потребитель
			 * пишет по нему ветвь, какая не исполнится никогда
			 *
			 * \~english
			 * @brief The codes of the refusals of the parsing and the writing
			 *
			 * \~
			 */
			enum class error_t : uint8_t {
				NONE                  = 0x00, // Ошибок не обнаружено
				MISSING_PRIORITY      = 0x01, // Запись не открывается приставкой приоритета
				INVALID_PRIORITY      = 0x02, // Приоритет построен ошибочно либо выходит за предел
				INVALID_VERSION       = 0x03, // Номер описания записи построен ошибочно
				UNSUPPORTED_VERSION   = 0x04, // Номер описания записи не поддерживается
				INCOMPLETE_HEADER     = 0x05, // Полей заголовка меньше положенного
				EMPTY_HEADER_FIELD    = 0x06, // Обязательное поле заголовка пусто
				INVALID_TIMESTAMP     = 0x07, // Дата сообщения ни одному из описаний не отвечает
				INVALID_HOSTNAME      = 0x08, // Имя узла содержит знак, описанием не дозволенный
				INVALID_PROCESS       = 0x09, // Опознаватель работы построен ошибочно
				UNCLOSED_STRUCTURE    = 0x0A, // Скобка структурированных данных не закрыта
				INVALID_STRUCTURE_ID  = 0x0B, // Опознаватель структурированных данных построен ошибочно
				INVALID_PARAM_NAME    = 0x0C, // Имя поля структурированных данных построено ошибочно
				UNQUOTED_PARAM_VALUE  = 0x0D, // Значение поля структурированных данных не взято в кавычки
				UNCLOSED_PARAM_VALUE  = 0x0E, // Кавычка значения структурированных данных не закрыта
				DUPLICATE_STRUCTURE   = 0x0F, // Опознаватель структурированных данных объявлен дважды
				NAME_TOO_LONG         = 0x10, // Длина имени превышает допустимую
				FIELD_TOO_LONG        = 0x11, // Длина поля заголовка превышает допустимую
				RECORD_TOO_LONG       = 0x12, // Длина записи превышает допустимую
				OVERFLOW_LIMIT        = 0x13, // Превышен предел, ЗАДАННЫЙ НАСТРОЙКАМИ разбора
				UNKNOWN_STANDARD      = 0x14, // Описание записи определить не удалось
				UNKNOWN_FIELD         = 0x15, // Поле с таким именем записью не объявлено
				UNREPRESENTABLE_VALUE = 0x16, // Значение такого вида запись SysLog выразить не может
				NESTED_VALUE          = 0x17, // Вложенное значение записи SysLog неведомо
				FILE_NOT_OPENED       = 0x18, // Файл записей открыть не удалось
				FILE_NOT_READ         = 0x19, // Файл записей прочитать не удалось
				INVALID_HEADER_FIELD  = 0x1A, // Поле заголовка содержит знак, описанием не дозволенный
				MULTIPLE_RECORDS      = 0x1B, // Файл несёт более одной записи, а документ держит одну
				INVALID_MESSAGE_UTF8  = 0x1C  // Текст сообщения меткою объявлен UTF-8, а годным UTF-8 не является
			};

			/**
			 * \~russian
			 * @brief Виды событий чтения сообщения системного журнала
			 *
			 * @details Чтение выдаёт события по мере разбора текста, не удерживая его целиком
			 *
			 * \~english
			 * @brief Kinds of the events of the reading of a system log message
			 *
			 * \~
			 */
			enum class event_t : uint8_t {
				NONE      = 0x00, // Событие не определено
				HEADER    = 0x01, // Поле заголовка записи
				STRUCTURE = 0x02, // Опознаватель начатого блока структурированных данных
				PARAM     = 0x03, // Поле структурированных данных «имя=значение»
				MESSAGE   = 0x04, // Свободный текст сообщения
				RECORD    = 0x05, // Запись разобрана до конца, следующая начинается заново
				FINISH    = 0x06  // Текст разобран до конца, событие видно после цикла разбора
			};

			/**
			 * \~russian
			 * @brief Описания записи системного журнала
			 *
			 * @details Самоопределение сличает вид поля, за приоритетом стоящего: число с
			 * пробелом означает RFC 5424, всё прочее - RFC 3164. Признак этот назначен
			 * самим RFC 5424, а не выведен из наблюдений: номер описания там обязателен и
			 * стоит первым после приоритета
			 *
			 * \~english
			 * @brief Descriptions of a record of the system log
			 *
			 * \~
			 */
			enum class standard_t : uint8_t {
				AUTO    = 0x00, // Описание определяется по виду записи
				RFC3164 = 0x01, // Устаревшее описание BSD
				RFC5424 = 0x02  // Нынешнее описание с опознавателем сообщения
			};

			/**
			 * \~russian
			 * @brief Поля заголовка записи системного журнала
			 *
			 * @details Порядок членов отвечает порядку полей в записи и служит их
			 * указателем: поле разбирается по счёту, а не по имени, ибо имён у полей
			 * заголовка запись не несёт вовсе. Поля VERSION, MESSAGE_ID и STRUCTURE
			 * принадлежат лишь RFC 5424, поле TAG - лишь RFC 3164
			 *
			 * \~english
			 * @brief Fields of the header of a system log record
			 *
			 * \~
			 */
			enum class field_t : uint8_t {
				NONE       = 0x00, // Поле не определено
				PRIORITY   = 0x01, // Приоритет: источник, восьмикратно взятый, и важность
				VERSION    = 0x02, // Номер описания записи, лишь RFC 5424
				TIMESTAMP  = 0x03, // Дата и время составления сообщения
				HOSTNAME   = 0x04, // Имя узла, сообщение составившего
				APPLICATION = 0x05, // Название приложения либо метка RFC 3164
				PROCESS    = 0x06, // Опознаватель работы, сообщение составившей
				MESSAGE_ID = 0x07  // Опознаватель разновидности сообщения, лишь RFC 5424
			};

			/**
			 * \~russian
			 * @brief Источники сообщения системного журнала
			 *
			 * @details Значения назначены RFC 5424, таблица 1. Источник есть частное от
			 * деления приоритета на восемь
			 *
			 * \~english
			 * @brief Sources of a system log message
			 *
			 * \~
			 */
			enum class facility_t : uint8_t {
				KERNEL   = 0x00, // Сообщения ядра системы
				USER     = 0x01, // Сообщения уровня пользователя
				MAIL     = 0x02, // Почтовая служба
				DAEMON   = 0x03, // Службы системы
				AUTH     = 0x04, // Опознание и безопасность
				SYSLOG   = 0x05, // Сообщения самой службы журнала
				PRINTER  = 0x06, // Служба печати
				NEWS     = 0x07, // Служба новостей
				UUCP     = 0x08, // Служба UUCP
				CLOCK    = 0x09, // Служба часов
				SECURITY = 0x0A, // Опознание и безопасность, второй набор
				FTP      = 0x0B, // Служба передачи файлов
				NTP      = 0x0C, // Служба сетевого времени
				AUDIT    = 0x0D, // Записи наблюдения за журналом
				ALERT    = 0x0E, // Тревоги наблюдения за журналом
				CLOCK2   = 0x0F, // Служба часов, второй набор
				LOCAL0   = 0x10, // Местное употребление, набор нулевой
				LOCAL1   = 0x11, // Местное употребление, набор первый
				LOCAL2   = 0x12, // Местное употребление, набор второй
				LOCAL3   = 0x13, // Местное употребление, набор третий
				LOCAL4   = 0x14, // Местное употребление, набор четвёртый
				LOCAL5   = 0x15, // Местное употребление, набор пятый
				LOCAL6   = 0x16, // Местное употребление, набор шестой
				LOCAL7   = 0x17  // Местное употребление, набор седьмой
			};

			/**
			 * \~russian
			 * @brief Степени важности сообщения системного журнала
			 *
			 * @details Значения назначены RFC 5424, таблица 2. Важность есть остаток от
			 * деления приоритета на восемь, и меньшее число означает БОЛЬШУЮ важность
			 *
			 * \~english
			 * @brief Degrees of the importance of a system log message
			 *
			 * \~
			 */
			enum class severity_t : uint8_t {
				EMERGENCY = 0x00, // Система непригодна к работе
				ALERT     = 0x01, // Вмешательство требуется немедленно
				CRITICAL  = 0x02, // Состояние тяжёлое
				ERROR     = 0x03, // Условие отказа
				WARNING   = 0x04, // Условие предостережения
				NOTICE    = 0x05, // Состояние обычное, но внимания достойное
				INFO      = 0x06, // Сообщение осведомительное
				DEBUG     = 0x07  // Сообщение отладочное
			};

			/**
			 * \~russian
			 * @brief Строгость сличения разбираемой записи с описанием
			 *
			 * @details Сличение стоит времени и потому берётся настройкой, а не ведётся
			 * всегда: сборщик журналов принимает миллионы записей от устройств, описанию
			 * следующих нестрого, и отвергать их значило бы терять события
			 *
			 * \~english
			 * @brief Strictness of the matching of a parsed record against the description
			 *
			 * \~
			 */
			enum class mode_t : uint8_t {
				NONE   = 0x00, // Сличения не ведётся: поля кладутся знаками, как в записи и стоят
				WEAK   = 0x01, // Сличается вид полей, но отказом отвечает лишь неразбираемое
				STRONG = 0x02  // Сличается всё: вид полей, длины, набор знаков имён
			};

			/**
			 * \~russian
			 * @brief Правила обращения с отсутствующим значением
			 *
			 * @details RFC 5424 назначает знак «-» отсутствию значения. Умолчание -
			 * опустить поле вовсе, ибо «неизвестно» и «пусто» суть разные утверждения
			 *
			 * \~english
			 * @brief Rules of the treatment of an absent value
			 *
			 * \~
			 */
			enum class nil_t : uint8_t {
				OMIT   = 0x00, // Поле в дерево не кладётся вовсе
				EMPTY  = 0x01, // Поле кладётся пустой последовательностью знаков
				LITERAL = 0x02 // Поле кладётся знаками «-», как в записи и стоит
			};

			/**
			 * \~russian
			 * @brief Правила обращения с вложенностью при записи дерева в запись SysLog
			 *
			 * @details Дерева произвольной глубины запись системного журнала не несёт:
			 * поля заголовка суть знаки, а структурированные данные глубже одного слоя
			 * не идут. Выбор исхода принадлежит не кодеку, а тому, кто пишет:
			 * молчаливое обращение в знаки оставляло бы потребителя с записью, которая
			 * разбирается, но означает иное
			 *
			 * \~english
			 * @brief Rules of the treatment of a nesting at the writing of a tree into a SysLog record
			 *
			 * \~
			 */
			enum class nested_t : uint8_t {
				STRICT = 0x00, // Отвечать отказом с кодом NESTED_VALUE
				TEXT   = 0x01, // Обращать в последовательность знаков
				SKIP   = 0x02  // Пропускать значение вовсе
			};

			/**
			 * \~russian
			 * @brief Метод получения текста сообщения об ошибке разбора
			 *
			 * @details Текст выдаётся на английском языке и предназначен журналу, а не
			 * потребителю: разбирать отказы надлежит по коду, а не по тексту
			 *
			 * @param error код ошибки разбора
			 * @return      текст сообщения об ошибке разбора
			 *
			 * \~english
			 * @brief Method of getting the text of the message about an error of the parsing
			 * @param error error code of the parsing
			 * @return      text of the message about an error of the parsing
			 *
			 * \~
			 */
			__AWH_SHARED_EXPORT__ const char * message(const error_t error) noexcept;

			/**
			 * \~russian
			 * @brief Метод получения имени источника сообщения
			 *
			 * @details Имена взяты из описания RFC 5424 и общеприняты службами журналов;
			 * при значении, за предел таблицы выходящем, выдаётся пустая строка
			 *
			 * @param facility источник сообщения
			 * @return         имя источника сообщения
			 *
			 * \~english
			 * @brief Method of getting the name of the source of a message
			 * @param facility source of a message
			 * @return         name of the source of a message
			 *
			 * \~
			 */
			__AWH_SHARED_EXPORT__ const char * name(const facility_t facility) noexcept;

			/**
			 * \~russian
			 * @brief Метод получения имени степени важности сообщения
			 *
			 * @param severity степень важности сообщения
			 * @return         имя степени важности сообщения
			 *
			 * \~english
			 * @brief Method of getting the name of the degree of the importance of a message
			 * @param severity degree of the importance of a message
			 * @return         name of the degree of the importance of a message
			 *
			 * \~
			 */
			__AWH_SHARED_EXPORT__ const char * name(const severity_t severity) noexcept;

			/**
			 * \~russian
			 * @brief Положение в разбираемом тексте
			 *
			 * @details Положение несёт и смещение в октетах от начала текста, и номер
			 * строки со столбцом
			 *
			 * \~english
			 * @brief Position in the parsed text
			 *
			 * \~
			 */
			typedef struct __AWH_SHARED_EXPORT__ Position {
				// Смещение в байтах от начала текста
				uint64_t offset;
				// Номер строки, считая от единицы
				uint64_t line;
				// Номер столбца в байтах, считая от единицы
				uint64_t column;
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
				Position() noexcept : offset(0), line(1), column(1) {}
			} pos_t;
		}
	}
}

/**
 * Возвращаем имена, системными макросами занятые
 */
#include "../../sys/macro/restore.hpp"
