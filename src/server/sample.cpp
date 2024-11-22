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
 * openEvent Метод обратного вызова при запуске работы
 * @param sid идентификатор схемы сети
 */
void awh::server::Sample::openEvent(const uint16_t sid) noexcept {
	// Если данные существуют
	if((this->_core != nullptr) && (sid > 0)){
		// Устанавливаем хост сервера
		const_cast <server::core_t *> (this->_core)->init(sid, this->_port, this->_host);
		// Выполняем запуск сервера
		const_cast <server::core_t *> (this->_core)->launch(sid);
	}
}
/**
 * statusEvent Метод обратного вызова при активации ядра сервера
 * @param status флаг запуска/остановки
 */
void awh::server::Sample::statusEvent(const awh::core_t::status_t status) noexcept {
	// Если объект сетевого ядра установлен
	if(this->_core != nullptr){
		// Определяем статус активности сетевого ядра
		switch(static_cast <uint8_t> (status)){
			// Если система запущена
			case static_cast <uint8_t> (awh::core_t::status_t::START): {
				// Выполняем биндинг ядра локального таймера
				const_cast <server::core_t *> (this->_core)->bind(&this->_timer);
				// Устанавливаем интервал времени на выполнения пинга клиента
				uint16_t tid = this->_timer.interval(this->_pingInterval);
				// Выполняем добавление функции обратного вызова
				this->_timer.set <void (const uint16_t)> (tid, std::bind(&sample_t::pinging, this, tid));
				// Устанавливаем интервал времени на удаление отключившихся клиентов раз в 3 секунды
				tid = this->_timer.interval(3000);
				// Выполняем добавление функции обратного вызова
				this->_timer.set <void (const uint16_t)> (tid, std::bind(&sample_t::erase, this, tid));
			} break;
			// Если система остановлена
			case static_cast <uint8_t> (awh::core_t::status_t::STOP): {
				// Останавливаем все установленные таймеры
				this->_timer.clear();
				// Выполняем анбиндинг ядра локального таймера
				const_cast <server::core_t *> (this->_core)->unbind(&this->_timer);
			} break;
		}
		// Если функция получения событий запуска и остановки сетевого ядра установлена
		if(this->_callbacks.is("status"))
			// Выводим функцию обратного вызова
			this->_callbacks.call <void (const awh::core_t::status_t)> ("status", status);
	}
}
/**
 * connectEvent Метод обратного вызова при подключении к серверу
 * @param bid идентификатор брокера
 * @param sid идентификатор схемы сети
 */
void awh::server::Sample::connectEvent(const uint64_t bid, const uint16_t sid) noexcept {
	// Если данные переданы верные
	if((bid > 0) && (sid > 0)){
		// Создаём брокера
		this->_scheme.set(bid);
		// Если функция обратного вызова при подключении/отключении установлена
		if(this->_callbacks.is("active"))
			// Выводим функцию обратного вызова
			this->_callbacks.call <void (const uint64_t, const mode_t)> ("active", bid, mode_t::CONNECT);
	}
}
/**
 * disconnectEvent Метод обратного вызова при отключении от сервера
 * @param bid идентификатор брокера
 * @param sid идентификатор схемы сети
 */
void awh::server::Sample::disconnectEvent(const uint64_t bid, const uint16_t sid) noexcept {
	// Если данные переданы верные
	if((bid > 0) && (sid > 0)){
		// Добавляем в очередь список отключившихся клиентов
		this->_disconnected.emplace(bid, this->_fmk->timestamp(fmk_t::stamp_t::MILLISECONDS));
		// Если функция обратного вызова при подключении/отключении установлена
		if(this->_callbacks.is("active"))
			// Выводим функцию обратного вызова
			this->_callbacks.call <void (const uint64_t, const mode_t)> ("active", bid, mode_t::DISCONNECT);
	}
}
/**
 * readEvent Метод обратного вызова при чтении сообщения с брокера
 * @param buffer бинарный буфер содержащий сообщение
 * @param size   размер бинарного буфера содержащего сообщение
 * @param bid    идентификатор брокера
 * @param sid    идентификатор схемы сети
 */
void awh::server::Sample::readEvent(const char * buffer, const size_t size, const uint64_t bid, const uint16_t sid) noexcept {
	// Если данные существуют
	if((buffer != nullptr) && (size > 0) && (bid > 0) && (sid > 0)){
		// Получаем параметры активного клиента
		scheme::sample_t::options_t * options = const_cast <scheme::sample_t::options_t *> (this->_scheme.get(bid));
		// Если параметры активного клиента получены
		if(options != nullptr){
			// Выполняем сброс закрытия подключения
			options->close = false;
			// Если разрешено получение данных
			if(options->allow.receive){
				// Если функция обратного вызова при получении входящих сообщений установлена
				if(this->_callbacks.is("message"))
					// Выводим данные полученного сообщения
					this->_callbacks.call <void (const uint64_t, const vector <char> &)> ("message", bid, vector <char> (buffer, buffer + size));
			}
		}
	}
}
/**
 * writeEvent Метод обратного вызова при записи сообщение брокеру
 * @param buffer бинарный буфер содержащий сообщение
 * @param size   размер записанных в сокет байт
 * @param bid    идентификатор брокера
 * @param sid    идентификатор схемы сети
 */
void awh::server::Sample::writeEvent(const char * buffer, const size_t size, const uint64_t bid, const uint16_t sid) noexcept {
	// Если данные существуют
	if((this->_core != nullptr) && (bid > 0) && (sid > 0)){
		// Получаем параметры активного клиента
		scheme::sample_t::options_t * options = const_cast <scheme::sample_t::options_t *> (this->_scheme.get(bid));
		// Если параметры активного клиента получены
		if(options != nullptr){
			// Если необходимо выполнить закрыть подключение
			if(!options->close && options->stopped){
				// Устанавливаем флаг закрытия подключения
				options->close = !options->close;
				// Принудительно выполняем отключение лкиента
				const_cast <server::core_t *> (this->_core)->close(bid);
			}
		}
	}
}
/**
 * acceptEvent Функция обратного вызова при проверке подключения брокера
 * @param ip   адрес интернет подключения брокера
 * @param mac  мак-адрес подключившегося брокера
 * @param port порт подключившегося брокера
 * @param sid  идентификатор схемы сети
 * @return     результат разрешения к подключению брокера
 */
bool awh::server::Sample::acceptEvent(const string & ip, const string & mac, const uint32_t port, const uint16_t sid) noexcept {
	// Результат работы функции
	bool result = true;
	// Если данные существуют
	if(!ip.empty() && !mac.empty() && (sid > 0)){
		// Если функция обратного вызова установлена
		if(this->_callbacks.is("accept"))
			// Выводим функцию обратного вызова
			return this->_callbacks.call <bool (const string &, const string &, const uint32_t)> ("accept", ip, mac, port);
	}
	// Разрешаем подключение брокеру
	return result;
}
/**
 * erase Метод удаления отключившихся клиентов
 * @param tid идентификатор таймера
 */
void awh::server::Sample::erase(const uint16_t tid) noexcept {
	// Если список отключившихся клиентов не пустой
	if((tid > 0) && !this->_disconnected.empty()){
		// Получаем текущее значение времени
		const time_t date = this->_fmk->timestamp(fmk_t::stamp_t::MILLISECONDS);
		// Выполняем переход по всему списку отключившихся клиентов
		for(auto i = this->_disconnected.begin(); i != this->_disconnected.end();){
			// Если брокер уже давно удалился
			if((date - i->second) >= 3000){
				// Получаем параметры активного клиента
				scheme::sample_t::options_t * options = const_cast <scheme::sample_t::options_t *> (this->_scheme.get(i->first));
				// Если параметры активного клиента получены
				if(options != nullptr)
					// Устанавливаем флаг отключения
					options->close = true;
				// Выполняем удаление параметров брокера
				this->_scheme.rm(i->first);
				// Выполняем удаление объекта брокеров из списка мусора
				i = this->_disconnected.erase(i);
			// Выполняем пропуск брокера
			} else ++i;
		}
	}
}
/**
 * pinging Метод таймера выполнения пинга клиента
 * @param tid идентификатор таймера
 */
void awh::server::Sample::pinging(const uint16_t tid) noexcept {
	// Если данные существуют
	if((this->_core != nullptr) && (tid > 0)){
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
		// Если объект сетевого ядра установлен
		if(this->_core != nullptr)
			// Выполняем установку unix-сокет
			const_cast <server::core_t *> (this->_core)->sockname(socket);
	#endif
}
/**
 * init Метод инициализации Rest брокера
 * @param port порт сервера
 * @param host хост сервера
 */
void awh::server::Sample::init(const uint32_t port, const string & host) noexcept {
	// Устанавливаем порт сервера
	this->_port = port;
	// Устанавливаем хост сервера
	this->_host = host;
}
/**
 * callbacks Метод установки функций обратного вызова
 * @param callbacks функции обратного вызова
 */
void awh::server::Sample::callbacks(const fn_t & callbacks) noexcept {
	// Выполняем установку функции обратного вызова на событие запуска или остановки подключения
	this->_callbacks.set("active", callbacks);
	// Выполняем установку функции обратного вызова получения событий запуска и остановки сетевого ядра
	this->_callbacks.set("status", callbacks);
	// Выполняем установку функции обратного вызова на событие активации брокера на сервере
	this->_callbacks.set("accept", callbacks);
	// Выполняем установку функции обратного вызова на событие получения сообщений
	this->_callbacks.set("message", callbacks);
}
/**
 * response Метод отправки сообщения брокеру
 * @param bid    идентификатор брокера
 * @param buffer буфер бинарных данных для отправки
 * @param size   размер бинарных данных для отправки
 */
void awh::server::Sample::send(const uint64_t bid, const char * buffer, const size_t size) const noexcept {
	// Если подключение выполнено
	if((this->_core != nullptr) && this->_core->working()){
		// Получаем параметры активного клиента
		scheme::sample_t::options_t * options = const_cast <scheme::sample_t::options_t *> (this->_scheme.get(bid));
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
uint32_t awh::server::Sample::port(const uint64_t bid) const noexcept {
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
	scheme::sample_t::options_t * options = const_cast <scheme::sample_t::options_t *> (this->_scheme.get(bid));
	// Если параметры активного клиента получены, устанавливаем флаг пдолгоживущего подключения
	if(options != nullptr)
		// Устанавливаем долгоживущее подключение
		options->alive = mode;
}
/**
 * stop Метод остановки сервера
 */
void awh::server::Sample::stop() noexcept {
	// Если объект сетевого ядра установлен
	if(this->_core != nullptr){
		// Если завершить работу разрешено
		if(this->_complete && this->_core->working())
			// Завершаем работу, если разрешено остановить
			const_cast <server::core_t *> (this->_core)->stop();
		// Если завершать работу запрещено, просто отключаемся
		else const_cast <server::core_t *> (this->_core)->close();
	}
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
	// Если объект сетевого ядра установлен
	if(this->_core != nullptr){
		// Получаем параметры активного клиента
		scheme::sample_t::options_t * options = const_cast <scheme::sample_t::options_t *> (this->_scheme.get(bid));
		// Если параметры активного клиента получены, устанавливаем флаг закрытия подключения
		if((options->close = (options != nullptr)))
			// Выполняем отключение брокера
			const_cast <server::core_t *> (this->_core)->close(bid);
	}
}
/**
 * pingInterval Метод установки интервала времени выполнения пингов
 * @param sec интервал времени выполнения пингов в секундах
 */
void awh::server::Sample::pingInterval(const time_t sec) noexcept {
	// Выполняем установку интервала времени выполнения пингов
	this->_pingInterval = (sec * 1000);
}
/**
 * waitMessage Метод ожидания входящих сообщений
 * @param sec интервал времени в секундах
 */
void awh::server::Sample::waitMessage(const time_t sec) noexcept {
	// Устанавливаем время ожидания получения данных
	this->_scheme.timeouts.wait = sec;
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
 * total Метод установки максимального количества одновременных подключений
 * @param total максимальное количество одновременных подключений
 */
void awh::server::Sample::total(const uint16_t total) noexcept {
	// Если объект сетевого ядра установлен
	if(this->_core != nullptr)
		// Устанавливаем максимальное количество одновременных подключений
		const_cast <server::core_t *> (this->_core)->total(this->_scheme.id, total);
}
/**
 * mode Метод установки флага модуля
 * @param flag флаг модуля для установки
 */
void awh::server::Sample::mode(const std::set <flag_t> & flags) noexcept {
	// Если объект сетевого ядра установлен
	if(this->_core != nullptr){
		// Активируем выполнение пинга
		this->_pinging = (flags.find(flag_t::NOT_PING) == flags.end());
		// Устанавливаем флаг анбиндинга ядра сетевого модуля
		this->_complete = (flags.find(flag_t::NOT_STOP) == flags.end());
		// Устанавливаем флаг запрещающий вывод информационных сообщений
		const_cast <server::core_t *> (this->_core)->verbose(flags.find(flag_t::NOT_INFO) == flags.end());
	}
}
/**
 * keepAlive Метод установки жизни подключения
 * @param cnt   максимальное количество попыток
 * @param idle  интервал времени в секундах через которое происходит проверка подключения
 * @param intvl интервал времени в секундах между попытками
 */
void awh::server::Sample::keepAlive(const int32_t cnt, const int32_t idle, const int32_t intvl) noexcept {
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
 _pid(::getpid()), _alive(false), _pinging(true), _complete(true),
 _port(SERVER_PORT), _host{""}, _uri(fmk), _timer(fmk, log), _pingInterval(PING_INTERVAL),
 _callbacks(log), _scheme(fmk, log), _cipher(hash_t::cipher_t::AES128), _fmk(fmk), _log(log), _core(core) {
	// Если объект сетевого ядра установлен
	if(this->_core != nullptr){
		// Выполняем отключение информационных сообщений сетевого ядра таймера
		this->_timer.verbose(false);
		// Добавляем схему сети в сетевое ядро
		const_cast <server::core_t *> (this->_core)->scheme(&this->_scheme);
		// Устанавливаем функцию активации ядра сервера
		const_cast <server::core_t *> (this->_core)->callback <void (const awh::core_t::status_t)> ("status", std::bind(&sample_t::statusEvent, this, _1));
		// Устанавливаем событие на запуск системы
		const_cast <server::core_t *> (this->_core)->callback <void (const uint16_t)> ("open", std::bind(&sample_t::openEvent, this, _1));
		// Устанавливаем событие подключения
		const_cast <server::core_t *> (this->_core)->callback <void (const uint64_t, const uint16_t)> ("connect", std::bind(&sample_t::connectEvent, this, _1, _2));
		// Устанавливаем событие отключения
		const_cast <server::core_t *> (this->_core)->callback <void (const uint64_t, const uint16_t)> ("disconnect", std::bind(&sample_t::disconnectEvent, this, _1, _2));
		// Устанавливаем функцию чтения данных
		const_cast <server::core_t *> (this->_core)->callback <void (const char *, const size_t, const uint64_t, const uint16_t)> ("read", std::bind(&sample_t::readEvent, this, _1, _2, _3, _4));
		// Устанавливаем функцию записи данных
		const_cast <server::core_t *> (this->_core)->callback <void (const char *, const size_t, const uint64_t, const uint16_t)> ("write", std::bind(&sample_t::writeEvent, this, _1, _2, _3, _4));
		// Добавляем событие аццепта брокера
		const_cast <server::core_t *> (this->_core)->callback <bool (const string &, const string &, const uint32_t, const uint64_t)> ("accept", std::bind(&sample_t::acceptEvent, this, _1, _2, _3, _4));
	}
}
