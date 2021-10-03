/**
 * author:    Yuriy Lobarev
 * telegram:  @forman
 * phone:     +7(910)983-95-90
 * email:     forman@anyks.com
 * site:      https://anyks.com
 * copyright: © Yuriy Lobarev
 */

#ifndef __AWH_SOCKS5_SERVER__
#define __AWH_SOCKS5_SERVER__

/**
 * Стандартная библиотека
 */
#include <unordered_map>

/**
 * Наши модули
 */
#include <core/socks5.hpp>

// Подписываемся на стандартное пространство имён
using namespace std;

/*
 * awh пространство имён
 */
namespace awh {
	/**
	 * Socks5Server Класс сервера для работы с socks5 прокси-сервером
	 */
	typedef class Socks5Server : public socks5_t {
		private:
			// Список пользователей для Basic авторизации
			const unordered_map <string, string> * users;
		public:
			/**
			 * resCmd Метод получения бинарного буфера ответа
			 */
			void resCmd() const noexcept;
			/**
			 * resAuth Метод получения бинарного буфера ответа на авторизацию клиента
			 */
			void resAuth() const noexcept;
			/**
			 * resMethod Метод получения бинарного буфера выбора метода подключения
			 * @param methods методы авторизаций выбранныйе пользователем
			 */
			void resMethod(const vector <method_t> & methods) const noexcept;
		public:
			/**
			 * parse Метод парсинга входящих данных
			 * @param buffer бинарный буфер входящих данных
			 * @param size   размер бинарного буфера входящих данных
			 */
			void parse(const char * buffer = nullptr, const size_t size = 0) noexcept;
		public:
			/**
			 * reset Метод сброса собранных данных
			 */
			void reset() noexcept;
		public:
			/**
			 * setUsers Метод добавления списка пользователей
			 * @param users список пользователей для добавления
			 */
			void setUsers(const unordered_map <string, string> * users) noexcept;
		public:
			/**
			 * Socks5Server Конструктор
			 * @param fmk объект фреймворка
			 * @param log объект для работы с логами
			 * @param uri объект для работы с URI
			 */
			Socks5Server(const fmk_t * fmk, const log_t * log, const uri_t * uri) noexcept : socks5_t(fmk, log, uri), users(nullptr) {}
			/**
			 * ~Socks5Server Деструктор
			 */
			~Socks5Server() noexcept {}
	} s5srv_t;
};

#endif // __AWH_SOCKS5_SERVER__
