/**
 * @file: common.hpp
 * @date: 2026-07-27
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @brief Общее окружение эталонных стендов сравнения протокола HTTP/3 — эталонная
 *        нагрузка, канонические потоки октетов, драйвер прогона и вывод результатов
 *
 * @details Нагрузка и границы замера обязаны совпадать у всех сравниваемых
 *          реализаций, а повторение этой логики в каждом стенде рано или поздно
 *          даёт расхождение. Канонические потоки октетов порождает эталонная
 *          реализация nghttp3: иначе показатель декодирования отражал бы не
 *          скорость декодера, а удачность кодера рядом с ним
 *
 * @copyright: Copyright © 2026
 *
 */

#ifndef __AWH_BENCHMARK_RIVAL_HTTP3__
#define __AWH_BENCHMARK_RIVAL_HTTP3__

/**
 * Стандартные заголовочные файлы
 */
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cstddef>
#include <cstdint>
#include <chrono>
#include <string>
#include <vector>

/**
 * Подключаем заголовочный файл эталонной реализации
 *
 * @details Эталонная реализация нужна каждому стенду независимо от того, какую
 *          реализацию он измеряет: ею порождаются канонические потоки октетов
 */
#include <nghttp3/nghttp3.h>

/**
 * Если стенд собран с аллокатором TcMalloc
 */
#if __AWH_USE_TCMALLOC__
	#include <gperftools/malloc_hook.h>
#endif

/**
 * @brief Пространство имён эталонных стендов сравнения
 *
 */
namespace rival {
	/**
	 * @brief Количество секций полей сценариев кодирования и декодирования
	 *
	 */
	static constexpr size_t SECTION_ROUNDS = 200000;
	/**
	 * @brief Количество секций полей сценария измерения сжатия
	 *
	 * @details Степень сжатия зависит от прогретости динамической таблицы, поэтому
	 *          измерять её на одной секции бессмысленно: первая секция сжимается
	 *          только статической таблицей
	 *
	 */
	static constexpr size_t RATIO_ROUNDS = 1000;
	/**
	 * @brief Количество запросов сценария разбора потока запросов
	 *
	 */
	static constexpr size_t REQUEST_ROUNDS = 100000;
	/**
	 * @brief Количество обменов сценариев полного цикла запрос-ответ
	 *
	 */
	static constexpr size_t ROUNDTRIP_ROUNDS = 50000;
	/**
	 * @brief Объём тела сценария приёма данных в октетах
	 *
	 */
	static constexpr size_t BODY_SIZE = (64 * 1024 * 1024);
	/**
	 * @brief Размер кадра данных сценария приёма данных
	 *
	 * @details Размер кадра HTTP/3 протоколом не ограничен вовсе. Значение взято
	 *          таким же, как у HTTP/2 по умолчанию, - иначе показатели двух
	 *          протоколов сравнивались бы на разной нагрузке
	 *
	 */
	static constexpr size_t FRAME_SIZE = 16384;
	/**
	 * @brief Количество одновременных потоков сценария мультиплексирования
	 *
	 */
	static constexpr size_t STREAM_COUNT = 64;
	/**
	 * @brief Размер порции подачи потока в октетах
	 *
	 */
	static constexpr size_t CHUNK_SIZE = (64 * 1024);
	/**
	 * @brief Количество различных наборов полей эталонной нагрузки
	 *
	 */
	static constexpr size_t SET_COUNT = 64;
	/**
	 * @brief Ёмкость динамической таблицы QPACK в октетах
	 *
	 * @details Совпадает с размером таблицы HPACK по умолчанию: сравнение сжатия
	 *          двух протоколов осмысленно только при равных таблицах. Кодер и декодер
	 *          каждого стенда обязаны держать одинаковую таблицу, иначе канонический
	 *          поток октетов не будет декодирован
	 *
	 */
	static constexpr uint64_t TABLE_CAPACITY = 4096;
	/**
	 * @brief Число потоков, которым разрешено ожидать пополнения таблицы
	 *
	 */
	static constexpr uint64_t BLOCKED_STREAMS = 16;
	/**
	 * @brief Идентификатор управляющего потока клиента (RFC 9000 §2.1)
	 *
	 */
	static constexpr uint64_t CONTROL_STREAM = 2;
	/**
	 * @brief Идентификатор потока инструкций кодера клиента
	 *
	 */
	static constexpr uint64_t ENCODER_STREAM = 6;

	/**
	 * @brief Структура пары поля эталонной нагрузки
	 *
	 * @details Тип намеренно свой, а не заимствованный у одной из сравниваемых
	 *          реализаций: перевод набора в собственное представление реализации
	 *          выполняется до замера и в измеряемое время не входит
	 *
	 */
	typedef struct Field {
		// Название поля
		std::string name;
		// Значение поля
		std::string value;
		/**
		 * Признак чувствительного значения (RFC 9204 §4.5.4)
		 *
		 * @details Поле кодируется представлением Literal Never Indexed и
		 *          в динамическую таблицу не попадает. Признак задан нагрузкой,
		 *          а не политикой реализации: иначе сравнивалась бы не скорость
		 *          кодирования, а осторожность каждого кодера в отношении печенья
		 */
		bool sensitive;
		/**
		 * @brief Конструктор
		 *
		 * @param name      название поля
		 * @param value     значение поля
		 * @param sensitive признак чувствительного значения
		 *
		 */
		explicit Field(std::string name, std::string value, const bool sensitive = false) noexcept :
		 name(::std::move(name)), value(::std::move(value)), sensitive(sensitive) {}
	} field_t;
	/**
	 * @brief Структура порции октетов одного потока
	 *
	 * @details Единого байтового потока у соединения HTTP/3 нет: каждый поток QUIC
	 *          доставляется отдельно, поэтому канонический поток октетов - это
	 *          последовательность адресованных порций, а не одна строка
	 *
	 */
	typedef struct Piece {
		// Идентификатор потока
		uint64_t sid;
		// Октеты потока
		std::string data;
		// Признак завершения потока
		bool fin;
		/**
		 * @brief Конструктор
		 *
		 * @param sid  идентификатор потока
		 * @param data октеты потока
		 * @param fin  признак завершения потока
		 *
		 */
		explicit Piece(const uint64_t sid, std::string data, const bool fin) noexcept :
		 sid(sid), data(::std::move(data)), fin(fin) {}
	} piece_t;
	/**
	 * @brief Структура закодированной секции полей канонического потока
	 *
	 * @details Секция и инструкции, которыми она обеспечена, хранятся вместе:
	 *          в QPACK они едут разными потоками, и декодер обязан получить
	 *          инструкции раньше секции, иначе поток заблокируется
	 *
	 */
	typedef struct Encoded {
		// Инструкции потока кодера, обеспечивающие секцию
		std::string instructions;
		// Октеты самой секции полей
		std::string section;
		// Идентификатор потока, которому принадлежит секция
		uint64_t sid;
		/**
		 * @brief Конструктор
		 *
		 * @param instructions инструкции потока кодера
		 * @param section      октеты секции полей
		 * @param sid          идентификатор потока
		 *
		 */
		explicit Encoded(std::string instructions, std::string section, const uint64_t sid) noexcept :
		 instructions(::std::move(instructions)), section(::std::move(section)), sid(sid) {}
	} encoded_t;
	/**
	 * @brief Структура итогов прогона сценария
	 *
	 */
	typedef struct Outcome {
		// Количество выполненных операций
		size_t operations;
		// Количество обработанных октетов
		size_t bytes;
		// Объём инструкций потока кодера в октетах
		size_t instructions;
		// Объём нагрузки до сжатия в октетах
		size_t original;
		// Затраченное время в секундах
		double seconds;
		// Количество выполненных выделений памяти
		size_t allocations;
		// Суммарный объём выделенной памяти в октетах
		size_t allocated;
		/**
		 * @brief Конструктор
		 *
		 */
		explicit Outcome() noexcept :
		 operations(0), bytes(0), instructions(0), original(0), seconds(0.0), allocations(0), allocated(0) {}
	} outcome_t;
	/**
	 * @brief Внутреннее состояние учёта выделений памяти
	 *
	 */
	namespace counter {
		// Количество выполненных выделений памяти
		static size_t count = 0;
		// Суммарный объём выделенной памяти в октетах
		static size_t bytes = 0;
		// Флаг активности учёта выделений памяти
		static bool enabled = false;
	};
	/**
	 * @brief Контрольная сумма обработанных данных
	 *
	 * @details Складывается из размеров разобранных полей и суммы октетов тела.
	 *          При одинаковой нагрузке и одинаковом объёме работы потребителя она
	 *          обязана совпасть у всех сравниваемых реализаций: расхождение
	 *          означает, что какая-то из них обрабатывает не то же самое
	 *
	 */
	static volatile size_t checksum = 0;
	/**
	 * @brief Функция учёта выполненного выделения памяти
	 *
	 * @param size объём выделенной памяти
	 *
	 */
	static inline void note(const size_t size) noexcept {
		// Если учёт выделений памяти включён
		if(counter::enabled){
			// Считаем выполненное выделение памяти
			counter::count++;
			// Суммируем объём выделенной памяти
			counter::bytes += size;
		}
	}
	/**
	 * @brief Функция потребления фрагмента тела сообщения
	 *
	 * @details Работа потребителя обязана совпадать у всех сравниваемых реализаций:
	 *          иначе показатель отражал бы не скорость разбора, а объём работы,
	 *          выполненной после него
	 *
	 * @param buffer буфер фрагмента тела
	 * @param size   размер фрагмента тела
	 *
	 */
	static inline void consume(const void * buffer, const size_t size) noexcept {
		// Контрольная сумма фрагмента тела
		size_t sum = 0;
		/**
		 * Выполняем перебор всех октетов фрагмента тела
		 */
		for(size_t i = 0; i < size; i++)
			// Накапливаем контрольную сумму фрагмента тела
			sum += static_cast <const uint8_t *> (buffer)[i];
		// Дописываем контрольную сумму фрагмента в общую
		checksum += sum;
	}
	/**
	 * @brief Функция учёта разобранного поля
	 *
	 * @param name  размер названия поля
	 * @param value размер значения поля
	 *
	 */
	static inline void account(const size_t name, const size_t value) noexcept {
		// Дописываем размеры разобранного поля в контрольную сумму
		checksum += (name + value);
	}
	/**
	 * @brief Функция управления учётом выделений памяти
	 *
	 * @param mode режим учёта выделений памяти
	 *
	 */
	static inline void counting(const bool mode) noexcept {
		// Если учёт выделений памяти включается
		if(mode){
			// Обнуляем количество выполненных выделений памяти
			counter::count = 0;
			// Обнуляем объём выделенной памяти
			counter::bytes = 0;
		}
		// Устанавливаем режим учёта выделений памяти
		counter::enabled = mode;
	}
	/**
	 * @brief Функция получения эталонного тела ответа
	 *
	 * @return тело ответа
	 *
	 */
	static inline const std::string & payload() noexcept {
		// Эталонное тело ответа
		static const std::string result(64, 'r');
		// Выводим тело ответа
		return result;
	}
	/**
	 * @brief Функция получения эталонного набора полей запроса браузера
	 *
	 * @details Набор повторяет запрос настоящего браузера и совпадает с нагрузкой
	 *          стендов HTTP/2 октет в октет: показатели двух протоколов имеет смысл
	 *          сравнивать только на одной и той же нагрузке
	 *
	 * @param index номер запроса в последовательности
	 * @return      набор полей запроса
	 *
	 */
	static inline std::vector <field_t> request(const size_t index) noexcept {
		// Результат работы функции - набор полей запроса
		std::vector <field_t> result;
		// Резервируем память под набор полей
		result.reserve(12);
		// Дописываем псевдо-поле метода запроса
		result.emplace_back(":method", "GET");
		// Дописываем псевдо-поле схемы запроса
		result.emplace_back(":scheme", "https");
		// Дописываем псевдо-поле авторитета запроса
		result.emplace_back(":authority", "www.example.com");
		// Дописываем псевдо-поле пути запроса с переменной частью
		result.emplace_back(":path", ("/assets/bundle-" + std::to_string(index % SET_COUNT) + ".js"));
		// Дописываем поле принимаемых типов содержимого
		result.emplace_back("accept", "text/html,application/xhtml+xml,application/xml;q=0.9,image/avif,image/webp,*/*;q=0.8");
		// Дописываем поле принимаемых кодировок содержимого
		result.emplace_back("accept-encoding", "gzip, deflate, br, zstd");
		// Дописываем поле принимаемых языков
		result.emplace_back("accept-language", "ru-RU,ru;q=0.9,en-US;q=0.8,en;q=0.7");
		// Дописываем поле клиентского приложения
		result.emplace_back("user-agent", "Mozilla/5.0 (Macintosh; Intel Mac OS X 10_15_7) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/126.0.0.0 Safari/537.36");
		// Дописываем поле источника перехода
		result.emplace_back("referer", "https://www.example.com/catalog/index.html");
		/**
		 * Дописываем поле печенья сессии: значение чувствительное, поэтому кодируется
		 * представлением Literal Never Indexed и в динамическую таблицу не попадает
		 * (RFC 9204 §4.5.4). Признак задан нагрузкой одинаково для всех
		 */
		result.emplace_back("cookie", "session=7f3c1a9b2e4d6f8a0c2e4b6d8f1a3c5e; theme=dark; lang=ru", true);
		// Дописываем поле политики кеширования
		result.emplace_back("cache-control", "no-cache");
		// Выводим набор полей запроса
		return result;
	}
	/**
	 * @brief Функция получения эталонного набора полей ответа сервера
	 *
	 * @param index номер ответа в последовательности
	 * @return      набор полей ответа
	 *
	 */
	static inline std::vector <field_t> response(const size_t index) noexcept {
		// Результат работы функции - набор полей ответа
		std::vector <field_t> result;
		// Резервируем память под набор полей
		result.reserve(8);
		// Дописываем псевдо-поле статуса ответа
		result.emplace_back(":status", "200");
		// Дописываем поле типа содержимого
		result.emplace_back("content-type", "application/javascript; charset=utf-8");
		// Дописываем поле длины содержимого: она обязана совпасть с фактическим телом
		result.emplace_back("content-length", std::to_string(payload().size()));
		// Дописываем поле политики кеширования
		result.emplace_back("cache-control", "public, max-age=31536000, immutable");
		// Дописываем поле метки версии содержимого
		result.emplace_back("etag", ("\"" + std::to_string(index) + "-a1b2c3d4\""));
		// Дописываем поле сервера
		result.emplace_back("server", "awh");
		// Выводим набор полей ответа
		return result;
	}
	/**
	 * @brief Функция подсчёта размера набора полей до сжатия
	 *
	 * @note Считается по сумме длин названий и значений, без надбавки в 32 октета
	 *       на запись: сравнивать сжатую секцию нужно с объёмом самих данных
	 *
	 * @param fields набор полей
	 * @return       размер набора полей в октетах
	 *
	 */
	static inline size_t length(const std::vector <field_t> & fields) noexcept {
		// Результат работы функции - размер набора полей
		size_t result = 0;
		/**
		 * Выполняем перебор всех полей набора
		 */
		for(const auto & field : fields)
			// Суммируем длины названия и значения поля
			result += (field.name.size() + field.value.size());
		// Выводим размер набора полей
		return result;
	}
	/**
	 * @brief Функция получения наборов полей запроса
	 *
	 * @param count количество формируемых наборов
	 * @return      наборы полей запроса
	 *
	 */
	static inline std::vector <std::vector <field_t>> requests(const size_t count) noexcept {
		// Результат работы функции - наборы полей запроса
		std::vector <std::vector <field_t>> result;
		// Резервируем память под наборы полей
		result.reserve(count);
		/**
		 * Выполняем формирование всех наборов полей
		 */
		for(size_t i = 0; i < count; i++)
			// Дописываем очередной набор полей
			result.push_back(request(i));
		// Выводим наборы полей запроса
		return result;
	}
	/**
	 * @brief Функция кодирования целого переменной длины QUIC (RFC 9000 §16)
	 *
	 * @param value кодируемое значение
	 * @return      закодированное значение
	 *
	 */
	static inline std::string varint(const uint64_t value) noexcept {
		// Результат работы функции - закодированное значение
		std::string result;
		// Если значение помещается в один октет
		if(value <= 0x3F)
			// Дописываем однооктетное представление
			result.push_back(static_cast <char> (value & 0xFF));
		// Если значение помещается в два октета
		else if(value <= 0x3FFF) {
			// Дописываем старший октет с признаком длины
			result.push_back(static_cast <char> (0x40 | ((value >> 8) & 0x3F)));
			// Дописываем младший октет значения
			result.push_back(static_cast <char> (value & 0xFF));
		// Если значение помещается в четыре октета
		} else if(value <= 0x3FFFFFFF) {
			// Дописываем старший октет с признаком длины
			result.push_back(static_cast <char> (0x80 | ((value >> 24) & 0x3F)));
			/**
			 * Выполняем дозапись оставшихся октетов значения
			 */
			for(int32_t shift = 16; shift >= 0; shift -= 8)
				// Дописываем очередной октет значения
				result.push_back(static_cast <char> ((value >> shift) & 0xFF));
		// Если значение требует восьми октетов
		} else {
			// Дописываем старший октет с признаком длины
			result.push_back(static_cast <char> (0xC0 | ((value >> 56) & 0x3F)));
			/**
			 * Выполняем дозапись оставшихся октетов значения
			 */
			for(int32_t shift = 48; shift >= 0; shift -= 8)
				// Дописываем очередной октет значения
				result.push_back(static_cast <char> ((value >> shift) & 0xFF));
		}
		// Выводим закодированное значение
		return result;
	}
	/**
	 * @brief Функция сборки кадра HTTP/3
	 *
	 * @param type    тип кадра
	 * @param payload полезная нагрузка кадра
	 * @return        собранный кадр
	 *
	 */
	static inline std::string frame(const uint64_t type, const std::string & payload) noexcept {
		// Заголовок собираемого кадра
		std::string result = (varint(type) + varint(payload.size()));
		// Резервируем память под кадр целиком
		result.reserve(result.size() + payload.size());
		// Дописываем полезную нагрузку кадра
		result += payload;
		// Выводим собранный кадр
		return result;
	}
	/**
	 * @brief Функция сборки байтов управляющего потока клиента
	 *
	 * @details Соединение HTTP/3 начинается не преамбулой, как в HTTP/2, а открытием
	 *          однонаправленных потоков. Первым кадром управляющего потока обязан
	 *          идти SETTINGS (RFC 9114 §6.2.1)
	 *
	 * @return байты управляющего потока: тип потока и кадр SETTINGS
	 *
	 */
	static inline std::string control() noexcept {
		// Нагрузка кадра параметров соединения
		std::string options;
		// Дописываем ёмкость динамической таблицы QPACK
		options += (varint(0x01) + varint(TABLE_CAPACITY));
		// Дописываем число потоков, которым разрешено ожидать пополнения таблицы
		options += (varint(0x07) + varint(BLOCKED_STREAMS));
		// Выводим байты управляющего потока
		return (varint(0x00) + frame(0x04, options));
	}
	/**
	 * @brief Функция сборки инструкции подтверждения секции полей (RFC 9204 §4.4.1)
	 *
	 * @details Без подтверждений кодер не вправе ссылаться на записи динамической
	 *          таблицы: пока декодер не сообщил, что вставку принял, ссылка на неё
	 *          заблокировала бы поток. Кодер честно отказывается от таблицы, как
	 *          только исчерпан лимит блокируемых потоков, и сжатие вырождается
	 *          в статическую таблицу с литералами - путь, которого на живом
	 *          соединении не бывает
	 *
	 * @param sid идентификатор потока, чья секция подтверждается
	 * @return    собранная инструкция потока декодера
	 *
	 */
	static inline std::string acknowledge(const uint64_t sid) noexcept {
		// Результат работы функции - собранная инструкция
		std::string result;
		// Если идентификатор потока помещается в семибитный префикс целиком
		if(sid < 0x7F){
			// Дописываем идентификатор потока прямо в префикс инструкции
			result.push_back(static_cast <char> (0x80 | static_cast <uint8_t> (sid)));
			// Выводим собранную инструкцию
			return result;
		}
		// Заполняем префикс единицами - признак продолжения значения
		result.push_back(static_cast <char> (0xFF));
		// Остаток идентификатора сверх префикса
		uint64_t rest = (sid - 0x7F);
		/**
		 * Выполняем дозапись остатка семибитными группами
		 */
		while(rest >= 128){
			// Дописываем очередную группу с признаком продолжения
			result.push_back(static_cast <char> ((rest & 0x7F) | 0x80));
			// Сдвигаем остаток на разобранную группу
			rest >>= 7;
		}
		// Дописываем последнюю группу остатка
		result.push_back(static_cast <char> (rest & 0x7F));
		// Выводим собранную инструкцию
		return result;
	}
	/**
	 * @brief Функция кодирования секции полей эталонной реализацией
	 *
	 * @details Канонический поток октетов порождает эталонная реализация, а не
	 *          кодер каждого стенда: декодеры обязаны получить одни и те же байты,
	 *          иначе показатель декодирования отражал бы удачность чужого кодера
	 *
	 * @param encoder кодер эталонной реализации
	 * @param fields  набор полей
	 * @param sid     идентификатор потока
	 * @return        закодированная секция вместе с обеспечивающими её инструкциями
	 *
	 */
	static inline encoded_t deflate(nghttp3_qpack_encoder * encoder, const std::vector <field_t> & fields, const uint64_t sid) noexcept {
		// Набор полей в представлении эталонной реализации
		std::vector <nghttp3_nv> items;
		// Резервируем память под набор полей
		items.reserve(fields.size());
		/**
		 * Выполняем перевод набора полей в представление эталонной реализации
		 */
		for(const auto & field : fields)
			// Дописываем очередное поле набора
			items.push_back(nghttp3_nv{
				reinterpret_cast <uint8_t *> (const_cast <char *> (field.name.data())),
				reinterpret_cast <uint8_t *> (const_cast <char *> (field.value.data())),
				field.name.size(), field.value.size(),
				static_cast <uint8_t> (field.sensitive ? NGHTTP3_NV_FLAG_NEVER_INDEX : NGHTTP3_NV_FLAG_NONE)
			});
		// Буфер префикса секции полей
		nghttp3_buf prefix;
		// Буфер представлений полей секции
		nghttp3_buf lines;
		// Буфер инструкций потока кодера
		nghttp3_buf instructions;
		// Выполняем инициализацию буфера префикса секции полей
		::nghttp3_buf_init(&prefix);
		// Выполняем инициализацию буфера представлений полей секции
		::nghttp3_buf_init(&lines);
		// Выполняем инициализацию буфера инструкций потока кодера
		::nghttp3_buf_init(&instructions);
		// Результат работы функции - закодированная секция полей
		encoded_t result("", "", sid);
		// Выполняем кодирование секции полей эталонной реализацией
		if(::nghttp3_qpack_encoder_encode(encoder, &prefix, &lines, &instructions, static_cast <int64_t> (sid), items.data(), items.size()) == 0){
			// Забираем инструкции потока кодера
			result.instructions.assign(reinterpret_cast <const char *> (instructions.pos), ::nghttp3_buf_len(&instructions));
			// Собираем секцию полей из префикса
			result.section.assign(reinterpret_cast <const char *> (prefix.pos), ::nghttp3_buf_len(&prefix));
			// Дописываем представления полей секции
			result.section.append(reinterpret_cast <const char *> (lines.pos), ::nghttp3_buf_len(&lines));
			/**
			 * Подтверждаем отправленную секцию кодеру: подтверждение синтезируется,
			 * а не берётся у настоящего декодера, потому что поток порождается
			 * заранее и второй стороны у него нет
			 */
			const std::string confirmation = acknowledge(sid);
			// Подаём подтверждение секции кодеру эталонной реализации
			::nghttp3_qpack_encoder_read_decoder(encoder, reinterpret_cast <const uint8_t *> (confirmation.data()), confirmation.size());
		}
		// Освобождаем буфер префикса секции полей
		::nghttp3_buf_free(&prefix, ::nghttp3_mem_default());
		// Освобождаем буфер представлений полей секции
		::nghttp3_buf_free(&lines, ::nghttp3_mem_default());
		// Освобождаем буфер инструкций потока кодера
		::nghttp3_buf_free(&instructions, ::nghttp3_mem_default());
		// Выводим закодированную секцию полей
		return result;
	}
	/**
	 * @brief Функция получения канонического потока секций полей
	 *
	 * @details Поток формируется один раз и переиспользуется всеми сценариями
	 *          стенда: состояние динамической таблицы в нём последовательно,
	 *          и декодер обязан повторить его секция за секцией
	 *
	 * @return канонический поток секций полей
	 *
	 */
	static inline const std::vector <encoded_t> & sections() noexcept {
		// Канонический поток секций полей
		static std::vector <encoded_t> result;
		// Если поток секций полей ещё не сформирован
		if(result.empty()){
			// Кодер эталонной реализации
			nghttp3_qpack_encoder * encoder = nullptr;
			// Создаём кодер эталонной реализации
			if(::nghttp3_qpack_encoder_new(&encoder, TABLE_CAPACITY, ::nghttp3_mem_default()) != 0)
				// Выводим пустой поток секций полей
				return result;
			// Устанавливаем ёмкость динамической таблицы кодера
			::nghttp3_qpack_encoder_set_max_dtable_capacity(encoder, TABLE_CAPACITY);
			// Устанавливаем число потоков, которым разрешено ожидать пополнения таблицы
			::nghttp3_qpack_encoder_set_max_blocked_streams(encoder, BLOCKED_STREAMS);
			// Резервируем память под поток секций полей
			result.reserve(SECTION_ROUNDS);
			/**
			 * Выполняем кодирование всех секций полей потока
			 */
			for(size_t i = 0; i < SECTION_ROUNDS; i++)
				// Дописываем очередную закодированную секцию полей
				result.push_back(deflate(encoder, request(i), static_cast <uint64_t> (i * 4)));
			// Удаляем кодер эталонной реализации
			::nghttp3_qpack_encoder_del(encoder);
		}
		// Выводим канонический поток секций полей
		return result;
	}
	/**
	 * @brief Функция получения канонического потока запросов
	 *
	 * @details Поток повторяет работу клиента: управляющий поток с параметрами
	 *          соединения, поток инструкций кодера и по одному кадру секции полей
	 *          на каждый поток запроса. Инструкции идут перед секцией, которую
	 *          обеспечивают: иначе поток запроса заблокировался бы
	 *
	 * @return канонический поток октетов запросов
	 *
	 */
	static inline const std::vector <piece_t> & requestStream() noexcept {
		// Канонический поток октетов запросов
		static std::vector <piece_t> result;
		// Если поток октетов запросов ещё не сформирован
		if(result.empty()){
			// Кодер эталонной реализации
			nghttp3_qpack_encoder * encoder = nullptr;
			// Создаём кодер эталонной реализации
			if(::nghttp3_qpack_encoder_new(&encoder, TABLE_CAPACITY, ::nghttp3_mem_default()) != 0)
				// Выводим пустой поток октетов запросов
				return result;
			// Устанавливаем ёмкость динамической таблицы кодера
			::nghttp3_qpack_encoder_set_max_dtable_capacity(encoder, TABLE_CAPACITY);
			// Устанавливаем число потоков, которым разрешено ожидать пополнения таблицы
			::nghttp3_qpack_encoder_set_max_blocked_streams(encoder, BLOCKED_STREAMS);
			// Резервируем память под поток запросов
			result.reserve(2 + (REQUEST_ROUNDS * 2));
			// Открываем управляющий поток клиента кадром параметров соединения
			result.emplace_back(CONTROL_STREAM, control(), false);
			// Открываем поток инструкций кодера клиента
			result.emplace_back(ENCODER_STREAM, varint(0x02), false);
			/**
			 * Выполняем формирование всех запросов потока
			 */
			for(size_t i = 0; i < REQUEST_ROUNDS; i++){
				// Идентификатор двунаправленного потока запроса
				const uint64_t sid = (i * 4);
				// Кодируем секцию полей запроса эталонной реализацией
				encoded_t item = deflate(encoder, request(i), sid);
				// Если инструкции потока кодера есть
				if(!item.instructions.empty())
					// Дописываем инструкции в поток кодера клиента
					result.emplace_back(ENCODER_STREAM, ::std::move(item.instructions), false);
				// Дописываем кадр секции полей с завершением потока
				result.emplace_back(sid, frame(0x01, item.section), true);
			}
			// Удаляем кодер эталонной реализации
			::nghttp3_qpack_encoder_del(encoder);
		}
		// Выводим канонический поток октетов запросов
		return result;
	}
	/**
	 * @brief Функция получения канонического потока тела сообщения
	 *
	 * @details Поток повторяет загрузку файла: управляющий поток с параметрами
	 *          соединения, кадр секции полей запроса и кадры данных
	 *
	 * @return канонический поток октетов тела
	 *
	 */
	static inline const std::vector <piece_t> & bodyStream() noexcept {
		// Канонический поток октетов тела
		static std::vector <piece_t> result;
		// Если поток октетов тела ещё не сформирован
		if(result.empty()){
			// Кодер эталонной реализации
			nghttp3_qpack_encoder * encoder = nullptr;
			// Создаём кодер эталонной реализации
			if(::nghttp3_qpack_encoder_new(&encoder, TABLE_CAPACITY, ::nghttp3_mem_default()) != 0)
				// Выводим пустой поток октетов тела
				return result;
			// Формируем поля запроса с телом
			std::vector <field_t> fields;
			// Дописываем псевдо-поле метода запроса
			fields.emplace_back(":method", "POST");
			// Дописываем псевдо-поле схемы запроса
			fields.emplace_back(":scheme", "https");
			// Дописываем псевдо-поле авторитета запроса
			fields.emplace_back(":authority", "www.example.com");
			// Дописываем псевдо-поле пути запроса
			fields.emplace_back(":path", "/upload");
			// Кодируем секцию полей запроса эталонной реализацией
			const encoded_t item = deflate(encoder, fields, 0);
			// Удаляем кодер эталонной реализации
			::nghttp3_qpack_encoder_del(encoder);
			// Открываем управляющий поток клиента кадром параметров соединения
			result.emplace_back(CONTROL_STREAM, control(), false);
			// Буфер октетов потока запроса
			std::string stream = frame(0x01, item.section);
			// Резервируем память под поток целиком
			stream.reserve(BODY_SIZE + ((BODY_SIZE / FRAME_SIZE) * 16) + 256);
			// Полезная нагрузка одного кадра данных
			const std::string portion(FRAME_SIZE, 'x');
			// Количество кадров данных потока
			const size_t total = (BODY_SIZE / FRAME_SIZE);
			/**
			 * Дописываем кадры данных: поток завершается признаком FIN транспорта,
			 * собственного флага завершения у кадров HTTP/3 нет
			 */
			for(size_t i = 0; i < total; i++)
				// Дописываем очередной кадр данных
				stream += frame(0x00, portion);
			// Дописываем поток запроса с завершением
			result.emplace_back(0, ::std::move(stream), true);
		}
		// Выводим канонический поток октетов тела
		return result;
	}
	/**
	 * @brief Функция прогона кодирования секций полей
	 *
	 * @note Функция объявлена шаблонной намеренно: сравниваемая реализация
	 *       подставляется по значению типа и вызовы её методов инлайнятся.
	 *       Виртуальный интерфейс добавил бы косвенный вызов на каждую секцию
	 *
	 * @tparam Codec сравниваемый кодек: `restart()`, `prepare(наборы)`,
	 *               `encode(номер, поток, объём инструкций)`, `acknowledge(поток)`
	 *
	 * @param codec  сравниваемый кодек
	 * @param output итоги прогона
	 * @return       результат прогона (false - секция закодирована с ошибкой)
	 *
	 */
	template <typename Codec>
	static bool encoding(Codec & codec, outcome_t & output) noexcept {
		// Наборы полей сценария
		const std::vector <std::vector <field_t>> sets = requests(SET_COUNT);
		// Выполняем сброс состояния кодека
		codec.restart();
		// Переводим наборы полей в представление сравниваемой реализации
		codec.prepare(sets);
		// Объём инструкций последней закодированной секции
		size_t instructions = 0;
		/**
		 * Прогреваем динамическую таблицу кодера: измерять установившийся режим
		 * осмысленнее, чем разовое кодирование по холодной таблице
		 */
		for(size_t i = 0; i < sets.size(); i++){
			// Если секция полей закодирована с ошибкой
			if(codec.encode(i, static_cast <uint64_t> (i * 4), instructions) == 0)
				// Выводим отрицательный результат
				return false;
			// Подтверждаем отправленную секцию кодеру
			codec.acknowledge(static_cast <uint64_t> (i * 4));
		}
		// Запоминаем момент начала измерения
		const auto start = std::chrono::steady_clock::now();
		/**
		 * Выполняем кодирование всех секций полей сценария
		 *
		 * Разбор подтверждений входит в замер по праву: это работа самого кодера,
		 * и на живом соединении он занят ею постоянно
		 */
		for(size_t i = 0; i < SECTION_ROUNDS; i++){
			// Суммируем объём закодированной секции
			output.bytes += codec.encode(i % SET_COUNT, static_cast <uint64_t> (i * 4), instructions);
			// Суммируем объём инструкций потока кодера
			output.instructions += instructions;
			// Подтверждаем отправленную секцию кодеру
			codec.acknowledge(static_cast <uint64_t> (i * 4));
		}
		// Запоминаем момент окончания измерения
		const auto finish = std::chrono::steady_clock::now();
		/**
		 * Считаем объём полей до сжатия после замера: подсчёт нужен только
		 * для сведений о прогоне и к работе кодера отношения не имеет
		 */
		for(size_t i = 0; i < SECTION_ROUNDS; i++)
			// Суммируем объём полей набора до сжатия
			output.original += length(sets[i % SET_COUNT]);
		// Устанавливаем количество выполненных операций
		output.operations = SECTION_ROUNDS;
		// Устанавливаем затраченное время
		output.seconds = std::chrono::duration <double> (finish - start).count();
		// Выводим положительный результат
		return true;
	}
	/**
	 * @brief Функция снятия статистики выделений памяти в итоги прогона
	 *
	 * @param measured признак выполнявшегося учёта выделений памяти
	 * @param output   итоги прогона
	 *
	 */
	static inline void harvest(const bool measured, outcome_t & output) noexcept {
		// Если учёт выделений памяти не выполнялся
		if(!measured)
			// Выходим без снятия статистики
			return;
		// Отключаем учёт выделений памяти
		counting(false);
		// Устанавливаем количество выполненных выделений памяти
		output.allocations = counter::count;
		// Устанавливаем объём выделенной памяти
		output.allocated = counter::bytes;
	}
	/**
	 * @brief Функция прогона декодирования секций полей
	 *
	 * @tparam Codec сравниваемый кодек: `restart()`, `decode(секция)`
	 *
	 * @param codec    сравниваемый кодек
	 * @param measured признак учёта выделений памяти
	 * @param output   итоги прогона
	 * @return         результат прогона (false - секция декодирована с ошибкой)
	 *
	 */
	template <typename Codec>
	static bool decoding(Codec & codec, const bool measured, outcome_t & output) noexcept {
		// Получаем канонический поток секций полей
		const std::vector <encoded_t> & stream = sections();
		// Если поток секций полей не сформирован
		if(stream.empty())
			// Выводим отрицательный результат
			return false;
		// Выполняем сброс состояния кодека
		codec.restart();
		// Если требуется учёт выделений памяти
		if(measured)
			// Включаем учёт выделений памяти
			counting(true);
		// Запоминаем момент начала измерения
		const auto start = std::chrono::steady_clock::now();
		/**
		 * Выполняем декодирование всех секций полей потока
		 */
		for(size_t i = 0; i < stream.size(); i++){
			// Если секция полей декодирована с ошибкой
			if(!codec.decode(stream[i])){
				// Снимаем статистику выделений памяти
				harvest(measured, output);
				// Выводим отрицательный результат
				return false;
			}
			// Суммируем объём декодированной секции
			output.bytes += stream[i].section.size();
			// Суммируем объём разобранных инструкций потока кодера
			output.instructions += stream[i].instructions.size();
		}
		// Запоминаем момент окончания измерения
		const auto finish = std::chrono::steady_clock::now();
		// Снимаем статистику выделений памяти
		harvest(measured, output);
		// Устанавливаем количество выполненных операций
		output.operations = stream.size();
		// Устанавливаем затраченное время
		output.seconds = std::chrono::duration <double> (finish - start).count();
		// Выводим положительный результат
		return true;
	}
	/**
	 * @brief Функция прогона измерения степени сжатия полей
	 *
	 * @details Сжатие измеряется на холодной динамической таблице: показатель
	 *          отражает установившийся режим, а установившийся режим наступает
	 *          через несколько десятков секций и должен попасть в замер вместе
	 *          с дорогой первой секцией
	 *
	 * @tparam Codec сравниваемый кодек: `restart()`, `prepare(наборы)`,
	 *               `encode(номер, поток, объём инструкций)`, `acknowledge(поток)`
	 *
	 * @param codec  сравниваемый кодек
	 * @param first  объём первой закодированной секции
	 * @param output итоги прогона
	 * @return       результат прогона (false - секция закодирована с ошибкой)
	 *
	 */
	template <typename Codec>
	static bool compression(Codec & codec, size_t & first, outcome_t & output) noexcept {
		// Наборы полей сценария
		const std::vector <std::vector <field_t>> sets = requests(RATIO_ROUNDS);
		// Выполняем сброс состояния кодека
		codec.restart();
		// Переводим наборы полей в представление сравниваемой реализации
		codec.prepare(sets);
		// Объём инструкций последней закодированной секции
		size_t instructions = 0;
		/**
		 * Выполняем кодирование всех секций полей сценария
		 */
		for(size_t i = 0; i < sets.size(); i++){
			// Кодируем очередную секцию полей
			const size_t size = codec.encode(i, static_cast <uint64_t> (i * 4), instructions);
			// Если секция полей закодирована с ошибкой
			if(size == 0)
				// Выводим отрицательный результат
				return false;
			// Подтверждаем отправленную секцию кодеру
			codec.acknowledge(static_cast <uint64_t> (i * 4));
			// Если закодирована первая секция - запоминаем её объём
			if(i == 0)
				// Запоминаем объём первой секции
				first = size;
			// Суммируем объём закодированной секции
			output.bytes += size;
			// Суммируем объём инструкций потока кодера
			output.instructions += instructions;
			// Суммируем объём полей набора до сжатия
			output.original += length(sets[i]);
		}
		// Устанавливаем количество выполненных операций
		output.operations = sets.size();
		// Выводим положительный результат
		return true;
	}
	/**
	 * @brief Функция прогона разбора канонического потока октетов
	 *
	 * @details Порции подаются кусками размером с чтение из сокета: подача одним
	 *          куском переложила бы на разборщик буферизацию всего объёма
	 *
	 * @tparam Session сравниваемая реализация: `feed(поток, данные, размер, признак завершения)`
	 *
	 * @param session  сравниваемая реализация
	 * @param stream   канонический поток октетов
	 * @param measured признак учёта выделений памяти
	 * @param output   итоги прогона
	 *
	 */
	template <typename Session>
	static void feeding(Session & session, const std::vector <piece_t> & stream, const bool measured, outcome_t & output) noexcept {
		// Если требуется учёт выделений памяти
		if(measured)
			// Включаем учёт выделений памяти
			counting(true);
		// Запоминаем момент начала измерения
		const auto start = std::chrono::steady_clock::now();
		/**
		 * Выполняем подачу всех порций канонического потока
		 */
		for(const auto & piece : stream){
			// Если порция помещается в один кусок целиком
			if(piece.data.size() <= CHUNK_SIZE){
				// Подаём порцию потока на разбор
				session.feed(piece.sid, piece.data.data(), piece.data.size(), piece.fin);
				// Переходим к следующей порции
				continue;
			}
			/**
			 * Подаём порцию кусками размером с чтение из сокета
			 */
			for(size_t offset = 0; offset < piece.data.size(); offset += CHUNK_SIZE){
				// Вычисляем размер очередного куска
				const size_t size = (((piece.data.size() - offset) < CHUNK_SIZE) ? (piece.data.size() - offset) : CHUNK_SIZE);
				// Подаём очередной кусок потока на разбор
				session.feed(piece.sid, piece.data.data() + offset, size, (piece.fin && ((offset + size) >= piece.data.size())));
			}
		}
		// Запоминаем момент окончания измерения
		const auto finish = std::chrono::steady_clock::now();
		// Снимаем статистику выделений памяти
		harvest(measured, output);
		// Устанавливаем затраченное время
		output.seconds = std::chrono::duration <double> (finish - start).count();
	}
	/**
	 * @brief Функция прогона полного обмена через пару реализаций
	 *
	 * @details Обмены выполняются пачками по числу одновременных потоков: пачка
	 *          отражает мультиплексирование, при котором стороны обслуживают
	 *          потоки вперемешку
	 *
	 * @tparam Pair сравниваемая пара: `prepare(наборы)`, `open(номер)`, `pump()`, `completed()`
	 *
	 * @param pair    сравниваемая пара клиента и сервера
	 * @param streams количество одновременных потоков
	 * @param output  итоги прогона
	 * @return        результат прогона (false - обмен не выполнен)
	 *
	 */
	template <typename Pair>
	static bool exchanging(Pair & pair, const size_t streams, outcome_t & output) noexcept {
		/**
		 * Готовим наборы полей заранее: сборка набора - работа приложения,
		 * и в замер стоимости обмена попадать не должна
		 */
		const std::vector <std::vector <field_t>> sets = requests(SET_COUNT);
		// Переводим наборы полей в представление сравниваемой реализации
		pair.prepare(sets);
		// Запоминаем момент начала измерения
		const auto start = std::chrono::steady_clock::now();
		/**
		 * Выполняем обмены пачками по числу одновременных потоков
		 */
		for(size_t i = 0; i < ROUNDTRIP_ROUNDS; i += streams){
			// Вычисляем размер очередной пачки обменов
			const size_t portion = (((ROUNDTRIP_ROUNDS - i) < streams) ? (ROUNDTRIP_ROUNDS - i) : streams);
			/**
			 * Открываем потоки пачки
			 */
			for(size_t j = 0; j < portion; j++){
				// Если поток открыть не удалось
				if(!pair.open((i + j) % SET_COUNT))
					// Выводим отрицательный результат
					return false;
			}
			// Выполняем прокачку исходящих очередей обеих сторон
			pair.pump();
		}
		// Запоминаем момент окончания измерения
		const auto finish = std::chrono::steady_clock::now();
		// Устанавливаем количество выполненных обменов
		output.operations = pair.completed();
		// Устанавливаем затраченное время
		output.seconds = std::chrono::duration <double> (finish - start).count();
		// Выводим результат прогона
		return (output.operations == ROUNDTRIP_ROUNDS);
	}
	/**
	 * @brief Функция извлечения количества операций в секунду
	 *
	 * @param outcome итоги прогона сценария
	 * @return        количество операций в секунду
	 *
	 */
	static inline double perSecond(const outcome_t & outcome) noexcept {
		// Выводим количество операций в секунду
		return ((outcome.seconds > 0.0) ? (static_cast <double> (outcome.operations) / outcome.seconds) : 0.0);
	}
	/**
	 * @brief Функция извлечения пропускной способности в мебибайтах в секунду
	 *
	 * @param bytes   объём обработанных данных
	 * @param outcome итоги прогона сценария
	 * @return        пропускная способность
	 *
	 */
	static inline double megabytes(const size_t bytes, const outcome_t & outcome) noexcept {
		// Выводим пропускную способность в мебибайтах в секунду
		return ((outcome.seconds > 0.0) ? ((static_cast <double> (bytes) / 1048576.0) / outcome.seconds) : 0.0);
	}
	/**
	 * @brief Функция извлечения количества выделений памяти на одну операцию
	 *
	 * @param outcome итоги прогона сценария
	 * @return        количество выделений памяти на одну операцию
	 *
	 */
	static inline double allocated(const outcome_t & outcome) noexcept {
		// Выводим количество выделений памяти на одну операцию
		return ((outcome.operations > 0) ? (static_cast <double> (outcome.allocations) / static_cast <double> (outcome.operations)) : 0.0);
	}
	/**
	 * @brief Функция вывода результата прогона сценария
	 *
	 * @note Формат вывода повторяет набор бенчмарков AWH, поэтому результаты
	 *       стендов и библиотеки сводятся в одну таблицу без пересчёта
	 *
	 * @param name    название сценария
	 * @param units   единица измерения характеристики
	 * @param value   измеренное значение характеристики
	 * @param details сведения о прогоне сценария
	 *
	 */
	static inline void report(const char * name, const char * units, const double value, const char * details) noexcept {
		// Выводим измеренное значение характеристики
		::printf("%-38s %14.2f   (%s)\n", name, value, units);
		// Выводим сведения о прогоне сценария
		::printf("%40s%s\n", "", details);
	}
	/**
	 * @brief Функция вывода сообщения о невыполненном сценарии
	 *
	 * @param name   название сценария
	 * @param reason причина отказа от прогона
	 *
	 */
	static inline void skip(const char * name, const char * reason) noexcept {
		// Выводим сообщение о невыполненном сценарии
		::printf("%-38s %14s   (%s)\n", name, "—", reason);
	}
	/**
	 * @brief Функция проверки соответствия сценария фильтру запуска
	 *
	 * @param name   название сценария
	 * @param filter фильтр названий сценариев
	 * @return       результат проверки соответствия
	 *
	 */
	static inline bool selected(const char * name, const char * filter) noexcept {
		// Если фильтр не задан, выполняются все сценарии
		return ((filter == nullptr) || (::strstr(name, filter) != nullptr));
	}
	/**
	 * @brief Функция вывода контрольной суммы обработанных данных
	 *
	 * @param argc длина массива параметров
	 * @param argv массив параметров
	 *
	 */
	static inline void digest(const int32_t argc, char ** argv) noexcept {
		/**
		 * Перебираем параметры запуска стенда
		 */
		for(int32_t i = 1; i < argc; i++){
			// Если запрошен вывод контрольной суммы обработанных данных
			if(::strcmp(argv[i], "--checksum") == 0){
				// Выводим контрольную сумму обработанных данных
				::printf("контрольная сумма: %zu\n", static_cast <size_t> (checksum));
				// Выходим из функции
				return;
			}
		}
	}
	/**
	 * @brief Функция получения фильтра названий сценариев из параметров запуска
	 *
	 * @param argc длина массива параметров
	 * @param argv массив параметров
	 * @return     фильтр названий сценариев
	 *
	 */
	static inline const char * filter(const int32_t argc, char ** argv) noexcept {
		/**
		 * Перебираем параметры запуска стенда
		 */
		for(int32_t i = 1; i < argc; i++){
			// Если параметр задаёт фильтр названий сценариев
			if(::strncmp(argv[i], "--filter=", 9) == 0)
				// Выводим фильтр названий сценариев
				return (argv[i] + 9);
		}
		// Фильтр названий сценариев не задан
		return nullptr;
	}
	/**
	 * Если стенд собран с аллокатором TcMalloc
	 */
	#if __AWH_USE_TCMALLOC__
		/**
		 * @brief Функция обратного вызова аллокатора о выполненном выделении памяти
		 *
		 * @note Аллокатор TcMalloc сам подменяет операторы выделения памяти языка,
		 *       поэтому перегрузить их повторно невозможно - учёт ведётся штатным
		 *       механизмом перехватчиков, который он для этого и предоставляет
		 *
		 * @param size размер выделенной памяти
		 *
		 */
		static void allocationHook([[maybe_unused]] const void * ptr, const size_t size){
			// Учитываем выполненное выделение памяти
			note(size);
		}
		/**
		 * @brief Функция установки перехватчика выделений памяти аллокатора
		 *
		 */
		static inline void attach() noexcept {
			// Признак установленного перехватчика выделений памяти
			static bool attached = false;
			// Если перехватчик выделений памяти ещё не установлен
			if(!attached)
				// Устанавливаем перехватчик выделений памяти аллокатора
				attached = MallocHook::AddNewHook(&allocationHook);
		}
	#else
		/**
		 * @brief Функция установки перехватчика выделений памяти аллокатора
		 *
		 * @details Учёт ведётся перегруженными операторами выделения памяти языка,
		 *          устанавливать нечего
		 *
		 */
		static inline void attach() noexcept {}
	#endif
};

/**
 * Если стенд собран без аллокатора TcMalloc
 */
#if !__AWH_USE_TCMALLOC__

/**
 * Оператор выделения памяти с учётом выполненных выделений
 *
 * @details Учёт выделений памяти выполняется подменой операторов языка: точка,
 *          общая для всех сравниваемых реализаций на C++ и для библиотек на C,
 *          собранных с этим же аллокатором
 *
 * @param size объём выделяемой памяти
 * @return     указатель на выделенную память
 *
 */
void * operator new (size_t size){
	// Учитываем выполненное выделение памяти
	rival::note(size);
	// Выполняем выделение памяти
	void * result = ::malloc(size > 0 ? size : 1);
	// Если память выделить не удалось
	if(result == nullptr)
		// Прерываем работу стенда
		::abort();
	// Выводим указатель на выделенную память
	return result;
}
/**
 * Оператор выделения памяти под массив с учётом выполненных выделений
 *
 * @param size объём выделяемой памяти
 * @return     указатель на выделенную память
 *
 */
void * operator new [] (size_t size){
	// Выполняем выделение памяти
	return ::operator new (size);
}
/**
 * Оператор освобождения памяти
 *
 * @param ptr указатель на освобождаемую память
 *
 */
void operator delete (void * ptr) noexcept {
	// Выполняем освобождение памяти
	::free(ptr);
}
/**
 * Оператор освобождения памяти массива
 *
 * @param ptr указатель на освобождаемую память
 *
 */
void operator delete [] (void * ptr) noexcept {
	// Выполняем освобождение памяти
	::free(ptr);
}
/**
 * Оператор освобождения памяти с известным размером
 *
 * @param ptr указатель на освобождаемую память
 *
 */
void operator delete (void * ptr, size_t) noexcept {
	// Выполняем освобождение памяти
	::free(ptr);
}
/**
 * Оператор освобождения памяти массива с известным размером
 *
 * @param ptr указатель на освобождаемую память
 *
 */
void operator delete [] (void * ptr, size_t) noexcept {
	// Выполняем освобождение памяти
	::free(ptr);
}
#endif

#endif // __AWH_BENCHMARK_RIVAL_HTTP3__
