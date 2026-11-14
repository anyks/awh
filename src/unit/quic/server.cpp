/**
 * @file server.cpp
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
 * Стандартные заголовочные файлы
 */
#include <vector>
#include <algorithm>

/**
 * Подключаем заголовочные файлы проекта
 */
#include <sys/fmk.hpp>
#include <sys/log.hpp>
#include <net/addr.hpp>
#include <unit/quic/server.hpp>

/**
 * Используем стандартное пространство имён
 */
using namespace std;

/**
 * Используем пространство имён placeholders
 */
using namespace placeholders;

/**
 * @brief Упаковка записи передачи соединения QUIC работнику кластера
 *
 */
__AWH_PACK_BEGIN__
/**
 * @brief Устройство записи передачи соединения QUIC работнику кластера
 *
 * @details Складывается из устройства сокета, адреса встречной стороны, снимка
 *          сокета, идентификатора соединения и первой датаграммы. Устройство
 *          нужно работнику затем, чтобы завести сокет тем же узлом, каким его
 *          завёл мастер, а идентификатор соединения - чтобы отличить новое
 *          соединение от миграции уже обслуживаемого
 *
 * @note Первая датаграмма едет в сообщении намеренно: её принял общий сокет
 *       мастера, и без неё работник ждал бы повторной отправки клиента целый
 *       таймаут проверки пути (RFC 9000 §6.2)
 *
 */
typedef struct Handover {
	// Длина идентификатора соединения QUIC
	uint8_t key;
	// Вид устройства обмена передаваемого сокета
	uint8_t type;
	// Семейство адресов передаваемого сокета
	uint8_t family;
	// Длина адреса встречной стороны передаваемого сокета
	uint8_t length;
	// Протокол передаваемого сокета
	uint8_t protocol;
	// Порт встречной стороны передаваемого сокета
	uint16_t port;
	// Размер снимка передаваемого сокета
	uint32_t size;
	// Размер первой датаграммы соединения
	uint32_t data;
} __AWH_PACKED__ handover_t;
// Снимаем упаковку записи передачи соединения
__AWH_PACK_END__

/**
 * @brief Конструктор сессии соединения
 *
 */
awh::unit::QuicServer::Session::Session() noexcept :
 handed(false), connected(false), dest(0),
 marking(event::ecn_t::NOT_ECT),
 tunnel(quic::connection_t::INVALID_STREAM),
 carried{""}, datagram{""}, connection(nullptr) {}

/**
 * @brief Конструктор параметров кластера
 *
 */
awh::unit::QuicServer::ClusterParams::ClusterParams() noexcept :
 rebirth(false), mode(event::mode_t::DISABLED),
 count(0), restartLimit(10), restartWindow(30000), name{""} {}

/**
 * @brief Метод заведения объекта кластера
 *
 * @note Зовётся отовсюду, где нужна роль процесса: заводится объект единожды, а
 *       повторные обращения ничего не делают
 *
 */
void awh::unit::QuicServer::clusterCreate() noexcept {
	// Если кластер не заказан либо уже заведён - делать нечего
	if((this->_clusterParams.mode != event::mode_t::ENABLED) || (this->_cluster != nullptr))
		// Выходим из метода
		return;
	// Создаём объект кластера для управления процессами сервера
	this->_cluster = make_unique <cluster_t> ();
	// Если имя кластера установлено
	if(!this->_clusterParams.name.empty())
		// Устанавливаем название кластера
		this->_cluster->name(this->_clusterParams.name);
	// Устанавливаем максимальное количество процессов кластера
	this->_cluster->count(this->_clusterParams.count);
	// Устанавливаем флаг автоматического возрождения процессов
	this->_cluster->rebirth(this->_clusterParams.rebirth);
	// Устанавливаем параметры защиты от цикла перезапусков процессов
	this->_cluster->rebirthLimit(this->_clusterParams.restartLimit, this->_clusterParams.restartWindow);
}
/**
 * @brief Метод запуска/остановки работы сервера
 *
 * @param status статус запуска/остановки сервера
 *
 */
void awh::unit::QuicServer::launch(const event::status_t status) noexcept {
	/**
	 * Определяем статус работы сервера
	 */
	switch(static_cast <uint8_t> (status)){
		// Если работа сервера запущена
		case static_cast <uint8_t> (event::status_t::LAUNCHED): {
			/**
			 * Проверяем требуется ли активировать кластер
			 */
			switch(static_cast <uint8_t> (this->_clusterParams.mode)){
				// Если необходимо активировать кластер
				case static_cast <uint8_t> (event::mode_t::ENABLED): {
					// Заводим объект кластера для управления процессами сервера
					this->clusterCreate();
					// Устанавливаем функцию обратного вызова на событие получения сигнала
					this->_cluster->on <void (const pid_t, const int32_t)> ("exit", &quic_server_t::exit, this, _1, _2);
					// Устанавливаем функцию обратного вызова на событие пересоздания процесса
					this->_cluster->on <void (const pid_t, const pid_t)> ("rebase", &quic_server_t::rebase, this, _1, _2);
					// Устанавливаем функцию обратного вызова на событие отправки сообщений
					this->_cluster->on <void (const pid_t, const size_t)> ("sending", &quic_server_t::sending, this, _1, _2);
					// Устанавливаем функцию обратного вызова на получение событий кластера
					this->_cluster->on <void (const pid_t, const unit::cluster_t::event_t)> ("events", &quic_server_t::cluster, this, _1, _2);
					// Устанавливаем функцию обратного вызова на событие получения сообщений
					this->_cluster->on <void (const pid_t, const uint8_t *, const size_t)> ("message", &quic_server_t::message, this, _1, _2, _3);
					// Устанавливаем функцию обратного вызова на событие изменения статуса кластера
					this->_cluster->on <void (const pid_t, const event::status_t)> ("state", static_cast <void (quic_server_t::*)(const pid_t, const event::status_t)> (&quic_server_t::status), this, _1, _2);
					// Устанавливаем функцию обратного вызова на событие получения ошибок кластера
					this->_cluster->on <void (const pid_t, const event::error_t, const string &)> ("error", static_cast <void (quic_server_t::*)(const pid_t, const event::error_t, const string &)> (&quic_server_t::error), this, _1, _2, _3);
					// Устанавливаем функцию обратного вызова на событие доступности/недоступности очереди исходящих данных кластера
					this->_cluster->on <void (const pid_t, const event::status_t, const size_t)> ("available", static_cast <void (quic_server_t::*)(const pid_t, const event::status_t, const size_t)> (&quic_server_t::available), this, _1, _2, _3);
					/**
					 * @brief Мастер поднимает свой общий сокет ПРЕЖДЕ порождения работников
					 *
					 * @details Разбор идентификаторов соединений ведёт мастер на общем
					 *          сокете, а работнику отдаёт подключённый сокет снимком.
					 *          Принимать первую датаграмму соединения мастер обязан уже
					 *          к мигу порождения работников, иначе соединения первых
					 *          клиентов достались бы никому.
					 *
					 * @note У работника отклик потребителя зовётся из cluster() ПОСЛЕ
					 *       того, как он связался с мастером и завёл собственные
					 *       события: своего слушающего сокета он не заводит вовсе
					 *
					 */
					if(!this->_cluster->worker())
						// Выполняем функцию обратного вызова
						this->_callback.call <void (const event::status_t)> ("server_status", status);
					// Запускаем работу кластера
					this->_cluster->start();
				} break;
				// Если активировать кластер не требуется
				case static_cast <uint8_t> (event::mode_t::DISABLED):
					// Выполняем функцию обратного вызова
					this->_callback.call <void (const event::status_t)> ("server_status", status);
				break;
			}
		} break;
		// Если работа сервера подлежит уничтожению
		case static_cast <uint8_t> (event::status_t::DESTROYED): {
			// Завершаем все сессии соединений сервера
			if(!this->_sessions.empty()){
				// Копируем список идентификаторов сессий для безопасного удаления во время итерации
				vector <event::id_t> sessions;
				// Резервируем память под список идентификаторов сессий
				sessions.reserve(this->_sessions.size());
				/**
				 * Формируем список идентификаторов сессий соединений
				 */
				for(const auto & item : this->_sessions)
					// Добавляем идентификатор сессии соединения в список
					sessions.push_back(item.first);
				/**
				 * Завершаем все сессии соединений
				 */
				for(const auto & oid : sessions)
					// Завершаем сессию соединения
					this->erase(oid);
			}
			// Если событие интервала таймеров соединений создано
			if(this->_tid != 0){
				// Уничтожаем событие интервала таймеров соединений
				engine::io::destroy(this->_tid);
				// Сбрасываем идентификатор события интервала таймеров соединений
				this->_tid = 0;
			}
			// Если событие сервера создано
			if(this->_eid != 0){
				// Снимаем функцию обратного вызова определения сессии
				engine::io::on(this->_eid, static_cast <engine::callback::origin_t> (nullptr));
				// Снимаем функцию обратного вызова на создание сессии соединения
				engine::io::on(this->_eid, static_cast <engine::callback::accept_t> (nullptr));
				// Уничтожаем событие сервера
				engine::io::destroy(this->_eid);
				// Сбрасываем идентификатор события сервера
				this->_eid = 0;
			}
			// Выполняем получение идентификатора функции обратного вызова
			const callback_t::id_t fid = this->_callback.id("server_status");
			// Если функция обратного вызова установлена
			if(this->_callback.is(fid)){
				// Выполняем функцию обратного вызова
				this->_callback.call <void (const event::status_t)> (fid, status);
				// Выполняем получение функции обратного вызова
				this->_callback.set(fid, this->_callback.id("status"), this->_callback);
			}
			/**
			 * @brief Останавливаем кластер ПОСЛЕ отклика потребителя
			 *
			 * @note Работник кластер не останавливает: он не владелец его, и остановка
			 *       от него была бы отказом с записью в журнал. Уходит работник своим
			 *       путём - по событию остановки кластера (см. cluster())
			 *
			 */
			if((this->_clusterParams.mode == event::mode_t::ENABLED) && (this->_cluster != nullptr) && this->_cluster->master())
				// Останавливаем работу кластера
				this->_cluster->stop();
		} break;
	}
}
/**
 * @brief Метод выдачи исходящей датаграммы сессии сетевому движку
 *
 * @param oid    идентификатор события сессии
 * @param buffer адрес указателя на буфер выдаваемых данных
 * @param size   ёмкость запроса на входе и размер выданных данных на выходе
 * @return       признак продолжения вытягивания данных
 *
 */
bool awh::unit::QuicServer::source(const event::id_t oid, const uint8_t ** buffer, size_t & size) noexcept {
	// Выполняем поиск сессии соединения
	auto i = this->_sessions.find(oid);
	// Если сессия соединения не найдена либо соединение не создано
	if((i == this->_sessions.end()) || (i->second.connection == nullptr)){
		// Выданных данных нет
		size = 0;
		// Выводим отрицательный результат - источник исчерпан
		return false;
	}
	// Если предыдущая датаграмма движком уже принята
	if(i->second.handed){
		// Освобождаем буфер под следующую датаграмму
		i->second.datagram.clear();
		// Снимаем флаг выдачи буфера сетевому движку
		i->second.handed = false;
	}
	// Если удерживаемой датаграммы нет
	if(i->second.datagram.empty()){
		// Если исходящих датаграмм у соединения больше нет
		if(!i->second.connection->write(i->second.datagram, fmk::milliseconds())){
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
	if(i->second.datagram.size() > size){
		// Выданных данных нет
		size = 0;
		// Выводим положительный результат - вытягивание продолжается
		return true;
	}
	/**
	 * Применяем маркировку соединения: путь соединения мог проверку поддержки ECN
	 * не пройти, и маркировать его датаграммы далее нельзя, тогда как остальным
	 * соединениям сервера маркировка сохраняется
	 */
	this->mark(oid, i->second, i->second.connection->marking());
	// Выдаём буфер исходящей датаграммы сетевому движку
	(* buffer) = reinterpret_cast <const uint8_t *> (i->second.datagram.data());
	// Выдаём размер исходящей датаграммы
	size = i->second.datagram.size();
	// Поднимаем флаг выдачи буфера сетевому движку
	i->second.handed = true;
	// Выводим положительный результат - вытягивание продолжается
	return true;
}
/**
 * @brief Метод завершения сессии соединения
 *
 * @param oid идентификатор события сессии
 *
 */
void awh::unit::QuicServer::erase(const event::id_t oid) noexcept {
	// Выполняем поиск сессии соединения
	auto i = this->_sessions.find(oid);
	// Если сессия соединения не найдена
	if(i == this->_sessions.end())
		// Выходим из метода
		return;
	// Признак оповещения приложения об установленном соединении
	const bool connected = i->second.connected;
	// Код ошибки завершения соединения для оповещения приложения
	const quic::error_t error = (connected ? i->second.connection->error() : quic::error_t::NO_ERROR);
	/**
	 * Удаляем сессию из карты ДО функции обратного вызова "close": приложение вправе
	 * реентрантно из неё вызвать destroy(oid) на этой же сессии, а раннее удаление
	 * делает вложенный erase(oid) no-op'ом и снимает риск двойного удаления итератора
	 */
	this->_sessions.erase(i);
	// Если приложение было оповещено об установленном соединении
	if(connected)
		// Выполняем функцию обратного вызова о завершённом соединении
		this->_callback.call <void (const event::id_t, const quic::error_t)> ("close", oid, error);
	/**
	 * @brief Снимаем маршруты завершённого соединения
	 *
	 * @note Маршрут живёт до завершения соединения: держать его дальше значило бы
	 *       копить память на всякий когда-либо виденный идентификатор, а отдавать
	 *       по нему датаграмму - значит отдавать её в пустоту
	 *
	 */
	for(auto it = this->_keys.begin(); it != this->_keys.end();){
		// Если идентификатор соединения принадлежит завершаемой сессии
		if(it->second == oid){
			// Если кластер в работе и соединение обслуживает работник
			if((this->_cluster != nullptr) && !this->_cluster->master()){
				// Буфер объявления отвода идентификатора соединения
				vector <uint8_t> notice;
				// Оставляем в начале объявления место под опознаватель отвода
				notice.push_back(quic_server_t::HANDOVER_KEY_REMOVE);
				// Добавляем в объявление отводимый идентификатор соединения
				notice.insert(notice.end(), it->first.begin(), it->first.end());
				// Отправляем мастеру объявление отвода идентификатора соединения
				this->_cluster->send(notice.data(), notice.size());
			}
			// Снимаем маршрут идентификатора соединения
			this->_route.erase(it->first);
			// Снимаем путь клиента снятого маршрута
			this->_routePath.erase(it->first);
			// Удаляем зеркало маршрутизации и переходим к следующему идентификатору
			it = this->_keys.erase(it);
		// Если идентификатор соединения принадлежит другой сессии
		} else
			// Переходим к идентификатору следующему
			++it;
	}
	// Уничтожаем событие сессии вместе с его ключами маршрутизации
	engine::io::destroy(oid);
}
/**
 * @brief Метод отправки готовых исходящих датаграмм соединения
 *
 * @param oid     идентификатор события сессии
 * @param session сессия соединения
 *
 */
bool awh::unit::QuicServer::flush(const event::id_t oid, session_t & session) noexcept {
	/**
	 * Если удерживаемой датаграммы нет - извлекаем первую исходящую датаграмму соединения:
	 * ею подтверждается, что соединению есть чем ответить, и она же уходит первой при
	 * вытягивании. Остальные датаграммы движок заберёт сам через функцию-источник
	 */
	if(!session.handed && session.datagram.empty()){
		// Если исходящих датаграмм у соединения нет
		if(!session.connection->write(session.datagram, fmk::milliseconds()))
			// Выводим отрицательный результат - отправлять нечего
			return false;
	}
	// Заводим вытягивание исходящих датаграмм сетевым движком
	engine::io::send(oid, nullptr, 0);
	// Выводим положительный результат
	return true;
}
/**
 * @brief Метод выдачи собранных данных потоков приложения
 *
 * @param oid     идентификатор события сессии
 * @param session сессия соединения
 *
 */
void awh::unit::QuicServer::process(const event::id_t oid) noexcept {
	/**
	 * Сессию берём по идентификатору, а не по удерживаемой ссылке: выдача данных
	 * вызывает функции обратного вызова приложения ("read"/"datagram"), а приложение
	 * вправе реентрантно завершить соединение прямо в них (destroy/erase). Тогда узел
	 * сессии освобождается, и любая ранее взятая ссылка session_t& становится висячей.
	 * Поэтому сессию перечитываем из карты после каждого колбэка и прекращаем выдачу,
	 * если она была снята
	 */
	auto i = this->_sessions.find(oid);
	// Если сессия соединения не найдена
	if(i == this->_sessions.end())
		// Выходим из метода
		return;
	/**
	 * Если для сессии установлено объединение данных (splice), собранные данные
	 * потоков и датаграмм перенаправляются в событие-приёмник вместо выдачи
	 * приложению - строится прозрачный канал между сессией и другим событием
	 */
	const bool spliced = (i->second.dest != 0);
	// Если объединение установлено либо функция обратного вызова на собранные данные потока установлена
	if(spliced || this->_callback.is("read")){
		// Список потоков с собранными данными
		vector <uint64_t> streams;
		// Получаем список потоков с собранными данными
		i->second.connection->readable(streams);
		/**
		 * Перебираем потоки с собранными данными
		 */
		for(auto & sid : streams){
			// Перечитываем сессию: предыдущий колбэк мог реентрантно снять её
			i = this->_sessions.find(oid);
			// Если сессия соединения снята приложением из функции обратного вызова
			if(i == this->_sessions.end())
				// Выходим из метода
				return;
			// Флаг завершения потока удалённым эндпоинтом
			bool fin = false;
			// Собранные данные потока приложения
			string data = "";
			// Если выдача собранных данных потока не выполнена
			if(i->second.connection->receive(sid, data, fin) != quic::status_t::OK)
				// Пропускаем поток с ошибкой выдачи
				continue;
			// Если данные получены либо поток завершён
			if(!data.empty() || fin){
				// Если для сессии установлено объединение данных - перенаправляем их в событие-приёмник
				if(spliced)
					// Перенаправляем собранные данные потока в событие-приёмник объединения
					this->forward(i->second.dest, data);
				// Иначе выдаём собранные данные потока приложению
				else this->_callback.call <void (const event::id_t, const uint64_t, const string &, const bool)> ("read", oid, sid, data, fin);
			}
		}
	}
	// Перечитываем сессию перед выдачей датаграмм: цикл потоков мог реентрантно снять её
	i = this->_sessions.find(oid);
	// Если сессия соединения снята приложением из функции обратного вызова
	if(i == this->_sessions.end())
		// Выходим из метода
		return;
	// Если функция обратного вызова на освобождение буфера отправки потока установлена
	if(this->_callback.is("writable")){
		// Список потоков с освободившимся буфером отправки
		vector <uint64_t> streams;
		// Получаем список потоков, готовых принять данные (буфер опустился ниже нижней метки)
		i->second.connection->drained(streams);
		/**
		 * Перебираем потоки с освободившимся буфером отправки: приложение, получившее
		 * частичный приём в send(), по этому сигналу возобновляет выдачу данных потока
		 */
		for(auto & sid : streams){
			// Выдаём приложению сигнал готовности потока принимать данные
			this->_callback.call <void (const event::id_t, const uint64_t)> ("writable", oid, sid);
			// Перечитываем сессию: колбэк writable мог реентрантно снять её
			i = this->_sessions.find(oid);
			// Если сессия соединения снята приложением из функции обратного вызова
			if(i == this->_sessions.end())
				// Выходим из метода
				return;
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
		while(i->second.connection->datagram(datagram)){
			// Если для сессии установлено объединение данных - перенаправляем датаграмму в событие-приёмник
			if(spliced)
				// Перенаправляем принятую датаграмму в событие-приёмник объединения
				this->forward(i->second.dest, datagram);
			// Иначе выдаём принятую датаграмму приложению
			else this->_callback.call <void (const event::id_t, const string &)> ("datagram", oid, datagram);
			// Перечитываем сессию: колбэк датаграммы мог реентрантно снять её
			i = this->_sessions.find(oid);
			// Если сессия соединения снята приложением из функции обратного вызова
			if(i == this->_sessions.end())
				// Выходим из метода
				return;
		}
	}
}
/**
 * @brief Метод формирования адреса удалённого эндпоинта сессии
 *
 * @note Узел-приёмник хранит адрес собеседника в адресе источника, а клиентский
 *       узел работника кластера - в цели подключения: его адрес источника свой
 *
 * @param oid идентификатор события сессии
 * @return    адрес удалённого эндпоинта в виде "адрес:порт"
 *
 */
string awh::unit::QuicServer::peer(const event::id_t oid) const noexcept {
	// Если сессия работника кластера заведена клиентским узлом движка
	if(engine::io::node(oid) == event::node_t::CLIENT)
		// Выводим адрес удалённого эндпоинта из цели подключения вместе с её портом
		return (engine::target::get(oid) + ":" + std::to_string(engine::target::port::get(oid)));
	// Извлекаем адрес удалённого эндпоинта сессии
	string result = engine::address::get(oid, event::address_t::IPV4);
	// Если адрес IPv4 не получен
	if(result.empty())
		// Извлекаем адрес удалённого эндпоинта сессии в семействе IPv6
		result = engine::address::get(oid, event::address_t::IPV6);
	// Выводим адрес удалённого эндпоинта вместе с портом
	return (result + ":" + std::to_string(engine::source::port::get(oid)));
}
/**
 * @brief Метод обработки события пересоздания процесса кластера
 *
 * @param old старый идентификатор процесса
 * @param pid текущий идентификатор процесса
 *
 */
void awh::unit::QuicServer::rebase(const pid_t old, const pid_t pid) noexcept {
	// Если кластер в работе
	if(this->_cluster != nullptr){
		// Ищем погибшего работника в списке готовых обслуживать соединения
		auto i = std::find(this->_handoverWorkers.begin(), this->_handoverWorkers.end(), old);
		// Если погибший работник был учтён
		if(i != this->_handoverWorkers.end())
			// Заменяем его возрождённым: место в очереди раздачи соединений сохраняется
			*i = pid;
		/**
		 * @brief Снимаем маршруты, ведущие к погибшему работнику
		 *
		 * @note Соединения погибли вместе с ним: возрождённый процесс начинает с
		 *       чистого листа, и держать маршрут на него значило бы отдавать
		 *       датаграммы миграции в пустоту
		 *
		 */
		for(auto it = this->_route.begin(); it != this->_route.end();){
			// Если маршрут ведёт к погибшему работнику
			if(it->second == old){
				// Снимаем путь клиента снятого маршрута
				this->_routePath.erase(it->first);
				// Удаляем маршрут и переходим к следующему
				it = this->_route.erase(it);
			// Если маршрут ведёт к другому работнику
			} else
				// Переходим к маршруту следующему
				++it;
		}
	}
	// Выполняем функцию обратного вызова
	this->_callback.call <void (const pid_t, const pid_t)> ("cluster_rebase", old, pid);
}
/**
 * @brief Метод получения события завершения работы процесса кластера
 *
 * @param pid    идентификатор процесса
 * @param status состояние, с которым завершился процесс
 *
 */
void awh::unit::QuicServer::exit(const pid_t pid, const int32_t status) noexcept {
	// Если кластер в работе
	if(this->_cluster != nullptr){
		// Ищем завершившегося работника в списке готовых обслуживать соединения
		auto i = std::find(this->_handoverWorkers.begin(), this->_handoverWorkers.end(), pid);
		// Если завершившийся работник был учтён
		if(i != this->_handoverWorkers.end())
			// Снимаем его с учёта: новые соединения ему больше не достаются
			this->_handoverWorkers.erase(i);
		/**
		 * @brief Снимаем маршруты, ведущие к завершившемуся работнику
		 *
		 * @note Соединения его погибли: датаграмма миграции по такому маршруту
		 *       ушла бы в пустоту, а без маршрута мастер хотя бы ответит
		 *       сбросом без сохранения состояния
		 *
		 */
		for(auto it = this->_route.begin(); it != this->_route.end();){
			// Если маршрут ведёт к завершившемуся работнику
			if(it->second == pid){
				// Снимаем путь клиента снятого маршрута
				this->_routePath.erase(it->first);
				// Удаляем маршрут и переходим к следующему
				it = this->_route.erase(it);
			// Если маршрут ведёт к другому работнику
			} else
				// Переходим к маршруту следующему
				++it;
		}
	}
	// Выполняем функцию обратного вызова
	this->_callback.call <void (const pid_t, const int32_t)> ("cluster_exit", pid, status);
}
/**
 * @brief Метод обработки события отправки сообщения процессу кластера
 *
 * @param pid  идентификатор процесса
 * @param size размер отправленного сообщения
 *
 */
void awh::unit::QuicServer::sending(const pid_t pid, const size_t size) noexcept {
	// Выполняем функцию обратного вызова
	this->_callback.call <void (const pid_t, const size_t)> ("cluster_sending", pid, size);
}
/**
 * @brief Метод перепривязки сессии соединения к новому сокету
 *
 * @param oid идентификатор события прежнего сокета сессии
 * @param cid идентификатор события нового сокета сессии
 *
 */
void awh::unit::QuicServer::rebind(const event::id_t oid, const event::id_t cid) noexcept {
	// Выполняем поиск сессии прежнего сокета соединения
	auto i = this->_sessions.find(oid);
	// Если сессия соединения не найдена
	if(i == this->_sessions.end()){
		// Удаляем новый сокет соединения
		engine::io::destroy(cid);
		// Выходим из метода
		return;
	}
	// Переносим сессию соединения на новый сокет целиком
	auto ret = this->_sessions.emplace(cid, std::move(i->second));
	// Снимаем сессию с прежнего сокета
	this->_sessions.erase(i);
	// Если сессия на новый сокет не перенесена
	if(!ret.second){
		// Удаляем новый сокет соединения
		engine::io::destroy(cid);
		// Выходим из метода
		return;
	}
	// Получаем перенесённую сессию соединения
	session_t & session = ret.first->second;
	// Устанавливаем функцию обратного вызова на чтение датаграмм сессии
	engine::io::on(cid, static_cast <engine::callback::read_t> (std::bind(&QuicServer::read, this, _1, _2, _3)));
	// Устанавливаем функцию обратного вызова на инъекцию объединённых данных сессии
	engine::io::on(cid, static_cast <engine::callback::inject_t> (std::bind(&QuicServer::inject, this, _1, _2, _3)));
	// Устанавливаем функцию обратного вызова источника исходящих датаграмм сессии
	engine::io::on(cid, static_cast <engine::callback::source_t> (std::bind(&QuicServer::source, this, _1, _2, _3)));
	// Обновляем адрес удалённого эндпоинта соединения адресом нового пути
	this->endpoint(cid, session.connection.get());
	/**
	 * @brief Переносим ключи маршрутизации соединения на новый сокет
	 *
	 * @note Ключей у соединения несколько: выданные клиенту идентификаторы живут
	 *       до их отвода, и клиент вправе адресовать соединение любым из них
	 *
	 */
	for(auto & item : this->_keys){
		// Если идентификатор соединения принадлежит перепривязываемой сессии
		if(item.second == oid){
			// Выделяем идентификатор соединения в ключ маршрутизации
			const net::origin_key_t route(reinterpret_cast <const uint8_t *> (item.first.data()), static_cast <uint8_t> (item.first.size()));
			// Снимаем идентификатор соединения с прежнего сокета
			engine::multicast::unbind(oid, route);
			// Привязываем идентификатор соединения к новому сокету
			engine::multicast::bind(cid, route);
			// Запоминаем новый сокет сессии в зеркале маршрутизации
			item.second = cid;
		}
	}
	// Снимаем флаг выдачи буфера исходящей датаграммы: выдавался он прежнему сокету
	session.handed = false;
	// Сбрасываем признак установленной маркировки: ставилась она прежнему сокету
	session.marking = event::ecn_t::NOT_ECT;
	// Применяем маркировку соединения к новому сокету
	this->mark(cid, session, session.connection->marking());
	// Удаляем прежний сокет соединения
	engine::io::destroy(oid);
}
/**
 * @brief Метод учёта работника, готового обслуживать соединения
 *
 * @param pid идентификатор процесса работника
 *
 */
void awh::unit::QuicServer::handover(const pid_t pid) noexcept {
	// Если работник уже учтён - учитывать его снова незачем
	if(std::find(this->_handoverWorkers.begin(), this->_handoverWorkers.end(), pid) != this->_handoverWorkers.end())
		// Выходим из метода
		return;
	// Запоминаем работника, готового обслуживать соединения
	this->_handoverWorkers.push_back(pid);
	// Записываем в лог сообщение о готовности работника обслуживать соединения
	log::print("Cluster worker process [%d] is ready to serve QUIC connections", log::flag_t::INFO, pid);
}
/**
 * @brief Метод подъёма соединения из снимка, присланного мастером
 *
 * @param data данные полученного сообщения
 * @param size размер данных полученного сообщения
 * @return     признак того, что сообщение было передачей соединения
 *
 */
bool awh::unit::QuicServer::handover(const uint8_t * data, const size_t size) noexcept {
	// Если сообщение короче опознавателя с устройством - передачей оно не является
	if((data == nullptr) || (size < (sizeof(uint8_t) + sizeof(handover_t))) || (data[0] != quic_server_t::HANDOVER_CONNECTION))
		// Выводим результат
		return false;
	// Устройство переданного соединения
	handover_t record{};
	// Извлекаем устройство переданного соединения
	::memcpy(&record, data + sizeof(uint8_t), sizeof(record));
	// Если остатка сообщения не хватает на адрес, снимок, идентификатор и датаграмму
	if((size - sizeof(uint8_t) - sizeof(record)) < (static_cast <size_t> (record.length) + static_cast <size_t> (record.size) + static_cast <size_t> (record.key) + static_cast <size_t> (record.data)))
		// Выводим результат
		return false;
	// Смещение снимка передаваемого сокета в сообщении
	const size_t offset = (sizeof(uint8_t) + sizeof(record) + static_cast <size_t> (record.length));
	// Получаем адрес встречной стороны переданного соединения
	const string target(reinterpret_cast <const char *> (data + sizeof(uint8_t) + sizeof(record)), static_cast <size_t> (record.length));
	// Получаем идентификатор переданного соединения
	const string key(reinterpret_cast <const char *> (data + offset + static_cast <size_t> (record.size)), static_cast <size_t> (record.key));
	// Получаем первую датаграмму переданного соединения
	const uint8_t * dgram = (data + offset + static_cast <size_t> (record.size) + static_cast <size_t> (record.key));
	/**
	 * @brief Заводим событие соединения узлом клиента
	 *
	 * @details Принятое соединение у работника заводится именно клиентом: серверный
	 *          узел занял бы общий порт повторно, а он держится мастером. Работа же
	 *          у подключённого сокета одна - связанный обмен с клиентом.
	 *
	 */
	const event::id_t cid = engine::io::event(
		event::node_t::CLIENT, static_cast <event::family_t> (record.family),
		static_cast <event::type_t> (record.type), static_cast <event::protocol_t> (record.protocol)
	);
	// Если событие соединения завести не удалось
	if(cid == 0){
		/**
		 * Если включён режим отладки
		 */
		#if defined(DEBUG_MODE)
			// Записываем ошибку в лог
			log::debug("QUIC cluster worker process [%d] cannot create an event for the connection received from the master", __PRETTY_FUNCTION__, {data, size}, log::flag_t::CRITICAL, ::getpid());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			log::print("QUIC cluster worker process [%d] cannot create an event for the connection received from the master", log::flag_t::CRITICAL, ::getpid());
		#endif
		// Выводим признак того, что сообщение было передачей соединения
		return true;
	}
	// Устанавливаем опции поднимаемого сокета соединения
	engine::options::set(cid, static_cast <uint16_t> (
		event::options::NO_SIGILL | event::options::NO_SIGPIPE | event::options::REUSE_ADDR |
		event::options::REUSE_PORT | event::options::NO_IO_BLOCK | event::options::CLOSE_ON_EXEC |
		(this->_ecn ? event::options::DGRAM_INFO : 0)
	));
	/**
	 * @brief Задаём событию соединения адрес встречной стороны
	 *
	 * @note Узлу клиента адрес этот обязателен: фиксация настроек без него отвечает
	 *       отказом. Сокет при этом уже связан, и к установке связи адрес отношения
	 *       не имеет - он лишь описывает встречную сторону
	 *
	 */
	if(!engine::target::port::set(cid, record.port) || !engine::target::set(cid, target)){
		// Удаляем заведённое событие соединения
		engine::io::destroy(cid);
		/**
		 * Если включён режим отладки
		 */
		#if defined(DEBUG_MODE)
			// Записываем ошибку в лог
			log::debug("QUIC cluster worker process [%d] cannot set the target of the connection received from the master", __PRETTY_FUNCTION__, {data, size}, log::flag_t::CRITICAL, ::getpid());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			log::print("QUIC cluster worker process [%d] cannot set the target of the connection received from the master", log::flag_t::CRITICAL, ::getpid());
		#endif
		// Выводим признак того, что сообщение было передачей соединения
		return true;
	}
	/**
	 * @brief Поднимаем сокет из присланного снимка, подключаем и запускаем его
	 *
	 * @details Фиксация здесь не зовётся намеренно: описатель уже заведён мастером,
	 *          и заводить второй означало бы занимать порт повторно. Подключение
	 *          движок берёт лишь у узла в состоянии INITIAL, а запуск переводит его
	 *          в LAUNCHED - обратный порядок оставляет отправку молча нулевой.
	 *
	 */
	if(!engine::io::restore(cid, data + offset, static_cast <size_t> (record.size)) ||
	   !engine::io::connect(cid) || !engine::io::launch(cid)){
		// Удаляем заведённое событие соединения
		engine::io::destroy(cid);
		/**
		 * Если включён режим отладки
		 */
		#if defined(DEBUG_MODE)
			// Записываем ошибку в лог
			log::debug("QUIC cluster worker process [%d] cannot restore the connection received from the master", __PRETTY_FUNCTION__, {data, size}, log::flag_t::CRITICAL, ::getpid());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			log::print("QUIC cluster worker process [%d] cannot restore the connection received from the master", log::flag_t::CRITICAL, ::getpid());
		#endif
		// Выводим признак того, что сообщение было передачей соединения
		return true;
	}
	// Ищем свою сессию переданного соединения
	auto i = this->_keys.find(key);
	/**
	 * Если соединение у работника уже есть - это миграция пути (RFC 9000 §9)
	 *
	 * @note Сессию не заводим заново: клиент сменил адрес, а соединение осталось
	 *       прежним, со всеми своими ключами и состоянием
	 *
	 */
	if(i != this->_keys.end())
		// Выполняем перепривязку сессии соединения к новому сокету
		this->rebind(i->second, cid);
	// Если соединение новое
	else
		// Отдаём соединение потребителю обычным путём принятого соединения
		this->accept(this->_eid, cid);
	// Устанавливаем функцию обратного вызова на отчёт движка об установленном подключении сессии
	engine::io::on(cid, static_cast <engine::callback::connect_t> (std::bind(&QuicServer::connected, this, _1, _2)));
	// Выполняем поиск сессии принятого соединения
	auto s = this->_sessions.find(cid);
	// Если сессия соединения найдена
	if(s != this->_sessions.end())
		/**
		 * @brief Удерживаем первую датаграмму, принятую мастером, до отчёта движка о подключении
		 *
		 * @note Без неё работник ждал бы повторной отправки клиента целый таймаут
		 *       проверки пути (RFC 9000 §6.2)
		 *
		 * @warning Скормить её сразу нельзя: узел работника заводится клиентом, а движок
		 *          позволяет отправку его датаграмм лишь после вызова подписки `connect_t`.
		 *          До этого отправка уходит получателю явным адресом на уже связанном
		 *          сокете, система отвечает отказом, и движок уничтожает узел - ответ
		 *          соединения пропадает, а клиент повторяет отправку по кругу
		 *
		 */
		s->second.carried.assign(reinterpret_cast <const char *> (dgram), static_cast <size_t> (record.data));
	// Выводим признак того, что сообщение было передачей соединения
	return true;
}
/**
 * @brief Метод передачи соединения работнику кластера
 *
 * @param pid  идентификатор процесса работника
 * @param oid  идентификатор события сессии, принявшей первую датаграмму
 * @param data данные первой датаграммы соединения
 * @param size размер первой датаграммы соединения
 * @return     признак того, что соединение отдано работнику
 *
 */
bool awh::unit::QuicServer::handover(const pid_t pid, const event::id_t oid, const uint8_t * data, const size_t size) noexcept {
	// Если данных первой датаграммы нет - передавать нечего
	if((data == nullptr) || (size == 0))
		// Выводим результат отказа
		return false;
	// Получаем событие обмена, ведущее к работнику
	const event::id_t dest = this->_cluster->channel(pid);
	// Если события обмена с работником нет - работник пал
	if(dest == 0)
		// Выводим результат отказа
		return false;
	// Получаем семейство адресов сессии, принявшей датаграмму
	const event::family_t family = engine::io::family(oid);
	/**
	 * @brief Заводим подключённый сокет соединения узлом клиента
	 *
	 * @details Узел клиента, а не сервера: серверный узел занял бы порт повторно,
	 *          а подключённый сокет ядро различает четвёркой и отдаёт ему
	 *          датаграммы этого клиента раньше общего сокета мастера.
	 *
	 */
	const event::id_t cid = engine::io::event(event::node_t::CLIENT, family, event::type_t::DATAGRAM, event::protocol_t::UDP);
	// Если сокет соединения завести не удалось
	if(cid == 0){
		/**
		 * Если включён режим отладки
		 */
		#if defined(DEBUG_MODE)
			// Записываем ошибку в лог
			log::debug("QUIC cluster master process [%d] cannot create a socket for the connection handed to the worker [%d]", __PRETTY_FUNCTION__, {pid, oid, data, size}, log::flag_t::CRITICAL, ::getpid(), pid);
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			log::print("QUIC cluster master process [%d] cannot create a socket for the connection handed to the worker [%d]", log::flag_t::CRITICAL, ::getpid(), pid);
		#endif
		// Выводим результат отказа
		return false;
	}
	// Устанавливаем опции подключённого сокета соединения
	engine::options::set(cid, static_cast <uint16_t> (
		event::options::NO_SIGILL | event::options::NO_SIGPIPE | event::options::REUSE_ADDR |
		event::options::REUSE_PORT | event::options::NO_IO_BLOCK | event::options::CLOSE_ON_EXEC |
		(this->_ecn ? event::options::DGRAM_INFO : 0)
	));
	// Получаем адрес встречной стороны соединения
	const string target = engine::address::get(oid, ((family == event::family_t::IPV6) ? event::address_t::IPV6 : event::address_t::IPV4));
	/**
	 * @brief Задаём порты и адрес встречной стороны подключённого сокета
	 *
	 * @note Портом источника ставится общий порт кластера: клиент обязан видеть
	 *       ответ с того же порта, куда слал, иначе он отвергнет его как
	 *       пришедший не с того пути (RFC 9000 §9)
	 *
	 */
	if(target.empty() || !engine::source::port::set(cid, this->_listenPort) ||
	  !engine::target::port::set(cid, engine::source::port::get(oid)) || !engine::target::set(cid, target)){
		// Удаляем заведённый сокет соединения
		engine::io::destroy(cid);
		/**
		 * Если включён режим отладки
		 */
		#if defined(DEBUG_MODE)
			// Записываем ошибку в лог
			log::debug("QUIC cluster master process [%d] cannot set the endpoint of the connection handed to the worker [%d]", __PRETTY_FUNCTION__, {pid, oid, data, size}, log::flag_t::CRITICAL, ::getpid(), pid);
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			log::print("QUIC cluster master process [%d] cannot set the endpoint of the connection handed to the worker [%d]", log::flag_t::CRITICAL, ::getpid(), pid);
		#endif
		// Выводим результат отказа
		return false;
	}
	/**
	 * @brief Фиксируем, подключаем и запускаем подключённый сокет
	 *
	 * @details Порядок именно такой: подключение движок берёт лишь у узла в
	 *          состоянии INITIAL, а запуск переводит его в LAUNCHED. Запуск без
	 *          подключения оставляет датаграммную отправку молча нулевой.
	 *
	 */
	if(!engine::io::commit(cid) || !engine::io::connect(cid) || !engine::io::launch(cid)){
		// Удаляем заведённый сокет соединения
		engine::io::destroy(cid);
		/**
		 * Если включён режим отладки
		 */
		#if defined(DEBUG_MODE)
			// Записываем ошибку в лог
			log::debug("QUIC cluster master process [%d] cannot start the socket of the connection handed to the worker [%d]", __PRETTY_FUNCTION__, {pid, oid, data, size}, log::flag_t::CRITICAL, ::getpid(), pid);
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			log::print("QUIC cluster master process [%d] cannot start the socket of the connection handed to the worker [%d]", log::flag_t::CRITICAL, ::getpid(), pid);
		#endif
		// Выводим результат отказа
		return false;
	}
	// Буфер снимка подключённого сокета соединения
	vector <uint8_t> snapshot;
	// Если снимок подключённого сокета для процесса работника не снят
	if(!engine::io::snapshot(cid, dest, snapshot)){
		// Удаляем заведённый сокет соединения
		engine::io::destroy(cid);
		/**
		 * Если включён режим отладки
		 */
		#if defined(DEBUG_MODE)
			// Записываем ошибку в лог
			log::debug("QUIC cluster master process [%d] cannot take a snapshot of the connection handed to the worker [%d]", __PRETTY_FUNCTION__, {pid, oid, data, size}, log::flag_t::CRITICAL, ::getpid(), pid);
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			log::print("QUIC cluster master process [%d] cannot take a snapshot of the connection handed to the worker [%d]", log::flag_t::CRITICAL, ::getpid(), pid);
		#endif
		// Выводим результат отказа
		return false;
	}
	// Устройство передаваемого соединения
	handover_t record{};
	// Запоминаем семейство адресов передаваемого сокета
	record.family = static_cast <uint8_t> (family);
	// Запоминаем вид устройства обмена передаваемого сокета
	record.type = static_cast <uint8_t> (event::type_t::DATAGRAM);
	// Запоминаем протокол передаваемого сокета
	record.protocol = static_cast <uint8_t> (event::protocol_t::UDP);
	// Запоминаем порт встречной стороны соединения
	record.port = engine::source::port::get(oid);
	// Запоминаем длину адреса встречной стороны соединения
	record.length = static_cast <uint8_t> (target.size());
	// Запоминаем размер снимка передаваемого сокета
	record.size = static_cast <uint32_t> (snapshot.size());
	// Запоминаем длину идентификатора соединения
	record.key = static_cast <uint8_t> (this->_handoverKey.size());
	// Запоминаем размер первой датаграммы соединения
	record.data = static_cast <uint32_t> (size);
	// Буфер сообщения передачи соединения
	vector <uint8_t> message;
	// Оставляем в начале сообщения место под опознаватель передачи
	message.push_back(quic_server_t::HANDOVER_CONNECTION);
	// Добавляем в сообщение устройство передаваемого соединения
	message.insert(message.end(), reinterpret_cast <const uint8_t *> (&record), reinterpret_cast <const uint8_t *> (&record) + sizeof(record));
	// Добавляем в сообщение адрес встречной стороны соединения
	message.insert(message.end(), target.begin(), target.end());
	// Добавляем в сообщение снимок передаваемого сокета
	message.insert(message.end(), snapshot.begin(), snapshot.end());
	// Добавляем в сообщение идентификатор соединения
	message.insert(message.end(), this->_handoverKey.begin(), this->_handoverKey.end());
	// Добавляем в сообщение первую датаграмму соединения
	message.insert(message.end(), data, data + size);
	/**
	 * @brief Если сообщение передачи не помещается в буфер чтения движка
	 *
	 * @note Канал кластера принимает сообщение за один приём: большим его движок
	 *       не прочтёт, и передача оборвалась бы на середине молча
	 *
	 */
	if(message.size() > quic_server_t::HANDOVER_LIMIT){
		// Удаляем заведённый сокет соединения
		engine::io::destroy(cid);
		/**
		 * Если включён режим отладки
		 */
		#if defined(DEBUG_MODE)
			// Записываем ошибку в лог
			log::debug("QUIC cluster master process [%d] cannot hand the connection of %zu bytes to the worker [%d]: the message exceeds the limit of %zu bytes", __PRETTY_FUNCTION__, {pid, oid, data, size}, log::flag_t::CRITICAL, ::getpid(), message.size(), pid, quic_server_t::HANDOVER_LIMIT);
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			log::print("QUIC cluster master process [%d] cannot hand the connection of %zu bytes to the worker [%d]: the message exceeds the limit of %zu bytes", log::flag_t::CRITICAL, ::getpid(), message.size(), pid, quic_server_t::HANDOVER_LIMIT);
		#endif
		// Выводим результат отказа
		return false;
	}
	// Если отправить передачу работнику не удалось
	if(this->_cluster->send(pid, message.data(), message.size()) == 0){
		// Удаляем заведённый сокет соединения
		engine::io::destroy(cid);
		/**
		 * Если включён режим отладки
		 */
		#if defined(DEBUG_MODE)
			// Записываем ошибку в лог
			log::debug("QUIC cluster master process [%d] cannot send the connection to the worker [%d]", __PRETTY_FUNCTION__, {pid, oid, data, size}, log::flag_t::CRITICAL, ::getpid(), pid);
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			log::print("QUIC cluster master process [%d] cannot send the connection to the worker [%d]", log::flag_t::CRITICAL, ::getpid(), pid);
		#endif
		// Выводим результат отказа
		return false;
	}
	/**
	 * @brief Расстаёмся со своим подключённым сокетом: обслуживает его теперь работник
	 *
	 * @note Снимок уже снят, и объект сокета живёт у работника. Держи мастер свой
	 *       описатель дальше, датаграммы клиента доставались бы тому, кто спросит
	 *       первым, и соединение шло бы вразнос
	 *
	 */
	engine::io::destroy(cid);
	/**
	 * @brief Запоминаем маршрут соединения: обслуживает его работник
	 *
	 * @note Сессию, принятую на общем сокете, здесь НЕ снимаем: описатель у неё
	 *       общий с сервером, подключением мастера она не является. Снятая сессия
	 *       тотчас получает статус DESTROYED, и запрос адреса отправителя на ней
	 *       отвечает пустотой, тогда как ключи маршрутизации снимаются отложенно:
	 *       остаток той же пачки датаграмм пришёл бы на узел без адреса, передача
	 *       отказала бы уже заведённому маршруту и датаграмма была бы потеряна.
	 *       Под MS Windows так терялась датаграмма приложения (RFC 9221) - повторной
	 *       отправки у неё нет, в отличие от потока
	 *
	 * @note Сессия следует за отправителем сама: ключ маршрутизации у неё задан
	 *       идентификатором соединения, а адрес обновляется при смене пути,
	 *       поэтому миграция клиента проходит разбор у мастера и без пересоздания
	 *
	 */
	this->_route[this->_handoverKey] = pid;
	// Запоминаем путь клиента, на котором соединение передано работнику
	this->_routePath[this->_handoverKey] = this->peer(oid);
	// Выводим успешный результат: соединение отдано работнику
	return true;
}
/**
 * @brief Метод приёма пересланной мастером датаграммы соединения
 *
 * @param data данные полученного сообщения
 * @param size размер данных полученного сообщения
 * @return     признак того, что сообщение было пересылкой датаграммы
 *
 */
bool awh::unit::QuicServer::relay(const uint8_t * data, const size_t size) noexcept {
	// Размер заголовка сообщения пересылки: опознаватель и длина идентификатора
	const size_t head = (sizeof(uint8_t) + sizeof(uint8_t));
	// Если сообщение короче заголовка - пересылкой оно не является
	if((data == nullptr) || (size < head) || (data[0] != quic_server_t::HANDOVER_DATAGRAM))
		// Выводим результат
		return false;
	// Длина идентификатора соединения в сообщении
	const size_t length = static_cast <size_t> (data[1]);
	// Если остатка сообщения не хватает на идентификатор и датаграмму
	if((size - head) < (length + 1))
		// Выводим результат
		return false;
	// Получаем идентификатор переданного соединения
	const string key(reinterpret_cast <const char *> (data + head), length);
	// Ищем свою сессию переданного соединения
	auto i = this->_keys.find(key);
	/**
	 * @brief Если сессии соединения у работника нет - датаграмму обрабатывать некому
	 *
	 * @note Соединение могло завершиться, а мастер ещё не получил объявления отвода
	 *       идентификатора. Сообщение пересылки потребителю не предназначалось
	 *
	 */
	if(i == this->_keys.end())
		// Выводим признак того, что сообщение было пересылкой датаграммы
		return true;
	// Получаем данные датаграммы соединения
	const uint8_t * dgram = (data + head + length);
	/**
	 * @brief Скармливаем датаграмму соединению обычным путём принятой датаграммы
	 *
	 * @note Сокет соединения прежний и путь клиента не менялся: перепривязка не
	 *       нужна, а удержание датаграммы до отчёта о подключении тем более -
	 *       соединение уже подключено и отправку движок разрешает
	 *
	 */
	this->read(i->second, dgram, (size - head - length));
	// Выводим признак того, что сообщение было пересылкой датаграммы
	return true;
}
/**
 * @brief Метод пересылки датаграммы уже переданного соединения работнику
 *
 * @param pid  идентификатор процесса работника
 * @param oid  идентификатор события сессии, принявшей датаграмму
 * @param data данные датаграммы соединения
 * @param size размер датаграммы соединения
 * @return     признак того, что датаграмма переслана работнику
 *
 */
bool awh::unit::QuicServer::relay(const pid_t pid, const event::id_t oid, const uint8_t * data, const size_t size) noexcept {
	// Если данных датаграммы нет - пересылать нечего
	if((data == nullptr) || (size == 0))
		// Выводим результат отказа
		return false;
	// Ищем путь, на котором соединение передано работнику
	auto p = this->_routePath.find(this->_handoverKey);
	/**
	 * @brief Если путь мастеру неизвестен - соединение требует полной передачи
	 *
	 * @note Без пути мастер не отличит миграцию от смены идентификатора, а
	 *       угадывать путь по одному порту нельзя: полная передача заведёт
	 *       соединение у работника заново
	 *
	 */
	if((p == this->_routePath.end()) || p->second.empty())
		// Выводим результат отказа
		return false;
	/**
	 * @brief Сверяем путь отправителя с путём передачи соединения
	 *
	 * @note Смена пути означает миграцию (RFC 9000 §9): подключённый сокет работника
	 *       прежнего клиента больше не узнаёт, и соединение передаётся заново
	 *       с перепривязкой сессии к новому сокету
	 *
	 */
	if(p->second != this->peer(oid))
		// Выводим результат отказа
		return false;
	// Получаем событие обмена, ведущее к работнику
	const event::id_t dest = this->_cluster->channel(pid);
	// Если события обмена с работником нет - работник пал
	if(dest == 0)
		// Выводим результат отказа
		return false;
	// Буфер сообщения пересылки датаграммы
	vector <uint8_t> message;
	// Оставляем в начале сообщения место под опознаватель пересылки
	message.push_back(quic_server_t::HANDOVER_DATAGRAM);
	// Добавляем в сообщение длину идентификатора соединения
	message.push_back(static_cast <uint8_t> (this->_handoverKey.size()));
	// Добавляем в сообщение идентификатор соединения
	message.insert(message.end(), this->_handoverKey.begin(), this->_handoverKey.end());
	// Добавляем в сообщение датаграмму соединения
	message.insert(message.end(), data, data + size);
	/**
	 * @brief Если сообщение пересылки не помещается в буфер чтения движка
	 *
	 * @note Канал кластера принимает сообщение за один приём: большим его движок
	 *       не прочтёт, и пересылка оборвалась бы на середине молча
	 *
	 */
	if(message.size() > quic_server_t::HANDOVER_LIMIT){
		/**
		 * Если включён режим отладки
		 */
		#if defined(DEBUG_MODE)
			// Записываем ошибку в лог
			log::debug("QUIC cluster master process [%d] cannot forward the datagram of %zu bytes to the worker [%d]: the message exceeds the limit of %zu bytes", __PRETTY_FUNCTION__, {pid, oid, data, size}, log::flag_t::CRITICAL, ::getpid(), message.size(), pid, quic_server_t::HANDOVER_LIMIT);
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			log::print("QUIC cluster master process [%d] cannot forward the datagram of %zu bytes to the worker [%d]: the message exceeds the limit of %zu bytes", log::flag_t::CRITICAL, ::getpid(), message.size(), pid, quic_server_t::HANDOVER_LIMIT);
		#endif
		// Выводим результат отказа
		return false;
	}
	// Если отправить пересылку работнику не удалось
	if(this->_cluster->send(pid, message.data(), message.size()) == 0){
		/**
		 * Если включён режим отладки
		 */
		#if defined(DEBUG_MODE)
			// Записываем ошибку в лог
			log::debug("QUIC cluster master process [%d] cannot send the datagram to the worker [%d]", __PRETTY_FUNCTION__, {pid, oid, data, size}, log::flag_t::CRITICAL, ::getpid(), pid);
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			log::print("QUIC cluster master process [%d] cannot send the datagram to the worker [%d]", log::flag_t::CRITICAL, ::getpid(), pid);
		#endif
		// Выводим результат отказа
		return false;
	}
	// Выводим успешный результат: датаграмма переслана работнику
	return true;
}
/**
 * @brief Метод обработки отчёта движка об установленном подключении сессии
 *
 * @param oid идентификатор события сессии
 * @param ok  результат установления подключения
 *
 */
void awh::unit::QuicServer::connected(const event::id_t oid, const bool ok) noexcept {
	// Выполняем поиск сессии соединения
	auto i = this->_sessions.find(oid);
	// Если сессия соединения не найдена
	if(i == this->_sessions.end())
		// Выходим из метода
		return;
	// Если подключение движком не установлено
	if(!ok){
		// Завершаем сессию соединения
		this->erase(oid);
		// Выходим из метода
		return;
	}
	// Если удерживаемой первой датаграммы нет
	if(i->second.carried.empty())
		// Выходим из метода
		return;
	/**
	 * @brief Переносим удерживаемую датаграмму в собственный буфер
	 *
	 * @note Разбор датаграммы вправе завершить сессию, а буфер её к тому мгновению
	 *       уже освобождён: указатель на данные обязан пережить разбор
	 *
	 */
	const string datagram = std::move(i->second.carried);
	// Освобождаем буфер удерживаемой датаграммы сессии
	i->second.carried.clear();
	// Скармливаем соединению первую датаграмму, принятую мастером
	this->read(oid, reinterpret_cast <const uint8_t *> (datagram.data()), datagram.size());
}
/**
 * @brief Метод создания сессии нового соединения
 *
 * @param eid идентификатор события сервера
 * @param oid идентификатор события сессии
 *
 */
void awh::unit::QuicServer::accept([[maybe_unused]] const event::id_t eid, const event::id_t oid) noexcept {
	/**
	 * @brief Если соединение адресовано работнику кластера
	 *
	 * @details Сессию мастер не заводит: соединение обслуживает работник. Отклик
	 *          на чтение ставится затем, что первую датаграмму движок отдаёт
	 *          именно ему, сразу после создания сессии, - она и уедет работнику
	 *          вместе с подключённым сокетом.
	 *
	 */
	if((this->_cluster != nullptr) && this->_cluster->master() && (this->_handoverPid != 0)){
		// Устанавливаем функцию обратного вызова на чтение датаграмм сессии
		engine::io::on(oid, static_cast <engine::callback::read_t> (std::bind(&QuicServer::read, this, _1, _2, _3)));
		// Выходим из метода
		return;
	}
	// Если шаблон контекста безопасности не установлен
	if(this->_ctx == 0){
		/**
		 * Если включён режим отладки
		 */
		#if defined(DEBUG_MODE)
			// Записываем ошибку в лог
			log::debug("QUIC security context is not set", __PRETTY_FUNCTION__, {eid, oid}, log::flag_t::CRITICAL);
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			log::print("QUIC security context is not set", log::flag_t::CRITICAL);
		#endif
		// Уничтожаем сессию соединения
		engine::io::destroy(oid);
		// Выходим из метода
		return;
	}
	// Выполняем создание сессии соединения
	auto ret = this->_sessions.emplace(oid, session_t());
	// Если сессия соединения не создана
	if(!ret.second)
		// Выходим из метода
		return;
	// Получаем созданную сессию соединения
	session_t & session = ret.first->second;
	// Создаём соединение QUIC на шаблоне контекста безопасности
	session.connection = make_unique <quic::connection_t> (
		quic::endpoint_t::SERVER, this->_ctx
	);
	// Устанавливаем локальные транспортные параметры соединения
	session.connection->params(this->_params);
	// Устанавливаем адрес удалённого эндпоинта соединения из движка
	this->endpoint(oid, session.connection.get());
	// Устанавливаем режим проверки адреса клиента через пакет Retry
	session.connection->retry(this->_retry);
	// Устанавливаем режим маркировки исходящих датаграмм соединения
	session.connection->ecn(this->_ecn);
	// Устанавливаем общий ключ вывода токенов сброса без сохранения состояния
	session.connection->resetKey(this->_resetKey);
	// Устанавливаем функцию обратного вызова на чтение датаграмм сессии
	engine::io::on(oid, static_cast <engine::callback::read_t> (std::bind(&QuicServer::read, this, _1, _2, _3)));
	/**
	 * Устанавливаем функцию обратного вызова на инъекцию объединённых данных сессии:
	 * перенаправленные из события-источника байты отправляются туннельным потоком
	 * сессии с шифрованием на уровне соединения, а не пишутся сырьём в сокет (splice)
	 */
	engine::io::on(oid, static_cast <engine::callback::inject_t> (std::bind(&QuicServer::inject, this, _1, _2, _3)));
	/**
	 * Устанавливаем функцию обратного вызова источника исходящих датаграмм сессии:
	 * движок вытягивает датаграммы ровно тогда, когда готов их отправить, поэтому
	 * при переполнении очереди они не теряются, а маркировка ECN накладывается
	 * на ту датаграмму, к которой относится
	 */
	engine::io::on(oid, static_cast <engine::callback::source_t> (std::bind(&QuicServer::source, this, _1, _2, _3)));
	/**
	 * @brief Помечаем исходящие датаграммы соединения поддержкой ECN (RFC 9000 §13.4.1)
	 *
	 * @note Метка ставится соединению, а не сокету сервера: сокет один на все
	 *       соединения, и путь одного из них вправе проверку поддержки не пройти,
	 *       тогда как остальным маркировка сохраняется
	 *
	 */
	this->mark(oid, session, event::ecn_t::ECT0);
}
/**
 * @brief Метод обработки просроченных таймеров соединений
 *
 * @param eid    идентификатор события интервала
 * @param status статус события интервала
 *
 */
void awh::unit::QuicServer::tick([[maybe_unused]] const event::id_t eid, const event::status_t status) noexcept {
	// Если статус события интервала не успешен
	if(status != event::status_t::SUCCESS)
		// Выходим из метода
		return;
	// Текущее время в миллисекундах
	const uint64_t date = fmk::milliseconds();
	// Список сессий с завершёнными соединениями
	vector <event::id_t> completed;
	/**
	 * Перебираем список сессий соединений
	 */
	for(auto & item : this->_sessions){
		// Дедлайн ближайшего события таймера соединения
		const uint64_t timeout = item.second.connection->timeout();
		// Если дедлайн таймера соединения наступил
		if((timeout > 0) && (date >= timeout)){
			// Выполняем обработку просроченных таймеров соединения
			item.second.connection->tick(date);
			// Синхронизируем маршрутизацию по идентификаторам соединения
			this->reroute(item.first, item.second);
			// Отправляем все готовые исходящие датаграммы
			this->flush(item.first, item.second);
		}
		// Если соединение завершено
		if(item.second.connection->state() == quic::connection_t::state_t::DRAINING)
			// Запоминаем сессию с завершённым соединением
			completed.push_back(item.first);
	}
	/**
	 * Перебираем список сессий с завершёнными соединениями
	 */
	for(auto & oid : completed)
		// Выполняем завершение сессии соединения
		this->erase(oid);
}
/**
 * @brief Метод обработки принятой датаграммы сессии
 *
 * @param oid  идентификатор события сессии
 * @param data данные датаграммы
 * @param size размер датаграммы
 *
 */
void awh::unit::QuicServer::read(const event::id_t oid, const uint8_t * data, const size_t size) noexcept {
	// Выполняем поиск сессии соединения
	auto i = this->_sessions.find(oid);
	// Если сессия соединения не найдена
	if(i == this->_sessions.end()){
		/**
		 * @brief Если соединение адресовано работнику кластера
		 *
		 * @details Сессии у мастера под него нет намеренно: разбор идентификатора
		 *          соединения ведёт мастер, а обслуживает соединение работник.
		 *
		 */
		if((this->_cluster != nullptr) && this->_cluster->master() && (this->_handoverPid != 0)){
			/**
			 * @brief Если соединение уже передано работнику и путь клиента прежний - пересылаем датаграмму
			 *
			 * @details Под MS Windows ядро не отдаёт датаграммы подключённому сокету
			 *          работника раньше общего сокета мастера, и мастер получает весь
			 *          обмен соединения. Передавать снимок повторно нельзя: перепривязка
			 *          снимает прежний сокет работника, а удерживаемая датаграмма
			 *          перезаписывается до отчёта движка о подключении и пропадает.
			 *          У датаграммы приложения (RFC 9221) повторной отправки нет.
			 *
			 * @note На POSIX ядро разводит датаграммы четвёркой сокета, и работнику
			 *       они достаются напрямую: мастеру приходит только миграция, путь
			 *       которой сменился, и пересылка здесь не срабатывает
			 *
			 */
			if(this->relay(this->_handoverPid, oid, data, size))
				// Выходим из метода: датаграмма отдана работнику
				return;
			// Если передача соединения работнику не выполнена
			if(!this->handover(this->_handoverPid, oid, data, size)){
				// Ищем маршрут идентификатора соединения
				auto r = this->_route.find(this->_handoverKey);
				/**
				 * @brief Если маршрут не заведён - соединение новое, и мастер берёт его на себя
				 *
				 * @note Заведённый маршрут означает, что соединение уже живёт у
				 *       работника: отбирать его нельзя, клиент получил бы сброс
				 *       вместо продолжения. Датаграмма отбрасывается, клиент
				 *       повторит отправку, а маршрут сохраняется
				 *
				 */
				if(r == this->_route.end()){
					// Запоминаем, что соединение мастер обслуживает сам
					this->_route[this->_handoverKey] = 0;
					// Снимаем путь клиента: соединение мастеру передачей не доставалось
					this->_routePath.erase(this->_handoverKey);
					// Сбрасываем адресата передачи
					this->_handoverPid = 0;
					// Выполняем создание сессии соединения своим ходом
					this->accept(this->_eid, oid);
					// Выполняем обработку принятой датаграммы
					this->read(oid, data, size);
				}
			}
		}
		// Выходим из метода
		return;
	}
	/**
	 * Сессию ведём указателем, а не удерживаемой ссылкой: ниже вызываются функции
	 * обратного вызова приложения ("open", а также "read"/"datagram" внутри process),
	 * и приложение вправе реентрантно завершить соединение прямо в них. Тогда узел
	 * сессии освобождается, и удерживаемая ссылка становится висячей. Поэтому после
	 * каждого колбэка сессию перечитываем из карты и прекращаем обработку, если она снята
	 */
	session_t * session = &i->second;
	/**
	 * Обновляем адрес удалённого эндпоинта перед разбором датаграммы: смена
	 * адреса при установленном соединении означает миграцию на новый путь,
	 * которую соединение отслеживает самостоятельно (RFC 9000 §9)
	 */
	this->endpoint(oid, session->connection.get());
	// Если уведомление о перегрузке пути включено
	if(this->_ecn){
		/**
		 * @brief Извлекаем метаданные принятой датаграммы
		 *
		 * @details Маркировка снимается с заголовка IP-пакета при приёме, а сессии её
		 *          не хранят. Спрашивается она у того события, которое датаграмму
		 *          приняло: у работника кластера это подключённый сокет соединения,
		 *          у мастера и одиночного процесса - общий сокет сервера.
		 *
		 */
		const event::id_t src = (((this->_cluster != nullptr) && !this->_cluster->master()) ? oid : this->_eid);
		// Извлекаем метаданные принятой датаграммы с принявшего её события
		const net::dgram_info_t info = engine::io::trafficInfo(src);
		// Выполняем обработку входящей датаграммы с маркировкой перегрузки
		session->connection->read(data, size, fmk::milliseconds(), info.congestion);
	// Выполняем обработку входящей датаграммы
	} else session->connection->read(data, size, fmk::milliseconds());
	// Синхронизируем маршрутизацию по идентификаторам соединения
	this->reroute(oid, * session);
	// Если соединение установлено и приложение об этом ещё не оповещено
	if(!session->connected && (session->connection->state() == quic::connection_t::state_t::CONNECTED)){
		// Устанавливаем флаг оповещения приложения об установленном соединении
		session->connected = true;
		// Выполняем функцию обратного вызова об установленном соединении
		this->_callback.call <void (const event::id_t)> ("open", oid);
		// Перечитываем сессию: колбэк "open" мог реентрантно снять её
		i = this->_sessions.find(oid);
		// Если сессия соединения снята приложением из функции обратного вызова
		if(i == this->_sessions.end())
			// Выходим из метода
			return;
		// Обновляем указатель на сессию соединения
		session = &i->second;
	}
	// Выполняем выдачу собранных данных потоков приложения
	this->process(oid);
	/**
	 * Перечитываем сессию после выдачи данных: process() вызывает колбэки
	 * "read"/"datagram", в которых приложение могло реентрантно снять сессию
	 */
	i = this->_sessions.find(oid);
	// Если сессия соединения снята приложением из функции обратного вызова
	if(i == this->_sessions.end())
		// Выходим из метода
		return;
	// Обновляем указатель на сессию соединения
	session = &i->second;
	// Отправляем все готовые исходящие датаграммы
	const bool sent = this->flush(oid, * session);
	/**
	 * Если соединение не начато и отправлять в ответ нечего: датаграмма адресована
	 * соединению, о котором сервер ничего не помнит. Отвечаем сбросом без сохранения
	 * состояния и снимаем сессию - иначе удалённый узел будет слать датаграммы
	 * до самого таймаута простоя (RFC 9000 §10.3)
	 */
	if(!sent && (session->connection->state() == quic::connection_t::state_t::NONE)){
		// Отправляем сброс без сохранения состояния
		this->drop(oid, data, size);
		// Выполняем завершение сессии соединения
		this->erase(oid);
		// Выходим из метода
		return;
	}
	// Если удалённый эндпоинт завершил соединение
	if(session->connection->state() == quic::connection_t::state_t::DRAINING)
		// Выполняем завершение сессии соединения
		this->erase(oid);
}
/**
 * @brief Метод перенаправления собранных данных сессии в событие-приёмник объединения
 *
 * @param dest идентификатор события-приёмника объединения данных
 * @param data данные для перенаправления
 *
 */
void awh::unit::QuicServer::forward(const event::id_t dest, string_view data) noexcept {
	// Если данные для перенаправления присутствуют и приёмник задан
	if(!data.empty() && (dest != 0))
		/**
		 * Перенаправляем данные в событие-приёмник через сетевой движок: если приёмник
		 * шифрует данные на уровне соединения (QUIC-сессия, в т.ч. другого юнита) - они
		 * отправляются его туннельным потоком через функцию инъекции, иначе (TCP/UDP) -
		 * пишутся сырыми байтами в сокет
		 */
		engine::io::relay(dest, data.data(), data.size());
}
/**
 * @brief Метод синхронизации маршрутизации соединения
 *
 * @param oid     идентификатор события сессии
 * @param session сессия соединения
 *
 */
void awh::unit::QuicServer::reroute(const event::id_t oid, session_t & session) noexcept {
	// Идентификаторы, введённые в обращение
	vector <quic::cid_t> added;
	// Идентификаторы, выведенные из обращения
	vector <quic::cid_t> removed;
	// Извлекаем изменения набора идентификаторов соединения
	session.connection->issued(added, removed);
	/**
	 * Перебираем введённые в обращение идентификаторы соединения
	 */
	for(auto & cid : added){
		// Привязываем идентификатор соединения к сессии
		engine::multicast::bind(oid, net::origin_key_t(cid.data, static_cast <uint8_t> (cid.size)));
		// Запоминаем зеркало маршрутизации: идентификатор соединения и его сессия
		this->_keys[string(reinterpret_cast <const char *> (cid.data), cid.size)] = oid;
		// Если кластер в работе и соединение обслуживает работник
		if((this->_cluster != nullptr) && !this->_cluster->master()){
			// Буфер объявления нового идентификатора соединения
			vector <uint8_t> notice;
			// Оставляем в начале объявления место под опознаватель ввода
			notice.push_back(quic_server_t::HANDOVER_KEY_ADD);
			// Добавляем в объявление длину введённого идентификатора соединения
			notice.push_back(static_cast <uint8_t> (cid.size));
			// Добавляем в объявление введённый идентификатор соединения
			notice.insert(notice.end(), cid.data, cid.data + cid.size);
			// Извлекаем путь клиента, на котором работник обслуживает соединение
			const string path = this->peer(oid);
			// Добавляем в объявление путь клиента вслед за идентификатором
			notice.insert(notice.end(), path.begin(), path.end());
			/**
			 * @brief Объявляем мастеру новый идентификатор соединения
			 *
			 * @details Клиент вправе сменить адрес и использовать при этом любой из
			 *          выданных ему идентификаторов. Мастер встречает такую
			 *          датаграмму на общем сокете и без объявления отдал бы её
			 *          другому работнику - тому, у которого соединения нет.
			 *          Путь едет вместе с идентификатором, чтобы мастер отличил
			 *          смену идентификатора от миграции и переслал датаграмму
			 *          без повторной передачи снимка.
			 *
			 */
			this->_cluster->send(notice.data(), notice.size());
		}
	}
	/**
	 * Перебираем выведенные из обращения идентификаторы соединения
	 */
	for(auto & cid : removed){
		// Снимаем идентификатор соединения с сессии
		engine::multicast::unbind(oid, net::origin_key_t(cid.data, static_cast <uint8_t> (cid.size)));
		// Выделяем отведённый идентификатор соединения в строку маршрута
		const string key(reinterpret_cast <const char *> (cid.data), cid.size);
		// Снимаем зеркало маршрутизации отведённого идентификатора
		this->_keys.erase(key);
		// Снимаем маршрут отведённого идентификатора у мастера
		this->_route.erase(key);
		// Снимаем путь клиента отведённого идентификатора соединения
		this->_routePath.erase(key);
		// Если кластер в работе и соединение обслуживает работник
		if((this->_cluster != nullptr) && !this->_cluster->master()){
			// Буфер объявления отвода идентификатора соединения
			vector <uint8_t> notice;
			// Оставляем в начале объявления место под опознаватель отвода
			notice.push_back(quic_server_t::HANDOVER_KEY_REMOVE);
			// Добавляем в объявление отводимый идентификатор соединения
			notice.insert(notice.end(), cid.data, cid.data + cid.size);
			// Отправляем мастеру объявление отвода идентификатора соединения
			this->_cluster->send(notice.data(), notice.size());
		}
	}
}
/**
 * @brief Метод обработки событий изменения статуса процесса кластера
 *
 * @param pid    идентификатор процесса
 * @param status новый статус процесса кластера
 *
 */
void awh::unit::QuicServer::status(const pid_t pid, const event::status_t status) noexcept {
	// Выполняем функцию обратного вызова
	this->_callback.call <void (const pid_t, const event::status_t)> ("cluster_state", pid, status);
}
/**
 * @brief Метод получения событий активации/деактивации кластера
 *
 * @param pid   идентификатор процесса
 * @param event флаг события кластера
 *
 */
void awh::unit::QuicServer::cluster(const pid_t pid, const unit::cluster_t::event_t event) noexcept {
	/**
	 * Определяем полученное событие кластера
	 */
	switch(static_cast <uint8_t> (event)){
		// Если событие представляет из себя запуск процесса
		case static_cast <uint8_t> (unit::cluster_t::event_t::START): {
			/**
			 * @brief Если процесс является мастером кластера
			 *
			 * @note Событие запуска приходит мастеру ОДНО на весь кластер и несёт его
			 *       собственный номер процесса, а не номер работника: работники к
			 *       этому мигу порождены все
			 *
			 */
			if(this->_cluster->master()){
				/**
				 * @brief Мастер здесь не делает ничего
				 *
				 * @note Учёт работников идёт не отсюда: порождение работника ещё не
				 *       значит, что канал с ним связан, а отдать снимок в несвязанный
				 *       канал нельзя. Учитывается работник по своей собственной
				 *       просьбе - она и служит признаком связанности канала
				 *
				 */
			// Если процесс является работником кластера
			} else {
				/**
				 * @brief Работник расстаётся с унаследованным общим сокетом мастера
				 *
				 * @details Пересоздание движка после fork описатели не закрывает, и
				 *          общий сокет мастера остался бы у работника открытым:
				 *          датаграммы клиентов доставались бы и ему, а сессий под них
				 *          он не заводит. Уничтожается сокет явно.
				 *
				 * @note На MS Windows работник порождается заново и ничего не наследует:
				 *       своих событий у него к этому мигу нет вовсе, и разбор ниже
				 *       ничего не делает
				 *
				 */
				if(this->_eid != 0){
					// Снимаем функцию обратного вызова определения сессии
					engine::io::on(this->_eid, static_cast <engine::callback::origin_t> (nullptr));
					// Снимаем функцию обратного вызова на создание сессии соединения
					engine::io::on(this->_eid, static_cast <engine::callback::accept_t> (nullptr));
					// Уничтожаем унаследованное событие сервера вместе с его сокетом
					engine::io::destroy(this->_eid);
					// Запоминаем унаследованный идентификатор для приведения обращений фасада к собственному событию (см. actual())
					this->_iid = this->_eid;
					// Сбрасываем идентификатор события сервера
					this->_eid = 0;
				}
				// Если унаследованное событие интервала таймеров соединений создано
				if(this->_tid != 0){
					// Уничтожаем унаследованное событие интервала таймеров соединений
					engine::io::destroy(this->_tid);
					// Сбрасываем идентификатор события интервала таймеров соединений
					this->_tid = 0;
				}
				/**
				 * @brief Заводим собственные события работника
				 *
				 * @note Событие сервера работника остаётся НЕПРИВЯЗАННЫМ: общий сокет
				 *       держит мастер, а работнику достаётся подключённый сокет
				 *       соединения снимком. Событие нужно лишь опознавателем для
				 *       обращений фасада; таймер же работник поднимает всерьёз -
				 *       соединения его живут по своим срокам
				 *
				 */
				if(this->issue(this->_family) != 0){
					// Если адрес прослушивания сохранён
					if(!this->_listenHost.empty())
						// Восстанавливаем адрес прослушивания на событии сервера работника
						engine::address::set(this->_eid, this->_listenType, this->_listenHost);
					// Устанавливаем сохранённый порт прослушивания опознавателем
					engine::source::port::set(this->_eid, this->_listenPort);
					// Если привязка и запуск события интервала таймеров соединений выполнены
					if(engine::io::commit(this->_tid) && engine::io::launch(this->_tid)){
						// Опознаватель просьбы работника о передаче соединений
						const uint8_t request = quic_server_t::HANDOVER_REQUEST;
						/**
						 * @brief Объявляем мастеру о своей готовности обслуживать соединения
						 *
						 * @details Просьба эта и задаёт порядок: к мигу её отправки канал
						 *          заведомо связан - работник только что по нему написал, -
						 *          тогда как мастер о связанности канала узнать не может вовсе.
						 *
						 */
						this->_cluster->send(&request, sizeof(request));
						// Выполняем функцию обратного вызова
						this->_callback.call <void (const event::status_t)> ("server_status", event::status_t::LAUNCHED);
					}
				}
			}
		} break;
		// Если событие представляет из себя остановку процесса
		case static_cast <uint8_t> (unit::cluster_t::event_t::STOP): {
			// Если процесс является работником кластера и его работа запущена
			if(!this->_cluster->master() && this->working())
				// Останавливаем работу основного юнита
				unit_t::stop();
		} break;
	}
	// Выполняем функцию обратного вызова
	this->_callback.call <void (const pid_t, const unit::cluster_t::event_t)>  ("cluster_events", pid, event);
}
/**
 * @brief Метод обработки события получения сообщения от процесса кластера
 *
 * @param pid  идентификатор процесса
 * @param data данные полученного сообщения
 * @param size размер данных полученного сообщения
 *
 */
void awh::unit::QuicServer::message(const pid_t pid, const uint8_t * data, const size_t size) noexcept {
	// Если кластер в работе
	if(this->_cluster != nullptr){
		/**
		 * @brief Если сообщение получено мастером
		 *
		 * @note Служебные сообщения потребителю не предназначались: это разговор
		 *       юнита с юнитом, и наружу он не выходит
		 *
		 */
		if(this->_cluster->master()){
			// Если сообщение оказалось просьбой работника о передаче соединений
			if((data != nullptr) && (size == sizeof(uint8_t)) && (data[0] == quic_server_t::HANDOVER_REQUEST)){
				// Учитываем работника, готового обслуживать соединения
				this->handover(pid);
				// Прерываем выполнение: сообщение потребителю не предназначалось
				return;
			}
			/**
			 * @brief Если сообщение оказалось объявлением идентификатора соединения
			 *
			 * @details Работник объявляет идентификаторы, которые выдал клиенту и
			 *          которые клиент вправе использовать при миграции. Без объявления
			 *          мастер встретил бы датаграмму миграции с незнакомым
			 *          идентификатором и отдал бы её другому работнику - тому, у
			 *          которого соединения нет.
			 *
			 */
			if((data != nullptr) && (size > sizeof(uint8_t)) &&
			  ((data[0] == quic_server_t::HANDOVER_KEY_ADD) || (data[0] == quic_server_t::HANDOVER_KEY_REMOVE))){
				// Если работник объявил новый идентификатор соединения
				if(data[0] == quic_server_t::HANDOVER_KEY_ADD){
					// Длина заголовка объявления: опознаватель и длина идентификатора
					const size_t head = (sizeof(uint8_t) + sizeof(uint8_t));
					// Выделяем длину объявленного идентификатора соединения
					const size_t length = static_cast <size_t> (data[1]);
					// Если объявление не вмещает идентификатор соединения заявленной длины
					if((size - head) < length)
						// Прерываем выполнение: объявление повреждено
						return;
					// Выделяем идентификатор соединения из объявления
					const string key(reinterpret_cast <const char *> (data + head), length);
					// Выделяем путь клиента, на котором работник обслуживает соединение
					const string path(reinterpret_cast <const char *> (data + head + length), (size - head - length));
					// Запоминаем маршрут: соединение обслуживает объявивший работник
					this->_route[key] = pid;
					/**
					 * @brief Запоминаем путь клиента объявленного идентификатора соединения
					 *
					 * @note Путь сообщил работник, обслуживающий соединение: смена
					 *       идентификатора не есть миграция, и датаграмму по нему
					 *       мастер перешлёт без повторной передачи снимка
					 *
					 */
					this->_routePath[key] = path;
				// Если работник объявил отвод идентификатора соединения
				} else {
					// Выделяем идентификатор соединения из объявления
					const string key(reinterpret_cast <const char *> (data + sizeof(uint8_t)), size - sizeof(uint8_t));
					// Снимаем маршрут отведённого идентификатора соединения
					this->_route.erase(key);
					// Снимаем путь клиента отведённого идентификатора соединения
					this->_routePath.erase(key);
				}
				// Прерываем выполнение: сообщение потребителю не предназначалось
				return;
			}
		// Если работник получил переданное мастером соединение
		} else if(this->handover(data, size))
			// Прерываем выполнение: сообщение потребителю не предназначалось
			return;
		// Если работник получил пересланную мастером датаграмму соединения
		else if(this->relay(data, size))
			// Прерываем выполнение: сообщение потребителю не предназначалось
			return;
	}
	// Выполняем функцию обратного вызова
	this->_callback.call <void (const pid_t, const uint8_t *, const size_t)> ("cluster_message", pid, data, size);
}
/**
 * @brief Метод обработки события доступности/недоступности очереди исходящих сообщений кластера
 *
 * @param pid    идентификатор процесса
 * @param status статус доступности очереди
 * @param size   размер доступных данных очереди
 *
 */
void awh::unit::QuicServer::available(const pid_t pid, const event::status_t status, const size_t size) noexcept {
	// Выполняем функцию обратного вызова
	this->_callback.call <void (const pid_t, const event::status_t, const size_t)> ("cluster_available", pid, status, size);
}
/**
 * @brief Метод обработки событий ошибок кластера
 *
 * @param pid         идентификатор процесса
 * @param error       тип ошибки
 * @param description описание ошибки
 *
 */
void awh::unit::QuicServer::error(const pid_t pid, const event::error_t error, const string & description) noexcept {
	// Выполняем функцию обратного вызова
	this->_callback.call <void (const pid_t, const event::error_t, const string &)> ("cluster_error", pid, error, description);
}
/**
 * @brief Метод установки адреса удалённого эндпоинта соединению из движка
 *
 * @note Узел-приёмник хранит адрес собеседника в адресе источника, а клиентский
 *       узел работника кластера - в цели подключения: его адрес источника свой
 *
 * @param oid        идентификатор события сессии
 * @param connection соединение, которому устанавливается адрес эндпоинта
 *
 */
void awh::unit::QuicServer::endpoint(const event::id_t oid, quic::connection_t * connection) const noexcept {
	// Структура сетевого адреса удалённого эндпоинта сессии
	unique_ptr <net::addr_t> addr = nullptr;
	// Порт удалённого эндпоинта сессии
	uint16_t port = 0;
	// Если сессия работника кластера заведена клиентским узлом движка
	if(engine::io::node(oid) == event::node_t::CLIENT){
		// Извлекаем структуру адреса удалённого эндпоинта из цели подключения узла
		engine::target::get(oid, addr);
		// Извлекаем порт удалённого эндпоинта из цели подключения узла
		port = engine::target::port::get(oid);
	// Если сессия заведена узлом-приёмником движка
	} else {
		// Если структура адреса удалённого эндпоинта в семействе IPv4 не получена
		if(!engine::address::get(oid, event::address_t::IPV4, addr))
			// Извлекаем структуру адреса удалённого эндпоинта в семействе IPv6
			engine::address::get(oid, event::address_t::IPV6, addr);
		// Извлекаем порт удалённого эндпоинта из адреса источника узла
		port = engine::source::port::get(oid);
	}
	// Устанавливаем адрес удалённого эндпоинта соединению штатной структурой адреса и портом
	connection->address(addr.get(), port);
}
/**
 * @brief Метод отправки объединённых данных в туннельный поток сессии
 *
 * @param oid  идентификатор события сессии-приёмника
 * @param data данные для отправки в туннельный поток
 * @param size размер данных для отправки
 * @return     результат постановки данных в очередь отправки
 *
 */
bool awh::unit::QuicServer::inject(const event::id_t oid, const uint8_t * data, const size_t size) noexcept {
	// Если данные для отправки отсутствуют
	if((data == nullptr) || (size == 0))
		// Возвращаем значение по умолчанию
		return false;
	// Выполняем поиск сессии-приёмника объединения данных
	auto i = this->_sessions.find(oid);
	// Если сессия-приёмник не найдена либо соединение не создано
	if((i == this->_sessions.end()) || (i->second.connection == nullptr))
		// Возвращаем значение по умолчанию
		return false;
	// Если туннельный поток входящих объединённых данных ещё не открыт
	if(i->second.tunnel == quic::connection_t::INVALID_STREAM)
		// Открываем двунаправленный туннельный поток приложения (RFC 9000 §2.1)
		i->second.tunnel = i->second.connection->open(false);
	// Если туннельный поток открыть не удалось
	if(i->second.tunnel == quic::connection_t::INVALID_STREAM)
		// Возвращаем значение по умолчанию
		return false;
	// Если постановка байт в очередь отправки туннельного потока не выполнена
	if(i->second.connection->send(i->second.tunnel, string_view(reinterpret_cast <const char *> (data), size), false) < size)
		// Возвращаем значение по умолчанию
		return false;
	// Отправляем готовые исходящие датаграммы удалённому эндпоинту
	this->flush(oid, i->second);
	// Возвращаем положительный результат
	return true;
}
/**
 * @brief Метод отправки сброса без сохранения состояния (RFC 9000 §10.3)
 *
 * @param oid  идентификатор события сессии
 * @param data данные вызвавшей сброс датаграммы
 * @param size размер вызвавшей сброс датаграммы
 * @return     результат отправки
 *
 */
bool awh::unit::QuicServer::drop(const event::id_t oid, const uint8_t * data, const size_t size) noexcept {
	// Ключ маршрутизации вызвавшей сброс датаграммы
	net::origin_key_t key;
	// Если идентификатор соединения получателя извлечь не удалось
	if(!quic::route(data, size, quic::connection_t::LOCAL_CID_SIZE, key))
		// Выводим отрицательный результат
		return false;
	// Идентификатор соединения получателя вызвавшей сброс датаграммы
	quic::cid_t cid;
	// Если размер извлечённого идентификатора превышает предел QUIC
	if(key.size > quic::proto::MAX_CID_SIZE)
		// Выводим отрицательный результат
		return false;
	// Устанавливаем длину идентификатора соединения получателя
	cid.size = key.size;
	// Копируем данные идентификатора соединения получателя
	::memcpy(cid.data, key.data, key.size);
	// Буфер пакета сброса без сохранения состояния
	string reset = "";
	// Если сборка пакета сброса не выполнена
	if(!quic::reset(reset, this->_resetKey, cid, size))
		// Выводим отрицательный результат
		return false;
	// Если отправка пакета сброса удалённому эндпоинту не выполнена
	if(engine::io::send(oid, reset.data(), reset.size()) == 0)
		// Выводим отрицательный результат
		return false;
	// Записываем в лог сообщение об отправке сброса без сохранения состояния
	log::print("QUIC stateless reset is sent: ID=%u", log::flag_t::INFO, oid);
	// Выводим положительный результат
	return true;
}
/**
 * @brief Метод применения маркировки соединения к сокету события сервера
 *
 * @param marking требуемая маркировка исходящих датаграмм
 *
 */
void awh::unit::QuicServer::mark(const event::id_t oid, session_t & session, const event::ecn_t marking) noexcept {
	// Если уведомление о перегрузке пути отключено либо маркировка уже установлена
	if(!this->_ecn || (marking == session.marking))
		// Выходим из метода - смена маркировки не требуется
		return;
	/**
	 * @brief Если смена маркировки исходящих датаграмм соединения не выполнена
	 *
	 * @note Маркировка ставится событию соединения: движок кладёт её в управляющие
	 *       данные отправки, и она едет вместе со своей датаграммой. Метка сокета
	 *       здесь не годится - сокет один на все соединения сервера
	 *
	 */
	if(!engine::qos::ecn::set(oid, this->_family, marking)){
		/**
		 * Если включён режим отладки
		 */
		#if defined(DEBUG_MODE)
			// Записываем предупреждение в лог
			log::debug("QUIC outgoing datagrams marking is not applied: ID=%u", __PRETTY_FUNCTION__, {oid, static_cast <uint16_t> (marking)}, log::flag_t::WARNING, oid);
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем предупреждение в лог
			log::print("QUIC outgoing datagrams marking is not applied: ID=%u", log::flag_t::WARNING, oid);
		#endif
		// Выходим из метода - установленная маркировка неизвестна
		return;
	}
	// Запоминаем установленную маркировку соединения
	session.marking = marking;
}
/**
 * @brief Метод определения сессии принятой датаграммы (RFC 9000 §17.2)
 *
 * @param eid  идентификатор события сервера
 * @param data данные датаграммы
 * @param size размер датаграммы
 * @param key  выводимый ключ сессии
 * @return     результат определения сессии
 *
 */
bool awh::unit::QuicServer::origin([[maybe_unused]] const event::id_t eid, const uint8_t * data, const size_t size, net::origin_key_t & key) noexcept {
	// Если идентификатор соединения получателя из датаграммы не извлечён
	if(!quic::route(data, size, quic::connection_t::LOCAL_CID_SIZE, key))
		// Выводим отрицательный результат: датаграмма протоколу не принадлежит
		return false;
	/**
	 * @brief Если разбор соединений ведёт мастер кластера
	 *
	 * @details Общий сокет один на весь кластер, и разбирает его мастер: ядро
	 *          разводит датаграммы по четвёрке сокета, а соединение QUIC живёт по
	 *          идентификатору, и клиент, сменивший адрес, достался бы работнику
	 *          без сессии. Работнику отдаётся подключённый сокет соединения - он
	 *          и держит клиента при нём, а миграция возвращает датаграмму мастеру.
	 *
	 * @note Передача здесь не выполняется: отклик определения сессии адреса
	 *       отправителя не несёт, а без него подключённый сокет не завести.
	 *       Решение запоминается адресатом, а передачу делает создание сессии
	 *       с первой датаграммой
	 *
	 */
	if((this->_cluster != nullptr) && this->_cluster->master()){
		// Выделяем идентификатор соединения в строку маршрута
		const string route(reinterpret_cast <const char *> (key.data), static_cast <size_t> (key.size));
		// Ищем маршрут идентификатора соединения
		auto i = this->_route.find(route);
		// Запоминаем идентификатор соединения для передачи
		this->_handoverKey = route;
		/**
		 * @brief Если соединение уже обслуживается, а датаграмма пришла на общий сокет
		 *
		 * @details Так бывает при миграции пути (RFC 9000 §9): клиент сменил адрес,
		 *          подключённый сокет работника его больше не узнаёт, и датаграмма
		 *          достаётся общему сокету мастера. Маршрут при этом уже заведён,
		 *          и работник у соединения прежний.
		 *
		 */
		if(i != this->_route.end())
			// Запоминаем адресата передачи: нуль - соединение обслуживает сам мастер
			this->_handoverPid = i->second;
		// Если работников, готовых обслуживать соединения, нет вовсе
		else if(this->_handoverWorkers.empty()){
			// Запоминаем, что соединение мастер обслуживает сам
			this->_route[route] = 0;
			// Сбрасываем адресата передачи
			this->_handoverPid = 0;
		// Если соединение новое и работники есть
		} else {
			// Получаем работника, которому соединение достаётся по очереди
			const pid_t pid = this->_handoverWorkers.at(this->_handoverNext % this->_handoverWorkers.size());
			// Передвигаем очередь на работника следующего
			this->_handoverNext++;
			/**
			 * @brief Запоминаем адресата передачи
			 *
			 * @note Маршрут заводится только по успешной передаче: неудачную
			 *       передачу нового соединения мастер берёт на себя, а неудачную
			 *       передачу миграции отбрасывает, не отбирая соединение у работника
			 *
			 */
			this->_handoverPid = pid;
		}
	}
	// Выводим положительный результат: сессия заводится своим ходом
	return true;
}
/**
 * @brief Метод установки названия кластера
 *
 * @param name название кластера для установки
 *
 */
void awh::unit::QuicServer::clusterName(string_view name) noexcept {
	// Устанавливаем название кластера
	this->_clusterParams.name = name;
	// Если кластер инициализирован
	if(this->_cluster != nullptr)
		// Устанавливаем название кластера
		this->_cluster->name(this->_clusterParams.name);
}
/**
 * @brief Метод получения семейства кластера
 *
 * @return семейство к которому принадлежит кластер (MASTER или CHILDREN)
 *
 */
awh::unit::cluster_t::family_t awh::unit::QuicServer::clusterFamily() const noexcept {
	// Если кластер инициализирован
	if(this->_cluster != nullptr){
		// Если текущий процесс кластера является родительским
		if(this->_cluster->master())
			// Устанавливаем результат работы функции
			return cluster_t::family_t::MASTER;
		// Если текущий процесс кластера является дочерним
		return cluster_t::family_t::CHILDREN;
	}
	// Возвращаем значение по умолчанию
	return cluster_t::family_t::NONE;
}
/**
 * @brief Метод получения максимального количества процессов
 *
 * @return максимальное количество процессов
 *
 */
uint16_t awh::unit::QuicServer::clusterCount() const noexcept {
	// Если кластер инициализирован
	if(this->_cluster != nullptr)
		// Возвращаем максимальное количество процессов кластера
		return this->_cluster->count();
	// Возвращаем значение по умолчанию
	return 0;
}
/**
 * @brief Метод установки максимального количества процессов
 *
 * @param count максимальное количество процессов
 *
 */
void awh::unit::QuicServer::clusterCount(const uint16_t count) noexcept {
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		// Локальная переменная для работы с количеством процессов
		this->_clusterParams.count = count;
		// Если количество процессов передано пустое
		if(this->_clusterParams.count == 0){
			// Устанавливаем количество доступных ядер в системе
			this->_clusterParams.count = static_cast <uint16_t> (thread::hardware_concurrency());
			// Если количество доступных воркеров больше одного, уменьшаем пополам
			if(this->_clusterParams.count > 1)
				// Уменьшаем количество воркеров в два раза
				this->_clusterParams.count /= 2;
		}
		// Если кластер инициализирован
		if(this->_cluster != nullptr)
			// Устанавливаем максимальное количество процессов кластера
			this->_cluster->count(this->_clusterParams.count);
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		/**
		 * Если включён режим отладки
		 */
		#if defined(DEBUG_MODE)
			// Записываем ошибку в лог
			log::debug("%s", __PRETTY_FUNCTION__, {count}, log::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			log::print("%s", log::flag_t::CRITICAL, error.what());
		#endif
	}
}
/**
 * @brief Метод получения режима активации кластера
 *
 * @return режим активации кластера
 *
 */
awh::event::mode_t awh::unit::QuicServer::clusterMode() const noexcept {
	// Извлекаем режим активации кластера
	return this->_clusterParams.mode;
}
/**
 * @brief Метод установки режима работы кластера
 *
 * @param mode режим активации/деактивации кластера
 *
 */
void awh::unit::QuicServer::clusterMode(const event::mode_t mode) noexcept {
	// Устанавливаем режим активации кластера
	this->_clusterParams.mode = mode;
}
/**
 * @brief Метод получения списка дочерних процессов
 *
 * @return список дочерних процессов
 *
 */
unordered_set <pid_t> awh::unit::QuicServer::clusterWorkers() const noexcept {
	// Если кластер инициализирован
	if(this->_cluster != nullptr)
		// Возвращаем количество активных воркеров кластера
		return this->_cluster->workers();
	// Возвращаем значение по умолчанию
	return {};
}
/**
 * @brief Метод установки флага автоматического возрождения процессов
 *
 * @param mode флаг возрождения процессов
 *
 */
void awh::unit::QuicServer::clusterRebirth(const bool mode) noexcept {
	// Устанавливаем флаг автоматического возрождения процессов
	this->_clusterParams.rebirth = mode;
	// Если кластер инициализирован
	if(this->_cluster != nullptr)
		// Устанавливаем флаг автоматического возрождения процессов
		this->_cluster->rebirth(this->_clusterParams.rebirth);
}
/**
 * @brief Метод установки параметров защиты от цикла перезапусков процессов кластера
 *
 * @param limit  максимальное число подряд идущих быстрых падений до остановки кластера (0 — без ограничения)
 * @param window временное окно «быстрого» (раннего) падения процесса в миллисекундах
 *
 */
void awh::unit::QuicServer::clusterRebirthLimit(const uint16_t limit, const uint64_t window) noexcept {
	// Устанавливаем максимальное число подряд идущих быстрых падений процессов
	this->_clusterParams.restartLimit = limit;
	// Устанавливаем временное окно «быстрого» падения процесса
	this->_clusterParams.restartWindow = window;
	// Если кластер инициализирован
	if(this->_cluster != nullptr)
		// Устанавливаем параметры защиты от цикла перезапусков процессов
		this->_cluster->rebirthLimit(this->_clusterParams.restartLimit, this->_clusterParams.restartWindow);
}
/**
 * @brief Метод получения типа протокола передачи данных между воркерами
 *
 * @return тип протокола передачи данных между воркерами
 *
 */
awh::event::type_t awh::unit::QuicServer::clusterGetTypeEventMessage() const noexcept {
	// Если кластер инициализирован
	if(this->_cluster != nullptr)
		// Выполняем получение типа протокола передачи данных между воркерами
		return this->_cluster->getTypeEventMessage();
	// Возвращаем значение по умолчанию
	return event::type_t::NONE;
}
/**
 * @brief Метод установки типа протокола передачи данных между воркерами
 *
 * @param type тип протокола передачи данных между воркерами для установки
 *
 */
void awh::unit::QuicServer::clusterSetTypeEventMessage(const event::type_t type) noexcept {
	// Если кластер инициализирован
	if(this->_cluster != nullptr)
		// Выполняем установку типа протокола передачи данных между воркерами
		this->_cluster->setTypeEventMessage(type);
}
/**
 * @brief Метод отправки сообщения всем дочерним процессам
 *
 * @param buffer бинарный буфер для отправки сообщения
 * @param size   размер бинарного буфера для отправки сообщения
 * @return       количество байт отправленного сообщения
 *
 */
size_t awh::unit::QuicServer::clusterBroadcast(const void * buffer, const size_t size) noexcept {
	// Если кластер инициализирован
	if(this->_cluster != nullptr)
		// Выполняем отправку сообщения всем дочерним процессам
		return this->_cluster->broadcast(buffer, size);
	// Возвращаем значение по умолчанию
	return 0;
}
/**
 * @brief Метод отправки сообщения родительскому процессу
 *
 * @param buffer бинарный буфер для отправки сообщения
 * @param size   размер бинарного буфера для отправки сообщения
 * @return       количество байт отправленного сообщения
 *
 */
size_t awh::unit::QuicServer::clusterSend(const void * buffer, const size_t size) noexcept {
	// Если кластер инициализирован
	if(this->_cluster != nullptr)
		// Выполняем отправку сообщения родительскому процессу
		return this->_cluster->send(buffer, size);
	// Возвращаем значение по умолчанию
	return 0;
}
/**
 * @brief Метод отправки сообщения дочернему процессу
 *
 * @param pid    идентификатор процесса для получения сообщения
 * @param buffer бинарный буфер для отправки сообщения
 * @param size   размер бинарного буфера для отправки сообщения
 * @return       количество байт отправленного сообщения
 *
 */
size_t awh::unit::QuicServer::clusterSend(const pid_t pid, const void * buffer, const size_t size) noexcept {
	// Если кластер инициализирован
	if(this->_cluster != nullptr)
		// Выполняем отправку сообщения дочернему процессу
		return this->_cluster->send(pid, buffer, size);
	// Возвращаем значение по умолчанию
	return 0;
}
/**
 * @brief Метод получения размера буфера события
 *
 * @param pid    идентификатор процесса
 * @param action тип действия события
 * @return       размер буфера события
 *
 */
size_t awh::unit::QuicServer::clusterGetBufferSize(const pid_t pid, const event::action_t action) const noexcept {
	// Если кластер инициализирован
	if(this->_cluster != nullptr)
		// Выполняем получение размера буфера кластера для события кластера
		return this->_cluster->getBufferSize(pid, action);
	// Возвращаем значение по умолчанию
	return 0;
}
/**
 * @brief Метод установки размера буфера события
 *
 * @param pid    идентификатор процесса
 * @param action тип действия события
 * @param size   размер буфера события
 * @return       результат выполнения установки
 *
 */
bool awh::unit::QuicServer::clusterSetBufferSize(const pid_t pid, const event::action_t action, const size_t size) noexcept {
	// Если кластер инициализирован
	if(this->_cluster != nullptr)
		// Выполняем установку размера буфера кластера для события кластера
		return this->_cluster->setBufferSize(pid, action, size);
	// Возвращаем значение по умолчанию
	return false;
}
/**
 * @brief Метод установки уведомления о перегрузке пути (RFC 9000 §13.4)
 *
 * @param mode режим уведомления о перегрузке пути
 *
 */
void awh::unit::QuicServer::ecn(const bool mode) noexcept {
	// Устанавливаем режим уведомления о перегрузке пути
	this->_ecn = mode;
}
/**
 * @brief Метод установки проверки адреса клиента через пакет Retry (RFC 9000 §8.1.2)
 *
 * @param mode режим проверки адреса клиента
 *
 */
void awh::unit::QuicServer::retry(const bool mode) noexcept {
	// Устанавливаем режим проверки адреса клиента
	this->_retry = mode;
}
/**
 * @brief Метод установки общего ключа вывода токенов сброса (RFC 9000 §10.3.2)
 *
 * @param key общий ключ вывода токенов сброса
 *
 */
void awh::unit::QuicServer::resetKey(string_view key) noexcept {
	// Устанавливаем общий ключ вывода токенов сброса
	this->_resetKey.assign(key.begin(), key.end());
}
/**
 * @brief Метод приостановки работы события сервера
 *
 * @param eid идентификатор события сервера
 * @return    результат выполнения приостановки работы
 *
 */
bool awh::unit::QuicServer::pause(const event::id_t eid) noexcept {
	// Если событие сервера является актуальным
	if(this->isActual(eid))
		// Выполняем приостановку работы события сервера
		return engine::io::pause(this->_eid);
	// Возвращаем значение по умолчанию
	return false;
}
/**
 * @brief Метод возобновления работы события сервера
 *
 * @param eid идентификатор события сервера
 * @return    результат выполнения возобновления работы
 *
 */
bool awh::unit::QuicServer::resume(const event::id_t eid) noexcept {
	// Если событие сервера является актуальным
	if(this->isActual(eid))
		// Выполняем возобновление работы события сервера
		return engine::io::resume(this->_eid);
	// Возвращаем значение по умолчанию
	return false;
}
/**
 * @brief Метод фиксации настроек события сервера
 *
 * @param eid идентификатор события сервера
 * @return    результат выполнения фиксации
 *
 */
bool awh::unit::QuicServer::commit(const event::id_t eid) noexcept {
	// Если событие сервера является актуальным
	if(this->isActual(eid)){
		/**
		 * @brief Если фиксацию заказывает работник кластера
		 *
		 * @details Общий сокет держит мастер, а работнику достаётся подключённый сокет
		 *          соединения снимком. Привяжи работник свой общий сокет, он отобрал
		 *          бы датаграммы у мастера, а сессий под них не завёл бы.
		 *
		 */
		if((this->_cluster != nullptr) && !this->_cluster->master())
			// Выполняем фиксацию настроек события интервала таймеров соединений
			return engine::io::commit(this->_tid);
		// Выполняем фиксацию настроек события сервера и события интервала таймеров соединений
		return (engine::io::commit(this->_eid) && engine::io::commit(this->_tid));
	}
	// Возвращаем значение по умолчанию
	return false;
}
/**
 * @brief Метод запуска работы события сервера
 *
 * @param eid идентификатор события сервера
 * @return    результат выполнения запуска
 *
 */
bool awh::unit::QuicServer::launch(const event::id_t eid) noexcept {
	// Если событие сервера не актуально
	if(!this->isActual(eid))
		// Возвращаем значение по умолчанию
		return false;
	/**
	 * @brief Если запуск заказывает работник кластера
	 *
	 * @note Событие сервера работника не привязано и не запускается: приём ведут
	 *       подключённые сокеты соединений, а от события сервера работнику нужен
	 *       лишь опознаватель для обращений фасада
	 *
	 */
	if((this->_cluster != nullptr) && !this->_cluster->master())
		// Выполняем запуск работы события интервала таймеров соединений
		return engine::io::launch(this->_tid);
	// Если запуск события сервера и события интервала таймеров соединений не выполнен
	if(!engine::io::launch(this->_eid) || !engine::io::launch(this->_tid))
		// Возвращаем значение по умолчанию
		return false;
	/**
	 * @brief Возвращаем положительный результат
	 *
	 * @details Маркировать здесь нечего: метка принадлежит соединению, а не сокету сервера,
	 *          и ставится каждому соединению при заведении его сессии (RFC 9000 §13.4.1).
	 *
	 */ 
	return true;
}
/**
 * @brief Метод уничтожения события сервера
 *
 * @param eid идентификатор события для уничтожения
 *
 */
void awh::unit::QuicServer::destroy(const event::id_t eid) noexcept {
	// Если событие сервера не актуально
	if(!this->isActual(eid))
		// Выходим из метода
		return;
	/**
	 * Если идентификатор адресует отдельную сессию, а не событие сервера: завершаем
	 * только её. Иначе уничтожение одной сессии (в т.ч. реентрантно из функции
	 * обратного вызова) сносило бы весь сервер со всеми прочими сессиями, событием
	 * приёма и таймером - для отдельной сессии это неверно (RFC 9000 §10)
	 */
	if(this->_sessions.find(eid) != this->_sessions.end()){
		// Завершаем сессию соединения
		this->erase(eid);
		// Выходим из метода
		return;
	}
	// Завершаем все сессии соединений сервера
	if(!this->_sessions.empty()){
		// Копируем список идентификаторов сессий для безопасного удаления во время итерации
		vector <event::id_t> sessions;
		// Резервируем память под список идентификаторов сессий
		sessions.reserve(this->_sessions.size());
		/**
		 * Формируем список идентификаторов сессий соединений
		 */
		for(const auto & item : this->_sessions)
			// Добавляем идентификатор сессии соединения в список
			sessions.push_back(item.first);
		/**
		 * Завершаем все сессии соединений
		 */
		for(const auto & oid : sessions)
			// Завершаем сессию соединения
			this->erase(oid);
	}
	/**
	 * Если событие сервера уже уничтожено реентрантно: приложение вправе из функции
	 * обратного вызова "close" (вызванной erase выше) уничтожить сервер целиком, тогда
	 * _eid уже обнулён вложенным вызовом, и дальнейший снос по нулевому идентификатору
	 * лишь повторял бы работу вхолостую
	 */
	if(this->_eid == 0)
		// Выходим из метода
		return;
	// Если событие интервала таймеров соединений создано
	if(this->_tid != 0){
		// Уничтожаем событие интервала таймеров соединений
		engine::io::destroy(this->_tid);
		// Сбрасываем идентификатор события интервала таймеров соединений
		this->_tid = 0;
	}
	// Снимаем функцию обратного вызова определения сессии
	engine::io::on(this->_eid, static_cast <engine::callback::origin_t> (nullptr));
	// Снимаем функцию обратного вызова на создание сессии соединения
	engine::io::on(this->_eid, static_cast <engine::callback::accept_t> (nullptr));
	// Уничтожаем событие сервера
	engine::io::destroy(this->_eid);
	// Сбрасываем идентификатор события сервера
	this->_eid = 0;
}
/**
 * @brief Метод прослушивания порта сервера для приёма входящих соединений
 *
 * @param eid идентификатор события сервера
 * @param max максимальный размер очереди ожидания соединений
 * @return    результат выполнения прослушивания
 *
 */
bool awh::unit::QuicServer::listen(const event::id_t eid, const uint32_t max) noexcept {
	// Если событие сервера является актуальным
	if(this->isActual(eid)){
		// Кэшируем размер очереди ожидания для пересоздания сокета в дочернем процессе кластера
		if(eid == this->_eid)
			// Сохраняем максимальный размер очереди ожидания соединений
			this->_backlog = max;
		/**
		 * Транспорт QUIC работает поверх дейтаграммного UDP-сокета: приём входящих
		 * соединений ведётся приёмом датаграмм по событию сервера и демультиплексацией
		 * по идентификатору соединения, поэтому отдельного перевода сокета в режим
		 * прослушивания (доступного лишь потоковым транспортам STREAM/SEQPACKET) не
		 * требуется - размер очереди лишь сохраняется
		 */
		return true;
	}
	// Возвращаем значение по умолчанию
	return false;
}
/**
 * @brief Метод установки шаблона контекста безопасности соединений
 *
 * @param ctx идентификатор шаблона контекста безопасности
 *
 */
void awh::unit::QuicServer::context(const tls::coder::id_t ctx) noexcept {
	// Устанавливаем идентификатор шаблона контекста безопасности
	this->_ctx = ctx;
}
/**
 * @brief Метод установки контекста события клиента
 *
 * @param eid идентификатор события клиента
 * @param ctx контекст события клиента
 * @return    результат выполнения установки
 *
 */
bool awh::unit::QuicServer::setContext([[maybe_unused]] const event::id_t eid, [[maybe_unused]] void * ctx) noexcept {
	// Для транспорта QUIC контекст сессии ведётся самим модулем по идентификатору события сессии
	return false;
}
/**
 * @brief Метод получения порта сервера
 *
 * @param eid идентификатор события сервера
 * @return    порт сервера
 *
 */
uint16_t awh::unit::QuicServer::getPort(const event::id_t eid) const noexcept {
	// Если событие актуально (для события сервера - порт прослушивания, для сессии - порт пира)
	if(this->isActual(eid))
		// Выполняем получение порта для актуального события (унаследованный кластером идентификатор приводится к собственному)
		return engine::source::port::get(this->actual(eid));
	// Возвращаем значение по умолчанию
	return 0;
}
/**
 * @brief Метод установки порта сервера
 *
 * @param eid  идентификатор события сервера
 * @param port порт сервера для установки
 * @return     результат выполнения установки
 *
 */
bool awh::unit::QuicServer::setPort(const event::id_t eid, const uint16_t port) noexcept {
	// Если событие сервера является актуальным
	if(this->isActual(eid)){
		// Запоминаем порт: к нему мастер привязывает передаваемый работнику сокет, а работник восстанавливает его на своём событии
		if(eid == this->_eid)
			// Сохраняем порт прослушивания сервера
			this->_listenPort = port;
		// Выполняем установку порта сервера для события сервера
		return engine::source::port::set(this->_eid, port);
	}
	// Возвращаем значение по умолчанию
	return false;
}
/**
 * @brief Метод получения сетевого интерфейса сервера
 *
 * @param eid идентификатор события сервера
 * @return    сетевой интерфейс сервера
 *
 */
string awh::unit::QuicServer::getIface(const event::id_t eid) const noexcept {
	// Если событие сервера является актуальным
	if(this->isActual(eid))
		// Выполняем получение сетевого интерфейса для события сервера
		return engine::iface::get(this->_eid);
	// Возвращаем значение по умолчанию
	return "";
}
/**
 * @brief Метод установки сетевого интерфейса сервера
 *
 * @param eid  идентификатор события сервера
 * @param name имя сетевого интерфейса для установки
 * @return     результат выполнения установки
 *
 */
bool awh::unit::QuicServer::setIface(const event::id_t eid, string_view name) noexcept {
	// Если событие сервера является актуальным
	if(this->isActual(eid))
		// Выполняем установку сетевого интерфейса для события сервера
		return engine::iface::set(this->_eid, name);
	// Возвращаем значение по умолчанию
	return false;
}
/**
 * @brief Метод получения семейства адресов события сервера
 *
 * @param eid идентификатор события сервера
 * @return    семейство адресов события сервера
 *
 */
awh::event::family_t awh::unit::QuicServer::family(const event::id_t eid) const noexcept {
	// Приводим унаследованный дочерним процессом кластера идентификатор к собственному
	return this->unit_t::family(this->actual(eid));
}
/**
 * @brief Метод установки локальных транспортных параметров соединений (RFC 9000 §7.4)
 *
 * @param params локальные транспортные параметры
 *
 */
void awh::unit::QuicServer::params(const quic::params::params_t & params) noexcept {
	// Устанавливаем локальные транспортные параметры соединений
	this->_params = params;
}
/**
 * @brief Метод проверки актуальности события сервера
 *
 * @param eid идентификатор события
 * @return    результат проверки актуальности события
 *
 */
bool awh::unit::QuicServer::isActual(const event::id_t eid) const noexcept {
	// Событие актуально, если это событие сервера (в т.ч. унаследованное фасадом) либо активная сессия соединения
	return (((this->_eid != 0) && (this->actual(eid) == this->_eid)) || (this->_sessions.find(eid) != this->_sessions.end()));
}
/**
 * @brief Метод приведения переданного фасадом идентификатора события сервера к актуальному
 *
 * @param eid идентификатор события, переданный фасадом
 * @return    актуальный идентификатор события сервера
 *
 */
awh::event::id_t awh::unit::QuicServer::actual(const event::id_t eid) const noexcept {
	/**
	 * В режиме кластера дочерний процесс пересоздаёт собственное событие сервера с
	 * новым идентификатором, а фасад продолжает обращаться к унаследованному до fork
	 * идентификатору: подменяем унаследованный идентификатор на собственный. Активная
	 * сессия с таким же идентификатором (при повторном выделении) имеет приоритет
	 */
	if((this->_eid != 0) && (eid != 0) && (eid == this->_iid) && (this->_sessions.find(eid) == this->_sessions.end()))
		// Возвращаем собственный идентификатор события сервера дочернего процесса
		return this->_eid;
	// Возвращаем переданный фасадом идентификатор события без изменения
	return eid;
}
/**
 * @brief Метод объединения потоков данных между двумя событиями
 *
 * @param eid  идентификатор события-источника
 * @param dest идентификатор события-приёмника
 * @return     результат объединения
 *
 */
bool awh::unit::QuicServer::splice(const event::id_t eid, const event::id_t dest) noexcept {
	/**
	 * @brief Выполняем поиск сессии-источника объединения данных
	 *
	 * @details Объединение данных QUIC ведётся на уровне сессии, а не сокета: собранные
	 *          (расшифрованные) данные потоков сессии-источника перенаправляются в
	 *          событие-приёмник, а входящие в сессию объединённые байты отправляются
	 *          туннельным потоком с шифрованием на уровне соединения. Направление
	 *          задаётся отдельным вызовом на каждую сторону (как для прочих транспортов).
	 *
	 */
	auto i = this->_sessions.find(eid);
	// Если сессия-источник найдена (прямое направление: данные сессии -> приёмнику)
	if(i != this->_sessions.end()){
		// Устанавливаем событие-приёмник объединения данных сессии
		i->second.dest = dest;
		/**
		 * Сразу выдаём уже собранные данные потоков, накопленные до установки
		 * приёмника: без нового входящего события метод process() иначе не будет
		 * вызван и буферизованные данные не пойдут в событие-приёмник
		 */
		this->process(eid);
		// Возвращаем положительный результат
		return true;
	}
	/**
	 * Обратное направление: приёмник - сессия, а источник (eid) - внешнее событие
	 * (напр. TCP-бэкенд). Привязываем источник к сессии на уровне сетевого движка:
	 * входящие в источник байты будут инъецированы в туннельный поток сессии, минуя
	 * выдачу приложению (см. inject). Так обеспечивается симметрия с QuicClient::splice
	 */
	if(this->_sessions.find(dest) != this->_sessions.end())
		// Объединяем внешнее событие-источник с сессией-приёмником на уровне движка
		return engine::io::splice(eid, dest);
	// Возвращаем значение по умолчанию
	return false;
}
/**
 * @brief Метод получения метаданных последнего принятого дейтаграммного пакета
 *
 * @param eid идентификатор события сервера
 * @return    метаданные последнего принятого дейтаграммного пакета
 *
 */
awh::net::dgram_info_t awh::unit::QuicServer::getTrafficInfo(const event::id_t eid) const noexcept {
	// Если событие сервера является актуальным
	if(this->isActual(eid))
		// Выполняем получение метаданных последнего принятого дейтаграммного пакета для события сервера
		return engine::io::trafficInfo(this->_eid);
	// Возвращаем значение по умолчанию
	return net::dgram_info_t();
}
/**
 * @brief Метод получения опций события сервера
 *
 * @param eid идентификатор события сервера
 * @return    опции события сервера
 *
 */
uint16_t awh::unit::QuicServer::getOptions(const event::id_t eid) const noexcept {
	// Если событие сервера является актуальным
	if(this->isActual(eid))
		// Выполняем получение опций для события сервера
		return engine::options::get(this->_eid);
	// Возвращаем значение по умолчанию
	return 0;
}
/**
 * @brief Метод установки опций события сервера
 *
 * @param eid     идентификатор события сервера
 * @param options опции события сервера для установки
 * @return        результат выполнения установки
 *
 */
bool awh::unit::QuicServer::setOptions(const event::id_t eid, const uint16_t options) noexcept {
	// Если событие сервера является актуальным
	if(this->isActual(eid))
		// Выполняем установку опций для события сервера
		return engine::options::set(this->_eid, options);
	// Возвращаем значение по умолчанию
	return false;
}
/**
 * @brief Метод установки опции события сервера
 *
 * @param eid    идентификатор события сервера
 * @param option опция события сервера для установки
 * @param mode   режим установки опции события сервера
 * @return       результат выполнения установки
 *
 */
bool awh::unit::QuicServer::setOption(const event::id_t eid, const uint16_t option, const bool mode) noexcept {
	// Если событие сервера является актуальным
	if(this->isActual(eid))
		// Выполняем установку опции для события сервера
		return engine::options::set(this->_eid, option, mode);
	// Возвращаем значение по умолчанию
	return false;
}
/**
 * @brief Метод получения максимального количества хопов, через которые может пройти пакет
 *
 * @param eid идентификатор события сервера
 * @return    максимальное количество хопов
 *
 */
awh::event::hops_t awh::unit::QuicServer::getHops(const event::id_t eid) const noexcept {
	// Если событие сервера является актуальным
	if(this->isActual(eid))
		// Выполняем получение максимального количества хопов для события сервера
		return engine::hops::get(this->_eid);
	// Возвращаем значение по умолчанию
	return event::hops_t::LOOPBACK;
}
/**
 * @brief Метод установки максимального количества хопов, через которые может пройти пакет
 *
 * @param eid  идентификатор события сервера
 * @param hops максимальное количество хопов
 * @return     результат работы функции
 *
 */
bool awh::unit::QuicServer::setHops(const event::id_t eid, const event::hops_t hops) noexcept {
	// Если событие сервера является актуальным
	if(this->isActual(eid))
		// Выполняем установку максимального количества хопов для события сервера
		return engine::hops::set(this->_eid, hops);
	// Возвращаем значение по умолчанию
	return false;
}
/**
 * @brief Метод получения количества хопов последнего принятого пакета
 *
 * @param eid идентификатор события сервера
 * @return    количество хопов последнего принятого пакета
 *
 */
uint8_t awh::unit::QuicServer::getCountHops(const event::id_t eid) const noexcept {
	// Если событие сервера является актуальным
	if(this->isActual(eid))
		// Выполняем получение количества хопов последнего принятого пакета для события сервера
		return engine::hops::count::get(this->_eid);
	// Возвращаем значение по умолчанию
	return 0;
}
/**
 * @brief Метод установки количества хопов последнего принятого пакета
 *
 * @param eid  идентификатор события сервера
 * @param hops количество хопов последнего принятого пакета
 * @return     результат выполнения установки
 *
 */
bool awh::unit::QuicServer::setCountHops(const event::id_t eid, const uint8_t hops) noexcept {
	// Если событие сервера является актуальным
	if(this->isActual(eid))
		// Выполняем установку количества хопов последнего принятого пакета для события сервера
		return engine::hops::count::set(this->_eid, hops);
	// Возвращаем значение по умолчанию
	return false;
}
/**
 * @brief Метод получения данных от клиента
 *
 * @param eid идентификатор события клиента
 * @return    результат получения данных
 *
 */
bool awh::unit::QuicServer::recv([[maybe_unused]] const event::id_t eid) noexcept {
	// Для транспорта QUIC приём датаграмм сессий выполняется самим модулем по событию сервера
	return false;
}
/**
 * @brief Метод отправки данных клиенту
 *
 * @param eid    идентификатор события сессии
 * @param buffer буфер данных для отправки
 * @param size   размер данных для отправки
 * @return       количество байт, поставленных в очередь отправки
 *
 */
size_t awh::unit::QuicServer::send(const event::id_t eid, const void * buffer, const size_t size) noexcept {
	// Если данные для отправки отсутствуют
	if((buffer == nullptr) || (size == 0))
		// Возвращаем значение по умолчанию
		return 0;
	// Выполняем поиск сессии соединения по идентификатору события сессии
	auto i = this->_sessions.find(eid);
	// Если сессия соединения не найдена либо соединение не создано
	if((i == this->_sessions.end()) || (i->second.connection == nullptr))
		// Возвращаем значение по умолчанию
		return 0;
	// Открываем двунаправленный поток приложения по умолчанию (RFC 9000 §2.1)
	const uint64_t sid = i->second.connection->open(false);
	// Если поток приложения не открыт
	if(sid == quic::connection_t::INVALID_STREAM)
		// Возвращаем значение по умолчанию
		return 0;
	// Возвращаем число поставленных в очередь отправки байт (частичный приём)
	return this->send(eid, sid, string_view(reinterpret_cast <const char *> (buffer), size), false);
}
/**
 * @brief Метод отправки данных в поток приложения соединения
 *
 * @param oid  идентификатор события сессии
 * @param sid  идентификатор потока приложения
 * @param data отправляемые данные
 * @param fin  флаг завершения потока
 * @return     результат постановки данных в очередь отправки
 *
 */
size_t awh::unit::QuicServer::send(const event::id_t oid, const uint64_t sid, string_view data, const bool fin) noexcept {
	// Выполняем поиск сессии соединения
	auto i = this->_sessions.find(oid);
	// Если сессия соединения не найдена
	if(i == this->_sessions.end())
		// Ничего не принято в очередь отправки
		return 0;
	// Ставим данные в очередь отправки потока (частичный приём: движок возвращает принятый объём)
	const size_t accepted = i->second.connection->send(sid, data, fin);
	// Если данные приняты либо требуется отправка завершения потока - отправляем готовые датаграммы
	if((accepted > 0) || fin)
		// Отправляем все готовые исходящие датаграммы
		this->flush(oid, i->second);
	// Выводим число принятых в очередь отправки байт
	return accepted;
}
/**
 * @brief Метод установки водяных меток буфера отправки потоков соединения (backpressure)
 *
 * @param oid  идентификатор события сессии
 * @param high верхняя водяная метка (ёмкость буфера отправки потока)
 * @param low  нижняя водяная метка (порог сигнала "writable")
 *
 */
void awh::unit::QuicServer::sendWaterMarks(const event::id_t oid, const size_t high, const size_t low) noexcept {
	// Выполняем поиск сессии соединения
	auto i = this->_sessions.find(oid);
	// Если сессия соединения не найдена либо соединение не создано
	if((i == this->_sessions.end()) || (i->second.connection == nullptr))
		// Выходим из метода
		return;
	// Устанавливаем водяные метки буфера отправки потоков соединения
	i->second.connection->sendWaterMarks(high, low);
}
/**
 * @brief Метод назначения pull-источника данных потока (RFC 9000 §2.2)
 *
 * @param oid    идентификатор события сессии
 * @param sid    идентификатор потока приложения
 * @param source pull-источник данных тела потока
 *
 */
void awh::unit::QuicServer::dataSource(const event::id_t oid, const uint64_t sid, quic::connection_t::data_source_callback_t source) noexcept {
	// Выполняем поиск сессии соединения
	auto i = this->_sessions.find(oid);
	// Если сессия соединения не найдена либо соединение не создано
	if((i == this->_sessions.end()) || (i->second.connection == nullptr))
		// Выходим из метода
		return;
	// Назначаем pull-источник данных тела потока
	i->second.connection->dataSource(sid, source);
	// Отправляем готовые исходящие датаграммы: движок начнёт тянуть данные источника
	this->flush(oid, i->second);
}
/**
 * @brief Метод получения адреса сервера
 *
 * @param eid     идентификатор события сервера
 * @param address тип адреса сервера
 * @return        значение адреса сервера
 *
 */
string awh::unit::QuicServer::getAddress(const event::id_t eid, const event::address_t address) const noexcept {
	// Если событие актуально (для события сервера - адрес прослушивания, для сессии - адрес пира)
	if(this->isActual(eid))
		// Выполняем получение адреса для актуального события (унаследованный кластером идентификатор приводится к собственному)
		return engine::address::get(this->actual(eid), address);
	// Возвращаем значение по умолчанию
	return "";
}
/**
 * @brief Метод установки адреса сервера
 *
 * @param eid     идентификатор события сервера
 * @param address тип адреса сервера
 * @param value   значение адреса сервера
 * @return        результат выполнения установки
 *
 */
bool awh::unit::QuicServer::setAddress(const event::id_t eid, const event::address_t address, string_view value) noexcept {
	// Если событие сервера является актуальным
	if(this->isActual(eid)){
		// Кэшируем адрес прослушивания для пересоздания сокета в дочернем процессе кластера
		if(eid == this->_eid){
			// Сохраняем тип адреса прослушивания сервера
			this->_listenType = address;
			// Сохраняем хост прослушивания сервера
			this->_listenHost = value;
		}
		// Выполняем установку адреса сервера для события сервера
		return engine::address::set(this->_eid, address, value);
	}
	// Возвращаем значение по умолчанию
	return false;
}
/**
 * @brief Метод установки адреса сервера
 *
 * @param eid     идентификатор события сервера
 * @param address тип адреса сервера
 * @param value   структура сетевого адреса сервера
 * @return        результат выполнения установки
 *
 */
bool awh::unit::QuicServer::setAddress(const event::id_t eid, const event::address_t address, const net::addr_t * value) noexcept {
	// Если событие сервера является актуальным
	if(this->isActual(eid))
		// Выполняем установку адреса сервера для события сервера
		return engine::address::set(this->_eid, address, value);
	// Возвращаем значение по умолчанию
	return false;
}
/**
 * @brief Метод получения адреса сервера
 *
 * @param eid     идентификатор события сервера
 * @param address тип адреса сервера
 * @param value   объект для извлечения адреса сервера
 * @return        результат выполнения извлечения адреса сервера
 *
 */
bool awh::unit::QuicServer::getAddress(const event::id_t eid, const event::address_t address, unique_ptr <net::addr_t> & value) const noexcept {
	// Если событие актуально (для события сервера - адрес прослушивания, для сессии - адрес пира)
	if(this->isActual(eid))
		// Выполняем получение адреса для актуального события (унаследованный кластером идентификатор приводится к собственному)
		return engine::address::get(this->actual(eid), address, value);
	// Возвращаем значение по умолчанию
	return false;
}
/**
 * @brief Метод получения режима трансляции пакетов сервера
 *
 * @param eid идентификатор события сервера
 * @return    режим трансляции пакетов (unicast, multicast, broadcast)
 *
 */
awh::event::delivery_mode_t awh::unit::QuicServer::getDelivery(const event::id_t eid) const noexcept {
	// Если событие сервера является актуальным
	if(this->isActual(eid))
		// Выполняем получение режима трансляции пакетов сервера для события сервера
		return engine::delivery::get(this->_eid);
	// Возвращаем значение по умолчанию
	return event::delivery_mode_t::NONE;
}
/**
 * @brief Метод установки режима трансляции пакетов сервера
 *
 * @param eid      идентификатор события сервера
 * @param delivery режим трансляции пакетов (unicast, multicast, broadcast)
 * @return         результат выполнения установки
 *
 */
bool awh::unit::QuicServer::setDelivery(const event::id_t eid, const event::delivery_mode_t delivery) noexcept {
	// Если событие сервера является актуальным
	if(this->isActual(eid))
		// Выполняем установку режима трансляции пакетов сервера для события сервера
		return engine::delivery::set(this->_eid, delivery);
	// Возвращаем значение по умолчанию
	return false;
}
/**
 * @brief Метод получения размера буфера сервера
 *
 * @param eid    идентификатор события сервера
 * @param action тип действия сервера
 * @return       размер буфера сервера
 *
 */
size_t awh::unit::QuicServer::getBufferSize(const event::id_t eid, const event::action_t action) const noexcept {
	// Если событие сервера является актуальным
	if(this->isActual(eid))
		// Выполняем получение размера буфера сервера для события сервера
		return engine::buffer::size::get(this->_eid, action);
	// Возвращаем значение по умолчанию
	return 0;
}
/**
 * @brief Метод установки размера буфера сервера
 *
 * @param eid    идентификатор события сервера
 * @param action тип действия сервера
 * @param size   размер буфера сервера
 * @return       результат выполнения установки
 *
 */
bool awh::unit::QuicServer::setBufferSize(const event::id_t eid, const event::action_t action, const size_t size) noexcept {
	// Если событие сервера является актуальным
	if(this->isActual(eid))
		// Выполняем установку размера буфера сервера для события сервера
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
awh::event::usage_t awh::unit::QuicServer::getUsageReadTimeout(const event::id_t eid) const noexcept {
	// Если событие сервера является актуальным
	if(this->isActual(eid))
		// Выполняем получение режима использования таймаута на чтение события для события сервера
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
void awh::unit::QuicServer::setUsageReadTimeout(const event::id_t eid, const event::usage_t usage) noexcept {
	// Если событие сервера является актуальным
	if(this->isActual(eid))
		// Выполняем установку режима использования таймаута на чтение события для события сервера
		engine::timer::usage(this->_eid, usage);
}
/**
 * @brief Метод получения таймаута сервера
 *
 * @param eid    идентификатор события сервера
 * @param action тип действия сервера
 * @return       значение таймаута в миллисекундах
 *
 */
uint32_t awh::unit::QuicServer::getTimeout(const event::id_t eid, const event::action_t action) const noexcept {
	// Если событие сервера является актуальным
	if(this->isActual(eid))
		// Выполняем получение параметров таймаута для сервера
		return engine::timer::timeout(this->_eid, action);
	// Возвращаем значение по умолчанию
	return 0;
}
/**
 * @brief Метод установки таймаута сервера
 *
 * @param eid     идентификатор события сервера
 * @param action  тип действия сервера
 * @param timeout значение таймаута в миллисекундах
 *
 */
void awh::unit::QuicServer::setTimeout(const event::id_t eid, const event::action_t action, const uint32_t timeout) noexcept {
	// Если событие сервера является актуальным
	if(this->isActual(eid))
		// Выполняем установку параметров таймаута для сервера
		engine::timer::timeout(this->_eid, action, timeout);
}
/**
 * @brief Метод установки пропускной способности сервера
 *
 * @param eid       идентификатор события сервера
 * @param limiting  режим ограничения пропускной способности сервера (egress или ingress)
 * @param bandwidth пропускная способность сервера для установки
 * @return          результат выполнения установки
 *
 */
bool awh::unit::QuicServer::bandwidth(const event::id_t eid, const event::limiting_t limiting, string_view bandwidth) noexcept {
	// Если событие сервера является актуальным
	if(this->isActual(eid))
		// Выполняем установку параметров пропускной способности для сервера
		return engine::io::bandwidth(this->_eid, limiting, bandwidth);
	// Возвращаем значение по умолчанию
	return false;
}
/**
 * @brief Метод установки параметров keep-alive для сервера
 *
 * @param eid   идентификатор события сервера
 * @param cnt   количество пакетов keep-alive
 * @param idle  время простоя перед отправкой первого пакета keep-alive в секундах
 * @param intvl интервал между пакетами keep-alive в секундах
 * @return      результат выполнения установки
 *
 */
bool awh::unit::QuicServer::keepAlive(const event::id_t eid, const int32_t cnt, const int32_t idle, const int32_t intvl) noexcept {
	// Если событие сервера является актуальным
	if(this->isActual(eid))
		// Выполняем установку параметров keep-alive для сервера
		return engine::connection::keepalive(this->_eid, cnt, idle, intvl);
	// Возвращаем значение по умолчанию
	return false;
}
/**
 * @brief Метод получения MTU сетевого интерфейса
 *
 * @param eid идентификатор события сервера
 * @return    MTU сетевого интерфейса
 *
 */
uint16_t awh::unit::QuicServer::getMaximumTransmissionUnit(const event::id_t eid) const noexcept {
	// Если событие сервера является актуальным
	if(this->isActual(eid))
		// Выполняем получение MTU сетевого интерфейса для события сервера
		return engine::mtu::get(this->_eid);
	// Возвращаем значение по умолчанию
	return 0;
}
/**
 * @brief Метод установки MTU сетевого интерфейса
 *
 * @param eid идентификатор события сервера
 * @param mtu размер MTU интерфейса
 * @return    результат установки MTU сетевого интерфейса
 *
 */
bool awh::unit::QuicServer::setMaximumTransmissionUnit(const event::id_t eid, const uint32_t mtu) const noexcept {
	// Если событие сервера является актуальным
	if(this->isActual(eid))
		// Выполняем установку MTU сетевого интерфейса для события сервера
		return engine::mtu::set(this->_eid, mtu);
	// Возвращаем значение по умолчанию
	return false;
}
/**
 * @brief Метод получения режима обнаружения максимального размера пакета (MTU)
 *
 * @param eid идентификатор события сервера
 * @return    текущий режим обнаружения MTU
 *
 */
awh::event::mtu_discover_t awh::unit::QuicServer::getMaximumTransmissionUnitDiscover(const event::id_t eid) const noexcept {
	// Если событие сервера является актуальным
	if(this->isActual(eid))
		// Выполняем получение режима обнаружения MTU для события сервера
		return engine::mtu::discover::get(this->_eid, engine::io::family(this->_eid));
	// Возвращаем значение по умолчанию
	return event::mtu_discover_t::NONE;
}
/**
 * @brief Метод установки режима обнаружения максимального размера пакета (MTU)
 *
 * @param eid  идентификатор события сервера
 * @param mode режим обнаружения максимального размера пакета (MTU)
 * @return     результат работы функции
 *
 */
bool awh::unit::QuicServer::setMaximumTransmissionUnitDiscover(const event::id_t eid, const event::mtu_discover_t mode) const noexcept {
	// Если событие сервера является актуальным
	if(this->isActual(eid))
		// Выполняем установку режима обнаружения MTU для события сервера
		return engine::mtu::discover::set(this->_eid, engine::io::family(this->_eid), mode);
	// Возвращаем значение по умолчанию
	return false;
}
/**
 * @brief Метод получения значения поля Differentiated Services Code Point (DSCP) в заголовке IP-пакета
 *
 * @param eid идентификатор события сервера
 * @return    значение DSCP
 *
 */
awh::event::dscp_t awh::unit::QuicServer::getDifferentiatedServicesCodePoint(const event::id_t eid) const noexcept {
	// Если событие сервера является актуальным
	if(this->isActual(eid))
		// Выполняем получение значения поля Differentiated Services Code Point (DSCP) для сервера
		return engine::qos::dscp::get(this->_eid, engine::io::family(this->_eid));
	// Возвращаем значение по умолчанию
	return event::dscp_t::CS0;
}
/**
 * @brief Метод установки значения поля Differentiated Services Code Point (DSCP) в заголовке IP-пакета
 *
 * @param eid  идентификатор события сервера
 * @param dscp значение DSCP
 * @return     результат работы функции
 *
 */
bool awh::unit::QuicServer::setDifferentiatedServicesCodePoint(const event::id_t eid, const event::dscp_t dscp) const noexcept {
	// Если событие сервера является актуальным
	if(this->isActual(eid))
		// Выполняем установку значения поля Differentiated Services Code Point (DSCP) для сервера
		return engine::qos::dscp::set(this->_eid, engine::io::family(this->_eid), dscp);
	// Возвращаем значение по умолчанию
	return false;
}
/**
 * @brief Метод активации/деактивации мультикаст-группы
 *
 * @param eid    идентификатор события сервера
 * @param mode   режим активации/деактивации
 * @param group  мультикаст-группа для активации/деактивации
 * @param source адрес сетевого интерфейса с которого выполняется подписка
 * @param port   порт мультикаст-группы с которого выполняется подписка
 * @return       результат выполнения установки
 *
 */
bool awh::unit::QuicServer::membership(const event::id_t eid, const event::mode_t mode, string_view group, string_view source, const uint16_t port) noexcept {
	// Если событие сервера является актуальным
	if(this->isActual(eid))
		// Выполняем активацию/деактивацию мультикаст-группы для сервера
		return engine::multicast::membership(this->_eid, mode, group, source, port);
	// Возвращаем значение по умолчанию
	return false;
}
/**
 * @brief Метод активации/деактивации мультикаст-группы
 *
 * @param eid    идентификатор события сервера
 * @param mode   режим активации/деактивации
 * @param group  мультикаст-группа для активации/деактивации
 * @param source адрес сетевого интерфейса с которого выполняется подписка
 * @param port   порт мультикаст-группы с которого выполняется подписка
 * @return       результат выполнения установки
 *
 */
bool awh::unit::QuicServer::membership(const event::id_t eid, const event::mode_t mode, const net::addr_t * group, const net::addr_t * source, const uint16_t port) noexcept {
	// Если событие сервера является актуальным
	if(this->isActual(eid))
		// Выполняем активацию/деактивацию мультикаст-группы для сервера
		return engine::multicast::membership(this->_eid, mode, group, source, port);
	// Возвращаем значение по умолчанию
	return false;
}
/**
 * @brief Метод создания события сервера QUIC поверх UDP
 *
 * @param family   семейство адресов события сервера
 * @param type     тип события (игнорируется, приводится к DATAGRAM)
 * @param protocol протокол события (игнорируется, приводится к UDP)
 * @return         идентификатор созданного события сервера
 *
 */
awh::event::id_t awh::unit::QuicServer::issue(const event::family_t family, [[maybe_unused]] const event::type_t type, [[maybe_unused]] const event::protocol_t protocol) noexcept {
	// Если событие сервера уже создано
	if(this->_eid != 0)
		// Выводим идентификатор созданного ранее события сервера
		return this->_eid;
	// Добавляем событие сервера поверх UDP (транспорт QUIC работает поверх дейтаграммного UDP-сокета)
	this->_eid = engine::io::event(event::node_t::SERVER, family, event::type_t::DATAGRAM, event::protocol_t::UDP);
	// Добавляем событие интервала таймеров соединений
	this->_tid = engine::io::event(event::node_t::INTERVAL, event::family_t::TIMER);
	// Если события не созданы
	if((this->_eid == 0) || (this->_tid == 0)){
		/**
		 * Если включён режим отладки
		 */
		#if defined(DEBUG_MODE)
			// Записываем ошибку в лог
			log::debug("QUIC server events are not created", __PRETTY_FUNCTION__, {
				static_cast <uint16_t> (family),
				static_cast <uint16_t> (type),
				static_cast <uint16_t> (protocol)
			}, log::flag_t::CRITICAL);
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			log::print("QUIC server events are not created", log::flag_t::CRITICAL);
		#endif
		// Выводим отрицательный результат
		return 0;
	}
	// Запоминаем семейство адресов события сервера
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
	/**
	 * Если общий ключ вывода токенов сброса не задан приложением: генерируем его
	 * случайно - сброс будет работать в пределах жизни процесса
	 */
	if(this->_resetKey.empty() && !quic::resetKey(this->_resetKey))
		/**
		 * Если включён режим отладки
		 */
		#if defined(DEBUG_MODE)
			// Записываем предупреждение в лог - сброс без сохранения состояния недоступен
			log::debug("QUIC stateless reset key is not generated", __PRETTY_FUNCTION__, {
				static_cast <uint16_t> (family),
				static_cast <uint16_t> (type),
				static_cast <uint16_t> (protocol)
			}, log::flag_t::WARNING);
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем предупреждение в лог - сброс без сохранения состояния недоступен
			log::print("QUIC stateless reset key is not generated", log::flag_t::WARNING);
		#endif
	// Устанавливаем интервал проверки таймеров соединений
	engine::timer::timeout(this->_tid, event::action_t::NONE, 25);
	/**
	 * Устанавливаем опции события сервера: извлечение метаданных датаграмм
	 * включается только при уведомлении о перегрузке - оно переводит приём на
	 * разбор служебных сообщений и обходится дороже
	 */
	engine::options::set(this->_eid, static_cast <uint16_t> (
		event::options::NO_SIGILL | event::options::NO_SIGPIPE | event::options::REUSE_ADDR |
		event::options::REUSE_PORT | event::options::NO_IO_BLOCK | event::options::CLOSE_ON_EXEC |
		(this->_ecn ? event::options::DGRAM_INFO : 0)
	));
	/**
	 * Устанавливаем функцию обратного вызова определения сессии: соединение
	 * адресуется идентификатором соединения, а не четвёркой сокета, поэтому
	 * смена адреса клиента его не разрывает
	 */
	engine::io::on(this->_eid, static_cast <engine::callback::origin_t> (std::bind(&QuicServer::origin, this, _1, _2, _3, _4)));
	// Устанавливаем функцию обратного вызова на создание сессии соединения
	engine::io::on(this->_eid, static_cast <engine::callback::accept_t> (std::bind(&QuicServer::accept, this, _1, _2)));
	// Устанавливаем функцию обратного вызова на событие интервала таймеров соединений
	engine::io::on(this->_tid, static_cast <engine::callback::status_t> (std::bind(&QuicServer::tick, this, _1, _2)));
	// Выводим идентификатор созданного события сервера
	return this->_eid;
}
/**
 * @brief Метод остановки сервера
 *
 */
void awh::unit::QuicServer::stop() noexcept {
	// Если работа юнита запущена
	if(this->working()){
		/**
		 * Для операционной системы MS Windows
		 */
		#if defined(_WIN32) || defined(_WIN64)
			// Если кластер в работе и процесс является мастером
			if((this->_cluster != nullptr) && this->_cluster->master())
				// Останавливаем работу кластера: работники уходят вместе с мастером
				this->_cluster->stop();
			// Выполняем остановку работы основного юнита
			unit_t::stop();
		/**
		 * @brief Для операционных систем, отличных от MS Windows
		 *
		 * @note Кластер здесь останавливается откликом о завершении работы юнита
		 *       (см. launch()): сперва потребитель завершает свои соединения, и лишь
		 *       затем уходят работники. Общий сокет мастера к тому мигу уже уничтожен,
		 *       и новых соединений кластер не принимает
		 *
		 */
		#else
			// Выполняем остановку работы основного юнита
			unit_t::stop();
		#endif
	}
}
/**
 * @brief Метод запуска сервера
 *
 */
void awh::unit::QuicServer::start() noexcept {
	// Если работа юнита ещё не запущена
	if(!this->working()){
		/**
		 * @brief Для операционной системы MS Windows кластер заводится ДО запуска юнита
		 *
		 * @details Работник у этой системы порождается заново той же строкой доводов,
		 *          и отличает его лишь метка роли в окружении: спросить её не у чего,
		 *          покуда объекта кластера нет. Знать роль нужно раньше, чем
		 *          потребитель заведёт свои события, - иначе работник завёл бы
		 *          собственный общий сокет вместо ожидаемой передачи соединений.
		 *
		 * @note Заводится здесь только объект: запускается кластер прежним порядком -
		 *       откликом о запуске юнита, когда общий сокет мастера уже поднят
		 *
		 * @note На системах, отличных от MS Windows, роль известна и без кластера:
		 *       работник порождается fork'ом и отличается от мастера собственным номером
		 *       процесса (см. Unit::master()). Объект там заводится тем же откликом о
		 *       запуске юнита - раньше он не нужен
		 *
		 */
		#if defined(_WIN32) || defined(_WIN64)
			// Заводим объект кластера для управления процессами сервера
			this->clusterCreate();
		#endif
		// Выполняем получение идентификатора функции обратного вызова
		const callback_t::id_t fid = this->_callback.id("status");
		// Если функция обратного вызова установлена
		if(this->_callback.is(fid))
			// Переименовываем callback status в server_status
			this->_callback.set(fid, this->_callback.id("server_status"), this->_callback);
		// Устанавливаем функцию обратного вызова на запуск системы
		this->_callback.on <void (const event::status_t)> (fid, static_cast <void (quic_server_t::*)(const event::status_t)> (&quic_server_t::launch), this, _1);
		// Выполняем запуск работы основного юнита
		unit_t::start();
	}
}
/**
 * @brief Метод получения адреса удалённого эндпоинта соединения
 *
 * @param oid идентификатор события сессии
 * @return    адрес удалённого эндпоинта в виде "адрес:порт"
 *
 */
string awh::unit::QuicServer::address(const event::id_t oid) const noexcept {
	// Если сессия соединения не найдена
	if(this->_sessions.find(oid) == this->_sessions.end())
		// Выводим пустой результат
		return "";
	// Выводим адрес удалённого эндпоинта соединения
	return this->peer(oid);
}
/**
 * @brief Метод установки функций обратного вызова
 *
 * @param callback функции обратного вызова
 *
 */
void awh::unit::QuicServer::callback(const callback_t & callback) noexcept {
	// Выполняем установку функции обратного вызова на установленное соединение
	this->_callback.set("open", callback);
	// Выполняем установку функции обратного вызова на собранные данные потока
	this->_callback.set("read", callback);
	// Выполняем установку функции обратного вызова на завершённое соединение
	this->_callback.set("close", callback);
	// Выполняем установку функции обратного вызова на освобождение буфера отправки потока
	this->_callback.set("writable", callback);
	// Выполняем установку функции обратного вызова на принятую датаграмму приложения
	this->_callback.set("datagram", callback);
	// Выполняем установку функции обратного вызова при завершении работы процесса кластера
	this->_callback.set("cluster_exit", callback);
	// Выполняем установку функции обратного вызова при получении ошибок кластера
	this->_callback.set("cluster_error", callback);
	// Выполняем установку функции обратного вызова при получении состояния процесса кластера
	this->_callback.set("cluster_state", callback);
	// Выполняем установку функции обратного вызова при пересоздании процесса кластера
	this->_callback.set("cluster_rebase", callback);
	// Выполняем установку функции обратного вызова при запуске/остановке процесса кластера
	this->_callback.set("cluster_events", callback);
	// Выполняем установку функции обратного вызова при отправке сообщения кластера
	this->_callback.set("cluster_sending", callback);
	// Выполняем установку функции обратного вызова при получении сообщения кластера
	this->_callback.set("cluster_message", callback);
	// Выполняем установку функции обратного вызова при изменении доступности очереди исходящих сообщений кластера
	this->_callback.set("cluster_available", callback);
}
/**
 * @brief Метод получения согласованного ALPN-протокола соединения
 *
 * @param oid идентификатор события сессии
 * @return    согласованный ALPN-протокол
 *
 */
awh::tls::coder::alpn_t awh::unit::QuicServer::alpn(const event::id_t oid) const noexcept {
	// Выполняем поиск сессии соединения
	auto i = this->_sessions.find(oid);
	// Если сессия соединения найдена
	if(i != this->_sessions.end())
		// Выводим согласованный ALPN-протокол соединения
		return i->second.connection->alpn();
	// Выводим пустой результат
	return tls::coder::alpn_t(0, "");
}
/**
 * @brief Метод получения предельного размера отправляемой датаграммы (RFC 9221 §3)
 *
 * @param oid идентификатор события сессии
 * @return    предельный размер данных датаграммы в октетах (0 - датаграммы не поддерживаются)
 *
 */
size_t awh::unit::QuicServer::datagrams(const event::id_t oid) const noexcept {
	// Выполняем поиск сессии соединения
	auto i = this->_sessions.find(oid);
	// Если сессия соединения не найдена
	if(i == this->_sessions.end())
		// Выводим нулевой предел - отправка датаграмм невозможна
		return 0;
	// Выводим предельный размер данных отправляемой датаграммы
	return i->second.connection->datagrams();
}
/**
 * @brief Метод отправки датаграммы приложения соединению (RFC 9221)
 *
 * @param oid  идентификатор события сессии
 * @param data данные датаграммы приложения
 * @return     результат отправки
 *
 */
bool awh::unit::QuicServer::datagram(const event::id_t oid, string_view data) noexcept {
	// Выполняем поиск сессии соединения
	auto i = this->_sessions.find(oid);
	// Если сессия соединения не найдена
	if(i == this->_sessions.end())
		// Выводим отрицательный результат
		return false;
	// Если постановка датаграммы в очередь отправки не выполнена
	if(i->second.connection->datagram(data) != quic::status_t::OK)
		// Выводим отрицательный результат
		return false;
	// Отправляем все готовые исходящие датаграммы
	this->flush(oid, i->second);
	// Выводим положительный результат
	return true;
}
/**
 * @brief Метод открытия потока приложения соединения
 *
 * @param oid  идентификатор события сессии
 * @param mode режим однонаправленного потока
 * @return     идентификатор открытого потока
 *
 */
uint64_t awh::unit::QuicServer::open(const event::id_t oid, const bool mode) noexcept {
	// Выполняем поиск сессии соединения
	auto i = this->_sessions.find(oid);
	// Если сессия соединения не найдена
	if(i == this->_sessions.end())
		// Выводим отрицательный результат
		return quic::connection_t::INVALID_STREAM;
	// Выводим идентификатор открытого потока приложения
	return i->second.connection->open(mode);
}
/**
 * @brief Метод завершения соединения приложением (RFC 9000 §10.2)
 *
 * @param oid    идентификатор события сессии
 * @param code   код ошибки приложения
 * @param reason человекочитаемая причина завершения
 *
 */
void awh::unit::QuicServer::close(const event::id_t oid, const uint64_t code, string_view reason) noexcept {
	// Выполняем поиск сессии соединения
	auto i = this->_sessions.find(oid);
	// Если сессия соединения не найдена
	if(i == this->_sessions.end())
		// Выходим из метода
		return;
	// Выполняем завершение соединения приложением
	i->second.connection->close(code, reason);
	// Отправляем фрейм завершения соединения удалённому эндпоинту
	this->flush(oid, i->second);
}
/**
 * @brief Конструктор
 *
 */
awh::unit::QuicServer::QuicServer() noexcept :
 _ecn(false), _retry(false),
 _eid(0), _tid(0), _iid(0),
 _listenPort(0), _backlog(0), _ctx(0),
 _family(event::family_t::IPV4), _listenType(event::address_t::IPV4),
 _resetKey{""}, _listenHost{""}, _cluster(nullptr) {
	// Сбрасываем позицию очереди раздачи соединений между работниками
	this->_handoverNext = 0;
	// Сбрасываем адресата передачи соединения
	this->_handoverPid = 0;
}
/**
 * @brief Деструктор
 *
 */
awh::unit::QuicServer::~QuicServer() noexcept {
	/**
	 * Перебираем список сессий соединений
	 */
	for(auto & item : this->_sessions){
		// Снимаем функцию обратного вызова на чтение датаграмм сессии
		engine::io::on(item.first, static_cast <engine::callback::read_t> (nullptr));
		// Снимаем функцию обратного вызова источника исходящих датаграмм
		engine::io::on(item.first, static_cast <engine::callback::source_t> (nullptr));
		// Уничтожаем событие сессии вместе с его ключами маршрутизации
		engine::io::destroy(item.first);
	}
	// Очищаем список сессий соединений
	this->_sessions.clear();
	// Если событие интервала таймеров соединений создано
	if(this->_tid != 0)
		// Уничтожаем событие интервала таймеров соединений
		engine::io::destroy(this->_tid);
	// Если событие сервера создано
	if(this->_eid != 0)
		// Уничтожаем событие сервера
		engine::io::destroy(this->_eid);
}
