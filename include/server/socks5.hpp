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
			unordered_map <string, string> users;
		public:
			/**
			 * resCmd Метод получения бинарного буфера ответа
			 * @return бинарный буфер ответа
			 */
			vector <char> resCmd() const noexcept;
			/**
			 * resAuth Метод получения бинарного буфера ответа на авторизацию клиента
			 * @return бинарный буфер ответа
			 */
			vector <char> resAuth() const noexcept;
			/**
			 * resMethod Метод получения бинарного буфера выбора метода подключения
			 * @return бинарный буфер ответа
			 */
			vector <char> resMethod() const noexcept;
		public:
			/**
			 * parse Метод парсинга входящих данных
			 * @param buffer бинарный буфер входящих данных
			 * @param size   размер бинарного буфера входящих данных
			 */
			void parse(const char * buffer, const size_t size) noexcept;
		public:
			/**
			 * reset Метод сброса собранных данных
			 */
			void reset() noexcept;
		public:
			/**
			 * clearUsers Метод очистки списка пользователей
			 */
			void clearUsers() noexcept;
			/**
			 * setUsers Метод добавления списка пользователей
			 * @param users список пользователей для добавления
			 */
			void setUsers(const unordered_map <string, string> & users) noexcept;
		public:
			/**
			 * Socks5Server Конструктор
			 * @param uri объект для работы с URI
			 * @param fmk объект фреймворка
			 * @param log объект для работы с логами
			 */
			Socks5Server(const uri_t * uri, const fmk_t * fmk, const log_t * log) noexcept : socks5_t(uri, fmk, log) {}
			/**
			 * ~Socks5Server Деструктор
			 */
			~Socks5Server() noexcept {}
	} s5srv_t;
};

#endif // __AWH_SOCKS5_SERVER__
