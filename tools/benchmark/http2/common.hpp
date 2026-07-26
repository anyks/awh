/**
 * @file: common.hpp
 * @date: 2026-07-26
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @brief Общее окружение эталонных стендов сравнения протокола HTTP/2 —
 *        эталонная нагрузка, канонический поток октетов, драйверы прогона
 *        сценариев, учёт выделений памяти и вывод результатов
 *
 * @copyright: Copyright © 2026
 *
 */

#ifndef __AWH_BENCHMARK_RIVAL_HTTP2__
#define __AWH_BENCHMARK_RIVAL_HTTP2__

/**
 * Стандартные заголовочные файлы
 */
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>
#include <chrono>

/**
 * Подключаем заголовочный файл эталонной реализации протокола
 *
 * @details Эталонная реализация участвует в общем окружении не как соперник,
 *          а как источник канонического потока октетов: декодеры и разборщики
 *          всех стендов обязаны получить на вход одни и те же байты, иначе
 *          сравнивалась бы не скорость разбора, а удачность чужого кодера
 */
#include <nghttp2/nghttp2.h>

/**
 * Если стенд собран с аллокатором TcMalloc
 */
#if __AWH_USE_TCMALLOC__
	#include <gperftools/malloc_hook.h>
#endif

/**
 * @brief Пространство имён эталонных стендов сравнения протокола HTTP/2
 *
 * @details Эталонная нагрузка и параметры сценариев обязаны совпадать с
 *          `benchmark/proto/http2` библиотеки AWH: сравниваются реализации
 *          протокола, а не разные объёмы работы, поэтому любое расхождение
 *          здесь обесценивает отчёт целиком
 *
 */
namespace rival {
	/**
	 * @brief Количество блоков заголовков сценариев кодирования и декодирования
	 *
	 */
	static constexpr size_t BLOCK_ROUNDS = 200000;
	/**
	 * @brief Количество блоков заголовков сценария измерения сжатия
	 *
	 * @details Степень сжатия зависит от прогретости динамической таблицы, поэтому
	 *          измерять её на одном блоке бессмысленно: первый блок сжимается только
	 *          статической таблицей, а установившийся режим наступает через несколько
	 *          десятков блоков
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
	 * @details Совпадает со значением SETTINGS_MAX_FRAME_SIZE по умолчанию: именно
	 *          такими кадрами нагружает соединение любой согласованный пир
	 *
	 */
	static constexpr size_t FRAME_SIZE = 16384;
	/**
	 * @brief Количество одновременных потоков сценария мультиплексирования
	 *
	 */
	static constexpr size_t STREAM_COUNT = 64;
	/**
	 * @brief Размер порции подачи входящего потока в октетах
	 *
	 * @details Поток подаётся порциями размером с типичное чтение из сокета, а не
	 *          целиком: подача одним куском переложила бы на разборщик буферизацию
	 *          всего объёма и завысила бы показатель - в реальной работе такого не бывает
	 *
	 */
	static constexpr size_t CHUNK_SIZE = (64 * 1024);
	/**
	 * @brief Количество различных наборов заголовков запроса
	 *
	 * @details Переменной частью запроса служит путь, и различных наборов ровно
	 *          столько, сколько их порождает эта переменная часть
	 *
	 */
	static constexpr size_t SET_COUNT = 64;
	/**
	 * @brief Размер динамической таблицы HPACK в октетах
	 *
	 * @details Значение SETTINGS_HEADER_TABLE_SIZE по умолчанию (RFC 9113 §6.5.2).
	 *          Кодер и декодер каждого стенда обязаны держать одинаковую таблицу,
	 *          иначе канонический поток октетов не будет декодирован
	 *
	 */
	static constexpr uint32_t TABLE_SIZE = 4096;

	/**
	 * @brief Структура пары заголовка эталонной нагрузки
	 *
	 * @details Тип намеренно свой, а не заимствованный у одной из сравниваемых
	 *          реализаций: перевод набора в собственное представление реализации
	 *          выполняется до замера и в измеряемое время не входит
	 *
	 */
	typedef struct Field {
		// Название заголовка
		std::string name;
		// Значение заголовка
		std::string value;
		/**
		 * Признак чувствительного значения (RFC 7541 §7.1.3)
		 *
		 * @details Заголовок кодируется представлением Literal Never Indexed и
		 *          в динамическую таблицу не попадает. Признак задан нагрузкой,
		 *          а не политикой реализации: иначе сравнивалась бы не скорость
		 *          кодирования, а осторожность каждого кодера в отношении печенья
		 */
		bool sensitive;
		/**
		 * @brief Конструктор
		 *
		 * @param name      название заголовка
		 * @param value     значение заголовка
		 * @param sensitive признак чувствительного значения
		 *
		 */
		explicit Field(std::string name, std::string value, const bool sensitive = false) noexcept :
		 name(::std::move(name)), value(::std::move(value)), sensitive(sensitive) {}
	} field_t;
	/**
	 * @brief Структура итогов прогона сценария
	 *
	 */
	typedef struct Outcome {
		// Количество выполненных операций
		size_t operations;
		// Количество обработанных октетов
		size_t bytes;
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
		 operations(0), bytes(0), original(0), seconds(0.0), allocations(0), allocated(0) {}
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
	 * @details Потребитель обязан прочитать каждый октет тела и учесть каждый
	 *          разобранный заголовок: без чтения замерялась бы не пропускная
	 *          способность, а скорость перешагивания через данные. Накопитель
	 *          объявлен изменчивым, чтобы оптимизатор не удалил чтение целиком
	 *
	 */
	static volatile size_t checksum = 0;
	/**
	 * @brief Функция учёта выполненного выделения памяти
	 *
	 * @details Точка учёта общая для всех способов перехвата: оператора выделения
	 *          памяти языка, перехватчика аллокатора и подменённого аллокатора
	 *          сравниваемой реализации
	 *
	 * @param size размер выделенной памяти
	 *
	 */
	static inline void note(const size_t size) noexcept {
		// Если учёт выделений памяти активен
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
	 * @note Повторяет работу потребителя сценариев AWH октет в октет:
	 *       сумма фрагмента накапливается в регистре и переносится в
	 *       изменчивый накопитель один раз на фрагмент
	 *
	 * @param buffer буфер фрагмента тела сообщения
	 * @param size   размер фрагмента тела сообщения
	 *
	 */
	static inline void consume(const void * buffer, const size_t size) noexcept {
		// Сумма октетов принятого фрагмента тела
		size_t sum = 0;
		// Получаем байтовый указатель на фрагмент тела
		const uint8_t * data = static_cast <const uint8_t *> (buffer);
		/**
		 * Читаем фрагмент тела целиком
		 */
		for(size_t i = 0; i < size; i++)
			// Суммируем очередной октет фрагмента тела
			sum += data[i];
		// Накапливаем контрольную сумму принятых данных
		checksum = (checksum + sum);
	}
	/**
	 * @brief Функция учёта разобранного заголовка сообщения
	 *
	 * @param name  размер названия заголовка
	 * @param value размер значения заголовка
	 *
	 */
	static inline void account(const size_t name, const size_t value) noexcept {
		// Накапливаем контрольную сумму разобранных заголовков
		checksum = (checksum + name + value);
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
			// Сбрасываем количество выполненных выделений памяти
			counter::count = 0;
			// Сбрасываем объём выделенной памяти
			counter::bytes = 0;
		}
		// Устанавливаем режим учёта выделений памяти
		counter::enabled = mode;
	}
	/**
	 * @brief Функция получения эталонного тела ответа
	 *
	 * @note Объявленная в заголовках длина обязана совпадать с фактической:
	 *       расхождение - малформированное сообщение (RFC 9113 §8.1.1),
	 *       и разборщик справедливо оборвёт поток
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
	 * @brief Функция получения эталонного набора заголовков запроса браузера
	 *
	 * @details Набор повторяет запрос настоящего браузера: псевдо-заголовки,
	 *          длинный user-agent, печенье и переменная часть в виде пути.
	 *          Постоянный путь целиком уходил бы в динамическую таблицу и дал бы
	 *          недостижимо оптимистичное сжатие
	 *
	 * @param index номер запроса в последовательности
	 * @return      набор заголовков запроса
	 *
	 */
	static inline std::vector <field_t> request(const size_t index) noexcept {
		// Результат работы функции - набор заголовков запроса
		std::vector <field_t> result;
		// Резервируем память под набор заголовков
		result.reserve(12);
		// Дописываем псевдо-заголовок метода запроса
		result.emplace_back(":method", "GET");
		// Дописываем псевдо-заголовок схемы запроса
		result.emplace_back(":scheme", "https");
		// Дописываем псевдо-заголовок авторитета запроса
		result.emplace_back(":authority", "www.example.com");
		// Дописываем псевдо-заголовок пути запроса с переменной частью
		result.emplace_back(":path", ("/assets/bundle-" + std::to_string(index % SET_COUNT) + ".js"));
		// Дописываем заголовок принимаемых типов содержимого
		result.emplace_back("accept", "text/html,application/xhtml+xml,application/xml;q=0.9,image/avif,image/webp,*/*;q=0.8");
		// Дописываем заголовок принимаемых кодировок содержимого
		result.emplace_back("accept-encoding", "gzip, deflate, br, zstd");
		// Дописываем заголовок принимаемых языков
		result.emplace_back("accept-language", "ru-RU,ru;q=0.9,en-US;q=0.8,en;q=0.7");
		// Дописываем заголовок клиентского приложения
		result.emplace_back("user-agent", "Mozilla/5.0 (Macintosh; Intel Mac OS X 10_15_7) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/126.0.0.0 Safari/537.36");
		// Дописываем заголовок источника перехода
		result.emplace_back("referer", "https://www.example.com/catalog/index.html");
		/**
		 * Дописываем заголовок печенья сессии: значение чувствительное, поэтому
		 * кодируется представлением Literal Never Indexed и в динамическую таблицу
		 * не попадает (RFC 7541 §7.1.3). Признак задан нагрузкой одинаково для всех
		 */
		result.emplace_back("cookie", "session=7f3c1a9b2e4d6f8a0c2e4b6d8f1a3c5e; theme=dark; lang=ru", true);
		// Дописываем заголовок политики кеширования
		result.emplace_back("cache-control", "no-cache");
		// Выводим набор заголовков запроса
		return result;
	}
	/**
	 * @brief Функция получения эталонного набора заголовков ответа сервера
	 *
	 * @param index номер ответа в последовательности
	 * @return      набор заголовков ответа
	 *
	 */
	static inline std::vector <field_t> response(const size_t index) noexcept {
		// Результат работы функции - набор заголовков ответа
		std::vector <field_t> result;
		// Резервируем память под набор заголовков
		result.reserve(8);
		// Дописываем псевдо-заголовок статуса ответа
		result.emplace_back(":status", "200");
		// Дописываем заголовок типа содержимого
		result.emplace_back("content-type", "application/javascript; charset=utf-8");
		// Дописываем заголовок длины содержимого: она обязана совпасть с фактическим телом
		result.emplace_back("content-length", std::to_string(payload().size()));
		// Дописываем заголовок политики кеширования
		result.emplace_back("cache-control", "public, max-age=31536000, immutable");
		// Дописываем заголовок метки версии содержимого
		result.emplace_back("etag", ("\"" + std::to_string(index) + "-a1b2c3d4\""));
		// Дописываем заголовок сервера
		result.emplace_back("server", "awh");
		// Выводим набор заголовков ответа
		return result;
	}
	/**
	 * @brief Функция подсчёта размера набора заголовков до сжатия
	 *
	 * @note Считается по правилу RFC 9113 §6.5.2 без надбавки в 32 октета
	 *       на запись: сравнивать сжатый блок нужно с объёмом самих данных,
	 *       а не с оценкой памяти под них
	 *
	 * @param fields набор заголовков
	 * @return       размер набора заголовков в октетах
	 *
	 */
	static inline size_t length(const std::vector <field_t> & fields) noexcept {
		// Результат работы функции - размер набора заголовков
		size_t result = 0;
		/**
		 * Выполняем перебор всех заголовков набора
		 */
		for(const auto & field : fields)
			// Суммируем длины названия и значения заголовка
			result += (field.name.size() + field.value.size());
		// Выводим размер набора заголовков
		return result;
	}
	/**
	 * @brief Функция сборки кадра HTTP/2
	 *
	 * @param type    тип кадра
	 * @param flags   флаги кадра
	 * @param sid     идентификатор потока
	 * @param payload полезная нагрузка кадра
	 * @return        собранный кадр
	 *
	 */
	static inline std::string frame(const uint8_t type, const uint8_t flags, const uint32_t sid, const std::string & payload) noexcept {
		// Результат работы функции - собранный кадр
		std::string result;
		// Резервируем память под кадр целиком
		result.reserve(9 + payload.size());
		// Дописываем 24-битную длину полезной нагрузки
		result.push_back(static_cast <char> ((payload.size() >> 16) & 0xFF));
		result.push_back(static_cast <char> ((payload.size() >> 8) & 0xFF));
		result.push_back(static_cast <char> (payload.size() & 0xFF));
		// Дописываем тип кадра
		result.push_back(static_cast <char> (type));
		// Дописываем флаги кадра
		result.push_back(static_cast <char> (flags));
		// Дописываем идентификатор потока
		result.push_back(static_cast <char> ((sid >> 24) & 0x7F));
		result.push_back(static_cast <char> ((sid >> 16) & 0xFF));
		result.push_back(static_cast <char> ((sid >> 8) & 0xFF));
		result.push_back(static_cast <char> (sid & 0xFF));
		// Дописываем полезную нагрузку
		result += payload;
		// Выводим собранный кадр
		return result;
	}
	/**
	 * @brief Функция сборки преамбулы соединения клиента
	 *
	 * @return преамбула соединения: magic-строка и кадр SETTINGS
	 *
	 */
	static inline std::string preface() noexcept {
		// Результат работы функции - преамбула соединения
		std::string result("PRI * HTTP/2.0\r\n\r\nSM\r\n\r\n");
		// Дописываем кадр SETTINGS без параметров
		result += frame(0x04, 0x00, 0, "");
		// Выводим преамбулу соединения
		return result;
	}
	/**
	 * @brief Функция получения наборов заголовков запроса
	 *
	 * @param count количество формируемых наборов
	 * @return      наборы заголовков запроса
	 *
	 */
	static inline std::vector <std::vector <field_t>> requests(const size_t count) noexcept {
		// Результат работы функции - наборы заголовков запроса
		std::vector <std::vector <field_t>> result;
		// Резервируем память под наборы заголовков
		result.reserve(count);
		/**
		 * Выполняем формирование всех наборов заголовков
		 */
		for(size_t i = 0; i < count; i++)
			// Дописываем очередной набор заголовков
			result.push_back(request(i));
		// Выводим наборы заголовков запроса
		return result;
	}
	/**
	 * @brief Функция кодирования блока заголовков эталонной реализацией
	 *
	 * @details Канонический поток октетов порождает эталонная реализация, а не
	 *          кодер каждого стенда: декодеры обязаны получить одни и те же байты,
	 *          иначе показатель декодирования отражал бы удачность чужого кодера
	 *
	 * @param deflater кодер эталонной реализации
	 * @param fields   набор заголовков
	 * @return         закодированный блок заголовков
	 *
	 */
	static inline std::string deflate(nghttp2_hd_deflater * deflater, const std::vector <field_t> & fields) noexcept {
		// Набор заголовков в представлении эталонной реализации
		std::vector <nghttp2_nv> items;
		// Резервируем память под набор заголовков
		items.reserve(fields.size());
		/**
		 * Выполняем перевод набора заголовков в представление эталонной реализации
		 */
		for(const auto & field : fields)
			// Дописываем очередной заголовок набора
			items.push_back(nghttp2_nv{
				reinterpret_cast <uint8_t *> (const_cast <char *> (field.name.data())),
				reinterpret_cast <uint8_t *> (const_cast <char *> (field.value.data())),
				field.name.size(), field.value.size(),
				static_cast <uint8_t> (field.sensitive ? NGHTTP2_NV_FLAG_NO_INDEX : NGHTTP2_NV_FLAG_NONE)
			});
		// Результат работы функции - закодированный блок заголовков
		std::string result;
		// Выделяем место под блок заголовков по верхней оценке кодера
		result.resize(nghttp2_hd_deflate_bound(deflater, items.data(), items.size()));
		// Выполняем кодирование блока заголовков
		const nghttp2_ssize size = nghttp2_hd_deflate_hd2(
			deflater, reinterpret_cast <uint8_t *> (&result[0]), result.size(), items.data(), items.size()
		);
		// Если блок заголовков закодирован с ошибкой
		if(size < 0)
			// Очищаем блок заголовков
			result.clear();
		// Если блок заголовков закодирован
		else result.resize(static_cast <size_t> (size));
		// Выводим закодированный блок заголовков
		return result;
	}
	/**
	 * @brief Функция получения канонического потока блоков заголовков
	 *
	 * @details Поток формируется один раз и переиспользуется всеми сценариями
	 *          стенда: состояние динамической таблицы в нём последовательно,
	 *          и декодер обязан повторить его блок за блоком
	 *
	 * @return канонический поток блоков заголовков
	 *
	 */
	static inline const std::vector <std::string> & blocks() noexcept {
		// Канонический поток блоков заголовков
		static std::vector <std::string> result;
		// Если поток блоков заголовков ещё не сформирован
		if(result.empty()){
			// Кодер эталонной реализации
			nghttp2_hd_deflater * deflater = nullptr;
			// Создаём кодер эталонной реализации
			if(nghttp2_hd_deflate_new(&deflater, TABLE_SIZE) != 0)
				// Выводим пустой поток блоков заголовков
				return result;
			// Резервируем память под поток блоков заголовков
			result.reserve(BLOCK_ROUNDS);
			/**
			 * Выполняем кодирование всех блоков заголовков потока
			 */
			for(size_t i = 0; i < BLOCK_ROUNDS; i++)
				// Дописываем очередной закодированный блок заголовков
				result.push_back(deflate(deflater, request(i)));
			// Удаляем кодер эталонной реализации
			nghttp2_hd_deflate_del(deflater);
		}
		// Выводим канонический поток блоков заголовков
		return result;
	}
	/**
	 * @brief Функция получения канонического потока запросов
	 *
	 * @details Поток повторяет работу клиента: преамбула соединения и кадры
	 *          заголовков с завершением потока, по одному запросу на поток
	 *
	 * @return канонический поток октетов запросов
	 *
	 */
	static inline const std::string & requestStream() noexcept {
		// Канонический поток октетов запросов
		static std::string result;
		// Если поток октетов запросов ещё не сформирован
		if(result.empty()){
			// Кодер эталонной реализации
			nghttp2_hd_deflater * deflater = nullptr;
			// Создаём кодер эталонной реализации
			if(nghttp2_hd_deflate_new(&deflater, TABLE_SIZE) != 0)
				// Выводим пустой поток октетов запросов
				return result;
			// Дописываем преамбулу соединения клиента
			result = preface();
			// Резервируем память под поток запросов
			result.reserve(REQUEST_ROUNDS * 128);
			/**
			 * Выполняем формирование всех кадров заголовков потока
			 */
			for(size_t i = 0; i < REQUEST_ROUNDS; i++)
				// Дописываем кадр заголовков с завершением потока
				result += frame(0x01, 0x05, static_cast <uint32_t> (1 + (i * 2)), deflate(deflater, request(i)));
			// Удаляем кодер эталонной реализации
			nghttp2_hd_deflate_del(deflater);
		}
		// Выводим канонический поток октетов запросов
		return result;
	}
	/**
	 * @brief Функция получения канонического потока тела сообщения
	 *
	 * @details Поток повторяет загрузку файла: преамбула соединения, кадр
	 *          заголовков запроса и кадры данных предельного размера
	 *
	 * @return канонический поток октетов тела
	 *
	 */
	static inline const std::string & bodyStream() noexcept {
		// Канонический поток октетов тела
		static std::string result;
		// Если поток октетов тела ещё не сформирован
		if(result.empty()){
			// Кодер эталонной реализации
			nghttp2_hd_deflater * deflater = nullptr;
			// Создаём кодер эталонной реализации
			if(nghttp2_hd_deflate_new(&deflater, TABLE_SIZE) != 0)
				// Выводим пустой поток октетов тела
				return result;
			// Формируем заголовки запроса с телом
			std::vector <field_t> fields;
			// Дописываем псевдо-заголовок метода запроса
			fields.emplace_back(":method", "POST");
			// Дописываем псевдо-заголовок схемы запроса
			fields.emplace_back(":scheme", "https");
			// Дописываем псевдо-заголовок пути запроса
			fields.emplace_back(":path", "/upload");
			// Дописываем псевдо-заголовок авторитета запроса
			fields.emplace_back(":authority", "www.example.com");
			// Дописываем преамбулу соединения клиента
			result = preface();
			// Резервируем память под поток целиком
			result.reserve(BODY_SIZE + ((BODY_SIZE / FRAME_SIZE) * 16) + 256);
			// Дописываем кадр заголовков запроса
			result += frame(0x01, 0x04, 1, deflate(deflater, fields));
			// Полезная нагрузка одного кадра данных
			const std::string portion(FRAME_SIZE, 'x');
			// Количество кадров данных потока
			const size_t total = (BODY_SIZE / FRAME_SIZE);
			/**
			 * Дописываем кадры данных: последний кадр завершает поток
			 */
			for(size_t i = 0; i < total; i++)
				// Дописываем очередной кадр данных
				result += frame(0x00, ((i + 1 == total) ? 0x01 : 0x00), 1, portion);
			// Удаляем кодер эталонной реализации
			nghttp2_hd_deflate_del(deflater);
		}
		// Выводим канонический поток октетов тела
		return result;
	}
	/**
	 * @brief Функция прогона кодирования блоков заголовков
	 *
	 * @note Функция объявлена шаблонной намеренно: сравниваемая реализация
	 *       подставляется по значению типа и вызовы её методов инлайнятся.
	 *       Виртуальный интерфейс добавил бы косвенный вызов на каждый блок
	 *
	 * @tparam Codec сравниваемый кодек: `restart()`, `prepare(наборы)`, `encode(номер)`
	 *
	 * @param codec  сравниваемый кодек
	 * @param output итоги прогона
	 * @return       результат прогона (false - блок закодирован с ошибкой)
	 *
	 */
	template <typename Codec>
	static bool encoding(Codec & codec, outcome_t & output) noexcept {
		// Наборы заголовков сценария
		const std::vector <std::vector <field_t>> sets = requests(SET_COUNT);
		// Выполняем сброс состояния кодека
		codec.restart();
		// Переводим наборы заголовков в представление сравниваемой реализации
		codec.prepare(sets);
		/**
		 * Прогреваем динамическую таблицу кодера: измерять установившийся режим
		 * осмысленнее, чем разовое кодирование по холодной таблице
		 */
		for(size_t i = 0; i < sets.size(); i++){
			// Если блок заголовков закодирован с ошибкой
			if(codec.encode(i) == 0)
				// Выводим отрицательный результат
				return false;
		}
		// Запоминаем момент начала измерения
		const auto start = std::chrono::steady_clock::now();
		/**
		 * Выполняем кодирование всех блоков заголовков сценария
		 */
		for(size_t i = 0; i < BLOCK_ROUNDS; i++)
			// Суммируем объём закодированного блока
			output.bytes += codec.encode(i % SET_COUNT);
		// Запоминаем момент окончания измерения
		const auto finish = std::chrono::steady_clock::now();
		/**
		 * Считаем объём заголовков до сжатия после замера: подсчёт нужен только
		 * для сведений о прогоне и к работе кодера отношения не имеет
		 */
		for(size_t i = 0; i < BLOCK_ROUNDS; i++)
			// Суммируем объём заголовков набора до сжатия
			output.original += length(sets[i % SET_COUNT]);
		// Устанавливаем количество выполненных операций
		output.operations = BLOCK_ROUNDS;
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
	 * @brief Функция прогона декодирования блоков заголовков
	 *
	 * @tparam Codec сравниваемый кодек: `restart()`, `decode(блок)`
	 *
	 * @param codec    сравниваемый кодек
	 * @param measured признак учёта выделений памяти
	 * @param output   итоги прогона
	 * @return         результат прогона (false - блок декодирован с ошибкой)
	 *
	 */
	template <typename Codec>
	static bool decoding(Codec & codec, const bool measured, outcome_t & output) noexcept {
		// Получаем канонический поток блоков заголовков
		const std::vector <std::string> & stream = blocks();
		// Если поток блоков заголовков не сформирован
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
		 * Выполняем декодирование всех блоков заголовков потока
		 */
		for(size_t i = 0; i < stream.size(); i++){
			// Если блок заголовков декодирован с ошибкой
			if(!codec.decode(stream[i])){
				// Снимаем статистику выделений памяти
				harvest(measured, output);
				// Выводим отрицательный результат
				return false;
			}
			// Суммируем объём декодированного блока
			output.bytes += stream[i].size();
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
	 * @brief Функция прогона измерения степени сжатия заголовков
	 *
	 * @details Сжатие измеряется на холодной динамической таблице: показатель
	 *          отражает установившийся режим, а установившийся режим наступает
	 *          через несколько десятков блоков и должен попасть в замер вместе
	 *          с дорогим первым блоком
	 *
	 * @tparam Codec сравниваемый кодек: `restart()`, `prepare(наборы)`, `encode(номер)`
	 *
	 * @param codec  сравниваемый кодек
	 * @param first  объём первого закодированного блока
	 * @param output итоги прогона
	 * @return       результат прогона (false - блок закодирован с ошибкой)
	 *
	 */
	template <typename Codec>
	static bool compression(Codec & codec, size_t & first, outcome_t & output) noexcept {
		// Наборы заголовков сценария
		const std::vector <std::vector <field_t>> sets = requests(RATIO_ROUNDS);
		// Выполняем сброс состояния кодека
		codec.restart();
		// Переводим наборы заголовков в представление сравниваемой реализации
		codec.prepare(sets);
		/**
		 * Выполняем кодирование всех блоков заголовков сценария
		 */
		for(size_t i = 0; i < sets.size(); i++){
			// Кодируем очередной блок заголовков
			const size_t size = codec.encode(i);
			// Если блок заголовков закодирован с ошибкой
			if(size == 0)
				// Выводим отрицательный результат
				return false;
			// Если закодирован первый блок - запоминаем его объём
			if(i == 0)
				// Запоминаем объём первого блока
				first = size;
			// Суммируем объём закодированного блока
			output.bytes += size;
			// Суммируем объём заголовков набора до сжатия
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
	 * @details Поток подаётся порциями размером с чтение из сокета: подача одним
	 *          куском переложила бы на разборщик буферизацию всего объёма
	 *
	 * @tparam Session сравниваемая реализация: `feed(данные, размер)`
	 *
	 * @param session  сравниваемая реализация
	 * @param stream   канонический поток октетов
	 * @param measured признак учёта выделений памяти
	 * @param output   итоги прогона
	 *
	 */
	template <typename Session>
	static void feeding(Session & session, const std::string & stream, const bool measured, outcome_t & output) noexcept {
		// Если требуется учёт выделений памяти
		if(measured)
			// Включаем учёт выделений памяти
			counting(true);
		// Запоминаем момент начала измерения
		const auto start = std::chrono::steady_clock::now();
		/**
		 * Подаём поток октетов порциями размером с чтение из сокета
		 */
		for(size_t offset = 0; offset < stream.size(); offset += CHUNK_SIZE)
			// Подаём очередную порцию потока на разбор
			session.feed(stream.data() + offset, ((stream.size() - offset) < CHUNK_SIZE ? (stream.size() - offset) : CHUNK_SIZE));
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
	 *          отражает мультиплексирование, при котором планировщик обслуживает
	 *          потоки вперемешку
	 *
	 * @tparam Pair сравниваемая пара: `open(набор)`, `pump()`, `completed()`
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
		 * Готовим наборы заголовков заранее: сборка набора - работа приложения,
		 * и в замер стоимости обмена попадать не должна
		 */
		const std::vector <std::vector <field_t>> sets = requests(SET_COUNT);
		// Переводим наборы заголовков в представление сравниваемой реализации
		pair.prepare(sets);
		// Запоминаем момент начала измерения
		const auto start = std::chrono::steady_clock::now();
		/**
		 * Выполняем обмены пачками по числу одновременных потоков
		 */
		for(size_t i = 0; i < ROUNDTRIP_ROUNDS; i += streams){
			// Вычисляем размер очередной пачки обменов
			const size_t portion = ((ROUNDTRIP_ROUNDS - i) < streams ? (ROUNDTRIP_ROUNDS - i) : streams);
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
		::printf("%-36s %14.2f   (%s)\n", name, value, units);
		// Выводим сведения о прогоне сценария
		::printf("%38s%s\n", "", details);
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
		::printf("%-36s %14s   (%s)\n", name, "—", reason);
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
	 * @details Контрольная сумма складывается из размеров разобранных заголовков
	 *          и суммы октетов тела: при одинаковой нагрузке и одинаковом объёме
	 *          работы потребителя она обязана совпасть у всех сравниваемых
	 *          реализаций. Расхождение означает, что какая-то из них разбирает
	 *          не то же самое - например, отдаёт часть заголовков в собственном
	 *          представлении или пропускает часть тела
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
			// Если задан фильтр названий выполняемых сценариев
			if(::strncmp(argv[i], "--filter=", 9) == 0)
				// Выводим фильтр названий сценариев
				return (argv[i] + 9);
		}
		// Выводим отсутствие фильтра названий сценариев
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
		 *       механизмом перехватчиков, который он для этого и предоставляет.
		 *       Перехватчик ловит только операторы языка: вызовы malloc из
		 *       реализаций на языке C на macOS через него не проходят
		 *
		 * @param ptr  указатель на выделенную память
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
 * @brief Оператор выделения памяти с учётом статистики
 *
 * @note Оператор подменяется на уровне программы, поэтому заголовочный файл
 *       подключается ровно одной единицей трансляции каждого стенда. Учёт
 *       ведётся только по операторам языка: реализации на языке C выделяют
 *       память через malloc, и для них общего механизма перехвата нет -
 *       такие реализации либо предоставляют подмену аллокатора, либо
 *       остаются без показателя выделений вовсе
 *
 * @param size размер выделяемой памяти
 * @return     указатель на выделенную память
 *
 */
void * operator new (size_t size){
	// Учитываем выполненное выделение памяти
	rival::note(size);
	// Выполняем выделение памяти
	void * result = ::malloc(size > 0 ? size : 1);
	// Если память не выделена
	if(result == nullptr)
		// Завершаем приложение - обработка нехватки памяти в стенде не предусмотрена
		::abort();
	// Выводим указатель на выделенную память
	return result;
}
/**
 * @brief Оператор выделения памяти под массив с учётом статистики
 *
 * @param size размер выделяемой памяти
 * @return     указатель на выделенную память
 *
 */
void * operator new [] (size_t size){
	// Выполняем выделение памяти
	return operator new (size);
}
/**
 * @brief Оператор освобождения памяти
 *
 * @param ptr указатель на освобождаемую память
 *
 */
void operator delete (void * ptr) noexcept {
	// Выполняем освобождение памяти
	::free(ptr);
}
/**
 * @brief Оператор освобождения памяти массива
 *
 * @param ptr указатель на освобождаемую память
 *
 */
void operator delete [] (void * ptr) noexcept {
	// Выполняем освобождение памяти
	::free(ptr);
}
/**
 * @brief Оператор освобождения памяти с указанием размера
 *
 * @param ptr указатель на освобождаемую память
 *
 */
void operator delete (void * ptr, size_t) noexcept {
	// Выполняем освобождение памяти
	::free(ptr);
}
/**
 * @brief Оператор освобождения памяти массива с указанием размера
 *
 * @param ptr указатель на освобождаемую память
 *
 */
void operator delete [] (void * ptr, size_t) noexcept {
	// Выполняем освобождение памяти
	::free(ptr);
}
#endif

#endif // __AWH_BENCHMARK_RIVAL_HTTP2__
