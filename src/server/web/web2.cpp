/**
 * @file: web2.cpp
 * @date: 2022-10-01
 * @license: GPL-3.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @copyright: Copyright © 2022
 */

// Подключаем заголовочный файл
#include <server/web/web.hpp>

/**
 * eventsCallback Функция обратного вызова при активации ядра сервера
 * @param status флаг запуска/остановки
 * @param core   объект сетевого ядра
 */
void awh::server::Web2::eventsCallback(const awh::core_t::status_t status, awh::core_t * core) noexcept {
	// Если данные существуют
	if(core != nullptr){
		// Если система была остановлена
		if(status == awh::core_t::status_t::STOP){
			// Если список сессий не пустой
			if(!this->_sessions.empty())
				// Выполняем очистку списка активных сессий
				this->_sessions.clear();
		}
		// Выполняем переадресацию выполняемого события в родительский модуль
		web_t::eventsCallback(status, core);
	}
}
/**
 * connectCallback Метод обратного вызова при подключении к серверу
 * @param bid  идентификатор брокера
 * @param sid  идентификатор схемы сети
 * @param core объект сетевого ядра
 */
void awh::server::Web2::connectCallback(const uint64_t bid, const uint16_t sid, awh::core_t * core) noexcept {
	// Если флаг инициализации сессии HTTP/2 не активирован, но протокол HTTP/2 поддерживается сервером
	if((this->_sessions.count(bid) == 0) && (core->proto(bid) == engine_t::proto_t::HTTP2)){
		// Если список параметров настроек не пустой
		if(!this->_settings.empty()){
			// Создаём параметры сессии и активируем запрет использования той самой неудачной системы приоритизации из первой итерации HTTP/2.
			vector <nghttp2_settings_entry> iv = {{NGHTTP2_SETTINGS_NO_RFC7540_PRIORITIES, 1}};
			// Выполняем переход по всему списку настроек
			for(auto & setting : this->_settings){
				// Определяем тип настройки
				switch(static_cast <uint8_t> (setting.first)){
					// Если мы получили разрешение присылать push-уведомления
					case static_cast <uint8_t> (settings_t::ENABLE_PUSH):
						// Устанавливаем разрешение присылать push-уведомления
						iv.push_back({NGHTTP2_SETTINGS_ENABLE_PUSH, setting.second});
					break;
					// Если мы получили максимальный размер фрейма
					case static_cast <uint8_t> (settings_t::FRAME_SIZE):
						/**
						 * Устанавливаем максимальный размер кадра в октетах, который собеседнику разрешено отправлять.
						 * Значение по умолчанию, оно же минимальное — 16 384=214 октетов.
						 */
						iv.push_back({NGHTTP2_SETTINGS_MAX_FRAME_SIZE, setting.second});
					break;
					// Если мы получили максимальный размер таблицы заголовков
					case static_cast <uint8_t> (settings_t::HEADER_TABLE_SIZE):
						// Устанавливаем максимальный размер таблицы заголовков
						iv.push_back({NGHTTP2_SETTINGS_HEADER_TABLE_SIZE, setting.second});
					break;
					// Если мы получили максимальный размер окна полезной нагрузки
					case static_cast <uint8_t> (settings_t::WINDOW_SIZE):
						// Устанавливаем элемент управления потоком (flow control)
						iv.push_back({NGHTTP2_SETTINGS_INITIAL_WINDOW_SIZE, setting.second});
					break;
					// Если мы получили максимальное количество потоков
					case static_cast <uint8_t> (settings_t::STREAMS):
						/**
						 * Устанавливаем максимальное количество потоков, которое собеседнику разрешается использовать одновременно.
						 * Считаются только открытые потоки, то есть по которым что-то ещё передаётся.
						 * Можно указать значение 0: тогда собеседник не сможет отправлять новые сообщения, пока сторона его снова не увеличит.
						 */
						iv.push_back({NGHTTP2_SETTINGS_MAX_CONCURRENT_STREAMS, setting.second});
					break;
					// Если мы получили разрешение использования метода CONNECT
					case static_cast <uint8_t> (settings_t::CONNECT):
						// Устанавливаем разрешение применения метода CONNECT
						iv.push_back({NGHTTP2_SETTINGS_ENABLE_CONNECT_PROTOCOL, setting.second});
					break;
				}
			}
			// Выполняем создание нового объекта сессии HTTP/2
			auto ret = this->_sessions.emplace(bid, unique_ptr <http2_t> (new http2_t(this->_fmk, this->_log)));
			// Выполняем установку функции обратного вызова начала открытии потока
			ret.first->second->on((function <int (const int32_t)>) std::bind(&web2_t::beginSignal, this, _1, bid));
			// Выполняем установку функции обратного вызова при отправки сообщения клиенту
			ret.first->second->on((function <void (const uint8_t *, const size_t)>) std::bind(&web2_t::sendSignal, this, bid, _1, _2));
			// Выполняем установку функции обратного вызова при создании нового фрейма клиента
			ret.first->second->on((function <int (const int32_t, const http2_t::frame_t)>) std::bind(&web2_t::createSignal, this, _1, bid, _2));
			// Выполняем установку функции обратного вызова при закрытии потока
			ret.first->second->on((function <int (const int32_t, const http2_t::error_t)>) std::bind(&web2_t::closedSignal, this, _1, bid, _2));
			// Выполняем установку функции обратного вызова при получении чанка с сервера
			ret.first->second->on((function <int (const int32_t, const uint8_t *, const size_t)>) std::bind(&web2_t::chunkSignal, this, _1, bid, _2, _3));
			// Выполняем установку функции обратного вызова при получении данных заголовка
			ret.first->second->on((function <int (const int32_t, const string &, const string &)>) std::bind(&web2_t::headerSignal, this, _1, bid, _2, _3));
			// Выполняем установку функции обратного вызова получения фрейма HTTP/2
			ret.first->second->on((function <int (const int32_t, const http2_t::direct_t, const http2_t::frame_t, const set <http2_t::flag_t> &)>) std::bind(&web2_t::frameSignal, this, _1, bid, _2, _3, _4));
			// Если функция обратного вызова на на вывод ошибок установлена
			if(this->_callback.is("error"))
				// Выполняем установку функции обратного вызова на событие получения ошибки
				ret.first->second->on(std::bind(this->_callback.get <void (const uint64_t, const log_t::flag_t, const http::error_t, const string &)> ("error"), bid, _1, _2, _3));
			// Если инициализация модуля NgHttp2 не выполнена
			if(!ret.first->second->init(http2_t::mode_t::SERVER, std::move(iv)))
				// Выполняем удаление созданного ранее объекта
				this->_sessions.erase(ret.first);
		}
	}
}
/**
 * sendSignal Метод обратного вызова при отправки данных HTTP/2
 * @param bid    идентификатор брокера
 * @param buffer буфер бинарных данных
 * @param size   размер буфера данных для отправки
 */
void awh::server::Web2::sendSignal(const uint64_t bid, const uint8_t * buffer, const size_t size) noexcept {
	// Если объект сетевого ядра инициализирован
	if(this->_core != nullptr)
		// Выполняем отправку заголовков ответа клиенту
		const_cast <server::core_t *> (this->_core)->write((const char *) buffer, size, bid);
}
/**
 * ping Метод выполнения пинга клиента
 * @param bid идентификатор брокера
 * @return    результат работы пинга
 */
bool awh::server::Web2::ping(const uint64_t bid) noexcept {
	// Выполняем поиск брокера в списке активных сессий
	auto it = this->_sessions.find(bid);
	// Если активная сессия найдена
	if(it != this->_sessions.end())
		// Выполняем пинг удалённого сервера
		return it->second->ping();
	// Выводим результат
	return false;
}
/**
 * shutdown Метод отправки клиенту сообщения корректного завершения
 * @param bid идентификатор брокера
 * @return    результат выполнения операции
 */
bool awh::server::Web2::shutdown(const uint64_t bid) noexcept {
	// Результат работы функции
	bool result = false;
	// Если флаг инициализации сессии HTTP/2 установлен и подключение выполнено
	if((this->_core != nullptr) && this->_core->working()){
		// Выполняем поиск брокера в списке активных сессий
		auto it = this->_sessions.find(bid);
		// Если активная сессия найдена
		if((result = (it != this->_sessions.end()))){
			// Выполняем закрытие полное закрытие подключения клиента
			if(!(result = it->second->shutdown())){
				// Выполняем закрытие подключения
				const_cast <server::core_t *> (this->_core)->close(bid);
				// Выходим из функции
				return result;
			}
		}
	}
	// Выводим результат
	return result;
}
/**
 * reject Метод выполнения сброса подключения
 * @param id    идентификатор потока
 * @param bid   идентификатор брокера
 * @param error код отправляемой ошибки
 * @return      результат отправки сообщения
 */
bool awh::server::Web2::reject(const int32_t id, const uint64_t bid, const http2_t::error_t error) noexcept {
	// Результат работы функции
	bool result = false;
	// Если флаг инициализации сессии HTTP/2 установлен и подключение выполнено
	if((this->_core != nullptr) && this->_core->working()){
		// Выполняем поиск брокера в списке активных сессий
		auto it = this->_sessions.find(bid);
		// Если активная сессия найдена
		if((result = (it != this->_sessions.end()))){
			// Выполняем закрытие подключения клиента
			if(!(result = it->second->reject(id, error))){
				// Выполняем закрытие подключения
				const_cast <server::core_t *> (this->_core)->close(bid);
				// Выходим из функции
				return result;
			}
		}
	}
	// Выводим результат
	return result;
}
/**
 * goaway Метод отправки сообщения закрытия всех потоков
 * @param last   идентификатор последнего потока
 * @param bid    идентификатор брокера
 * @param error  код отправляемой ошибки
 * @param buffer буфер отправляемых данных если требуется
 * @param size   размер отправляемого буфера данных
 * @return       результат отправки данных фрейма
 */
bool awh::server::Web2::goaway(const int32_t last, const uint64_t bid, const http2_t::error_t error, const uint8_t * buffer, const size_t size) noexcept {
	// Результат работы функции
	bool result = false;
	// Если флаг инициализации сессии HTTP/2 установлен и подключение выполнено
	if((this->_core != nullptr) && this->_core->working() && (last > -1)){
		// Выполняем поиск брокера в списке активных сессий
		auto it = this->_sessions.find(bid);
		// Если активная сессия найдена
		if((result = (it != this->_sessions.end()))){
			// Выполняем отправку требования клиенту выполнить отключение от сервера
			if(!(result = it->second->goaway(last, error, buffer, size))){
				// Выполняем закрытие подключения
				const_cast <server::core_t *> (this->_core)->close(bid);
				// Выходим из функции
				return result;
			}
		}
	}
	// Выводим результат
	return result;
}
/**
 * sendOrigin Метод отправки списка разрешённых источников
 * @param bid     идентификатор брокера
 * @param origins список разрешённых источников
 * @return        результат отправки данных фрейма
 */
bool awh::server::Web2::sendOrigin(const uint64_t bid, const vector <string> & origins) noexcept {
	// Результат работы функции
	bool result = false;
	// Выполняем поиск брокера в списке активных сессий
	auto it = this->_sessions.find(bid);
	// Если активная сессия найдена
	if(it != this->_sessions.end()){
		// Если список разрешённых источников отправить не удалось
		if((this->_core != nullptr) && !(result = it->second->sendOrigin(origins))){
			// Выполняем закрытие подключения
			const_cast <server::core_t *> (this->_core)->close(bid);
			// Выходим из функции
			return result;
		}
	}
	// Выводим результат
	return result;
}
/**
 * sendAltSvc Метод отправки расширения альтернативного сервиса RFC7383
 * @param id     идентификатор потока
 * @param bid    идентификатор брокера
 * @param origin название сервиса
 * @param field  поле сервиса
 * @return       результат отправки расширения
 */
bool awh::server::Web2::sendAltSvc(const int32_t id, const uint64_t bid, const string & origin, const string & field) noexcept {
	// Результат работы функции
	bool result = false;
	// Если флаг инициализации сессии HTTP/2 установлен и подключение выполнено
	if((this->_core != nullptr) && this->_core->working()){
		// Выполняем поиск брокера в списке активных сессий
		auto it = this->_sessions.find(bid);
		// Если активная сессия найдена
		if((result = (it != this->_sessions.end()))){
			// Выполняем отправку расширения клиенту с требованием подключиться к другому протоколу
			if(!(result = it->second->sendAltSvc(id, origin, field))){
				// Выполняем закрытие подключения
				const_cast <server::core_t *> (this->_core)->close(bid);
				// Выходим из функции
				return result;
			}
		}
	}
	// Выводим результат
	return result;
}
/**
 * send Метод отправки трейлеров
 * @param id      идентификатор потока
 * @param bid     идентификатор брокера
 * @param headers заголовки отправляемые
 * @return        результат отправки данных указанному клиенту
 */
bool awh::server::Web2::send(const int32_t id, const uint64_t bid, const vector <pair <string, string>> & headers) noexcept {
	// Результат работы функции
	bool result = false;
	// Если флаг инициализации сессии HTTP/2 установлен и подключение выполнено
	if((this->_core != nullptr) && this->_core->working() && !headers.empty()){
		// Выполняем поиск брокера в списке активных сессий
		auto it = this->_sessions.find(bid);
		// Если активная сессия найдена
		if((result = (it != this->_sessions.end()))){
			// Выполняем отправку трейлеров
			if(!(result = it->second->sendTrailers(id, headers))){
				// Выполняем закрытие подключения
				const_cast <server::core_t *> (this->_core)->close(bid);
				// Выходим из функции
				return result;
			}
		}
	}
	// Выводим результат
	return result;
}
/**
 * send Метод отправки сообщения клиенту
 * @param id     идентификатор потока
 * @param bid    идентификатор брокера
 * @param buffer буфер бинарных данных передаваемых
 * @param size   размер сообщения в байтах
 * @param flag   флаг передаваемого потока по сети
 * @return       результат отправки данных указанному клиенту
 */
bool awh::server::Web2::send(const int32_t id, const uint64_t bid, const char * buffer, const size_t size, const http2_t::flag_t flag) noexcept {
	// Результат работы функции
	bool result = false;
	// Если флаг инициализации сессии HTTP/2 установлен и подключение выполнено
	if((this->_core != nullptr) && this->_core->working() && (buffer != nullptr) && (size > 0)){
		// Выполняем поиск брокера в списке активных сессий
		auto it = this->_sessions.find(bid);
		// Если активная сессия найдена
		if((result = (it != this->_sessions.end()))){
			// Выполняем отправку тела ответа
			if(!(result = it->second->sendData(id, reinterpret_cast <const uint8_t *> (buffer), size, flag))){
				// Выполняем закрытие подключения
				const_cast <server::core_t *> (this->_core)->close(bid);
				// Выходим из функции
				return result;
			}
		}
	}
	// Выводим результат
	return result;
}
/**
 * send Метод отправки заголовков
 * @param id      идентификатор потока
 * @param bid     идентификатор брокера
 * @param headers заголовки отправляемые
 * @param flag    флаг передаваемого потока по сети
 * @return        флаг последнего сообщения после которого поток закрывается
 */
int32_t awh::server::Web2::send(const int32_t id, const uint64_t bid, const vector <pair <string, string>> & headers, const http2_t::flag_t flag) noexcept {
	// Результат работы функции
	int32_t result = -1;
	// Если флаг инициализации сессии HTTP/2 установлен и подключение выполнено
	if((this->_core != nullptr) && this->_core->working() && !headers.empty()){
		// Выполняем поиск брокера в списке активных сессий
		auto it = this->_sessions.find(bid);
		// Если активная сессия найдена
		if(it != this->_sessions.end()){
			// Если ответ не получилось отправить
			if((result = it->second->sendHeaders(id, headers, flag)) < 0){
				// Выполняем закрытие подключения
				const_cast <server::core_t *> (this->_core)->close(bid);
				// Выходим из функции
				return result;
			}
		}
	}
	// Выводим результат
	return result;
}
/**
 * push Метод отправки push-уведомлений
 * @param id      идентификатор потока
 * @param bid     идентификатор брокера
 * @param headers заголовки отправляемые
 * @param flag    флаг передаваемого потока по сети
 * @return        флаг последнего сообщения после которого поток закрывается
 */
int32_t awh::server::Web2::push(const int32_t id, const uint64_t bid, const vector <pair <string, string>> & headers, const http2_t::flag_t flag) noexcept {
	// Результат работы функции
	int32_t result = -1;
	// Если флаг инициализации сессии HTTP/2 установлен и подключение выполнено
	if((this->_core != nullptr) && this->_core->working() && !headers.empty()){
		// Выполняем поиск брокера в списке активных сессий
		auto it = this->_sessions.find(bid);
		// Если активная сессия найдена
		if(it != this->_sessions.end()){
			// Если ответ не получилось отправить push-уведомление
			if((result = it->second->sendPush(id, headers, flag)) < 0)
				// Выходим из функции
				return result;
		}
	}
	// Выводим результат
	return result;
}
/**
 * addOrigin Метод добавления разрешённого источника
 * @param origin разрешённый источнико
 */
void awh::server::Web2::addOrigin(const string & origin) noexcept {
	// Если разрешённый источник получен
	if(!origin.empty())
		// Выполняем добавления разрешённого источника
		this->_origins.push_back(origin);
}
/**
 * setOrigin Метод установки списка разрешённых источников
 * @param origins список разрешённых источников
 */
void awh::server::Web2::setOrigin(const vector <string> & origins) noexcept {
	// Выполняем установку списка разрешённых источников
	this->_origins.assign(origins.begin(), origins.end());
}
/**
 * addAltSvc Метод добавления альтернативного сервиса
 * @param origin название альтернативного сервиса
 * @param field  поле альтернативного сервиса
 */
void awh::server::Web2::addAltSvc(const string & origin, const string & field) noexcept {
	// Если данные альтернативного сервиса переданы
	if(!origin.empty() && !field.empty())
		// Выполняем добавление альтернативного сервиса
		this->_altsvc.emplace(origin, field);
}
/**
 * setAltSvc Метод установки списка разрешённых источников
 * @param origins список альтернативных сервисов
 */
void awh::server::Web2::setAltSvc(const unordered_multimap <string, string> & origins) noexcept {
	// Выполняем установку списка разрешённых источников
	this->_altsvc = origins;
}
/**
 * settings Модуль установки настроек протокола HTTP/2
 * @param settings список настроек протокола HTTP/2
 */
void awh::server::Web2::settings(const map <settings_t, uint32_t> & settings) noexcept {
	// Если список настроек протокола HTTP/2 передан
	if(!settings.empty())
		// Выполняем установку списка настроек
		this->_settings = settings;
	// Если максимальное количество потоков не установлено
	if(this->_settings.count(settings_t::STREAMS) == 0)
		// Выполняем установку максимального количества потоков
		this->_settings.emplace(settings_t::STREAMS, CONCURRENT_STREAMS);
	// Если максимальный размер фрейма не установлен
	if(this->_settings.count(settings_t::FRAME_SIZE) == 0)
		// Выполняем установку максимального размера фрейма
		this->_settings.emplace(settings_t::FRAME_SIZE, MAX_FRAME_SIZE_MIN);
	// Если максимальный размер фрейма установлен
	else {
		// Выполняем извлечение максимального размера фрейма
		auto it = this->_settings.find(settings_t::FRAME_SIZE);
		// Если максимальный размер фрейма больше самого максимального значения
		if(it->second > MAX_FRAME_SIZE_MAX)
			// Выполняем корректировку максимального размера фрейма
			it->second = MAX_FRAME_SIZE_MAX;
		// Если максимальный размер фрейма меньше самого минимального значения
		else if(it->second < MAX_FRAME_SIZE_MIN)
			// Выполняем корректировку максимального размера фрейма
			it->second = MAX_FRAME_SIZE_MIN;
	}
	// Если максимальный размер окна фрейма не установлен
	if(this->_settings.count(settings_t::WINDOW_SIZE) == 0)
		// Выполняем установку максимального размера окна фрейма
		this->_settings.emplace(settings_t::WINDOW_SIZE, MAX_WINDOW_SIZE);
	// Если максимальный размер блока заголовоков не установлен
	if(this->_settings.count(settings_t::HEADER_TABLE_SIZE) == 0)
		// Выполняем установку максимального размера блока заголовоков
		this->_settings.emplace(settings_t::HEADER_TABLE_SIZE, HEADER_TABLE_SIZE);
	// Если флаг разрешения принимать push-уведомления не установлено
	if(this->_settings.count(settings_t::ENABLE_PUSH) == 0)
		// Выполняем установку флага отключения принёма push-уведомлений
		this->_settings.emplace(settings_t::ENABLE_PUSH, 0);
}
/**
 * Web2 Конструктор
 * @param fmk объект фреймворка
 * @param log объект для работы с логами
 */
awh::server::Web2::Web2(const fmk_t * fmk, const log_t * log) noexcept : web_t(fmk, log) {
	// Выполняем установку список настроек протокола HTTP/2
	this->settings();
}
/**
 * Web2 Конструктор
 * @param core объект сетевого ядра
 * @param fmk  объект фреймворка
 * @param log  объект для работы с логами
 */
awh::server::Web2::Web2(const server::core_t * core, const fmk_t * fmk, const log_t * log) noexcept : web_t(core, fmk, log) {
	// Выполняем установку список настроек протокола HTTP/2
	this->settings();
}
