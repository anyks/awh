/**
 * @file client.cpp
 * @date 2026-11-14
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
 * @brief Реализация модулей QUIC — связывание конечного автомата соединения QUIC с движком ввода-вывода:
 *        приём и отправка датаграмм, управление сессиями, миграция клиентского адреса,
 *        кластеризация сервера и возобновление сессии по 0-RTT
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Подключаем заголовочные файлы проекта
 */
#include <sys/fmk.hpp>
#include <sys/log.hpp>
#include <net/addr.hpp>
#include <unit/quic/client.hpp>

/**
 * Используем стандартное пространство имён
 */
using namespace std;

/**
 * Используем пространство имён placeholders
 */
using namespace placeholders;

/**
 * @brief Метод отправки готовых исходящих датаграмм соединения
 *
 */
void awh::unit::QuicClient::flush() noexcept {
	// Если соединение уничтожено приложением реентрантно из функции обратного вызова - отправлять нечего
	if(this->_connection == nullptr)
		// Выходим из метода
		return;
	/**
	 * Если удерживаемой датаграммы нет - извлекаем первую исходящую датаграмму соединения:
	 * она уходит первой при вытягивании, а остальные датаграммы движок заберёт сам
	 * через функцию-источник
	 */
	if(!this->_handed && this->_datagram.empty()){
		// Если исходящих датаграмм у соединения нет
		if(!this->_connection->write(this->_datagram, fmk::milliseconds()))
			// Выходим из метода - отправлять нечего
			return;
	}
	// Заводим вытягивание исходящих датаграмм сетевым движком
	engine::io::send(this->_eid, nullptr, 0);
}
/**
 * @brief Метод выдачи собранных данных потоков приложения
 *
 */
void awh::unit::QuicClient::process() noexcept {
	/**
	 * Соединение может быть уничтожено приложением реентрантно - из функции
	 * обратного вызова, вызываемой этим же методом (напр. "read" открывает путь
	 * к destroy()). После этого объект соединения освобождён, и продолжать выдачу
	 * нельзя (RFC здесь ни при чём - это защита от повторного входа)
	 */
	if(this->_connection == nullptr)
		// Выходим из метода
		return;
	/**
	 * Если для соединения установлено объединение данных (splice), собранные данные
	 * потоков и датаграмм перенаправляются в событие-приёмник вместо выдачи
	 * приложению - строится прозрачный канал между соединением и другим событием
	 */
	const bool spliced = (this->_dest != 0);
	// Если объединение установлено либо функция обратного вызова на собранные данные потока установлена
	if(spliced || this->_callback.is("read")){
		// Список потоков с собранными данными
		vector <uint64_t> streams;
		// Получаем список потоков с собранными данными
		this->_connection->readable(streams);
		/**
		 * Перебираем потоки с собранными данными
		 */
		for(auto & sid : streams){
			// Если соединение уничтожено приложением реентрантно из функции обратного вызова - прекращаем выдачу
			if(this->_connection == nullptr)
				// Выходим из метода
				return;
			// Флаг завершения потока удалённым эндпоинтом
			bool fin = false;
			// Собранные данные потока приложения
			string data = "";
			// Если выдача собранных данных потока не выполнена
			if(this->_connection->receive(sid, data, fin) != quic::status_t::OK)
				// Пропускаем поток с ошибкой выдачи
				continue;
			// Если данные получены либо поток завершён
			if(!data.empty() || fin){
				// Если для соединения установлено объединение данных - перенаправляем их в событие-приёмник
				if(spliced)
					// Перенаправляем собранные данные потока в событие-приёмник объединения
					this->forward(data);
				// Иначе выдаём собранные данные потока приложению
				else this->_callback.call <void (const event::id_t, const uint64_t, const string &, const bool)> ("read", this->_eid, sid, data, fin);
			}
		}
	}
	// Если соединение уничтожено приложением реентрантно из функции обратного вызова на собранные данные потока
	if(this->_connection == nullptr)
		// Выходим из метода
		return;
	// Если функция обратного вызова на освобождение буфера отправки потока установлена
	if(this->_callback.is("writable")){
		// Список потоков с освободившимся буфером отправки
		vector <uint64_t> streams;
		// Получаем список потоков, готовых принять данные (буфер опустился ниже нижней метки)
		this->_connection->drained(streams);
		/**
		 * Перебираем потоки с освободившимся буфером отправки: приложение, получившее
		 * частичный приём в send(), по этому сигналу возобновляет выдачу данных потока
		 */
		for(auto & sid : streams){
			// Если соединение уничтожено приложением реентрантно - прекращаем выдачу
			if(this->_connection == nullptr)
				// Выходим из метода
				return;
			// Выдаём приложению сигнал готовности потока принимать данные
			this->_callback.call <void (const event::id_t, const uint64_t)> ("writable", this->_eid, sid);
		}
	}
	// Если объединение установлено либо функция обратного вызова на принятую датаграмму приложения установлена
	if(spliced || this->_callback.is("datagram")){
		// Буфер принятой датаграммы приложения
		string datagram = "";
		/**
		 * Выдаём принятые датаграммы приложения: они доставляются вне потоков
		 * и порядка доставки не имеют (RFC 9221)
		 */
		while(this->_connection->datagram(datagram)){
			// Если для соединения установлено объединение данных - перенаправляем датаграмму в событие-приёмник
			if(spliced)
				// Перенаправляем принятую датаграмму в событие-приёмник объединения
				this->forward(datagram);
			// Иначе выдаём принятую датаграмму приложению
			else this->_callback.call <void (const event::id_t, const string &)> ("datagram", this->_eid, datagram);
			// Если соединение уничтожено приложением реентрантно из функции обратного вызова - прекращаем выдачу
			if(this->_connection == nullptr)
				// Выходим из метода
				return;
		}
	}
}
/**
 * @brief Метод оповещения приложения о завершённом соединении
 *
 */
void awh::unit::QuicClient::complete() noexcept {
	// Если соединение не создано либо ещё не завершено
	if((this->_connection == nullptr) || (this->_connection->state() != quic::connection_t::state_t::DRAINING))
		// Выходим из метода
		return;
	/**
	 * Оповещение выполняется однократно и независимо от того, было ли соединение
	 * установлено: сорванное рукопожатие приложению требуется отличать от
	 * успешно завершённого обмена, а без оповещения попытка подключения
	 * заканчивается молча и причина разрыва снаружи неразличима
	 */
	if(!this->_notified){
		// Устанавливаем флаг выполненного оповещения о завершённом соединении
		this->_notified = true;
		// Сбрасываем флаг оповещения приложения об установленном соединении
		this->_connected = false;
		// Выполняем функцию обратного вызова о завершённом соединении
		this->_callback.call <void (const event::id_t, const quic::error_t)> ("close", this->_eid, this->_connection->error());
	}
}
/**
 * @brief Метод перенаправления собранных данных соединения в событие-приёмник объединения
 *
 * @param data данные для перенаправления
 *
 */
void awh::unit::QuicClient::forward(string_view data) noexcept {
	// Если данные для перенаправления присутствуют и приёмник задан
	if(!data.empty() && (this->_dest != 0))
		/**
		 * Перенаправляем собранные данные соединения в событие-приёмник через сетевой
		 * движок: если приёмник шифрует данные на уровне соединения (QUIC-сессия) - они
		 * отправляются его туннельным потоком через функцию инъекции, иначе (TCP/UDP) -
		 * пишутся сырыми байтами в сокет
		 */
		engine::io::relay(this->_dest, data.data(), data.size());
}
/**
 * @brief Метод применения маркировки соединения к сокету события клиента
 *
 * @param marking требуемая маркировка исходящих датаграмм
 *
 */
void awh::unit::QuicClient::mark(const event::ecn_t marking) noexcept {
	// Если уведомление о перегрузке пути отключено либо маркировка уже установлена
	if(!this->_ecn || (marking == this->_marking))
		// Выходим из метода - смена маркировки не требуется
		return;
	// Если смена маркировки исходящих датаграмм не выполнена
	if(!engine::qos::ecn::set(this->_eid, this->_family, marking)){
		/**
		 * Если включён режим отладки
		 */
		#if defined(DEBUG_MODE)
			// Записываем предупреждение в лог
			log::debug("QUIC outgoing datagrams marking is not applied", __PRETTY_FUNCTION__, {static_cast <uint16_t> (marking)}, log::flag_t::WARNING);
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем предупреждение в лог
			log::print("QUIC outgoing datagrams marking is not applied", log::flag_t::WARNING);
		#endif
		// Выходим из метода - установленная на сокете маркировка неизвестна
		return;
	}
	// Запоминаем установленную на сокете маркировку
	this->_marking = marking;
}
/**
 * @brief Метод проверки актуальности события клиента
 *
 * @param eid идентификатор события
 * @return    результат проверки актуальности события
 *
 */
bool awh::unit::QuicClient::isActual(const event::id_t eid) const noexcept {
	// Событие клиента актуально, если оно создано и совпадает с идентификатором события клиента
	return ((this->_eid != 0) && (eid == this->_eid));
}
/**
 * @brief Метод запуска/остановки работы клиента
 *
 * @param status статус запуска/остановки клиента
 *
 */
void awh::unit::QuicClient::launch(const event::status_t status) noexcept {
	/**
	 * Определяем статус работы клиента
	 */
	switch(static_cast <uint8_t> (status)){
		// Если работа клиента запущена
		case static_cast <uint8_t> (event::status_t::LAUNCHED):
			// Выполняем функцию обратного вызова
			this->_callback.call <void (const event::status_t)> ("client_status", status);
		break;
		// Если работа клиента подлежит уничтожению
		case static_cast <uint8_t> (event::status_t::DESTROYED): {
			// Освобождаем объект соединения QUIC
			this->_connection.reset(nullptr);
			// Если событие интервала таймеров соединения создано
			if(this->_tid != 0){
				// Уничтожаем событие интервала таймеров соединения
				engine::io::destroy(this->_tid);
				// Сбрасываем идентификатор события интервала таймеров соединения
				this->_tid = 0;
			}
			// Если событие клиента создано
			if(this->_eid != 0){
				// Снимаем функцию обратного вызова на чтение датаграмм соединения
				engine::io::on(this->_eid, static_cast <engine::callback::read_t> (nullptr));
				// Снимаем функцию обратного вызова источника исходящих датаграмм
				engine::io::on(this->_eid, static_cast <engine::callback::source_t> (nullptr));
				// Снимаем функцию обратного вызова на завершение подключения к серверу
				engine::io::on(this->_eid, static_cast <engine::callback::connect_t> (nullptr));
				// Уничтожаем событие клиента
				engine::io::destroy(this->_eid);
				// Сбрасываем идентификатор события клиента
				this->_eid = 0;
			}
			// Выполняем получение идентификатора функции обратного вызова
			const callback_t::id_t fid = this->_callback.id("client_status");
			// Если функция обратного вызова установлена
			if(this->_callback.is(fid)){
				// Выполняем функцию обратного вызова
				this->_callback.call <void (const event::status_t)> (fid, status);
				// Выполняем получение функции обратного вызова
				this->_callback.set(fid, this->_callback.id("status"), this->_callback);
			}
		} break;
	}
}
/**
 * @brief Метод обработки завершения подключения к серверу
 *
 * @param eid идентификатор события клиента
 * @param ok  результат подключения к серверу
 *
 */
void awh::unit::QuicClient::connected([[maybe_unused]] const event::id_t eid, const bool ok) noexcept {
	// Если подключение к серверу не выполнено либо соединение не создано
	if(!ok || (this->_connection == nullptr))
		// Выходим из метода
		return;
	/**
	 * Начинаем соединение: до завершения подключения событие к отправке не
	 * готово, поэтому хендшейк запускается именно здесь
	 */
	if(this->_connection->connect() != quic::status_t::OK){
		/**
		 * Если включён режим отладки
		 */
		#if defined(DEBUG_MODE)
			// Записываем ошибку в лог
			log::debug("QUIC connection is not started", __PRETTY_FUNCTION__, {eid, ok}, log::flag_t::CRITICAL);
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			log::print("QUIC connection is not started", log::flag_t::CRITICAL);
		#endif
		// Выходим из метода
		return;
	}
	/**
	 * Оповещаем приложение о готовности к отправке ранних данных: ключи их защиты
	 * выданы возобновлённой сессией, а лимиты взяты из прошлого соединения, так
	 * что поставленное сейчас содержимое уйдёт вместе с первым пакетом хендшейка,
	 * не дожидаясь его завершения (RFC 9001 §4.6)
	 */
	if(this->_connection->writable())
		// Выполняем функцию обратного вызова о готовности к отправке ранних данных
		this->_callback.call <void (const event::id_t)> ("early", this->_eid);
	// Отправляем первый пакет хендшейка серверу
	this->flush();
}
/**
 * @brief Метод обработки просроченных таймеров соединения
 *
 * @param eid    идентификатор события интервала
 * @param status статус события интервала
 *
 */
void awh::unit::QuicClient::tick([[maybe_unused]] const event::id_t eid, const event::status_t status) noexcept {
	// Если статус события интервала не успешен либо соединение не создано
	if((status != event::status_t::SUCCESS) || (this->_connection == nullptr))
		// Выходим из метода
		return;
	// Текущее время в миллисекундах
	const uint64_t date = fmk::milliseconds();
	// Дедлайн ближайшего события таймера соединения
	const uint64_t timeout = this->_connection->timeout();
	// Если дедлайн таймера соединения наступил
	if((timeout > 0) && (date >= timeout)){
		// Выполняем обработку просроченных таймеров соединения
		this->_connection->tick(date);
		// Отправляем все готовые исходящие датаграммы
		this->flush();
		/**
		 * Выполняем оповещение приложения о завершённом соединении: завершение
		 * наступает и по таймеру - истечением периода завершения либо таймаута
		 * простоя, когда приходить от удалённого эндпоинта уже нечему
		 */
		this->complete();
	}
}
/**
 * @brief Метод отправки объединённых данных в туннельный поток соединения
 *
 * @param data данные для отправки в туннельный поток
 * @param size размер данных для отправки
 * @return     результат постановки данных в очередь отправки
 *
 */
bool awh::unit::QuicClient::inject(const uint8_t * data, const size_t size) noexcept {
	// Если данные для отправки отсутствуют либо соединение не создано
	if((data == nullptr) || (size == 0) || (this->_connection == nullptr))
		// Возвращаем значение по умолчанию
		return false;
	// Если туннельный поток входящих объединённых данных ещё не открыт
	if(this->_tunnel == quic::connection_t::INVALID_STREAM)
		// Открываем двунаправленный туннельный поток приложения (RFC 9000 §2.1)
		this->_tunnel = this->_connection->open(false);
	// Если туннельный поток открыть не удалось
	if(this->_tunnel == quic::connection_t::INVALID_STREAM)
		// Возвращаем значение по умолчанию
		return false;
	// Если постановка байт в очередь отправки туннельного потока не выполнена
	if(this->_connection->send(this->_tunnel, string_view(reinterpret_cast <const char *> (data), size), false) < size)
		// Возвращаем значение по умолчанию
		return false;
	// Отправляем готовые исходящие датаграммы удалённому серверу
	this->flush();
	// Возвращаем положительный результат
	return true;
}
/**
 * @brief Метод обработки принятой датаграммы соединения
 *
 * @param eid  идентификатор события клиента
 * @param data данные датаграммы
 * @param size размер датаграммы
 *
 */
void awh::unit::QuicClient::read([[maybe_unused]] const event::id_t eid, const uint8_t * data, const size_t size) noexcept {
	// Если соединение не создано
	if(this->_connection == nullptr)
		// Выходим из метода
		return;
	// Если уведомление о перегрузке пути включено
	if(this->_ecn){
		/**
		 * Извлекаем метаданные принятой датаграммы с события клиента: маркировка
		 * снимается с заголовка IP-пакета при приёме
		 */
		const net::dgram_info_t info = engine::io::trafficInfo(this->_eid);
		// Выполняем обработку входящей датаграммы с маркировкой перегрузки
		this->_connection->read(data, size, fmk::milliseconds(), info.congestion);
	// Выполняем обработку входящей датаграммы
	} else this->_connection->read(data, size, fmk::milliseconds());
	// Если соединение установлено и приложение об этом ещё не оповещено
	if(!this->_connected && (this->_connection->state() == quic::connection_t::state_t::CONNECTED)){
		// Устанавливаем флаг оповещения приложения об установленном соединении
		this->_connected = true;
		// Выполняем функцию обратного вызова об установленном соединении
		this->_callback.call <void (const event::id_t)> ("open", this->_eid);
	}
	// Выполняем выдачу собранных данных потоков приложения
	this->process();
	// Отправляем все готовые исходящие датаграммы
	this->flush();
	/**
	 * @brief Сохраняем присланный сервером токен проверки адреса: он приходит фреймом
	 *        NEW_TOKEN уже после установления соединения, поэтому проверяется на каждой
	 *        датаграмме, пока не будет получен
     *
     * @details Билет возобновления сессии соединение сохраняет самостоятельно в кэш кодера
	 *          по ключу сервера (RFC 9001 §4.6), поэтому шаттлинг билета юнитом не требуется.
     *
	 */
	if((this->_connection != nullptr) && this->_token.empty())
		// Сохраняем токен проверки адреса для следующего подключения
		this->_token = this->_connection->token();
	// Выполняем оповещение приложения о завершённом соединении
	this->complete();
}
/**
 * @brief Метод выдачи исходящей датаграммы соединения сетевому движку
 *
 * @param eid    идентификатор события клиента
 * @param buffer адрес указателя на буфер выдаваемых данных
 * @param size   ёмкость запроса на входе и размер выданных данных на выходе
 * @return       признак продолжения вытягивания данных
 *
 */
bool awh::unit::QuicClient::source([[maybe_unused]] const event::id_t eid, const uint8_t ** buffer, size_t & size) noexcept {
	// Если соединение уничтожено приложением реентрантно из функции обратного вызова
	if(this->_connection == nullptr){
		// Выданных данных нет
		size = 0;
		// Выводим отрицательный результат - источник исчерпан
		return false;
	}
	// Если предыдущая датаграмма движком уже принята
	if(this->_handed){
		// Освобождаем буфер под следующую датаграмму
		this->_datagram.clear();
		// Снимаем флаг выдачи буфера сетевому движку
		this->_handed = false;
	}
	// Если удерживаемой датаграммы нет
	if(this->_datagram.empty()){
		// Если исходящих датаграмм у соединения больше нет
		if(!this->_connection->write(this->_datagram, fmk::milliseconds())){
			// Выданных данных нет
			size = 0;
			// Выводим отрицательный результат - источник исчерпан
			return false;
		}
	}
	/**
	 * Если выданной ёмкости не хватает под целую датаграмму: датаграмма делению не
	 * подлежит, поэтому удерживаем её до следующего запроса движка, а вытягивание
	 * при этом не прекращаем (RFC 9000 §12.2)
	 */
	if(this->_datagram.size() > size){
		// Выданных данных нет
		size = 0;
		// Выводим положительный результат - вытягивание продолжается
		return true;
	}
	/**
	 * Применяем маркировку соединения к сокету: путь мог проверку поддержки
	 * ECN не пройти, и маркировать датаграммы далее нельзя
	 */
	this->mark(this->_connection->marking());
	// Выдаём буфер исходящей датаграммы сетевому движку
	(* buffer) = reinterpret_cast <const uint8_t *> (this->_datagram.data());
	// Выдаём размер исходящей датаграммы
	size = this->_datagram.size();
	// Поднимаем флаг выдачи буфера сетевому движку
	this->_handed = true;
	// Выводим положительный результат - вытягивание продолжается
	return true;
}
/**
 * @brief Метод проверки принятия ранних данных удалённым сервером (RFC 9001 §4.6.2)
 *
 * @return результат проверки
 *
 */
bool awh::unit::QuicClient::early() const noexcept {
	// Если соединение не создано
	if(this->_connection == nullptr)
		// Выводим отрицательный результат
		return false;
	// Выводим результат принятия ранних данных соединением
	return this->_connection->early();
}
/**
 * @brief Метод установки уведомления о перегрузке пути (RFC 9000 §13.4)
 *
 * @param mode режим уведомления о перегрузке пути
 *
 */
void awh::unit::QuicClient::ecn(const bool mode) noexcept {
	// Устанавливаем режим уведомления о перегрузке пути
	this->_ecn = mode;
}
/**
 * @brief Метод извлечения сохранённого токена проверки адреса (RFC 9000 §8.1.3)
 *
 * @return токен проверки адреса (пусто - токен не получен)
 *
 */
const string & awh::unit::QuicClient::token() const noexcept {
	// Выводим сохранённый токен проверки адреса
	return this->_token;
}
/**
 * @brief Метод установки сохранённого токена проверки адреса (RFC 9000 §8.1.3)
 *
 * @param token токен проверки адреса
 *
 */
void awh::unit::QuicClient::token(string_view token) noexcept {
	// Устанавливаем сохранённый токен проверки адреса
	this->_token.assign(token.begin(), token.end());
}
/**
 * @brief Метод фиксации настроек события клиента
 *
 * @param eid идентификатор события клиента
 * @return    результат выполнения фиксации
 *
 */
bool awh::unit::QuicClient::commit(const event::id_t eid) noexcept {
	// Если событие клиента является актуальным
	if(this->isActual(eid))
		// Выполняем фиксацию настроек события клиента и события интервала таймеров соединения
		return (engine::io::commit(this->_eid) && engine::io::commit(this->_tid));
	// Возвращаем значение по умолчанию
	return false;
}
/**
 * @brief Метод запуска работы события клиента
 *
 * @param eid идентификатор события клиента
 * @return    результат выполнения запуска
 *
 */
bool awh::unit::QuicClient::launch(const event::id_t eid) noexcept {
	// Если событие клиента является актуальным
	if(this->isActual(eid))
		// Выполняем запуск события клиента и события интервала таймеров соединения
		return (engine::io::launch(this->_eid) && engine::io::launch(this->_tid));
	// Возвращаем значение по умолчанию
	return false;
}
/**
 * @brief Метод уничтожения события клиента
 *
 * @param eid идентификатор события для уничтожения
 *
 */
void awh::unit::QuicClient::destroy(const event::id_t eid) noexcept {
	// Если событие клиента не актуально
	if(!this->isActual(eid))
		// Выходим из метода
		return;
	// Освобождаем объект соединения QUIC
	this->_connection.reset(nullptr);
	// Сбрасываем состояние сплайса: туннельный поток и цель форвардинга недействительны после уничтожения события
	this->_tunnel = quic::connection_t::INVALID_STREAM;
	// Сбрасываем цель форвардинга сплайса
	this->_dest = 0;
	// Если событие интервала таймеров соединения создано
	if(this->_tid != 0){
		// Уничтожаем событие интервала таймеров соединения
		engine::io::destroy(this->_tid);
		// Сбрасываем идентификатор события интервала таймеров соединения
		this->_tid = 0;
	}
	// Снимаем функцию обратного вызова на чтение датаграмм соединения
	engine::io::on(this->_eid, static_cast <engine::callback::read_t> (nullptr));
	// Снимаем функцию обратного вызова источника исходящих датаграмм
	engine::io::on(this->_eid, static_cast <engine::callback::source_t> (nullptr));
	// Снимаем функцию обратного вызова на завершение подключения к серверу
	engine::io::on(this->_eid, static_cast <engine::callback::connect_t> (nullptr));
	// Уничтожаем событие клиента
	engine::io::destroy(this->_eid);
	// Сбрасываем идентификатор события клиента
	this->_eid = 0;
}
/**
 * @brief Метод приостановки работы события клиента
 *
 * @param eid идентификатор события клиента
 * @return    результат выполнения приостановки работы
 *
 */
bool awh::unit::QuicClient::pause(const event::id_t eid) noexcept {
	// Если событие клиента является актуальным
	if(this->isActual(eid))
		// Выполняем приостановку работы события клиента
		return engine::io::pause(this->_eid);
	// Возвращаем значение по умолчанию
	return false;
}
/**
 * @brief Метод возобновления работы события клиента
 *
 * @param eid идентификатор события клиента
 * @return    результат выполнения возобновления работы
 *
 */
bool awh::unit::QuicClient::resume(const event::id_t eid) noexcept {
	// Если событие клиента является актуальным
	if(this->isActual(eid))
		// Выполняем возобновление работы события клиента
		return engine::io::resume(this->_eid);
	// Возвращаем значение по умолчанию
	return false;
}
/**
 * @brief Метод отключения клиента от удалённого сервера
 *
 * @param eid идентификатор события клиента
 * @return    результат выполнения отключения
 *
 */
bool awh::unit::QuicClient::disconnect(const event::id_t eid) noexcept {
	// Если событие клиента является актуальным
	if(this->isActual(eid))
		// Выполняем отключение клиента от удалённого сервера
		return engine::io::disconnect(this->_eid);
	// Возвращаем значение по умолчанию
	return false;
}
/**
 * @brief Метод подключения клиента к удалённому серверу
 *
 * @param eid идентификатор события клиента
 * @return    результат выполнения подключения
 *
 */
bool awh::unit::QuicClient::connect(const event::id_t eid) noexcept {
	// Если событие клиента не актуально
	if(!this->isActual(eid))
		// Выводим отрицательный результат
		return false;
	// Если шаблон контекста безопасности не установлен
	if(this->_ctx == 0){
		/**
		 * Если включён режим отладки
		 */
		#if defined(DEBUG_MODE)
			// Записываем ошибку в лог
			log::debug("QUIC security context is not set", __PRETTY_FUNCTION__, {eid}, log::flag_t::CRITICAL);
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			log::print("QUIC security context is not set", log::flag_t::CRITICAL);
		#endif
		// Выводим отрицательный результат
		return false;
	}
	// Сбрасываем флаг выполненного оповещения о завершённом соединении
	this->_notified = false;
	/**
	 * Сбрасываем идентификатор туннельного потока сплайса: нумерация потоков
	 * ведётся в пределах соединения, поэтому на новом соединении прежний поток
	 * недействителен и должен быть открыт заново при следующей передаче (см. inject())
	 */
	this->_tunnel = quic::connection_t::INVALID_STREAM;
	// Создаём соединение QUIC на шаблоне контекста безопасности
	this->_connection = make_unique <quic::connection_t> (
		quic::endpoint_t::CLIENT, this->_ctx
	);
	// Устанавливаем локальные транспортные параметры соединения
	this->_connection->params(this->_params);
	// Устанавливаем адрес удалённого эндпоинта соединению из ранее установленной структуры адреса и порта
	this->_connection->address(this->_target.get(), this->_targetPort);
	// Устанавливаем режим маркировки исходящих датаграмм соединения
	this->_connection->ecn(this->_ecn);
	/**
     * @brief Билет возобновления сессии соединение подставляет самостоятельно из кэша
	 *        кодера по ключу сервера при начале хендшейка (RFC 9001 §4.6)
     *
	 * @details Подставляем сохранённый токен проверки адреса: предъявление токена в первом
	 *          пакете подтверждает адрес клиента сразу, и обмен пакетом Retry не выполняется
	 *          (RFC 9000 §8.1.3).
     *
	 */
	if(!this->_token.empty())
		// Устанавливаем сохранённый токен проверки адреса соединению
		this->_connection->token(this->_token);
	// Выполняем подключение к удалённому серверу
	return engine::io::connect(this->_eid);
}
/**
 * @brief Метод подключения клиента к удалённому серверу
 *
 * @param ids список идентификаторов событий для подключения
 * @return    результат выполнения подключения
 *
 */
bool awh::unit::QuicClient::connect(const vector <event::id_t> & ids) noexcept {
	// Для транспорта QUIC поддерживается единственное соединение на событие клиента
	if((ids.size() == 1) && this->isActual(ids.front()))
		// Выполняем подключение к удалённому серверу
		return this->connect(ids.front());
	// Возвращаем значение по умолчанию
	return false;
}
/**
 * @brief Метод установки шаблона контекста безопасности соединения
 *
 * @param ctx идентификатор шаблона контекста безопасности
 *
 */
void awh::unit::QuicClient::context(const tls::coder::id_t ctx) noexcept {
	// Устанавливаем идентификатор шаблона контекста безопасности
	this->_ctx = ctx;
}
/**
 * @brief Метод установки локальных транспортных параметров соединения (RFC 9000 §7.4)
 *
 * @param params локальные транспортные параметры
 *
 */
void awh::unit::QuicClient::params(const quic::params::params_t & params) noexcept {
	// Устанавливаем локальные транспортные параметры соединения
	this->_params = params;
}
/**
 * @brief Метод получения сетевого интерфейса клиента
 *
 * @param eid идентификатор события клиента
 * @return    сетевой интерфейс клиента
 *
 */
string awh::unit::QuicClient::getIface(const event::id_t eid) const noexcept {
	// Если событие клиента является актуальным
	if(this->isActual(eid))
		// Выполняем получение сетевого интерфейса для события клиента
		return engine::iface::get(this->_eid);
	// Возвращаем значение по умолчанию
	return "";
}
/**
 * @brief Метод установки сетевого интерфейса клиента
 *
 * @param eid  идентификатор события клиента
 * @param name имя сетевого интерфейса для установки
 * @return     результат выполнения установки
 *
 */
bool awh::unit::QuicClient::setIface(const event::id_t eid, string_view name) noexcept {
	// Если событие клиента является актуальным
	if(this->isActual(eid))
		// Выполняем установку сетевого интерфейса для события клиента
		return engine::iface::set(this->_eid, name);
	// Возвращаем значение по умолчанию
	return false;
}
/**
 * @brief Метод получения внутреннего порта события
 *
 * @param eid идентификатор события
 * @return    внутренний порт события
 *
 */
uint16_t awh::unit::QuicClient::getSourcePort(const event::id_t eid) const noexcept {
	// Если событие клиента является актуальным
	if(this->isActual(eid))
		// Выполняем получение внутреннего порта для события клиента
		return engine::source::port::get(this->_eid);
	// Возвращаем значение по умолчанию
	return 0;
}
/**
 * @brief Метод установки внутреннего порта события
 *
 * @param eid  идентификатор события
 * @param port внутренний порт события
 * @return     результат выполнения установки
 *
 */
bool awh::unit::QuicClient::setSourcePort(const event::id_t eid, const uint16_t port) noexcept {
	// Если событие клиента является актуальным
	if(this->isActual(eid))
		// Выполняем установку внутреннего порта для события клиента
		return engine::source::port::set(this->_eid, port);
	// Возвращаем значение по умолчанию
	return false;
}
/**
 * @brief Метод получения порта удалённого сервера
 *
 * @param eid идентификатор события клиента
 * @return    порт удалённого сервера
 *
 */
uint16_t awh::unit::QuicClient::getTargetPort(const event::id_t eid) const noexcept {
	// Если событие клиента является актуальным
	if(this->isActual(eid))
		// Выполняем получение порта удалённого сервера для события клиента
		return engine::target::port::get(this->_eid);
	// Возвращаем значение по умолчанию
	return 0;
}
/**
 * @brief Метод установки порта удалённого сервера
 *
 * @param eid  идентификатор события клиента
 * @param port порт удалённого сервера для установки
 * @return     результат выполнения установки
 *
 */
bool awh::unit::QuicClient::setTargetPort(const event::id_t eid, const uint16_t port) noexcept {
	// Если событие клиента не актуально
	if(!this->isActual(eid))
		// Возвращаем значение по умолчанию
		return false;
	// Запоминаем порт удалённого сервера для установки адреса эндпоинта соединению
	this->_targetPort = port;
	// Выполняем установку порта удалённого сервера для события клиента
	return engine::target::port::set(this->_eid, port);
}
/**
 * @brief Метод получения адреса хоста целевой машины
 *
 * @param eid идентификатор события клиента
 * @return    адрес хоста целевой машины
 *
 */
string awh::unit::QuicClient::getTarget(const event::id_t eid) const noexcept {
	// Если событие клиента является актуальным
	if(this->isActual(eid))
		// Выполняем получение адреса хоста целевой машины для события клиента
		return engine::target::get(this->_eid);
	// Возвращаем значение по умолчанию
	return "";
}
/**
 * @brief Метод установки адреса хоста целевой машины
 *
 * @param eid    идентификатор события клиента
 * @param target адрес хоста целевой машины
 * @return       результат выполнения установки
 *
 */
bool awh::unit::QuicClient::setTarget(const event::id_t eid, string_view target) noexcept {
	// Если событие клиента не актуально
	if(!this->isActual(eid))
		// Возвращаем значение по умолчанию
		return false;
	// Разбираем адрес удалённого сервера в штатную структуру сетевого адреса фреймворка
	net_addr_t resolver;
	// Если разбор адреса удалённого сервера выполнен успешно
	if(resolver.parse(target))
		// Запоминаем структуру сетевого адреса удалённого сервера для установки адреса эндпоинта соединению
		this->_target = resolver.source();
	// Выполняем установку адреса хоста целевой машины для события клиента
	return engine::target::set(this->_eid, target);
}
/**
 * @brief Метод установки адреса хоста целевой машины
 *
 * @param eid    идентификатор события клиента
 * @param target структура сетевого адреса хоста целевой машины
 * @return       результат выполнения установки
 *
 */
bool awh::unit::QuicClient::setTarget(const event::id_t eid, const net::addr_t * target) noexcept {
	// Если событие клиента не актуально
	if(!this->isActual(eid))
		// Возвращаем значение по умолчанию
		return false;
	// Запоминаем структуру сетевого адреса удалённого сервера для установки адреса эндпоинта соединению
	if(target != nullptr){
		/**
		 * Выполняем глубокое копирование штатной структуры сетевого адреса
		 */
		switch(target->size){
			// Для сетевого адреса IPv4
			case 4: {
				// Выделяем память под структуру адреса IPv4
				auto addr = make_unique <net::addr_net_ipv4_t> ();
				// Копируем IPv4-адрес в чистом виде
				addr->address = static_cast <const net::addr_net_ipv4_t *> (target)->address;
				// Запоминаем структуру сетевого адреса удалённого сервера
				this->_target = ::move(addr);
			} break;
			// Для сетевого адреса IPv6
			case 16: {
				// Выделяем память под структуру адреса IPv6
				auto addr = make_unique <net::addr_net_ipv6_t> ();
				// Копируем IPv6-адрес в чистом виде
				addr->address = static_cast <const net::addr_net_ipv6_t *> (target)->address;
				// Запоминаем структуру сетевого адреса удалённого сервера
				this->_target = ::move(addr);
			} break;
		}
	}
	// Выполняем установку адреса хоста целевой машины для события клиента
	return engine::target::set(this->_eid, target);
}
/**
 * @brief Метод получения адреса хоста целевой машины
 *
 * @param eid    идентификатор события клиента
 * @param target объект для извлечения адреса хоста целевой машины
 * @return       результат выполнения извлечения адреса хоста целевой машины
 *
 */
bool awh::unit::QuicClient::getTarget(const event::id_t eid, unique_ptr <net::addr_t> & target) const noexcept {
	// Если событие клиента является актуальным
	if(this->isActual(eid))
		// Выполняем получение адреса хоста целевой машины для события клиента
		return engine::target::get(this->_eid, target);
	// Возвращаем значение по умолчанию
	return false;
}
/**
 * @brief Метод объединения потоков данных между двумя событиями
 *
 * @param eid  идентификатор события-источника
 * @param dest идентификатор события-приёмника
 * @return     результат объединения
 *
 */
bool awh::unit::QuicClient::splice(const event::id_t eid, const event::id_t dest) noexcept {
	/**
	 * @brief Если событие клиента является источником объединения данных
	 *
	 * @details Объединение данных QUIC ведётся на уровне сессии, а не сокета: собранные
	 *          (расшифрованные) данные потоков соединения перенаправляются в событие-приёмник,
	 *          а входящие в соединение объединённые байты отправляются туннельным потоком
	 *          с шифрованием на уровне соединения. Направление задаётся отдельным вызовом
	 *          на каждую сторону (как для прочих транспортов).
     *
	 */
	if(this->isActual(eid)){
		// Устанавливаем событие-приёмник объединения данных соединения
		this->_dest = dest;
		/**
		 * Сразу выдаём уже собранные данные потоков, накопленные до установки
		 * приёмника: без нового входящего события метод process() иначе не будет
		 * вызван и буферизованные данные не пойдут в событие-приёмник
		 */
		this->process();
		// Возвращаем положительный результат
		return true;
	// Если событие клиента является приёмником объединения данных
	} else if(this->isActual(dest))
		/**
		 * Входящее направление (событие-источник -> клиент) обслуживается инъекцией
		 * байт в туннельный поток соединения через сетевой движок: привязываем
		 * внешнее событие-источник к клиенту на уровне движка, тогда входящие в
		 * источник байты будут инъецированы в туннельный поток соединения, минуя
		 * выдачу приложению (см. inject). Так обеспечивается симметрия с сервером
		 */
		return engine::io::splice(eid, dest);
	// Возвращаем значение по умолчанию
	return false;
}
/**
 * @brief Метод получения метаданных последнего принятого дейтаграммного пакета
 *
 * @param eid идентификатор события клиента
 * @return    метаданные последнего принятого дейтаграммного пакета
 *
 */
awh::net::dgram_info_t awh::unit::QuicClient::getTrafficInfo(const event::id_t eid) const noexcept {
	// Если событие клиента является актуальным
	if(this->isActual(eid))
		// Выполняем получение метаданных последнего принятого дейтаграммного пакета для события клиента
		return engine::io::trafficInfo(this->_eid);
	// Возвращаем значение по умолчанию
	return net::dgram_info_t();
}
/**
 * @brief Метод получения опций события клиента
 *
 * @param eid идентификатор события клиента
 * @return    опции события клиента
 *
 */
uint16_t awh::unit::QuicClient::getOptions(const event::id_t eid) const noexcept {
	// Если событие клиента является актуальным
	if(this->isActual(eid))
		// Выполняем получение опций для события клиента
		return engine::options::get(this->_eid);
	// Возвращаем значение по умолчанию
	return 0;
}
/**
 * @brief Метод установки опций события клиента
 *
 * @param eid     идентификатор события клиента
 * @param options опции события клиента для установки
 * @return        результат выполнения установки
 *
 */
bool awh::unit::QuicClient::setOptions(const event::id_t eid, const uint16_t options) noexcept {
	// Если событие клиента является актуальным
	if(this->isActual(eid))
		// Выполняем установку опций для события клиента
		return engine::options::set(this->_eid, options);
	// Возвращаем значение по умолчанию
	return false;
}
/**
 * @brief Метод установки опции события клиента
 *
 * @param eid    идентификатор события клиента
 * @param option опция события клиента для установки
 * @param mode   режим установки опции события клиента
 * @return       результат выполнения установки
 *
 */
bool awh::unit::QuicClient::setOption(const event::id_t eid, const uint16_t option, const bool mode) noexcept {
	// Если событие клиента является актуальным
	if(this->isActual(eid))
		// Выполняем установку опции для события клиента
		return engine::options::set(this->_eid, option, mode);
	// Возвращаем значение по умолчанию
	return false;
}
/**
 * @brief Метод получения максимального количества хопов, через которые может пройти пакет
 *
 * @param eid идентификатор события клиента
 * @return    максимальное количество хопов
 *
 */
awh::event::hops_t awh::unit::QuicClient::getHops(const event::id_t eid) const noexcept {
	// Если событие клиента является актуальным
	if(this->isActual(eid))
		// Выполняем получение максимального количества хопов для события клиента
		return engine::hops::get(this->_eid);
	// Возвращаем значение по умолчанию
	return event::hops_t::LOOPBACK;
}
/**
 * @brief Метод установки максимального количества хопов, через которые может пройти пакет
 *
 * @param eid  идентификатор события клиента
 * @param hops максимальное количество хопов
 * @return     результат работы функции
 *
 */
bool awh::unit::QuicClient::setHops(const event::id_t eid, const event::hops_t hops) noexcept {
	// Если событие клиента является актуальным
	if(this->isActual(eid))
		// Выполняем установку максимального количества хопов для события клиента
		return engine::hops::set(this->_eid, hops);
	// Возвращаем значение по умолчанию
	return false;
}
/**
 * @brief Метод получения количества хопов последнего принятого пакета
 *
 * @param eid идентификатор события клиента
 * @return    количество хопов последнего принятого пакета
 *
 */
uint8_t awh::unit::QuicClient::getCountHops(const event::id_t eid) const noexcept {
	// Если событие клиента является актуальным
	if(this->isActual(eid))
		// Выполняем получение количества хопов последнего принятого пакета для события клиента
		return engine::hops::count::get(this->_eid);
	// Возвращаем значение по умолчанию
	return 0;
}
/**
 * @brief Метод установки количества хопов последнего принятого пакета
 *
 * @param eid  идентификатор события клиента
 * @param hops количество хопов последнего принятого пакета
 * @return     результат выполнения установки
 *
 */
bool awh::unit::QuicClient::setCountHops(const event::id_t eid, const uint8_t hops) noexcept {
	// Если событие клиента является актуальным
	if(this->isActual(eid))
		// Выполняем установку количества хопов последнего принятого пакета для события клиента
		return engine::hops::count::set(this->_eid, hops);
	// Возвращаем значение по умолчанию
	return false;
}
/**
 * @brief Метод получения данных от удалённого сервера
 *
 * @param eid идентификатор события клиента
 * @return    результат получения данных
 *
 */
bool awh::unit::QuicClient::recv(const event::id_t eid) noexcept {
	// Если событие клиента является актуальным
	if(this->isActual(eid))
		// Выполняем получение данных от удалённого сервера
		return engine::io::recv(this->_eid);
	// Возвращаем значение по умолчанию
	return false;
}
/**
 * @brief Метод отправки данных в поток приложения соединения
 *
 * @param sid  идентификатор потока приложения
 * @param data отправляемые данные
 * @param fin  флаг завершения потока
 * @return     результат постановки данных в очередь отправки
 *
 */
size_t awh::unit::QuicClient::send(const uint64_t sid, string_view data, const bool fin) noexcept {
	// Если соединение не создано
	if(this->_connection == nullptr)
		// Ничего не принято в очередь отправки
		return 0;
	// Ставим данные в очередь отправки потока (частичный приём: движок возвращает принятый объём)
	const size_t accepted = this->_connection->send(sid, data, fin);
	// Если данные приняты либо требуется отправка завершения потока - отправляем готовые датаграммы
	if((accepted > 0) || fin)
		// Отправляем все готовые исходящие датаграммы
		this->flush();
	// Выводим число принятых в очередь отправки байт
	return accepted;
}
/**
 * @brief Метод отправки данных удалённому серверу
 *
 * @param eid    идентификатор события клиента
 * @param buffer буфер данных для отправки
 * @param size   размер данных для отправки
 * @return       количество байт, поставленных в очередь отправки
 *
 */
size_t awh::unit::QuicClient::send(const event::id_t eid, const void * buffer, const size_t size) noexcept {
	// Если событие не актуально либо соединение не создано либо данные отсутствуют
	if(!this->isActual(eid) || (this->_connection == nullptr) || (buffer == nullptr) || (size == 0))
		// Возвращаем значение по умолчанию
		return 0;
	// Открываем двунаправленный поток приложения по умолчанию (RFC 9000 §2.1)
	const uint64_t sid = this->_connection->open(false);
	// Если поток приложения не открыт
	if(sid == quic::connection_t::INVALID_STREAM)
		// Возвращаем значение по умолчанию
		return 0;
	// Если постановка данных в очередь отправки потока не выполнена
	if(this->_connection->send(sid, string_view(reinterpret_cast <const char *> (buffer), size), false) < size)
		// Возвращаем значение по умолчанию
		return 0;
	// Отправляем все готовые исходящие датаграммы
	this->flush();
	// Выводим количество поставленных в очередь отправки байт
	return size;
}
/**
 * @brief Метод установки водяных меток буфера отправки потоков соединения (backpressure)
 *
 * @param high верхняя водяная метка (ёмкость буфера отправки потока)
 * @param low  нижняя водяная метка (порог сигнала "writable")
 *
 */
void awh::unit::QuicClient::sendWaterMarks(const size_t high, const size_t low) noexcept {
	// Если соединение не создано
	if(this->_connection == nullptr)
		// Выходим из метода
		return;
	// Устанавливаем водяные метки буфера отправки потоков соединения
	this->_connection->sendWaterMarks(high, low);
}
/**
 * @brief Метод назначения pull-источника данных потока (RFC 9000 §2.2)
 *
 * @param sid    идентификатор потока приложения
 * @param source pull-источник данных тела потока
 *
 */
void awh::unit::QuicClient::dataSource(const uint64_t sid, quic::connection_t::data_source_callback_t source) noexcept {
	// Если соединение не создано
	if(this->_connection == nullptr)
		// Выходим из метода
		return;
	// Назначаем pull-источник данных тела потока
	this->_connection->dataSource(sid, source);
	// Отправляем готовые исходящие датаграммы: движок начнёт тянуть данные источника
	this->flush();
}
/**
 * @brief Метод получения адреса клиента
 *
 * @param eid     идентификатор события клиента
 * @param address тип адреса клиента
 * @return        значение адреса клиента
 *
 */
string awh::unit::QuicClient::getAddress(const event::id_t eid, const event::address_t address) const noexcept {
	// Если событие клиента является актуальным
	if(this->isActual(eid))
		// Выполняем получение адреса клиента для события клиента
		return engine::address::get(this->_eid, address);
	// Возвращаем значение по умолчанию
	return "";
}
/**
 * @brief Метод установки адреса клиента
 *
 * @param eid     идентификатор события клиента
 * @param address тип адреса клиента
 * @param value   значение адреса клиента
 * @return        результат выполнения установки
 *
 */
bool awh::unit::QuicClient::setAddress(const event::id_t eid, const event::address_t address, string_view value) noexcept {
	// Если событие клиента является актуальным
	if(this->isActual(eid))
		// Выполняем установку адреса клиента для события клиента
		return engine::address::set(this->_eid, address, value);
	// Возвращаем значение по умолчанию
	return false;
}
/**
 * @brief Метод установки адреса клиента
 *
 * @param eid     идентификатор события клиента
 * @param address тип адреса клиента
 * @param value   структура сетевого адреса клиента
 * @return        результат выполнения установки
 *
 */
bool awh::unit::QuicClient::setAddress(const event::id_t eid, const event::address_t address, const net::addr_t * value) noexcept {
	// Если событие клиента является актуальным
	if(this->isActual(eid))
		// Выполняем установку адреса клиента для события клиента
		return engine::address::set(this->_eid, address, value);
	// Возвращаем значение по умолчанию
	return false;
}
/**
 * @brief Метод получения адреса клиента
 *
 * @param eid     идентификатор события клиента
 * @param address тип адреса клиента
 * @param value   объект для извлечения адреса клиента
 * @return        результат выполнения извлечения адреса клиента
 *
 */
bool awh::unit::QuicClient::getAddress(const event::id_t eid, const event::address_t address, unique_ptr <net::addr_t> & value) const noexcept {
	// Если событие клиента является актуальным
	if(this->isActual(eid))
		// Выполняем получение адреса клиента для события клиента
		return engine::address::get(this->_eid, address, value);
	// Возвращаем значение по умолчанию
	return false;
}
/**
 * @brief Метод получения режима трансляции пакетов клиента
 *
 * @param eid идентификатор события клиента
 * @return    режим трансляции пакетов (unicast, multicast, broadcast)
 *
 */
awh::event::delivery_mode_t awh::unit::QuicClient::getDelivery(const event::id_t eid) const noexcept {
	// Если событие клиента является актуальным
	if(this->isActual(eid))
		// Выполняем получение режима трансляции пакетов клиента для события клиента
		return engine::delivery::get(this->_eid);
	// Возвращаем значение по умолчанию
	return event::delivery_mode_t::NONE;
}
/**
 * @brief Метод установки режима трансляции пакетов клиента
 *
 * @param eid      идентификатор события клиента
 * @param delivery режим трансляции пакетов (unicast, multicast, broadcast)
 * @return         результат выполнения установки
 *
 */
bool awh::unit::QuicClient::setDelivery(const event::id_t eid, const event::delivery_mode_t delivery) noexcept {
	// Если событие клиента является актуальным
	if(this->isActual(eid))
		// Выполняем установку режима трансляции пакетов клиента для события клиента
		return engine::delivery::set(this->_eid, delivery);
	// Возвращаем значение по умолчанию
	return false;
}
/**
 * @brief Метод получения размера буфера клиента
 *
 * @param eid    идентификатор события клиента
 * @param action тип действия клиента
 * @return       размер буфера клиента
 *
 */
size_t awh::unit::QuicClient::getBufferSize(const event::id_t eid, const event::action_t action) const noexcept {
	// Если событие клиента является актуальным
	if(this->isActual(eid))
		// Выполняем получение размера буфера клиента для события клиента
		return engine::buffer::size::get(this->_eid, action);
	// Возвращаем значение по умолчанию
	return 0;
}
/**
 * @brief Метод установки размера буфера клиента
 *
 * @param eid    идентификатор события клиента
 * @param action тип действия клиента
 * @param size   размер буфера клиента
 * @return       результат выполнения установки
 *
 */
bool awh::unit::QuicClient::setBufferSize(const event::id_t eid, const event::action_t action, const size_t size) noexcept {
	// Если событие клиента является актуальным
	if(this->isActual(eid))
		// Выполняем установку размера буфера клиента для события клиента
		return engine::buffer::size::set(this->_eid, action, size);
	// Возвращаем значение по умолчанию
	return false;
}
/**
 * @brief Метод получения режима использования таймаута на чтение события
 *
 * @param eid идентификатор события
 * @return    режим использования таймаута на чтение события
 *
 */
awh::event::usage_t awh::unit::QuicClient::getUsageReadTimeout(const event::id_t eid) const noexcept {
	// Если событие клиента является актуальным
	if(this->isActual(eid))
		// Выполняем получение режима использования таймаута на чтение события для события клиента
		return engine::timer::usage(this->_eid);
	// Возвращаем значение по умолчанию
	return event::usage_t::NONE;
}
/**
 * @brief Метод установки режима использования таймаута на чтение события
 *
 * @param eid   идентификатор события
 * @param usage режим использования таймаута на чтение события (reusable или disposable)
 *
 */
void awh::unit::QuicClient::setUsageReadTimeout(const event::id_t eid, const event::usage_t usage) noexcept {
	// Если событие клиента является актуальным
	if(this->isActual(eid))
		// Выполняем установку режима использования таймаута на чтение события для события клиента
		engine::timer::usage(this->_eid, usage);
}
/**
 * @brief Метод получения таймаута клиента
 *
 * @param eid    идентификатор события клиента
 * @param action тип действия клиента
 * @return       значение таймаута в миллисекундах
 *
 */
uint32_t awh::unit::QuicClient::getTimeout(const event::id_t eid, const event::action_t action) const noexcept {
	// Если событие клиента является актуальным
	if(this->isActual(eid))
		// Выполняем получение параметров таймаута для клиента
		return engine::timer::timeout(this->_eid, action);
	// Возвращаем значение по умолчанию
	return 0;
}
/**
 * @brief Метод установки таймаута клиента
 *
 * @param eid     идентификатор события клиента
 * @param action  тип действия клиента
 * @param timeout значение таймаута в миллисекундах
 *
 */
void awh::unit::QuicClient::setTimeout(const event::id_t eid, const event::action_t action, const uint32_t timeout) noexcept {
	// Если событие клиента является актуальным
	if(this->isActual(eid))
		// Выполняем установку параметров таймаута для клиента
		engine::timer::timeout(this->_eid, action, timeout);
}
/**
 * @brief Метод установки пропускной способности клиента
 *
 * @param eid       идентификатор события клиента
 * @param limiting  режим ограничения пропускной способности клиента (egress или ingress)
 * @param bandwidth пропускная способность клиента для установки
 * @return          результат выполнения установки
 *
 */
bool awh::unit::QuicClient::bandwidth(const event::id_t eid, const event::limiting_t limiting, string_view bandwidth) noexcept {
	// Если событие клиента является актуальным
	if(this->isActual(eid))
		// Выполняем установку параметров пропускной способности для клиента
		return engine::io::bandwidth(this->_eid, limiting, bandwidth);
	// Возвращаем значение по умолчанию
	return false;
}
/**
 * @brief Метод установки параметров keep-alive для клиента
 *
 * @param eid   идентификатор события клиента
 * @param cnt   количество пакетов keep-alive
 * @param idle  время простоя перед отправкой первого пакета keep-alive в секундах
 * @param intvl интервал между пакетами keep-alive в секундах
 * @return      результат выполнения установки
 *
 */
bool awh::unit::QuicClient::keepAlive(const event::id_t eid, const int32_t cnt, const int32_t idle, const int32_t intvl) noexcept {
	// Если событие клиента является актуальным
	if(this->isActual(eid))
		// Выполняем установку параметров keep-alive для клиента
		return engine::connection::keepalive(this->_eid, cnt, idle, intvl);
	// Возвращаем значение по умолчанию
	return false;
}
/**
 * @brief Метод получения MTU сетевого интерфейса
 *
 * @param eid идентификатор события клиента
 * @return    MTU сетевого интерфейса
 *
 */
uint16_t awh::unit::QuicClient::getMaximumTransmissionUnit(const event::id_t eid) const noexcept {
	// Если событие клиента является актуальным
	if(this->isActual(eid))
		// Выполняем получение MTU сетевого интерфейса для события клиента
		return engine::mtu::get(this->_eid);
	// Возвращаем значение по умолчанию
	return 0;
}
/**
 * @brief Метод установки MTU сетевого интерфейса
 *
 * @param eid идентификатор события клиента
 * @param mtu размер MTU интерфейса
 * @return    результат установки MTU сетевого интерфейса
 *
 */
bool awh::unit::QuicClient::setMaximumTransmissionUnit(const event::id_t eid, const uint32_t mtu) const noexcept {
	// Если событие клиента является актуальным
	if(this->isActual(eid))
		// Выполняем установку MTU сетевого интерфейса для события клиента
		return engine::mtu::set(this->_eid, mtu);
	// Возвращаем значение по умолчанию
	return false;
}
/**
 * @brief Метод получения режима обнаружения максимального размера пакета (MTU)
 *
 * @param eid идентификатор события клиента
 * @return    текущий режим обнаружения MTU
 *
 */
awh::event::mtu_discover_t awh::unit::QuicClient::getMaximumTransmissionUnitDiscover(const event::id_t eid) const noexcept {
	// Если событие клиента является актуальным
	if(this->isActual(eid))
		// Выполняем получение режима обнаружения MTU для события клиента
		return engine::mtu::discover::get(this->_eid, engine::io::family(this->_eid));
	// Возвращаем значение по умолчанию
	return event::mtu_discover_t::NONE;
}
/**
 * @brief Метод установки режима обнаружения максимального размера пакета (MTU)
 *
 * @param eid  идентификатор события клиента
 * @param mode режим обнаружения максимального размера пакета (MTU)
 * @return     результат работы функции
 *
 */
bool awh::unit::QuicClient::setMaximumTransmissionUnitDiscover(const event::id_t eid, const event::mtu_discover_t mode) const noexcept {
	// Если событие клиента является актуальным
	if(this->isActual(eid))
		// Выполняем установку режима обнаружения MTU для события клиента
		return engine::mtu::discover::set(this->_eid, engine::io::family(this->_eid), mode);
	// Возвращаем значение по умолчанию
	return false;
}
/**
 * @brief Метод получения значения поля Differentiated Services Code Point (DSCP) в заголовке IP-пакета
 *
 * @param eid идентификатор события клиента
 * @return    значение DSCP
 *
 */
awh::event::dscp_t awh::unit::QuicClient::getDifferentiatedServicesCodePoint(const event::id_t eid) const noexcept {
	// Если событие клиента является актуальным
	if(this->isActual(eid))
		// Выполняем получение значения поля Differentiated Services Code Point (DSCP) для клиента
		return engine::qos::dscp::get(this->_eid, engine::io::family(this->_eid));
	// Возвращаем значение по умолчанию
	return event::dscp_t::CS0;
}
/**
 * @brief Метод установки значения поля Differentiated Services Code Point (DSCP) в заголовке IP-пакета
 *
 * @param eid  идентификатор события клиента
 * @param dscp значение DSCP
 * @return     результат работы функции
 *
 */
bool awh::unit::QuicClient::setDifferentiatedServicesCodePoint(const event::id_t eid, const event::dscp_t dscp) const noexcept {
	// Если событие клиента является актуальным
	if(this->isActual(eid))
		// Выполняем установку значения поля Differentiated Services Code Point (DSCP) для клиента
		return engine::qos::dscp::set(this->_eid, engine::io::family(this->_eid), dscp);
	// Возвращаем значение по умолчанию
	return false;
}
/**
 * @brief Метод активации/деактивации мультикаст-группы
 *
 * @param eid    идентификатор события клиента
 * @param mode   режим активации/деактивации
 * @param group  мультикаст-группа для активации/деактивации
 * @param source адрес сетевого интерфейса с которого выполняется подписка
 * @param port   порт мультикаст-группы с которого выполняется подписка
 * @return       результат выполнения установки
 *
 */
bool awh::unit::QuicClient::membership(const event::id_t eid, const event::mode_t mode, string_view group, string_view source, const uint16_t port) noexcept {
	// Если событие клиента является актуальным
	if(this->isActual(eid))
		// Выполняем активацию/деактивацию мультикаст-группы для клиента
		return engine::multicast::membership(this->_eid, mode, group, source, port);
	// Возвращаем значение по умолчанию
	return false;
}
/**
 * @brief Метод активации/деактивации мультикаст-группы
 *
 * @param eid    идентификатор события клиента
 * @param mode   режим активации/деактивации
 * @param group  мультикаст-группа для активации/деактивации
 * @param source адрес сетевого интерфейса с которого выполняется подписка
 * @param port   порт мультикаст-группы с которого выполняется подписка
 * @return       результат выполнения установки
 *
 */
bool awh::unit::QuicClient::membership(const event::id_t eid, const event::mode_t mode, const net::addr_t * group, const net::addr_t * source, const uint16_t port) noexcept {
	// Если событие клиента является актуальным
	if(this->isActual(eid))
		// Выполняем активацию/деактивацию мультикаст-группы для клиента
		return engine::multicast::membership(this->_eid, mode, group, source, port);
	// Возвращаем значение по умолчанию
	return false;
}
/**
 * @brief Метод создания события клиента QUIC поверх UDP
 *
 * @param family   семейство адресов события клиента
 * @param type     тип события (игнорируется, приводится к DATAGRAM)
 * @param protocol протокол события (игнорируется, приводится к UDP)
 * @return         идентификатор созданного события клиента
 *
 */
awh::event::id_t awh::unit::QuicClient::issue(const event::family_t family, [[maybe_unused]] const event::type_t type, [[maybe_unused]] const event::protocol_t protocol) noexcept {
	// Если шаблон контекста безопасности не установлен
	if(this->_ctx == 0){
		/**
		 * Если включён режим отладки
		 */
		#if defined(DEBUG_MODE)
			// Записываем ошибку в лог
			log::debug("QUIC security context is not set", __PRETTY_FUNCTION__, {
				static_cast <uint16_t> (family),
				static_cast <uint16_t> (type),
				static_cast <uint16_t> (protocol)
			}, log::flag_t::CRITICAL);
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			log::print("QUIC security context is not set", log::flag_t::CRITICAL);
		#endif
		// Выводим отрицательный результат
		return 0;
	}
	// Если событие клиента уже создано
	if(this->_eid != 0)
		// Выводим идентификатор созданного ранее события клиента
		return this->_eid;
	// Добавляем событие клиента поверх UDP (транспорт QUIC работает поверх дейтаграммного UDP-сокета)
	this->_eid = engine::io::event(event::node_t::CLIENT, family, event::type_t::DATAGRAM, event::protocol_t::UDP);
	// Добавляем событие интервала таймеров соединения
	this->_tid = engine::io::event(event::node_t::INTERVAL, event::family_t::TIMER);
	// Если события не созданы
	if((this->_eid == 0) || (this->_tid == 0)){
		/**
		 * Если включён режим отладки
		 */
		#if defined(DEBUG_MODE)
			// Записываем ошибку в лог
			log::debug("QUIC client events are not created", __PRETTY_FUNCTION__, {
				static_cast <uint16_t> (family),
				static_cast <uint16_t> (type),
				static_cast <uint16_t> (protocol)
			}, log::flag_t::CRITICAL);
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			log::print("QUIC client events are not created", log::flag_t::CRITICAL);
		#endif
		// Выводим отрицательный результат
		return 0;
	}
	// Запоминаем семейство адресов события клиента
	this->_family = ((family == event::family_t::IPV6) ? event::family_t::IPV6 : event::family_t::IPV4);
	/**
	 * @brief Снимаем уведомление о перегрузке пути, если система отметок не выдаёт
	 *
	 * @details Отметку ставит маршрутизатор, а показывает её принимающему уже ядро.
	 *          Показывают не все: NetBSD и OpenBSD по IPv4 не показывают вовсе. Помечай
	 *          мы там свои пакеты поддержкой отметок, обратных не увидели бы никогда -
	 *          и договор велел бы признать проверку несостоявшейся и отметки отключить
	 *          (RFC 9000 §13.4.2). Здесь тот же исход достигается сразу, без лишнего круга.
	 *
	 * @note Управление заторами от этого не страдает: оно переходит на потери, которые
	 *       видны всегда. Теряется один сигнал из двух, и только там, где его и нет
	 *
	 * @warning Признак снимается **после** выбора семейства, а не в методе его установки:
	 *          на миг вызова того метода семейство ещё не выбрано, и снимать было бы не
	 *          по чему
	 *
	 */
	if(this->_ecn && !engine::qos::ecn::available(this->_family)){
		// Снимаем уведомление о перегрузке пути, так-как отметок система не выдаёт
		this->_ecn = false;
		/**
		 * Если включён режим отладки
		 */
		#if defined(DEBUG_MODE)
			// Сообщаем, что уведомление о перегрузке пути на этой системе недоступно
			log::debug("QUIC explicit congestion notification is not supported by the system for the selected address family", __PRETTY_FUNCTION__, {
				static_cast <uint16_t> (family),
				static_cast <uint16_t> (type),
				static_cast <uint16_t> (protocol)
			}, log::flag_t::WARNING);
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Сообщаем, что уведомление о перегрузке пути на этой системе недоступно
			log::print("QUIC explicit congestion notification is not supported by the system for the selected address family", log::flag_t::WARNING);
		#endif
	}
	// Сбрасываем флаг выполненного оповещения о завершённом соединении
	this->_notified = false;
	// Устанавливаем интервал проверки таймеров соединения
	engine::timer::timeout(this->_tid, event::action_t::NONE, 25);
	/**
	 * Устанавливаем опции события клиента: извлечение метаданных датаграмм
	 * включается только при уведомлении о перегрузке - оно переводит приём на
	 * разбор служебных сообщений и обходится дороже
	 */
	engine::options::set(this->_eid, static_cast <uint16_t> (
		event::options::NO_SIGILL | event::options::NO_SIGPIPE | event::options::REUSE_ADDR |
		event::options::NO_IO_BLOCK | event::options::CLOSE_ON_EXEC |
		(this->_ecn ? event::options::DGRAM_INFO : 0)
	));
	// Если локальный адрес события клиента не установлен
	if(!engine::address::set(this->_eid, ((this->_family == event::family_t::IPV6) ? event::address_t::IPV6 : event::address_t::IPV4), ((this->_family == event::family_t::IPV6) ? "::" : "0.0.0.0"))){
		/**
		 * Если включён режим отладки
		 */
		#if defined(DEBUG_MODE)
			// Записываем ошибку в лог
			log::debug("QUIC client address is not set", __PRETTY_FUNCTION__, {
				static_cast <uint16_t> (family),
				static_cast <uint16_t> (type),
				static_cast <uint16_t> (protocol)
			}, log::flag_t::CRITICAL);
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			log::print("QUIC client address is not set", log::flag_t::CRITICAL);
		#endif
		// Выводим отрицательный результат
		return 0;
	}
	// Устанавливаем функцию обратного вызова на чтение датаграмм соединения
	engine::io::on(this->_eid, static_cast <engine::callback::read_t> (std::bind(&QuicClient::read, this, _1, _2, _3)));
	/**
	 * Устанавливаем функцию обратного вызова на инъекцию объединённых данных соединения:
	 * перенаправленные из события-источника байты отправляются туннельным потоком
	 * соединения с шифрованием на уровне соединения, а не пишутся сырьём в сокет (splice)
	 */
	engine::io::on(this->_eid, static_cast <engine::callback::inject_t> (std::bind(&QuicClient::inject, this, _2, _3)));
	/**
	 * Устанавливаем функцию обратного вызова источника исходящих датаграмм соединения:
	 * движок вытягивает датаграммы ровно тогда, когда готов их отправить, поэтому
	 * при переполнении очереди они не теряются, а маркировка ECN накладывается
	 * на ту датаграмму, к которой относится
	 */
	engine::io::on(this->_eid, static_cast <engine::callback::source_t> (std::bind(&QuicClient::source, this, _1, _2, _3)));
	// Устанавливаем функцию обратного вызова на завершение подключения к серверу
	engine::io::on(this->_eid, static_cast <engine::callback::connect_t> (std::bind(&QuicClient::connected, this, _1, _2)));
	// Устанавливаем функцию обратного вызова на событие интервала таймеров соединения
	engine::io::on(this->_tid, static_cast <engine::callback::status_t> (std::bind(&QuicClient::tick, this, _1, _2)));
	// Выводим идентификатор созданного события клиента
	return this->_eid;
}
/**
 * @brief Метод остановки клиента
 *
 */
void awh::unit::QuicClient::stop() noexcept {
	// Если работа юнита запущена
	if(this->working())
		// Выполняем остановку работы основного юнита
		unit_t::stop();
}
/**
 * @brief Метод запуска клиента
 *
 */
void awh::unit::QuicClient::start() noexcept {
	// Если работа юнита ещё не запущена
	if(!this->working()){
		// Выполняем получение идентификатора функции обратного вызова
		const callback_t::id_t fid = this->_callback.id("status");
		// Если функция обратного вызова установлена
		if(this->_callback.is(fid))
			// Выполняем получение функции обратного вызова
			this->_callback.set(fid, this->_callback.id("client_status"), this->_callback);
		// Устанавливаем функцию обратного вызова на запуск системы
		this->_callback.on <void (const event::status_t)> (fid, static_cast <void (quic_client_t::*)(const event::status_t)> (&quic_client_t::launch), this, _1);
		// Выполняем запуск работы основного юнита
		unit_t::start();
	}
}
/**
 * @brief Метод установки функций обратного вызова
 *
 * @param callback функции обратного вызова
 *
 */
void awh::unit::QuicClient::callback(const callback_t & callback) noexcept {
	// Выполняем установку функции обратного вызова на установленное соединение
	this->_callback.set("open", callback);
	// Выполняем установку функции обратного вызова на собранные данные потока
	this->_callback.set("read", callback);
    // Выполняем установку функции обратного вызова на завершённое соединение
	this->_callback.set("close", callback);
    // Выполняем установку функции обратного вызова на готовность отправки ранних данных
	this->_callback.set("early", callback);
	// Выполняем установку функции обратного вызова на освобождение буфера отправки потока
	this->_callback.set("writable", callback);
	// Выполняем установку функции обратного вызова на принятую датаграмму приложения
	this->_callback.set("datagram", callback);
}
/**
 * @brief Метод получения согласованного ALPN-протокола соединения
 *
 * @return согласованный ALPN-протокол
 *
 */
awh::tls::coder::alpn_t awh::unit::QuicClient::alpn() const noexcept {
	// Если соединение не создано
	if(this->_connection == nullptr)
		// Выводим пустой результат
		return tls::coder::alpn_t(0, "");
	// Выводим согласованный ALPN-протокол соединения
	return this->_connection->alpn();
}
/**
 * @brief Метод получения предельного размера отправляемой датаграммы (RFC 9221 §3)
 *
 * @return предельный размер данных датаграммы в октетах (0 - датаграммы не поддерживаются)
 *
 */
size_t awh::unit::QuicClient::datagrams() const noexcept {
	// Если соединение не создано
	if(this->_connection == nullptr)
		// Выводим нулевой предел - отправка датаграмм невозможна
		return 0;
	// Выводим предельный размер данных отправляемой датаграммы
	return this->_connection->datagrams();
}
/**
 * @brief Метод отправки датаграммы приложения серверу (RFC 9221)
 *
 * @param data данные датаграммы приложения
 * @return     результат отправки
 *
 */
bool awh::unit::QuicClient::datagram(string_view data) noexcept {
	// Если соединение не создано
	if(this->_connection == nullptr)
		// Выводим отрицательный результат
		return false;
	// Если постановка датаграммы в очередь отправки не выполнена
	if(this->_connection->datagram(data) != quic::status_t::OK)
		// Выводим отрицательный результат
		return false;
	// Отправляем все готовые исходящие датаграммы
	this->flush();
	// Выводим положительный результат
	return true;
}
/**
 * @brief Метод открытия потока приложения соединения
 *
 * @param mode режим однонаправленного потока
 * @return     идентификатор открытого потока
 *
 */
uint64_t awh::unit::QuicClient::open(const bool mode) noexcept {
	// Если соединение не создано
	if(this->_connection == nullptr)
		// Выводим отрицательный результат
		return quic::connection_t::INVALID_STREAM;
	// Выводим идентификатор открытого потока приложения
	return this->_connection->open(mode);
}
/**
 * @brief Метод завершения соединения приложением (RFC 9000 §10.2)
 *
 * @param code   код ошибки приложения
 * @param reason человекочитаемая причина завершения
 *
 */
void awh::unit::QuicClient::close(const uint64_t code, string_view reason) noexcept {
	// Если соединение не создано
	if(this->_connection == nullptr)
		// Выходим из метода
		return;
	// Выполняем завершение соединения приложением
	this->_connection->close(code, reason);
	// Отправляем фрейм завершения соединения удалённому серверу
	this->flush();
}
/**
 * @brief Конструктор
 *
 */
awh::unit::QuicClient::QuicClient() noexcept :
 _ecn(false), _handed(false), _notified(false), _connected(false),
 _eid(0), _tid(0), _dest(0), _tunnel(quic::connection_t::INVALID_STREAM),
 _targetPort(0), _marking(event::ecn_t::NOT_ECT), _family(event::family_t::IPV4),
 _ctx(0), _token{""}, _datagram{""}, _target(nullptr), _connection(nullptr) {}
/**
 * @brief Деструктор
 *
 */
awh::unit::QuicClient::~QuicClient() noexcept {
	// Если событие интервала таймеров соединения создано
	if(this->_tid != 0)
		// Уничтожаем событие интервала таймеров соединения
		engine::io::destroy(this->_tid);
	// Если событие клиента создано
	if(this->_eid != 0){
		// Снимаем функцию обратного вызова на чтение датаграмм соединения
		engine::io::on(this->_eid, static_cast <engine::callback::read_t> (nullptr));
		// Снимаем функцию обратного вызова источника исходящих датаграмм
		engine::io::on(this->_eid, static_cast <engine::callback::source_t> (nullptr));
		// Уничтожаем событие клиента
		engine::io::destroy(this->_eid);
	}
	// Освобождаем объект соединения
	this->_connection.reset(nullptr);
}
