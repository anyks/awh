/**
 * @file: throughput.cpp
 * @date: 2026-07-27
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @brief Сценарии измерения пропускной способности протокола HTTP/3 — разбор потока
 *        запросов, приём тела сообщения, полный обмен через пару парсеров и стоимость
 *        мультиплексирования множества потоков
 *
 * @copyright: Copyright © 2026
 *
 */

/**
 * Стандартные заголовочные файлы
 */
#include <deque>
#include <tuple>
#include <chrono>
#include <string>
#include <vector>
#include <cstdio>
#include <algorithm>

/**
 * Подключаем заголовочный файл бенчмарков протокола HTTP/3
 */
#include "http3.hpp"

/**
 * Используем стандартное пространство имён
 */
using namespace std;

/**
 * Подписываемся на пространство имён HTTP-протокола
 */
using namespace awh::http;

/**
 * @brief Внутренние параметры и сценарии бенчмарков пропускной способности
 *
 */
namespace {
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
	 * @details Размер кадра HTTP/3 протоколом не ограничен вовсе: параметра, подобного
	 *          SETTINGS_MAX_FRAME_SIZE, у него нет. Значение взято таким же, как
	 *          у HTTP/2 по умолчанию, - иначе показатели двух протоколов сравнивались
	 *          бы на разной нагрузке
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
	 * @details Поток подаётся порциями размером с типичное чтение из сокета, а не
	 *          целиком: подача одним куском переложила бы на парсер буферизацию всего
	 *          объёма и завысила бы показатель - в реальной работе такого не бывает
	 *
	 */
	static constexpr size_t CHUNK_SIZE = (64 * 1024);
	/**
	 * @brief Порог скорости разбора потока запросов в запросах в секунду
	 *
	 * @details Пороги откалиброваны по отладочной сборке с четырёхкратным запасом:
	 *          они ловят регрессии на порядок, а не колебания окружения. Показательный
	 *          пример такой регрессии - удаление разобранного префикса из начала
	 *          буфера потока вместо продвижения курсора: стоимость становится
	 *          квадратичной по объёму подачи
	 *
	 */
	static constexpr double REQUEST_THRESHOLD = 39000.0;
	/**
	 * @brief Порог скорости приёма тела в мегабайтах в секунду
	 *
	 * @details Окон управления потоком у HTTP/3 нет - их ведёт транспорт, - поэтому
	 *          порог сторожит другое: отдачу тела представлением без копирования.
	 *          Копирование каждого кадра в буфер обвалит показатель на порядок
	 *
	 */
	static constexpr double BODY_THRESHOLD = 6900.0;
	/**
	 * @brief Порог скорости полного обмена в обменах в секунду
	 *
	 */
	static constexpr double ROUNDTRIP_THRESHOLD = 8000.0;
	/**
	 * @brief Порог скорости полного обмена по множеству потоков
	 *
	 */
	static constexpr double MULTIPLEX_THRESHOLD = 6000.0;
	/**
	 * @brief Порог количества выделений памяти на разобранный запрос
	 *
	 * @details Разбор запроса создаёт состояние потока и провайдер полей, поэтому
	 *          выделения неизбежны. Ограничение сверху ловит появление выделения
	 *          на каждое поле либо на каждый кадр. Порог снижен с 3.5 после того,
	 *          как буфер накопления нагрузки кадра стал брать ёмкость из накопителя,
	 *          а не растить её с нуля на каждую секцию полей: измеренное значение
	 *          опустилось с 3 до 2, и прежний порог перестал сторожить откат
	 *
	 */
	static constexpr double REQUEST_ALLOCATIONS_THRESHOLD = 2.5;
	/**
	 * @brief Порог количества выделений памяти на кадр данных
	 *
	 * @details Приём тела в установившемся режиме выделений требовать не должен:
	 *          тело отдаётся приложению представлением прямо из входного буфера,
	 *          не накапливаясь
	 *
	 */
	static constexpr double BODY_ALLOCATIONS_THRESHOLD = 0.1;

	/**
	 * @brief Структура итогов прогона обмена
	 *
	 */
	typedef struct Transfer {
		// Количество выполненных операций
		size_t operations;
		// Количество принятых октетов
		size_t received;
		// Количество переданных кадров
		size_t frames;
		// Затраченное время в секундах
		double seconds;
		// Количество выполненных выделений памяти
		size_t allocations;
		// Суммарный объём выделенной памяти в октетах
		size_t bytes;
		/**
		 * @brief Конструктор
		 *
		 */
		explicit Transfer() noexcept :
		 operations(0), received(0), frames(0), seconds(0.0), allocations(0), bytes(0) {}
	} transfer_t;

	/**
	 * @brief Функция подписки парсера на обязательные функции обратного вызова транспорта
	 *
	 * @details Без выдачи идентификаторов однонаправленных потоков парсер не сможет
	 *          открыть ни управляющий поток, ни потоки инструкций QPACK: в HTTP/3
	 *          их выделяет транспорт, а не сам протокол
	 *
	 * @param parser парсер, подписываемый на функции обратного вызова
	 * @param next   идентификатор следующего выдаваемого однонаправленного потока
	 * @param sink   накопитель исходящих байтов потоков
	 *
	 */
	static void attach(parser_http3_t & parser, uint64_t & next, deque <tuple <uint64_t, string, bool>> * sink) noexcept {
		// Устанавливаем функцию обратного вызова открытия однонаправленного потока
		parser.on(parser_http3_t::open_callback_t([&next]() noexcept -> int64_t {
			// Выделяем идентификатор однонаправленного потока
			const int64_t sid = static_cast <int64_t> (next);
			// Продвигаем идентификатор следующего однонаправленного потока
			next += 4;
			// Выводим идентификатор открытого потока
			return sid;
		}));
		// Устанавливаем функцию обратного вызова записи исходящих байтов потока
		parser.on(parser_http3_t::write_callback_t([sink](const uint64_t sid, const void * buffer, const size_t size, const bool fin) noexcept {
			// Если накопитель исходящих байтов задан
			if(sink != nullptr)
				// Складываем исходящие байты в накопитель
				sink->emplace_back(sid, string(reinterpret_cast <const char *> (buffer), size), fin);
		}));
	}
	/**
	 * @brief Функция передачи накопленных байтов противоположной стороне
	 *
	 * @param queue накопитель исходящих байтов потоков
	 * @param peer  парсер противоположной стороны
	 *
	 */
	static void pump(deque <tuple <uint64_t, string, bool>> & queue, parser_http3_t & peer) noexcept {
		/**
		 * Передаём все накопленные порции: разбор порции способен породить ответ,
		 * поэтому очередь читается до опустошения
		 */
		while(!queue.empty()){
			// Забираем очередную порцию исходящих байтов
			const auto item = queue.front();
			// Удаляем порцию из накопителя
			queue.pop_front();
			// Подаём порцию на разбор противоположной стороне
			peer.parse(get <0> (item), get <1> (item).data(), get <1> (item).size(), get <2> (item));
		}
	}

	/**
	 * @brief Функция прогона разбора потока запросов
	 *
	 * @param counting признак учёта выделений памяти
	 * @param output   итоги прогона
	 * @return         результат прогона
	 *
	 */
	static bool requests(const bool counting, transfer_t & output) noexcept {
		// Создаём объект кодера полей клиента
		h3::qpack::encoder_t encoder;
		// Устанавливаем ёмкость динамической таблицы кодера
		encoder.maxCapacity(h3::proto::QPACK_TABLE_CAPACITY);
		// Устанавливаем число потоков, которым разрешено ожидать пополнения таблицы
		encoder.maxBlocked(h3::proto::QPACK_BLOCKED_STREAMS);
		// Порции байтов потоков клиента в порядке отправки
		vector <tuple <uint64_t, string, bool>> input;
		// Резервируем память под порции байтов потоков
		input.reserve(2 + (REQUEST_ROUNDS * 2));
		// Открываем управляющий поток клиента кадром параметров соединения
		input.emplace_back(awh::benchmark::http3::CONTROL_STREAM, awh::benchmark::http3::control(), false);
		// Открываем поток инструкций кодера клиента
		input.emplace_back(awh::benchmark::http3::ENCODER_STREAM, awh::benchmark::http3::varint(static_cast <uint64_t> (h3::unistream_t::QPACK_ENCODER)), false);
		// Объём поданного потока байтов
		size_t volume = (get <1> (input[0]).size() + get <1> (input[1]).size());
		/**
		 * Готовим поток запросов заранее: кодирование полей клиентом к измеряемой
		 * работе сервера отношения не имеет
		 */
		for(size_t i = 0; i < REQUEST_ROUNDS; i++){
			// Идентификатор двунаправленного потока запроса
			const uint64_t sid = (i * 4);
			// Буфер закодированной секции полей
			string section;
			// Кодируем секцию полей запроса
			encoder.encode(sid, awh::benchmark::http3::request(i), section, true);
			// Получаем инструкции, выставленные кодером в свой поток
			const string_view instructions = encoder.pending();
			/**
			 * Инструкции идут перед секцией: без них секция ссылается на записи,
			 * которых в таблице декодера ещё нет, и поток блокируется
			 */
			if(!instructions.empty()){
				// Дописываем инструкции в поток кодера клиента
				input.emplace_back(awh::benchmark::http3::ENCODER_STREAM, string(instructions), false);
				// Суммируем объём поданного потока
				volume += instructions.size();
				// Освобождаем выданные кодером инструкции
				encoder.consumePending(instructions.size());
			}
			// Собираем кадр секции полей запроса
			string current = awh::benchmark::http3::frame(static_cast <uint64_t> (h3::frame_t::HEADERS), section);
			// Суммируем объём поданного потока
			volume += current.size();
			// Дописываем кадр секции полей с завершением потока
			input.emplace_back(sid, std::move(current), true);
			/**
			 * Подтверждаем секцию кодеру сразу: поток запросов собирается заранее,
			 * поэтому настоящих подтверждений от сервера кодер не увидит, а без них
			 * он откажется от динамической таблицы - и стенд измерял бы разбор
			 * секций с одними литералами, которых на живом соединении не бывает
			 */
			// Собираем подтверждение отправленной секции
			const string confirmation = awh::benchmark::http3::acknowledge(sid);
			// Количество разобранных октетов подтверждения
			size_t consumed = 0;
			// Код ошибки разбора подтверждения
			h3::error_t error = h3::error_t::H3_NO_ERROR;
			// Подаём подтверждение секции кодеру
			encoder.decodeDecoderStream(confirmation, consumed, error);
			// Считаем сформированный кадр
			output.frames++;
		}
		// Идентификатор следующего однонаправленного потока сервера
		uint64_t next = 3;
		// Создаём объект парсера сервера
		parser_http3_t server(direct_t::REQUEST, awh::benchmark::http3::fmk(), awh::benchmark::http3::log());
		// Подписываем сервер на функции обратного вызова транспорта
		::attach(server, next, nullptr);
		// Количество разобранных запросов
		size_t parsed = 0;
		// Минимальный ответ сервера: поля без тела
		vector <h3::qpack::field_t> answer;
		// Дописываем псевдо-поле статуса ответа
		answer.emplace_back(":status", "200");
		// Дописываем поле длины содержимого
		answer.emplace_back("content-length", "0");
		/**
		 * Устанавливаем функцию обратного вызова провайдера полей. Сервер обязан
		 * ответить: без ответа поток остаётся полуоткрытым, такие потоки копятся
		 * и упираются в лимит одновременных потоков - ровно как у настоящего сервера
		 */
		server.on(parser_http3_t::provider_callback_t([&](const uint64_t sid, const provider_t * provider, const bool) noexcept -> bool {
			// Если получен провайдер запроса
			if(provider != nullptr){
				// Считаем разобранный запрос
				parsed++;
				// Отправляем минимальный ответ с завершением потока
				server.sendHeaders(sid, answer, true);
			}
			// Продолжаем разбор
			return true;
		}));
		// Отправляем параметры соединения сервера
		server.sendSettings();
		// Если требуется учёт выделений памяти
		if(counting)
			// Включаем учёт выделений памяти
			awh::benchmark::counting(true);
		// Запоминаем момент начала измерения
		const auto start = std::chrono::steady_clock::now();
		/**
		 * Подаём порции байтов потоков в порядке отправки
		 */
		for(const auto & item : input)
			// Подаём очередную порцию потока на разбор
			server.parse(get <0> (item), get <1> (item).data(), get <1> (item).size(), get <2> (item));
		// Запоминаем момент окончания измерения
		const auto finish = std::chrono::steady_clock::now();
		// Если выполнялся учёт выделений памяти
		if(counting){
			// Отключаем учёт выделений памяти
			awh::benchmark::counting(false);
			// Получаем статистику выделений памяти
			awh::benchmark::allocations(output.allocations, output.bytes);
		}
		// Устанавливаем количество разобранных запросов
		output.operations = parsed;
		// Устанавливаем объём разобранного потока
		output.received = volume;
		// Устанавливаем затраченное время
		output.seconds = std::chrono::duration <double> (finish - start).count();
		// Выводим результат прогона
		return (parsed == REQUEST_ROUNDS);
	}
	/**
	 * @brief Функция прогона приёма тела сообщения
	 *
	 * @param counting признак учёта выделений памяти
	 * @param output   итоги прогона
	 * @return         результат прогона
	 *
	 */
	static bool body(const bool counting, transfer_t & output) noexcept {
		// Создаём объект кодера полей клиента
		h3::qpack::encoder_t encoder;
		// Формируем поля запроса с телом
		vector <h3::qpack::field_t> fields;
		// Дописываем псевдо-поле метода запроса
		fields.emplace_back(":method", "POST");
		// Дописываем псевдо-поле схемы запроса
		fields.emplace_back(":scheme", "https");
		// Дописываем псевдо-поле пути запроса
		fields.emplace_back(":path", "/upload");
		// Дописываем псевдо-поле авторитета запроса
		fields.emplace_back(":authority", "www.example.com");
		// Буфер закодированной секции полей
		string section;
		// Кодируем секцию полей запроса
		encoder.encode(0, fields, section, true);
		// Буфер байтов потока запроса
		string input = awh::benchmark::http3::frame(static_cast <uint64_t> (h3::frame_t::HEADERS), section);
		// Резервируем память под поток целиком
		input.reserve(BODY_SIZE + ((BODY_SIZE / FRAME_SIZE) * 16) + 256);
		// Полезная нагрузка одного кадра данных
		const string payload(FRAME_SIZE, 'x');
		// Количество кадров данных потока
		const size_t total = (BODY_SIZE / FRAME_SIZE);
		/**
		 * Дописываем кадры данных: поток завершается признаком FIN транспорта,
		 * собственного флага завершения у кадров HTTP/3 нет
		 */
		for(size_t i = 0; i < total; i++){
			// Дописываем кадр данных
			input += awh::benchmark::http3::frame(static_cast <uint64_t> (h3::frame_t::DATA), payload);
			// Считаем сформированный кадр
			output.frames++;
		}
		// Идентификатор следующего однонаправленного потока сервера
		uint64_t next = 3;
		// Создаём объект парсера сервера
		parser_http3_t server(direct_t::REQUEST, awh::benchmark::http3::fmk(), awh::benchmark::http3::log());
		// Подписываем сервер на функции обратного вызова транспорта
		::attach(server, next, nullptr);
		// Объём принятого тела
		size_t accepted = 0;
		// Устанавливаем функцию обратного вызова тела сообщения
		server.on(parser_http3_t::data_callback_t([&](const uint64_t, const void *, const size_t size, const bool) noexcept -> bool {
			// Суммируем объём принятого тела
			accepted += size;
			// Продолжаем разбор
			return true;
		}));
		// Отправляем параметры соединения сервера
		server.sendSettings();
		// Открываем управляющий поток клиента кадром параметров соединения
		const string options = awh::benchmark::http3::control();
		// Подаём управляющий поток клиента на разбор
		server.parse(awh::benchmark::http3::CONTROL_STREAM, options.data(), options.size(), false);
		// Если требуется учёт выделений памяти
		if(counting)
			// Включаем учёт выделений памяти
			awh::benchmark::counting(true);
		// Запоминаем момент начала измерения
		const auto start = std::chrono::steady_clock::now();
		/**
		 * Подаём поток порциями размером с чтение из сокета
		 */
		for(size_t offset = 0; offset < input.size(); offset += CHUNK_SIZE){
			// Вычисляем размер очередной порции
			const size_t size = ::min(CHUNK_SIZE, (input.size() - offset));
			// Подаём очередную порцию потока на разбор
			server.parse(0, input.data() + offset, size, ((offset + size) >= input.size()));
		}
		// Запоминаем момент окончания измерения
		const auto finish = std::chrono::steady_clock::now();
		// Если выполнялся учёт выделений памяти
		if(counting){
			// Отключаем учёт выделений памяти
			awh::benchmark::counting(false);
			// Получаем статистику выделений памяти
			awh::benchmark::allocations(output.allocations, output.bytes);
		}
		// Устанавливаем объём принятого тела
		output.operations = accepted;
		// Устанавливаем объём разобранного потока
		output.received = input.size();
		// Устанавливаем затраченное время
		output.seconds = std::chrono::duration <double> (finish - start).count();
		// Выводим результат прогона
		return (accepted == BODY_SIZE);
	}
	/**
	 * @brief Функция прогона полного обмена через пару парсеров
	 *
	 * @param streams количество одновременных потоков обмена
	 * @param output  итоги прогона
	 * @return        результат прогона
	 *
	 */
	static bool roundtrip(const size_t streams, transfer_t & output) noexcept {
		// Накопитель исходящих байтов клиента
		deque <tuple <uint64_t, string, bool>> fromClient;
		// Накопитель исходящих байтов сервера
		deque <tuple <uint64_t, string, bool>> fromServer;
		// Идентификатор следующего однонаправленного потока клиента
		uint64_t clientUni = 2;
		// Идентификатор следующего однонаправленного потока сервера
		uint64_t serverUni = 3;
		// Создаём объект парсера клиента
		parser_http3_t client(direct_t::RESPONSE, awh::benchmark::http3::fmk(), awh::benchmark::http3::log());
		// Создаём объект парсера сервера
		parser_http3_t server(direct_t::REQUEST, awh::benchmark::http3::fmk(), awh::benchmark::http3::log());
		// Подписываем клиента на функции обратного вызова транспорта
		::attach(client, clientUni, &fromClient);
		// Подписываем сервер на функции обратного вызова транспорта
		::attach(server, serverUni, &fromServer);
		// Количество завершённых обменов
		size_t completed = 0;
		// Эталонный набор полей ответа сервера
		const vector <h3::qpack::field_t> answer = awh::benchmark::http3::response(0);
		/**
		 * Устанавливаем функцию обратного вызова провайдера полей сервера:
		 * на каждый принятый запрос сервер отвечает полями и телом
		 */
		server.on(parser_http3_t::provider_callback_t([&](const uint64_t sid, const provider_t * provider, const bool) noexcept -> bool {
			// Если получен провайдер запроса
			if(provider != nullptr){
				// Отправляем секцию полей ответа
				server.sendHeaders(sid, answer, false);
				// Отправляем тело ответа вместе с завершением потока
				server.sendData(sid, awh::benchmark::http3::payload().data(), awh::benchmark::http3::payload().size(), true);
			}
			// Продолжаем разбор
			return true;
		}));
		/**
		 * Устанавливаем функцию обратного вызова фазы приёма сообщения клиента:
		 * обмен считается завершённым по полному приёму ответа
		 */
		client.on(parser_http3_t::phase_callback_t([&](const uint64_t, const parser_t::phase_t phase, const parser_t::part_t part) noexcept -> bool {
			// Если ответ принят целиком
			if((phase == parser_t::phase_t::END) && (part == parser_t::part_t::NONE))
				// Считаем завершённый обмен
				completed++;
			// Продолжаем разбор
			return true;
		}));
		// Отправляем параметры соединения клиента
		client.sendSettings();
		// Отправляем параметры соединения сервера
		server.sendSettings();
		// Передаём накопленные клиентом байты серверу
		::pump(fromClient, server);
		// Передаём накопленные сервером байты клиенту
		::pump(fromServer, client);
		// Идентификатор следующего двунаправленного потока клиента
		uint64_t bidi = 0;
		// Количество выполненных обменов
		size_t performed = 0;
		// Запоминаем момент начала измерения
		const auto start = std::chrono::steady_clock::now();
		/**
		 * Выполняем обмены группами по числу одновременных потоков
		 */
		while(performed < ROUNDTRIP_ROUNDS){
			// Определяем размер очередной группы обменов
			const size_t group = ::min(streams, (ROUNDTRIP_ROUNDS - performed));
			/**
			 * Выполняем отправку запросов всей группы
			 */
			for(size_t i = 0; i < group; i++){
				// Отправляем секцию полей запроса с завершением потока
				client.sendHeaders(bidi, awh::benchmark::http3::request(performed + i), true);
				// Продвигаем идентификатор следующего двунаправленного потока
				bidi += 4;
				// Считаем переданный кадр
				output.frames++;
			}
			// Передаём накопленные клиентом байты серверу
			::pump(fromClient, server);
			// Передаём накопленные сервером байты клиенту
			::pump(fromServer, client);
			// Передаём накопленные клиентом подтверждения серверу
			::pump(fromClient, server);
			// Считаем выполненные обмены
			performed += group;
		}
		// Запоминаем момент окончания измерения
		const auto finish = std::chrono::steady_clock::now();
		// Устанавливаем количество завершённых обменов
		output.operations = completed;
		// Устанавливаем затраченное время
		output.seconds = std::chrono::duration <double> (finish - start).count();
		// Выводим результат прогона
		return (completed == ROUNDTRIP_ROUNDS);
	}

	/**
	 * @brief Функция измерения скорости разбора потока запросов
	 *
	 * @return результат измерения
	 *
	 */
	static awh::benchmark::result_t requestThroughput() noexcept {
		// Результат измерения
		awh::benchmark::result_t result;
		// Итоги прогона
		transfer_t transfer;
		// Если прогон не выполнен
		if(!::requests(false, transfer)){
			// Устанавливаем сведения о неудачном прогоне
			result.details = "прогон не выполнен: разобраны не все запросы";
			// Выводим результат измерения
			return result;
		}
		// Вычисляем скорость разбора потока запросов
		result.value = ((transfer.seconds > 0.0) ? (static_cast <double> (transfer.operations) / transfer.seconds) : 0.0);
		// Буфер сведений о прогоне
		char details[256];
		// Формируем сведения о прогоне
		::snprintf(
			details, sizeof(details), "запросов: %zu, поток: %.1f МБ, МБ/с: %.1f, на запрос: %.1f октетов",
			transfer.operations, (static_cast <double> (transfer.received) / (1024.0 * 1024.0)),
			((transfer.seconds > 0.0) ? ((static_cast <double> (transfer.received) / (1024.0 * 1024.0)) / transfer.seconds) : 0.0),
			(static_cast <double> (transfer.received) / static_cast <double> (transfer.operations))
		);
		// Устанавливаем сведения о прогоне
		result.details = details;
		// Выводим результат измерения
		return result;
	}
	/**
	 * @brief Функция измерения скорости приёма тела сообщения
	 *
	 * @return результат измерения
	 *
	 */
	static awh::benchmark::result_t bodyThroughput() noexcept {
		// Результат измерения
		awh::benchmark::result_t result;
		// Итоги прогона
		transfer_t transfer;
		// Если прогон не выполнен
		if(!::body(false, transfer)){
			// Устанавливаем сведения о неудачном прогоне
			result.details = "прогон не выполнен: тело принято не полностью";
			// Выводим результат измерения
			return result;
		}
		// Вычисляем скорость приёма тела в мегабайтах в секунду
		result.value = ((transfer.seconds > 0.0) ? ((static_cast <double> (transfer.operations) / (1024.0 * 1024.0)) / transfer.seconds) : 0.0);
		// Буфер сведений о прогоне
		char details[256];
		// Формируем сведения о прогоне
		::snprintf(
			details, sizeof(details), "тело: %.1f МБ, кадров: %zu, кадров/с: %.0f",
			(static_cast <double> (transfer.operations) / (1024.0 * 1024.0)), transfer.frames,
			((transfer.seconds > 0.0) ? (static_cast <double> (transfer.frames) / transfer.seconds) : 0.0)
		);
		// Устанавливаем сведения о прогоне
		result.details = details;
		// Выводим результат измерения
		return result;
	}
	/**
	 * @brief Функция измерения скорости полного обмена
	 *
	 * @param streams количество одновременных потоков обмена
	 * @return        результат измерения
	 *
	 */
	static awh::benchmark::result_t roundtripThroughput(const size_t streams) noexcept {
		// Результат измерения
		awh::benchmark::result_t result;
		// Итоги прогона
		transfer_t transfer;
		// Если прогон не выполнен
		if(!::roundtrip(streams, transfer)){
			// Устанавливаем сведения о неудачном прогоне
			result.details = "прогон не выполнен: завершены не все обмены";
			// Выводим результат измерения
			return result;
		}
		// Вычисляем скорость полного обмена
		result.value = ((transfer.seconds > 0.0) ? (static_cast <double> (transfer.operations) / transfer.seconds) : 0.0);
		// Буфер сведений о прогоне
		char details[256];
		// Формируем сведения о прогоне
		::snprintf(
			details, sizeof(details), "обменов: %zu, потоков в группе: %zu, запросов/с: %.0f",
			transfer.operations, streams,
			((transfer.seconds > 0.0) ? (static_cast <double> (transfer.frames) / transfer.seconds) : 0.0)
		);
		// Устанавливаем сведения о прогоне
		result.details = details;
		// Выводим результат измерения
		return result;
	}
	/**
	 * @brief Функция измерения количества выделений памяти на разобранный запрос
	 *
	 * @return результат измерения
	 *
	 */
	static awh::benchmark::result_t requestAllocations() noexcept {
		// Результат измерения
		awh::benchmark::result_t result;
		// Итоги прогона
		transfer_t transfer;
		// Если прогон не выполнен
		if(!::requests(true, transfer)){
			// Устанавливаем сведения о неудачном прогоне
			result.details = "прогон не выполнен: разобраны не все запросы";
			// Выводим результат измерения
			return result;
		}
		// Вычисляем количество выделений памяти на запрос
		result.value = (static_cast <double> (transfer.allocations) / static_cast <double> (transfer.operations));
		// Буфер сведений о прогоне
		char details[256];
		// Формируем сведения о прогоне
		::snprintf(
			details, sizeof(details), "запросов: %zu, выделений: %zu, выделено: %.1f МБ",
			transfer.operations, transfer.allocations, (static_cast <double> (transfer.bytes) / (1024.0 * 1024.0))
		);
		// Устанавливаем сведения о прогоне
		result.details = details;
		// Выводим результат измерения
		return result;
	}
	/**
	 * @brief Функция измерения количества выделений памяти на кадр данных
	 *
	 * @return результат измерения
	 *
	 */
	static awh::benchmark::result_t bodyAllocations() noexcept {
		// Результат измерения
		awh::benchmark::result_t result;
		// Итоги прогона
		transfer_t transfer;
		// Если прогон не выполнен
		if(!::body(true, transfer)){
			// Устанавливаем сведения о неудачном прогоне
			result.details = "прогон не выполнен: тело принято не полностью";
			// Выводим результат измерения
			return result;
		}
		// Вычисляем количество выделений памяти на кадр данных
		result.value = (static_cast <double> (transfer.allocations) / static_cast <double> (transfer.frames));
		// Буфер сведений о прогоне
		char details[256];
		// Формируем сведения о прогоне
		::snprintf(
			details, sizeof(details), "кадров: %zu, выделений: %zu, выделено: %.1f МБ",
			transfer.frames, transfer.allocations, (static_cast <double> (transfer.bytes) / (1024.0 * 1024.0))
		);
		// Устанавливаем сведения о прогоне
		result.details = details;
		// Выводим результат измерения
		return result;
	}

	// Регистрируем сценарий разбора потока запросов
	static const bool gRequest = awh::benchmark::add(
		"http3/parse/request-stream", "запросов/с", REQUEST_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, &::requestThroughput
	);
	// Регистрируем сценарий приёма тела сообщения
	static const bool gBody = awh::benchmark::add(
		"http3/parse/data-body", "МБ/с", BODY_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, &::bodyThroughput
	);
	// Регистрируем сценарий полного обмена по одному потоку
	static const bool gRoundtrip = awh::benchmark::add(
		"http3/session/round-trip", "обменов/с", ROUNDTRIP_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, [](){ return ::roundtripThroughput(1); }
	);
	// Регистрируем сценарий полного обмена по множеству потоков
	static const bool gMultiplex = awh::benchmark::add(
		"http3/session/multiplexed", "обменов/с", MULTIPLEX_THRESHOLD,
		awh::benchmark::bound_t::MINIMUM, [](){ return ::roundtripThroughput(STREAM_COUNT); }
	);
	// Регистрируем сценарий количества выделений памяти на разобранный запрос
	static const bool gRequestAllocations = awh::benchmark::add(
		"http3/allocations/per-request", "выделений", REQUEST_ALLOCATIONS_THRESHOLD,
		awh::benchmark::bound_t::MAXIMUM, &::requestAllocations
	);
	// Регистрируем сценарий количества выделений памяти на кадр данных
	static const bool gBodyAllocations = awh::benchmark::add(
		"http3/allocations/per-data-frame", "выделений", BODY_ALLOCATIONS_THRESHOLD,
		awh::benchmark::bound_t::MAXIMUM, &::bodyAllocations
	);
};
