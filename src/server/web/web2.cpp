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
 * sendSignal Метод обратного вызова при отправки данных HTTP/2
 * @param aid    идентификатор адъютанта
 * @param buffer буфер бинарных данных
 * @param size   размер буфера данных для отправки
 */
void awh::server::Web2::sendSignal(const uint64_t aid, const uint8_t * buffer, const size_t size) noexcept {
	// Выполняем отправку заголовков запроса клиенту
	const_cast <server::core_t *> (this->_core)->write((const char *) buffer, size, aid);
}
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
	}
	// Выполняем переадресацию выполняемого события в родительский модуль
	web_t::eventsCallback(status, core);
}
/**
 * connectCallback Метод обратного вызова при подключении к серверу
 * @param aid  идентификатор адъютанта
 * @param sid  идентификатор схемы сети
 * @param core объект сетевого ядра
 */
void awh::server::Web2::connectCallback(const uint64_t aid, const uint16_t sid, awh::core_t * core) noexcept {
	// Если флаг инициализации сессии HTTP2 не активирован, но протокол HTTP/2 поддерживается сервером
	if((this->_sessions.count(aid) == 0) && (dynamic_cast <server::core_t *> (core)->proto(aid) == engine_t::proto_t::HTTP2)){
		// Если список параметров настроек не пустой
		if(!this->_settings.empty()){
			// Создаём параметры сессии подключения с HTTP/2 сервером
			vector <nghttp2_settings_entry> iv;
			// Выполняем переход по всему списку настроек
			for(auto & setting : this->_settings){
				// Определяем тип настройки
				switch(static_cast <uint8_t> (setting.first)){
					// Если мы получили разрешение присылать пуш-уведомления
					case static_cast <uint8_t> (settings_t::ENABLE_PUSH):
						// Устанавливаем разрешение присылать пуш-уведомления
						iv.push_back({NGHTTP2_SETTINGS_ENABLE_PUSH, setting.second});
					break;
					// Если мы получили максимальный размер фрейма
					case static_cast <uint8_t> (settings_t::FRAME_SIZE):
						// Устанавливаем максимальный размер фрейма
						iv.push_back({NGHTTP2_SETTINGS_MAX_FRAME_SIZE, setting.second});
					break;
					// Если мы получили максимальный размер таблицы заголовков
					case static_cast <uint8_t> (settings_t::HEADER_TABLE_SIZE):
						// Устанавливаем максимальный размер таблицы заголовков
						iv.push_back({NGHTTP2_SETTINGS_HEADER_TABLE_SIZE, setting.second});
					break;
					// Если мы получили максимальный размер окна полезной нагрузки
					case static_cast <uint8_t> (settings_t::WINDOW_SIZE):
						// Устанавливаем максимальный размер окна полезной нагрузки
						iv.push_back({NGHTTP2_SETTINGS_INITIAL_WINDOW_SIZE, setting.second});
					break;
					// Если мы получили максимальное количество потоков
					case static_cast <uint8_t> (settings_t::STREAMS):
						// Устанавливаем максимальное количество потоков
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
			auto ret = this->_sessions.emplace(aid, unique_ptr <nghttp2_t> (new nghttp2_t(this->_fmk, this->_log)));
			// Выполняем установку функции обратного вызова начала открытии потока
			ret.first->second->on((function <int (const int32_t)>) std::bind(&web2_t::beginSignal, this, _1, aid));
			// Выполняем установку функции обратного вызова при отправки сообщения клиенту
			ret.first->second->on((function <void (const uint8_t *, const size_t)>) std::bind(&web2_t::sendSignal, this, aid, _1, _2));
			// Выполняем установку функции обратного вызова при закрытии потока
			ret.first->second->on((function <int (const int32_t, const uint32_t)>) std::bind(&web2_t::closedSignal, this, _1, aid, _2));
			// Выполняем установку функции обратного вызова при получении чанка с сервера
			ret.first->second->on((function <int (const int32_t, const uint8_t *, const size_t)>) std::bind(&web2_t::chunkSignal, this, _1, aid, _2, _3));
			// Выполняем установку функции обратного вызова при получении данных заголовка
			ret.first->second->on((function <int (const int32_t, const string &, const string &)>) std::bind(&web2_t::headerSignal, this, _1, aid, _2, _3));
			// Выполняем установку функции обратного вызова получения фрейма HTTP/2
			ret.first->second->on((function <int (const int32_t, const nghttp2_t::direct_t direct, const uint8_t, const uint8_t)>) std::bind(&web2_t::frameSignal, this, _1, aid, _2, _3, _4));
			// Если функция обратного вызова на на вывод ошибок установлена
			if(this->_callback.is("error"))
				// Выполняем установку функции обратного вызова на событие получения ошибки
				ret.first->second->on(std::bind(this->_callback.get <void (const uint64_t, const log_t::flag_t, const http::error_t, const string &)> ("error"), aid, _1, _2, _3));
			// Если инициализация модуля NgHttp2 не выполнена
			if(!ret.first->second->init(nghttp2_t::mode_t::SERVER, std::move(iv)))
				// Выполняем удаление созданного ранее объекта
				this->_sessions.erase(ret.first);
			// Если инициализация модуля NgHttp2 прошла успешно и список ресурсов с которым должен работать сервер установлен
			else if(!this->_origins.empty())
				// Выполняем установку список ресурсов с которым должен работать сервер
				this->sendOrigin(aid, this->_origins);
		}
	}
}
/**
 * persistCallback Функция персистентного вызова
 * @param aid  идентификатор адъютанта
 * @param sid  идентификатор схемы сети
 * @param core объект сетевого ядра
 */
void awh::server::Web2::persistCallback(const uint64_t aid, const uint16_t sid, awh::core_t * core) noexcept {
	// Если данные существуют
	if((aid > 0) && (sid > 0) && (core != nullptr)){
		// Если переключение протокола на HTTP/2 выполнено и пинг не прошёл
		if((this->_sessions.count(aid) > 0) && !this->ping(aid))
			// Завершаем работу
			dynamic_cast <server::core_t *> (core)->close(aid);
	}
}
/**
 * ping Метод выполнения пинга клиента
 * @param aid идентификатор адъютанта
 * @return    результат работы пинга
 */
bool awh::server::Web2::ping(const uint64_t aid) noexcept {
	// Выполняем поиск адъютанта в списке активных сессий
	auto it = this->_sessions.find(aid);
	// Если активная сессия найдена
	if(it != this->_sessions.end()){
		// Если сессия HTTP/2 инициализированна
		if(it->second->session != nullptr){
			// Результат выполнения поерации
			int rv = -1;
			// Выполняем пинг удалённого сервера
			if((rv = nghttp2_submit_ping(it->second->session, 0, nullptr)) != 0){
				// Выводим сообщение об полученной ошибке
				this->_log->print("%s", log_t::flag_t::CRITICAL, nghttp2_strerror(rv));
				// Если функция обратного вызова на на вывод ошибок установлена
				if(this->_callback.is("error"))
					// Выводим функцию обратного вызова
					this->_callback.call <const uint64_t, const log_t::flag_t, const http::error_t, const string &> ("error", aid, log_t::flag_t::CRITICAL, http::error_t::HTTP2_PING, nghttp2_strerror(rv));
				// Выходим из функции
				return false;
			}
			// Если сессия HTTP/2 инициализированна
			if(it->second->session != nullptr){
				// Фиксируем отправленный результат
				if((rv = nghttp2_session_send(it->second->session)) != 0){
					// Выводим сообщение об полученной ошибке
					this->_log->print("%s", log_t::flag_t::CRITICAL, nghttp2_strerror(rv));
					// Если функция обратного вызова на на вывод ошибок установлена
					if(this->_callback.is("error"))
						// Выводим функцию обратного вызова
						this->_callback.call <const uint64_t, const log_t::flag_t, const http::error_t, const string &> ("error", aid, log_t::flag_t::CRITICAL, http::error_t::HTTP2_SEND, nghttp2_strerror(rv));
					// Выходим из функции
					return false;
				}
			}
		}
		// Выводим результат
		return true;
	}
	// Выводим результат
	return false;
}
/**
 * send Метод отправки сообщения клиенту
 * @param id     идентификатор потока HTTP/2
 * @param aid    идентификатор адъютанта
 * @param buffer буфер бинарных данных передаваемых на сервер
 * @param size   размер сообщения в байтах
 * @param end    флаг последнего сообщения после которого поток закрывается
 */
void awh::server::Web2::send(const int32_t id, const uint64_t aid, const char * buffer, const size_t size, const bool end) noexcept {
	// Если флаг инициализации сессии HTTP2 установлен и подключение выполнено
	if(this->_core->working() && (buffer != nullptr) && (size > 0)){
		// Выполняем поиск адъютанта в списке активных сессий
		auto it = this->_sessions.find(aid);
		// Если активная сессия найдена
		if(it != this->_sessions.end()){
			// Список файловых дескрипторов
			int fds[2];
			/**
			 * Методы только для OS Windows
			 */
			#if defined(_WIN32) || defined(_WIN64)
				// Выполняем инициализацию файловых дескрипторов для обмена сообщениями
				const int rv = _pipe(fds, 4096, O_BINARY);
			/**
			 * Для всех остальных операционных систем
			 */
			#else
				// Выполняем инициализацию файловых дескрипторов для обмена сообщениями
				const int rv = ::pipe(fds);
			#endif
			// Выполняем подписку на основной канал передачи данных
			if(rv != 0){
				// Выводим в лог сообщение
				this->_log->print("%s", log_t::flag_t::CRITICAL, strerror(errno));
				// Если функция обратного вызова на на вывод ошибок установлена
				if(this->_callback.is("error"))
					// Выводим функцию обратного вызова
					this->_callback.call <const uint64_t, const log_t::flag_t, const http::error_t, const string &> ("error", aid, log_t::flag_t::CRITICAL, http::error_t::HTTP2_PIPE_INIT, strerror(errno));
				// Выполняем закрытие подключения
				const_cast <server::core_t *> (this->_core)->close(aid);
				// Выходим из функции
				return;
			}
			/**
			 * Методы только для OS Windows
			 */
			#if defined(_WIN32) || defined(_WIN64)
				// Если данные небыли записаны в сокет
				if(static_cast <int> (_write(fds[1], buffer, size)) != static_cast <int> (size)){
					// Выполняем закрытие сокета для чтения
					_close(fds[0]);
					// Выполняем закрытие сокета для записи
					_close(fds[1]);
					// Выводим в лог сообщение
					this->_log->print("%s", log_t::flag_t::CRITICAL, strerror(errno));
					// Если функция обратного вызова на на вывод ошибок установлена
					if(this->_callback.is("error"))
						// Выводим функцию обратного вызова
						this->_callback.call <const uint64_t, const log_t::flag_t, const http::error_t, const string &> ("error", aid, log_t::flag_t::CRITICAL, http::error_t::HTTP2_PIPE_WRITE, strerror(errno));
					// Выполняем закрытие подключения
					const_cast <server::core_t *> (this->_core)->close(aid);
					// Выходим из функции
					return;
				}
			/**
			 * Для всех остальных операционных систем
			 */
			#else
				// Если данные небыли записаны в сокет
				if(static_cast <int> (::write(fds[1], buffer, size)) != static_cast <int> (size)){
					// Выполняем закрытие сокета для чтения
					::close(fds[0]);
					// Выполняем закрытие сокета для записи
					::close(fds[1]);
					// Выводим в лог сообщение
					this->_log->print("%s", log_t::flag_t::CRITICAL, strerror(errno));
					// Если функция обратного вызова на на вывод ошибок установлена
					if(this->_callback.is("error"))
						// Выводим функцию обратного вызова
						this->_callback.call <const uint64_t, const log_t::flag_t, const http::error_t, const string &> ("error", aid, log_t::flag_t::CRITICAL, http::error_t::HTTP2_PIPE_WRITE, strerror(errno));
					// Выполняем закрытие подключения
					const_cast <server::core_t *> (this->_core)->close(aid);
					// Выходим из функции
					return;
				}
			#endif
			/**
			 * Методы только для OS Windows
			 */
			#if defined(_WIN32) || defined(_WIN64)
				// Выполняем закрытие подключения
				_close(fds[1]);
			/**
			 * Для всех остальных операционных систем
			 */
			#else
				// Выполняем закрытие подключения
				::close(fds[1]);
			#endif
			// Создаём объект передачи данных тела полезной нагрузки
			nghttp2_data_provider data;
			// Зануляем передаваемый контекст
			data.source.ptr = nullptr;
			// Устанавливаем файловый дескриптор
			data.source.fd = fds[0];
			// Устанавливаем функцию обратного вызова
			data.read_callback = &nghttp2_t::read;
			// Если сессия HTTP/2 инициализированна
			if(it->second->session != nullptr){
				// Результат фиксации сессии
				int rv = -1;
				// Выполняем формирование данных фрейма для отправки
				if((rv = nghttp2_submit_data(it->second->session, (end ? NGHTTP2_FLAG_END_STREAM : NGHTTP2_FLAG_NONE), id, &data)) != 0){
					// Выводим сообщение об полученной ошибке
					this->_log->print("%s", log_t::flag_t::CRITICAL, nghttp2_strerror(rv));
					// Если функция обратного вызова на на вывод ошибок установлена
					if(this->_callback.is("error"))
						// Выводим функцию обратного вызова
						this->_callback.call <const uint64_t, const log_t::flag_t, const http::error_t, const string &> ("error", aid, log_t::flag_t::CRITICAL, http::error_t::HTTP2_SUBMIT, nghttp2_strerror(rv));
					// Выходим из функции
					return;
				}
				// Если сессия HTTP/2 инициализированна
				if(it->second->session != nullptr){
					// Фиксируем отправленный результат
					if((rv = nghttp2_session_send(it->second->session)) != 0){
						// Выводим сообщение об полученной ошибке
						this->_log->print("%s", log_t::flag_t::CRITICAL, nghttp2_strerror(rv));
						// Если функция обратного вызова на на вывод ошибок установлена
						if(this->_callback.is("error"))
							// Выводим функцию обратного вызова
							this->_callback.call <const uint64_t, const log_t::flag_t, const http::error_t, const string &> ("error", aid, log_t::flag_t::CRITICAL, http::error_t::HTTP2_SEND, nghttp2_strerror(rv));
						// Выходим из функции
						return;
					}
				}
			}
		}
	}
}
/**
 * send Метод отправки заголовков на сервер
 * @param id      идентификатор потока HTTP/2
 * @param aid     идентификатор адъютанта
 * @param headers заголовки отправляемые на сервер
 * @param end     размер сообщения в байтах
 * @return        флаг последнего сообщения после которого поток закрывается
 */
int32_t awh::server::Web2::send(const int32_t id, const uint64_t aid, const vector <pair <string, string>> & headers, const bool end) noexcept {
	// Результат работы функции
	int32_t result = -1;
	// Если флаг инициализации сессии HTTP2 установлен и подключение выполнено
	if(this->_core->working() && !headers.empty()){
		// Выполняем поиск адъютанта в списке активных сессий
		auto it = this->_sessions.find(aid);
		// Если активная сессия найдена
		if(it != this->_sessions.end()){
			// Список заголовков для запроса
			vector <nghttp2_nv> nva;
			// Выполняем перебор всех заголовков HTTP/2 запроса
			for(auto & header : headers){
				// Выполняем добавление метода запроса
				nva.push_back({
					(uint8_t *) header.first.c_str(),
					(uint8_t *) header.second.c_str(),
					header.first.size(),
					header.second.size(),
					NGHTTP2_NV_FLAG_NONE
				});
			}
			// Если сессия HTTP/2 инициализированна
			if(it->second->session != nullptr){
				// Выполняем запрос на удалённый сервер			
				result = nghttp2_submit_headers(it->second->session, (end ? NGHTTP2_FLAG_END_STREAM : NGHTTP2_FLAG_NONE), id, nullptr, nva.data(), nva.size(), nullptr);
				// Если запрос не получилось отправить
				if(result < 0){
					// Выводим в лог сообщение
					this->_log->print("%s", log_t::flag_t::CRITICAL, nghttp2_strerror(result));
					// Если функция обратного вызова на на вывод ошибок установлена
					if(this->_callback.is("error"))
						// Выводим функцию обратного вызова
						this->_callback.call <const uint64_t, const log_t::flag_t, const http::error_t, const string &> ("error", aid, log_t::flag_t::CRITICAL, http::error_t::HTTP2_SUBMIT, nghttp2_strerror(result));
					// Выполняем закрытие подключения
					const_cast <server::core_t *> (this->_core)->close(aid);
					// Выходим из функции
					return result;
				}
				// Если сессия HTTP/2 инициализированна
				if(it->second->session != nullptr){
					// Результат фиксации сессии
					int rv = -1;
					// Фиксируем отправленный результат
					if((rv = nghttp2_session_send(it->second->session)) != 0){
						// Выводим сообщение об полученной ошибке
						this->_log->print("%s", log_t::flag_t::CRITICAL, nghttp2_strerror(rv));
						// Если функция обратного вызова на на вывод ошибок установлена
						if(this->_callback.is("error"))
							// Выводим функцию обратного вызова
							this->_callback.call <const uint64_t, const log_t::flag_t, const http::error_t, const string &> ("error", aid, log_t::flag_t::CRITICAL, http::error_t::HTTP2_SEND, nghttp2_strerror(rv));
						// Выполняем закрытие подключения
						const_cast <server::core_t *> (this->_core)->close(aid);
						// Выходим из функции
						return result;
					}
				}
			}
		}
	}
	// Выводим результат
	return result;
}
/**
 * setOrigin Метод установки списка разрешенных источников
 * @param origins список разрешённых источников
 */
void awh::server::Web2::setOrigin(const vector <string> & origins) noexcept {
	// Выполняем установку списка источников
	this->_origins = origins;
}
/**
 * sendOrigin Метод отправки списка разрешенных источников
 * @param aid     идентификатор адъютанта
 * @param origins список разрешённых источников
 */
void awh::server::Web2::sendOrigin(const uint64_t aid, const vector <string> & origins) noexcept {
	// Выполняем поиск адъютанта в списке активных сессий
	auto it = this->_sessions.find(aid);
	// Если активная сессия найдена
	if(it != this->_sessions.end()){
		// Список источников для установки на клиенте
		vector <nghttp2_origin_entry> ov;
		// Если список источников передан
		if(!origins.empty()){
			// Выполняем перебор списка источников
			for(auto & origin : origins)
				// Выполняем добавление источника в списку
				ov.push_back({(uint8_t *) origin.c_str(), origin.size()});
		}
		// Если сессия HTTP/2 инициализированна
		if(it->second->session != nullptr){
			// Результат выполнения поерации
			int rv = -1;
			// Выполняем установку фрейма полученных источников
			if((rv = nghttp2_submit_origin(it->second->session, NGHTTP2_FLAG_NONE, (!ov.empty() ? ov.data() : nullptr), ov.size())) != 0){
				// Выводим сообщение об полученной ошибке
				this->_log->print("%s", log_t::flag_t::CRITICAL, nghttp2_strerror(rv));
				// Выходим из функции
				return;
			}
			// Если сессия HTTP/2 инициализированна
			if(it->second->session != nullptr){
				// Фиксируем отправленный результат
				if((rv = nghttp2_session_send(it->second->session)) != 0){
					// Выводим сообщение об полученной ошибке
					this->_log->print("%s", log_t::flag_t::CRITICAL, nghttp2_strerror(rv));
					// Выходим из функции
					return;
				}
			}
		}
	}
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
	// Если флаг разрешения принимать пуш-уведомления не установлено
	if(this->_settings.count(settings_t::ENABLE_PUSH) == 0)
		// Выполняем установку флага отключения принёма пуш-уведомлений
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
