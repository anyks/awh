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
 * statusEvents Метод обратного вызова при активации ядра сервера
 * @param status флаг запуска/остановки
 */
void awh::server::Web2::statusEvents(const awh::core_t::status_t status) noexcept {
	// Если система была остановлена
	if(status == awh::core_t::status_t::STOP){
		// Если список сессий не пустой
		if(!this->_sessions.empty())
			// Выполняем очистку списка активных сессий
			this->_sessions.clear();
	}
	// Выполняем переадресацию выполняемого события в родительский модуль
	web_t::statusEvents(status);
}
/**
 * connectEvents Метод обратного вызова при подключении к серверу
 * @param bid идентификатор брокера
 * @param sid идентификатор схемы сети
 */
void awh::server::Web2::connectEvents(const uint64_t bid, const uint16_t sid) noexcept {
	// Если флаг инициализации сессии HTTP/2 не активирован
	if(this->_sessions.find(bid) == this->_sessions.end()){
		// Если список параметров настроек не пустой и протокол HTTP/2 поддерживается сервером
		if(!this->_settings.empty() && (this->_core->proto(bid) == engine_t::proto_t::HTTP2)){
			/**
			 * Я не знаю что за хуйня, но каким-то образом изначально эта проверка не работает и приходится проверять второй раз
			 */
			if(this->_core->proto(bid) != engine_t::proto_t::HTTP2)
				// Выходим из функции
				return;
			// Создаём локальный контейнер функций обратного вызова
			fn_t callbacks(this->_log);
			// Выполняем установку функции обратного вызова начала открытии потока
			callbacks.set <int32_t (const int32_t)> ("begin", std::bind(&web2_t::beginSignal, this, _1, bid));
			// Выполняем установку функции обратного вызова при отправки сообщения на сервер
			callbacks.set <void (const uint8_t *, const size_t)> ("send", std::bind(&web2_t::sendSignal, this, bid, _1, _2));
			// Выполняем установку функции обратного вызова при закрытии потока
			callbacks.set <int32_t (const int32_t, const http2_t::error_t)> ("close", std::bind(&web2_t::closedSignal, this, _1, bid, _2));
			// Выполняем установку функции обратного вызова при получении чанка с сервера
			callbacks.set <int32_t (const int32_t, const uint8_t *, const size_t)> ("chunk", std::bind(&web2_t::chunkSignal, this, _1, bid, _2, _3));
			// Выполняем установку функции обратного вызова при получении данных заголовка
			callbacks.set <int32_t (const int32_t, const string &, const string &)> ("header", std::bind(&web2_t::headerSignal, this, _1, bid, _2, _3));
			// Выполняем установку функции обратного вызова получения фрейма
			callbacks.set <int32_t (const int32_t, const http2_t::direct_t, const http2_t::frame_t, const std::set <http2_t::flag_t> &)> ("frame", std::bind(&web2_t::frameSignal, this, _1, bid, _2, _3, _4));
			// Если функция обратного вызова на на вывод ошибок установлена
			if(this->_callbacks.is("error"))
				// Устанавливаем функцию обработки вызова на событие получения ошибок
				callbacks.set <void (const log_t::flag_t, const http::error_t, const string &)> ("error", std::bind(this->_callbacks.get <void (const uint64_t, const log_t::flag_t, const http::error_t, const string &)> ("error"), bid, _1, _2, _3));
			// Выполняем создание нового объекта сессии HTTP/2
			auto ret = this->_sessions.emplace(bid, std::unique_ptr <http2_t> (new http2_t(this->_fmk, this->_log)));
			// Выполняем установку функции обратного вызова
			ret.first->second->callbacks(callbacks);
			// Если инициализация модуля NgHttp2 не выполнена
			if(!ret.first->second->init(http2_t::mode_t::SERVER, this->_settings))
				// Выполняем удаление созданного ранее объекта
				this->_sessions.erase(ret.first);
			// Если список разрешённых источников установлен
			if(!this->_origins.empty())
				// Устанавливаем список разрешённых источников
				ret.first->second->origin(this->_origins);
			// Если список альтернативных сервисов установлен
			if(!this->_altsvc.empty())
				// Устанавливаем список альтернативных сервисов
				ret.first->second->altsvc(this->_altsvc);
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
		const_cast <server::core_t *> (this->_core)->send(reinterpret_cast <const char *> (buffer), size, bid);
}
/**
 * close Метод выполнения закрытия подключения
 * @param bid идентификатор брокера
 */
void awh::server::Web2::close(const uint64_t bid) noexcept {
	// Если флаг инициализации сессии HTTP/2 установлен и подключение выполнено
	if((this->_core != nullptr) && this->_core->working()){
		// Выполняем поиск брокера в списке активных сессий
		auto i = this->_sessions.find(bid);
		// Если активная сессия найдена
		if(i != this->_sessions.end())
			// Выполняем установку функции обратного вызова триггера, для закрытия соединения после завершения всех процессов
			i->second->callback <void (void)> (1, std::bind(static_cast <void (server::core_t::*)(const uint64_t)> (&server::core_t::close), const_cast <server::core_t *> (this->_core), bid));
		// Завершаем работу
		else const_cast <server::core_t *> (this->_core)->close(bid);
	}
}
/**
 * ping Метод выполнения пинга клиента
 * @param bid идентификатор брокера
 * @return    результат работы пинга
 */
bool awh::server::Web2::ping(const uint64_t bid) noexcept {
	// Выполняем поиск брокера в списке активных сессий
	auto i = this->_sessions.find(bid);
	// Если активная сессия найдена
	if(i != this->_sessions.end())
		// Выполняем пинг удалённого сервера
		return i->second->ping();
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
		auto i = this->_sessions.find(bid);
		// Если активная сессия найдена
		if((result = (i != this->_sessions.end()))){
			// Выполняем закрытие полное закрытие подключения клиента
			if(!(result = i->second->shutdown())){
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
 * @param sid   идентификатор потока
 * @param bid   идентификатор брокера
 * @param error код отправляемой ошибки
 * @return      результат отправки сообщения
 */
bool awh::server::Web2::reject(const int32_t sid, const uint64_t bid, const http2_t::error_t error) noexcept {
	// Результат работы функции
	bool result = false;
	// Если флаг инициализации сессии HTTP/2 установлен и подключение выполнено
	if((this->_core != nullptr) && this->_core->working()){
		// Выполняем поиск брокера в списке активных сессий
		auto i = this->_sessions.find(bid);
		// Если активная сессия найдена
		if((result = (i != this->_sessions.end()))){
			// Выполняем закрытие подключения клиента
			if(!(result = i->second->reject(sid, error))){
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
		auto i = this->_sessions.find(bid);
		// Если активная сессия найдена
		if((result = (i != this->_sessions.end()))){
			// Выполняем отправку требования клиенту выполнить отключение от сервера
			if(!(result = i->second->goaway(last, error, buffer, size))){
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
 * @param sid     идентификатор потока
 * @param bid     идентификатор брокера
 * @param headers заголовки отправляемые
 * @return        результат отправки данных указанному клиенту
 */
bool awh::server::Web2::send(const int32_t sid, const uint64_t bid, const vector <std::pair <string, string>> & headers) noexcept {
	// Результат работы функции
	bool result = false;
	// Если флаг инициализации сессии HTTP/2 установлен и подключение выполнено
	if((this->_core != nullptr) && this->_core->working() && !headers.empty()){
		// Выполняем поиск брокера в списке активных сессий
		auto i = this->_sessions.find(bid);
		// Если активная сессия найдена
		if((result = (i != this->_sessions.end()))){
			// Выполняем отправку трейлеров
			if(!(result = i->second->sendTrailers(sid, headers))){
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
 * @param sid    идентификатор потока
 * @param bid    идентификатор брокера
 * @param buffer буфер бинарных данных передаваемых
 * @param size   размер сообщения в байтах
 * @param flag   флаг передаваемого потока по сети
 * @return       результат отправки данных указанному клиенту
 */
bool awh::server::Web2::send(const int32_t sid, const uint64_t bid, const char * buffer, const size_t size, const http2_t::flag_t flag) noexcept {
	// Результат работы функции
	bool result = false;
	// Если флаг инициализации сессии HTTP/2 установлен и подключение выполнено
	if((this->_core != nullptr) && this->_core->working() && (buffer != nullptr) && (size > 0)){
		// Выполняем поиск брокера в списке активных сессий
		auto i = this->_sessions.find(bid);
		// Если активная сессия найдена
		if((result = (i != this->_sessions.end()))){
			// Выполняем отправку тела ответа
			if(!(result = i->second->sendData(sid, reinterpret_cast <const uint8_t *> (buffer), size, flag))){
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
 * @param sid     идентификатор потока
 * @param bid     идентификатор брокера
 * @param headers заголовки отправляемые
 * @param flag    флаг передаваемого потока по сети
 * @return        флаг последнего сообщения после которого поток закрывается
 */
int32_t awh::server::Web2::send(const int32_t sid, const uint64_t bid, const vector <std::pair <string, string>> & headers, const http2_t::flag_t flag) noexcept {
	// Результат работы функции
	int32_t result = -1;
	// Если флаг инициализации сессии HTTP/2 установлен и подключение выполнено
	if((this->_core != nullptr) && this->_core->working() && !headers.empty()){
		// Выполняем поиск брокера в списке активных сессий
		auto i = this->_sessions.find(bid);
		// Если активная сессия найдена
		if(i != this->_sessions.end()){
			// Если ответ не получилось отправить
			if((result = i->second->sendHeaders(sid, headers, flag)) < 0){
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
 * @param sid     идентификатор потока
 * @param bid     идентификатор брокера
 * @param headers заголовки отправляемые
 * @param flag    флаг передаваемого потока по сети
 * @return        флаг последнего сообщения после которого поток закрывается
 */
int32_t awh::server::Web2::push(const int32_t sid, const uint64_t bid, const vector <std::pair <string, string>> & headers, const http2_t::flag_t flag) noexcept {
	// Результат работы функции
	int32_t result = -1;
	// Если флаг инициализации сессии HTTP/2 установлен и подключение выполнено
	if((this->_core != nullptr) && this->_core->working() && !headers.empty()){
		// Выполняем поиск брокера в списке активных сессий
		auto i = this->_sessions.find(bid);
		// Если активная сессия найдена
		if(i != this->_sessions.end()){
			// Если ответ не получилось отправить push-уведомление
			if((result = i->second->sendPush(sid, headers, flag)) < 0)
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
 * setAltSvc Метод установки списка альтернативных сервисов
 * @param origins список альтернативных сервисов
 */
void awh::server::Web2::setAltSvc(const std::unordered_multimap <string, string> & origins) noexcept {
	// Выполняем установку списка альтернативных сервисов
	this->_altsvc = origins;
}
/**
 * settings Модуль установки настроек протокола HTTP/2
 * @param settings список настроек протокола HTTP/2
 */
void awh::server::Web2::settings(const std::map <http2_t::settings_t, uint32_t> & settings) noexcept {
	// Если список настроек протокола HTTP/2 передан
	if(!settings.empty())
		// Выполняем установку списка настроек
		this->_settings = settings;
	// Если максимальное количество потоков не установлено
	if(this->_settings.find(http2_t::settings_t::STREAMS) == this->_settings.end())
		// Выполняем установку максимального количества потоков
		this->_settings.emplace(http2_t::settings_t::STREAMS, http2_t::CONCURRENT_STREAMS);
	// Если максимальный размер фрейма не установлен
	if(this->_settings.find(http2_t::settings_t::FRAME_SIZE) == this->_settings.end())
		// Выполняем установку максимального размера фрейма
		this->_settings.emplace(http2_t::settings_t::FRAME_SIZE, http2_t::MAX_FRAME_SIZE_MIN);
	// Если максимальный размер фрейма установлен
	else {
		// Выполняем извлечение максимального размера фрейма
		auto i = this->_settings.find(http2_t::settings_t::FRAME_SIZE);
		// Если максимальный размер фрейма больше самого максимального значения
		if(i->second > http2_t::MAX_FRAME_SIZE_MAX)
			// Выполняем корректировку максимального размера фрейма
			i->second = http2_t::MAX_FRAME_SIZE_MAX;
		// Если максимальный размер фрейма меньше самого минимального значения
		else if(i->second < http2_t::MAX_FRAME_SIZE_MIN)
			// Выполняем корректировку максимального размера фрейма
			i->second = http2_t::MAX_FRAME_SIZE_MIN;
	}
	// Если максимальный размер окна фрейма не установлен
	if(this->_settings.find(http2_t::settings_t::WINDOW_SIZE) == this->_settings.end())
		// Выполняем установку максимального размера окна фрейма
		this->_settings.emplace(http2_t::settings_t::WINDOW_SIZE, http2_t::MAX_WINDOW_SIZE);
	// Если максимальный размер буфера полезной нагрузки не установлен
	if(this->_settings.find(http2_t::settings_t::PAYLOAD_SIZE) == this->_settings.end())
		// Выполняем установку максимального размера буфера полезной нагрузки
		this->_settings.emplace(http2_t::settings_t::PAYLOAD_SIZE, http2_t::MAX_PAYLOAD_SIZE);
	// Если максимальный размер блока заголовоков не установлен
	if(this->_settings.find(http2_t::settings_t::HEADER_TABLE_SIZE) == this->_settings.end())
		// Выполняем установку максимального размера блока заголовоков
		this->_settings.emplace(http2_t::settings_t::HEADER_TABLE_SIZE, http2_t::HEADER_TABLE_SIZE);
	// Если флаг разрешения принимать push-уведомления не установлено
	if(this->_settings.find(http2_t::settings_t::ENABLE_PUSH) == this->_settings.end())
		// Выполняем установку флага отключения принёма push-уведомлений
		this->_settings.emplace(http2_t::settings_t::ENABLE_PUSH, 0);
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
