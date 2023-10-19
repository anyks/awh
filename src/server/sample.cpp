/**
 * @file: sample.cpp
 * @date: 2023-10-18
 * @license: GPL-3.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @copyright: Copyright © 2023
 */

// Подключаем заголовочный файл
#include <server/sample.hpp>

/**
 * openCallback Метод обратного вызова при запуске работы
 * @param sid  идентификатор схемы сети
 * @param core объект сетевого ядра
 */
void awh::server::Sample::openCallback(const uint16_t sid, awh::core_t * core) noexcept {
	// Если данные существуют
	if((sid > 0) && (core != nullptr)){
		// Устанавливаем хост сервера
		dynamic_cast <server::core_t *> (core)->init(sid, this->_port, this->_host);
		// Выполняем запуск сервера
		dynamic_cast <server::core_t *> (core)->run(sid);
	}
}
/**
 * eventsCallback Метод обратного вызова при активации ядра сервера
 * @param status флаг запуска/остановки
 * @param core   объект сетевого ядра
 */
void awh::server::Sample::eventsCallback(const awh::core_t::status_t status, awh::core_t * core) noexcept {
	// Если данные существуют
	if(core != nullptr){
		// Определяем статус активности сетевого ядра
		switch(static_cast <uint8_t> (status)){
			// Если система запущена
			case static_cast <uint8_t> (awh::core_t::status_t::START): {
				// Выполняем биндинг ядра локального таймера
				core->bind(&this->_timer);
				// Устанавливаем интервал времени на удаление отключившихся клиентов раз в 5 секунд
				this->_timer.setInterval(5000, std::bind(&sample_t::erase, this, _1, _2));
				// Устанавливаем интервал времени на выполнения пинга удалённого сервера
				this->_timer.setInterval(PING_INTERVAL, std::bind(&sample_t::pinging, this, _1, _2));
			} break;
			// Если система остановлена
			case static_cast <uint8_t> (awh::core_t::status_t::STOP): {
				// Останавливаем все установленные таймеры
				this->_timer.clearTimers();
				// Выполняем анбиндинг ядра локального таймера
				core->unbind(&this->_timer);
			} break;
		}
		// Если функция получения событий запуска и остановки сетевого ядра установлена
		if(this->_callback.is("events"))
			// Выводим функцию обратного вызова
			this->_callback.call <const awh::core_t::status_t, awh::core_t *> ("events", status, core);
	}
}
/**
 * connectCallback Метод обратного вызова при подключении к серверу
 * @param bid  идентификатор брокера
 * @param sid  идентификатор схемы сети
 * @param core объект сетевого ядра
 */
void awh::server::Sample::connectCallback(const uint64_t bid, const uint16_t sid, awh::core_t * core) noexcept {
	// Если данные переданы верные
	if((bid > 0) && (sid > 0) && (core != nullptr)){
		// Создаём брокера
		this->_scheme.set(bid);
		// Если функция обратного вызова при подключении/отключении установлена
		if(this->_callback.is("active"))
			// Выводим функцию обратного вызова
			this->_callback.call <const uint64_t, const mode_t> ("active", bid, mode_t::CONNECT);
	}
}
/**
 * disconnectCallback Метод обратного вызова при отключении от сервера
 * @param bid  идентификатор брокера
 * @param sid  идентификатор схемы сети
 * @param core объект сетевого ядра
 */
void awh::server::Sample::disconnectCallback(const uint64_t bid, const uint16_t sid, awh::core_t * core) noexcept {
	// Если данные переданы верные
	if((bid > 0) && (sid > 0) && (core != nullptr)){
		// Добавляем в очередь список отключившихся клиентов
		this->_disconnected.emplace(bid, this->_fmk->timestamp(fmk_t::stamp_t::MILLISECONDS));
		// Если функция обратного вызова при подключении/отключении установлена
		if(this->_callback.is("active"))
			// Выводим функцию обратного вызова
			this->_callback.call <const uint64_t, const mode_t> ("active", bid, mode_t::DISCONNECT);
	}
}
/**
 * readCallback Метод обратного вызова при чтении сообщения с брокера
 * @param buffer бинарный буфер содержащий сообщение
 * @param size   размер бинарного буфера содержащего сообщение
 * @param bid    идентификатор брокера
 * @param sid    идентификатор схемы сети
 * @param core   объект сетевого ядра
 */
void awh::server::Sample::readCallback(const char * buffer, const size_t size, const uint64_t bid, const uint16_t sid, awh::core_t * core) noexcept {
	// Если данные существуют
	if((buffer != nullptr) && (size > 0) && (bid > 0) && (sid > 0)){
		// Получаем параметры активного клиента
		sample_scheme_t::options_t * options = const_cast <sample_scheme_t::options_t *> (this->_scheme.get(bid));
		// Если параметры активного клиента получены
		if(options != nullptr){
			// Выполняем сброс закрытия подключения
			options->close = false;
			// Если разрешено получение данных
			if(options->allow.receive){
				// Если функция обратного вызова при получении входящих сообщений установлена
				if(this->_callback.is("message"))
					// Выводим данные полученного сообщения
					this->_callback.call <const uint64_t, const vector <char> &> ("message", bid, vector <char> (buffer, buffer + size));
			}
		}
	}
}
/**
 * writeCallback Метод обратного вызова при записи сообщение брокеру
 * @param buffer бинарный буфер содержащий сообщение
 * @param size   размер записанных в сокет байт
 * @param bid    идентификатор брокера
 * @param sid    идентификатор схемы сети
 * @param core   объект сетевого ядра
 */
void awh::server::Sample::writeCallback(const char * buffer, const size_t size, const uint64_t bid, const uint16_t sid, awh::core_t * core) noexcept {
	// Если данные существуют
	if((bid > 0) && (sid > 0) && (core != nullptr)){
		// Получаем параметры активного клиента
		sample_scheme_t::options_t * options = const_cast <sample_scheme_t::options_t *> (this->_scheme.get(bid));
		// Если параметры активного клиента получены
		if(options != nullptr){
			// Если необходимо выполнить закрыть подключение
			if(!options->close && options->stopped){
				// Устанавливаем флаг закрытия подключения
				options->close = !options->close;
				// Принудительно выполняем отключение лкиента
				core->close(bid);
			}
		}
	}
}
/**
 * acceptCallback Функция обратного вызова при проверке подключения брокера
 * @param ip   адрес интернет подключения брокера
 * @param mac  мак-адрес подключившегося брокера
 * @param port порт подключившегося брокера
 * @param sid  идентификатор схемы сети
 * @param core объект сетевого ядра
 * @return     результат разрешения к подключению брокера
 */
bool awh::server::Sample::acceptCallback(const string & ip, const string & mac, const u_int port, const uint16_t sid, awh::core_t * core) noexcept {
	// Результат работы функции
	bool result = true;
	// Если данные существуют
	if(!ip.empty() && !mac.empty() && (sid > 0) && (core != nullptr)){
		// Если функция обратного вызова установлена
		if(this->_callback.is("accept"))
			// Выводим функцию обратного вызова
			return this->_callback.apply <bool, const string &, const string &, const u_int> ("accept", ip, mac, port);
	}
	// Разрешаем подключение брокеру
	return result;
}
/**
 * erase Метод удаления отключившихся клиентов
 * @param tid  идентификатор таймера
 * @param core объект сетевого ядра
 */
void awh::server::Sample::erase(const uint16_t tid, awh::core_t * core) noexcept {
	// Если список отключившихся клиентов не пустой
	if((tid > 0) && (core != nullptr) && !this->_disconnected.empty()){
		// Получаем текущее значение времени
		const time_t date = this->_fmk->timestamp(fmk_t::stamp_t::MILLISECONDS);
		// Выполняем переход по всему списку отключившихся клиентов
		for(auto it = this->_disconnected.begin(); it != this->_disconnected.end();){
			// Если брокер уже давно удалился
			if((date - it->second) >= 5000){
				// Получаем параметры активного клиента
				sample_scheme_t::options_t * options = const_cast <sample_scheme_t::options_t *> (this->_scheme.get(it->first));
				// Если параметры активного клиента получены
				if(options != nullptr)
					// Устанавливаем флаг отключения
					options->close = true;
				// Выполняем удаление параметров брокера
				this->_scheme.rm(it->first);
				// Выполняем удаление объекта брокеров из списка мусора
				it = this->_disconnected.erase(it);
			// Выполняем пропуск брокера
			} else ++it;
		}
	}
}
/**
 * pinging Метод таймера выполнения пинга клиента
 * @param tid  идентификатор таймера
 * @param core объект сетевого ядра
 */
void awh::server::Sample::pinging(const uint16_t tid, awh::core_t * core) noexcept {
	// Если данные существуют
	if((tid > 0) && (core != nullptr)){
		// Выполняем перебор всех активных клиентов
		for(auto & item : this->_scheme.get()){
			// Если параметры активного клиента получены
			if((!item.second->alive && !this->_alive) || item.second->close){
				// Если брокер давно должен был быть отключён, отключаем его
				if(item.second->close)
					// Выполняем закрытие подключение брокера
					const_cast <server::core_t *> (this->_core)->close(item.first);
			}
		}
	}
}
/**
 * init Метод инициализации Rest брокера
 * @param socket unix-сокет для биндинга
 */
void awh::server::Sample::init(const string & socket) noexcept {
	/**
	 * Если операционной системой не является Windows
	 */
	#if !defined(_WIN32) && !defined(_WIN64)
		// Выполняем установку unix-сокет
		const_cast <server::core_t *> (this->_core)->unixSocket(socket);
	#endif
}
/**
 * init Метод инициализации Rest брокера
 * @param port порт сервера
 * @param host хост сервера
 */
void awh::server::Sample::init(const u_int port, const string & host) noexcept {
	// Устанавливаем порт сервера
	this->_port = port;
	// Устанавливаем хост сервера
	this->_host = host;
	/**
	 * Если операционной системой не является Windows
	 */
	#if !defined(_WIN32) && !defined(_WIN64)
		// Удаляем unix-сокет ранее установленный
		const_cast <server::core_t *> (this->_core)->removeUnixSocket();
	#endif
}
/**
 * on Метод установки функции обратного вызова на событие запуска или остановки подключения
 * @param callback функция обратного вызова
 */
void awh::server::Sample::on(function <void (const uint64_t, const mode_t)> callback) noexcept {
	// Устанавливаем функцию обратного вызова
	this->_callback.set <void (const uint64_t, const mode_t)> ("active", callback);
}
/**
 * on Метод установки функции обратного вызова на событие получения сообщений
 * @param callback функция обратного вызова
 */
void awh::server::Sample::on(function <void (const uint64_t, const vector <char> &)> callback) noexcept {
	// Устанавливаем функцию обратного вызова для получения входящих сообщений
	this->_callback.set <void (const uint64_t, const vector <char> &)> ("message", callback);
}
/**
 * on Метод установки функции обратного вызова получения событий запуска и остановки сетевого ядра
 * @param callback функция обратного вызова
 */
void awh::server::Sample::on(function <void (const awh::core_t::status_t, awh::core_t *)> callback) noexcept {
	// Устанавливаем функцию обратного вызова
	this->_callback.set <void (const awh::core_t::status_t, awh::core_t *)> ("events", callback);
}
/**
 * on Метод установки функции обратного вызова на событие активации брокера на сервере
 * @param callback функция обратного вызова
 */
void awh::server::Sample::on(function <bool (const string &, const string &, const u_int)> callback) noexcept {
	// Устанавливаем функцию обратного вызова
	this->_callback.set <bool (const string &, const string &, const u_int)> ("accept", callback);
}
/**
 * response Метод отправки сообщения брокеру
 * @param bid    идентификатор брокера
 * @param buffer буфер бинарных данных для отправки
 * @param size   размер бинарных данных для отправки
 */
void awh::server::Sample::send(const uint64_t bid, const char * buffer, const size_t size) const noexcept {
	// Если подключение выполнено
	if(this->_core->working()){
		// Получаем параметры активного клиента
		sample_scheme_t::options_t * options = const_cast <sample_scheme_t::options_t *> (this->_scheme.get(bid));
		// Если параметры активного клиента получены
		if((options->stopped = (options != nullptr))){
			// Если включён режим отладки
			#if defined(DEBUG_MODE)
				// Выводим заголовок ответа
				cout << "\x1B[33m\x1B[1m^^^^^^^^^ RESPONSE ^^^^^^^^^\x1B[0m" << endl;
				// Выводим параметры ответа
				cout << string(buffer, size) << endl;
			#endif
			// Отправляем тело на сервер
			const_cast <server::core_t *> (this->_core)->write(buffer, size, bid);
		}
	}
}
/**
 * port Метод получения порта подключения брокера
 * @param bid идентификатор брокера
 * @return    порт подключения брокера
 */
u_int awh::server::Sample::port(const uint64_t bid) const noexcept {
	// Выводим результат
	return this->_scheme.port(bid);
}
/**
 * ip Метод получения IP-адреса брокера
 * @param bid идентификатор брокера
 * @return    адрес интернет подключения брокера
 */
const string & awh::server::Sample::ip(const uint64_t bid) const noexcept {
	// Выводим результат
	return this->_scheme.ip(bid);
}
/**
 * mac Метод получения MAC-адреса брокера
 * @param bid идентификатор брокера
 * @return    адрес устройства брокера
 */
const string & awh::server::Sample::mac(const uint64_t bid) const noexcept {
	// Выводим результат
	return this->_scheme.mac(bid);
}
/**
 * alive Метод установки долгоживущего подключения
 * @param mode флаг долгоживущего подключения
 */
void awh::server::Sample::alive(const bool mode) noexcept {
	// Устанавливаем флаг долгоживущего подключения
	this->_alive = mode;
}
/**
 * alive Метод установки долгоживущего подключения
 * @param bid  идентификатор брокера
 * @param mode флаг долгоживущего подключения
 */
void awh::server::Sample::alive(const uint64_t bid, const bool mode) noexcept {
	// Получаем параметры активного клиента
	sample_scheme_t::options_t * options = const_cast <sample_scheme_t::options_t *> (this->_scheme.get(bid));
	// Если параметры активного клиента получены, устанавливаем флаг пдолгоживущего подключения
	if(options != nullptr)
		// Устанавливаем долгоживущее подключение
		options->alive = mode;
}
/**
 * stop Метод остановки сервера
 */
void awh::server::Sample::stop() noexcept {
	// Если подключение выполнено
	if(this->_core->working())
		// Завершаем работу, если разрешено остановить
		const_cast <server::core_t *> (this->_core)->stop();
}
/**
 * start Метод запуска сервера
 */
void awh::server::Sample::start() noexcept {
	// Если биндинг не запущен, выполняем запуск биндинга
	if(!this->_core->working())
		// Выполняем запуск биндинга
		const_cast <server::core_t *> (this->_core)->start();
}
/**
 * close Метод закрытия подключения брокера
 * @param bid идентификатор брокера
 */
void awh::server::Sample::close(const uint64_t bid) noexcept {
	// Получаем параметры активного клиента
	sample_scheme_t::options_t * options = const_cast <sample_scheme_t::options_t *> (this->_scheme.get(bid));
	// Если параметры активного клиента получены, устанавливаем флаг закрытия подключения
	if((options->close = (options != nullptr)))
		// Выполняем отключение брокера
		const_cast <server::core_t *> (this->_core)->close(bid);
}
/**
 * waitTimeDetect Метод детекции сообщений по количеству секунд
 * @param read  количество секунд для детекции по чтению
 * @param write количество секунд для детекции по записи
 */
void awh::server::Sample::waitTimeDetect(const time_t read, const time_t write) noexcept {
	// Устанавливаем количество секунд на чтение
	this->_scheme.timeouts.read = read;
	// Устанавливаем количество секунд на запись
	this->_scheme.timeouts.write = write;
}
/**
 * bytesDetect Метод детекции сообщений по количеству байт
 * @param read  количество байт для детекции по чтению
 * @param write количество байт для детекции по записи
 */
void awh::server::Sample::bytesDetect(const scheme_t::mark_t read, const scheme_t::mark_t write) noexcept {
	// Устанавливаем количество байт на чтение
	this->_scheme.marker.read = read;
	// Устанавливаем количество байт на запись
	this->_scheme.marker.write = write;
	// Если минимальный размер данных для чтения, не установлен
	if(this->_scheme.marker.read.min == 0)
		// Устанавливаем размер минимальных для чтения данных по умолчанию
		this->_scheme.marker.read.min = BUFFER_READ_MIN;
	// Если максимальный размер данных для записи не установлен, устанавливаем по умолчанию
	if(this->_scheme.marker.write.max == 0)
		// Устанавливаем размер максимальных записываемых данных по умолчанию
		this->_scheme.marker.write.max = BUFFER_WRITE_MAX;
}
/**
 * total Метод установки максимального количества одновременных подключений
 * @param total максимальное количество одновременных подключений
 */
void awh::server::Sample::total(const u_short total) noexcept {
	// Устанавливаем максимальное количество одновременных подключений
	const_cast <server::core_t *> (this->_core)->total(this->_scheme.sid, total);
}
/**
 * mode Метод установки флага модуля
 * @param flag флаг модуля для установки
 */
void awh::server::Sample::mode(const set <flag_t> & flags) noexcept {
	// Устанавливаем флаг ожидания входящих сообщений
	this->_scheme.wait = (flags.count(flag_t::WAIT_MESS) > 0);
	// Устанавливаем флаг запрещающий вывод информационных сообщений
	const_cast <server::core_t *> (this->_core)->noInfo(flags.count(flag_t::NOT_INFO) > 0);
	// Выполняем установку флага проверки домена
	const_cast <server::core_t *> (this->_core)->verifySSL(flags.count(flag_t::VERIFY_SSL) > 0);
}
/**
 * clusterAutoRestart Метод установки флага перезапуска процессов
 * @param mode флаг перезапуска процессов
 */
void awh::server::Sample::clusterAutoRestart(const bool mode) noexcept {
	// Выполняем установку флага автоматического перезапуска
	const_cast <server::core_t *> (this->_core)->clusterAutoRestart(this->_scheme.sid, mode);
}
/**
 * keepAlive Метод установки жизни подключения
 * @param cnt   максимальное количество попыток
 * @param idle  интервал времени в секундах через которое происходит проверка подключения
 * @param intvl интервал времени в секундах между попытками
 */
void awh::server::Sample::keepAlive(const int cnt, const int idle, const int intvl) noexcept {
	// Выполняем установку максимального количества попыток
	this->_scheme.keepAlive.cnt = cnt;
	// Выполняем установку интервала времени в секундах через которое происходит проверка подключения
	this->_scheme.keepAlive.idle = idle;
	// Выполняем установку интервала времени в секундах между попытками
	this->_scheme.keepAlive.intvl = intvl;
}
/**
 * Sample Конструктор
 * @param core объект сетевого ядра
 * @param fmk  объект фреймворка
 * @param log  объект для работы с логами
 */
awh::server::Sample::Sample(const server::core_t * core, const fmk_t * fmk, const log_t * log) noexcept :
 _pid(getpid()), _alive(false), _port(SERVER_PORT), _host{""}, _uri(fmk), _timer(fmk, log),
 _callback(log), _scheme(fmk, log), _cipher(hash_t::cipher_t::AES128), _fmk(fmk), _log(log), _core(core) {
	// Выполняем отключение информационных сообщений сетевого ядра таймера
	this->_timer.noInfo(true);
	// Добавляем схему сети в сетевое ядро
	const_cast <server::core_t *> (this->_core)->add(&this->_scheme);
	// Устанавливаем функцию активации ядра сервера
	const_cast <server::core_t *> (this->_core)->on(std::bind(&sample_t::eventsCallback, this, _1, _2));
	// Устанавливаем событие на запуск системы
	this->_scheme.callback.set <void (const uint16_t, awh::core_t *)> ("open", std::bind(&sample_t::openCallback, this, _1, _2));
	// Устанавливаем событие подключения
	this->_scheme.callback.set <void (const uint64_t, const uint16_t, awh::core_t *)> ("connect", std::bind(&sample_t::connectCallback, this, _1, _2, _3));
	// Устанавливаем событие отключения
	this->_scheme.callback.set <void (const uint64_t, const uint16_t, awh::core_t *)> ("disconnect", std::bind(&sample_t::disconnectCallback, this, _1, _2, _3));
	// Устанавливаем функцию чтения данных
	this->_scheme.callback.set <void (const char *, const size_t, const uint64_t, const uint16_t, awh::core_t *)> ("read", std::bind(&sample_t::readCallback, this, _1, _2, _3, _4, _5));
	// Устанавливаем функцию записи данных
	this->_scheme.callback.set <void (const char *, const size_t, const uint64_t, const uint16_t, awh::core_t *)> ("write", std::bind(&sample_t::writeCallback, this, _1, _2, _3, _4, _5));
	// Добавляем событие аццепта брокера
	this->_scheme.callback.set <bool (const string &, const string &, const u_int, const uint64_t, awh::core_t *)> ("accept", std::bind(&sample_t::acceptCallback, this, _1, _2, _3, _4, _5));
}
