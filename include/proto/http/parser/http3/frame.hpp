/**
 * @file: frame.hpp
 * @date: 2026-07-27
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @brief Заголовочный файл слоя кадров HTTP/3 (RFC 9114 §7) — разбор и сборка кадров,
 *        закодированных целыми переменной длины QUIC, без состояния соединения
 *
 * @copyright: Copyright © 2026
 *
 */

#ifndef __AWH_HTTP_PARSER_HTTP3_FRAME__
#define __AWH_HTTP_PARSER_HTTP3_FRAME__

/**
 * Стандартные заголовочные файлы
 */
#include <string>
#include <vector>
#include <cstddef>
#include <cstdint>
#include <string_view>
#include <unordered_set>

/**
 * Подключаем заголовочные файлы проекта
 */
#include "h3.hpp"
#include "../../../quic/varint.hpp"
#include "../../../../sys/global.hpp"

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
		 * @brief Пространство имён внутренних слоёв протокола HTTP/3
		 *
		 */
		namespace h3 {
			/**
			 * @brief Пространство имён слоя кадров HTTP/3 (RFC 9114 §7): разбор и сборка кадров
			 *
			 * @details Разбор zero-copy: полезная нагрузка отдаётся как string_view с указателем
			 *          во входной буфер. Сборка дописывает байты в string (выходной буфер потока).
			 *          Слой не хранит состояния соединения - это чистые функции над байтами.
			 *
			 *          Устройство кадра предельно простое: тип и длина нагрузки, оба целыми
			 *          переменной длины QUIC (RFC 9000 §16). Ни флагов, ни идентификатора потока
			 *          в кадре нет: поток задаёт транспорт, а роль флагов END_STREAM и END_HEADERS
			 *          исполняют признак FIN потока QUIC и сама граница кадра.
			 *
			 * @note Верхней границы длины кадра протокол не задаёт - параметра, подобного
			 *       SETTINGS_MAX_FRAME_SIZE, в HTTP/3 нет. Поэтому нагрузка кадра DATA обязана
			 *       разбираться по частям по мере поступления, а не собираться в буфере целиком:
			 *       иначе отправитель одним кадром задавал бы потребление памяти получателем.
			 *       Функции этого слоя разбирают нагрузку целиком только для управляющих кадров,
			 *       длина которых ограничена лимитами парсера сессии
			 *
			 */
			namespace frame {
				/**
				 * @brief Порог перехода на множество при поиске повторов SETTINGS
				 *
				 * @details Обычный кадр несёт единицы параметров, и перебор набора
				 *          дешевле хеш-множества вместе с его аллокацией. Кадр
				 *          предельного размера вмещает тысячи различных
				 *          идентификаторов, и перебор становится квадратичным
				 *
				 */
				static constexpr size_t SETTINGS_LOOKUP_THRESHOLD = (16);
				/**
				 * @brief Структура параметра SETTINGS (идентификатор + значение)
				 *
				 * @note Оба поля - целые переменной длины, поэтому 64-битные: в HTTP/2
				 *       идентификатор занимал 16 бит, а значение 32
				 *
				 */
				typedef struct __AWH_SHARED_EXPORT__ Setting {
					// Идентификатор параметра
					uint64_t id;
					// Значение параметра
					uint64_t value;
					/**
					 * @brief Конструктор
					 *
					 */
					explicit Setting() noexcept;
				} setting_entry_t;
				/**
				 * @brief Структура разобранного заголовка кадра (RFC 9114 §7.1)
				 *
				 */
				typedef struct __AWH_SHARED_EXPORT__ Header {
					// Тип кадра
					uint64_t type;
					// Длина полезной нагрузки в октетах
					uint64_t length;
					/**
					 * @brief Конструктор
					 *
					 */
					explicit Header() noexcept;
				} header_t;
				/**
				 * @brief Структура полезной нагрузки PUSH_PROMISE (RFC 9114 §7.2.5)
				 *
				 */
				typedef struct __AWH_SHARED_EXPORT__ Push_Promise {
					// Идентификатор обещанного push
					uint64_t pushId;
					// Секция полей запроса, закодированная QPACK (zero-copy во входной буфер)
					string_view block;
					/**
					 * @brief Конструктор
					 *
					 */
					explicit Push_Promise() noexcept;
				} push_promise_t;
				/**
				 * @brief Структура полезной нагрузки PRIORITY_UPDATE (RFC 9218 §7.2)
				 *
				 */
				typedef struct __AWH_SHARED_EXPORT__ Priority_Update {
					// Признак того, что приоритет назначается потоку push, а не потоку запроса
					bool push;
					// Идентификатор потока запроса либо идентификатор push
					uint64_t id;
					// Значение поля приоритета в синтаксисе структурированных полей (zero-copy)
					string_view value;
					/**
					 * @brief Конструктор
					 *
					 */
					explicit Priority_Update() noexcept;
				} priority_update_t;

				/**
				 * @brief Пространство имён функций разбора кадров HTTP/3 (RFC 9114 §7)
				 *
				 * @details Функции разбора нагрузки принимают её целиком: длина управляющих
				 *          кадров ограничена лимитами парсера сессии, поэтому их накопление
				 *          в буфере безопасно. Нагрузка кадра DATA через этот слой не проходит -
				 *          она отдаётся потребителю по частям парсером сессии
				 *
				 */
				namespace parser {
					/**
					 * @brief Функция разбора заголовка кадра
					 *
					 * @details Заголовок кадра занимает от двух до шестнадцати октетов: тип и длина
					 *          кодируются целыми переменной длины независимо друг от друга
					 *
					 * @param data   входной буфер
					 * @param size   доступно байт
					 * @param output разобранный заголовок кадра
					 * @return       количество прочитанных октетов либо 0, если данных недостаточно
					 *
					 */
					__AWH_SHARED_EXPORT__ size_t header(const uint8_t * data, const size_t size, header_t & output) noexcept;
					/**
					 * @brief Функция разбора нагрузки кадра из единственного целого переменной длины
					 *
					 * @details Общая форма нагрузки кадров CANCEL_PUSH, MAX_PUSH_ID и GOAWAY.
					 *          Лишние октеты после числа - ошибка H3_FRAME_ERROR (RFC 9114 §7.1)
					 *
					 * @param payload полезная нагрузка кадра
					 * @param size    размер полезной нагрузки
					 * @param value   разобранное число
					 * @param error   код ошибки протокола
					 * @return        результат разбора (OK/ERROR)
					 *
					 */
					__AWH_SHARED_EXPORT__ status_t identifier(const uint8_t * payload, const size_t size, uint64_t & value, error_t & error) noexcept;
					/**
					 * @brief Функция разбора нагрузки кадра SETTINGS (RFC 9114 §7.2.4)
					 *
					 * @details Повторно встреченный идентификатор - ошибка H3_SETTINGS_ERROR
					 *          (RFC 9114 §7.2.4.1), а обрыв нагрузки посреди пары - H3_FRAME_ERROR
					 *          (RFC 9114 §7.1): требования к нагрузке любого кадра и требования
					 *          именно к SETTINGS нарушаются по-разному и разводятся по кодам.
					 *          Зарезервированные идентификаторы отдаются наружу как есть: решение
					 *          об их пропуске принимает парсер сессии
					 *
					 * @param payload полезная нагрузка кадра
					 * @param size    размер полезной нагрузки
					 * @param output  разобранный набор параметров
					 * @param error   код ошибки протокола
					 * @return        результат разбора (OK/ERROR)
					 *
					 */
					__AWH_SHARED_EXPORT__ status_t settings(const uint8_t * payload, const size_t size, vector <setting_entry_t> & output, error_t & error) noexcept;
					/**
					 * @brief Функция разбора нагрузки кадра PUSH_PROMISE (RFC 9114 §7.2.5)
					 *
					 * @param payload полезная нагрузка кадра
					 * @param size    размер полезной нагрузки
					 * @param output  разобранная полезная нагрузка
					 * @param error   код ошибки протокола
					 * @return        результат разбора (OK/ERROR)
					 *
					 */
					__AWH_SHARED_EXPORT__ status_t pushPromise(const uint8_t * payload, const size_t size, push_promise_t & output, error_t & error) noexcept;
					/**
					 * @brief Функция разбора нагрузки кадра PRIORITY_UPDATE (RFC 9218 §7.2)
					 *
					 * @param type    тип кадра, различающий поток запроса и поток push
					 * @param payload полезная нагрузка кадра
					 * @param size    размер полезной нагрузки
					 * @param output  разобранная полезная нагрузка
					 * @param error   код ошибки протокола
					 * @return        результат разбора (OK/ERROR)
					 *
					 */
					__AWH_SHARED_EXPORT__ status_t priorityUpdate(const uint64_t type, const uint8_t * payload, const size_t size, priority_update_t & output, error_t & error) noexcept;
				};

				/**
				 * @brief Пространство имён функций сборки кадров HTTP/3 (RFC 9114 §7)
				 *
				 */
				namespace serialize {
					/**
					 * @brief Функция записи заголовка кадра
					 *
					 * @param output выходной буфер
					 * @param type   тип кадра
					 * @param length длина полезной нагрузки
					 *
					 */
					__AWH_SHARED_EXPORT__ void header(string & output, const uint64_t type, const uint64_t length) noexcept;
					/**
					 * @brief Функция записи типа однонаправленного потока (RFC 9114 §6.2)
					 *
					 * @details Тип потока отправляется единственным целым переменной длины
					 *          в самое начало потока, до любых кадров
					 *
					 * @param output выходной буфер
					 * @param type   тип однонаправленного потока
					 *
					 */
					__AWH_SHARED_EXPORT__ void unistream(string & output, const uint64_t type) noexcept;
					/**
					 * @brief Функция записи кадра DATA (RFC 9114 §7.2.1)
					 *
					 * @param output выходной буфер
					 * @param data   данные тела
					 *
					 */
					__AWH_SHARED_EXPORT__ void data(string & output, string_view data) noexcept;
					/**
					 * @brief Функция записи кадра HEADERS (RFC 9114 §7.2.2)
					 *
					 * @note Нарезки на несколько кадров, подобной CONTINUATION в HTTP/2, здесь нет:
					 *       длина кадра не ограничена, поэтому секция полей передаётся целиком
					 *
					 * @param output выходной буфер
					 * @param block  секция полей, закодированная QPACK
					 *
					 */
					__AWH_SHARED_EXPORT__ void headers(string & output, string_view block) noexcept;
					/**
					 * @brief Функция записи кадра GOAWAY (RFC 9114 §7.2.6)
					 *
					 * @param output выходной буфер
					 * @param id     идентификатор потока запроса (от сервера) либо push (от клиента)
					 *
					 */
					__AWH_SHARED_EXPORT__ void goaway(string & output, const uint64_t id) noexcept;
					/**
					 * @brief Функция записи кадра CANCEL_PUSH (RFC 9114 §7.2.3)
					 *
					 * @param output выходной буфер
					 * @param pushId идентификатор отменяемого push
					 *
					 */
					__AWH_SHARED_EXPORT__ void cancelPush(string & output, const uint64_t pushId) noexcept;
					/**
					 * @brief Функция записи кадра MAX_PUSH_ID (RFC 9114 §7.2.7)
					 *
					 * @param output выходной буфер
					 * @param pushId наибольший допустимый идентификатор push
					 *
					 */
					__AWH_SHARED_EXPORT__ void maxPushId(string & output, const uint64_t pushId) noexcept;
					/**
					 * @brief Функция записи кадра SETTINGS (RFC 9114 §7.2.4)
					 *
					 * @param output выходной буфер
					 * @param items  набор параметров
					 * @param count  количество параметров
					 *
					 */
					__AWH_SHARED_EXPORT__ void settings(string & output, const setting_entry_t * items, const size_t count) noexcept;
					/**
					 * @brief Функция записи кадра PUSH_PROMISE (RFC 9114 §7.2.5)
					 *
					 * @param output выходной буфер
					 * @param pushId идентификатор обещанного push
					 * @param block  секция полей запроса, закодированная QPACK
					 *
					 */
					__AWH_SHARED_EXPORT__ void pushPromise(string & output, const uint64_t pushId, string_view block) noexcept;
					/**
					 * @brief Функция записи кадра PRIORITY_UPDATE (RFC 9218 §7.2)
					 *
					 * @param output выходной буфер
					 * @param push   признак назначения приоритета потоку push, а не потоку запроса
					 * @param id     идентификатор потока запроса либо идентификатор push
					 * @param value  значение поля приоритета в синтаксисе структурированных полей
					 *
					 */
					__AWH_SHARED_EXPORT__ void priorityUpdate(string & output, const bool push, const uint64_t id, string_view value) noexcept;
					/**
					 * @brief Функция записи зарезервированного кадра (RFC 9114 §7.2.8)
					 *
					 * @details Кадр с типом вида (0x1F * N + 0x21) и произвольной нагрузкой обязан
					 *          игнорироваться получателем. Отправка такого кадра - единственный
					 *          способ убедиться, что пир не считает набор типов кадров закрытым
					 *
					 * @param output выходной буфер
					 * @param seed   порядковый номер N в последовательности зарезервированных типов
					 * @param data   произвольная нагрузка кадра
					 *
					 */
					__AWH_SHARED_EXPORT__ void reserved(string & output, const uint64_t seed, string_view data = {}) noexcept;
				};
			};
		}
	};
};

#endif // __AWH_HTTP_PARSER_HTTP3_FRAME__
