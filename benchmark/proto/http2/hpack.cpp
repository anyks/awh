/**
 * @file hpack.cpp
 * @date 2026-07-26
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
 * @brief Сценарии измерения сжатия заголовков HPACK — скорость кодирования и
 *        декодирования блоков, достигаемая степень сжатия и стоимость декодирования
 *        в выделениях памяти
 *
 * @copyright Copyright © 2026
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
 * Подключаем заголовочный файл бенчмарков протокола HTTP/2
 */
#include "http2.hpp"

/**
 * Используем стандартное пространство имён
 */
using namespace std;

/**
 * Подписываемся на пространство имён HPACK
 */
using namespace awh::http::h2::hpack;

/**
 * @brief Внутренние параметры и сценарии бенчмарков сжатия заголовков
 *
 */
namespace {
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
	 * @brief Порог скорости кодирования блоков заголовков в блоках в секунду
	 *
	 * @details Пороги откалиброваны по отладочной сборке с четырёхкратным запасом:
	 *          они ловят регрессии на порядок, а не колебания окружения. Показательный
	 *          пример такой регрессии для кодера - переход от индекса статической
	 *          таблицы к её линейному перебору на каждом заголовке
	 *
	 */
	static constexpr double ENCODE_THRESHOLD = 44000.0;
	/**
	 * @brief Порог скорости кодирования блоков заголовков без Huffman
	 *
	 * @details Путь без сжатия строк отдельный: строка копируется как есть, а выбор
	 *          представления не требует прохода по ней. До появления сценария этот
	 *          путь не был покрыт вовсе, хотя доступен приложению параметром encode
	 *          и используется тестами.
	 *          Сценарий сторожит протекание работы с Huffman-пути на путь без него.
	 *          Такое уже случалось: длина строки после сжатия вычислялась до проверки
	 *          флага, то есть полным проходом по каждой строке впустую. Заметим, что
	 *          та регрессия стоила около 1.5% и порогом бы не поймалась - порог ловит
	 *          обвал, а мелкое протекание видно только сравнением прогонов
	 *
	 */
	static constexpr double ENCODE_PLAIN_THRESHOLD = 57000.0;
	/**
	 * @brief Порог скорости декодирования блоков заголовков в блоках в секунду
	 *
	 */
	static constexpr double DECODE_THRESHOLD = 135000.0;
	/**
	 * @brief Порог степени сжатия заголовков в процентах от исходного объёма
	 *
	 * @details Ограничение сверху: сжатие хуже порога означает, что динамическая
	 *          таблица перестала использоваться - например, из-за вытеснения записей
	 *          или отказа от инкрементальной индексации. Показатель от машины
	 *          и режима сборки не зависит, поэтому порог задан вплотную
	 *          к измеренному значению.
	 *          Порог обязан ловить отказ адаптивной индексации: без неё таблица
	 *          заполняется разовыми значениями, вытесняет повторяющиеся заголовки,
	 *          и сжатие возвращается к 14.9% - именно этот случай порог и отсекает
	 *
	 */
	static constexpr double RATIO_THRESHOLD = 13.0;
	/**
	 * @brief Порог количества выделений памяти на декодированный блок заголовков
	 *
	 * @details Декодер собирает строки в арену и отдаёт наружу представления, поэтому
	 *          в установившемся режиме выделений быть не должно вовсе: ёмкость арены
	 *          и списка представлений переиспользуется между блоками. Ненулевое
	 *          значение означает, что переиспользование сломано
	 *
	 */
	static constexpr double DECODE_ALLOCATIONS_THRESHOLD = 0.1;

	/**
	 * @brief Функция измерения скорости кодирования блоков заголовков
	 *
	 * @return результат измерения
	 *
	 */
	static awh::benchmark::result_t encode(const bool useHuffman) noexcept {
		// Результат измерения
		awh::benchmark::result_t result;
		// Создаём объект кодера заголовков
		encoder_t encoder;
		// Буфер закодированного блока заголовков
		string block;
		// Резервируем память под буфер блока
		block.reserve(4096);
		// Суммарный объём закодированных блоков
		size_t compressed = 0;
		// Суммарный объём заголовков до сжатия
		size_t original = 0;
		/**
		 * Готовим наборы заголовков заранее: сборка набора - это работа приложения,
		 * а не кодера, и в замер попадать не должна. Различных наборов ровно столько,
		 * сколько их порождает переменная часть запроса
		 */
		vector <vector <field_t>> sets;
		// Резервируем память под наборы заголовков
		sets.reserve(64);
		/**
		 * Выполняем формирование всех наборов заголовков
		 */
		for(size_t i = 0; i < 64; i++)
			// Дописываем очередной набор заголовков
			sets.push_back(awh::benchmark::http2::request(i));
		/**
		 * Прогреваем динамическую таблицу кодера: измерять установившийся режим
		 * осмысленнее, чем разовое кодирование по холодной таблице
		 */
		for(size_t i = 0; i < sets.size(); i++){
			// Очищаем буфер закодированного блока
			block.clear();
			// Кодируем блок заголовков запроса
			encoder.encode(sets[i], block, useHuffman);
		}
		// Запоминаем момент начала измерения
		const auto start = std::chrono::steady_clock::now();
		/**
		 * Выполняем кодирование всех блоков заголовков сценария
		 */
		for(size_t i = 0; i < BLOCK_ROUNDS; i++){
			// Очищаем буфер закодированного блока
			block.clear();
			// Кодируем блок заголовков запроса
			encoder.encode(sets[i % sets.size()], block, useHuffman);
			// Суммируем объём закодированного блока
			compressed += block.size();
		}
		// Запоминаем момент окончания измерения
		const auto finish = std::chrono::steady_clock::now();
		/**
		 * Считаем объём заголовков до сжатия после замера: подсчёт нужен только
		 * для сведений о прогоне и к работе кодера отношения не имеет
		 */
		for(size_t i = 0; i < BLOCK_ROUNDS; i++)
			// Суммируем объём заголовков набора до сжатия
			original += awh::benchmark::http2::length(sets[i % sets.size()]);
		// Вычисляем затраченное время
		const double seconds = std::chrono::duration <double> (finish - start).count();
		// Вычисляем скорость кодирования блоков заголовков
		result.value = ((seconds > 0.0) ? (static_cast <double> (BLOCK_ROUNDS) / seconds) : 0.0);
		// Буфер сведений о прогоне
		char details[256];
		// Формируем сведения о прогоне
		::snprintf(
			details, sizeof(details),
			"блоков: %zu, до сжатия: %.1f МБ, после: %.1f МБ (%.1f%%), на блок: %.1f октетов",
			BLOCK_ROUNDS,
			(static_cast <double> (original) / (1024.0 * 1024.0)),
			(static_cast <double> (compressed) / (1024.0 * 1024.0)),
			((original > 0) ? ((static_cast <double> (compressed) / static_cast <double> (original)) * 100.0) : 0.0),
			(static_cast <double> (compressed) / static_cast <double> (BLOCK_ROUNDS))
		);
		// Устанавливаем сведения о прогоне
		result.details = details;
		// Выводим результат измерения
		return result;
	}
	/**
	 * @brief Функция измерения скорости декодирования блоков заголовков
	 *
	 * @param counting признак измерения выделений памяти вместо скорости
	 * @return         результат измерения
	 *
	 */
	static awh::benchmark::result_t decode(const bool counting) noexcept {
		// Результат измерения
		awh::benchmark::result_t result;
		// Создаём объект кодера заголовков
		encoder_t encoder;
		// Создаём объект декодера заголовков
		decoder_t decoder;
		// Список закодированных блоков заголовков
		vector <string> blocks;
		// Резервируем память под список блоков
		blocks.reserve(BLOCK_ROUNDS);
		/**
		 * Готовим поток блоков заранее: кодирование в замер попадать не должно,
		 * а состояние динамической таблицы обязано быть последовательным -
		 * декодер повторяет его блок за блоком
		 */
		for(size_t i = 0; i < BLOCK_ROUNDS; i++){
			// Буфер закодированного блока заголовков
			string block;
			// Кодируем блок заголовков запроса
			encoder.encode(awh::benchmark::http2::request(i), block, true);
			// Дописываем закодированный блок в список
			blocks.push_back(block);
		}
		// Список декодированных заголовков
		vector <field_view_t> fields;
		// Код ошибки декодирования
		awh::http::h2::error_t error = awh::http::h2::error_t::NO_ERROR;
		// Количество декодированных заголовков
		size_t headers = 0;
		// Если измеряются выделения памяти - включаем учёт
		if(counting)
			// Включаем учёт выделений памяти
			awh::benchmark::counting(true);
		// Запоминаем момент начала измерения
		const auto start = std::chrono::steady_clock::now();
		/**
		 * Выполняем декодирование всех блоков заголовков сценария
		 */
		for(size_t i = 0; i < BLOCK_ROUNDS; i++){
			// Если декодирование блока не удалось
			if(decoder.decode(blocks[i], fields, 0, error) != awh::http::h2::status_t::OK){
				// Устанавливаем сведения о неудачном прогоне
				result.details = "прогон не выполнен: блок заголовков не декодирован";
				// Выводим результат измерения
				return result;
			}
			// Считаем декодированные заголовки
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
			// Вычисляем количество выделений памяти на блок заголовков
			result.value = (static_cast <double> (allocations) / static_cast <double> (BLOCK_ROUNDS));
			// Формируем сведения о прогоне
			::snprintf(
				details, sizeof(details), "блоков: %zu, выделений: %zu, выделено: %.1f МБ",
				BLOCK_ROUNDS, allocations, (static_cast <double> (bytes) / (1024.0 * 1024.0))
			);
		// Если измеряется скорость декодирования
		} else {
			// Вычисляем скорость декодирования блоков заголовков
			result.value = ((seconds > 0.0) ? (static_cast <double> (BLOCK_ROUNDS) / seconds) : 0.0);
			// Формируем сведения о прогоне
			::snprintf(
				details, sizeof(details), "блоков: %zu, заголовков: %zu (%.1f на блок), заголовков/с: %.0f",
				BLOCK_ROUNDS, headers, (static_cast <double> (headers) / static_cast <double> (BLOCK_ROUNDS)),
				((seconds > 0.0) ? (static_cast <double> (headers) / seconds) : 0.0)
			);
		}
		// Устанавливаем сведения о прогоне
		result.details = details;
		// Выводим результат измерения
		return result;
	}
	/**
	 * @brief Функция измерения степени сжатия заголовков
	 *
	 * @return результат измерения
	 *
	 */
	static awh::benchmark::result_t compression() noexcept {
		// Результат измерения
		awh::benchmark::result_t result;
		// Создаём объект кодера заголовков
		encoder_t encoder;
		// Суммарный объём заголовков до сжатия
		size_t original = 0;
		// Суммарный объём закодированных блоков
		size_t compressed = 0;
		// Объём первого закодированного блока
		size_t first = 0;
		/**
		 * Выполняем кодирование всех блоков заголовков сценария
		 */
		for(size_t i = 0; i < RATIO_ROUNDS; i++){
			// Формируем набор заголовков запроса
			const auto fields = awh::benchmark::http2::request(i);
			// Буфер закодированного блока заголовков
			string block;
			// Кодируем блок заголовков запроса
			encoder.encode(fields, block, true);
			// Суммируем объём заголовков до сжатия
			original += awh::benchmark::http2::length(fields);
			// Суммируем объём закодированного блока
			compressed += block.size();
			// Если закодирован первый блок - запоминаем его объём
			if(i == 0)
				// Запоминаем объём первого блока
				first = block.size();
		}
		// Вычисляем степень сжатия в процентах от исходного объёма
		result.value = ((original > 0) ? ((static_cast <double> (compressed) / static_cast <double> (original)) * 100.0) : 100.0);
		// Буфер сведений о прогоне
		char details[256];
		// Формируем сведения о прогоне
		::snprintf(
			details, sizeof(details),
			"блоков: %zu, до сжатия: %.1f октетов на блок, после: %.1f, первый блок: %zu октетов",
			RATIO_ROUNDS,
			(static_cast <double> (original) / static_cast <double> (RATIO_ROUNDS)),
			(static_cast <double> (compressed) / static_cast <double> (RATIO_ROUNDS)),
			first
		);
		// Устанавливаем сведения о прогоне
		result.details = details;
		// Выводим результат измерения
		return result;
	}

	// Регистрируем сценарий кодирования блоков заголовков
	static const bool gEncode = awh::benchmark::add(
		"http2/hpack/encode", "блоков/с", ENCODE_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, [](){ return ::encode(true); }
	);
	// Регистрируем сценарий кодирования блоков заголовков без Huffman
	static const bool gEncodePlain = awh::benchmark::add(
		"http2/hpack/encode-plain", "блоков/с", ENCODE_PLAIN_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, [](){ return ::encode(false); }
	);
	// Регистрируем сценарий декодирования блоков заголовков
	static const bool gDecode = awh::benchmark::add(
		"http2/hpack/decode", "блоков/с", DECODE_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, [](){ return ::decode(false); }
	);
	// Регистрируем сценарий степени сжатия заголовков
	static const bool gRatio = awh::benchmark::add(
		"http2/hpack/compression", "% от исходного", RATIO_THRESHOLD,
		awh::benchmark::bound_t::MAXIMUM, &::compression
	);
	// Регистрируем сценарий количества выделений памяти на декодированный блок
	static const bool gAllocations = awh::benchmark::add(
		"http2/allocations/per-decoded-block", "выделений", DECODE_ALLOCATIONS_THRESHOLD,
		awh::benchmark::bound_t::MAXIMUM, [](){ return ::decode(true); }
	);
};
