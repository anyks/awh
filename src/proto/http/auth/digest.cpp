/**
 * @file: digest.cpp
 * @date: 2026-07-14
 * @license: GPL-3.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @copyright: Copyright © 2026
 */

/**
 * Стандартные заголовочные файлы
 */
#include <cstdlib>

/**
 * Подключаем заголовочный файл проекта
 */
#include <proto/http/auth/digest.hpp>

/**
 * Используем стандартное пространство имён
 */
using namespace std;

/**
 * @brief Метод перевода алгоритма хэширования в текстовое представление
 *
 * @return название алгоритма хэширования (MD5, SHA-256 и т.д.)
 */
string awh::http::Digest::algorithm() const noexcept {
	// Название базового алгоритма (по умолчанию MD5)
	string result = "MD5";
	/**
	 * Определяем алгоритм хэширования
	 */
	switch(static_cast <uint8_t> (this->_params.hash)){
		// Если алгоритм хэширования MD5
		case static_cast <uint8_t> (crypto_t::hash_t::MD5):
			// Устанавливаем название алгоритма
			result = "MD5";
		break;
		// Если алгоритм хэширования SHA1
		case static_cast <uint8_t> (crypto_t::hash_t::SHA1):
			// Устанавливаем название алгоритма
			result = "SHA1";
		break;
		// Если алгоритм хэширования SHA224
		case static_cast <uint8_t> (crypto_t::hash_t::SHA224):
			// Устанавливаем название алгоритма
			result = "SHA-224";
		break;
		// Если алгоритм хэширования SHA256
		case static_cast <uint8_t> (crypto_t::hash_t::SHA256):
			// Устанавливаем название алгоритма
			result = "SHA-256";
		break;
		// Если алгоритм хэширования SHA384
		case static_cast <uint8_t> (crypto_t::hash_t::SHA384):
			// Устанавливаем название алгоритма
			result = "SHA-384";
		break;
		// Если алгоритм хэширования SHA512
		case static_cast <uint8_t> (crypto_t::hash_t::SHA512):
			// Устанавливаем название алгоритма
			result = "SHA-512";
		break;
	}
	// Если используется сессионный режим - добавляем суффикс -sess
	if(this->_params.digest.sess)
		// Дополняем название алгоритма суффиксом сессионного режима
		result.append("-sess");
	// Выводим результат
	return result;
}
/**
 * @brief Метод расчёта ответа Digest-авторизации
 *
 * @param user логин пользователя
 * @param pass пароль пользователя
 * @return     ответ в шестнадцатеричном виде
 */
string awh::http::Digest::response(const string & user, const string & pass) const noexcept {
	// Результат работы функции
	string result = "";
	// Получаем ссылку на параметры Digest-авторизации
	const auth_t::digest_t & digest = this->_params.digest;
	// Если данные, необходимые для расчёта ответа, переданы
	if(!user.empty() && !pass.empty() && !digest.nonce.empty() && !digest.cnonce.empty() && (this->_crypto != nullptr)){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Определяем алгоритм хэширования
			const crypto_t::hash_t hash = this->_params.hash;
			// Формируем первый этап расчёта: HA1 = H(user:realm:pass)
			string ha1 = ::move(this->_crypto->hash <string> (this->_fmk->format("%s:%s:%s", user.c_str(), digest.realm.c_str(), pass.c_str()), hash));
			// Если используется сессионный режим (-sess): HA1 = H(HA1:nonce:cnonce)
			if(!ha1.empty() && digest.sess)
				// Пересчитываем первый этап с учётом nonce и cnonce
				ha1 = ::move(this->_crypto->hash <string> (this->_fmk->format("%s:%s:%s", ha1.c_str(), digest.nonce.c_str(), digest.cnonce.c_str()), hash));
			// Если первый этап расчёта получен
			if(!ha1.empty()){
				// Формируем второй этап расчёта: HA2 = H(method:uri)
				const string & ha2 = this->_crypto->hash <string> (this->_fmk->format("%s:%s", this->_params.method.c_str(), digest.uri.c_str()), hash);
				// Если второй этап расчёта получен
				if(!ha2.empty())
					// Формируем итоговый ответ: response = H(HA1:nonce:nc:cnonce:qop:HA2)
					result = ::move(this->_crypto->hash <string> (this->_fmk->format("%s:%s:%s:%s:%s:%s", ha1.c_str(), digest.nonce.c_str(), digest.nc.c_str(), digest.cnonce.c_str(), digest.qop.c_str(), ha2.c_str()), hash));
			}
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Записываем ошибку в лог
				this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(user, pass), log_t::flag_t::CRITICAL, error.what());
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Выводим в лог сообщение об ошибке
				this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
			#endif
		}
	}
	// Выводим результат
	return result;
}
/**
 * @brief Метод проверки учётных данных (только для сервера)
 *
 * @return результат проверки
 */
bool awh::http::Digest::check() noexcept {
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		// На стороне клиента проверка не требуется
		if(this->_owner == auth_t::owner_t::CLIENT)
			// Подтверждаем корректность
			return true;
		// Если функция извлечения пароля не установлена или логин/ответ отсутствуют
		if((this->_params.callback.extractPass == nullptr) || this->_params.user.empty() || this->_params.digest.response.empty())
			// Сообщаем о неудачной проверке
			return false;
		// Извлекаем пароль пользователя по логину
		const string & pass = this->_params.callback.extractPass(this->_params.user);
		// Если пароль пользователя не получен
		if(pass.empty())
			// Сообщаем о неудачной проверке
			return false;
		// Получаем ссылку на параметры Digest-авторизации
		const auth_t::digest_t & digest = this->_params.digest;
		/**
		 * Проверяем, что клиент вернул именно тот ключ (nonce), который выдал сервер.
		 * Это отсекает воспроизведение перехваченных учётных данных со старым ключом
		 * на новом соединении, где счётчик nc ещё не накоплен.
		 */
		if(!digest.issued.empty() && (digest.nonce.compare(digest.issued) != 0)){
			// Пишем диагностический лог о несовпадении выданного и полученного ключа сервера
			this->_log->print(
				"Digest auth nonce mismatch for user \"%s\": expected nonce=\"%s\", received nonce=\"%s\"",
				log_t::flag_t::WARNING,
				this->_params.user.c_str(),
				digest.issued.c_str(),
				digest.nonce.c_str()
			);
			// Сообщаем о неудачной проверке
			return false;
		}
		// Выполняем расчёт ожидаемого ответа
		const string & response = this->response(this->_params.user, pass);
		// Если ожидаемый ответ не совпадает с ответом клиента
		if(response.empty() || (response.compare(digest.response) != 0)){
			/**
			 * Пишем диагностический лог: рассинхронизация ожидаемого и полученного response
			 * часто указывает на некорректную реализацию клиента (известны баги Safari/WebKit
			 * с формированием nc/cnonce/qop). Лог помогает быстро определить сторону ошибки.
			 */
			this->_log->print(
				"Digest auth mismatch for user \"%s\": expected response=\"%s\", received=\"%s\" "
				"[algorithm=%s, realm=\"%s\", qop=\"%s\", nonce=\"%s\", cnonce=\"%s\", nc=\"%s\", uri=\"%s\", method=\"%s\"]",
				log_t::flag_t::WARNING,
				this->_params.user.c_str(),
				response.c_str(),
				digest.response.c_str(),
				this->algorithm().c_str(),
				digest.realm.c_str(),
				digest.qop.c_str(),
				digest.nonce.c_str(),
				digest.cnonce.c_str(),
				digest.nc.c_str(),
				digest.uri.c_str(),
				this->_params.method.c_str()
			);
			// Сообщаем о неудачной проверке
			return false;
		}
		// Получаем текущий счётчик запросов клиента (nonce count)
		const uint32_t nc = static_cast <uint32_t> (::strtoul(digest.nc.c_str(), nullptr, 16));
		// Получаем последний принятый сервером счётчик запросов
		const uint32_t last = static_cast <uint32_t> (::strtoul(digest.lnc.c_str(), nullptr, 16));
		/**
		 * Защита от повторного воспроизведения запроса (replay): счётчик nc должен строго
		 * возрастать в пределах одного nonce. Повтор или откат значения означает атаку
		 * либо некорректную работу клиента.
		 */
		if(nc <= last){
			// Пишем диагностический лог о повторном воспроизведении запроса
			this->_log->print(
				"Digest auth replay detected for user \"%s\": received nc=\"%s\", last accepted nc=\"%s\" [nonce=\"%s\"]",
				log_t::flag_t::WARNING,
				this->_params.user.c_str(),
				digest.nc.c_str(),
				digest.lnc.c_str(),
				digest.nonce.c_str()
			);
			// Сообщаем о неудачной проверке
			return false;
		}
		// Фиксируем принятый счётчик запросов как последний подтверждённый
		this->_params.digest.lnc = digest.nc;
		// Подтверждаем успешную проверку учётных данных
		return true;
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Записываем ошибку в лог
			this->_log->debug("%s", __PRETTY_FUNCTION__, {}, log_t::flag_t::CRITICAL, error.what());
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Выводим в лог сообщение об ошибке
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
	// Сообщаем о неудачной проверке
	return false;
}
/**
 * @brief Метод разбора входящего заголовка авторизации
 *
 * @param header значение заголовка (клиент: вызов, сервер: учётные данные)
 * @return       результат разбора
 */
bool awh::http::Digest::parse(const string_view header) noexcept {
	// Результат работы функции
	bool result = false;
	// Если заголовок передан и фреймворк установлен
	if(!header.empty() && (this->_fmk != nullptr)){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Название схемы авторизации
			const string type = "Digest";
			// Выполняем поиск схемы DIGEST-авторизации
			const size_t pos = header.find(type);
			// Если схема авторизации не найдена или после неё нет данных - завершаем разбор
			if((pos == string::npos) || ((pos + type.length() + 1) >= header.length()))
				// Сообщаем о неудачном разборе
				return result;
			// Получаем параметры Digest-авторизации
			const string digest(header.substr(pos + type.length() + 1));
			// Список параметров авторизации
			vector <string> params;
			// Выполняем разделение параметров авторизации
			this->_fmk->split(digest, ",", params);
			// Если список параметров получен
			if((result = !params.empty())){
				/**
				 * Переходим по всему списку параметров
				 */
				for(auto & param : params){
					// Выполняем поиск разделителя «ключ=значение»
					const size_t sep = param.find("=");
					// Если разделитель не найден - переходим к следующему параметру
					if(sep == string::npos)
						// Пропускаем некорректный параметр
						continue;
					// Извлекаем и очищаем ключ параметра
					string key = param.substr(0, sep);
					// Извлекаем и очищаем значение параметра
					string value = param.substr(sep + 1);
					// Удаляем крайние пробелы у ключа
					this->_fmk->transform(key, fmk_t::transform_t::TRIM);
					// Удаляем крайние пробелы у значения
					this->_fmk->transform(value, fmk_t::transform_t::TRIM);
					// Если значение обёрнуто в кавычки - удаляем их
					if((value.length() > 1) && (value.front() == '"') && (value.back() == '"'))
						// Снимаем обрамляющие кавычки
						value = ::move(value.substr(1, value.length() - 2));
					// Если параметр является именем пользователя
					if(key.compare("username") == 0)
						// Устанавливаем логин пользователя
						this->_params.user = ::move(value);
					// Если параметр является идентификатором сервера
					else if(key.compare("realm") == 0)
						// Устанавливаем realm
						this->_params.digest.realm = ::move(value);
					// Если параметр является ключом, выданным сервером
					else if(key.compare("nonce") == 0) {
						// На стороне клиента при смене ключа сервера сбрасываем состояние счётчика
						if((this->_owner == auth_t::owner_t::CLIENT) && (this->_params.digest.nonce != value)){
							// Сбрасываем счётчик запросов (nonce count начинается заново)
							this->_params.digest.nc = "00000000";
							// Сбрасываем ключ клиента для его повторной генерации под новый nonce
							this->_params.digest.cnonce.clear();
						}
						// Устанавливаем nonce
						this->_params.digest.nonce = ::move(value);
					}
					// Если параметр является временным ключом сессии сервера
					else if(key.compare("opaque") == 0)
						// Устанавливаем opaque
						this->_params.digest.opaque = ::move(value);
					// Если параметр является параметрами HTTP-запроса
					else if(key.compare("uri") == 0)
						// Устанавливаем uri
						this->_params.digest.uri = ::move(value);
					// Если параметр является ключом, сгенерированным клиентом
					else if(key.compare("cnonce") == 0)
						// Устанавливаем cnonce
						this->_params.digest.cnonce = ::move(value);
					// Если параметр является ответом клиента
					else if(key.compare("response") == 0)
						// Устанавливаем ответ клиента
						this->_params.digest.response = ::move(value);
					// Если параметр является счётчиком запросов
					else if(key.compare("nc") == 0)
						// Устанавливаем счётчик запросов
						this->_params.digest.nc = ::move(value);
					// Если параметр является типом защиты
					else if(key.compare("qop") == 0){
						// Если тип защиты передан верно
						if(value.find("auth") != string::npos)
							// Устанавливаем тип защиты
							this->_params.digest.qop = "auth";
					// Если параметр является алгоритмом хэширования
					} else if(key.compare("algorithm") == 0) {
						// Приводим название алгоритма к нижнему регистру
						this->_fmk->transform(value, fmk_t::transform_t::LOWER_CASE);
						// Определяем режим сессионного алгоритма (-sess)
						this->_params.digest.sess = (value.find("sess") != string::npos);
						// Удаляем суффикс сессионного режима из названия алгоритма
						value = this->_fmk->replace(value, "sess", "");
						// Удаляем разделитель из названия алгоритма (SHA-256 -> sha256)
						value = this->_fmk->replace(value, "-", "");
						// Если алгоритм является MD5
						if(value.compare("md5") == 0)
							// Устанавливаем алгоритм MD5
							this->_params.hash = crypto_t::hash_t::MD5;
						// Если алгоритм является SHA1
						else if(value.compare("sha1") == 0)
							// Устанавливаем алгоритм SHA1
							this->_params.hash = crypto_t::hash_t::SHA1;
						// Если алгоритм является SHA224
						else if(value.compare("sha224") == 0)
							// Устанавливаем алгоритм SHA224
							this->_params.hash = crypto_t::hash_t::SHA224;
						// Если алгоритм является SHA256
						else if(value.compare("sha256") == 0)
							// Устанавливаем алгоритм SHA256
							this->_params.hash = crypto_t::hash_t::SHA256;
						// Если алгоритм является SHA384
						else if(value.compare("sha384") == 0)
							// Устанавливаем алгоритм SHA384
							this->_params.hash = crypto_t::hash_t::SHA384;
						// Если алгоритм является SHA512
						else if(value.compare("sha512") == 0)
							// Устанавливаем алгоритм SHA512
							this->_params.hash = crypto_t::hash_t::SHA512;
					}
				}
			}
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Записываем ошибку в лог
				this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(header), log_t::flag_t::CRITICAL, error.what());
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Выводим в лог сообщение об ошибке
				this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
			#endif
		}
	}
	// Выводим результат
	return result;
}
/**
 * @brief Метод формирования исходящего заголовка авторизации
 *
 * @param full режим вывода вместе с именем заголовка
 * @return     значение заголовка авторизации
 */
string awh::http::Digest::header(const bool full) noexcept {
	// Результат работы функции
	string result = "";
	// Если фреймворк установлен
	if(this->_crypto != nullptr){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Получаем ссылку на параметры Digest-авторизации
			auth_t::digest_t & digest = this->_params.digest;
			/**
			 * В зависимости от стороны работы формируем заголовок авторизации
			 */
			switch(static_cast <uint8_t> (this->_owner)){
				// На стороне клиента формируем учётные данные (Authorization)
				case static_cast <uint8_t> (auth_t::owner_t::CLIENT): {
					// Продолжаем только если сервер передал необходимые данные вызова
					if(!this->_params.user.empty() && !this->_params.pass.empty() && !digest.nonce.empty()){
						// Если ключ клиента ещё не сгенерирован
						if(digest.cnonce.empty()){
							// Получаем текущий штамп времени в секундах
							const uint64_t stamp = this->_fmk->timestamp <uint64_t> (fmk_t::chrono_t::SECONDS);
							// Генерируем ключ клиента на основе штампа времени
							digest.cnonce = this->_crypto->hash <string> (::to_string(stamp) + this->_params.user, this->_params.hash);
							// Обрезаем ключ клиента до 16 символов
							if(digest.cnonce.length() > 16)
								// Оставляем только первые 16 символов
								digest.cnonce = digest.cnonce.substr(0, 16);
						}
						// Увеличиваем счётчик запросов клиента на каждый новый запрос с текущим nonce
						const uint32_t nc = static_cast <uint32_t> (::strtoul(digest.nc.c_str(), nullptr, 16));
						// Формируем счётчик запросов в виде 8 шестнадцатеричных цифр (nonce count)
						digest.nc = this->_fmk->format("%08x", nc + 1);
						// Выполняем расчёт ответа клиента
						digest.response = this->response(this->_params.user, this->_params.pass);
						// Если ответ клиента рассчитан
						if(!digest.response.empty()){
							// Формируем значение заголовка авторизации
							result = this->_fmk->format(
								"Digest username=\"%s\", realm=\"%s\", nonce=\"%s\", uri=\"%s\", qop=%s, nc=%s, cnonce=\"%s\", response=\"%s\", opaque=\"%s\", algorithm=%s",
								this->_params.user.c_str(),
								digest.realm.c_str(),
								digest.nonce.c_str(),
								digest.uri.c_str(),
								digest.qop.c_str(),
								digest.nc.c_str(),
								digest.cnonce.c_str(),
								digest.response.c_str(),
								digest.opaque.c_str(),
								this->algorithm().c_str()
							);
							// Если требуется вывести заголовок вместе с его именем
							if(full)
								// Дополняем результат именем заголовка
								result = this->_fmk->format("%s: %s\r\n", this->name().c_str(), result.c_str());
						}
					}
				} break;
				// На стороне сервера формируем вызов авторизации (WWW-Authenticate)
				case static_cast <uint8_t> (auth_t::owner_t::SERVER): {
					// Флаг необходимости повторить запрос клиенту
					string stale = "false";
					// Флаг создания нового ключа nonce
					bool createNonce = false;
					// Получаем текущий штамп времени в секундах
					const uint64_t stamp = this->_fmk->timestamp <uint64_t> (fmk_t::chrono_t::SECONDS);
					// Если ключ клиента не создан или истёк срок его жизни (30 минут)
					if((createNonce = (digest.nonce.empty() || ((stamp - digest.stamp) >= 1800)))){
						// Обновляем штамп времени генерации ключа
						digest.stamp = stamp;
						// Если ключ клиента ранее уже выдавался - запрашиваем повтор
						if(!digest.nonce.empty())
							// Помечаем ключ как устаревший
							stale = "true";
					}
					// Если необходимо создать новый ключ клиента
					if(createNonce){
						// Генерируем уникальный ключ клиента
						digest.nonce = this->_crypto->hash <string> (::to_string(stamp) + digest.realm, this->_params.hash);
						// Запоминаем фактически выданный сервером ключ для последующей сверки
						digest.issued = digest.nonce;
						// Сбрасываем последний принятый счётчик запросов для нового ключа сервера
						digest.lnc = "00000000";
					}
					// Если временный ключ сессии сервера ещё не создан
					if(digest.opaque.empty())
						// Генерируем временный ключ сессии сервера
						digest.opaque = this->_crypto->hash <string> (digest.realm, this->_params.hash);
					// Формируем значение вызова авторизации
					result = this->_fmk->format(
						"Digest realm=\"%s\", qop=\"%s\", stale=%s, algorithm=%s, nonce=\"%s\", opaque=\"%s\"",
						digest.realm.c_str(),
						digest.qop.c_str(),
						stale.c_str(),
						this->algorithm().c_str(),
						digest.nonce.c_str(),
						digest.opaque.c_str()
					);
					// Если требуется вывести заголовок вместе с его именем
					if(full)
						// Дополняем результат именем заголовка
						result = this->_fmk->format("%s: %s\r\n", this->name().c_str(), result.c_str());
				} break;
			}
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Записываем ошибку в лог
				this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(full), log_t::flag_t::CRITICAL, error.what());
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Выводим в лог сообщение об ошибке
				this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
			#endif
		}
	}
	// Выводим результат
	return result;
}
/**
 * @brief Конструктор
 *
 * @param owner  сторона работы (клиент/сервер)
 * @param params общие параметры авторизации
 * @param crypto объект криптографии
 * @param fmk    объект фреймворка
 * @param log    объект для работы с логами
 */
awh::http::Digest::Digest(const auth_t::owner_t owner, auth_t::params_t & params, const crypto_t * crypto, const fmk_t * fmk, const log_t * log) noexcept :
 auth_t::scheme_t(owner, params, crypto, fmk, log) {}
/**
 * @brief Деструктор
 *
 */
awh::http::Digest::~Digest() noexcept {}
