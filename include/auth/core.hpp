/**
 * @file: core.hpp
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

#ifndef __AWH_AUTH__
#define __AWH_AUTH__

/**
 * Стандартные модули
 */
#include <string>

/**
 * Наши модули
 */
#include <sys/fmk.hpp>
#include <sys/log.hpp>
#include <sys/hash.hpp>

/**
 * awh пространство имён
 */
namespace awh {
	/**
	 * Подписываемся на стандартное пространство имён
	 */
	using namespace std;
	/**
	 * Authorization Класс работы с авторизацией
	 */
	typedef class AWHSHARED_EXPORT Authorization {
		public:
			/**
			 * Тип авторизации
			 */
			enum class type_t : uint8_t {
				NONE   = 0x00, // Авторизация не установлена
				BASIC  = 0x01, // BASIC авторизация
				DIGEST = 0x02  // DIGEST авторизация
			};
			/**
			 * Алгоритм шифрования для авторизации Digest
			 */
			enum class hash_t : uint8_t {
				MD5    = 0x01, // Хэш-ключ MD5
				SHA1   = 0x02, // Хэш-ключ SHA1
				SHA224 = 0x03, // Хэш-ключ SHA224
				SHA256 = 0x04, // Хэш-ключ SHA256
				SHA384 = 0x05, // Хэш-ключ SHA384
				SHA512 = 0x06  // Хэш-ключ SHA512
			};
		protected:
			/**
			 * Digest Структура параметров дайджест авторизации
			 */
			typedef struct Digest {
				hash_t hash;   // Алгоритм шифрования (MD5, SHA1, SHA256, SHA512)
				uint64_t date; // Штамп времени последнего создания nonce
				string nc;     // Счётчик 16-го секретного кода клиента
				string uri;    // Параметры HTTP запроса
				string qop;    // Тип авторизации (auth, auth-int)
				string resp;   // Результат ответа клиента
				string realm;  // Название сервера или e-mail
				string nonce;  // Уникальный ключ клиента выдаваемый сервером
				string opaque; // Временный ключ сессии сервера
				string cnonce; // 16-й секретный код клиента
				/**
				 * Digest Конструктор
				 */
				Digest() noexcept :
				 nc{"00000000"}, uri{""}, qop{"auth"},
				 realm{AWH_HOST}, nonce{""}, opaque{""},
				 cnonce{""}, resp{""}, date{0}, hash{hash_t::MD5} {}
			} digest_t;
		protected:
			// Тип авторизации
			type_t _type;
			// Параметры Digest авторизации
			digest_t _digest;
			// Объект хэширования
			awh::hash_t _hash;
		protected:
			// Объект фреймворка
			const fmk_t * _fmk;
			// Объект работы с логами
			const log_t * _log;
		public:
			/**
			 * digest Метод получения параметров Digest авторизации
			 * @return параметры Digest авторизации
			 */
			const digest_t & digest() const noexcept;
		public:
			/**
			 * header Метод установки параметров авторизации из заголовков
			 * @param header заголовок HTTP с параметрами авторизации
			 */
			virtual void header(const string & header) noexcept = 0;
		protected:
			/**
			 * response Метод создания ответа на дайджест авторизацию
			 * @param method метод HTTP запроса
			 * @param digest параметры дайджест авторизации
			 * @param user   логин пользователя для проверки
			 * @param pass   пароль пользователя для проверки
			 * @return       ответ в 16-м виде
			 */
			string response(const string & method, const string & user, const string & pass, const digest_t & digest) const noexcept;
		public:
			/**
			 * type Метод получени типа авторизации
			 * @return тип авторизации
			 */
			type_t type() const noexcept;
			/**
			 * type Метод установки типа авторизации
			 * @param type тип авторизации
			 * @param hash алгоритм шифрования для Digest авторизации
			 */
			void type(const type_t type, const hash_t hash = hash_t::MD5) noexcept;
		public:
			/**
			 * Authorization Конструктор
			 * @param fmk объект фреймворка
			 * @param log объект для работы с логами
			 */
			Authorization(const fmk_t * fmk, const log_t * log) noexcept :
			 _type(type_t::NONE), _hash(log), _fmk(fmk), _log(log) {}
			/*
			 * ~Authorization Деструктор
			 */
			virtual ~Authorization() noexcept {}
	} auth_t;
};

#endif // __AWH_AUTH__
