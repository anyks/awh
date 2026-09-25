/**
 * @file: server.cpp
 * @date: 2022-09-03
 * @license: GPL-3.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @copyright: Copyright © 2025
 */

/**
 * Подключаем заголовочный файл
 */
#include <auth/server.hpp>

/**
 * Стандартные модули
 */
#include <mutex>
#include <random>
#include <unordered_map>

/**
 * Подписываемся на стандартное пространство имён
 */
using namespace std;

/**
 * @brief Структура учёта счётчиков запросов nonce
 *
 */
typedef struct DigestCounter {
	uint64_t max;  // Максимальный полученный счётчик запросов
	uint64_t mask; // Маска уже полученных счётчиков ниже максимального
	uint64_t date; // Штамп времени выдачи ключа nonce
	uint64_t used; // Штамп времени последнего использования
	/**
	 * @brief Конструктор
	 *
	 */
	DigestCounter() noexcept : max(0), mask(0), date(0), used(0) {}
} digest_counter_t;
/**
 * Мютекс для блокировки учёта счётчиков запросов
 */
static std::mutex digestMtx;
/**
 * Штамп времени последней очистки устаревших счётчиков запросов
 */
static uint64_t digestSweep = 0;
/**
 * Максимальное количество учитываемых пар nonce и cnonce: cnonce выбирает клиент,
 * поэтому при переполнении вытесняется давно не использованная пара
 */
static constexpr size_t DIGEST_COUNTERS_MAX = 0x4000;
/**
 * Список счётчиков запросов для каждой пары ключей nonce и cnonce
 */
static std::unordered_map <string, digest_counter_t> digestCounters;
/**
 * @brief Функция получения типа хэш-суммы для алгоритма Digest авторизации
 *
 * @param hash алгоритм шифрования Digest авторизации
 * @return     тип хэш-суммы
 */
static awh::hash_t::type_t digestHashType(const awh::Authorization::hash_t hash) noexcept {
	/**
	 * Определяем алгоритм шифрования
	 */
	switch(static_cast <uint8_t> (hash)){
		// Если алгоритм шифрования SHA1
		case static_cast <uint8_t> (awh::Authorization::hash_t::SHA1): return awh::hash_t::type_t::SHA1;
		// Если алгоритм шифрования SHA224
		case static_cast <uint8_t> (awh::Authorization::hash_t::SHA224): return awh::hash_t::type_t::SHA224;
		// Если алгоритм шифрования SHA256
		case static_cast <uint8_t> (awh::Authorization::hash_t::SHA256): return awh::hash_t::type_t::SHA256;
		// Если алгоритм шифрования SHA384
		case static_cast <uint8_t> (awh::Authorization::hash_t::SHA384): return awh::hash_t::type_t::SHA384;
		// Если алгоритм шифрования SHA512
		case static_cast <uint8_t> (awh::Authorization::hash_t::SHA512): return awh::hash_t::type_t::SHA512;
	}
	// Выводим алгоритм шифрования по умолчанию
	return awh::hash_t::type_t::MD5;
}
/**
 * @brief Функция получения секретного ключа подписи nonce
 *
 * Ключ создаётся один раз на процесс. Nonce подписывается им, поэтому сервер проверяет
 * свой nonce на любом подключении и в любом потоке HTTP/2, не храня выданные ключи
 *
 * @return секретный ключ подписи
 */
static const string & digestSecret() noexcept {
	/**
	 * Создаём секретный ключ подписи
	 */
	static const string secret = []() noexcept -> string {
		// Результат работы функции
		string result = "";
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Создаём генератор случайных чисел
			random_device rd;
			// Выполняем сборку случайного ключа
			for(uint8_t i = 0; i < 8; i++)
				// Добавляем очередную часть ключа
				result.append(std::to_string(rd()));
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception &) {
			// Выполняем очистку ключа
			result.clear();
		}
		// Добавляем в ключ текущее время и адрес переменной
		result.append(std::to_string(chrono::steady_clock::now().time_since_epoch().count()));
		// Добавляем в ключ адрес переменной
		result.append(std::to_string(reinterpret_cast <uintptr_t> (&result)));
		// Выводим результат
		return result;
	}();
	// Выводим секретный ключ
	return secret;
}
/**
 * @brief Функция разбора шестнадцатеричного числа
 *
 * @param text   текст для разбора
 * @param result результат разбора
 * @return       результат проверки корректности числа
 */
static bool digestHex(const string & text, uint64_t & result) noexcept {
	// Выполняем сброс результата
	result = 0;
	// Если текст пустой или слишком длинный
	if(text.empty() || (text.size() > 16))
		// Выводим результат
		return false;
	// Выполняем перебор всех символов
	for(auto & c : text){
		// Если символ является цифрой
		if((c >= '0') && (c <= '9'))
			// Добавляем цифру
			result = ((result << 4) | static_cast <uint64_t> (c - '0'));
		// Если символ является буквой в нижнем регистре
		else if((c >= 'a') && (c <= 'f'))
			// Добавляем цифру
			result = ((result << 4) | static_cast <uint64_t> (c - 'a' + 10));
		// Если символ является буквой в верхнем регистре
		else if((c >= 'A') && (c <= 'F'))
			// Добавляем цифру
			result = ((result << 4) | static_cast <uint64_t> (c - 'A' + 10));
		// Если символ не является шестнадцатеричной цифрой
		else return false;
	}
	// Выводим результат
	return true;
}
/**
 * @brief Функция создания подписанного ключа nonce
 *
 * @param hash объект хэширования
 * @param type тип хэш-суммы
 * @param date штамп времени выдачи ключа
 * @return     ключ nonce
 */
static string digestNonce(const awh::hash_t & hash, const awh::hash_t::type_t type, const uint64_t date) noexcept {
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		// Буфер для штампа времени
		char buffer[17];
		// Формируем штамп времени в шестнадцатеричном виде
		::snprintf(buffer, sizeof(buffer), "%016llx", static_cast <unsigned long long> (date));
		// Получаем штамп времени
		const string stamp(buffer, 16);
		// Выводим ключ: штамп времени и подпись
		return (stamp + hash.hmac <string> (digestSecret(), stamp, type));
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception &) {
		// Выводим пустое значение
		return "";
	}
}
/**
 * @brief Функция проверки подписанного ключа nonce
 *
 * @param hash  объект хэширования
 * @param type  тип хэш-суммы
 * @param nonce ключ nonce для проверки
 * @param date  штамп времени выдачи ключа
 * @return      результат проверки подписи
 */
static bool digestNonce(const awh::hash_t & hash, const awh::hash_t::type_t type, const string & nonce, uint64_t & date) noexcept {
	// Выполняем сброс штампа времени
	date = 0;
	// Если ключ имеет минимальный размер
	if(nonce.size() > 16){
		// Если штамп времени получен
		if(digestHex(nonce.substr(0, 16), date))
			// Выполняем сравнение ключа с ожидаемым
			return (digestNonce(hash, type, date) == nonce);
	}
	// Выводим результат
	return false;
}
/**
 * @brief Функция учёта счётчика запросов ключа nonce
 *
 * Счётчик ведётся для пары nonce и cnonce: клиенты (например curl) начинают nc с единицы
 * для каждого нового cnonce, а перехваченный заголовок повторяется целиком вместе со своим cnonce.
 * В паре принимается любой ещё не использованный счётчик в окне 64 значений ниже максимального;
 * повтор тройки nonce, cnonce и nc, а также счётчик за пределами окна отклоняются
 *
 * @param nonce  ключ nonce
 * @param cnonce ключ клиента cnonce
 * @param nc     счётчик запроса
 * @param date   штамп времени выдачи ключа
 * @param now    текущий штамп времени
 * @return       результат проверки, что счётчик не использовался
 */
static bool digestCount(const string & nonce, const string & cnonce, const uint64_t nc, const uint64_t date, const uint64_t now) noexcept {
	// Если счётчик нулевой
	if(nc == 0)
		// Выводим результат
		return false;
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		// Выполняем блокировку потока
		const lock_guard <std::mutex> lock(digestMtx);
		// Если пришло время очистки устаревших счётчиков
		if((now - digestSweep) >= 60000){
			// Запоминаем время очистки
			digestSweep = now;
			// Выполняем перебор всех счётчиков
			for(auto i = digestCounters.begin(); i != digestCounters.end();){
				// Если время жизни ключа истекло
				if((now - i->second.date) >= DIGEST_ALIVE_NONCE)
					// Удаляем устаревший счётчик
					i = digestCounters.erase(i);
				// Переходим к следующему счётчику
				else ++i;
			}
		}
		// Формируем ключ пары nonce и cnonce
		const string key = (nonce + '\n' + cnonce);
		// Выполняем поиск счётчика пары
		auto i = digestCounters.find(key);
		// Если счётчик пары ещё не создан
		if(i == digestCounters.end()){
			// Если список счётчиков переполнен
			if(digestCounters.size() >= DIGEST_COUNTERS_MAX){
				// Самый давно использованный счётчик
				auto j = digestCounters.begin();
				// Выполняем поиск самого давно использованного счётчика
				for(auto k = digestCounters.begin(); k != digestCounters.end(); ++k){
					// Если счётчик использовался раньше
					if(k->second.used < j->second.used)
						// Запоминаем счётчик
						j = k;
				}
				// Вытесняем самый давно использованный счётчик
				digestCounters.erase(j);
			}
			// Создаём счётчик пары
			i = digestCounters.emplace(key, digest_counter_t()).first;
		}
		// Получаем счётчик пары
		digest_counter_t & counter = i->second;
		// Запоминаем штамп времени выдачи ключа
		counter.date = date;
		// Запоминаем время использования
		counter.used = now;
		// Если счётчик больше максимального
		if(nc > counter.max){
			// Получаем величину сдвига окна
			const uint64_t shift = (nc - counter.max);
			// Выполняем сдвиг окна
			counter.mask = ((shift >= 64) ? 0 : (counter.mask << shift));
			// Помечаем счётчик как использованный
			counter.mask |= 1;
			// Запоминаем максимальный счётчик
			counter.max = nc;
			// Выводим результат
			return true;
		}
		// Получаем отставание счётчика от максимального
		const uint64_t diff = (counter.max - nc);
		// Если счётчик за пределами окна
		if(diff >= 64)
			// Выводим результат
			return false;
		// Если счётчик уже использовался
		if((counter.mask & (static_cast <uint64_t> (1) << diff)) != 0)
			// Выводим результат
			return false;
		// Помечаем счётчик как использованный
		counter.mask |= (static_cast <uint64_t> (1) << diff);
		// Выводим результат
		return true;
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception &) {
		// Выводим результат
		return false;
	}
}

/**
 * @brief Метод извлечения данных авторизации
 *
 * @return данные модуля авторизации
 */
awh::server::Auth::data_t awh::server::Auth::data() const noexcept {
	// Результат работы функции
	data_t result;
	// Выполняем установку типа авторизации
	result.type = &this->_type;
	// Выполняем установку параметров Digest авторизации
	result.digest = &this->_digest;
	// Выполняем установку пользовательских параметров Digest авторизации
	result.locale = &this->_locale;
	// Выполняем установку логина пользователя
	result.user = &this->_user;
	// Выполняем установку пароля пользователя
	result.pass = &this->_pass;
	// Выводим результат
	return result;
}
/**
 * @brief Метод установки данных авторизации
 *
 * @param data данные авторизации для установки
 */
void awh::server::Auth::data(const data_t & data) noexcept {
	// Если данные переданы
	if((data.type != nullptr) && (data.digest != nullptr) && (data.locale != nullptr) && (data.user != nullptr) && (data.pass != nullptr)){
		// Выполняем установку типа авторизации
		this->_type = (* data.type);
		// Выполняем установку параметров Digest авторизации
		this->_digest = (* data.digest);
		// Выполняем установку пользовательских параметров Digest авторизации
		this->_locale = (* data.locale);
		// Выполняем установку логина пользователя
		this->_user.assign(data.user->begin(), data.user->end());
		// Выполняем установку пароля пользователя
		this->_pass.assign(data.pass->begin(), data.pass->end());
	}
}
/**
 * @brief Метод проверки авторизации
 *
 * @param method метод HTTP запроса
 * @return       результат проверки авторизации
 */
bool awh::server::Auth::check(const string & method) noexcept {
	// Результат работы функции
	bool result = false;
	// Если адрес запроса задан, а ответ Digest рассчитан для другого адреса
	if((this->_type == type_t::DIGEST) && !this->_target.empty() && !this->_locale.uri.empty() &&
	   (this->_target.compare(this->target(this->_uri.parse(this->_locale.uri))) != 0))
		// Ответ для чужого адреса не принимаем
		return result;
	/**
	 * Определяем тип авторизации
	 */
	switch(static_cast <uint8_t> (this->_type)){
		// Если тип авторизации - Базовая
		case static_cast <uint8_t> (type_t::BASIC): {
			// Если функция обратного вызова установлена
			if(this->_callback.is("auth"))
				// Выполняем проверку авторизации
				return this->_callback.call <bool (const string &, const string &)> ("auth", this->_user, this->_pass);
		} break;
		// Если тип авторизации - Дайджест
		case static_cast <uint8_t> (type_t::DIGEST): {
			// Если данные пользователя переданы
			if(!method.empty() && !this->_user.empty() && !this->_locale.nc.empty() && !this->_locale.uri.empty() && !this->_locale.nonce.empty() && !this->_locale.cnonce.empty() && !this->_locale.resp.empty()){
				// Счётчик запросов клиента
				uint64_t nc = 0;
				// Если счётчик клиента корректный и функция извлечения пароля установлена
				if(digestHex(this->_locale.nc, nc) && (nc > 0) && this->_callback.is("extract")){
					// Получаем пароль пользователя
					const string & pass = this->_callback.call <string (const string &)> ("extract", this->_user);
					// Если пароль пользователя получен
					if(!pass.empty()){
						// Параметры проверки дайджест авторизации
						digest_t digest;
						/**
						 * Ответ считается по realm и qop самого сервера,
						 * а nonce и opaque проверяются на выдачу этим сервером ниже
						 */
						digest.nc     = this->_locale.nc;
						digest.hash   = this->_digest.hash;
						digest.uri    = this->_locale.uri;
						digest.qop    = this->_digest.qop;
						digest.realm  = this->_digest.realm;
						digest.nonce  = this->_locale.nonce;
						digest.opaque = this->_locale.opaque;
						digest.cnonce = this->_locale.cnonce;
						// Если ответ клиента соответствует паролю
						if(this->_fmk->compare(this->response(this->_fmk->transform(string(method), fmk_t::transform_t::UPPER), this->_user, pass, digest), this->_locale.resp)){
							// Штамп времени выдачи ключа
							uint64_t date = 0;
							// Получаем текущее значение штампа времени
							const uint64_t now = this->_fmk->timestamp <uint64_t> (fmk_t::chrono_t::MILLISECONDS);
							// Если ключ выдан этим сервером и его время жизни не истекло
							if(digestNonce(this->_hash, digestHashType(this->_digest.hash), this->_locale.nonce, date) && (date <= now) && ((now - date) < DIGEST_ALIVE_NONCE)){
								// Если ключ сессии сервера ещё не создан
								if(this->_digest.opaque.empty())
									// Создаём ключ сервера так же, как при выдаче запроса авторизации
									this->_hash.hashing(AWH_SITE, digestHashType(this->_digest.hash), this->_digest.opaque);
								// Если ключ сессии сервера совпадает с выданным
								if(!this->_digest.opaque.empty() && (this->_digest.opaque.compare(this->_locale.opaque) == 0)){
									// Если счётчик запросов ранее не использовался
									if((result = digestCount(this->_locale.nonce, this->_locale.cnonce, nc, date, now)))
										// Запоминаем счётчик клиента
										this->_digest.nc = this->_locale.nc;
								// Сообщаем клиенту, что необходимо повторить запрос с новыми ключами
								} else this->_stale = true;
							// Сообщаем клиенту, что ключ устарел и необходимо повторить запрос с новым ключом
							} else this->_stale = true;
						}
					}
				}
			}
		} break;
	}
	// Выводим результат
	return result;
}
/**
 * @brief Метод приведения адреса запроса к виду для сверки
 *
 * @param url адрес запроса
 * @return    путь с завершающим слэшем и параметрами в едином кодировании
 */
string awh::server::Auth::target(const uri_t::url_t & url) const noexcept {
	// Выполняем сборку пути запроса
	string result = this->_uri.joinPath(url.path);
	// Если путь оканчивается слэшем
	if(url.trailing && !url.path.empty())
		// Добавляем завершающий слэш
		result.append(1, '/');
	// Добавляем параметры запроса
	result.append(this->_uri.joinParams(url.params));
	// Выводим результат
	return result;
}
/**
 * @brief Метод установки адреса запроса для сверки с uri из ответа Digest
 *
 * @param url адрес запроса
 */
void awh::server::Auth::uri(const uri_t::url_t & url) noexcept {
	// Если адрес не передан, сверка отключается
	if(url.empty())
		// Очищаем адрес запроса
		this->_target.clear();
	// Запоминаем адрес запроса в виде для сверки
	else this->_target = this->target(url);
}
/**
 * @brief Метод установки название сервера
 *
 * @param realm название сервера
 */
void awh::server::Auth::realm(const string & realm) noexcept {
	// Если название сервера передано
	if(!realm.empty())
		// Выполняем установку название сервера
		this->_digest.realm = realm;
}
/**
 * @brief Метод установки временного ключа сессии сервера
 *
 * @param opaque временный ключ сессии сервера
 */
void awh::server::Auth::opaque(const string & opaque) noexcept {
	// Если временный ключ сессии сервера передан
	if(!opaque.empty())
		// Выполняем установку временного ключа сессии сервера
		this->_digest.opaque = opaque;
}
/**
 * @brief Метод добавления функции извлечения пароля
 *
 * @param callback функция обратного вызова для извлечения пароля
 */
void awh::server::Auth::extractPassCallback(function <string (const string &)> callback) noexcept {
	// Устанавливаем функцию обратного вызова
	this->_callback.on <string (const string &)> ("extract", callback);
}
/**
 * @brief Метод добавления функции обработки авторизации
 *
 * @param callback функция обратного вызова для обработки авторизации
 */
void awh::server::Auth::authCallback(function <bool (const string &, const string &)> callback) noexcept {
	// Устанавливаем функцию обратного вызова
	this->_callback.on <bool (const string &, const string &)> ("auth", callback);
}
/**
 * @brief Метод установки параметров авторизации из заголовков
 *
 * @param header заголовок HTTP с параметрами авторизации
 */
void awh::server::Auth::header(const string & header) noexcept {
	// Если заголовок передан
	if(!header.empty() && (this->_fmk != nullptr)){
		/**
		 * Определяем тип авторизации
		 */
		switch(static_cast <uint8_t> (this->_type)){
			// Если тип авторизации Digest
			case static_cast <uint8_t> (type_t::DIGEST): {
				/**
				 * Выполняем отлов ошибок
				 */
				try {
					// Выполняем сброс логина пользователя
					this->_user.clear();
					// Выполняем сброс параметров Digest авторизации пользователя
					this->_locale = digest_t();
					// Тип авторизации на сервере
					const string type = "Digest";
					// Выполняем поиск схемы авторизации без учёта регистра (RFC 7235: digest и Digest — одна схема)
					string lower = header;
					// Переводим копию заголовка в нижний регистр (сам заголовок не трогаем: nonce и response чувствительны к регистру)
					this->_fmk->transform(lower, fmk_t::transform_t::LOWER);
					// Позиция схемы авторизации
					size_t pos = lower.find("digest");
					// Если авторизация получена
					if((pos != string::npos) && ((pos + type.length()) < header.length())){
						// Получаем параметры авторизации
						const string & digest = header.substr(pos + type.length() + 1);
						// Если параметры дайджест авторизации получены
						if(!digest.empty()){
							// Переходим по всем параметрам (значения в кавычках могут содержать запятые: uri="/api?ids=1,2")
							for(auto & param : this->params(digest)){
								// Получаем ключ параметра
								const string & key = param.first;
								// Получаем значение параметра
								const string & value = param.second;
								// Если параметр является именем пользователя
								if(this->_fmk->compare(key, "username"))
									// Получаем логин пользователя
									this->_user = value;
								// Если параметр является идентификатором сайта
								else if(this->_fmk->compare(key, "realm"))
									// Устанавливаем relam
									this->_locale.realm = value;
								// Если параметр является ключём сгенерированным сервером
								else if(this->_fmk->compare(key, "nonce"))
									// Устанавливаем nonce
									this->_locale.nonce = value;
								// Если параметр являеются параметры запроса
								else if(this->_fmk->compare(key, "uri"))
									// Устанавливаем uri
									this->_locale.uri = value;
								// Если параметр является ключём сгенерированным клиентом
								else if(this->_fmk->compare(key, "cnonce"))
									// Устанавливаем cnonce
									this->_locale.cnonce = value;
								// Если параметр является ключём ответа клиента
								else if(this->_fmk->compare(key, "response"))
									// Устанавливаем response
									this->_locale.resp = value;
								// Если параметр является ключём сервера
								else if(this->_fmk->compare(key, "opaque"))
									// Устанавливаем opaque
									this->_locale.opaque = value;
								// Если параметр является типом авторизации
								else if(this->_fmk->compare(key, "qop"))
									// Устанавливаем qop
									this->_locale.qop = value;
								// Если параметр является счётчиком запросов
								else if(this->_fmk->compare(key, "nc"))
									// Устанавливаем nc
									this->_locale.nc = value;
							}
						}
					}
				/**
				 * Если возникает ошибка
				 */
				} catch(const exception & error) {
					// Выполняем сброс логина пользователя
					this->_user.clear();
					// Выполняем сброс параметров Digest авторизации пользователя
					this->_locale = digest_t();
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Выводим сообщение об ошибке
						this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(header), log_t::flag_t::CRITICAL, error.what());
					/**
					* Если режим отладки не включён
					*/
					#else
						// Выводим сообщение об ошибке
						this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
					#endif
				}
			} break;
			// Если тип авторизации Basic
			case static_cast <uint8_t> (type_t::BASIC): {
				/**
				 * Выполняем отлов ошибок
				 */
				try {
					// Выполняем сброс логина пользователя
					this->_user.clear();
					// Выполняем сброс пароля пользователя
					this->_pass.clear();
					// Тип авторизации на сервере
					const string type = "Basic";
					// Выполняем поиск схемы авторизации без учёта регистра (RFC 7235: digest и Digest — одна схема)
					string lower = header;
					// Переводим копию заголовка в нижний регистр (сам заголовок не трогаем: nonce и response чувствительны к регистру)
					this->_fmk->transform(lower, fmk_t::transform_t::LOWER);
					// Позиция схемы авторизации
					size_t pos = lower.find("basic");
					// Если авторизация получена
					if((pos != string::npos) && ((pos + type.length()) < header.length())){
						// Получаем значение заголовка для дешифрования
						const string & value = this->_fmk->transform(header.substr(pos + type.length() + 1), fmk_t::transform_t::TRIM);
						// Если значение получено
						if(!value.empty()){
							// Выполняем шифрование полезной нагрузки
							const string & result = this->_hash.decode <string> (value.data(), value.size(), awh::hash_t::cipher_t::BASE64);
							// Если хэш получен
							if(!result.empty()){
								// Выполняем поиск разделителя
								pos = result.find(":");
								// Если разделитель получен
								if(pos != string::npos){
									// Записываем полученный логин клиента
									this->_user = result.substr(0, pos);
									// Записываем полученный пароль клиента
									this->_pass = result.substr(pos + 1);
								}
							}
						}
					}
				/**
				 * Если возникает ошибка
				 */
				} catch(const exception & error) {
					// Выполняем сброс логина пользователя
					this->_user.clear();
					// Выполняем сброс пароля пользователя
					this->_pass.clear();
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Выводим сообщение об ошибке
						this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(header), log_t::flag_t::CRITICAL, error.what());
					/**
					* Если режим отладки не включён
					*/
					#else
						// Выводим сообщение об ошибке
						this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
					#endif
				}
			} break;
		}
	}
}
/**
 * @brief Оператор вывода строки авторизации
 *
 * @return строка авторизации
 */
awh::server::Auth::operator string() noexcept {
	// Результат работы функции
	string result = "";
	// Если фреймворк установлен
	if(this->_fmk != nullptr){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			/**
			 * Определяем тип авторизации
			 */
			switch(static_cast <uint8_t> (this->_type)){
				// Если тип авторизации Digest
				case static_cast <uint8_t> (type_t::DIGEST): {
					// Флаг нужно ли клиенту повторить запрос
					string stale = "FALSE";
					// Алгоритм шифрования
					string algorithm = "MD5";
					// Флаг создания нового ключа nonce
					bool createNonce = false;
					// Получаем текущее значение штампа времени
					const uint64_t date = this->_fmk->timestamp <uint64_t> (fmk_t::chrono_t::MILLISECONDS);
					// Если ключ клиента не создан или прошло времени больше 30-ти минут
					if((createNonce = (this->_digest.nonce.empty() || ((date - this->_digest.date) >= DIGEST_ALIVE_NONCE)))){
						// Устанавливаем штамп времени
						this->_digest.date = date;
						// Если ключ клиента, ещё небыл сгенерирован
						if(!this->_digest.nonce.empty())
							// Выполняем установку полученного значения
							stale = "TRUE";
					}
					// Если клиент прислал верный ответ на чужой или устаревший ключ
					if(this->_stale){
						// Снимаем флаг устаревшего ключа
						this->_stale = false;
						// Сообщаем клиенту, что достаточно повторить запрос с новым ключом
						stale = "TRUE";
						// Выполняем создание нового ключа
						createNonce = true;
						// Устанавливаем штамп времени
						this->_digest.date = date;
					}
					/**
					 * Определяем алгоритм шифрования
					 */
					switch(static_cast <uint16_t> (this->_digest.hash)){
						// Если алгоритм шифрования MD5
						case static_cast <uint16_t> (hash_t::MD5): {
							// Устанавливаем тип шифрования
							algorithm = "MD5";
							// Создаём ключ сервера
							if(this->_digest.opaque.empty())
								// Выполняем установку полученного значения
								this->_hash.hashing(AWH_SITE, awh::hash_t::type_t::MD5, this->_digest.opaque);
						} break;
						// Если алгоритм шифрования SHA1
						case static_cast <uint16_t> (hash_t::SHA1): {
							// Устанавливаем тип шифрования
							algorithm = "SHA1";
							// Создаём ключ сервера
							if(this->_digest.opaque.empty())
								// Выполняем установку полученного значения
								this->_hash.hashing(AWH_SITE, awh::hash_t::type_t::SHA1, this->_digest.opaque);
						} break;
						// Если алгоритм шифрования SHA224
						case static_cast <uint16_t> (hash_t::SHA224): {
							// Устанавливаем тип шифрования
							algorithm = "SHA224";
							// Создаём ключ сервера
							if(this->_digest.opaque.empty())
								// Выполняем установку полученного значения
								this->_hash.hashing(AWH_SITE, awh::hash_t::type_t::SHA224, this->_digest.opaque);
						} break;
						// Если алгоритм шифрования SHA256
						case static_cast <uint16_t> (hash_t::SHA256): {
							// Устанавливаем тип шифрования
							algorithm = "SHA256";
							// Создаём ключ сервера
							if(this->_digest.opaque.empty())
								// Выполняем установку полученного значения
								this->_hash.hashing(AWH_SITE, awh::hash_t::type_t::SHA256, this->_digest.opaque);
						} break;
						// Если алгоритм шифрования SHA384
						case static_cast <uint16_t> (hash_t::SHA384): {
							// Устанавливаем тип шифрования
							algorithm = "SHA384";
							// Создаём ключ сервера
							if(this->_digest.opaque.empty())
								// Выполняем установку полученного значения
								this->_hash.hashing(AWH_SITE, awh::hash_t::type_t::SHA384, this->_digest.opaque);
						} break;
						// Если алгоритм шифрования SHA512
						case static_cast <uint16_t> (hash_t::SHA512): {
							// Устанавливаем тип шифрования
							algorithm = "SHA512";
							// Создаём ключ сервера
							if(this->_digest.opaque.empty())
								// Выполняем установку полученного значения
								this->_hash.hashing(AWH_SITE, awh::hash_t::type_t::SHA512, this->_digest.opaque);
						} break;
					}
					// Название алгоритма по RFC 7616 (SHA-256, а не SHA256: иначе браузеры и curl запрос авторизации не принимают)
					algorithm = this->algorithm(this->_digest.hash);
					// Если требуется создать новый ключ клиента
					if(createNonce){
						/**
						 * Ключ подписывается секретом процесса и содержит время выдачи,
						 * поэтому проверяется на любом подключении и потоке без хранения
						 */
						this->_digest.nonce = digestNonce(this->_hash, digestHashType(this->_digest.hash), date);
						// Выполняем сброс счётчика запросов для нового ключа
						this->_digest.nc = "00000000";
					}
					// Создаём строку запроса авторизации
					result = this->_fmk->format(
						"Digest realm=\"%s\", qop=\"%s\", stale=%s, algorithm=%s, nonce=\"%s\", opaque=\"%s\"",
						this->_digest.realm.c_str(),
						this->_digest.qop.c_str(),
						stale.c_str(), algorithm.c_str(),
						this->_digest.nonce.c_str(),
						this->_digest.opaque.c_str()
					);
				} break;
				// Если тип авторизации Basic
				case static_cast <uint8_t> (type_t::BASIC):
					// Создаём строку запроса авторизации
					result = this->_fmk->format("Basic realm=\"%s\", charset=\"UTF-8\"", "Please login for access");
				break;
			}
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Выводим сообщение об ошибке
				this->_log->debug("%s", __PRETTY_FUNCTION__, {}, log_t::flag_t::CRITICAL, error.what());
			/**
			* Если режим отладки не включён
			*/
			#else
				// Выводим сообщение об ошибке
				this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
			#endif
		}
	}
	// Выводим результат
	return result;
}
