/**
 * @file: sample.cpp
 * @date: 2022-09-01
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
#include <server/sample.hpp>

/**
 * openCallback Функция обратного вызова при запуске работы
 * @param sid  идентификатор схемы сети
 * @param core объект сетевого ядра
 */
void awh::server::Sample::openCallback(const size_t sid, awh::core_t * core) noexcept {
	// Если данные существуют
	if((sid > 0) && (core != nullptr)){
		// Устанавливаем хост сервера
		reinterpret_cast <server::core_t *> (core)->init(sid, this->_port, this->_host);
		// Выполняем запуск сервера
		reinterpret_cast <server::core_t *> (core)->run(sid);
	}
}
/**
 * persistCallback Функция персистентного вызова
 * @param aid  идентификатор адъютанта
 * @param sid  идентификатор схемы сети
 * @param core объект сетевого ядра
 */
void awh::server::Sample::persistCallback(const size_t aid, const size_t sid, awh::core_t * core) noexcept {
	// Если данные существуют
	if((aid > 0) && (sid > 0) && (core != nullptr)){
		// Получаем параметры подключения адъютанта
		sample_scheme_t::coffer_t * adj = const_cast <sample_scheme_t::coffer_t *> (this->_scheme.get(aid));
		// Если параметры подключения адъютанта получены
		if((adj != nullptr) && ((!adj->alive && !this->_alive) || adj->close))
			// Если адъютант давно должен был быть отключён, отключаем его
			if(adj->close) reinterpret_cast <server::core_t *> (core)->close(aid);
	}
}
/**
 * connectCallback Функция обратного вызова при подключении к серверу
 * @param aid  идентификатор адъютанта
 * @param sid  идентификатор схемы сети
 * @param core объект сетевого ядра
 */
void awh::server::Sample::connectCallback(const size_t aid, const size_t sid, awh::core_t * core) noexcept {
	// Если данные переданы верные
	if((aid > 0) && (sid > 0) && (core != nullptr)){
		// Создаём адъютанта
		this->_scheme.set(aid);
		// Получаем параметры подключения адъютанта
		sample_scheme_t::coffer_t * adj = const_cast <sample_scheme_t::coffer_t *> (this->_scheme.get(aid));
		// Если параметры подключения адъютанта получены
		if(adj != nullptr){
			// Устанавливаем экшен выполнения
			adj->action = sample_scheme_t::action_t::CONNECT;
			// Выполняем запуск обработчика событий
			this->handler(aid);
		}
	}
}
/**
 * disconnectCallback Функция обратного вызова при отключении от сервера
 * @param aid  идентификатор адъютанта
 * @param sid  идентификатор схемы сети
 * @param core объект сетевого ядра
 */
void awh::server::Sample::disconnectCallback(const size_t aid, const size_t sid, awh::core_t * core) noexcept {
	// Если данные переданы верные
	if((aid > 0) && (sid > 0) && (core != nullptr)){
		// Получаем параметры подключения адъютанта
		sample_scheme_t::coffer_t * adj = const_cast <sample_scheme_t::coffer_t *> (this->_scheme.get(aid));
		// Если параметры подключения адъютанта получены
		if(adj != nullptr){
			// Устанавливаем экшен выполнения
			adj->action = sample_scheme_t::action_t::DISCONNECT;
			// Выполняем запуск обработчика событий
			this->handler(aid);
		}
	}
}
/**
 * readCallback Функция обратного вызова при чтении сообщения с адъютанта
 * @param buffer бинарный буфер содержащий сообщение
 * @param size   размер бинарного буфера содержащего сообщение
 * @param aid    идентификатор адъютанта
 * @param sid    идентификатор схемы сети
 * @param core   объект сетевого ядра
 */
void awh::server::Sample::readCallback(const char * buffer, const size_t size, const size_t aid, const size_t sid, awh::core_t * core) noexcept {
	// Если данные существуют
	if((buffer != nullptr) && (size > 0) && (aid > 0) && (sid > 0)){
		// Получаем параметры подключения адъютанта
		sample_scheme_t::coffer_t * adj = const_cast <sample_scheme_t::coffer_t *> (this->_scheme.get(aid));
		// Если параметры подключения адъютанта получены
		if(adj != nullptr){
			// Если дисконнекта ещё не произошло
			if((adj->action == sample_scheme_t::action_t::NONE) || (adj->action == sample_scheme_t::action_t::READ)){
				// Выполняем сброс закрытия подключения
				adj->close = false;
				// Если разрешено получение данных
				if(adj->allow.receive){
					// Устанавливаем экшен выполнения
					adj->action = sample_scheme_t::action_t::READ;
					// Добавляем полученные данные в буфер
					adj->buffer.insert(adj->buffer.end(), buffer, buffer + size);
					// Выполняем запуск обработчика событий
					this->handler(aid);
				}
			}
		}
	}
}
/**
 * writeCallback Функция обратного вызова при записи сообщение адъютанту
 * @param buffer бинарный буфер содержащий сообщение
 * @param size   размер записанных в сокет байт
 * @param aid    идентификатор адъютанта
 * @param sid    идентификатор схемы сети
 * @param core   объект сетевого ядра
 */
void awh::server::Sample::writeCallback(const char * buffer, const size_t size, const size_t aid, const size_t sid, awh::core_t * core) noexcept {
	// Если данные существуют
	if((aid > 0) && (sid > 0) && (core != nullptr)){
		// Получаем параметры подключения адъютанта
		sample_scheme_t::coffer_t * adj = const_cast <sample_scheme_t::coffer_t *> (this->_scheme.get(aid));
		// Если параметры подключения адъютанта получены
		if(adj != nullptr){
			// Если необходимо выполнить закрыть подключение
			if(!adj->close && adj->stopped){
				// Выполняем очистку оставшихся данных
				adj->buffer.clear();
				// Устанавливаем флаг закрытия подключения
				adj->close = !adj->close;
				// Принудительно выполняем отключение лкиента
				const_cast <server::core_t *> (this->_core)->close(aid);
			}
		}
	}
}
/**
 * acceptCallback Функция обратного вызова при проверке подключения адъютанта
 * @param ip   адрес интернет подключения адъютанта
 * @param mac  мак-адрес подключившегося адъютанта
 * @param port порт подключившегося адъютанта
 * @param sid  идентификатор схемы сети
 * @param core объект сетевого ядра
 * @return     результат разрешения к подключению адъютанта
 */
bool awh::server::Sample::acceptCallback(const string & ip, const string & mac, const u_int port, const size_t sid, awh::core_t * core) noexcept {
	// Результат работы функции
	bool result = true;
	// Если данные существуют
	if(!ip.empty() && !mac.empty() && (sid > 0) && (core != nullptr)){
		// Если функция обратного вызова установлена, проверяем
		if(this->_callback.accept != nullptr)
			// Выполняем проверку на разрешение подключения
			result = this->_callback.accept(ip, mac, port, this);
	}
	// Разрешаем подключение адъютанту
	return result;
}
/**
 * handler Метод управления входящими методами
 * @param aid идентификатор адъютанта
 */
void awh::server::Sample::handler(const size_t aid) noexcept {
	// Получаем параметры подключения адъютанта
	sample_scheme_t::coffer_t * adj = const_cast <sample_scheme_t::coffer_t *> (this->_scheme.get(aid));
	// Если параметры подключения адъютанта получены
	if(adj != nullptr){
		// Если управляющий блокировщик не заблокирован
		if(!adj->locker.mode){
			// Выполняем блокировку потока
			const lock_guard <recursive_mutex> lock(adj->locker.mtx);
			// Флаг разрешающий циклический перебор экшенов
			bool loop = true;
			// Выполняем блокировку обработчика
			adj->locker.mode = true;
			// Выполняем обработку всех экшенов
			while(loop && (adj->action != sample_scheme_t::action_t::NONE)){
				// Определяем обрабатываемый экшен
				switch((uint8_t) adj->action){
					// Если необходимо запустить экшен обработки данных поступающих с сервера
					case (uint8_t) sample_scheme_t::action_t::READ: this->actionRead(aid); break;
					// Если необходимо запустить экшен обработки подключения к серверу
					case (uint8_t) sample_scheme_t::action_t::CONNECT: this->actionConnect(aid); break;
					// Если необходимо запустить экшен обработки отключения от сервера
					case (uint8_t) sample_scheme_t::action_t::DISCONNECT: this->actionDisconnect(aid); break;
					// Если сработал неизвестный экшен, выходим
					default: loop = false;
				}
			}
			// Выполняем разблокировку обработчика
			adj->locker.mode = false;
		}
	}
}
/**
 * actionRead Метод обработки экшена чтения с сервера
 * @param aid идентификатор адъютанта
 */
void awh::server::Sample::actionRead(const size_t aid) noexcept {
	// Если данные существуют
	if(aid > 0){
		// Получаем параметры подключения адъютанта
		sample_scheme_t::coffer_t * adj = const_cast <sample_scheme_t::coffer_t *> (this->_scheme.get(aid));
		// Если параметры подключения адъютанта получены
		if(adj != nullptr){
			// Если экшен соответствует, выполняем его сброс
			if(adj->action == sample_scheme_t::action_t::READ)
				// Выполняем сброс экшена
				adj->action = sample_scheme_t::action_t::NONE;
			// Если функция обратного вызова, установлена
			if(this->_callback.message != nullptr)
				// Отправляем полученный результат
				this->_callback.message(aid, adj->buffer, const_cast <Sample *> (this));
		}
	}
}
/**
 * actionConnect Метод обработки экшена подключения к серверу
 * @param aid идентификатор адъютанта
 */
void awh::server::Sample::actionConnect(const size_t aid) noexcept {
	// Если данные существуют
	if(aid > 0){
		// Получаем параметры подключения адъютанта
		sample_scheme_t::coffer_t * adj = const_cast <sample_scheme_t::coffer_t *> (this->_scheme.get(aid));
		// Если параметры подключения адъютанта получены
		if(adj != nullptr){
			// Если экшен соответствует, выполняем его сброс
			if(adj->action == sample_scheme_t::action_t::CONNECT)
				// Выполняем сброс экшена
				adj->action = sample_scheme_t::action_t::NONE;
			// Если функция обратного вызова установлена
			if(this->_callback.active != nullptr)
				// Выполняем функцию обратного вызова
				this->_callback.active(aid, mode_t::CONNECT, this);
		}
	}
}
/**
 * actionDisconnect Метод обработки экшена отключения от сервера
 * @param aid идентификатор адъютанта
 */
void awh::server::Sample::actionDisconnect(const size_t aid) noexcept {
	// Если данные существуют
	if(aid > 0){
		// Получаем параметры подключения адъютанта
		sample_scheme_t::coffer_t * adj = const_cast <sample_scheme_t::coffer_t *> (this->_scheme.get(aid));
		// Если параметры подключения адъютанта получены
		if(adj != nullptr){
			// Устанавливаем флаг отключения
			adj->close = true;
			// Выполняем очистку оставшихся данных
			adj->buffer.clear();
			// Если экшен соответствует, выполняем его сброс
			if(adj->action == sample_scheme_t::action_t::DISCONNECT)
				// Выполняем сброс экшена
				adj->action = sample_scheme_t::action_t::NONE;
			// Выполняем удаление параметров адъютанта
			this->_scheme.rm(aid);
			// Если функция обратного вызова установлена, выполняем
			if(this->_callback.active != nullptr)
				// Выполняем функцию обратного вызова
				this->_callback.active(aid, mode_t::DISCONNECT, this);
		}
	}
}
/**
 * init Метод инициализации Rest адъютанта
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
 * init Метод инициализации Rest адъютанта
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
void awh::server::Sample::on(function <void (const size_t, const mode_t, Sample *)> callback) noexcept {
	// Устанавливаем функцию запуска и остановки
	this->_callback.active = callback;
}
/**
 * on Метод установки функции обратного вызова на событие получения сообщений
 * @param callback функция обратного вызова
 */
void awh::server::Sample::on(function <void (const size_t, const vector <char> &, Sample *)> callback) noexcept {
	// Устанавливаем функцию получения сообщений с сервера
	this->_callback.message = callback;
}
/**
 * on Метод установки функции обратного вызова на событие активации адъютанта на сервере
 * @param callback функция обратного вызова
 */
void awh::server::Sample::on(function <bool (const string &, const string &, const u_int, Sample *)> callback) noexcept {
	// Устанавливаем функцию запуска и остановки
	this->_callback.accept = callback;
}
/**
 * response Метод отправки сообщения адъютанту
 * @param aid    идентификатор адъютанта
 * @param buffer буфер бинарных данных для отправки
 * @param size   размер бинарных данных для отправки
 */
void awh::server::Sample::send(const size_t aid, const char * buffer, const size_t size) const noexcept {
	// Если подключение выполнено
	if(this->_core->working()){
		// Получаем параметры подключения адъютанта
		sample_scheme_t::coffer_t * adj = const_cast <sample_scheme_t::coffer_t *> (this->_scheme.get(aid));
		// Если параметры подключения адъютанта получены
		if(adj != nullptr){
			// Если включён режим отладки
			#if defined(DEBUG_MODE)
				// Выводим заголовок ответа
				cout << "\x1B[33m\x1B[1m^^^^^^^^^ RESPONSE ^^^^^^^^^\x1B[0m" << endl;
				// Выводим параметры ответа
				cout << string(buffer, size) << endl;
			#endif
			// Устанавливаем флаг завершения работы
			adj->stopped = true;
			// Отправляем тело на сервер
			((awh::core_t *) const_cast <server::core_t *> (this->_core))->write(buffer, size, aid);
		}
	}
}
/**
 * port Метод получения порта подключения адъютанта
 * @param aid идентификатор адъютанта
 * @return    порт подключения адъютанта
 */
u_int awh::server::Sample::port(const size_t aid) const noexcept {
	// Выводим результат
	return this->_scheme.getPort(aid);
}
/**
 * ip Метод получения IP адреса адъютанта
 * @param aid идентификатор адъютанта
 * @return    адрес интернет подключения адъютанта
 */
const string & awh::server::Sample::ip(const size_t aid) const noexcept {
	// Выводим результат
	return this->_scheme.getIp(aid);
}
/**
 * mac Метод получения MAC адреса адъютанта
 * @param aid идентификатор адъютанта
 * @return    адрес устройства адъютанта
 */
const string & awh::server::Sample::mac(const size_t aid) const noexcept {
	// Выводим результат
	return this->_scheme.getMac(aid);
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
 * @param aid  идентификатор адъютанта
 * @param mode флаг долгоживущего подключения
 */
void awh::server::Sample::alive(const size_t aid, const bool mode) noexcept {
	// Получаем параметры подключения адъютанта
	sample_scheme_t::coffer_t * adj = const_cast <sample_scheme_t::coffer_t *> (this->_scheme.get(aid));
	// Если параметры подключения адъютанта получены, устанавливаем флаг пдолгоживущего подключения
	if(adj != nullptr) adj->alive = mode;
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
 * close Метод закрытия подключения адъютанта
 * @param aid идентификатор адъютанта
 */
void awh::server::Sample::close(const size_t aid) noexcept {
	// Получаем параметры подключения адъютанта
	sample_scheme_t::coffer_t * adj = const_cast <sample_scheme_t::coffer_t *> (this->_scheme.get(aid));
	// Если параметры подключения адъютанта получены, устанавливаем флаг закрытия подключения
	if(adj != nullptr){
		// Устанавливаем флаг закрытия подключения адъютанта
		adj->close = true;
		// Выполняем отключение адъютанта
		const_cast <server::core_t *> (this->_core)->close(aid);
	}
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
 * mode Метод установки флага модуля
 * @param flag флаг модуля для установки
 */
void awh::server::Sample::mode(const u_short flag) noexcept {
	// Устанавливаем флаг ожидания входящих сообщений
	this->_scheme.wait = (flag & (uint8_t) flag_t::WAIT_MESS);
	// Устанавливаем флаг запрещающий вывод информационных сообщений
	const_cast <server::core_t *> (this->_core)->noInfo(flag & (uint8_t) flag_t::NOT_INFO);
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
 _pid(getpid()), _port(SERVER_PORT), _host(""), _net(fmk, log), _uri(fmk, &_net), _scheme(fmk, log),
 _cipher(hash_t::cipher_t::AES128), _alive(false), _fmk(fmk), _log(log), _core(core) {
	// Устанавливаем событие на запуск системы
	this->_scheme.callback.set <void (const size_t, awh::core_t *)> ("open", std::bind(&Sample::openCallback, this, _1, _2));
	// Устанавливаем функцию персистентного вызова
	this->_scheme.callback.set <void (const size_t, const size_t, awh::core_t *)> ("persist", std::bind(&Sample::persistCallback, this, _1, _2, _3));
	// Устанавливаем событие подключения
	this->_scheme.callback.set <void (const size_t, const size_t, awh::core_t *)> ("connect", std::bind(&Sample::connectCallback, this, _1, _2, _3));
	// Устанавливаем событие отключения
	this->_scheme.callback.set <void (const size_t, const size_t, awh::core_t *)> ("disconnect", std::bind(&Sample::disconnectCallback, this, _1, _2, _3));
	// Устанавливаем функцию чтения данных
	this->_scheme.callback.set <void (const char *, const size_t, const size_t, const size_t, awh::core_t *)> ("read", std::bind(&Sample::readCallback, this, _1, _2, _3, _4, _5));
	// Устанавливаем функцию записи данных
	this->_scheme.callback.set <void (const char *, const size_t, const size_t, const size_t, awh::core_t *)> ("write", std::bind(&Sample::writeCallback, this, _1, _2, _3, _4, _5));
	// Добавляем событие аццепта адъютанта
	this->_scheme.callback.set <bool (const string &, const string &, const u_int, const size_t, awh::core_t *)> ("accept", std::bind(&Sample::acceptCallback, this, _1, _2, _3, _4, _5));
	// Активируем персистентный запуск для работы пингов
	const_cast <server::core_t *> (this->_core)->persistEnable(true);
	// Добавляем схему сети в сетевое ядро
	const_cast <server::core_t *> (this->_core)->add(&this->_scheme);
}
