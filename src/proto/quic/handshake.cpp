/**
 * @file handshake.cpp
 * @date 2026-07-21
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
 * @brief Реализация криптографического рукопожатия QUIC — управление уровнями шифрования,
 *        обмен CRYPTO-данными с A-NTS, установка ключей защиты и передача транспортных параметров соединения
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Стандартные заголовочные файлы
 */
#include <cstring>

/**
 * Заголовочные файлы библиотеки A-NTS
 */
#include <anyks/nts/client/client.hpp>
#include <anyks/nts/server/server.hpp>

/**
 * Подключаем заголовочные файлы проекта
 */
#include <sys/log.hpp>
#include <proto/quic/handshake.hpp>

/**
 * Подписываемся на стандартное пространство имён
 */
using namespace std;

/**
 * @brief Внутреннее пространство имён вспомогательных функций
 *
 */
namespace {
	/**
	 * @brief Функция преобразования внутреннего уровня шифрования в уровень защиты A-NTS
	 *
	 * @param level внутренний уровень шифрования
	 * @return      уровень защиты A-NTS
	 *
	 */
	static nts::context::level_t __awh_level_to_nts__(const awh::quic::level_t level) noexcept {
		/**
		 * Определяем внутренний уровень шифрования
		 */
		switch(level){
			// Уровень Initial
			case awh::quic::level_t::INITIAL:
				// Выводим уровень защиты A-NTS
				return nts::context::level_t::INITIAL;
			// Уровень ранних данных 0-RTT
			case awh::quic::level_t::EARLY_DATA:
				// Выводим уровень защиты A-NTS
				return nts::context::level_t::EARLY;
			// Уровень хендшейка
			case awh::quic::level_t::HANDSHAKE:
				// Выводим уровень защиты A-NTS
				return nts::context::level_t::HANDSHAKE;
			// Уровень приложения 1-RTT
			case awh::quic::level_t::APPLICATION:
				// Выводим уровень защиты A-NTS
				return nts::context::level_t::APPLICATION;
		}
		// Выводим уровень Initial по умолчанию
		return nts::context::level_t::INITIAL;
	}
	/**
	 * @brief Функция определения криптографического набора по набору шифрования TLS (RFC 9001 §5.1)
	 *
	 * @param suite набор шифрования, согласованный TLS-стеком
	 * @return      определённый криптографический набор
	 *
	 */
	static awh::quic::crypto::suite_t __awh_suite_from_code__(const nts::protect::suite_t suite) noexcept {
		/**
		 * Определяем набор шифрования IANA
		 */
		switch(suite){
			// Шифр TLS_AES_128_GCM_SHA256
			case nts::protect::suite_t::AES_128_GCM_SHA256:
				// Выводим криптографический набор
				return awh::quic::crypto::suite_t::AES_128_GCM_SHA256;
			// Шифр TLS_AES_256_GCM_SHA384
			case nts::protect::suite_t::AES_256_GCM_SHA384:
				// Выводим криптографический набор
				return awh::quic::crypto::suite_t::AES_256_GCM_SHA384;
			// Шифр TLS_CHACHA20_POLY1305_SHA256
			case nts::protect::suite_t::CHACHA20_POLY1305_SHA256:
				// Выводим криптографический набор
				return awh::quic::crypto::suite_t::CHACHA20_POLY1305_SHA256;
			// Во всех остальных случаях
			default:
				// Выводим обязательный набор для уровня Initial
				return awh::quic::crypto::suite_t::AES_128_GCM_SHA256;
		}
	}
};

/**
 * @brief Конструктор состояния уровня шифрования
 *
 */
awh::quic::Handshake::Level::Level() noexcept : hasRead(false), hasWrite(false), data{""} {}

/**
 * @brief Метод продвижения TLS-хендшейка по связи
 *
 * @param ctx связь TLS
 * @return   результат продвижения (OK - хендшейк продолжается или завершён)
 *
 */
template <typename T>
awh::quic::status_t awh::quic::Handshake::processImpl(T & ctx) noexcept {
	// Получаем криптографический набор связи
	const crypto::suite_t suite = ::__awh_suite_from_code__(ctx.suite());
	/**
	 * Перебираем уровни шифрования и выводим появившиеся секреты
	 */
	for(size_t i = 0; i < LEVELS; i++){
		// Получаем состояние уровня шифрования
		auto & item = this->_levels[i];
		// Уровень защиты A-NTS
		const nts::context::level_t level = ::__awh_level_to_nts__(static_cast <level_t> (i));
		// Буфер секрета направления
		uint8_t secret[64];
		// Размер секрета направления
		size_t size = 0;
		// Если ключи чтения ещё не выведены
		if(!item.hasRead){
			// Если секрет чтения выведен
			if(ctx.secret(level, nts::context::direction_t::READ, secret, size)){
				// Устанавливаем криптографический набор
				item.read.suite = suite;
				// Устанавливаем секрет направления
				item.read.secret.assign(reinterpret_cast <const char *> (secret), size);
				// Выводим ключи защиты пакетов из секрета
				if(crypto::derive(item.read))
					// Устанавливаем флаг наличия ключей чтения
					item.hasRead = true;
			}
		}
		// Если ключи записи ещё не выведены
		if(!item.hasWrite){
			// Если секрет записи выведен
			if(ctx.secret(level, nts::context::direction_t::WRITE, secret, size)){
				// Устанавливаем криптографический набор
				item.write.suite = suite;
				// Устанавливаем секрет направления
				item.write.secret.assign(reinterpret_cast <const char *> (secret), size);
				// Выводим ключи защиты пакетов из секрета
				if(crypto::derive(item.write))
					// Устанавливаем флаг наличия ключей записи
					item.hasWrite = true;
			}
		}
	}
	// Если связь установлена
	if(ctx.established()){
		// Устанавливаем состояние успешного завершения хендшейка
		this->_state = state_t::COMPLETED;
		// Если локальный эндпоинт является клиентом и билет ещё не снят
		if((this->_endpoint == endpoint_t::CLIENT) && this->_ticket.empty()){
			// Получаем список билетов возобновления связи
			const auto & tickets = ctx.tickets();
			// Если билет получен
			if(!tickets.empty()){
				// Накопитель записи билета
				vector <uint8_t> record;
				// Если билет записан в октеты
				if(nts::context::encode(tickets.front(), record) == nts::context::error_t::NONE)
					// Принимаем запись билета
					this->_ticket.assign(reinterpret_cast <const char *> (record.data()), record.size());
			}
		}
		// Выводим положительный результат
		return status_t::OK;
	}
	// Если связь оборвана отказом
	if(ctx.error() != nts::context::error_t::NONE){
		// Устанавливаем состояние ошибки хендшейка
		this->_state = state_t::FAILED;
		// Записываем ошибку в лог
		awh::log::print("QUIC TLS handshake failed", awh::log::flag_t::CRITICAL);
		// Выводим отрицательный результат
		return status_t::ERROR;
	}
	// Выводим положительный результат - хендшейк продолжается
	return status_t::OK;
}

/**
 * @brief Метод продвижения TLS-хендшейка
 *
 * @return результат продвижения (OK - хендшейк продолжается или завершён)
 *
 */
awh::quic::status_t awh::quic::Handshake::process() noexcept {
	// Если локальный эндпоинт является клиентом
	if(this->_endpoint == endpoint_t::CLIENT)
		// Продвигаем хендшейк по связи клиента
		return this->processImpl(* this->_client);
	// Продвигаем хендшейк по связи сервера
	return this->processImpl(* this->_server);
}

/**
 * @brief Метод извлечения согласованного ALPN-протокола
 *
 * @return согласованный ALPN-протокол (пустое название - согласование не выполнено)
 *
 */
awh::tls::coder::alpn_t awh::quic::Handshake::alpn() const noexcept {
	// Согласованный ALPN-протокол
	tls::coder::alpn_t result{0, ""};
	// Если связь не заведена
	if((this->_client == nullptr) && (this->_server == nullptr))
		// Выводим пустой результат
		return result;
	// Название согласованного протокола
	const string & protocol = (this->_endpoint == endpoint_t::CLIENT) ? this->_client->protocol() : this->_server->protocol();
	// Если ALPN-протокол не согласован
	if(protocol.empty())
		// Выводим пустой результат
		return result;
	// Устанавливаем название согласованного протокола
	result.protocol = protocol;
	/**
	 * Восстанавливаем идентификатор протокола по списку контекста: согласование
	 * возвращает только название, а идентификатор назначается вызывающим кодом
	 * при настройке списка поддерживаемых протоколов
	 */
	for(auto & item : this->_protocols){
		// Если название протокола совпадает с согласованным
		if(item.protocol == result.protocol){
			// Устанавливаем идентификатор согласованного протокола
			result.id = item.id;
			// Прекращаем поиск
			break;
		}
	}
	// Выводим согласованный ALPN-протокол
	return result;
}

/**
 * @brief Метод извлечения возобновляемой сессии хендшейка (RFC 9001 §4.6)
 *
 * @return сериализованная сессия (пусто - сессия недоступна)
 *
 */
string awh::quic::Handshake::session() const noexcept {
	// Выводим принятый от удалённого узла билет возобновления
	return this->_ticket;
}

/**
 * @brief Метод установки возобновляемой сессии хендшейка (RFC 9001 §4.6)
 *
 * @param session сериализованная сессия
 * @return        результат установки
 *
 */
bool awh::quic::Handshake::session(string_view session) noexcept {
	// Если хендшейк уже начат либо эндпоинт не является клиентом
	if((this->_state != state_t::NONE) || (this->_endpoint != endpoint_t::CLIENT))
		// Выводим отрицательный результат
		return false;
	// Устанавливаем сериализованную сессию возобновления
	this->_session.assign(session.begin(), session.end());
	// Выводим положительный результат
	return true;
}

/**
 * @brief Метод проверки принятия ранних данных удалённым узлом
 *
 * @return результат проверки
 *
 */
bool awh::quic::Handshake::early() const noexcept {
	// Если связь клиента не заведена либо в ранних данных отказано
	if((this->_client == nullptr) || this->_rejected)
		// Выводим отрицательный результат
		return false;
	// Выводим результат принятия ранних данных удалённым узлом
	return this->_client->accepted();
}

/**
 * @brief Метод проверки отказа удалённого узла в ранних данных (RFC 9001 §4.6.2)
 *
 * @return результат проверки
 *
 */
bool awh::quic::Handshake::rejected() const noexcept {
	// Если связь клиента не заведена
	if(this->_client == nullptr)
		// Выводим отрицательный результат
		return false;
	// Выводим флаг отказа удалённого узла в ранних данных
	return this->_client->rejected();
}

/**
 * @brief Метод установки локальных транспортных параметров (RFC 9000 §7.4)
 *
 * @param params локальные транспортные параметры
 * @return       результат установки (false - ошибка сериализации)
 *
 */
bool awh::quic::Handshake::params(const quic::params::params_t & params) noexcept {
	// Очищаем сериализованные локальные транспортные параметры
	this->_params.clear();
	// Очищаем контекст ранних данных
	this->_early.clear();
	// Если сериализация транспортных параметров не выполнена
	if(!quic::params::serialize::encode(this->_params, params, this->_endpoint))
		// Выводим отрицательный результат
		return false;
	// Собираем контекст ранних данных и выводим результат
	return quic::params::serialize::early(this->_early, params);
}

/**
 * @brief Метод извлечения транспортных параметров удалённого узла (RFC 9000 §7.4)
 *
 * @param params транспортные параметры удалённого узла
 * @param error  код ошибки транспорта
 * @return       результат извлечения (OK/INCOMPLETE/ERROR)
 *
 */
awh::quic::status_t awh::quic::Handshake::peer(quic::params::params_t & params, error_t & error) const noexcept {
	// Если связь не заведена, выводим результат отсутствия данных
	if((this->_client == nullptr) && (this->_server == nullptr))
		// Выводим результат отсутствия данных
		return status_t::INCOMPLETE;
	// Параметры переноса удалённого узла
	const vector <uint8_t> & data = (this->_endpoint == endpoint_t::CLIENT) ? this->_client->parameters() : this->_server->parameters();
	// Если транспортные параметры ещё не получены
	if(data.empty())
		// Выводим результат отсутствия данных
		return status_t::INCOMPLETE;
	// Определяем роль удалённого узла (противоположна локальной)
	const endpoint_t sender = ((this->_endpoint == endpoint_t::CLIENT) ? endpoint_t::SERVER : endpoint_t::CLIENT);
	// Разбираем транспортные параметры удалённого узла и выводим результат
	return quic::params::parser::decode(data.data(), data.size(), sender, params, error);
}

/**
 * @brief Метод вывода ключей уровня Initial (RFC 9001 §5.2)
 *
 * @param dcid идентификатор соединения получателя первого пакета Initial клиента
 * @return     результат вывода (false - ошибка криптографической библиотеки)
 *
 */
bool awh::quic::Handshake::initial(const cid_t & dcid) noexcept {
	// Ключи защиты пакетов клиента
	crypto::keys_t client;
	// Ключи защиты пакетов сервера
	crypto::keys_t server;
	// Выводим ключи Initial обоих направлений
	if(!crypto::initial(dcid, client, server))
		// Выводим отрицательный результат
		return false;
	// Получаем состояние уровня шифрования Initial
	auto & item = this->_levels[static_cast <size_t> (level_t::INITIAL)];
	// Если локальный эндпоинт является клиентом
	if(this->_endpoint == endpoint_t::CLIENT){
		// Устанавливаем клиентские ключи направлением записи
		item.write = ::move(client);
		// Устанавливаем серверные ключи направлением чтения
		item.read = ::move(server);
	// Если локальный эндпоинт является сервером
	} else {
		// Устанавливаем серверные ключи направлением записи
		item.write = ::move(server);
		// Устанавливаем клиентские ключи направлением чтения
		item.read = ::move(client);
	}
	// Устанавливаем флаг наличия ключей чтения
	item.hasRead = true;
	// Устанавливаем флаг наличия ключей записи
	item.hasWrite = true;
	// Выводим положительный результат
	return true;
}

/**
 * @brief Метод начала хендшейка
 *
 * @return результат начала хендшейка (OK/ERROR)
 *
 */
awh::quic::status_t awh::quic::Handshake::start() noexcept {
	// Если хендшейк уже начат либо не установлены транспортные параметры
	if((this->_state != state_t::NONE) || this->_params.empty()){
		// Записываем ошибку в лог
		awh::log::print("QUIC TLS start rejected: %s", awh::log::flag_t::CRITICAL,
		 (this->_state != state_t::NONE ? "handshake is already started" : "transport parameters are not set"));
		// Выводим отрицательный результат
		return status_t::ERROR;
	}
	// Если связь не создана
	if((this->_client == nullptr) && (this->_server == nullptr)){
		// Устанавливаем состояние ошибки хендшейка
		this->_state = state_t::FAILED;
		// Записываем ошибку в лог
		awh::log::print("QUIC TLS context is not set", awh::log::flag_t::CRITICAL);
		// Выводим отрицательный результат
		return status_t::ERROR;
	}
	// Параметры переноса связи
	const vector <uint8_t> parameters(this->_params.begin(), this->_params.end());
	// Если локальный эндпоинт является клиентом
	if(this->_endpoint == endpoint_t::CLIENT){
		// Устанавливаем локальные транспортные параметры
		this->_client->parameters(parameters);
		// Если сессия возобновления установлена
		if(!this->_session.empty()){
			// Билет возобновления связи
			nts::context::ticket_t ticket;
			// Если запись сессии разобрана в билет
			if(nts::context::decode(nts::view_t(reinterpret_cast <const uint8_t *> (this->_session.data()), this->_session.size()), ticket) == nts::context::error_t::NONE)
				// Подаём билет возобновления связи
				this->_client->resume(::move(ticket));
			// Иначе записываем предупреждение в лог
			else awh::log::print("QUIC session is not restored", awh::log::flag_t::WARNING);
		}
		// Заводим связь
		if(this->_client->start() != nts::context::error_t::NONE){
			// Устанавливаем состояние ошибки хендшейка
			this->_state = state_t::FAILED;
			// Записываем ошибку в лог
			awh::log::print("QUIC TLS connection is not started", awh::log::flag_t::CRITICAL);
			// Выводим отрицательный результат
			return status_t::ERROR;
		}
	// Если локальный эндпоинт является сервером
	} else {
		// Устанавливаем локальные транспортные параметры
		this->_server->parameters(parameters);
		// Заводим связь
		if(this->_server->start() != nts::context::error_t::NONE){
			// Устанавливаем состояние ошибки хендшейка
			this->_state = state_t::FAILED;
			// Записываем ошибку в лог
			awh::log::print("QUIC TLS connection is not started", awh::log::flag_t::CRITICAL);
			// Выводим отрицательный результат
			return status_t::ERROR;
		}
	}
	// Устанавливаем состояние выполнения хендшейка
	this->_state = state_t::PROCESS;
	// Если локальный эндпоинт является клиентом
	if(this->_endpoint == endpoint_t::CLIENT)
		// Продвигаем хендшейк для формирования ClientHello и выводим результат
		return this->process();
	// Выводим положительный результат - сервер ожидает данных клиента
	return status_t::OK;
}

/**
 * @brief Метод обработки входящих данных CRYPTO-фреймов по связи
 *
 * @param ctx   связь TLS
 * @param level уровень шифрования пакета с CRYPTO-фреймом
 * @param data  данные CRYPTO-фрейма
 * @param size  размер данных CRYPTO-фрейма
 * @return      результат обработки (OK/ERROR)
 *
 */
template <typename T>
awh::quic::status_t awh::quic::Handshake::cryptoImpl(T & ctx, const level_t level, const uint8_t * data, const size_t size) noexcept {
	// Передаём данные CRYPTO-фрейма связи
	if(ctx.push(::__awh_level_to_nts__(level), nts::view_t(data, size)) != nts::context::error_t::NONE){
		// Устанавливаем состояние ошибки хендшейка
		this->_state = state_t::FAILED;
		// Выводим отрицательный результат
		return status_t::ERROR;
	}
	// Продвигаем хендшейк и выводим результат
	return this->processImpl(ctx);
}

/**
 * @brief Метод обработки входящих данных CRYPTO-фреймов
 *
 * @param level уровень шифрования пакета с CRYPTO-фреймом
 * @param data  данные CRYPTO-фрейма
 * @param size  размер данных CRYPTO-фрейма
 * @return      результат обработки (OK/ERROR)
 *
 */
awh::quic::status_t awh::quic::Handshake::crypto(const level_t level, const uint8_t * data, const size_t size) noexcept {
	// Если хендшейк не начат либо завершился ошибкой
	if((this->_state == state_t::NONE) || (this->_state == state_t::FAILED))
		// Выводим отрицательный результат
		return status_t::ERROR;
	// Если локальный эндпоинт является клиентом
	if(this->_endpoint == endpoint_t::CLIENT)
		// Обрабатываем данные CRYPTO-фрейма по связи клиента
		return this->cryptoImpl(* this->_client, level, data, size);
	// Обрабатываем данные CRYPTO-фрейма по связи сервера
	return this->cryptoImpl(* this->_server, level, data, size);
}

/**
 * @brief Метод проверки наличия исходящих CRYPTO-данных уровня
 *
 * @param level уровень шифрования
 * @return      результат проверки (true - есть данные для отправки)
 *
 */
bool awh::quic::Handshake::pending(const level_t level) const noexcept {
	// Если локальный эндпоинт является клиентом
	if(this->_endpoint == endpoint_t::CLIENT)
		// Выводим результат проверки наличия исходящих CRYPTO-данных уровня
		return (this->_client != nullptr) && (this->_client->output(::__awh_level_to_nts__(level)).size > 0);
	// Выводим результат проверки наличия исходящих CRYPTO-данных уровня
	return (this->_server != nullptr) && (this->_server->output(::__awh_level_to_nts__(level)).size > 0);
}

/**
 * @brief Метод извлечения исходящих CRYPTO-данных уровня
 *
 * @param level уровень шифрования
 * @return      исходящие CRYPTO-данные уровня
 *
 */
string awh::quic::Handshake::data(const level_t level) noexcept {
	// Результат работы функции
	string result = "";
	// Уровень защиты A-NTS
	const nts::context::level_t ntsLevel = ::__awh_level_to_nts__(level);
	// Если локальный эндпоинт является клиентом
	if(this->_endpoint == endpoint_t::CLIENT){
		// Получаем накопленные байты уровня
		const nts::view_t view = this->_client->output(ntsLevel);
		// Если байты накоплены
		if(view.size > 0){
			// Переносим байты в результат
			result.assign(reinterpret_cast <const char *> (view.data), view.size);
			// Снимаем отправленные байты с накопителя
			this->_client->consume(ntsLevel, view.size);
		}
	// Если локальный эндпоинт является сервером
	} else {
		// Получаем накопленные байты уровня
		const nts::view_t view = this->_server->output(ntsLevel);
		// Если байты накоплены
		if(view.size > 0){
			// Переносим байты в результат
			result.assign(reinterpret_cast <const char *> (view.data), view.size);
			// Снимаем отправленные байты с накопителя
			this->_server->consume(ntsLevel, view.size);
		}
	}
	// Выводим результат
	return result;
}

/**
 * @brief Метод извлечения ключей защиты исходящих пакетов уровня
 *
 * @param level уровень шифрования
 * @return      ключи защиты пакетов либо nullptr если ключи ещё не выведены
 *
 */
const awh::quic::crypto::keys_t * awh::quic::Handshake::encryption(const level_t level) const noexcept {
	// Получаем состояние уровня шифрования
	const auto & item = this->_levels[static_cast <size_t> (level)];
	// Выводим ключи защиты исходящих пакетов уровня
	return (item.hasWrite ? &item.write : nullptr);
}

/**
 * @brief Метод извлечения ключей снятия защиты входящих пакетов уровня
 *
 * @param level уровень шифрования
 * @return      ключи защиты пакетов либо nullptr если ключи ещё не выведены
 *
 */
const awh::quic::crypto::keys_t * awh::quic::Handshake::decryption(const level_t level) const noexcept {
	// Получаем состояние уровня шифрования
	const auto & item = this->_levels[static_cast <size_t> (level)];
	// Выводим ключи снятия защиты входящих пакетов уровня
	return (item.hasRead ? &item.read : nullptr);
}

/**
 * @brief Метод замены ключей уровня шифрования (RFC 9001 §6)
 *
 * @param level уровень шифрования
 * @param read  ключи снятия защиты входящих пакетов уровня
 * @param write ключи защиты исходящих пакетов уровня
 *
 */
void awh::quic::Handshake::install(const level_t level, const crypto::keys_t & read, const crypto::keys_t & write) noexcept {
	// Получаем состояние уровня шифрования
	auto & item = this->_levels[static_cast <size_t> (level)];
	// Устанавливаем ключи снятия защиты входящих пакетов уровня
	item.read = read;
	// Устанавливаем ключи защиты исходящих пакетов уровня
	item.write = write;
	// Устанавливаем флаг наличия ключей чтения
	item.hasRead = true;
	// Устанавливаем флаг наличия ключей записи
	item.hasWrite = true;
}

/**
 * @brief Метод сброса ключей уровня шифрования (RFC 9001 §4.9)
 *
 * @param level уровень шифрования
 *
 */
void awh::quic::Handshake::discard(const level_t level) noexcept {
	// Получаем состояние уровня шифрования
	auto & item = this->_levels[static_cast <size_t> (level)];
	// Сбрасываем флаг наличия ключей чтения
	item.hasRead = false;
	// Сбрасываем флаг наличия ключей записи
	item.hasWrite = false;
	// Очищаем ключи снятия защиты входящих пакетов
	item.read = crypto::keys_t();
	// Очищаем ключи защиты исходящих пакетов
	item.write = crypto::keys_t();
	// Очищаем очередь исходящих CRYPTO-данных уровня
	item.data.clear();
}

/**
 * @brief Метод получения состояния хендшейка
 *
 * @return состояние хендшейка
 *
 */
awh::quic::Handshake::state_t awh::quic::Handshake::state() const noexcept {
	// Выводим состояние хендшейка
	return this->_state;
}

/**
 * @brief Метод получения кода ошибки транспорта (RFC 9001 §4.8)
 *
 * @return код ошибки транспорта (NO_ERROR - ошибки нет)
 *
 */
awh::quic::error_t awh::quic::Handshake::error() const noexcept {
	// Если связь оборвана фатальным извещением
	if((this->_endpoint == endpoint_t::CLIENT) && (this->_client != nullptr) && nts::alert::fatal(this->_client->alert()))
		// Выводим код ошибки транспорта диапазона CRYPTO_ERROR (0x0100 + код алерта)
		return static_cast <error_t> (static_cast <uint64_t> (error_t::CRYPTO_ERROR) + static_cast <uint64_t> (this->_client->alert().description));
	// Если связь оборвана фатальным извещением
	if((this->_endpoint == endpoint_t::SERVER) && (this->_server != nullptr) && nts::alert::fatal(this->_server->alert()))
		// Выводим код ошибки транспорта диапазона CRYPTO_ERROR (0x0100 + код алерта)
		return static_cast <error_t> (static_cast <uint64_t> (error_t::CRYPTO_ERROR) + static_cast <uint64_t> (this->_server->alert().description));
	// Если хендшейк завершился ошибкой
	if(this->_state == state_t::FAILED)
		// Выводим код внутренней ошибки реализации
		return error_t::INTERNAL_ERROR;
	// Выводим код отсутствия ошибки
	return error_t::NO_ERROR;
}

/**
 * @brief Конструктор
 *
 * @param endpoint роль локального эндпоинта на соединении
 * @param ctx      идентификатор шаблона контекста безопасности
 *
 */
awh::quic::Handshake::Handshake(const endpoint_t endpoint, const tls::coder::id_t ctx) noexcept :
 _endpoint(endpoint), _state(state_t::NONE),
 _params{""}, _early{""}, _session{""}, _ticket{""}, _rejected(false), _protocols(tls::coder::protocols(ctx)) {
	// Создаём связь TLS по уровням из шаблона контекста безопасности
	void * context = tls::coder::quicContext(ctx);
	// Если связь создана
	if(context != nullptr){
		// Если локальный эндпоинт является клиентом
		if(endpoint == endpoint_t::CLIENT)
			// Принимаем связь клиента во владение
			this->_client.reset(reinterpret_cast <nts::client::context_t *> (context));
		// Если локальный эндпоинт является сервером
		else this->_server.reset(reinterpret_cast <nts::server::context_t *> (context));
	}
}

/**
 * @brief Деструктор
 *
 */
awh::quic::Handshake::~Handshake() noexcept {
	// Связи TLS освобождаются умными указателями
}
