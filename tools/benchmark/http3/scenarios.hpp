/**
 * @file: scenarios.hpp
 * @date: 2026-07-27
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @brief Общий набор сценариев эталонных стендов сравнения протокола HTTP/3 —
 *        порядок прогона, вычисление характеристик и вход в стенд
 *
 * @details Набор подключается каждым стендом последним и опирается на типы
 *          `Codec`, `Server` и `Pair`, объявленные стендом выше. Порядок
 *          сценариев, границы замера и вычисление характеристик обязаны
 *          совпадать у всех сравниваемых реализаций, а повторение этой
 *          логики в каждом стенде рано или поздно даёт расхождение
 *
 * @copyright: Copyright © 2026
 *
 */

#ifndef __AWH_BENCHMARK_RIVAL_HTTP3_SCENARIOS__
#define __AWH_BENCHMARK_RIVAL_HTTP3_SCENARIOS__

/**
 * Признак поддержки стендом сценариев уровня соединения
 *
 * @details Часть сравниваемых реализаций - это кодеки полей, а не реализации
 *          протокола: разбирать потоки соединения и вести обмен им нечем
 */
#ifndef RIVAL_SESSIONS
	#define RIVAL_SESSIONS 1
#endif

/**
 * Признак поддержки стендом учёта выделений памяти
 */
#ifndef RIVAL_ALLOCATIONS
	#define RIVAL_ALLOCATIONS 1
#endif

/**
 * @brief Внутренние сценарии эталонного стенда
 *
 */
namespace {
	/**
	 * @brief Функция выполнения сценария кодирования секций полей
	 *
	 * @param mask фильтр названий сценариев
	 *
	 */
	static void encodeScenario(const char * mask) noexcept {
		// Название выполняемого сценария
		static constexpr const char * NAME = "http3/qpack/encode";
		// Если название сценария не соответствует фильтру
		if(!rival::selected(NAME, mask))
			// Выходим без выполнения сценария
			return;
		// Итоги прогона сценария
		rival::outcome_t outcome;
		// Создаём объект сжатия полей сравниваемой реализации
		Codec codec;
		// Если секции полей закодированы с ошибкой
		if(!rival::encoding(codec, outcome)){
			// Выводим сообщение о невыполненном сценарии
			rival::skip(NAME, "прогон не выполнен: секция полей не закодирована");
			// Выходим из сценария
			return;
		}
		// Буфер сведений о прогоне
		char details[256];
		// Формируем сведения о прогоне
		::snprintf(
			details, sizeof(details),
			"секций: %zu, до сжатия: %.1f МБ, после: %.1f МБ (%.1f%%), инструкций: %.1f МБ, на секцию: %.1f октетов",
			outcome.operations,
			(static_cast <double> (outcome.original) / 1048576.0),
			(static_cast <double> (outcome.bytes) / 1048576.0),
			((outcome.original > 0) ? ((static_cast <double> (outcome.bytes + outcome.instructions) / static_cast <double> (outcome.original)) * 100.0) : 0.0),
			(static_cast <double> (outcome.instructions) / 1048576.0),
			(static_cast <double> (outcome.bytes) / static_cast <double> (outcome.operations))
		);
		// Выводим результат прогона сценария
		rival::report(NAME, "секций/с", rival::perSecond(outcome), details);
	}
	/**
	 * @brief Функция выполнения сценария декодирования секций полей
	 *
	 * @param measured признак учёта выделений памяти
	 * @param mask     фильтр названий сценариев
	 *
	 */
	static void decodeScenario(const bool measured, const char * mask) noexcept {
		// Название выполняемого сценария
		const char * name = (measured ? "http3/allocations/per-decoded-section" : "http3/qpack/decode");
		// Если название сценария не соответствует фильтру
		if(!rival::selected(name, mask))
			// Выходим без выполнения сценария
			return;
		/**
		 * Если стенд не поддерживает учёт выделений памяти
		 */
		#if !RIVAL_ALLOCATIONS
			// Если запрошен сценарий учёта выделений памяти
			if(measured){
				// Выводим сообщение о невыполненном сценарии
				rival::skip(name, RIVAL_ALLOCATIONS_REASON);
				// Выходим из сценария
				return;
			}
		#endif
		// Итоги прогона сценария
		rival::outcome_t outcome;
		// Создаём объект сжатия полей сравниваемой реализации
		Codec codec;
		// Если секции полей декодированы с ошибкой
		if(!rival::decoding(codec, measured, outcome)){
			// Выводим сообщение о невыполненном сценарии
			rival::skip(name, "прогон не выполнен: секция полей не декодирована");
			// Выходим из сценария
			return;
		}
		// Буфер сведений о прогоне
		char details[256];
		// Если измерялись выделения памяти
		if(measured){
			// Формируем сведения о прогоне
			::snprintf(
				details, sizeof(details), "секций: %zu, выделений: %zu, выделено: %.1f МБ",
				outcome.operations, outcome.allocations, (static_cast <double> (outcome.allocated) / 1048576.0)
			);
			// Выводим результат прогона сценария
			rival::report(name, "выделений", rival::allocated(outcome), details);
		// Если измерялась скорость декодирования
		} else {
			// Формируем сведения о прогоне
			::snprintf(
				details, sizeof(details), "секций: %zu, объём потока: %.1f МБ (%.1f октетов на секцию), инструкций: %.1f МБ, %.1f МБ/с",
				outcome.operations, (static_cast <double> (outcome.bytes) / 1048576.0),
				(static_cast <double> (outcome.bytes) / static_cast <double> (outcome.operations)),
				(static_cast <double> (outcome.instructions) / 1048576.0),
				rival::megabytes((outcome.bytes + outcome.instructions), outcome)
			);
			// Выводим результат прогона сценария
			rival::report(name, "секций/с", rival::perSecond(outcome), details);
		}
	}
	/**
	 * @brief Функция выполнения сценария измерения степени сжатия полей
	 *
	 * @param mask фильтр названий сценариев
	 *
	 */
	static void compressionScenario(const char * mask) noexcept {
		// Название выполняемого сценария
		static constexpr const char * NAME = "http3/qpack/compression";
		// Если название сценария не соответствует фильтру
		if(!rival::selected(NAME, mask))
			// Выходим без выполнения сценария
			return;
		// Итоги прогона сценария
		rival::outcome_t outcome;
		// Объём первой закодированной секции
		size_t first = 0;
		// Создаём объект сжатия полей сравниваемой реализации
		Codec codec;
		// Если секции полей закодированы с ошибкой
		if(!rival::compression(codec, first, outcome)){
			// Выводим сообщение о невыполненном сценарии
			rival::skip(NAME, "прогон не выполнен: секция полей не закодирована");
			// Выходим из сценария
			return;
		}
		// Буфер сведений о прогоне
		char details[256];
		// Формируем сведения о прогоне
		::snprintf(
			details, sizeof(details),
			"секций: %zu, до сжатия: %.1f октетов на секцию, после: %.1f, инструкций: %.1f, первая секция: %zu октетов",
			outcome.operations,
			(static_cast <double> (outcome.original) / static_cast <double> (outcome.operations)),
			(static_cast <double> (outcome.bytes) / static_cast <double> (outcome.operations)),
			(static_cast <double> (outcome.instructions) / static_cast <double> (outcome.operations)),
			first
		);
		/**
		 * Степень сжатия считается по всему, что уходит на провод: и по секциям,
		 * и по инструкциям потока кодера. В HPACK изменения таблицы едут внутри
		 * блока, а в QPACK - отдельным потоком, и не учесть их значило бы объявить
		 * сжатие лучше, чем оно есть
		 */
		rival::report(
			NAME, "% от исходного",
			((outcome.original > 0) ? ((static_cast <double> (outcome.bytes + outcome.instructions) / static_cast <double> (outcome.original)) * 100.0) : 100.0),
			details
		);
	}
	/**
	 * Если стенд поддерживает сценарии уровня соединения
	 */
	#if RIVAL_SESSIONS
		/**
		 * @brief Функция выполнения сценария разбора потока запросов
		 *
		 * @param measured признак учёта выделений памяти
		 * @param mask     фильтр названий сценариев
		 *
		 */
		static void requestScenario(const bool measured, const char * mask) noexcept {
			// Название выполняемого сценария
			const char * name = (measured ? "http3/allocations/per-request" : "http3/parse/request-stream");
			// Если название сценария не соответствует фильтру
			if(!rival::selected(name, mask))
				// Выходим без выполнения сценария
				return;
			/**
			 * Если стенд не поддерживает учёт выделений памяти
			 */
			#if !RIVAL_ALLOCATIONS
				// Если запрошен сценарий учёта выделений памяти
				if(measured){
					// Выводим сообщение о невыполненном сценарии
					rival::skip(name, RIVAL_ALLOCATIONS_REASON);
					// Выходим из сценария
					return;
				}
			#endif
			// Получаем канонический поток октетов запросов
			const std::vector <rival::piece_t> & stream = rival::requestStream();
			// Объём канонического потока октетов
			size_t volume = 0;
			/**
			 * Считаем объём канонического потока октетов
			 */
			for(const auto & piece : stream)
				// Суммируем объём порции потока
				volume += piece.data.size();
			// Итоги прогона сценария
			rival::outcome_t outcome;
			// Создаём объект разбора входящего потока сравниваемой реализацией
			Server server(true);
			// Выполняем подачу канонического потока октетов на разбор
			rival::feeding(server, stream, measured, outcome);
			// Устанавливаем количество разобранных запросов
			outcome.operations = server.handled();
			// Если разобраны не все запросы потока
			if(outcome.operations != rival::REQUEST_ROUNDS){
				// Буфер сведений о невыполненном сценарии
				char reason[128];
				// Формируем сведения о невыполненном сценарии
				::snprintf(reason, sizeof(reason), "прогон не выполнен: разобрано %zu запросов из %zu", outcome.operations, rival::REQUEST_ROUNDS);
				// Выводим сообщение о невыполненном сценарии
				rival::skip(name, reason);
				// Выходим из сценария
				return;
			}
			// Буфер сведений о прогоне
			char details[256];
			// Если измерялись выделения памяти
			if(measured){
				// Формируем сведения о прогоне
				::snprintf(
					details, sizeof(details), "запросов: %zu, выделений: %zu, выделено: %.1f МБ (%.1f октетов на запрос)",
					outcome.operations, outcome.allocations,
					(static_cast <double> (outcome.allocated) / 1048576.0),
					(static_cast <double> (outcome.allocated) / static_cast <double> (outcome.operations))
				);
				// Выводим результат прогона сценария
				rival::report(name, "выделений", rival::allocated(outcome), details);
			// Если измерялась скорость разбора потока запросов
			} else {
				// Формируем сведения о прогоне
				::snprintf(
					details, sizeof(details), "запросов: %zu, объём потока: %.1f МБ (%.1f октетов на запрос), %.1f МБ/с",
					outcome.operations, (static_cast <double> (volume) / 1048576.0),
					(static_cast <double> (volume) / static_cast <double> (outcome.operations)),
					rival::megabytes(volume, outcome)
				);
				// Выводим результат прогона сценария
				rival::report(name, "запросов/с", rival::perSecond(outcome), details);
			}
		}
		/**
		 * @brief Функция выполнения сценария приёма тела сообщения
		 *
		 * @param measured признак учёта выделений памяти
		 * @param mask     фильтр названий сценариев
		 *
		 */
		static void bodyScenario(const bool measured, const char * mask) noexcept {
			// Название выполняемого сценария
			const char * name = (measured ? "http3/allocations/per-data-frame" : "http3/parse/data-body");
			// Если название сценария не соответствует фильтру
			if(!rival::selected(name, mask))
				// Выходим без выполнения сценария
				return;
			/**
			 * Если стенд не поддерживает учёт выделений памяти
			 */
			#if !RIVAL_ALLOCATIONS
				// Если запрошен сценарий учёта выделений памяти
				if(measured){
					// Выводим сообщение о невыполненном сценарии
					rival::skip(name, RIVAL_ALLOCATIONS_REASON);
					// Выходим из сценария
					return;
				}
			#endif
			// Получаем канонический поток октетов тела
			const std::vector <rival::piece_t> & stream = rival::bodyStream();
			/**
			 * Выполняем прогревочный проход, результат которого отбрасывается.
			 *
			 * Остальные сценарии повторяют работу от пятидесяти до двухсот тысяч раз,
			 * и холодный старт в них тонет. Этот выполняет ровно один проход по телу,
			 * то есть без прогрева измеряет разгон реализации, а не её установившуюся
			 * скорость - и измеряет по-разному у разных реализаций. Замеренная цена
			 * этой ошибки: приём тела показывал 85-89 % эталона там, где установившийся
			 * режим даёт паритет
			 */
			{
				// Создаём отдельный объект разбора для прогревочного прохода
				Server warming(false);
				// Итоги прогревочного прохода, которые отбрасываются
				rival::outcome_t discarded;
				// Выполняем прогревочную подачу потока без учёта выделений памяти
				rival::feeding(warming, stream, false, discarded);
			}
			// Итоги прогона сценария
			rival::outcome_t outcome;
			// Создаём объект разбора входящего потока сравниваемой реализацией
			Server server(false);
			// Выполняем подачу канонического потока октетов на разбор
			rival::feeding(server, stream, measured, outcome);
			// Устанавливаем количество принятых кадров данных
			outcome.operations = (rival::BODY_SIZE / rival::FRAME_SIZE);
			// Если тело принято не полностью
			if(server.accepted() != rival::BODY_SIZE){
				// Буфер сведений о невыполненном сценарии
				char reason[128];
				// Формируем сведения о невыполненном сценарии
				::snprintf(reason, sizeof(reason), "прогон не выполнен: принято %zu октетов из %zu", server.accepted(), rival::BODY_SIZE);
				// Выводим сообщение о невыполненном сценарии
				rival::skip(name, reason);
				// Выходим из сценария
				return;
			}
			// Буфер сведений о прогоне
			char details[256];
			// Если измерялись выделения памяти
			if(measured){
				// Формируем сведения о прогоне
				::snprintf(
					details, sizeof(details), "кадров: %zu, выделений: %zu, выделено: %.1f МБ",
					outcome.operations, outcome.allocations, (static_cast <double> (outcome.allocated) / 1048576.0)
				);
				// Выводим результат прогона сценария
				rival::report(name, "выделений", rival::allocated(outcome), details);
			// Если измерялась скорость приёма тела
			} else {
				// Формируем сведения о прогоне
				::snprintf(
					details, sizeof(details), "кадров: %zu, принято: %.1f МБ, кадров/с: %.0f",
					outcome.operations, (static_cast <double> (rival::BODY_SIZE) / 1048576.0), rival::perSecond(outcome)
				);
				// Выводим результат прогона сценария
				rival::report(name, "МБ/с", rival::megabytes(rival::BODY_SIZE, outcome), details);
			}
		}
		/**
		 * @brief Функция выполнения сценария полного обмена
		 *
		 * @param streams количество одновременных потоков
		 * @param mask    фильтр названий сценариев
		 *
		 */
		static void exchangeScenario(const size_t streams, const char * mask) noexcept {
			// Название выполняемого сценария
			const char * name = ((streams > 1) ? "http3/session/multiplexed" : "http3/session/round-trip");
			// Если название сценария не соответствует фильтру
			if(!rival::selected(name, mask))
				// Выходим без выполнения сценария
				return;
			// Итоги прогона сценария
			rival::outcome_t outcome;
			// Создаём объект полного обмена парой сравниваемых реализаций
			Pair pair;
			// Если обмен выполнен с ошибкой
			if(!rival::exchanging(pair, streams, outcome)){
				// Буфер сведений о невыполненном сценарии
				char reason[128];
				// Формируем сведения о невыполненном сценарии
				::snprintf(reason, sizeof(reason), "прогон не выполнен: завершено %zu обменов из %zu", outcome.operations, rival::ROUNDTRIP_ROUNDS);
				// Выводим сообщение о невыполненном сценарии
				rival::skip(name, reason);
				// Выходим из сценария
				return;
			}
			// Буфер сведений о прогоне
			char details[256];
			// Формируем сведения о прогоне
			::snprintf(
				details, sizeof(details), "обменов: %zu, одновременных потоков: %zu",
				outcome.operations, streams
			);
			// Выводим результат прогона сценария
			rival::report(name, "обменов/с", rival::perSecond(outcome), details);
		}
	#endif
};

/**
 * @brief Функция входа в стенд
 *
 * @param argc длина массива параметров
 * @param argv массив параметров
 * @return     код выхода из стенда
 *
 */
int32_t main(int32_t argc, char ** argv) noexcept {
	// Получаем фильтр названий выполняемых сценариев
	const char * mask = rival::filter(argc, argv);
	// Устанавливаем перехватчик выделений памяти аллокатора
	rival::attach();
	// Выполняем сценарий кодирования секций полей
	::encodeScenario(mask);
	// Выполняем сценарий декодирования секций полей
	::decodeScenario(false, mask);
	// Выполняем сценарий измерения степени сжатия полей
	::compressionScenario(mask);
	// Выполняем сценарий количества выделений памяти на декодированную секцию
	::decodeScenario(true, mask);
	/**
	 * Если стенд поддерживает сценарии уровня соединения
	 */
	#if RIVAL_SESSIONS
		// Выполняем сценарий разбора потока запросов
		::requestScenario(false, mask);
		// Выполняем сценарий приёма тела сообщения
		::bodyScenario(false, mask);
		// Выполняем сценарий полного обмена по одному потоку
		::exchangeScenario(1, mask);
		// Выполняем сценарий полного обмена по множеству потоков
		::exchangeScenario(rival::STREAM_COUNT, mask);
		// Выполняем сценарий количества выделений памяти на разобранный запрос
		::requestScenario(true, mask);
		// Выполняем сценарий количества выделений памяти на кадр данных
		::bodyScenario(true, mask);
	/**
	 * Если стенд не поддерживает сценарии уровня соединения
	 */
	#else
		/**
		 * Выполняем перебор всех сценариев уровня соединения
		 */
		for(const char * name : {
			"http3/parse/request-stream", "http3/parse/data-body",
			"http3/session/round-trip", "http3/session/multiplexed",
			"http3/allocations/per-request", "http3/allocations/per-data-frame"
		}){
			// Если название сценария соответствует фильтру
			if(rival::selected(name, mask))
				// Выводим сообщение о невыполненном сценарии
				rival::skip(name, RIVAL_SESSIONS_REASON);
		}
	#endif
	// Выводим контрольную сумму обработанных данных
	rival::digest(argc, argv);
	// Выводим успешный код выхода
	return 0;
}

#endif // __AWH_BENCHMARK_RIVAL_HTTP3_SCENARIOS__
