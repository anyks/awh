/**
 * @file: partners.cpp
 * @date: 2025-09-17
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
#include <events/partners.hpp>

/**
 * Подписываемся на стандартное пространство имён
 */
using namespace std;

/**
 * @brief Метод проверки существования сокета
 *
 * @param sock сокет для проверки
 * @return     результат проверки
 */
bool awh::Partners::has(const SOCKET sock) noexcept {
	// Выполняем проверку наличия сокета в списке
	return (this->_base.find(sock) != this->_base.end());
}
/**
 * @brief Метод удаления сокета
 *
 * @param sock сокет для удаления
 */
void awh::Partners::del(const SOCKET sock) noexcept {
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		// Выполняем блокировку потока
		const lock_guard <std::mutex> lock(this->_mtx);
		// Выполняем поиск сокета
		auto i = this->_base.find(sock);
		// Если сокет успешно найден
		if(i != this->_base.end()){
			// Если сокеты партнёры отличаются друг от друга
			if(i->first != i->second){
				// Выполняем партнёрской сокет
				auto j = this->_base.find(i->second);
				// Если партнёрский сокет найден
				if(j != this->_base.end()){
					/**
					 * Для операционной системы OS Windows
					 */
					#if _WIN32 || _WIN64
						// Закрываем сокет
						::closesocket(j->first);
					/**
					 * Для всех остальных операционных систем
					 */
					#else
						// Закрываем сокет
						::close(j->first);
					#endif
					// Выполняем удаление сокета
					this->_base.erase(j);
				}
			}
			/**
			 * Для операционной системы OS Windows
			 */
			#if _WIN32 || _WIN64
				// Закрываем сокет
				::closesocket(i->first);
			/**
			 * Для всех остальных операционных систем
			 */
			#else
				// Закрываем сокет
				::close(i->first);
			#endif
			// Выполняем удаление сокета
			this->_base.erase(i);
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
			this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(sock), log_t::flag_t::CRITICAL, error.what());
		/**
		* Если режим отладки не включён
		*/
		#else
			// Выводим сообщение об ошибке
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
}
/**
 * @brief Метод объединения партнёрских сокетов
 * 
 * @param sock1 первый сокет для добавления
 * @param sock2 второй сокет для добавления
 * @return      результат объединения
 */
bool awh::Partners::merge(const SOCKET sock1, const SOCKET sock2) noexcept {
	// Результат работы функции
	bool result = false;
	/**
	 * Выполняем перехват ошибок
	 */
	try {
		// Выполняем блокировку потока
		const lock_guard <std::mutex> lock(this->_mtx);
		// Выполняем добавление первой записи
		if((result = this->_base.emplace(sock1, sock2).first->first) && (sock1 != sock2))
			// Добавляем вторую запись
			result = this->_base.emplace(sock2, sock1).first->first;
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Выводим сообщение об ошибке
			this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(sock1, sock2), log_t::flag_t::CRITICAL, error.what());
		/**
		* Если режим отладки не включён
		*/
		#else
			// Выводим сообщение об ошибке
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
	// Выводим результат
	return result;
}
/**
 * @brief Конструктор
 * 
 * @param log объект для работы с логами
 */
awh::Partners::Partners(const log_t * log) noexcept : _log(log) {}
/**
 * @brief Деструктор
 * 
 */
awh::Partners::~Partners() noexcept {
	// Если база сокетов не пустая
	if(!this->_base.empty()){
		/**
		 * Выполняем перехват ошибок
		 */
		try {
			// Выполняем перебор всего списка сокетов
			for(auto & sock : this->_base){
				/**
				 * Для операционной системы OS Windows
				 */
				#if _WIN32 || _WIN64
					// Закрываем сокет
					::closesocket(sock.first);
				/**
				 * Для всех остальных операционных систем
				 */
				#else
					// Закрываем сокет
					::close(sock.first);
				#endif
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
}
