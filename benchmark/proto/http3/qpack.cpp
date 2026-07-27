/**
 * @file: qpack.cpp
 * @date: 2026-07-27
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @brief Сценарии измерения сжатия полей QPACK — скорость кодирования и декодирования
 *        секций, достигаемая степень сжатия и стоимость декодирования в выделениях памяти
 *
 * @copyright: Copyright © 2026
 *
 */

/**
 * Стандартные заголовочные файлы
 */
#include <chrono>
#include <string>
#include <vector>
#include <cstdio>

/**
 * Подключаем заголовочный файл бенчмарков протокола HTTP/3
 */
#include "http3.hpp"

/**
 * Используем стандартное пространство имён
 */
using namespace std;

/**
 * Подписываемся на пространство имён QPACK
 */
using namespace awh::http::h3::qpack;

/**
 * @brief Внутренние параметры и сценарии бенчмарков сжатия полей
 *
 */
namespace {
	/**
	 * @brief Количество секций полей сценариев кодирования и декодирования
	 *
	 */
	static constexpr size_t SECTION_ROUNDS = 200000;
	/**
	 * @brief Количество секций полей сценария измерения сжатия
	 *
	 * @details Степень сжатия зависит от прогретости динамической таблицы, поэтому
	 *          измерять её на одной секции бессмысленно: первая секция сжимается только
	 *          статической таблицей, а установившийся режим наступает через несколько
	 *          десятков секций
	 *
	 */
	static constexpr size_t RATIO_ROUNDS = 1000;
	/**
	 * @brief Ёмкость динамической таблицы сценариев
	 *
	 * @note Совпадает с размером таблицы HPACK по умолчанию: сравнение сжатия
	 *       с HTTP/2 осмысленно только при равных таблицах
	 *
	 */
	static constexpr uint64_t TABLE_CAPACITY = 4096;
	/**
	 * @brief Число потоков, которым разрешено ожидать пополнения таблицы
	 *
	 */
	static constexpr uint64_t BLOCKED_STREAMS = 16;
	/**
	 * @brief Порог скорости кодирования секций полей в секциях в секунду
	 *
	 * @details Пороги откалиброваны по отладочной сборке с четырёхкратным запасом:
	 *          они ловят регрессии на порядок, а не колебания окружения. Показательный
	 *          пример такой регрессии для кодера - переход от индекса статической
	 *          таблицы к её линейному перебору на каждом поле
	 *
	 */
	static constexpr double ENCODE_THRESHOLD = 26000.0;
	/**
	 * @brief Порог скорости кодирования секций полей без Huffman
	 *
	 * @details Путь без сжатия строк отдельный: строка копируется как есть, а выбор
	 *          представления не требует прохода по ней. Сценарий сторожит протекание
	 *          работы с Huffman-пути на путь без него
	 *
	 */
	static constexpr double ENCODE_PLAIN_THRESHOLD = 27000.0;
	/**
	 * @brief Порог скорости декодирования секций полей в секциях в секунду
	 *
	 * @details В отличие от HPACK декодер обязан разобрать ещё и поток инструкций
	 *          кодера, поэтому в замер входит обе работы: без инструкций секция
	 *          не разберётся вовсе
	 *
	 */
	static constexpr double DECODE_THRESHOLD = 95000.0;
	/**
	 * @brief Порог степени сжатия полей в процентах от исходного объёма
	 *
	 * @details Ограничение сверху: сжатие хуже порога означает, что динамическая
	 *          таблица перестала использоваться - например, из-за вытеснения записей
	 *          или отказа от инкрементальной индексации. Показатель от машины
	 *          и режима сборки не зависит, поэтому порог задан вплотную
	 *          к измеренному значению.
	 *          В отличие от HPACK в объём засчитываются и инструкции потока кодера:
	 *          изменения таблицы в QPACK передаются отдельно от секции, и не учесть
	 *          их значило бы объявить сжатие лучше, чем оно есть на проводе
	 *
	 */
	static constexpr double RATIO_THRESHOLD = 13.5;
	/**
	 * @brief Порог количества выделений памяти на декодированную секцию полей
	 *
	 * @details Декодер собирает строки в арену и отдаёт наружу представления, поэтому
	 *          в установившемся режиме выделений быть не должно вовсе: ёмкость арены
	 *          и списка представлений переиспользуется между секциями. Ненулевое
	 *          значение означает, что переиспользование сломано
	 *
	 */
	static constexpr double DECODE_ALLOCATIONS_THRESHOLD = 0.1;

	/**
	 * @brief Функция измерения скорости кодирования секций полей
	 *
	 * @param useHuffman признак сжатия строк по Huffman
	 * @return           результат измерения
	 *
	 */
	static awh::benchmark::result_t encode(const bool useHuffman) noexcept {
		// Результат измерения
		awh::benchmark::result_t result;
		// Создаём объект кодера полей
		encoder_t encoder;
		// Устанавливаем ёмкость динамической таблицы
		encoder.maxCapacity(TABLE_CAPACITY);
		// Устанавливаем число потоков, которым разрешено ожидать пополнения таблицы
		encoder.maxBlocked(BLOCKED_STREAMS);
		// Буфер закодированной секции полей
		string section;
		// Резервируем память под буфер секции
		section.reserve(4096);
		// Суммарный объём закодированных секций
		size_t compressed = 0;
		// Суммарный объём инструкций потока кодера
		size_t instructions = 0;
		// Суммарный объём полей до сжатия
		size_t original = 0;
		/**
		 * Готовим наборы полей заранее: сборка набора - это работа приложения,
		 * а не кодера, и в замер попадать не должна. Различных наборов ровно столько,
		 * сколько их порождает переменная часть запроса
		 */
		vector <vector <field_t>> sets;
		// Резервируем память под наборы полей
		sets.reserve(64);
		/**
		 * Выполняем формирование всех наборов полей
		 */
		for(size_t i = 0; i < 64; i++)
			// Дописываем очередной набор полей
			sets.push_back(awh::benchmark::http3::request(i));
		/**
		 * Прогреваем динамическую таблицу кодера: измерять установившийся режим
		 * осмысленнее, чем разовое кодирование по холодной таблице
		 */
		for(size_t i = 0; i < sets.size(); i++){
			// Очищаем буфер закодированной секции
			section.clear();
			// Кодируем секцию полей запроса
			encoder.encode(static_cast <uint64_t> (i * 4), sets[i], section, useHuffman);
			// Освобождаем выставленные кодером инструкции
			encoder.consumePending(encoder.pending().size());
			// Собираем подтверждение отправленной секции
			const string confirmation = awh::benchmark::http3::acknowledge(static_cast <uint64_t> (i * 4));
			// Количество разобранных октетов подтверждения
			size_t consumed = 0;
			// Код ошибки разбора подтверждения
			awh::http::h3::error_t error = awh::http::h3::error_t::H3_NO_ERROR;
			// Подаём подтверждение секции кодеру
			encoder.decodeDecoderStream(confirmation, consumed, error);
		}
		// Запоминаем момент начала измерения
		const auto start = std::chrono::steady_clock::now();
		/**
		 * Выполняем кодирование всех секций полей сценария
		 */
		for(size_t i = 0; i < SECTION_ROUNDS; i++){
			// Очищаем буфер закодированной секции
			section.clear();
			// Кодируем секцию полей запроса
			encoder.encode(static_cast <uint64_t> (i * 4), sets[i % sets.size()], section, useHuffman);
			// Суммируем объём закодированной секции
			compressed += section.size();
			// Суммируем объём выставленных кодером инструкций
			instructions += encoder.pending().size();
			// Освобождаем выставленные кодером инструкции
			encoder.consumePending(encoder.pending().size());
			/**
			 * Разбор подтверждений - работа самого кодера, а не декодера, и входит
			 * в замер по праву: на живом соединении кодер занят ею постоянно
			 */
			// Собираем подтверждение отправленной секции
			const string confirmation = awh::benchmark::http3::acknowledge(static_cast <uint64_t> (i * 4));
			// Количество разобранных октетов подтверждения
			size_t consumed = 0;
			// Код ошибки разбора подтверждения
			awh::http::h3::error_t error = awh::http::h3::error_t::H3_NO_ERROR;
			// Подаём подтверждение секции кодеру
			encoder.decodeDecoderStream(confirmation, consumed, error);
		}
		// Запоминаем момент окончания измерения
		const auto finish = std::chrono::steady_clock::now();
		/**
		 * Считаем объём полей до сжатия после замера: подсчёт нужен только
		 * для сведений о прогоне и к работе кодера отношения не имеет
		 */
		for(size_t i = 0; i < SECTION_ROUNDS; i++)
			// Суммируем объём полей набора до сжатия
			original += awh::benchmark::http3::length(sets[i % sets.size()]);
		// Вычисляем затраченное время
		const double seconds = std::chrono::duration <double> (finish - start).count();
		// Вычисляем скорость кодирования секций полей
		result.value = ((seconds > 0.0) ? (static_cast <double> (SECTION_ROUNDS) / seconds) : 0.0);
		// Буфер сведений о прогоне
		char details[256];
		// Формируем сведения о прогоне
		::snprintf(
			details, sizeof(details),
			"секций: %zu, до сжатия: %.1f МБ, после: %.1f МБ (%.1f%%), инструкций: %.1f МБ, на секцию: %.1f октетов",
			SECTION_ROUNDS,
			(static_cast <double> (original) / (1024.0 * 1024.0)),
			(static_cast <double> (compressed) / (1024.0 * 1024.0)),
			((original > 0) ? ((static_cast <double> (compressed + instructions) / static_cast <double> (original)) * 100.0) : 0.0),
			(static_cast <double> (instructions) / (1024.0 * 1024.0)),
			(static_cast <double> (compressed) / static_cast <double> (SECTION_ROUNDS))
		);
		// Устанавливаем сведения о прогоне
		result.details = details;
		// Выводим результат измерения
		return result;
	}
	/**
	 * @brief Функция измерения скорости декодирования секций полей
	 *
	 * @param counting признак измерения выделений памяти вместо скорости
	 * @return         результат измерения
	 *
	 */
	static awh::benchmark::result_t decode(const bool counting) noexcept {
		// Результат измерения
		awh::benchmark::result_t result;
		// Создаём объект кодера полей
		encoder_t encoder;
		// Устанавливаем ёмкость динамической таблицы кодера
		encoder.maxCapacity(TABLE_CAPACITY);
		// Устанавливаем число потоков, которым разрешено ожидать пополнения таблицы
		encoder.maxBlocked(BLOCKED_STREAMS);
		// Создаём объект декодера полей
		decoder_t decoder(TABLE_CAPACITY, BLOCKED_STREAMS);
		// Список закодированных секций полей
		vector <string> sections;
		// Резервируем память под список секций
		sections.reserve(SECTION_ROUNDS);
		// Список инструкций потока кодера, сопровождающих каждую секцию
		vector <string> deltas;
		// Резервируем память под список инструкций
		deltas.reserve(SECTION_ROUNDS);
		/**
		 * Готовим поток секций заранее: кодирование в замер попадать не должно,
		 * а состояние динамической таблицы обязано быть последовательным -
		 * декодер повторяет его секция за секцией
		 */
		for(size_t i = 0; i < SECTION_ROUNDS; i++){
			// Буфер закодированной секции полей
			string section;
			// Кодируем секцию полей запроса
			encoder.encode(static_cast <uint64_t> (i * 4), awh::benchmark::http3::request(i), section, true);
			// Дописываем инструкции, выставленные кодером в свой поток
			deltas.emplace_back(encoder.pending());
			// Освобождаем выставленные кодером инструкции
			encoder.consumePending(encoder.pending().size());
			// Собираем подтверждение отправленной секции
			const string confirmation = awh::benchmark::http3::acknowledge(static_cast <uint64_t> (i * 4));
			// Количество разобранных октетов подтверждения
			size_t used = 0;
			// Код ошибки разбора подтверждения
			awh::http::h3::error_t issue = awh::http::h3::error_t::H3_NO_ERROR;
			/**
			 * Подтверждаем секцию сразу: без подтверждений кодер отказывается
			 * от динамической таблицы, и замер пришёлся бы на путь с одними
			 * литералами - тот, которого на живом соединении не бывает
			 */
			encoder.decodeDecoderStream(confirmation, used, issue);
			// Дописываем закодированную секцию в список
			sections.push_back(section);
		}
		// Список декодированных полей
		vector <field_view_t> fields;
		// Код ошибки декодирования
		awh::http::h3::error_t error = awh::http::h3::error_t::H3_NO_ERROR;
		// Количество декодированных полей
		size_t headers = 0;
		// Если измеряются выделения памяти - включаем учёт
		if(counting)
			// Включаем учёт выделений памяти
			awh::benchmark::counting(true);
		// Запоминаем момент начала измерения
		const auto start = std::chrono::steady_clock::now();
		/**
		 * Выполняем декодирование всех секций полей сценария
		 *
		 * Инструкции потока кодера подаются перед секцией: без них секция ссылается
		 * на записи, которых в таблице декодера ещё нет, и поток блокируется
		 */
		for(size_t i = 0; i < SECTION_ROUNDS; i++){
			// Количество разобранных октетов инструкций
			size_t consumed = 0;
			// Если инструкции потока кодера есть
			if(!deltas[i].empty()){
				// Если разбор инструкций потока кодера не удался
				if(decoder.decodeEncoderStream(deltas[i], consumed, error) != awh::http::h3::status_t::OK){
					// Устанавливаем сведения о неудачном прогоне
					result.details = "прогон не выполнен: инструкция потока кодера не разобрана";
					// Выводим результат измерения
					return result;
				}
				// Освобождаем выставленные декодером подтверждения
				decoder.consumePending(decoder.pending().size());
			}
			// Если декодирование секции не удалось
			if(decoder.decode(static_cast <uint64_t> (i * 4), sections[i], fields, 0, error) != awh::http::h3::status_t::OK){
				// Устанавливаем сведения о неудачном прогоне
				result.details = "прогон не выполнен: секция полей не декодирована";
				// Выводим результат измерения
				return result;
			}
			// Освобождаем выставленные декодером подтверждения
			decoder.consumePending(decoder.pending().size());
			// Считаем декодированные поля
			headers += fields.size();
		}
		// Запоминаем момент окончания измерения
		const auto finish = std::chrono::steady_clock::now();
		// Количество выполненных выделений памяти
		size_t allocations = 0;
		// Суммарный объём выделенной памяти
		size_t bytes = 0;
		// Если измеряются выделения памяти
		if(counting){
			// Отключаем учёт выделений памяти
			awh::benchmark::counting(false);
			// Получаем статистику выделений памяти
			awh::benchmark::allocations(allocations, bytes);
		}
		// Вычисляем затраченное время
		const double seconds = std::chrono::duration <double> (finish - start).count();
		// Буфер сведений о прогоне
		char details[256];
		// Если измеряются выделения памяти
		if(counting){
			// Вычисляем количество выделений памяти на секцию полей
			result.value = (static_cast <double> (allocations) / static_cast <double> (SECTION_ROUNDS));
			// Формируем сведения о прогоне
			::snprintf(
				details, sizeof(details), "секций: %zu, выделений: %zu, выделено: %.1f МБ",
				SECTION_ROUNDS, allocations, (static_cast <double> (bytes) / (1024.0 * 1024.0))
			);
		// Если измеряется скорость декодирования
		} else {
			// Вычисляем скорость декодирования секций полей
			result.value = ((seconds > 0.0) ? (static_cast <double> (SECTION_ROUNDS) / seconds) : 0.0);
			// Формируем сведения о прогоне
			::snprintf(
				details, sizeof(details), "секций: %zu, полей: %zu (%.1f на секцию), полей/с: %.0f",
				SECTION_ROUNDS, headers, (static_cast <double> (headers) / static_cast <double> (SECTION_ROUNDS)),
				((seconds > 0.0) ? (static_cast <double> (headers) / seconds) : 0.0)
			);
		}
		// Устанавливаем сведения о прогоне
		result.details = details;
		// Выводим результат измерения
		return result;
	}
	/**
	 * @brief Функция измерения степени сжатия полей
	 *
	 * @return результат измерения
	 *
	 */
	static awh::benchmark::result_t compression() noexcept {
		// Результат измерения
		awh::benchmark::result_t result;
		// Создаём объект кодера полей
		encoder_t encoder;
		// Устанавливаем ёмкость динамической таблицы
		encoder.maxCapacity(TABLE_CAPACITY);
		// Устанавливаем число потоков, которым разрешено ожидать пополнения таблицы
		encoder.maxBlocked(BLOCKED_STREAMS);
		// Суммарный объём полей до сжатия
		size_t original = 0;
		// Суммарный объём закодированных секций
		size_t compressed = 0;
		// Суммарный объём инструкций потока кодера
		size_t instructions = 0;
		// Объём первой закодированной секции
		size_t first = 0;
		/**
		 * Выполняем кодирование всех секций полей сценария
		 */
		for(size_t i = 0; i < RATIO_ROUNDS; i++){
			// Формируем набор полей запроса
			const auto fields = awh::benchmark::http3::request(i);
			// Буфер закодированной секции полей
			string section;
			// Кодируем секцию полей запроса
			encoder.encode(static_cast <uint64_t> (i * 4), fields, section, true);
			// Суммируем объём полей до сжатия
			original += awh::benchmark::http3::length(fields);
			// Суммируем объём закодированной секции
			compressed += section.size();
			// Суммируем объём выставленных кодером инструкций
			instructions += encoder.pending().size();
			// Освобождаем выставленные кодером инструкции
			encoder.consumePending(encoder.pending().size());
			// Собираем подтверждение отправленной секции
			const string confirmation = awh::benchmark::http3::acknowledge(static_cast <uint64_t> (i * 4));
			// Количество разобранных октетов подтверждения
			size_t consumed = 0;
			// Код ошибки разбора подтверждения
			awh::http::h3::error_t issue = awh::http::h3::error_t::H3_NO_ERROR;
			// Подаём подтверждение секции кодеру
			encoder.decodeDecoderStream(confirmation, consumed, issue);
			// Если закодирована первая секция - запоминаем её объём
			if(i == 0)
				// Запоминаем объём первой секции
				first = section.size();
		}
		/**
		 * Степень сжатия считается по всему, что уходит на провод: и по секциям,
		 * и по инструкциям потока кодера. Учитывать одни секции значило бы объявить
		 * сжатие лучше, чем оно есть: изменения таблицы в QPACK передаются отдельно
		 */
		result.value = ((original > 0) ? ((static_cast <double> (compressed + instructions) / static_cast <double> (original)) * 100.0) : 100.0);
		// Буфер сведений о прогоне
		char details[256];
		// Формируем сведения о прогоне
		::snprintf(
			details, sizeof(details),
			"секций: %zu, до сжатия: %.1f октетов на секцию, после: %.1f, инструкций: %.1f, первая секция: %zu октетов",
			RATIO_ROUNDS,
			(static_cast <double> (original) / static_cast <double> (RATIO_ROUNDS)),
			(static_cast <double> (compressed) / static_cast <double> (RATIO_ROUNDS)),
			(static_cast <double> (instructions) / static_cast <double> (RATIO_ROUNDS)),
			first
		);
		// Устанавливаем сведения о прогоне
		result.details = details;
		// Выводим результат измерения
		return result;
	}

	// Регистрируем сценарий кодирования секций полей
	static const bool gEncode = awh::benchmark::add(
		"http3/qpack/encode", "секций/с", ENCODE_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, [](){ return ::encode(true); }
	);
	// Регистрируем сценарий кодирования секций полей без Huffman
	static const bool gEncodePlain = awh::benchmark::add(
		"http3/qpack/encode-plain", "секций/с", ENCODE_PLAIN_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, [](){ return ::encode(false); }
	);
	// Регистрируем сценарий декодирования секций полей
	static const bool gDecode = awh::benchmark::add(
		"http3/qpack/decode", "секций/с", DECODE_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, [](){ return ::decode(false); }
	);
	// Регистрируем сценарий степени сжатия полей
	static const bool gRatio = awh::benchmark::add(
		"http3/qpack/compression", "% от исходного", RATIO_THRESHOLD,
		awh::benchmark::bound_t::MAXIMUM, &::compression
	);
	// Регистрируем сценарий количества выделений памяти на декодированную секцию
	static const bool gAllocations = awh::benchmark::add(
		"http3/allocations/per-decoded-section", "выделений", DECODE_ALLOCATIONS_THRESHOLD,
		awh::benchmark::bound_t::MAXIMUM, [](){ return ::decode(true); }
	);
};
