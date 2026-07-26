/**
 * @file: http2.cpp
 * @date: 2026-07-26
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @copyright: Copyright © 2026
 */

/**
 * Стандартные заголовочные файлы
 */
#include <string>
#include <vector>
#include <memory>
#include <random>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cstdint>
#include <algorithm>

/**
 * Подключаем заголовочный файл проекта
 */
#include <proto/http/parser/http2/http.hpp>

/**
 * Используем стандартное пространство имён
 */
using namespace std;

/**
 * Подписываемся на пространство имён HTTP-протокола
 */
using namespace awh::http;

/**
 * @brief Внутренние вспомогательные функции генератора (внутренняя компоновка)
 *
 */
namespace {
	/**
	 * @brief Функция сборки произвольного кадра HTTP/2
	 *
	 * @param type    тип кадра
	 * @param flags   флаги кадра
	 * @param sid     идентификатор потока
	 * @param payload полезная нагрузка кадра
	 * @return        собранный кадр
	 */
	string frame(const uint8_t type, const uint8_t flags, const uint32_t sid, const string & payload) noexcept {
		// Результат работы функции - собранный кадр
		string result;
		// Дописываем 24-битную длину полезной нагрузки
		result.push_back(static_cast <char> ((payload.size() >> 16) & 0xFF));
		// Дописываем средний байт длины
		result.push_back(static_cast <char> ((payload.size() >> 8) & 0xFF));
		// Дописываем младший байт длины
		result.push_back(static_cast <char> (payload.size() & 0xFF));
		// Дописываем тип кадра
		result.push_back(static_cast <char> (type));
		// Дописываем флаги кадра
		result.push_back(static_cast <char> (flags));
		// Дописываем старший байт идентификатора потока
		result.push_back(static_cast <char> ((sid >> 24) & 0xFF));
		// Дописываем второй байт идентификатора потока
		result.push_back(static_cast <char> ((sid >> 16) & 0xFF));
		// Дописываем третий байт идентификатора потока
		result.push_back(static_cast <char> ((sid >> 8) & 0xFF));
		// Дописываем младший байт идентификатора потока
		result.push_back(static_cast <char> (sid & 0xFF));
		// Дописываем полезную нагрузку кадра
		result.append(payload);
		// Выводим собранный кадр
		return result;
	}
	/**
	 * @brief Функция записи 32-битного числа в сетевом порядке байт
	 *
	 * @param value записываемое число
	 * @return      записанное число
	 */
	string u32(const uint32_t value) noexcept {
		// Результат работы функции - записанное число
		string result;
		// Дописываем старший байт числа
		result.push_back(static_cast <char> ((value >> 24) & 0xFF));
		// Дописываем второй байт числа
		result.push_back(static_cast <char> ((value >> 16) & 0xFF));
		// Дописываем третий байт числа
		result.push_back(static_cast <char> ((value >> 8) & 0xFF));
		// Дописываем младший байт числа
		result.push_back(static_cast <char> (value & 0xFF));
		// Выводим записанное число
		return result;
	}
};

/**
 * @brief Функция генератора нештатного трафика HTTP/2
 *
 * @details Генератор структурный: кадры формируются валидными для текущего состояния
 *          соединения (корректный preface, SETTINGS первым кадром, HEADERS с настоящим
 *          HPACK-блоком, DATA на открытых потоках, WINDOW_UPDATE с ненулевым инкрементом),
 *          и лишь часть из них портится точечной заменой байта. Без этого разбор
 *          обрывался бы на первом же кадре и глубокие состояния сессии не достигались.
 *
 *          Пользовательские функции обратного вызова намеренно ведут себя враждебно:
 *          изредка требуют сбросить поток и вызывают reset()/clear() прямо из обработчика.
 *          Именно этот класс реентрантности дважды давал дефекты работы с памятью.
 *
 * @param argc количество параметров командной строки
 * @param argv параметры командной строки
 * @return     код выхода процесса
 */
int32_t main(int32_t argc, char * argv[]) noexcept {
	// Количество итераций генератора (по две сессии на итерацию - клиент и сервер)
	const int32_t count = ((argc > 1) ? ::atoi(argv[1]) : 3000);
	// Создаём объект фреймворка
	unique_ptr <awh::fmk_t> fmk(new awh::fmk_t());
	// Создаём объект для работы с логами
	unique_ptr <awh::log_t> log(new awh::log_t(fmk.get()));
	// Отключаем вывод логов: генератор намеренно создаёт ошибочный трафик
	log->level(awh::log_t::level_t::NONE);
	// Инициализируем генератор псевдослучайных чисел фиксированным зерном (воспроизводимость)
	mt19937 rng(20260726);
	// Количество выполненных сессий
	size_t sessions = 0;
	// Количество сессий, переживших весь входящий поток кадров
	size_t deep = 0;
	// Количество сформированных кадров
	size_t frames = 0;
	// Количество испорченных кадров
	size_t corrupted = 0;
	/**
	 * Выполняем перебор всех итераций генератора
	 */
	for(int32_t iteration = 0; iteration < count; ++iteration){
		/**
		 * Выполняем перебор обеих ролей эндпоинта
		 */
		for(int32_t direct = 0; direct < 2; ++direct){
			// Определяем роль проверяемого эндпоинта
			const bool server = (direct != 0);
			// Создаём объект парсера проверяемой роли
			unique_ptr <parser_http2_t> parser(new parser_http2_t(server ? direct_t::REQUEST : direct_t::RESPONSE, fmk.get(), log.get()));
			// Устанавливаем функцию обратного вызова для обработки фрагмента тела потока
			parser->on(parser_http2_t::data_callback_t([&](const uint32_t, const void * buffer, const size_t size, const bool) noexcept -> bool {
				// Контрольная сумма тела потока
				volatile uint8_t sum = 0;
				/**
				 * Вычитываем всё тело: обращение к буферу ловит висячие представления
				 */
				for(size_t i = 0; i < size; ++i)
					// Накапливаем контрольную сумму тела
					sum = static_cast <uint8_t> (sum ^ static_cast <const uint8_t *> (buffer)[i]);
				// Изредка приложение полностью очищает парсер прямо из обработчика
				if((rng() % 256) == 0)
					// Выполняем очистку парсера
					parser->clear();
				// Изредка приложение требует сбросить поток
				return ((rng() % 64) != 0);
			}));
			// Устанавливаем функцию обратного вызова для обработки заголовков потока
			parser->on(parser_http2_t::header_callback_t([&](const uint32_t, const string_view name, const string_view value, const parser_t::part_t) noexcept -> bool {
				// Обращаемся к представлениям заголовка: они ссылаются в арену декодера
				volatile size_t length = (name.size() + value.size());
				// Не используемый результат
				(void) length;
				// Изредка приложение сбрасывает состояние соединения прямо из обработчика
				if((rng() % 256) == 0)
					// Выполняем сброс состояния соединения
					parser->reset();
				// Изредка приложение требует сбросить поток
				return ((rng() % 128) != 0);
			}));
			// Устанавливаем функцию обратного вызова для обработки открытия нового потока
			parser->on(parser_http2_t::begin_callback_t([&](const uint32_t) noexcept -> bool {
				// Изредка приложение отклоняет поток
				return ((rng() % 32) != 0);
			}));
			// Устанавливаем функцию обратного вызова для обработки фазы приёма сообщения
			parser->on(parser_http2_t::phase_callback_t([&](const uint32_t, const parser_t::phase_t, const parser_t::part_t) noexcept -> bool {
				// Изредка приложение сбрасывает состояние соединения прямо из обработчика
				if((rng() % 512) == 0)
					// Выполняем сброс состояния соединения
					parser->reset();
				// Изредка приложение требует сбросить поток
				return ((rng() % 128) != 0);
			}));
			// Устанавливаем функцию обратного вызова для обработки закрытия потока
			parser->on(parser_http2_t::close_callback_t([&](const uint32_t, const parser_http2_t::error_t) noexcept {
				// Изредка приложение сбрасывает состояние соединения прямо из обработчика
				if((rng() % 512) == 0)
					// Выполняем сброс состояния соединения
					parser->reset();
			}));
			// Устанавливаем функцию обратного вызова о готовности потока принимать данные
			parser->on(parser_http2_t::writable_callback_t([&](const uint32_t) noexcept {
				// Изредка приложение сбрасывает состояние соединения прямо из обработчика
				if((rng() % 256) == 0)
					// Выполняем сброс состояния соединения
					parser->reset();
			}));
			// Устанавливаем функцию обратного вызова для обработки анонса server push
			parser->on(parser_http2_t::push_callback_t([&](const uint32_t, const uint32_t) noexcept -> bool {
				// Изредка приложение отклоняет push
				return ((rng() % 8) != 0);
			}));
			// Получаем параметры SETTINGS парсера
			parser_http2_t::settings_t settings = parser->settings();
			// Разрешаем расширенный метод CONNECT (RFC 8441)
			settings.enableConnectProtocol = 1;
			// Применяем параметры SETTINGS парсера
			parser->settings(settings);
			// Отправляем свой connection preface
			parser->sendPreface();
			// Создаём объект кодера заголовков, синхронный с декодером парсера
			awh::http::h2::hpack::encoder_t encoder;
			// Буфер формируемого входящего потока байт
			string input;
			// Если проверяется сервер - дописываем клиентскую magic-строку
			if(server)
				// Дописываем magic-строку connection preface
				input.append(awh::http::h2::proto::PREFACE.data(), awh::http::h2::proto::PREFACE.size());
			// Дописываем корректный SETTINGS первым кадром (иначе разбор оборвётся сразу)
			input.append(::frame(0x04, 0x00, 0, ""));
			// Идентификатор следующего потока, инициируемого пиром
			uint32_t peer = (server ? 1 : 2);
			// Список открытых пиром потоков
			vector <uint32_t> open;
			// Идентификатор ассоциированного потока для анонса server push
			uint32_t assoc = 0;
			// Количество кадров в текущей сессии
			const int32_t total = (4 + static_cast <int32_t> (rng() % 20));
			/**
			 * Выполняем формирование всех кадров сессии
			 */
			for(int32_t i = 0; i < total; ++i){
				// Буфер формируемого кадра
				string current;
				/**
				 * Выбираем тип формируемого кадра
				 */
				switch(rng() % 12){
					// Блок заголовков, открывающий новый поток
					case 0: case 1: case 2: {
						// Список заголовков блока
						vector <awh::http::h2::hpack::field_t> fields;
						// Если формируется запрос клиента
						if(server){
							// Дописываем псевдо-заголовок метода запроса
							fields.emplace_back(":method", (((rng() & 1) != 0) ? "GET" : "POST"));
							// Дописываем псевдо-заголовок схемы запроса
							fields.emplace_back(":scheme", "https");
							// Дописываем псевдо-заголовок пути запроса
							fields.emplace_back(":path", "/x");
							// Дописываем псевдо-заголовок авторитета запроса
							fields.emplace_back(":authority", "example.com");
						// Иначе формируем ответ сервера
						} else fields.emplace_back(":status", "200");
						// Дописываем обычный заголовок переменной длины
						fields.emplace_back("x-n", string(1 + (rng() % 64), 'q'));
						// Буфер закодированного блока заголовков
						string block;
						// Кодируем блок заголовков
						encoder.encode(fields, block, ((rng() & 1) != 0));
						// Определяем признак завершения потока блоком заголовков
						const bool endStream = ((rng() % 3) == 0);
						// Собираем флаги кадра
						const uint8_t flags = static_cast <uint8_t> (0x04 | (endStream ? 0x01 : 0x00));
						// Идентификатор потока формируемого кадра
						uint32_t sid = 0;
						// Если формируется ответ на уже открытый поток
						if(!server && ((rng() % 3) != 0) && !open.empty())
							// Выбираем случайный открытый поток
							sid = open[rng() % open.size()];
						// Иначе открываем новый поток пира
						else {
							// Выделяем идентификатор нового потока пира
							sid = peer;
							// Смещаем идентификатор следующего потока пира
							peer += 2;
						}
						// Если поток не завершается - запоминаем его как открытый
						if(!endStream)
							// Добавляем поток в список открытых
							open.push_back(sid);
						// Запоминаем поток как ассоциированный для анонса push
						assoc = sid;
						// Изредка режем блок на HEADERS и CONTINUATION
						if(((rng() % 4) == 0) && (block.size() > 4)){
							// Определяем точку разреза блока заголовков
							const size_t cut = (1 + (rng() % (block.size() - 1)));
							// Дописываем кадр HEADERS без завершения блока
							current = ::frame(0x01, static_cast <uint8_t> (flags & ~0x04), sid, block.substr(0, cut));
							// Дописываем кадр CONTINUATION с завершением блока
							current += ::frame(0x09, 0x04, sid, block.substr(cut));
						// Иначе формируем блок одним кадром
						} else current = ::frame(0x01, flags, sid, block);
					} break;
					// Данные тела на открытом потоке
					case 3: case 4: {
						// Если открытых потоков нет - кадр не формируется
						if(open.empty())
							// Прекращаем формирование кадра
							break;
						// Выбираем случайный открытый поток
						const uint32_t sid = open[rng() % open.size()];
						// Определяем признак завершения потока
						const bool endStream = ((rng() % 4) == 0);
						// Формируем тело потока
						const string body(rng() % 512, 'd');
						// Изредка добавляем заполнитель
						if((rng() % 5) == 0){
							// Формируем заполнитель
							const string pad(rng() % 16, 0);
							// Буфер полезной нагрузки кадра
							string payload;
							// Дописываем длину заполнителя
							payload.push_back(static_cast <char> (pad.size()));
							// Дописываем тело потока
							payload += body;
							// Дописываем заполнитель
							payload += pad;
							// Формируем кадр DATA с заполнителем
							current = ::frame(0x00, static_cast <uint8_t> (0x08 | (endStream ? 0x01 : 0x00)), sid, payload);
						// Иначе формируем кадр без заполнителя
						} else current = ::frame(0x00, (endStream ? 0x01 : 0x00), sid, body);
					} break;
					// Обновление окна соединения
					case 5: current = ::frame(0x08, 0x00, 0, ::u32(1 + (rng() % 65535))); break;
					// Обновление окна потока
					case 6: {
						// Если открытых потоков нет - кадр не формируется
						if(open.empty())
							// Прекращаем формирование кадра
							break;
						// Формируем кадр обновления окна случайного открытого потока
						current = ::frame(0x08, 0x00, open[rng() % open.size()], ::u32(1 + (rng() % 65535)));
					} break;
					// Проверка живости соединения
					case 7: current = ::frame(0x06, 0x00, 0, string(8, static_cast <char> (rng() & 0xFF))); break;
					// Аварийное закрытие потока
					case 8: {
						// Если открытых потоков нет - кадр не формируется
						if(open.empty())
							// Прекращаем формирование кадра
							break;
						// Выбираем случайный открытый поток
						const size_t index = (rng() % open.size());
						// Формируем кадр аварийного закрытия потока
						current = ::frame(0x03, 0x00, open[index], ::u32(8));
						// Удаляем поток из списка открытых
						open.erase(open.begin() + index);
					} break;
					// Параметры соединения
					case 9: {
						// Буфер полезной нагрузки кадра
						string payload;
						// Определяем количество передаваемых параметров
						const int32_t items = (1 + static_cast <int32_t> (rng() % 4));
						/**
						 * Выполняем формирование всех параметров
						 */
						for(int32_t j = 0; j < items; ++j){
							// Выбираем идентификатор параметра
							const uint16_t id = static_cast <uint16_t> (1 + (rng() % 9));
							// Выбираем значение параметра
							uint32_t value = rng();
							// Начальное окно потока обязано укладываться в допустимый диапазон
							if(id == 4)
								// Ограничиваем значение параметра
								value %= 0x7FFFFFFF;
							// Максимальный размер кадра обязан укладываться в допустимый диапазон
							if(id == 5)
								// Ограничиваем значение параметра
								value = (16384 + (rng() % 100000));
							// Булевы параметры принимают только 0 и 1
							if((id == 2) || (id == 8) || (id == 9))
								// Ограничиваем значение параметра
								value %= 2;
							// Дописываем старший байт идентификатора параметра
							payload.push_back(static_cast <char> (id >> 8));
							// Дописываем младший байт идентификатора параметра
							payload.push_back(static_cast <char> (id & 0xFF));
							// Дописываем значение параметра
							payload += ::u32(value);
						}
						// Формируем кадр параметров соединения
						current = ::frame(0x04, 0x00, 0, payload);
					} break;
					// Обновление расширенного приоритета потока
					case 10: {
						// Буфер полезной нагрузки кадра
						string payload;
						// Выбираем приоритизируемый поток
						const uint32_t target = (server ? (1 + 2 * (rng() % 3)) : (2 + 2 * (rng() % 3)));
						// Дописываем идентификатор приоритизируемого потока
						payload += ::u32(target);
						// Таблица проверяемых значений поля приоритета
						static const char * values[] = {"u=0", "u=7, i", "i", "u=3", "garbage", "u=9", "", "u=2, i=?0"};
						// Дописываем значение поля приоритета
						payload += values[rng() % 8];
						// Формируем кадр обновления приоритета
						current = ::frame(0x10, 0x00, 0, payload);
					} break;
					// Запрос расширенного метода CONNECT
					case 11: {
						// Список заголовков блока
						vector <awh::http::h2::hpack::field_t> fields;
						// Если формируется запрос клиента
						if(server){
							// Дописываем псевдо-заголовок метода запроса
							fields.emplace_back(":method", "CONNECT");
							// Дописываем псевдо-заголовок схемы запроса
							fields.emplace_back(":scheme", "https");
							// Дописываем псевдо-заголовок пути запроса
							fields.emplace_back(":path", "/ws");
							// Дописываем псевдо-заголовок авторитета запроса
							fields.emplace_back(":authority", "example.com");
							// Дописываем псевдо-заголовок протокола туннеля
							fields.emplace_back(":protocol", "websocket");
						// Иначе формируем ответ сервера
						} else fields.emplace_back(":status", "200");
						// Буфер закодированного блока заголовков
						string block;
						// Кодируем блок заголовков
						encoder.encode(fields, block, ((rng() & 1) != 0));
						// Выделяем идентификатор нового потока пира
						const uint32_t sid = peer;
						// Смещаем идентификатор следующего потока пира
						peer += 2;
						// Запоминаем поток как открытый
						open.push_back(sid);
						// Формируем кадр блока заголовков
						current = ::frame(0x01, 0x04, sid, block);
					} break;
				}
				// Если кадр не сформирован - переходим к следующему
				if(current.empty())
					// Переходим к следующему кадру
					continue;
				// Изредка портим случайный байт кадра
				if((rng() % 12) == 0){
					// Заменяем случайный байт кадра
					current[rng() % current.size()] = static_cast <char> (rng() & 0xFF);
					// Наращиваем счётчик испорченных кадров
					++corrupted;
				}
				// Дописываем кадр во входящий поток
				input += current;
				// Наращиваем счётчик сформированных кадров
				++frames;
			}
			// Текущая позиция подачи входящего потока
			size_t position = 0;
			/**
			 * Подаём входящий поток рваными кусками: разбор обязан переживать
			 * произвольную фрагментацию, включая разрыв посреди заголовка кадра
			 */
			while(position < input.size()){
				// Вычисляем размер очередного куска
				const size_t size = ::min <size_t> (1 + (rng() % 29), input.size() - position);
				// Выполняем разбор очередного куска
				parser->parse(input.data() + position, size);
				// Сдвигаем позицию подачи
				position += size;
			}
			// Если разбор входящего потока не оборвался ошибкой соединения
			if(parser->status() != parser_t::status_t::ERROR)
				// Наращиваем счётчик выживших сессий
				++deep;
			// Выделяем идентификатор нашего собственного потока
			const uint32_t own = parser->nextStreamId();
			// Если идентификатор выделен
			if(own != 0){
				// Список заголовков исходящего сообщения
				vector <awh::http::h2::hpack::field_t> fields;
				// Если проверяется сервер - формируем ответ
				if(server)
					// Дописываем псевдо-заголовок статуса ответа
					fields.emplace_back(":status", "200");
				// Иначе формируем запрос
				else {
					// Дописываем псевдо-заголовок метода запроса
					fields.emplace_back(":method", "POST");
					// Дописываем псевдо-заголовок схемы запроса
					fields.emplace_back(":scheme", "https");
					// Дописываем псевдо-заголовок пути запроса
					fields.emplace_back(":path", "/");
				}
				// Отправляем блок заголовков исходящего сообщения
				parser->sendHeaders(own, fields, false);
				// Формируем тело исходящего сообщения
				const string body(rng() % 8192, 'z');
				// Текущая позиция отправки тела
				size_t offset = 0;
				/**
				 * Отправляем тело порциями, пока парсер их принимает
				 */
				while(offset < body.size()){
					// Передаём очередную порцию тела
					const size_t taken = parser->sendData(own, body.data() + offset, body.size() - offset, (((offset + 4096) >= body.size()) && ((rng() % 2) == 0)));
					// Если порция не принята - прекращаем отправку
					if(taken == 0)
						// Прекращаем отправку тела
						break;
					// Сдвигаем позицию отправки
					offset += taken;
				}
				// Изредка завершаем поток секцией трейлеров вместо флага на теле
				if((rng() % 3) == 0){
					// Список заголовков секции трейлеров
					vector <awh::http::h2::hpack::field_t> trailers;
					// Дописываем заголовок секции трейлеров
					trailers.emplace_back("x-checksum", string(1 + (rng() % 32), 'c'));
					// Отправляем секцию трейлеров с завершением потока
					parser->sendHeaders(own, trailers, true);
				}
				// Изредка анонсируем server push
				if(server && (assoc != 0) && ((rng() % 3) == 0)){
					// Список заголовков обещанного запроса
					vector <awh::http::h2::hpack::field_t> promise;
					// Дописываем псевдо-заголовок метода обещанного запроса
					promise.emplace_back(":method", "GET");
					// Дописываем псевдо-заголовок схемы обещанного запроса
					promise.emplace_back(":scheme", "https");
					// Дописываем псевдо-заголовок пути обещанного запроса
					promise.emplace_back(":path", "/push");
					// Дописываем псевдо-заголовок авторитета обещанного запроса
					promise.emplace_back(":authority", "example.com");
					// Отправляем анонс server push
					parser->sendPushPromise(assoc, promise);
				}
				// Изредка выдаём тело второго потока pull-источником данных
				if((rng() % 3) == 0){
					// Выделяем идентификатор второго собственного потока
					const uint32_t pull = parser->nextStreamId();
					// Если идентификатор выделен
					if(pull != 0){
						// Отправляем блок заголовков второго потока
						parser->sendHeaders(pull, fields, false);
						// Остаток тела, выдаваемого источником
						shared_ptr <size_t> rest(new size_t(1 + (rng() % 20000)));
						/**
						 * Назначаем pull-источник данных тела: источник вызывается для живого
						 * объекта потока, поэтому его закрытие и сброс парсера прямо изнутри -
						 * отдельный класс реентерабельности
						 */
						parser->dataSource(pull, [&, rest](const uint32_t id, uint8_t * buffer, const size_t cap, bool & eof) noexcept -> int64_t {
							// Изредка приложение сбрасывает поток прямо из источника
							if((rng() % 16) == 0){
								// Сбрасываем собственный поток
								parser->sendRstStream(id, parser_http2_t::error_t::CANCEL);
								// Помечаем достижение конца тела
								eof = true;
								// Данных нет
								return 0;
							}
							// Изредка приложение сбрасывает состояние соединения прямо из источника
							if((rng() % 64) == 0){
								// Выполняем сброс состояния соединения
								parser->reset();
								// Помечаем достижение конца тела
								eof = true;
								// Данных нет
								return 0;
							}
							// Изредка источник нарушает контракт, объявляя больше записанного
							if((rng() % 128) == 0)
								// Выводим заведомо недопустимое число записанных байт
								return static_cast <int64_t> (cap + 1);
							// Вычисляем размер очередной порции тела
							const size_t chunk = ::min(cap, * rest);
							// Заполняем очередную порцию тела
							::memset(buffer, 'p', chunk);
							// Уменьшаем остаток тела
							(* rest) -= chunk;
							// Помечаем достижение конца тела
							eof = ((* rest) == 0);
							// Выводим число записанных байт
							return static_cast <int64_t> (chunk);
						});
					}
				}
				// Освобождаем половину накопленных исходящих байт (pull-модель)
				parser->consumePending(parser->pending().size() / 2);
			}
			// Уведомляем парсер о завершении потока данных
			parser->eof();
			// Наращиваем счётчик выполненных сессий
			++sessions;
		}
	}
	// Выводим итоговую статистику генератора
	::printf(
		"http2 fuzz: %zu sessions, %zu frames (%zu corrupted), %zu sessions survived the whole stream\n",
		sessions, frames, corrupted, deep
	);
	// Выводим успешный код выхода
	return EXIT_SUCCESS;
}
