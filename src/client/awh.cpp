/**
 * @file: awh.cpp
 * @date: 2023-09-19
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
#include <client/awh.hpp>

/**
 * Подписываемся на стандартное пространство имён
 */
using namespace std;

/**
 * proto Метод извлечения поддерживаемого протокола подключения
 * @return поддерживаемый протокол подключения (HTTP1_1, HTTP2)
 */
awh::engine_t::proto_t awh::client::AWH::proto() const noexcept {
	// Выполняем определение активного HTTP-протокола
	return this->_http.proto();
}
/**
 * sendError Метод отправки сообщения об ошибке на сервер Websocket
 * @param mess отправляемое сообщение об ошибке
 */
void awh::client::AWH::sendError(const ws::mess_t & mess) noexcept {
	// Выполняем отправку сообщения об ошибке
	this->_http.sendError(mess);
}
/**
 * sendMessage Метод отправки сообщения на сервер
 * @param message передаваемое сообщения в бинарном виде
 * @param text    данные передаются в текстовом виде
 * @return        результат отправки сообщения
 */
bool awh::client::AWH::sendMessage(const vector <char> & message, const bool text) noexcept {
	// Выполняем отправку сообщения на Websocket-сервер
	return this->_http.sendMessage(message, text);
}
/**
 * sendMessage Метод отправки сообщения на сервер
 * @param message передаваемое сообщения в бинарном виде
 * @param size    размер передаваемого сообещния
 * @param text    данные передаются в текстовом виде
 * @return        результат отправки сообщения
 */
bool awh::client::AWH::sendMessage(const char * message, const size_t size, const bool text) noexcept {
	// Выполняем отправку сообщения на Websocket-сервер
	return this->_http.sendMessage(message, size, text);
}
/**
 * send Метод отправки сообщения на сервер HTTP/2
 * @param request параметры запроса на удалённый сервер
 * @return        идентификатор отправленного запроса
 */
int32_t awh::client::AWH::send(const web_t::request_t & request) noexcept {
	// Выполняем отправку сообщения на удалённый сервер
	return this->_http.send(request);
}
/**
 * send Метод отправки данных в бинарном виде серверу
 * @param buffer буфер бинарных данных передаваемых серверу
 * @param size   размер сообщения в байтах
 * @return       результат отправки сообщения
 */
bool awh::client::AWH::send(const char * buffer, const size_t size) noexcept {
	// Выполняем отправку сообщения на удалённый сервер в сыром виде
	return this->_http.send(buffer, size);
}
/**
 * send Метод отправки тела сообщения на сервер
 * @param sid    идентификатор потока HTTP
 * @param buffer буфер бинарных данных передаваемых на сервер
 * @param size   размер сообщения в байтах
 * @param end    флаг последнего сообщения после которого поток закрывается
 * @return       результат отправки данных указанному клиенту
 */
bool awh::client::AWH::send(const int32_t sid, const char * buffer, const size_t size, const bool end) noexcept {
	// Выполняем отправку данных на удалённый сервер HTTP/2
	return this->_http.send(sid, buffer, size, end);
}
/**
 * send Метод отправки заголовков на сервер
 * @param sid     идентификатор потока HTTP
 * @param url     адрес запроса на сервере
 * @param method  метод запроса на сервере
 * @param headers заголовки отправляемые на сервер
 * @param end     размер сообщения в байтах
 * @return        идентификатор нового запроса
 */
int32_t awh::client::AWH::send(const int32_t sid, const uri_t::url_t & url, const awh::web_t::method_t method, const unordered_multimap <string, string> & headers, const bool end) noexcept {
	// Выполняем отправку заголовков на удалённый сервер HTTP/2
	return this->_http.send(sid, url, method, headers, end);
}
/**
 * send2 Метод HTTP/2 отправки сообщения на сервер
 * @param sid    идентификатор потока
 * @param buffer буфер бинарных данных передаваемых на сервер
 * @param size   размер сообщения в байтах
 * @param flag   флаг передаваемого потока по сети
 * @return       результат отправки данных указанному клиенту
 */
bool awh::client::AWH::send2(const int32_t sid, const char * buffer, const size_t size, const awh::http2_t::flag_t flag) noexcept {
	// Выполняем отправку сообщения на сервер
	return this->_http.send2(sid, buffer, size, flag);
}
/**
 * send2 Метод HTTP/2 отправки заголовков на сервер
 * @param sid     идентификатор потока
 * @param headers заголовки отправляемые на сервер
 * @param flag    флаг передаваемого потока по сети
 * @return        идентификатор нового запроса
 */
int32_t awh::client::AWH::send2(const int32_t sid, const vector <pair <string, string>> & headers, const awh::http2_t::flag_t flag) noexcept {
	// Выполняем отправку заголовков на сервер
	return this->_http.send2(sid, headers, flag);
}
/**
 * pause Метод установки на паузу клиента Websocket
 */
void awh::client::AWH::pause() noexcept {
	// Выполняем постановку клиента Websocket на паузу
	this->_http.pause();
}
/**
 * init Метод инициализации клиента
 * @param dest        адрес назначения удалённого сервера
 * @param compressors список поддерживаемых компрессоров
 */
void awh::client::AWH::init(const string & dest, const vector <awh::http_t::compressor_t> & compressors) noexcept {
	// Если список доступных компрессоров пустой
	if(compressors.empty())
		// Выполняем инициализацию клиента с ранее установленными компрессорами
		this->_http.init(dest, this->_compressors);
	// Выполняем инициализацию клиента
	else this->_http.init(dest, compressors);
}
/**
 * GET Метод запроса в формате HTTP методом GET
 * @param url     адрес запроса
 * @param headers заголовки запроса
 * @return        результат запроса
 */
vector <char> awh::client::AWH::GET(const uri_t::url_t & url, const unordered_multimap <string, string> & headers) noexcept {
	// Устанавливаем тепло запроса
	vector <char> result;
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		// Выполняем HTTP-запрос на сервер
		this->REQUEST(awh::web_t::method_t::GET, url, nullptr, 0, * const_cast <unordered_multimap <string, string> *> (&headers), result);
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		/**
		 * Если включён режим отладки
		 */
		#if defined(DEBUG_MODE)
			// Выводим сообщение об ошибке
			this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(static_cast <uint16_t> (awh::web_t::method_t::GET), url, headers.size()), log_t::flag_t::CRITICAL, error.what());
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
 * DEL Метод запроса в формате HTTP методом DEL
 * @param url     адрес запроса
 * @param headers заголовки запроса
 * @return        результат запроса
 */
vector <char> awh::client::AWH::DEL(const uri_t::url_t & url, const unordered_multimap <string, string> & headers) noexcept {
	// Устанавливаем тепло запроса
	vector <char> result;
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		// Выполняем HTTP-запрос на сервер
		this->REQUEST(awh::web_t::method_t::DEL, url, nullptr, 0, * const_cast <unordered_multimap <string, string> *> (&headers), result);
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		/**
		 * Если включён режим отладки
		 */
		#if defined(DEBUG_MODE)
			// Выводим сообщение об ошибке
			this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(static_cast <uint16_t> (awh::web_t::method_t::DEL), url, headers.size()), log_t::flag_t::CRITICAL, error.what());
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
 * PUT Метод запроса в формате HTTP методом PUT
 * @param url     адрес запроса
 * @param entity  тело запроса
 * @param headers заголовки запроса
 * @return        результат запроса
 */
vector <char> awh::client::AWH::PUT(const uri_t::url_t & url, const vector <char> & entity, const unordered_multimap <string, string> & headers) noexcept {
	// Устанавливаем тепло запроса
	vector <char> result;
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		// Выполняем HTTP-запрос на сервер
		this->REQUEST(awh::web_t::method_t::PUT, url, entity.data(), entity.size(), * const_cast <unordered_multimap <string, string> *> (&headers), result);
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		/**
		 * Если включён режим отладки
		 */
		#if defined(DEBUG_MODE)
			// Выводим сообщение об ошибке
			this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(static_cast <uint16_t> (awh::web_t::method_t::PUT), url, entity.size(), headers.size()), log_t::flag_t::CRITICAL, error.what());
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
 * PUT Метод запроса в формате HTTP методом PUT
 * @param url     адрес запроса
 * @param entity  тело запроса
 * @param size    размер тела запроса
 * @param headers заголовки запроса
 * @return        результат запроса
 */
vector <char> awh::client::AWH::PUT(const uri_t::url_t & url, const char * entity, const size_t size, const unordered_multimap <string, string> & headers) noexcept {
	// Устанавливаем тепло запроса
	vector <char> result;
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		// Выполняем HTTP-запрос на сервер
		this->REQUEST(awh::web_t::method_t::PUT, url, entity, size, * const_cast <unordered_multimap <string, string> *> (&headers), result);
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		/**
		 * Если включён режим отладки
		 */
		#if defined(DEBUG_MODE)
			// Выводим сообщение об ошибке
			this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(static_cast <uint16_t> (awh::web_t::method_t::PUT), url, entity, size, headers.size()), log_t::flag_t::CRITICAL, error.what());
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
 * PUT Метод запроса в формате HTTP методом PUT
 * @param url     адрес запроса
 * @param entity  тело запроса
 * @param headers заголовки запроса
 * @return        результат запроса
 */
vector <char> awh::client::AWH::PUT(const uri_t::url_t & url, const unordered_multimap <string, string> & entity, const unordered_multimap <string, string> & headers) noexcept {
	// Устанавливаем тепло запроса
	vector <char> result;
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		// Тело в формате X-WWW-Form-Urlencoded
		string body = "";
		// Переходим по всему списку тела запроса
		for(auto & param : entity){
			// Есди данные уже набраны
			if(!body.empty())
				// Добавляем разделитель
				body.append("&");
			// Добавляем в список набор параметров
			body.append(this->_uri.encode(param.first));
			// Добавляем разделитель
			body.append("=");
			// Добавляем значение
			body.append(this->_uri.encode(param.second));
		}
		// Если заголовки переданы заполненными
		if(!headers.empty()){
			// Выполняем перебор всех установленных заголовков
			for(auto i = headers.begin(); i != headers.end();){
				// Если заголовок соответствует типу контента
				if(this->_fmk->compare("Content-Type", i->second))
					// Выполняем удаление записи
					i = const_cast <unordered_multimap <string, string> &> (headers).erase(i);
				// Продолжаем перебор дальше
				else ++i;
			}
		}
		// Добавляем заголовок типа контента
		const_cast <unordered_multimap <string, string> &> (headers).emplace("Content-Type", "application/x-www-form-urlencoded");
		// Выполняем HTTP-запрос на сервер
		this->REQUEST(awh::web_t::method_t::PUT, url, body.c_str(), body.length(), * const_cast <unordered_multimap <string, string> *> (&headers), result);
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		/**
		 * Если включён режим отладки
		 */
		#if defined(DEBUG_MODE)
			// Выводим сообщение об ошибке
			this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(static_cast <uint16_t> (awh::web_t::method_t::PUT), url, entity.size(), headers.size()), log_t::flag_t::CRITICAL, error.what());
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
 * POST Метод запроса в формате HTTP методом POST
 * @param url     адрес запроса
 * @param entity  тело запроса
 * @param headers заголовки запроса
 * @return        результат запроса
 */
vector <char> awh::client::AWH::POST(const uri_t::url_t & url, const vector <char> & entity, const unordered_multimap <string, string> & headers) noexcept {
	// Устанавливаем тепло запроса
	vector <char> result;
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		// Выполняем HTTP-запрос на сервер
		this->REQUEST(awh::web_t::method_t::POST, url, entity.data(), entity.size(), * const_cast <unordered_multimap <string, string> *> (&headers), result);
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		/**
		 * Если включён режим отладки
		 */
		#if defined(DEBUG_MODE)
			// Выводим сообщение об ошибке
			this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(static_cast <uint16_t> (awh::web_t::method_t::POST), url, entity.size(), headers.size()), log_t::flag_t::CRITICAL, error.what());
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
 * POST Метод запроса в формате HTTP методом POST
 * @param url     адрес запроса
 * @param entity  тело запроса
 * @param size    размер тела запроса
 * @param headers заголовки запроса
 * @return        результат запроса
 */
vector <char> awh::client::AWH::POST(const uri_t::url_t & url, const char * entity, const size_t size, const unordered_multimap <string, string> & headers) noexcept {
	// Устанавливаем тепло запроса
	vector <char> result;
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		// Выполняем HTTP-запрос на сервер
		this->REQUEST(awh::web_t::method_t::POST, url, entity, size, * const_cast <unordered_multimap <string, string> *> (&headers), result);
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		/**
		 * Если включён режим отладки
		 */
		#if defined(DEBUG_MODE)
			// Выводим сообщение об ошибке
			this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(static_cast <uint16_t> (awh::web_t::method_t::POST), url, entity, size, headers.size()), log_t::flag_t::CRITICAL, error.what());
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
 * POST Метод запроса в формате HTTP методом POST
 * @param url     адрес запроса
 * @param entity  тело запроса
 * @param headers заголовки запроса
 * @return        результат запроса
 */
vector <char> awh::client::AWH::POST(const uri_t::url_t & url, const unordered_multimap <string, string> & entity, const unordered_multimap <string, string> & headers) noexcept {
	// Устанавливаем тепло запроса
	vector <char> result;
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		// Тело в формате X-WWW-Form-Urlencoded
		string body = "";
		// Переходим по всему списку тела запроса
		for(auto & param : entity){
			// Есди данные уже набраны
			if(!body.empty())
				// Добавляем разделитель
				body.append("&");
			// Добавляем в список набор параметров
			body.append(this->_uri.encode(param.first));
			// Добавляем разделитель
			body.append("=");
			// Добавляем значение
			body.append(this->_uri.encode(param.second));
		}
		// Если заголовки переданы заполненными
		if(!headers.empty()){
			// Выполняем перебор всех установленных заголовков
			for(auto i = headers.begin(); i != headers.end();){
				// Если заголовок соответствует типу контента
				if(this->_fmk->compare("Content-Type", i->second))
					// Выполняем удаление записи
					i = const_cast <unordered_multimap <string, string> &> (headers).erase(i);
				// Продолжаем перебор дальше
				else ++i;
			}
		}
		// Добавляем заголовок типа контента
		const_cast <unordered_multimap <string, string> &> (headers).emplace("Content-Type", "application/x-www-form-urlencoded");
		// Выполняем HTTP-запрос на сервер
		this->REQUEST(awh::web_t::method_t::POST, url, body.c_str(), body.length(), * const_cast <unordered_multimap <string, string> *> (&headers), result);
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		/**
		 * Если включён режим отладки
		 */
		#if defined(DEBUG_MODE)
			// Выводим сообщение об ошибке
			this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(static_cast <uint16_t> (awh::web_t::method_t::POST), url, entity.size(), headers.size()), log_t::flag_t::CRITICAL, error.what());
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
 * PATCH Метод запроса в формате HTTP методом PATCH
 * @param url     адрес запроса
 * @param entity  тело запроса
 * @param headers заголовки запроса
 * @return        результат запроса
 */
vector <char> awh::client::AWH::PATCH(const uri_t::url_t & url, const vector <char> & entity, const unordered_multimap <string, string> & headers) noexcept {
	// Устанавливаем тепло запроса
	vector <char> result;
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		// Выполняем HTTP-запрос на сервер
		this->REQUEST(awh::web_t::method_t::PATCH, url, entity.data(), entity.size(), * const_cast <unordered_multimap <string, string> *> (&headers), result);
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		/**
		 * Если включён режим отладки
		 */
		#if defined(DEBUG_MODE)
			// Выводим сообщение об ошибке
			this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(static_cast <uint16_t> (awh::web_t::method_t::PATCH), url, entity.size(), headers.size()), log_t::flag_t::CRITICAL, error.what());
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
 * PATCH Метод запроса в формате HTTP методом PATCH
 * @param url     адрес запроса
 * @param entity  тело запроса
 * @param size    размер тела запроса
 * @param headers заголовки запроса
 * @return        результат запроса
 */
vector <char> awh::client::AWH::PATCH(const uri_t::url_t & url, const char * entity, const size_t size, const unordered_multimap <string, string> & headers) noexcept {
	// Устанавливаем тепло запроса
	vector <char> result;
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		// Выполняем HTTP-запрос на сервер
		this->REQUEST(awh::web_t::method_t::PATCH, url, entity, size, * const_cast <unordered_multimap <string, string> *> (&headers), result);
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		/**
		 * Если включён режим отладки
		 */
		#if defined(DEBUG_MODE)
			// Выводим сообщение об ошибке
			this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(static_cast <uint16_t> (awh::web_t::method_t::PATCH), url, entity, size, headers.size()), log_t::flag_t::CRITICAL, error.what());
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
 * PATCH Метод запроса в формате HTTP методом PATCH
 * @param url     адрес запроса
 * @param entity  тело запроса
 * @param headers заголовки запроса
 * @return        результат запроса
 */
vector <char> awh::client::AWH::PATCH(const uri_t::url_t & url, const unordered_multimap <string, string> & entity, const unordered_multimap <string, string> & headers) noexcept {
	// Устанавливаем тепло запроса
	vector <char> result;
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		// Тело в формате X-WWW-Form-Urlencoded
		string body = "";
		// Переходим по всему списку тела запроса
		for(auto & param : entity){
			// Есди данные уже набраны
			if(!body.empty())
				// Добавляем разделитель
				body.append("&");
			// Добавляем в список набор параметров
			body.append(this->_uri.encode(param.first));
			// Добавляем разделитель
			body.append("=");
			// Добавляем значение
			body.append(this->_uri.encode(param.second));
		}
		// Если заголовки переданы заполненными
		if(!headers.empty()){
			// Выполняем перебор всех установленных заголовков
			for(auto i = headers.begin(); i != headers.end();){
				// Если заголовок соответствует типу контента
				if(this->_fmk->compare("Content-Type", i->second))
					// Выполняем удаление записи
					i = const_cast <unordered_multimap <string, string> &> (headers).erase(i);
				// Продолжаем перебор дальше
				else ++i;
			}
		}
		// Добавляем заголовок типа контента
		const_cast <unordered_multimap <string, string> &> (headers).emplace("Content-Type", "application/x-www-form-urlencoded");
		// Выполняем HTTP-запрос на сервер
		this->REQUEST(awh::web_t::method_t::PATCH, url, body.c_str(), body.length(), * const_cast <unordered_multimap <string, string> *> (&headers), result);
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		/**
		 * Если включён режим отладки
		 */
		#if defined(DEBUG_MODE)
			// Выводим сообщение об ошибке
			this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(static_cast <uint16_t> (awh::web_t::method_t::PATCH), url, entity.size(), headers.size()), log_t::flag_t::CRITICAL, error.what());
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
 * HEAD Метод запроса в формате HTTP методом HEAD
 * @param url     адрес запроса
 * @param headers заголовки запроса
 * @return        результат запроса
 */
unordered_multimap <string, string> awh::client::AWH::HEAD(const uri_t::url_t & url, const unordered_multimap <string, string> & headers) noexcept {
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		// Устанавливаем тепло запроса
		vector <char> entity;
		// Устанавливаем тепло запроса
		unordered_multimap <string, string> result = headers;
		// Выполняем HTTP-запрос на сервер
		this->REQUEST(awh::web_t::method_t::HEAD, url, nullptr, 0, result, entity);
		// Выводим результат
		return result;
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		/**
		 * Если включён режим отладки
		 */
		#if defined(DEBUG_MODE)
			// Выводим сообщение об ошибке
			this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(static_cast <uint16_t> (awh::web_t::method_t::HEAD), url, headers.size()), log_t::flag_t::CRITICAL, error.what());
		/**
		* Если режим отладки не включён
		*/
		#else
			// Выводим сообщение об ошибке
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
	// Выводим результат
	return unordered_multimap <string, string> ();
}
/**
 * TRACE Метод запроса в формате HTTP методом TRACE
 * @param url     адрес запроса
 * @param headers заголовки запроса
 * @return        результат запроса
 */
unordered_multimap <string, string> awh::client::AWH::TRACE(const uri_t::url_t & url, const unordered_multimap <string, string> & headers) noexcept {
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		// Устанавливаем тепло запроса
		vector <char> entity;
		// Устанавливаем тепло запроса
		unordered_multimap <string, string> result = headers;
		// Выполняем HTTP-запрос на сервер
		this->REQUEST(awh::web_t::method_t::TRACE, url, nullptr, 0, result, entity);
		// Выводим результат
		return result;
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		/**
		 * Если включён режим отладки
		 */
		#if defined(DEBUG_MODE)
			// Выводим сообщение об ошибке
			this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(static_cast <uint16_t> (awh::web_t::method_t::TRACE), url, headers.size()), log_t::flag_t::CRITICAL, error.what());
		/**
		* Если режим отладки не включён
		*/
		#else
			// Выводим сообщение об ошибке
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
	// Выводим результат
	return unordered_multimap <string, string> ();
}
/**
 * OPTIONS Метод запроса в формате HTTP методом OPTIONS
 * @param url     адрес запроса
 * @param headers заголовки запроса
 * @return        результат запроса
 */
unordered_multimap <string, string> awh::client::AWH::OPTIONS(const uri_t::url_t & url, const unordered_multimap <string, string> & headers) noexcept {
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		// Устанавливаем тепло запроса
		vector <char> entity;
		// Устанавливаем тепло запроса
		unordered_multimap <string, string> result = headers;
		// Выполняем HTTP-запрос на сервер
		this->REQUEST(awh::web_t::method_t::OPTIONS, url, nullptr, 0, result, entity);
		// Выводим результат
		return result;
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		/**
		 * Если включён режим отладки
		 */
		#if defined(DEBUG_MODE)
			// Выводим сообщение об ошибке
			this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(static_cast <uint16_t> (awh::web_t::method_t::OPTIONS), url, headers.size()), log_t::flag_t::CRITICAL, error.what());
		/**
		* Если режим отладки не включён
		*/
		#else
			// Выводим сообщение об ошибке
			this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
		#endif
	}
	// Выводим результат
	return unordered_multimap <string, string> ();
}
/**
 * REQUEST Метод выполнения запроса HTTP
 * @param method  метод запроса
 * @param url     адрес запроса
 * @param entity  тело запроса
 * @param headers заголовки запроса
 */
void awh::client::AWH::REQUEST(const awh::web_t::method_t method, const uri_t::url_t & url, vector <char> & entity, unordered_multimap <string, string> & headers) noexcept {
	// Результат работы функции
	vector <char> result;
	// Выполняем запрос на удалённый сервер
	this->REQUEST(method, url, entity.data(), entity.size(), headers, result);
	// Если результат получен
	if(!result.empty())
		// Выполняем установку полученного результата
		entity.assign(result.begin(), result.end());
}
/**
 * REQUEST Метод выполнения запроса HTTP
 * @param method  метод запроса
 * @param url     адрес запроса
 * @param entity  тело запроса
 * @param size    размер тела запроса
 * @param headers заголовки запроса
 * @param result  результат работы функции
 */
void awh::client::AWH::REQUEST(const awh::web_t::method_t method, const uri_t::url_t & url, const char * entity, const size_t size, unordered_multimap <string, string> & headers, vector <char> & result) noexcept {
	// Если данные запроса переданы
	if(!url.empty()){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Создаём объект запроса
			web_t::request_t request;
			// Устанавливаем адрес запроса
			request.url = url;
			// Устанавливаем метод запроса
			request.method = method;
			// Запоминаем переданные заголовки
			request.headers = headers;
			// Если тело запроса передано
			if((entity != nullptr) && (size > 0))
				// Устанавливаем тепло запроса
				request.entity.assign(entity, entity + size);
			// Создаём объект ответа сервера
			response_t response(headers, result);
			/**
			 * Подписываемся на событие коннекта и дисконнекта клиента
			 * @param mode событие модуля HTTP
			 */
			this->callback <void (const web_t::mode_t)> ("active", [&request, this](const web_t::mode_t mode) noexcept -> void {
				// Если подключение выполнено
				if(mode == client::web_t::mode_t::CONNECT)
					// Выполняем запрос на сервер
					this->send(request);
				// Выполняем остановку работы модуля
				else this->stop();
			});
			/**
			 * Подписываемся на получение сообщения сервера
			 * @param sid     идентификатор потока
			 * @param rid     идентификатор запроса
			 * @param code    код ответа сервера
			 * @param message сообщение ответа сервера
			 */
			this->callback <void (const int32_t, const uint64_t, const uint32_t, const string &)> ("response", [&response, this](const int32_t sid, const uint64_t rid, const uint32_t code, const string & message) noexcept -> void {
				// Устанавливаем идентификатор потока
				response.sid = sid;
				// Устанавливаем идентификатор запроса
				response.rid = rid;
				// Устанавливаем код ответа сервера
				response.code = code;
				// Устанавливаем сообщение ответа сервера
				response.message = message;
				// Если возникла ошибка, выводим сообщение
				if(response.code >= 300){
					/**
					 * Если включён режим отладки
					 */
					#if defined(DEBUG_MODE)
						// Выводим сообщение об ошибке
						this->_log->debug("%s: %u %s", __PRETTY_FUNCTION__, {}, log_t::flag_t::WARNING, "Request failed", response.code, response.message.c_str());
					/**
					* Если режим отладки не включён
					*/
					#else
						// Выводим сообщение об ошибке
						this->_log->print("%s: %u %s", log_t::flag_t::WARNING, "Request failed", response.code, response.message.c_str());
					#endif
				}
			});
			/**
			 * Подписываем на событие получения ответа с сервера
			 * @param sid     идентификатор потока
			 * @param rid     идентификатор запроса
			 * @param code    код ответа сервера
			 * @param message сообщение ответа сервера
			 * @param entity  данные полученного тела сообщения
			 * @param headers данные полученных заголовков сообщения
			 */
			this->callback <void (const int32_t, const uint64_t, const uint32_t, const string &, const vector <char> &, const unordered_multimap <string, string> &)> ("complete", [&response, this](const int32_t sid, const uint64_t rid, const uint32_t code, const string & message, const vector <char> & entity, const unordered_multimap <string, string> & headers) noexcept -> void {
				// Устанавливаем идентификатор потока
				response.sid = sid;
				// Устанавливаем идентификатор запроса
				response.rid = rid;
				// Устанавливаем код ответа сервера
				response.code = code;
				// Устанавливаем сообщение ответа сервера
				response.message = message;
				// Если заголовки ответа получены
				if(!headers.empty()){
					/**
					 * Выполняем отлов ошибок
					 */
					try {
						// Извлекаем полученный список заголовков
						response.headers = headers;
						// Если тело ответа получено
						if(!entity.empty())
							// Формируем результат ответа
							response.entity.assign(entity.begin(), entity.end());
						// Выполняем очистку тела запроса
						else response.entity.clear();
					/**
					 * Если возникает ошибка
					 */
					} catch(const length_error & error) {
						/**
						 * Если включён режим отладки
						 */
						#if defined(DEBUG_MODE)
							// Выводим сообщение об ошибке
							this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(response.sid, response.rid, response.code, response.message, response.entity.data(), response.entity.size(), response.headers.size()), log_t::flag_t::CRITICAL, error.what());
						/**
						* Если режим отладки не включён
						*/
						#else
							// Выводим сообщение об ошибке
							this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
						#endif
					/**
					 * Если возникает ошибка
					 */
					} catch(const exception & error) {
						/**
						 * Если включён режим отладки
						 */
						#if defined(DEBUG_MODE)
							// Выводим сообщение об ошибке
							this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(response.sid, response.rid, response.code, response.message, response.entity.data(), response.entity.size(), response.headers.size()), log_t::flag_t::CRITICAL, error.what());
						/**
						* Если режим отладки не включён
						*/
						#else
							// Выводим сообщение об ошибке
							this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
						#endif
					}
				}
				// Выполняем остановку
				this->stop();
			});
			// Если список доступных компрессоров пустой
			if(this->_compressors.empty()){
				// Выполняем инициализацию подключения
				this->init(this->_uri.origin(url), {
					awh::http_t::compressor_t::ZSTD,
					awh::http_t::compressor_t::BROTLI,
					awh::http_t::compressor_t::GZIP,
					awh::http_t::compressor_t::DEFLATE
				});
			// Выполняем инициализацию клиента
			} else this->init(this->_uri.origin(url), this->_compressors);
			// Выполняем запуск работы
			this->start();
		/**
		 * Если возникает ошибка
		 */
		} catch(const length_error & error) {
			/**
			 * Если включён режим отладки
			 */
			#if defined(DEBUG_MODE)
				// Выводим сообщение об ошибке
				this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(static_cast <uint16_t> (method), url, entity, size), log_t::flag_t::CRITICAL, error.what());
			/**
			* Если режим отладки не включён
			*/
			#else
				// Выводим сообщение об ошибке
				this->_log->print("%s", log_t::flag_t::CRITICAL, error.what());
			#endif
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			/**
			 * Если включён режим отладки
			 */
			#if defined(DEBUG_MODE)
				// Выводим сообщение об ошибке
				this->_log->debug("%s", __PRETTY_FUNCTION__, make_tuple(static_cast <uint16_t> (method), url, entity, size), log_t::flag_t::CRITICAL, error.what());
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
/**
 * open Метод открытия подключения
 */
void awh::client::AWH::open() noexcept {
	// Выполняем открытие подключения
	this->_http.open();
}
/**
 * reset Метод принудительного сброса подключения
 */
void awh::client::AWH::reset() noexcept {
	// Выполняем отправку сигнала таймаута
	this->_http.reset();
}
/**
 * stop Метод остановки клиента
 */
void awh::client::AWH::stop() noexcept {
	// Выполняем остановку работы модуля
	this->_http.stop();
}
/**
 * start Метод запуска клиента
 */
void awh::client::AWH::start() noexcept {
	// Выполняем запуск работы модуля
	this->_http.start();
}
/**
 * waitPong Метод установки времени ожидания ответа WebSocket-сервера
 * @param sec время ожидания в секундах
 */
void awh::client::AWH::waitPong(const time_t sec) noexcept {
	// Выполняем установку времени ожидания
	this->_http.waitPong(sec);
}
/**
 * pingInterval Метод установки интервала времени выполнения пингов
 * @param sec интервал времени выполнения пингов в секундах
 */
void awh::client::AWH::pingInterval(const time_t sec) noexcept {
	// Выполняем установку интервала времени выполнения пингов в секундах
	this->_http.pingInterval(sec);
}
/**
 * callbacks Метод установки функций обратного вызова
 * @param callbacks функции обратного вызова
 */
void awh::client::AWH::callbacks(const fn_t & callbacks) noexcept {
	// Выполняем установку функций обратного вызова
	this->_http.callbacks(callbacks);
}
/**
 * subprotocol Метод установки поддерживаемого сабпротокола
 * @param subprotocol сабпротокол для установки
 */
void awh::client::AWH::subprotocol(const string & subprotocol) noexcept {
	// Выполняем установку поддерживаемого сабпротокола
	this->_http.subprotocol(subprotocol);
}
/**
 * subprotocol Метод получения списка выбранных сабпротоколов
 * @return список выбранных сабпротоколов
 */
const set <string> & awh::client::AWH::subprotocols() const noexcept {
	// Выполняем извлечение списка выбранных сабпротоколов
	return this->_http.subprotocols();
}
/**
 * subprotocols Метод установки списка поддерживаемых сабпротоколов
 * @param subprotocols сабпротоколы для установки
 */
void awh::client::AWH::subprotocols(const set <string> & subprotocols) noexcept {
	// Выполняем установку поддерживаемых сабпротоколов
	this->_http.subprotocols(subprotocols);
}
/**
 * extensions Метод извлечения списка расширений Websocket
 * @return список поддерживаемых расширений
 */
const vector <vector <string>> & awh::client::AWH::extensions() const noexcept {
	// Выполняем извлечение списка расширений
	return this->_http.extensions();
}
/**
 * extensions Метод установки списка расширений Websocket
 * @param extensions список поддерживаемых расширений
 */
void awh::client::AWH::extensions(const vector <vector <string>> & extensions) noexcept {
	// Выполняем установку списка расширений
	this->_http.extensions(extensions);
}
/**
 * bandwidth Метод установки пропускной способности сети
 * @param read  пропускная способность на чтение (bps, kbps, Mbps, Gbps)
 * @param write пропускная способность на запись (bps, kbps, Mbps, Gbps)
 */
void awh::client::AWH::bandwidth(const string & read, const string & write) noexcept {
	// Выполняем установку пропускной способности сети
	this->_http.bandwidth(read, write);
}
/**
 * mode Метод установки флагов настроек модуля
 * @param flags список флагов настроек модуля для установки
 */
void awh::client::AWH::mode(const set <web_t::flag_t> & flags) noexcept {
	// Выполняем установку флагов настроек модуля
	this->_http.mode(flags);
}
/**
 * settings Модуль установки настроек протокола HTTP/2
 * @param settings список настроек протокола HTTP/2
 */
void awh::client::AWH::settings(const map <awh::http2_t::settings_t, uint32_t> & settings) noexcept {
	// Выполняем установку списока настроек протокола HTTP/2
	this->_http.settings(settings);
}
/**
 * chunk Метод установки размера чанка
 * @param size размер чанка для установки
 */
void awh::client::AWH::chunk(const size_t size) noexcept {
	// Выполняем установку размера чанка
	this->_http.chunk(size);
}
/**
 * segmentSize Метод установки размеров сегментов фрейма Websocket
 * @param size минимальный размер сегмента
 */
void awh::client::AWH::segmentSize(const size_t size) noexcept {
	// Выполняем установку размера сегмента фрейма Websocket
	this->_http.segmentSize(size);
}
/**
 * attempts Метод установки общего количества попыток
 * @param attempts общее количество попыток
 */
void awh::client::AWH::attempts(const uint8_t attempts) noexcept {
	// Выполняем установку количества попыток редиректа
	this->_http.attempts(attempts);
}
/**
 * hosts Метод загрузки файла со списком хостов
 * @param filename адрес файла для загрузки
 */
void awh::client::AWH::hosts(const string & filename) noexcept {
	// Если адрес файла с хостами в операционной системе передан
	if(!filename.empty())
		// Выполняем установку адреса файла хостов в операционной системе
		this->_dns.hosts(filename);
}
/**
 * user Метод установки параметров авторизации
 * @param login    логин пользователя для авторизации на сервере
 * @param password пароль пользователя для авторизации на сервере
 */
void awh::client::AWH::user(const string & login, const string & password) noexcept {
	// Выполняем установку логина и пароля пользователя
	this->_http.user(login, password);
}
/**
 * compressors Метод установки списка поддерживаемых компрессоров
 * @param compressors список поддерживаемых компрессоров
 */
void awh::client::AWH::compressors(const vector <awh::http_t::compressor_t> & compressors) noexcept {
	// Выполняем установку списка доступных компрессоров
	this->_compressors = compressors;
	// Выполняем установку списка поддерживаемых компрессоров
	this->_http.compressors(this->_compressors);
}
/**
 * keepAlive Метод установки жизни подключения
 * @param cnt   максимальное количество попыток
 * @param idle  интервал времени в секундах через которое происходит проверка подключения
 * @param intvl интервал времени в секундах между попытками
 */
void awh::client::AWH::keepAlive(const int32_t cnt, const int32_t idle, const int32_t intvl) noexcept {
	// Выполняем установку жизни подключения
	this->_http.keepAlive(cnt, idle, intvl);
}
/**
 * multiThreads Метод активации многопоточности в Websocket
 * @param count количество потоков для активации
 * @param mode  флаг активации/деактивации мультипоточности
 */
void awh::client::AWH::multiThreads(const uint16_t count, const bool mode) noexcept {
	// Выполняем активацию многопоточности при получения данных в Websocket
	this->_http.multiThreads(count, mode);
}
/**
 * userAgent Метод установки User-Agent для HTTP-запроса
 * @param userAgent агент пользователя для HTTP-запроса
 */
void awh::client::AWH::userAgent(const string & userAgent) noexcept {
	// Выполняем установку User-Agent для HTTP-запроса
	this->_http.userAgent(userAgent);
}
/**
 * ident Метод установки идентификации клиента
 * @param id   идентификатор сервиса
 * @param name название сервиса
 * @param ver  версия сервиса
 */
void awh::client::AWH::ident(const string & id, const string & name, const string & ver) noexcept {
	// Выполняем установку данных сервиса
	this->_http.ident(id, name, ver);
}
/**
 * proxy Метод активации/деактивации прокси-склиента
 * @param work флаг активации/деактивации прокси-клиента
 */
void awh::client::AWH::proxy(const client::scheme_t::work_t work) noexcept {
	// Выполняем установку флага активации/деактивации прокси-склиента
	this->_http.proxy(work);
}
/**
 * proxy Метод установки прокси-сервера
 * @param uri    параметры прокси-сервера
 * @param family семейстово интернет протоколов (IPV4 / IPV6 / NIX)
 */
void awh::client::AWH::proxy(const string & uri, const scheme_t::family_t family) noexcept {
	// Выполняем установку прокси-сервера
	this->_http.proxy(uri, family);
}
/**
 * flushDNS Метод сброса кэша DNS-резолвера
 * @return результат работы функции
 */
bool awh::client::AWH::flushDNS() noexcept {
	// Выполняем сброс кэша DNS-резолвера
	return this->_dns.flush();
}
/**
 * timeoutDNS Метод установки времени ожидания выполнения запроса DNS-резолвера
 * @param sec интервал времени выполнения запроса в секундах
 */
void awh::client::AWH::timeoutDNS(const uint8_t sec) noexcept {
	// Если время ожидания выполнения DNS-запроса передано
	if(sec > 0)
		// Выполняем установку времени ожидания получения данных с DNS-сервера
		this->_dns.timeout(sec);
}
/**
 * prefixDNS Метод установки префикса переменной окружения для извлечения серверов имён
 * @param prefix префикс переменной окружения для установки
 */
void awh::client::AWH::prefixDNS(const string & prefix) noexcept {
	// Если префикс переменной окружения для извлечения серверов имён передан
	if(!prefix.empty())
		// Выполняем установку префикса переменной окружения
		this->_dns.prefix(prefix);
}
/**
 * clearDNSBlackList Метод очистки чёрного списка DNS-резолвера
 * @param domain доменное имя для которого очищается чёрный список
 */
void awh::client::AWH::clearDNSBlackList(const string & domain) noexcept {
	// Если доменное имя для удаления из чёрного списока передано
	if(!domain.empty())
		// Выполняем удаление доменного имени из чёрного списока
		this->_dns.clearBlackList(domain);
}
/**
 * delInDNSBlackList Метод удаления IP-адреса из чёрного списока DNS-резолвера
 * @param domain доменное имя соответствующее IP-адресу
 * @param ip     адрес для удаления из чёрного списка
 */
void awh::client::AWH::delInDNSBlackList(const string & domain, const string & ip) noexcept {
	// Если доменное имя для удаления из чёрного списока и соответствующий ему IP-адрес переданы
	if(!domain.empty() && !ip.empty())
		// Выполняем удаление доменного имени из чёрного списока
		this->_dns.delInBlackList(domain, ip);
}
/**
 * setToDNSBlackList Метод добавления IP-адреса в чёрный список DNS-резолвера
 * @param domain доменное имя соответствующее IP-адресу
 * @param ip     адрес для добавления в чёрный список
 */
void awh::client::AWH::setToDNSBlackList(const string & domain, const string & ip) noexcept {
	// Если доменное имя для добавление в чёрный список и соответствующий ему IP-адрес переданы
	if(!domain.empty() && !ip.empty())
		// Выполняем установку доменного имени в чёрный список
		this->_dns.setToBlackList(domain, ip);
}
/**
 * cork Метод отключения/включения алгоритма TCP/CORK
 * @param mode режим применимой операции
 * @return     результат выполенния операции
 */
bool awh::client::AWH::cork(const engine_t::mode_t mode) noexcept {
	// Выполняем отключение/включение алгоритма TCP/CORK
	return this->_http.cork(mode);
}
/**
 * nodelay Метод отключения/включения алгоритма Нейгла
 * @param mode режим применимой операции
 * @return     результат выполенния операции
 */
bool awh::client::AWH::nodelay(const engine_t::mode_t mode) noexcept {
	// Выполняем отключение/включение алгоритма TCP/CORK
	return this->_http.nodelay(mode);
}
/**
 * crypted Метод получения флага шифрования
 * @param sid идентификатор потока
 * @return    результат проверки
 */
bool awh::client::AWH::crypted(const int32_t sid) const noexcept {
	// Выполняем получение флага шифрования
	return this->_http.crypted(sid);
}
/**
 * encryption Метод активации шифрования
 * @param mode флаг активации шифрования
 */
void awh::client::AWH::encryption(const bool mode) noexcept {
	// Выполняем установку флага шифрования
	this->_http.encryption(mode);
}
/**
 * encryption Метод установки параметров шифрования
 * @param pass   пароль шифрования передаваемых данных
 * @param salt   соль шифрования передаваемых данных
 * @param cipher размер шифрования передаваемых данных
 */
void awh::client::AWH::encryption(const string & pass, const string & salt, const hash_t::cipher_t cipher) noexcept {
	// Выполняем установку параметров шифрования
	this->_http.encryption(pass, salt, cipher);
}
/**
 * authType Метод установки типа авторизации
 * @param type тип авторизации
 * @param hash алгоритм шифрования для Digest-авторизации
 */
void awh::client::AWH::authType(const auth_t::type_t type, const auth_t::hash_t hash) noexcept {
	// Выполняем установку типа авторизации
	this->_http.authType(type, hash);
}
/**
 * authTypeProxy Метод установки типа авторизации прокси-сервера
 * @param type тип авторизации
 * @param hash алгоритм шифрования для Digest-авторизации
 */
void awh::client::AWH::authTypeProxy(const auth_t::type_t type, const auth_t::hash_t hash) noexcept {
	// Выполняем установку типа авторизации на прокси-сервере
	this->_http.authTypeProxy(type, hash);
}
/**
 * waitMessage Метод ожидания входящих сообщений
 * @param sec интервал времени в секундах
 */
void awh::client::AWH::waitMessage(const time_t sec) noexcept {
	// Выполняем установку интервала времени ожидания входящих сообщений
	this->_http.waitMessage(sec);
}
/**
 * waitTimeDetect Метод детекции сообщений по количеству секунд
 * @param read    количество секунд для детекции по чтению
 * @param write   количество секунд для детекции по записи
 * @param connect количество секунд для детекции по подключению
 */
void awh::client::AWH::waitTimeDetect(const time_t read, const time_t write, const time_t connect) noexcept {
	// Выполняем установку детекции сообщений по количеству секунд
	this->_http.waitTimeDetect(read, write, connect);
}
/**
 * network Метод установки параметров сети
 * @param ips    список IP-адресов компьютера с которых разрешено выходить в интернет
 * @param ns     список серверов имён, через которые необходимо производить резолвинг доменов
 * @param family тип протокола интернета (IPV4 / IPV6 / NIX)
 */
void awh::client::AWH::network(const vector <string> & ips, const vector <string> & ns, const scheme_t::family_t family) noexcept {
	// Если список IP-адресов передан
	if(!ips.empty()){
		// Определяем тип протокола интернета
		switch(static_cast <uint8_t> (family)){
			// Если протокол интернета соответствует IPv4
			case static_cast <uint8_t> (scheme_t::family_t::IPV4):
				// Добавляем список IP-адресов через которые нужно выходить в интернет
				this->_dns.network(AF_INET, ips);
			break;
			// Если протокол интернета соответствует IPv6
			case static_cast <uint8_t> (scheme_t::family_t::IPV6):
				// Добавляем список IP-адресов через которые нужно выходить в интернет
				this->_dns.network(AF_INET6, ips);
			break;
		}
		// Устанавливаем параметры сети для сетевого ядра
		const_cast <client::core_t *> (this->_core)->network(ips, family);
	}
	// Если список DNS-серверов передан
	if(!ns.empty()){
		// Определяем тип протокола интернета
		switch(static_cast <uint8_t> (family)){
			// Если протокол интернета соответствует IPv4
			case static_cast <uint8_t> (scheme_t::family_t::IPV4):
				// Выполняем установку списка DNS-серверов
				this->_dns.replace(AF_INET, ns);
			break;
			// Если протокол интернета соответствует IPv6
			case static_cast <uint8_t> (scheme_t::family_t::IPV6):
				// Выполняем установку списка DNS-серверов
				this->_dns.replace(AF_INET6, ns);
			break;
		}
	}
}
/**
 * AWH Конструктор
 * @param core объект сетевого ядра
 * @param fmk  объект фреймворка
 * @param log  объект для работы с логами
 */
awh::client::AWH::AWH(const client::core_t * core, const fmk_t * fmk, const log_t * log) noexcept :
 _uri(fmk, log), _dns(fmk, log), _http(core, fmk, log), _fmk(fmk), _log(log), _core(core) {
	// Выполняем установку DNS-резолвера
	const_cast <client::core_t *> (core)->resolver(&this->_dns);
}
