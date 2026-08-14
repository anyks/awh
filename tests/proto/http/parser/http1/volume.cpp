/**
 * @file volume.cpp
 * @date 2026-07-28
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
 * @brief Тесты подмодуля парсера HTTP/1.x на больших объёмах данных, обрывах
 *        соединения и числовых границах кадрирования
 *
 * @details Свойства, не проявляющиеся на коротких сообщениях: доставка тела до
 *          октета, поведение при обрыве посреди тела и накопление объявленных
 *          чисел без переполнения. Тело не хранится массивом ни на одной стороне -
 *          оно порождается и сверяется позиционной функцией, иначе тест измерял бы
 *          собственное потребление памяти
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Подключаем заголовочный файл тестов
 */
#include "http1.hpp"

/**
 * Подписываемся на пространство имён HTTP-протокола
 */
using namespace awh::http;

/**
 * @brief Внутреннее окружение тестов больших объёмов
 *
 */
namespace {
	// Объём тела проверяемых сообщений
	static constexpr size_t VOLUME = (16 * 1024 * 1024);
	// Размер порции подачи данных приёмнику
	static constexpr size_t FEED = (64 * 1024);
	/**
	 * @brief Функция получения октета тела по его позиции
	 *
	 * @details Тело порождается позиционной функцией, а не хранится: сверка
	 *          многомегабайтного тела массивом измеряла бы память самого теста
	 *
	 * @param position позиция октета в теле
	 * @return         октет тела
	 *
	 */
	static inline uint8_t octet(const uint64_t position) noexcept {
		// Выводим октет тела, зависящий от позиции нелинейно
		return static_cast <uint8_t> ((position * 31u) ^ (position >> 7));
	}
}

/**
 * @brief Метод тестирования доставки тела фиксированного размера до октета
 *
 * @details Тело в шестнадцать мебибайт приходит сотнями порций, каждая из которых
 *          пересекает границы внутренних буферов парсера. Сверяется не только объём,
 *          но и содержимое: потеря или перестановка порции даёт тот же объём
 *
 */
TEST_F(ParserFixture, VolumeIdentityBodyTest){
	// Создаём объект парсера-приёмника ответа
	auto parser = this->make(direct_t::RESPONSE);
	// Формируем лимиты безопасности с поднятым пределом размера тела
	parser_http_t::limits_t limits;
	// Поднимаем предел размера тела до проверяемого объёма
	limits.maxBodySize = (VOLUME + 1);
	// Применяем лимиты безопасности разбора
	parser->limits(limits);
	// Устанавливаем метод запроса, которому соответствует ожидаемый ответ
	parser->method(method_t::GET);
	// Позиция сверки принятого тела
	uint64_t position = 0;
	// Количество расхождений содержимого тела
	uint64_t mismatches = 0;
	// Устанавливаем функцию обратного вызова обработки фрагмента тела сообщения
	parser->on(parser_http_t::data_callback_t([&position, &mismatches](const uint32_t, const void * buffer, const size_t size, const bool) noexcept -> bool {
		// Получаем указатель на принятые октеты тела
		const uint8_t * data = static_cast <const uint8_t *> (buffer);
		/**
		 * Сверяем принятые октеты тела с ожидаемыми
		 */
		for(size_t i = 0; i < size; i++){
			// Если принятый октет не совпал с ожидаемым
			if(data[i] != ::octet(position + i))
				// Считаем расхождение содержимого тела
				mismatches++;
		}
		// Сдвигаем позицию сверки принятого тела
		position += size;
		// Продолжаем разбор
		return true;
	}));
	// Формируем блок заголовков ответа
	const std::string head = ("HTTP/1.1 200 OK\r\nContent-Length: " + std::to_string(VOLUME) + "\r\n\r\n");
	// Подаём приёмнику блок заголовков ответа
	parser->parse(head.data(), head.size());
	// Формируем буфер подачи тела
	std::vector <uint8_t> feed(FEED);
	// Позиция подачи тела
	size_t sent = 0;
	/**
	 * Подаём тело сообщения порциями
	 */
	while(sent < VOLUME){
		// Определяем размер очередной порции подачи
		const size_t size = std::min(FEED, (VOLUME - sent));
		/**
		 * Заполняем буфер подачи очередной порцией тела
		 */
		for(size_t i = 0; i < size; i++)
			// Формируем очередной октет порции тела
			feed[i] = ::octet(sent + i);
		// Подаём приёмнику очередную порцию тела
		parser->parse(feed.data(), size);
		// Сдвигаем позицию подачи тела
		sent += size;
	}
	// Проверяем что сообщение разобрано целиком
	ASSERT_EQ(parser->status(), parser_t::status_t::COMPLETE);
	// Проверяем что тело доставлено целиком
	ASSERT_EQ(position, static_cast <uint64_t> (VOLUME));
	// Проверяем что содержимое тела доставлено до октета
	ASSERT_EQ(mismatches, 0u);
}

/**
 * @brief Метод тестирования доставки тела в кодировке chunked до октета
 *
 * @details Кадрирование chunked добавляет к телу строки размера, и ошибка учёта
 *          в них проявляется смещением содержимого, а не изменением объёма
 *
 */
TEST_F(ParserFixture, VolumeChunkedBodyTest){
	// Размер одного чанка тела
	static constexpr size_t CHUNK = (16 * 1024);
	// Создаём объект парсера-приёмника ответа
	auto parser = this->make(direct_t::RESPONSE);
	// Формируем лимиты безопасности с поднятыми пределами
	parser_http_t::limits_t limits;
	// Поднимаем предел размера тела до проверяемого объёма
	limits.maxBodySize = (VOLUME + 1);
	// Поднимаем предел размера чанка
	limits.maxChunkSize = (CHUNK + 1);
	// Применяем лимиты безопасности разбора
	parser->limits(limits);
	// Устанавливаем метод запроса, которому соответствует ожидаемый ответ
	parser->method(method_t::GET);
	// Позиция сверки принятого тела
	uint64_t position = 0;
	// Количество расхождений содержимого тела
	uint64_t mismatches = 0;
	// Устанавливаем функцию обратного вызова обработки фрагмента тела сообщения
	parser->on(parser_http_t::data_callback_t([&position, &mismatches](const uint32_t, const void * buffer, const size_t size, const bool) noexcept -> bool {
		// Получаем указатель на принятые октеты тела
		const uint8_t * data = static_cast <const uint8_t *> (buffer);
		/**
		 * Сверяем принятые октеты тела с ожидаемыми
		 */
		for(size_t i = 0; i < size; i++){
			// Если принятый октет не совпал с ожидаемым
			if(data[i] != ::octet(position + i))
				// Считаем расхождение содержимого тела
				mismatches++;
		}
		// Сдвигаем позицию сверки принятого тела
		position += size;
		// Продолжаем разбор
		return true;
	}));
	// Формируем блок заголовков ответа
	const std::string head = "HTTP/1.1 200 OK\r\nTransfer-Encoding: chunked\r\n\r\n";
	// Подаём приёмнику блок заголовков ответа
	parser->parse(head.data(), head.size());
	// Формируем буфер подачи чанка
	std::vector <uint8_t> feed(CHUNK);
	// Позиция подачи тела
	size_t sent = 0;
	// Текстовый буфер строки размера чанка
	char line[32];
	/**
	 * Подаём тело сообщения чанками
	 */
	while(sent < VOLUME){
		// Определяем размер очередного чанка тела
		const size_t size = std::min(CHUNK, (VOLUME - sent));
		// Формируем строку размера чанка
		const int32_t length = ::snprintf(line, sizeof(line), "%zX\r\n", size);
		// Подаём приёмнику строку размера чанка
		parser->parse(line, static_cast <size_t> (length));
		/**
		 * Заполняем буфер подачи очередным чанком тела
		 */
		for(size_t i = 0; i < size; i++)
			// Формируем очередной октет чанка тела
			feed[i] = ::octet(sent + i);
		// Подаём приёмнику данные чанка
		parser->parse(feed.data(), size);
		// Подаём приёмнику окончание данных чанка
		parser->parse("\r\n", 2);
		// Сдвигаем позицию подачи тела
		sent += size;
	}
	// Подаём приёмнику завершающий нулевой чанк
	parser->parse("0\r\n\r\n", 5);
	// Проверяем что сообщение разобрано целиком
	ASSERT_EQ(parser->status(), parser_t::status_t::COMPLETE);
	// Проверяем что тело доставлено целиком
	ASSERT_EQ(position, static_cast <uint64_t> (VOLUME));
	// Проверяем что содержимое тела доставлено до октета
	ASSERT_EQ(mismatches, 0u);
}

/**
 * @brief Метод тестирования сборки тела большого объёма обоими способами подачи
 *
 * @details Собранные байты не накапливаются, а разбираются встречным приёмником на
 *          лету: он же и сверяет тело до октета. Потребление памяти отправителя
 *          ограничено порогами выходного буфера и от объёма тела не зависит
 *
 */
TEST_F(ParserFixture, VolumeSendRoundtripTest){
	/**
	 * @brief Функция сборки и встречного разбора тела большого объёма
	 *
	 * @param source  признак подачи тела pull-источником вместо sendData
	 * @param chunked признак кадрирования тела методом chunked
	 *
	 */
	auto roundtrip = [this](const bool source, const bool chunked) noexcept -> void {
		// Создаём объект парсера-отправителя ответа
		auto sender = this->make(direct_t::RESPONSE);
		// Создаём объект парсера-приёмника собираемого ответа
		auto receiver = this->make(direct_t::RESPONSE);
		// Формируем лимиты безопасности с поднятыми пределами
		parser_http_t::limits_t limits;
		// Поднимаем предел размера тела до проверяемого объёма
		limits.maxBodySize = (VOLUME + 1);
		// Поднимаем предел размера чанка
		limits.maxChunkSize = (VOLUME + 1);
		// Применяем лимиты безопасности разбора
		receiver->limits(limits);
		// Устанавливаем метод запроса, которому соответствует ожидаемый ответ
		receiver->method(method_t::GET);
		// Позиция сверки принятого тела
		uint64_t position = 0;
		// Количество расхождений содержимого тела
		uint64_t mismatches = 0;
		// Устанавливаем функцию обратного вызова обработки фрагмента тела сообщения
		receiver->on(parser_http_t::data_callback_t([&position, &mismatches](const uint32_t, const void * buffer, const size_t size, const bool) noexcept -> bool {
			// Получаем указатель на принятые октеты тела
			const uint8_t * data = static_cast <const uint8_t *> (buffer);
			/**
			 * Сверяем принятые октеты тела с ожидаемыми
			 */
			for(size_t i = 0; i < size; i++){
				// Если принятый октет не совпал с ожидаемым
				if(data[i] != ::octet(position + i))
					// Считаем расхождение содержимого тела
					mismatches++;
			}
			// Сдвигаем позицию сверки принятого тела
			position += size;
			// Продолжаем разбор
			return true;
		}));
		// Получаем указатель на объект приёмника для функции записи
		parser_http_t * target = receiver.get();
		// Устанавливаем функцию обратного вызова записи исходящих байтов в сеть
		sender->on(parser_http_t::write_callback_t([target](const void * buffer, const size_t size) noexcept {
			// Подаём отданные байты встречному приёмнику
			target->parse(buffer, size);
		}));
		// Понижаем пороги выходного буфера: потребление памяти обязано ими ограничиваться
		sender->sendWaterMarks(256 * 1024, 64 * 1024);
		// Формируем контейнер заголовков ответа с провайдером
		headers_t response(std::make_unique <response_t> (version_t::HTTP1_1, static_cast <uint16_t> (200)));
		// Если тело кадрируется методом chunked
		if(chunked)
			// Объявляем кодирование тела ответа
			response.emplace("Transfer-Encoding", "chunked");
		// Объявляем размер тела ответа
		else response.emplace("Content-Length", std::to_string(VOLUME));
		// Отправляем заголовки ответа (тело последует)
		sender->sendHeaders(response, false);
		// Позиция выдачи тела сообщения
		size_t offset = 0;
		// Размер одной порции выдачи тела
		static constexpr size_t PORTION = (64 * 1024);
		// Если тело подаётся pull-источником
		if(source){
			// Назначаем pull-источник данных тела сообщения
			sender->dataSource(parser_http_t::data_source_callback_t([&offset](const uint32_t, uint8_t * buffer, const size_t cap, bool & eof) noexcept -> int64_t {
				// Определяем размер выдаваемой источником порции тела
				const size_t size = std::min(cap, (VOLUME - offset));
				/**
				 * Заполняем выдаваемую порцию тела
				 */
				for(size_t i = 0; i < size; i++)
					// Формируем очередной октет порции тела
					buffer[i] = ::octet(offset + i);
				// Сдвигаем позицию выдачи тела
				offset += size;
				// Устанавливаем признак достижения конца тела
				eof = (offset >= VOLUME);
				// Выводим размер выданной порции тела
				return static_cast <int64_t> (size);
			}));
			/**
			 * Прокачиваем источник до исчерпания тела
			 */
			while(sender->sourcePending()){
				// Если прокачка источника не возобновилась
				if(!sender->resumeSource())
					// Прекращаем прокачку источника
					break;
			}
		// Если тело выдаётся методом отправки
		} else {
			// Формируем буфер выдачи порции тела
			std::vector <uint8_t> portion(PORTION);
			/**
			 * Выдаём тело сообщения порциями
			 */
			while(offset < VOLUME){
				// Определяем размер выдаваемой порции тела
				const size_t size = std::min(PORTION, (VOLUME - offset));
				/**
				 * Заполняем выдаваемую порцию тела
				 */
				for(size_t i = 0; i < size; i++)
					// Формируем очередной октет порции тела
					portion[i] = ::octet(offset + i);
				// Выполняем выдачу очередной порции тела
				const size_t accepted = sender->sendData(portion.data(), size, ((offset + size) >= VOLUME));
				// Проверяем что отправитель принял порцию тела
				ASSERT_GT(accepted, 0u);
				// Сдвигаем позицию выдачи тела
				offset += accepted;
			}
		}
		// Проверяем что собранное сообщение разобралось встречным приёмником целиком
		ASSERT_EQ(receiver->status(), parser_t::status_t::COMPLETE);
		// Проверяем что тело доставлено целиком
		ASSERT_EQ(position, static_cast <uint64_t> (VOLUME));
		// Проверяем что содержимое тела доставлено до октета
		ASSERT_EQ(mismatches, 0u);
	};
	// Проверяем сборку тела фиксированного размера методом отправки
	roundtrip(false, false);
	// Проверяем сборку тела в кодировке chunked методом отправки
	roundtrip(false, true);
	// Проверяем сборку тела фиксированного размера pull-источником
	roundtrip(true, false);
	// Проверяем сборку тела в кодировке chunked pull-источником
	roundtrip(true, true);
}

/**
 * @brief Метод тестирования обрыва соединения посреди тела
 *
 * @details Обрыв обязан дать ошибку незавершённого сообщения, а не молча выдать
 *          усечённое тело за целое: иначе приложение приняло бы неполный файл за
 *          полученный. Пришедшее до обрыва обязано совпасть с отправленным до
 *          октета, а объект после очистки - разобрать следующее сообщение
 *
 */
TEST_F(ParserFixture, ConnectionBreakTest){
	// Позиция обрыва соединения
	static constexpr size_t CUT = (VOLUME / 3);
	// Создаём объект парсера-приёмника ответа
	auto parser = this->make(direct_t::RESPONSE);
	// Формируем лимиты безопасности с поднятым пределом размера тела
	parser_http_t::limits_t limits;
	// Поднимаем предел размера тела до проверяемого объёма
	limits.maxBodySize = (VOLUME + 1);
	// Применяем лимиты безопасности разбора
	parser->limits(limits);
	// Устанавливаем метод запроса, которому соответствует ожидаемый ответ
	parser->method(method_t::GET);
	// Позиция сверки принятого тела
	uint64_t position = 0;
	// Количество расхождений содержимого тела
	uint64_t mismatches = 0;
	/**
	 * @brief Функция подписки на приём тела сообщения
	 *
	 * @param target объект парсера-приёмника
	 *
	 */
	auto attach = [&position, &mismatches](parser_http_t * target) noexcept -> void {
		// Устанавливаем функцию обратного вызова обработки фрагмента тела сообщения
		target->on(parser_http_t::data_callback_t([&position, &mismatches](const uint32_t, const void * buffer, const size_t size, const bool) noexcept -> bool {
			// Получаем указатель на принятые октеты тела
			const uint8_t * data = static_cast <const uint8_t *> (buffer);
			/**
			 * Сверяем принятые октеты тела с ожидаемыми
			 */
			for(size_t i = 0; i < size; i++){
				// Если принятый октет не совпал с ожидаемым
				if(data[i] != ::octet(position + i))
					// Считаем расхождение содержимого тела
					mismatches++;
			}
			// Сдвигаем позицию сверки принятого тела
			position += size;
			// Продолжаем разбор
			return true;
		}));
	};
	// Подписываемся на приём тела сообщения
	attach(parser.get());
	// Формируем блок заголовков ответа
	const std::string head = ("HTTP/1.1 200 OK\r\nContent-Length: " + std::to_string(VOLUME) + "\r\n\r\n");
	// Формируем буфер подачи тела
	std::vector <uint8_t> feed(FEED);
	/**
	 * @brief Функция подачи приёмнику части тела
	 *
	 * @param target объект парсера-приёмника
	 * @param from   позиция начала части тела
	 * @param to     позиция конца части тела
	 *
	 */
	auto deliver = [&feed](parser_http_t * target, const size_t from, const size_t to) noexcept -> void {
		// Позиция подачи части тела
		size_t sent = from;
		/**
		 * Подаём часть тела порциями
		 */
		while(sent < to){
			// Определяем размер очередной порции подачи
			const size_t size = std::min(FEED, (to - sent));
			/**
			 * Заполняем буфер подачи очередной порцией тела
			 */
			for(size_t i = 0; i < size; i++)
				// Формируем очередной октет порции тела
				feed[i] = ::octet(sent + i);
			// Подаём приёмнику очередную порцию тела
			target->parse(feed.data(), size);
			// Сдвигаем позицию подачи части тела
			sent += size;
		}
	};
	// Подаём приёмнику блок заголовков ответа
	parser->parse(head.data(), head.size());
	// Подаём приёмнику тело до позиции обрыва
	deliver(parser.get(), 0, CUT);
	// Сообщаем приёмнику о разрыве соединения
	parser->eof();
	// Проверяем что обрыв посреди тела признан незавершённым сообщением
	ASSERT_EQ(parser->error(), parser_http_t::error_t::PREMATURE_EOF);
	// Проверяем что сообщение не выдано за полностью разобранное
	ASSERT_NE(parser->status(), parser_t::status_t::COMPLETE);
	// Проверяем что до обрыва доставлено ровно пришедшее
	ASSERT_EQ(position, static_cast <uint64_t> (CUT));
	// Проверяем что доставленное до обрыва совпало с отправленным до октета
	ASSERT_EQ(mismatches, 0u);
	/**
	 * Соединение оборвано - объект возвращается к состоянию нового: ровно так с ним
	 * поступит обвязка, закрывая разорванное соединение
	 */
	parser->clear();
	// Восстанавливаем лимиты безопасности разбора
	parser->limits(limits);
	// Восстанавливаем метод запроса, которому соответствует ожидаемый ответ
	parser->method(method_t::GET);
	// Сбрасываем позицию сверки принятого тела
	position = 0;
	// Подписываемся на приём тела следующего сообщения
	attach(parser.get());
	// Подаём приёмнику блок заголовков следующего сообщения
	parser->parse(head.data(), head.size());
	// Подаём приёмнику тело следующего сообщения целиком
	deliver(parser.get(), 0, VOLUME);
	// Проверяем что следующее сообщение разобрано целиком
	ASSERT_EQ(parser->status(), parser_t::status_t::COMPLETE);
	// Проверяем что тело следующего сообщения доставлено целиком
	ASSERT_EQ(position, static_cast <uint64_t> (VOLUME));
	// Проверяем что содержимое следующего сообщения доставлено до октета
	ASSERT_EQ(mismatches, 0u);
}

/**
 * @brief Метод тестирования докачки прерванного тела запросом диапазона
 *
 * @details Прерванную передачу возобновляет не парсер, а приложение: оно
 *          запрашивает недостающий диапазон, и сервер отвечает [206 Partial
 *          Content]. Проверяется, что обе части, принятые разными сообщениями на
 *          разных соединениях, складываются в исходное тело до октета
 *
 */
TEST_F(ParserFixture, ResumeByRangeTest){
	// Позиция обрыва передачи
	static constexpr size_t CUT = ((VOLUME * 2) / 5);
	// Позиция сверки принятого тела
	uint64_t position = 0;
	// Количество расхождений содержимого тела
	uint64_t mismatches = 0;
	// Формируем лимиты безопасности с поднятым пределом размера тела
	parser_http_t::limits_t limits;
	// Поднимаем предел размера тела до проверяемого объёма
	limits.maxBodySize = (VOLUME + 1);
	// Формируем буфер подачи тела
	std::vector <uint8_t> feed(FEED);
	/**
	 * @brief Функция подачи приёмнику части тела
	 *
	 * @param target объект парсера-приёмника
	 * @param from   позиция начала части тела
	 * @param to     позиция конца части тела
	 *
	 */
	auto deliver = [&feed](parser_http_t * target, const size_t from, const size_t to) noexcept -> void {
		// Позиция подачи части тела
		size_t sent = from;
		/**
		 * Подаём часть тела порциями
		 */
		while(sent < to){
			// Определяем размер очередной порции подачи
			const size_t size = std::min(FEED, (to - sent));
			/**
			 * Заполняем буфер подачи очередной порцией тела
			 */
			for(size_t i = 0; i < size; i++)
				// Формируем очередной октет порции тела
				feed[i] = ::octet(sent + i);
			// Подаём приёмнику очередную порцию тела
			target->parse(feed.data(), size);
			// Сдвигаем позицию подачи части тела
			sent += size;
		}
	};
	/**
	 * @brief Функция подписки на приём тела сообщения
	 *
	 * @param target объект парсера-приёмника
	 *
	 */
	auto attach = [&position, &mismatches](parser_http_t * target) noexcept -> void {
		// Устанавливаем функцию обратного вызова обработки фрагмента тела сообщения
		target->on(parser_http_t::data_callback_t([&position, &mismatches](const uint32_t, const void * buffer, const size_t size, const bool) noexcept -> bool {
			// Получаем указатель на принятые октеты тела
			const uint8_t * data = static_cast <const uint8_t *> (buffer);
			/**
			 * Сверяем принятые октеты тела с ожидаемыми: сверка продолжается с той же
			 * позиции, на которой оборвалась первая часть - обе части обязаны сложиться
			 * в исходное тело
			 */
			for(size_t i = 0; i < size; i++){
				// Если принятый октет не совпал с ожидаемым
				if(data[i] != ::octet(position + i))
					// Считаем расхождение содержимого тела
					mismatches++;
			}
			// Сдвигаем позицию сверки принятого тела
			position += size;
			// Продолжаем разбор
			return true;
		}));
	};
	/**
	 * Принимаем первую часть тела до обрыва соединения
	 */
	{
		// Создаём объект парсера-приёмника ответа
		auto parser = this->make(direct_t::RESPONSE);
		// Применяем лимиты безопасности разбора
		parser->limits(limits);
		// Устанавливаем метод запроса, которому соответствует ожидаемый ответ
		parser->method(method_t::GET);
		// Подписываемся на приём тела сообщения
		attach(parser.get());
		// Формируем блок заголовков ответа
		const std::string head = ("HTTP/1.1 200 OK\r\nContent-Length: " + std::to_string(VOLUME) + "\r\n\r\n");
		// Подаём приёмнику блок заголовков ответа
		parser->parse(head.data(), head.size());
		// Подаём приёмнику первую часть тела
		deliver(parser.get(), 0, CUT);
		// Сообщаем приёмнику о разрыве соединения
		parser->eof();
		// Проверяем что обрыв признан незавершённым сообщением
		ASSERT_EQ(parser->error(), parser_http_t::error_t::PREMATURE_EOF);
	}
	// Проверяем что до обрыва доставлена ровно первая часть тела
	ASSERT_EQ(position, static_cast <uint64_t> (CUT));
	/**
	 * Принимаем остаток тела ответом на запрос диапазона на новом соединении
	 */
	{
		// Создаём объект парсера-приёмника ответа
		auto parser = this->make(direct_t::RESPONSE);
		// Применяем лимиты безопасности разбора
		parser->limits(limits);
		// Устанавливаем метод запроса, которому соответствует ожидаемый ответ
		parser->method(method_t::GET);
		// Подписываемся на приём тела сообщения
		attach(parser.get());
		// Формируем блок заголовков ответа на запрос диапазона
		const std::string head = (
			"HTTP/1.1 206 Partial Content\r\nContent-Range: bytes " + std::to_string(CUT) + "-" +
			std::to_string(VOLUME - 1) + "/" + std::to_string(VOLUME) + "\r\nContent-Length: " +
			std::to_string(VOLUME - CUT) + "\r\n\r\n"
		);
		// Подаём приёмнику блок заголовков ответа
		parser->parse(head.data(), head.size());
		// Подаём приёмнику остаток тела
		deliver(parser.get(), CUT, VOLUME);
		// Проверяем что ответ на запрос диапазона разобран целиком
		ASSERT_EQ(parser->status(), parser_t::status_t::COMPLETE);
	}
	// Проверяем что обе части сложились в исходное тело
	ASSERT_EQ(position, static_cast <uint64_t> (VOLUME));
	// Проверяем что сложенное тело совпало с исходным до октета
	ASSERT_EQ(mismatches, 0u);
}

/**
 * @brief Метод тестирования обрыва соединения посреди сборки исходящего сообщения
 *
 * @details Разорванное соединение уносит с собой недоотправленное сообщение.
 *          Бросить его подготовкой к следующему нельзя: следующий блок заголовков
 *          лёг бы продолжением чужого тела, и получатель прочитал бы одно сообщение
 *          вместо двух. Отправитель обязан отказать, а восстанавливается объект
 *          полной очисткой - тем же способом, каким закрывается соединение
 *
 */
TEST_F(ParserFixture, SenderBreakTest){
	// Объём тела прерываемого сообщения
	static constexpr size_t LARGE = (4 * 1024 * 1024);
	// Позиция обрыва соединения
	static constexpr size_t CUT = (LARGE / 4);
	// Объём тела сообщения после обрыва
	static constexpr size_t NEXT = (128 * 1024);
	// Размер одной порции выдачи тела
	static constexpr size_t PORTION = (64 * 1024);
	// Создаём объект парсера-отправителя ответа
	auto sender = this->make(direct_t::RESPONSE);
	// Признак накопления отданных байтов
	bool collect = false;
	// Собранные байты исходящего сообщения
	std::string wire;
	/**
	 * @brief Функция подписки на запись исходящих байтов
	 *
	 * @param target объект парсера-отправителя
	 *
	 */
	auto attach = [&collect, &wire](parser_http_t * target) noexcept -> void {
		// Устанавливаем функцию обратного вызова записи исходящих байтов в сеть
		target->on(parser_http_t::write_callback_t([&collect, &wire](const void * buffer, const size_t size) noexcept {
			// Если байты накапливаются
			if(collect)
				// Собираем отданные сетевому слою байты
				wire.append(static_cast <const char *> (buffer), size);
		}));
	};
	// Подписываемся на запись исходящих байтов
	attach(sender.get());
	// Понижаем пороги выходного буфера
	sender->sendWaterMarks(256 * 1024, 64 * 1024);
	// Формируем буфер выдачи порции тела
	std::vector <uint8_t> portion(PORTION);
	/**
	 * Собираем сообщение до позиции обрыва соединения
	 */
	{
		// Формируем контейнер заголовков ответа с провайдером
		headers_t response(std::make_unique <response_t> (version_t::HTTP1_1, static_cast <uint16_t> (200)));
		// Объявляем размер тела ответа
		response.emplace("Content-Length", std::to_string(LARGE));
		// Отправляем заголовки ответа (тело последует)
		sender->sendHeaders(response, false);
		// Позиция выдачи тела сообщения
		size_t offset = 0;
		/**
		 * Выдаём тело сообщения порциями до позиции обрыва
		 */
		while(offset < CUT){
			// Определяем размер выдаваемой порции тела
			const size_t size = std::min(PORTION, (CUT - offset));
			// Выполняем выдачу очередной порции тела
			const size_t accepted = sender->sendData(portion.data(), size, false);
			// Проверяем что отправитель принял порцию тела
			ASSERT_GT(accepted, 0u);
			// Сдвигаем позицию выдачи тела
			offset += accepted;
		}
	}
	// Пробуем подготовить отправитель к следующему сообщению поверх незавершённого
	sender->resetSender();
	// Начинаем накапливать отданные байты
	collect = true;
	// Очищаем собранные байты
	wire.clear();
	/**
	 * Проверяем что подготовка отказала: собранный после неё блок заголовков на
	 * провод уйти не вправе - иначе он лёг бы продолжением чужого тела
	 */
	{
		// Формируем контейнер заголовков ответа с провайдером
		headers_t probe(std::make_unique <response_t> (version_t::HTTP1_1, static_cast <uint16_t> (204)));
		// Пробуем отправить заголовки ответа поверх незавершённого сообщения
		sender->sendHeaders(probe, true);
		// Проверяем что блок заголовков на провод не ушёл
		ASSERT_TRUE(wire.empty()) << wire;
	}
	/**
	 * Возвращаем объект к состоянию нового: разорванное соединение закрывается,
	 * а объект уходит обслуживать следующее
	 */
	sender->clear();
	// Восстанавливаем пороги выходного буфера
	sender->sendWaterMarks(256 * 1024, 64 * 1024);
	// Восстанавливаем подписку на запись исходящих байтов
	attach(sender.get());
	// Очищаем собранные байты прежнего соединения
	wire.clear();
	/**
	 * Собираем следующее сообщение целиком на новом соединении
	 */
	{
		// Формируем контейнер заголовков ответа с провайдером
		headers_t response(std::make_unique <response_t> (version_t::HTTP1_1, static_cast <uint16_t> (200)));
		// Объявляем размер тела ответа
		response.emplace("Content-Length", std::to_string(NEXT));
		// Отправляем заголовки ответа (тело последует)
		sender->sendHeaders(response, false);
		// Позиция выдачи тела сообщения
		size_t offset = 0;
		/**
		 * Выдаём тело следующего сообщения порциями
		 */
		while(offset < NEXT){
			// Определяем размер выдаваемой порции тела
			const size_t size = std::min(PORTION, (NEXT - offset));
			/**
			 * Заполняем выдаваемую порцию тела
			 */
			for(size_t i = 0; i < size; i++)
				// Формируем очередной октет порции тела
				portion[i] = ::octet(offset + i);
			// Выполняем выдачу очередной порции тела
			const size_t accepted = sender->sendData(portion.data(), size, ((offset + size) >= NEXT));
			// Проверяем что отправитель принял порцию тела
			ASSERT_GT(accepted, 0u);
			// Сдвигаем позицию выдачи тела
			offset += accepted;
		}
	}
	// Позиция сверки принятого тела
	uint64_t position = 0;
	// Количество расхождений содержимого тела
	uint64_t mismatches = 0;
	// Создаём объект парсера-приёмника собранного сообщения
	auto receiver = this->make(direct_t::RESPONSE);
	// Устанавливаем метод запроса, которому соответствует ожидаемый ответ
	receiver->method(method_t::GET);
	// Устанавливаем функцию обратного вызова обработки фрагмента тела сообщения
	receiver->on(parser_http_t::data_callback_t([&position, &mismatches](const uint32_t, const void * buffer, const size_t size, const bool) noexcept -> bool {
		// Получаем указатель на принятые октеты тела
		const uint8_t * data = static_cast <const uint8_t *> (buffer);
		/**
		 * Сверяем принятые октеты тела с ожидаемыми
		 */
		for(size_t i = 0; i < size; i++){
			// Если принятый октет не совпал с ожидаемым
			if(data[i] != ::octet(position + i))
				// Считаем расхождение содержимого тела
				mismatches++;
		}
		// Сдвигаем позицию сверки принятого тела
		position += size;
		// Продолжаем разбор
		return true;
	}));
	// Выполняем разбор собранного после обрыва сообщения
	const size_t consumed = receiver->parse(wire.data(), wire.size());
	// Проверяем что собранное после обрыва сообщение разобралось целиком
	ASSERT_EQ(receiver->status(), parser_t::status_t::COMPLETE);
	// Проверяем что на проводе не осталось непотреблённого хвоста прежнего сообщения
	ASSERT_EQ(consumed, wire.size());
	// Проверяем что тело следующего сообщения доставлено целиком
	ASSERT_EQ(position, static_cast <uint64_t> (NEXT));
	// Проверяем что содержимое следующего сообщения доставлено до октета
	ASSERT_EQ(mismatches, 0u);
}

/**
 * @brief Метод тестирования числовых границ объявленного кадрирования
 *
 * @details Переполнение при накоплении числа - классический источник дефектов
 *          кадрирования: обёрнутое значение превращает огромное тело в короткое,
 *          а его остаток получатель читает как начало следующего сообщения
 *
 */
TEST_F(ParserFixture, FramingNumericEdgesTest){
	/**
	 * @brief Функция разбора запроса и получения кода ошибки
	 *
	 * @param message разбираемое сообщение
	 * @return        код ошибки разбора
	 *
	 */
	auto probe = [this](const std::string & message) noexcept -> parser_http_t::error_t {
		// Создаём объект парсера-приёмника запроса
		auto parser = this->make(direct_t::REQUEST);
		// Выполняем разбор сформированного сообщения
		parser->parse(message.data(), message.size());
		// Выводим код ошибки разбора
		return parser->error();
	};
	// Начало разбираемого запроса
	const std::string head = "POST / HTTP/1.1\r\nHost: anyks.com\r\n";
	// Начало разбираемого запроса с объявленным кодированием тела
	const std::string chunked = (head + "Transfer-Encoding: chunked\r\n\r\n");
	/**
	 * Проверяем предельный размер тела, представимый знаковым типом: он выходит
	 * за предел размера тела, но числом остаётся корректным
	 */
	ASSERT_EQ(probe(head + "Content-Length: 9223372036854775807\r\n\r\n"), parser_http_t::error_t::BODY_OVERFLOW);
	// Проверяем размер тела на единицу больше представимого знаковым типом
	ASSERT_EQ(probe(head + "Content-Length: 9223372036854775808\r\n\r\n"), parser_http_t::error_t::INVALID_CONTENT_LENGTH);
	// Проверяем размер тела за пределом беззнакового типа
	ASSERT_EQ(probe(head + "Content-Length: 18446744073709551616\r\n\r\n"), parser_http_t::error_t::INVALID_CONTENT_LENGTH);
	// Проверяем размер тела, записанный далеко за пределом любого типа
	ASSERT_EQ(probe(head + "Content-Length: 1111111111111111111111111111111111111111\r\n\r\n"), parser_http_t::error_t::INVALID_CONTENT_LENGTH);
	// Проверяем что ведущие нули размеру тела не мешают (RFC 9110 §8.6 - 1*DIGIT)
	ASSERT_EQ(probe(head + "Content-Length: 000000000000005\r\n\r\nhello"), parser_http_t::error_t::NONE);
	// Проверяем размер тела со знаком плюс
	ASSERT_EQ(probe(head + "Content-Length: +5\r\n\r\nhello"), parser_http_t::error_t::INVALID_CONTENT_LENGTH);
	// Проверяем отрицательный размер тела
	ASSERT_EQ(probe(head + "Content-Length: -5\r\n\r\nhello"), parser_http_t::error_t::INVALID_CONTENT_LENGTH);
	// Проверяем пустой размер тела
	ASSERT_EQ(probe(head + "Content-Length:\r\n\r\n"), parser_http_t::error_t::INVALID_CONTENT_LENGTH);
	// Проверяем размер тела с внутренним пробелом
	ASSERT_EQ(probe(head + "Content-Length: 1 5\r\n\r\n"), parser_http_t::error_t::INVALID_CONTENT_LENGTH);
	// Проверяем размер тела в шестнадцатеричной записи
	ASSERT_EQ(probe(head + "Content-Length: 0x5\r\n\r\nhello"), parser_http_t::error_t::INVALID_CONTENT_LENGTH);
	/**
	 * Проверяем предельный размер чанка, представимый беззнаковым типом: он
	 * выходит за предел размера чанка, но числом остаётся корректным
	 */
	ASSERT_EQ(probe(chunked + "FFFFFFFFFFFFFFFF\r\n"), parser_http_t::error_t::CHUNK_OVERFLOW);
	/**
	 * Проверяем размер чанка за пределом беззнакового типа: добавление
	 * семнадцатой цифры обязано быть отвергнуто до сдвига, а не после
	 */
	ASSERT_EQ(probe(chunked + "10000000000000000\r\n"), parser_http_t::error_t::CHUNK_OVERFLOW);
	// Проверяем что ведущие нули размеру чанка не мешают (RFC 9112 §7.1 - 1*HEXDIG)
	ASSERT_EQ(probe(chunked + "000000000000000005\r\nhello\r\n0\r\n\r\n"), parser_http_t::error_t::NONE);
	// Проверяем пустой размер чанка
	ASSERT_EQ(probe(chunked + "\r\n"), parser_http_t::error_t::INVALID_CHUNK_SIZE);
	// Проверяем отрицательный размер чанка
	ASSERT_EQ(probe(chunked + "-5\r\n"), parser_http_t::error_t::INVALID_CHUNK_SIZE);
	// Проверяем что поток ведущих нулей упирается в предел длины строки размера чанка
	ASSERT_EQ(probe(chunked + std::string(200000, '0') + "5\r\nhello\r\n0\r\n\r\n"), parser_http_t::error_t::CHUNK_OVERFLOW);
	// Проверяем что поток расширений упирается в предел длины строки размера чанка
	ASSERT_EQ(probe(chunked + "5;" + std::string(200000, 'a') + "\r\nhello\r\n0\r\n\r\n"), parser_http_t::error_t::CHUNK_OVERFLOW);
	// Проверяем что поток ведущих нулей размера тела упирается в предел блока заголовков
	ASSERT_EQ(probe(head + "Content-Length: " + std::string(200000, '0') + "5\r\n\r\nhello"), parser_http_t::error_t::HEADER_OVERFLOW);
}
