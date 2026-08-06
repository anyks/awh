/**
 * @file: digest.cpp
 * @date: 2026-07-14
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @brief Реализация схемы DIGEST-авторизации (RFC 7616) — расчёт и проверка дайджеста по nonce, cnonce,
 *        nc и qop с контролем срока жизни nonce и защитой от повторного использования счётчика
 *
 * @copyright: Copyright © 2026
 *
 */

/**
 * Если максимальное число записей replay-защиты Digest (lncs) не указано
 */
#ifndef AWH_AUTH_DIGEST_LNCS_MAX
	/**
	 * Устанавливаем максимальное число записей replay-защиты Digest (lncs) в 4096
	 */
	#define AWH_AUTH_DIGEST_LNCS_MAX 0x1000
#endif

/**
 * Стандартные заголовочные файлы
 */
#include <cctype>
#include <chrono>
#include <random>
#include <cstdlib>

/**
 * Подключаем заголовочные файлы проекта
 */
#include <encoding/ascii.hpp>
#include <proto/http/auth/digest.hpp>

/**
 * Используем стандартное пространство имён
 */
using namespace std;

/**
 * @brief Инкапсулируем статические параметры в пространство имён (nc)
 *
 */
namespace nc {
	/**
	 * Используем пространство имён AWH
	 */
	using namespace awh;

	/**
	 * @brief Функция проверки корректности счётчика запросов (nc)
	 *
	 * @param nc значение счётчика в шестнадцатеричном виде
	 * @return   результат проверки
	 *
	 */
	static bool valid(const string & nc) noexcept {
		// Счётчик должен содержать ровно 8 шестнадцатеричных цифр
		if(nc.size() != 8)
			// Сообщаем о некорректном счётчике
			return false;
		/**
		 * Выполняем проверку каждого символа
		 */
		for(const char symbol : nc){
			// Если символ не является шестнадцатеричной цифрой
			if(!awh::ascii::isHex(symbol))
				// Сообщаем о некорректном счётчике
				return false;
		}
		// Подтверждаем корректность счётчика
		return true;
	}
	/**
	 * @brief Функция генерации случайного значения для непредсказуемости ключей
	 *
	 * @details Используется как источник энтропии при формировании серверных nonce/opaque
	 *          и клиентского cnonce. Смешивает криптостойкий источник (random_device) с
	 *          высокоточным временем, чтобы значения не были предсказуемыми по одному лишь
	 *          штампу времени и realm.
	 *
	 * @return случайное значение в текстовом виде
	 *
	 */
	static string entropy() noexcept {
		// Инициализируем генератор случайных чисел единожды на поток
		static thread_local mt19937_64 engine(
			static_cast <uint64_t> (random_device{}()) ^
			static_cast <uint64_t> (chrono::steady_clock::now().time_since_epoch().count())
		);
		// Возвращаем две 64-битные случайные выборки в текстовом виде
		return (::to_string(engine()) + ::to_string(engine()));
	}
	/**
	 * @brief Функция разбора счётчика запросов (nc)
	 *
	 * @param nc    значение счётчика
	 * @param value полученное числовое значение
	 * @return      результат разбора
	 *
	 */
	static bool parse(const string & nc, uint32_t & value) noexcept {
		// Если формат счётчика некорректен
		if(!valid(nc))
			// Сообщаем о некорректном счётчике
			return false;
		// Указатель на первый неразобранный символ
		char * end = nullptr;
		// Выполняем разбор счётчика как беззнакового 32-битного числа
		const uint64_t parsed = static_cast <uint64_t> (::strtoul(nc.c_str(), &end, 16));
		// Если разбор не завершён или значение не помещается в uint32_t
		if((end == nullptr) || (*end != '\0') || (parsed > 0xFFFFFFFFUL))
			// Сообщаем о некорректном счётчике
			return false;
		// Сохраняем разобранное значение
		value = static_cast <uint32_t> (parsed);
		// Подтверждаем успешный разбор
		return true;
	}
	/**
	 * @brief Функция обновления таблицы replay Digest с LRU-вытеснением
	 *
	 * @param digest параметры Digest-авторизации
	 * @param key    ключ пары «логин + nonce»
	 * @param value  последний принятый nc
	 *
	 */
	static void touchLRU(http::auth_t::digest_t & digest, const string & key, const string & value) noexcept {
		// Если запись уже существует — обновляем nc и переносим ключ в конец очереди
		if(const auto i = digest.lncs.find(key); i != digest.lncs.end()){
			// Сохраняем последний принятый nc
			i->second.first = value;
			// Переносим ключ в конец LRU-очереди за O(1) (перестановка узла списка)
			digest.lncsOrder.splice(digest.lncsOrder.end(), digest.lncsOrder, i->second.second);
			// Подтверждаем успешное обновление
			return;
		}
		// Если достигнут лимит — удаляем самую старую запись
		if((digest.lncs.size() >= AWH_AUTH_DIGEST_LNCS_MAX) && !digest.lncsOrder.empty()){
			// Удаляем старейший ключ из таблицы
			digest.lncs.erase(digest.lncsOrder.front());
			// Удаляем старейший ключ из LRU-очереди
			digest.lncsOrder.pop_front();
		}
		// Добавляем ключ в конец LRU-очереди
		const auto pos = digest.lncsOrder.insert(digest.lncsOrder.end(), key);
		// Сохраняем новую запись replay-защиты (nc + позиция в LRU-очереди)
		digest.lncs.emplace(key, make_pair(value, pos));
	}
};

/**
 * @brief Метод перевода алгоритма хэширования в текстовое представление
 *
 * @return название алгоритма хэширования (MD5, SHA-256 и т.д.)
 *
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
	if(this->_params.digest.mode.sess)
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
 *
 */
string awh::http::Digest::response(const string & user, const string & pass) const noexcept {
	// Результат работы функции
	string result = "";
	// Получаем ссылку на параметры Digest-авторизации
	const auth_t::digest_t & digest = this->_params.digest;
	// Если данные, необходимые для расчёта ответа, переданы
	if(!user.empty() && !pass.empty() && !digest.nonce.empty() && (!digest.mode.qop || !digest.cnonce.empty()) && (this->_crypto != nullptr)){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Определяем алгоритм хэширования
			const crypto_t::hash_t hash = this->_params.hash;
			// Формируем первый этап расчёта: HA1 = H(user:realm:pass)
			string ha1Input = "";
			// Резервируем память под строку HA1
			ha1Input.reserve(user.size() + digest.realm.size() + pass.size() + 2);
			// Собираем строку HA1 без промежуточного format()
			ha1Input.append(user).append(1, ':').append(digest.realm).append(1, ':').append(pass);
			// Выполняем хэширование первого этапа
			string ha1 = ::move(this->_crypto->hash <string> (ha1Input, hash));
			// Если используется сессионный режим (-sess): HA1 = H(HA1:nonce:cnonce)
			if(!ha1.empty() && digest.mode.sess){
				// Формируем строку для сессионного пересчёта HA1
				string sessInput = "";
				// Резервируем память под сессионную строку HA1
				sessInput.reserve(ha1.size() + digest.nonce.size() + digest.cnonce.size() + 2);
				// Собираем строку сессионного HA1
				sessInput.append(ha1).append(1, ':').append(digest.nonce).append(1, ':').append(digest.cnonce);
				// Пересчитываем первый этап с учётом nonce и cnonce
				ha1 = ::move(this->_crypto->hash <string> (sessInput, hash));
			}
			// Если первый этап расчёта получен
			if(!ha1.empty()){
				// Строка второго этапа расчёта HA2
				string ha2Input = "";
				// Если используется qop=auth-int
				if(digest.mode.authInt){
					// Хэш тела запроса для auth-int
					const string & entityHash = this->_crypto->hash <string> (digest.entity, hash);
					// Резервируем память под строку HA2
					ha2Input.reserve(this->_params.method.size() + digest.uri.size() + entityHash.size() + 2);
					// Формируем HA2 = H(method:uri:H(entity-body))
					ha2Input.append(this->_params.method).append(1, ':').append(digest.uri).append(1, ':').append(entityHash);
				// Если используется qop=auth или legacy-режим
				} else {
					// Резервируем память под строку HA2
					ha2Input.reserve(this->_params.method.size() + digest.uri.size() + 1);
					// Формируем HA2 = H(method:uri)
					ha2Input.append(this->_params.method).append(1, ':').append(digest.uri);
				}
				// Выполняем хэширование второго этапа
				const string & ha2 = this->_crypto->hash <string> (ha2Input, hash);
				// Если второй этап расчёта получен
				if(!ha2.empty()){
					// Строка итогового расчёта response
					string responseInput = "";
					// Если используется qop (RFC 7616)
					if(digest.mode.qop){
						// Резервируем память под строку response
						responseInput.reserve(ha1.size() + digest.nonce.size() + digest.nc.size() + digest.cnonce.size() + digest.qop.size() + ha2.size() + 5);
						// Формируем response = H(HA1:nonce:nc:cnonce:qop:HA2)
						responseInput.append(ha1).append(1, ':').append(digest.nonce).append(1, ':').append(digest.nc).append(1, ':').append(digest.cnonce).append(1, ':').append(digest.qop).append(1, ':').append(ha2);
					// Если qop не используется (legacy RFC 2069)
					} else {
						// Резервируем память под строку response
						responseInput.reserve(ha1.size() + digest.nonce.size() + ha2.size() + 2);
						// Формируем response = H(HA1:nonce:HA2)
						responseInput.append(ha1).append(1, ':').append(digest.nonce).append(1, ':').append(ha2);
					}
					// Формируем итоговый ответ
					result = ::move(this->_crypto->hash <string> (responseInput, hash));
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
 *
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
		// Если включён строгий режим проверки учётных данных
		if(this->_params.mode.validation == auth_t::mode_t::STRICT){
			// В строгом режиме legacy RFC 2069 (без qop) не допускается (RFC 7616)
			if(!digest.mode.qop){
				// Пишем диагностический лог об отклонении legacy-режима без qop
				this->_log->print(
					"Digest auth legacy mode without qop rejected in strict mode for user \"%s\" [nonce=\"%s\"]",
					log_t::flag_t::WARNING,
					this->_params.user.c_str(),
					digest.nonce.c_str()
				);
				// Сообщаем о неудачной проверке
				return false;
			}
			// В строгом режиме при использовании qop обязателен ключ клиента cnonce (RFC 7616)
			if(digest.cnonce.empty()){
				// Пишем диагностический лог об отсутствии cnonce
				this->_log->print(
					"Digest auth missing cnonce rejected in strict mode for user \"%s\" [nonce=\"%s\"]",
					log_t::flag_t::WARNING,
					this->_params.user.c_str(),
					digest.nonce.c_str()
				);
				// Сообщаем о неудачной проверке
				return false;
			}
			// В строгом режиме обязателен временный ключ сессии opaque
			if(digest.opaque.empty()){
				// Пишем диагностический лог об отсутствии opaque
				this->_log->print(
					"Digest auth missing opaque rejected in strict mode for user \"%s\" [nonce=\"%s\"]",
					log_t::flag_t::WARNING,
					this->_params.user.c_str(),
					digest.nonce.c_str()
				);
				// Сообщаем о неудачной проверке
				return false;
			}
			// В строгом режиме алгоритм из учётных данных должен совпадать с настроенным (защита от подмены алгоритма)
			if(this->_params.hash != this->_params.scheme){
				// Пишем диагностический лог о несовпадении алгоритма
				this->_log->print(
					"Digest auth algorithm downgrade rejected in strict mode for user \"%s\" [algorithm=%s]",
					log_t::flag_t::WARNING,
					this->_params.user.c_str(),
					this->algorithm().c_str()
				);
				// Сообщаем о неудачной проверке
				return false;
			}
		}
		// Получаем текущий счётчик запросов клиента (nonce count)
		uint32_t nc = 0;
		// Если используется qop — проверяем формат и значение nc до расчёта response
		if(digest.mode.qop){
			// Если формат nc некорректен, nc равен нулю или переполнен
			if(!::nc::parse(digest.nc, nc) || (nc == 0) || (nc >= 0xFFFFFFFF)){
				// Пишем диагностический лог о некорректном счётчике
				this->_log->print(
					"Digest auth invalid nc for user \"%s\": nc=\"%s\" [nonce=\"%s\"]",
					log_t::flag_t::WARNING,
					this->_params.user.c_str(),
					digest.nc.c_str(),
					digest.nonce.c_str()
				);
				// Сообщаем о неудачной проверке
				return false;
			}
		}
		// Формируем ключ учёта replay для пары «логин + nonce»
		const string replayKey = this->_params.user + '\x01' + digest.nonce;
		// Получаем последний принятый сервером счётчик запросов для этой пары (за один поиск)
		string lastNc = "00000000";
		// Если запись о последнем принятом счётчике найдена
		if(const auto it = digest.lncs.find(replayKey); it != digest.lncs.end())
			// Извлекаем последний принятый счётчик запросов
			lastNc = it->second.first;
		// Получаем последний принятый сервером счётчик запросов
		uint32_t last = 0;
		// Если используется qop — разбираем последний принятый nc
		if(digest.mode.qop && !::nc::parse(lastNc, last))
			// Считаем последний принятый счётчик равным нулю
			last = 0;
		/**
		 * Защита от повторного воспроизведения запроса (replay): счётчик nc должен строго
		 * возрастать в пределах одного nonce. Повтор или откат значения означает атаку
		 * либо некорректную работу клиента.
		 */
		if(digest.mode.qop && (nc <= last)){
			// Пишем диагностический лог о повторном воспроизведении запроса
			this->_log->print(
				"Digest auth replay detected for user \"%s\": received nc=\"%s\", last accepted nc=\"%s\" [nonce=\"%s\"]",
				log_t::flag_t::WARNING,
				this->_params.user.c_str(),
				digest.nc.c_str(),
				lastNc.c_str(),
				digest.nonce.c_str()
			);
			// Сообщаем о неудачной проверке
			return false;
		}
		/**
		 * Проверяем, что клиент вернул именно тот ключ (nonce), который выдал сервер.
		 * Это отсекает воспроизведение перехваченных учётных данных со старым ключом
		 * на новом соединении, где счётчик nc ещё не накоплен.
		 */
		if(!digest.issued.empty() && !secureCompare(digest.nonce, digest.issued)){
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
		/**
		 * Если nonce выдан самим сервером (известен штамп его генерации) — проверяем его возраст.
		 * Это ограничивает окно повторного воспроизведения перехваченных учётных данных:
		 * по истечении времени жизни nonce отклоняется, и клиент вынужден получить новый ключ.
		 * Для nonce, установленного вручную (mode.stamp == 0), проверка возраста не выполняется.
		 */
		if((digest.mode.stamp > 0) && (this->_params.mode.nonceMaxAge > 0)){
			// Получаем текущий штамп времени в секундах
			const uint64_t now = this->_fmk->timestamp <uint64_t> (fmk_t::chrono_t::SECONDS);
			// Если срок жизни выданного сервером nonce истёк
			if((now > digest.mode.stamp) && ((now - digest.mode.stamp) >= this->_params.mode.nonceMaxAge)){
				// Пишем диагностический лог об истечении срока жизни выданного ключа сервера
				this->_log->print(
					"Digest auth stale nonce for user \"%s\": nonce age=%llu s exceeds TTL=%llu s [nonce=\"%s\"]",
					log_t::flag_t::WARNING,
					this->_params.user.c_str(),
					static_cast <unsigned long long> (now - digest.mode.stamp),
					static_cast <unsigned long long> (this->_params.mode.nonceMaxAge),
					digest.nonce.c_str()
				);
				// Сообщаем о неудачной проверке
				return false;
			}
		}
		/**
		 * Проверяем, что клиент вернул именно тот opaque, который выдал сервер.
		 * Это усиливает привязку учётных данных к текущей auth-сессии сервера.
		 */
		if(!digest.issuedOpaque.empty() && !secureCompare(digest.opaque, digest.issuedOpaque)){
			// Пишем диагностический лог о несовпадении opaque
			this->_log->print(
				"Digest auth opaque mismatch for user \"%s\": expected opaque=\"%s\", received opaque=\"%s\"",
				log_t::flag_t::WARNING,
				this->_params.user.c_str(),
				digest.issuedOpaque.c_str(),
				digest.opaque.c_str()
			);
			// Сообщаем о неудачной проверке
			return false;
		}
		// Выполняем расчёт ожидаемого ответа
		const string & response = this->response(this->_params.user, pass);
		// Если ожидаемый ответ не совпадает с ответом клиента
		if(response.empty() || !secureCompare(response, digest.response)){
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
		// Фиксируем принятый счётчик запросов как последний подтверждённый для пары «логин + nonce»
		if(digest.mode.qop)
			// Обновляем таблицу replay с LRU-вытеснением
			::nc::touchLRU(this->_params.digest, replayKey, digest.nc);
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
 *
 */
bool awh::http::Digest::parse(const string_view header) noexcept {
	// Результат работы функции
	bool result = false;
	// Если заголовок передан
	if(!header.empty()){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			/**
			 * Сбрасываем флаги qop, sess и hash перед разбором (challenge/credentials могут не содержать algorithm)
			 */
			this->_params.digest.mode.qop     = false;
			this->_params.digest.mode.sess    = false;
			this->_params.digest.mode.authInt = false;
			this->_params.digest.qop          = "auth";
			this->_params.hash                = this->_params.scheme;
			// Полезная нагрузка заголовка после названия схемы
			string digest = "";
			// Название схемы авторизации
			const string type = "Digest";
			// Если схема DIGEST не распознана - завершаем разбор
			if(!this->schemePayload(header, type, digest))
				// Сообщаем о неудачном разборе
				return result;
			// Разбираем параметры Digest с учётом кавычек и запятых внутри значений
			const unordered_multimap <string, string> params = this->_fmk->kv(digest, ",", "=");
			// Если список параметров получен
			if((result = !params.empty())){
				// Флаг несовпадения realm с настроенным значением сервера
				bool realmMismatch = false;
				/**
				 * Переходим по всему списку параметров
				 */
				for(const auto & item : params){
					// Извлекаем и очищаем ключ параметра
					string key = item.first;
					// Извлекаем и очищаем значение параметра
					string value = item.second;
					// Удаляем крайние пробелы у ключа
					this->_fmk->transform(key, fmk_t::transform_t::TRIM);
					// Удаляем крайние пробелы у значения
					this->_fmk->transform(value, fmk_t::transform_t::TRIM);
					// Приводим ключ параметра к нижнему регистру (RFC 7616)
					this->_fmk->transform(key, fmk_t::transform_t::LOWER_CASE);
					// Если значение обёрнуто в кавычки - удаляем их
					if((value.length() > 1) && (value.front() == '"') && (value.back() == '"'))
						// Снимаем обрамляющие кавычки
						value = ::move(value.substr(1, value.length() - 2));
					// Если параметр является именем пользователя
					if(key.compare("username") == 0)
						// Устанавливаем логин пользователя
						this->_params.user = ::move(value);
					// Если параметр является идентификатором сервера
					else if(key.compare("realm") == 0) {
						// На сервере realm задаётся через auth.realm() и не перезаписывается
						if((this->_owner == auth_t::owner_t::SERVER) && !this->_params.digest.realm.empty()){
							// Если realm клиента не совпадает с настроенным — отклоняем разбор
							if(!secureCompare(value, this->_params.digest.realm))
								// Сообщаем о несовпадении realm
								realmMismatch = true;
						// На клиенте или если realm сервера не задан — принимаем значение из заголовка, устанавливаем realm
						} else this->_params.digest.realm = ::move(value);
					// Если параметр является ключом, выданным сервером
					} else if(key.compare("nonce") == 0) {
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
					else if(key.compare("qop") == 0) {
						// Фиксируем наличие qop (RFC 7616)
						this->_params.digest.mode.qop = true;
						// Если запрошен режим auth-int
						if(value.find("auth-int") != string::npos){
							// Устанавливаем тип защиты auth-int
							this->_params.digest.qop = "auth-int";
							// Включаем режим auth-int
							this->_params.digest.mode.authInt = true;
						// Если запрошен режим auth
						} else if(value.find("auth") != string::npos) {
							// Устанавливаем тип защиты auth
							this->_params.digest.qop = "auth";
							// Отключаем режим auth-int
							this->_params.digest.mode.authInt = false;
						}
					// Если параметр является алгоритмом хэширования
					} else if(key.compare("algorithm") == 0) {
						// Приводим название алгоритма к нижнему регистру
						this->_fmk->transform(value, fmk_t::transform_t::LOWER_CASE);
						// Определяем режим сессионного алгоритма (-sess)
						if((value.size() >= 5) && (value.compare(value.size() - 5, 5, "-sess") == 0)){
							// Включаем сессионный режим
							this->_params.digest.mode.sess = true;
							// Удаляем суффикс сессионного режима
							value.erase(value.size() - 5);
						// Отключаем сессионный режим
						} else this->_params.digest.mode.sess = false;
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
				// Если realm клиента не совпадает с настроенным сервером
				if(realmMismatch)
					// Сообщаем о неудачном разборе
					result = false;
				// На сервере учётные данные должны содержать логин и response
				else if((this->_owner == auth_t::owner_t::SERVER) && result && (this->_params.user.empty() || this->_params.digest.response.empty()))
					// Сообщаем о неудачном разборе
					result = false;
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
 *
 */
string awh::http::Digest::header(const bool full) noexcept {
	// Результат работы функции
	string result = "";
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
					// Если используется qop и ключ клиента ещё не сгенерирован
					if(digest.mode.qop && digest.cnonce.empty()){
						// Получаем текущий штамп времени в секундах
						const uint64_t stamp = this->_fmk->timestamp <uint64_t> (fmk_t::chrono_t::SECONDS);
						// Генерируем непредсказуемый ключ клиента (штамп времени + энтропия + логин)
						digest.cnonce = this->_crypto->hash <string> (::to_string(stamp) + ::nc::entropy() + this->_params.user, this->_params.hash);
						// Обрезаем ключ клиента до 16 символов
						if(digest.cnonce.length() > 16)
							// Оставляем только первые 16 символов
							digest.cnonce = digest.cnonce.substr(0, 16);
					}
					// Увеличиваем счётчик запросов клиента при использовании qop
					if(digest.mode.qop){
						// Получаем текущий счётчик запросов
						uint32_t nc = 0;
						// Если nc некорректен или достигнут предел — не формируем заголовок
						if(!::nc::parse(digest.nc, nc) || (nc >= 0xFFFFFFFF))
							// Завершаем формирование заголовка
							break;
						// Формируем счётчик запросов в виде 8 шестнадцатеричных цифр (nonce count)
						digest.nc = this->_fmk->format("%08x", nc + 1);
					}
					// Выполняем расчёт ответа клиента
					digest.response = this->response(this->_params.user, this->_params.pass);
					// Если ответ клиента рассчитан
					if(!digest.response.empty()){
						// Кэшируем имя алгоритма для формирования заголовка
						const string & algorithm = this->algorithm();
						// Формируем значение заголовка авторизации
						if(digest.mode.qop)
							// Формат RFC 7616 с qop, nc и cnonce
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
								algorithm.c_str()
							);
						// Если qop не используется (legacy RFC 2069)
						else {
							// Legacy-формат RFC 2069 без qop, nc и cnonce
							result = this->_fmk->format(
								"Digest username=\"%s\", realm=\"%s\", nonce=\"%s\", uri=\"%s\", response=\"%s\", opaque=\"%s\", algorithm=%s",
								this->_params.user.c_str(),
								digest.realm.c_str(),
								digest.nonce.c_str(),
								digest.uri.c_str(),
								digest.response.c_str(),
								digest.opaque.c_str(),
								algorithm.c_str()
							);
						}
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
				// Сервер всегда объявляет qop в современном вызове авторизации
				digest.mode.qop = true;
				// Если nonce не создан или истёк срок его жизни (при включённом ограничении возраста)
				if((createNonce = (digest.nonce.empty() || ((this->_params.mode.nonceMaxAge > 0) && (stamp > digest.mode.stamp) && ((stamp - digest.mode.stamp) >= this->_params.mode.nonceMaxAge))))){
					// Обновляем штамп времени генерации nonce
					digest.mode.stamp = stamp;
					// Если nonce ранее уже выдавался - запрашиваем повтор
					if(!digest.nonce.empty())
						// Помечаем nonce как устаревший
						stale = "true";
				}
				// Если необходимо создать новый nonce
				if(createNonce){
					// Генерируем непредсказуемый nonce сервера (штамп времени + энтропия + realm)
					digest.nonce = this->_crypto->hash <string> (::to_string(stamp) + ::nc::entropy() + digest.realm, this->_params.hash);
					// Запоминаем фактически выданный сервером nonce для последующей сверки
					digest.issued = digest.nonce;
					// Сбрасываем учёт replay для нового nonce
					digest.lncs.clear();
					// Сбрасываем учёт replay для нового nonce (удаляем все записи о последних nc)
					digest.lncsOrder.clear();
				}
				// Если временный ключ сессии сервера ещё не создан
				if(digest.opaque.empty()){
					// Генерируем непредсказуемый временный ключ сессии сервера (realm + энтропия)
					digest.opaque = this->_crypto->hash <string> (digest.realm + ::nc::entropy(), this->_params.hash);
					// Запоминаем фактически выданный opaque для последующей сверки
					digest.issuedOpaque = digest.opaque;
				}
				// Кэшируем имя алгоритма для формирования вызова
				const string & algorithm = this->algorithm();
				// Формируем значение вызова авторизации
				result = this->_fmk->format(
					"Digest realm=\"%s\", qop=\"%s\", stale=%s, algorithm=%s, nonce=\"%s\", opaque=\"%s\"",
					digest.realm.c_str(),
					digest.qop.c_str(),
					stale.c_str(),
					algorithm.c_str(),
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
 *
 */
awh::http::Digest::Digest(const auth_t::owner_t owner, auth_t::params_t & params, const crypto_t * crypto, const fmk_t * fmk, const log_t * log) noexcept :
 auth_t::scheme_t(owner, params, crypto, fmk, log) {}
/**
 * @brief Деструктор
 *
 */
awh::http::Digest::~Digest() noexcept {}
