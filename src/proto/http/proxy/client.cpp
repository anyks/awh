/**
 * @file client.cpp
 * @date 2026-11-23
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
 * @brief Реализация клиентской стороны протокола HTTP-прокси — формирование запросов CONNECT
 *        и absolute-form, разбор ответов прокси-сервера парсером HTTP/1.x
 *        и авторизация на прокси (Basic/Digest/Bearer)
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Стандартные заголовочные файлы
 */
#include <cstring>

/**
 * Подключаем заголовочные файлы проекта
 */
#include <sys/fmk.hpp>
#include <sys/log.hpp>

/**
 * Подключаем заголовочный файл модуля
 */
#include <proto/http/proxy/client.hpp>

/**
 * Используем стандартное пространство имён
 */
using namespace std;

/**
 * Используем пространство имён placeholders
 */
using namespace placeholders;

/**
 * @brief Инкапсулируем статические функции в пространство имён
 *
 */
namespace {
	/**
	 * Используем пространство имён AWH
	 */
	using namespace awh;

	/**
	 * @brief Функция регистронезависимого сравнения строк (ASCII)
	 *
	 * @param left  первая строка
	 * @param right вторая строка
	 * @return      результат сравнения
	 *
	 */
	static __AWH_INLINE__ bool __awh_iequals__(const string_view left, const string_view right) noexcept {
		// Если длины строк не совпадают
		if(left.size() != right.size())
			// Строки не равны
			return false;
		/**
		 * Сравниваем строки посимвольно
		 */
		for(size_t i = 0; i < left.size(); i++){
			// Символ левой строки в нижнем регистре
			const char leftChar = ((left[i] >= 'A') && (left[i] <= 'Z')) ? static_cast <char> (left[i] + ('a' - 'A')) : left[i];
			// Символ правой строки в нижнем регистре
			const char rightChar = ((right[i] >= 'A') && (right[i] <= 'Z')) ? static_cast <char> (right[i] + ('a' - 'A')) : right[i];
			// Если символы не совпадают
			if(leftChar != rightChar)
				// Строки не равны
				return false;
		}
		// Строки равны
		return true;
	}
	/**
	 * @brief Функция проверки заголовка по токенам Connection
	 *
	 * @details Заголовки, названные токенами заголовка Connection запроса
	 *          пользователя, не пересылаются через прокси (RFC 9110 §7.6.1).
	 *
	 * @param name   имя заголовка
	 * @param tokens токены заголовка Connection
	 * @return       признак того, что заголовок не пересылается
	 *
	 */
	static __AWH_INLINE__ bool __awh_nominated__(const string_view name, const vector <string_view> & tokens) noexcept {
		/**
		 * Перебираем токены заголовка Connection
		 */
		for(const auto & token : tokens){
			// Если имя заголовка совпадает с токеном
			if(::__awh_iequals__(name, token))
				// Заголовок назван токеном и не пересылается
				return true;
		}
		// Заголовок токенами не назван
		return false;
	}
	/**
	 * @brief Функция извлечения токена схемы авторизации из значения вызова
	 *
	 * @param value значение заголовка вызова авторизации
	 * @return      токен схемы авторизации
	 *
	 */
	static __AWH_INLINE__ string_view __awh_scheme__(const string_view value) noexcept {
		// Позиция первого пробельного символа после токена схемы
		const size_t pos = value.find_first_of(" \t");
		// Возвращаем токен схемы авторизации
		return ((pos == string_view::npos) ? value : value.substr(0, pos));
	}
	/**
	 * @brief Функция проверки принадлежности заголовка к hop-by-hop
	 *
	 * @details Заголовки hop-by-hop и заголовки кадрирования тела не пересылаются
	 *          через прокси: они относятся к одному соединению, а тело запроса
	 *          кадрируется заново при сборке пересылаемого запроса.
	 *
	 * @param name имя заголовка
	 * @return     признак того, что заголовок не пересылается
	 *
	 */
	static __AWH_INLINE__ bool __awh_hop_by_hop__(const string_view name) noexcept {
		// Заголовки, не пересылаемые через прокси
		static constexpr string_view HOP_BY_HOP[] = {
			"host", "connection", "keep-alive", "proxy-authenticate", "proxy-authorization",
			"proxy-connection", "te", "trailer", "transfer-encoding", "upgrade", "content-length"
		};
		/**
		 * Перебираем заголовки, не пересылаемые через прокси
		 */
		for(const auto & item : HOP_BY_HOP){
			// Если имя заголовка совпадает с не пересылаемым
			if(::__awh_iequals__(name, item))
				// Заголовок не пересылается
				return true;
		}
		// Заголовок пересылается
		return false;
	}
	/**
	 * @brief Функция сбора токенов заголовка Connection
	 *
	 * @details Токены разделяются запятыми, окружающие пробельные символы
	 *          не значимы (RFC 9110 §5.6.1); собранные токены называют
	 *          заголовки запроса, не пересылаемые через прокси (§7.6.1).
	 *
	 * @param value  значение заголовка Connection
	 * @param tokens контейнер собранных токенов
	 *
	 */
	static __AWH_INLINE__ void __awh_connection_tokens__(const string_view value, vector <string_view> & tokens) noexcept {
		// Позиция начала очередного токена
		size_t begin = 0;
		/**
		 * Разбираем список токенов, разделённых запятыми
		 */
		while(begin <= value.size()){
			// Позиция разделителя после текущего токена
			const size_t pos = value.find(',', begin);
			// Позиция конца текущего токена
			const size_t end = ((pos == string_view::npos) ? value.size() : pos);
			// Позиция начала токена без ведущих пробельных символов
			size_t first = begin;
			// Позиция конца токена без хвостовых пробельных символов
			size_t last = end;
			/**
			 * Пропускаем ведущие пробельные символы токена
			 */
			while((first < last) && ((value[first] == ' ') || (value[first] == '\t')))
				// Сдвигаем начало токена
				first++;
			/**
			 * Пропускаем хвостовые пробельные символы токена
			 */
			while((last > first) && ((value[last - 1] == ' ') || (value[last - 1] == '\t')))
				// Сдвигаем конец токена
				last--;
			// Если токен не пуст
			if(last > first)
				// Сохраняем токен
				tokens.push_back(value.substr(first, last - first));
			// Если разделитель не найден
			if(pos == string_view::npos)
				// Завершаем разбор списка
				break;
			// Сдвигаем начало за разделитель
			begin = pos + 1;
		}
	}
};

/**
 * @brief Конструктор
 *
 */
awh::http::proxy::Client::Context::Context() noexcept :
 mode(mode_t::TUNNEL), state(state_t::NONE),
 code(0), retries(0), host(nullptr) {}

/**
 * @brief Конструктор
 *
 */
awh::http::proxy::Client::Forward_Request::Forward_Request() noexcept :
 sent(false), retries(0), method(method_t::NONE),
 uri{""}, body{""}, wire{""}, methodName{""} {}

/**
 * @brief Метод настройки парсеров и подписки на их события
 *
 */
void awh::http::proxy::Client::setup() noexcept {
	// Устанавливаем протокол работы парсера ответов — соединение с прокси-сервером
	this->_rx.proto(proto_t::PROXY1);
	// Устанавливаем протокол работы парсера запросов — соединение с прокси-сервером
	this->_tx.proto(proto_t::PROXY1);
	// Объект лимитов безопасности парсера ответов
	parser_http_t::limits_t limits;
	// Устанавливаем лимит суммарного размера заголовков ответа прокси
	limits.maxHeadersTotal = HTTP_PROXY_RX_MAX_HEADERS;
	// Применяем лимиты безопасности к парсеру ответов
	this->_rx.limits(limits);
	// Устанавливаем ёмкость выходного буфера отправки запросов
	this->_tx.sendWaterMarks(HTTP_PROXY_TX_HIGH_WATER, parser_http_t::SEND_LOW_WATER);
	// Подписываемся на событие обработки тела запроса пользователя
	this->_tx.on(parser_http_t::data_callback_t(std::bind(&client_t::requestBody, this, _1, _2, _3, _4)));
	// Подписываемся на событие обработки тела ответа прокси
	this->_rx.on(parser_http_t::data_callback_t(std::bind(&client_t::responseData, this, _1, _2, _3, _4)));
	// Подписываемся на событие готовности провайдера запроса пользователя
	this->_tx.on(parser_http_t::provider_callback_t(std::bind(&client_t::requestProvider, this, _1, _2, _3)));
	// Подписываемся на событие обработки заголовков запроса пользователя
	this->_tx.on(parser_http_t::header_callback_t(std::bind(&client_t::requestHeader, this, _1, _2, _3, _4)));
	// Подписываемся на событие обработки заголовков ответа прокси
	this->_rx.on(parser_http_t::header_callback_t(std::bind(&client_t::responseHeader, this, _1, _2, _3, _4)));
	// Подписываемся на событие готовности провайдера ответа прокси
	this->_rx.on(parser_http_t::provider_callback_t(std::bind(&client_t::responseProvider, this, _1, _2, _3)));
}
/**
 * @brief Метод постановки разобранного запроса в очередь конвейера
 *
 * @param ctx объект контекста для установки параметров сообщения
 * @return    результат постановки запроса в очередь
 *
 */
bool awh::http::proxy::Client::enqueue(ctx_t & ctx) noexcept {
	// Если метод запроса пользователя не определён
	if((this->_method == method_t::NONE) && this->_methodName.empty())
		// Запрос не поставлен в очередь
		return false;
	// Объект пересылаемого запроса
	forward_request_t request;
	// Сохраняем тело запроса пользователя
	request.body = this->_body;
	// Сохраняем метод запроса пользователя
	request.method = this->_method;
	// Сохраняем заголовки запроса пользователя
	request.fields = this->_fields;
	// Сохраняем трейлеры запроса пользователя
	request.trailers = this->_trailers;
	// Сохраняем оригинальное написание метода запроса
	request.methodName = this->_methodName;
	// Цель запроса в absolute-form
	string target = this->_uri;
	// Если цель запроса не в absolute-form
	if((target.compare(0, 7, "http://") != 0) && (target.compare(0, 8, "https://") != 0)){
		// Значение заголовка Host запроса пользователя
		string host = "";
		/**
		 * Ищем заголовок Host в запросе пользователя
		 */
		for(const auto & field : this->_fields){
			// Если заголовок является заголовком Host
			if(::__awh_iequals__(field.first, "host")){
				// Сохраняем значение заголовка Host
				host = field.second;
				// Завершаем поиск
				break;
			}
		}
		// Если заголовок Host не найден
		if(host.empty())
			// Запрос не поставлен в очередь
			return false;
		// Формируем цель запроса в absolute-form
		target = "http://" + host + this->_uri;
	}
	// Если цель запроса указывает на https
	if(target.compare(0, 8, "https://") == 0){
		// Устанавливаем состояние ошибки: пересылка https невозможна без туннеля
		ctx.state = state_t::BROKEN;
		// Запрос не поставлен в очередь
		return false;
	}
	// Сохраняем цель запроса в absolute-form
	request.uri = target;
	// Собираем байты запроса (учётные данные добавляются при превентивной авторизации или состоявшемся вызове)
	if(!this->assemble(request, this->_challenged)){
		// Устанавливаем состояние ошибки сборки запроса
		ctx.state = state_t::BROKEN;
		// Запрос не поставлен в очередь
		return false;
	}
	// Ставим запрос в очередь конвейера
	this->_queue.push_back(::move(request));
	// Запрос поставлен в очередь
	return true;
}
/**
 * @brief Метод сборки запроса CONNECT
 *
 * @param ctx объект контекста для установки параметров сообщения
 * @return    результат сборки запроса
 *
 */
bool awh::http::proxy::Client::connect(ctx_t & ctx) noexcept {
	// Формируем строку authority целевого хоста
	const string target = this->authority(ctx);
	// Если строка authority не сформирована
	if(target.empty())
		// Запрос не собран
		return false;
	// Контейнер заголовков запроса CONNECT
	headers_t headers(make_unique <request_t> (version_t::HTTP1_1, method_t::CONNECT, target));
	// Добавляем обязательный заголовок Host
	headers.emplace("Host", target);
	// Устанавливаем метод запроса для расчёта ответа Digest
	this->_auth.method("CONNECT");
	// Устанавливаем цель запроса для расчёта ответа Digest
	this->_auth.uri(target);
	// Если выполняется повторная авторизация по ответу 407
	if(ctx.state == state_t::AUTH)
		// Добавляем заголовок авторизации на прокси
		this->authorize(headers);
	// Если включён режим превентивной авторизации
	else if(this->_preemptive) {
		// Если установлен токен доступа
		if(!this->_token.empty())
			// Выбираем схему авторизации Bearer
			this->_auth.type(auth_t::type_t::BEARER);
		// Если установлены учётные данные пользователя
		else if(!this->_username.empty() && !this->_password.empty())
			// Выбираем схему авторизации Basic
			this->_auth.type(auth_t::type_t::BASIC);
		// Добавляем заголовок авторизации на прокси
		this->authorize(headers);
	}
	// Отправляем блок заголовков запроса CONNECT (тела нет)
	this->_tx.sendHeaders(headers, true);
	// Если исходящий буфер пуст
	if(this->_tx.pending().empty())
		// Запрос не собран
		return false;
	// Устанавливаем состояние ожидания ответа прокси
	ctx.state = state_t::REQUEST;
	// Запрос собран
	return true;
}
/**
 * @brief Метод обработки полностью разобранного ответа прокси
 *
 * @param ctx объект контекста для установки параметров сообщения
 *
 */
void awh::http::proxy::Client::complete(ctx_t & ctx) noexcept {
	// Если ответ промежуточный (1xx)
	if((ctx.code >= 100) && (ctx.code < 200))
		// Сбрасываем парсер для разбора финального ответа
		this->_rx.reset();
	// Если режим работы — туннель
	else if(ctx.mode == mode_t::TUNNEL) {
		// Если требуется авторизация на прокси
		if(ctx.code == 407)
			// Повторяем авторизацию по вызову прокси
			this->reauthorize(ctx);
		// Если ответ успешный (2xx)
		else if((ctx.code >= 200) && (ctx.code < 300))
			// Устанавливаем состояние установленного туннеля
			ctx.state = state_t::HANDSHAKE;
		// Прочие ответы означают ошибку установки туннеля
		else ctx.state = state_t::BROKEN;
	// Если режим работы — пересылка
	} else {
		// Если очередь запросов пуста — ответ без запроса
		if(this->_queue.empty())
			// Устанавливаем состояние ошибки обмена
			ctx.state = state_t::BROKEN;
		// Сопоставляем ответ голове очереди конвейера (FIFO)
		else {
			// Извлекаем запрос, которому соответствует ответ
			forward_request_t request = ::move(this->_queue.front());
			// Удаляем запрос из головы очереди
			this->_queue.pop_front();
			// Если требуется авторизация на прокси
			if(ctx.code == 407){
				// Если повторы запроса не исчерпаны и схема выбрана по вызову
				if((request.retries < HTTP_PROXY_AUTH_RETRIES) && this->challenge(ctx)){
					// Помечаем вызов авторизации прокси состоявшимся в соединении
					this->_challenged = true;
					// Увеличиваем счётчик повторов авторизации запроса
					request.retries++;
					// Пересобираем запрос с учётными данными
					if(this->assemble(request, true)){
						// Помечаем запрос неотправленным
						request.sent = false;
						// Ставим повторный запрос в хвост очереди
						this->_queue.push_back(::move(request));
						// Устанавливаем состояние ожидания ответа
						ctx.state = state_t::REQUEST;
					// Сборка повторного запроса не выполнена
					} else ctx.state = state_t::BROKEN;
				// Повторная авторизация невозможна
				} else {
					// Если функция обратного вызова установлена
					if(this->_response)
						// Выдаём голову ответа 407 вызывающей стороне
						this->_response(ctx.code, ctx.headers);
					// Устанавливаем состояние ошибки обмена
					ctx.state = state_t::BROKEN;
				}
			// Финальный ответ доставлен (голова и тело выданы колбэками)
			} else ctx.state = (this->_queue.empty() ? state_t::NONE : state_t::REQUEST);
		}
	}
	// Если трейлеры ответа получены и функция обратного вызова установлена
	if(!this->_suppress && !ctx.trailers.empty() && this->_trailer)
		// Выдаём трейлеры ответа вызывающей стороне
		this->_trailer(ctx.trailers);
}
/**
 * @brief Метод повторной авторизации по ответу 407
 *
 * @param ctx объект контекста для установки параметров сообщения
 * @return    результат возможности повторной авторизации
 *
 */
bool awh::http::proxy::Client::reauthorize(ctx_t & ctx) noexcept {
	// Если повторы не исчерпаны и схема авторизации выбрана по вызову прокси
	if((ctx.retries < HTTP_PROXY_AUTH_RETRIES) && this->challenge(ctx)){
		// Устанавливаем состояние повторной авторизации
		ctx.state = state_t::AUTH;
		// Увеличиваем счётчик выполненных повторов
		ctx.retries++;
		// Повторная авторизация возможна
		return true;
	}
	// Если функция обратного вызова готовности головы ответа установлена
	if(this->_response)
		// Выдаём голову ответа 407 вызывающей стороне
		this->_response(ctx.code, ctx.headers);
	// Устанавливаем состояние ошибки обмена
	ctx.state = state_t::BROKEN;
	// Повторная авторизация невозможна
	return false;
}
/**
 * @brief Метод выбора схемы авторизации по вызову прокси
 *
 * @param ctx объект контекста с заголовками ответа прокси
 * @return    результат выбора и разбора схемы авторизации
 *
 */
bool awh::http::proxy::Client::challenge(const ctx_t & ctx) noexcept {
	// Значение вызова схемы Basic
	string_view basic = "";
	// Значение вызова схемы Digest
	string_view digest = "";
	// Значение вызова схемы Bearer
	string_view bearer = "";
	/**
	 * Перебираем заголовки ответа прокси
	 */
	for(const auto & header : ctx.headers){
		// Если заголовок не является вызовом авторизации прокси
		if(!::__awh_iequals__(header.first, "proxy-authenticate"))
			// Переходим к следующему заголовку
			continue;
		// Токен схемы авторизации вызова
		const string_view token = ::__awh_scheme__(header.second);
		// Если вызов предлагает схему Digest и она ещё не найдена
		if(digest.empty() && ::__awh_iequals__(token, "digest"))
			// Сохраняем значение вызова Digest
			digest = header.second;
		// Если вызов предлагает схему Basic и она ещё не найдена
		else if(basic.empty() && ::__awh_iequals__(token, "basic"))
			// Сохраняем значение вызова Basic
			basic = header.second;
		// Если вызов предлагает схему Bearer и она ещё не найдена
		else if(bearer.empty() && ::__awh_iequals__(token, "bearer"))
			// Сохраняем значение вызова Bearer
			bearer = header.second;
	}
	// Если установлены учётные данные пользователя
	if(!this->_username.empty() && !this->_password.empty()){
		// Если прокси предлагает схему Digest
		if(!digest.empty()){
			// Выбираем схему авторизации Digest
			this->_auth.type(auth_t::type_t::DIGEST);
			// Разбираем вызов схемы Digest
			return this->_auth.parse(digest);
		}
		// Если прокси предлагает схему Basic
		if(!basic.empty()){
			// Выбираем схему авторизации Basic
			this->_auth.type(auth_t::type_t::BASIC);
			// Разбираем вызов схемы Basic
			return this->_auth.parse(basic);
		}
	}
	// Если установлен токен доступа и прокси предлагает схему Bearer
	if(!this->_token.empty() && !bearer.empty()){
		// Выбираем схему авторизации Bearer
		this->_auth.type(auth_t::type_t::BEARER);
		// Разбираем вызов схемы Bearer
		return this->_auth.parse(bearer);
	}
	// Подходящая схема авторизации не найдена
	return false;
}
/**
 * @brief Метод формирования строки authority целевого хоста
 *
 * @param ctx объект контекста с параметрами целевого хоста
 * @return    строка authority (host:port, IPv6 в скобках)
 *
 */
string awh::http::proxy::Client::authority(const ctx_t & ctx) noexcept {
	// Строка authority целевого хоста
	string result = "";
	// Если параметры адреса целевого хоста установлены
	if(ctx.host != nullptr){
		/**
		 * Определяем тип адреса целевого хоста
		 */
		switch(static_cast <uint8_t> (ctx.host->type)){
			// Для типа FQDN
			case static_cast <uint8_t> (net::type_t::FQDN):
				// Формируем строку authority из доменного имени и порта
				result = fmk::format("%s:%u", awh_cast <const net::attr_fqdn_t *> (ctx.host.get())->domain.c_str(), awh_cast <const net::attr_fqdn_t *> (ctx.host.get())->port);
			break;
			// Для типа IPv4
			case static_cast <uint8_t> (net::type_t::IPV4): {
				// Устанавливаем IP-адрес целевого хоста в объект сетевых адресов
				this->_addr.source(awh_cast <const net::attr_net_t *> (ctx.host.get())->ip.get());
				// Формируем строку authority из IP-адреса и порта
				result = fmk::format("%s:%u", static_cast <string> (this->_addr).c_str(), awh_cast <const net::attr_net_t *> (ctx.host.get())->port);
			} break;
			// Для типа IPv6
			case static_cast <uint8_t> (net::type_t::IPV6): {
				// Устанавливаем IP-адрес целевого хоста в объект сетевых адресов
				this->_addr.source(awh_cast <const net::attr_net_t *> (ctx.host.get())->ip.get());
				// Текстовое представление IPv6-адреса целевого хоста
				string address = static_cast <string> (this->_addr);
				/**
				 * Приводим шестнадцатеричные символы адреса к нижнему регистру (RFC 5952 §4.3)
				 */
				for(auto & symbol : address){
					// Если символ является заглавной шестнадцатеричной цифрой
					if((symbol >= 'A') && (symbol <= 'F'))
						// Приводим символ к нижнему регистру
						symbol = static_cast <char> (symbol + ('a' - 'A'));
				}
				// Формируем строку authority из IP-адреса в скобках и порта
				result = fmk::format("[%s]:%u", address.c_str(), awh_cast <const net::attr_net_t *> (ctx.host.get())->port);
			} break;
		}
	}
	// Возвращаем строку authority целевого хоста
	return result;
}
/**
 * @brief Метод добавления заголовка авторизации в контейнер запроса
 *
 * @param headers контейнер заголовков запроса
 *
 */
void awh::http::proxy::Client::authorize(headers_t & headers) noexcept {
	// Если схема авторизации установлена
	if(this->_auth.type() != auth_t::type_t::NONE){
		// Формируем значение заголовка учётных данных
		const string credentials = this->_auth.header();
		// Если значение заголовка сформировано
		if(!credentials.empty())
			// Добавляем заголовок авторизации на прокси
			headers.emplace("Proxy-Authorization", credentials);
	}
}
/**
 * @brief Метод сборки байтов пересылаемого запроса в absolute-form
 *
 * @param request     объект пересылаемого запроса
 * @param credentials флаг принудительного добавления учётных данных
 * @return            результат сборки запроса
 *
 */
bool awh::http::proxy::Client::assemble(forward_request_t & request, const bool credentials) noexcept {
	// Объект провайдера пересылаемого запроса
	auto provider = make_unique <request_t> (version_t::HTTP1_1, request.method, request.uri);
	// Если метод запроса не распознан парсером
	if(request.method == method_t::UNKNOWN)
		// Устанавливаем оригинальное написание метода
		provider->methodName = request.methodName;
	// Контейнер заголовков пересылаемого запроса
	headers_t headers(::move(provider));
	// Позиция начала authority в цели запроса
	const size_t begin = 7;
	// Позиция первого символа пути в цели запроса
	const size_t end = request.uri.find('/', begin);
	// Authority целевого хоста для заголовка Host
	const string host = ((end == string::npos) ? request.uri.substr(begin) : request.uri.substr(begin, end - begin));
	// Добавляем заголовок Host по authority цели запроса
	headers.emplace("Host", host);
	// Токены заголовков, названных заголовком Connection запроса пользователя
	vector <string_view> nominated;
	/**
	 * Собираем токены заголовка Connection запроса пользователя
	 */
	for(const auto & field : request.fields){
		// Если заголовок является заголовком Connection
		if(::__awh_iequals__(field.first, "connection"))
			// Собираем токены значения заголовка
			::__awh_connection_tokens__(field.second, nominated);
	}
	/**
	 * Копируем заголовки запроса пользователя, исключая не пересылаемые
	 */
	for(const auto & field : request.fields){
		// Если заголовок не пересылается через прокси
		if(::__awh_hop_by_hop__(field.first) || ::__awh_nominated__(field.first, nominated))
			// Переходим к следующему заголовку
			continue;
		// Добавляем заголовок запроса пользователя
		headers.emplace(field.first, field.second, headers_t::mode_t::APPEND);
	}
	// Объект провайдера запроса для получения имени метода
	request_t method(request.method);
	// Устанавливаем оригинальное написание метода
	method.methodName = request.methodName;
	// Устанавливаем метод запроса для расчёта ответа Digest
	this->_auth.method(string(methodName(&method)));
	// Устанавливаем цель запроса для расчёта ответа Digest
	this->_auth.uri(request.uri);
	// Если добавление учётных данных принудительное (повторная авторизация)
	if(credentials)
		// Добавляем заголовок авторизации на прокси
		this->authorize(headers);
	// Если включён режим превентивной авторизации
	else if(this->_preemptive) {
		// Если установлен токен доступа
		if(!this->_token.empty())
			// Выбираем схему авторизации Bearer
			this->_auth.type(auth_t::type_t::BEARER);
		// Если установлены учётные данные пользователя
		else if(!this->_username.empty() && !this->_password.empty())
			// Выбираем схему авторизации Basic
			this->_auth.type(auth_t::type_t::BASIC);
		// Добавляем заголовок авторизации на прокси
		this->authorize(headers);
	}
	// Готовим отправитель к сборке очередного запроса
	this->_tx.resetSender();
	// Если тело запроса пользователя отсутствует
	if(request.body.empty())
		// Отправляем блок заголовков пересылаемого запроса (тела нет)
		this->_tx.sendHeaders(headers, true);
	// Если у запроса пользователя есть трейлеры
	else if(!request.trailers.empty()) {
		// Отправляем блок заголовков пересылаемого запроса (тело кадрируется chunked)
		this->_tx.sendHeaders(headers, false);
		// Отправляем тело запроса без завершения сообщения
		if(this->_tx.sendData(request.body.data(), request.body.size(), false) != request.body.size())
			// Запрос не собран
			return false;
		// Контейнер трейлеров пересылаемого запроса
		headers_t trailers;
		/**
		 * Копируем трейлеры запроса пользователя
		 */
		for(const auto & field : request.trailers)
			// Добавляем трейлер запроса пользователя
			trailers.emplace(field.first, field.second, headers_t::mode_t::APPEND);
		// Отправляем блок трейлеров, завершающий тело сообщения
		this->_tx.sendHeaders(trailers, true);
	// Если тело запроса пользователя присутствует без трейлеров
	} else {
		// Объявляем размер тела пересылаемого запроса
		headers.emplace("Content-Length", ::to_string(request.body.size()));
		// Отправляем блок заголовков пересылаемого запроса
		this->_tx.sendHeaders(headers, false);
		// Отправляем тело запроса, завершая сообщение
		if(this->_tx.sendData(request.body.data(), request.body.size(), true) != request.body.size())
			// Запрос не собран
			return false;
	}
	// Если исходящий буфер пуст
	if(this->_tx.pending().empty())
		// Запрос не собран
		return false;
	// Сохраняем сформированные байты запроса
	request.wire.assign(this->_tx.pending().data(), this->_tx.pending().size());
	// Освобождаем исходящий буфер отправителя
	this->_tx.consumePending(request.wire.size());
	// Запрос собран
	return true;
}
/**
 * @brief Метод обработки провайдера разобранного ответа прокси
 *
 * @param id       идентификатор потока
 * @param provider объект провайдера стартовой строки ответа
 * @param end      флаг завершения сообщения
 * @return         результат обработки
 *
 */
bool awh::http::proxy::Client::responseProvider(const uint32_t, const provider_t * provider, const bool) noexcept {
	// Если активный контекст или провайдер не установлены
	if((this->_ctx == nullptr) || (provider == nullptr))
		// Продолжаем разбор
		return true;
	// Получаем код состояния ответа прокси
	const uint16_t code = awh_cast <const response_t *> (provider)->code;
	// Устанавливаем код состояния ответа в контекст
	this->_ctx->code = code;
	// Устанавливаем флаг подавления тела: ответы 407 и неуспешные ответы туннеля не выдаются
	this->_suppress = ((code == 407) || ((this->_ctx->mode == mode_t::TUNNEL) && ((code < 200) || (code >= 300))));
	// Если ответ финальный и не 407, а функция обратного вызова установлена
	if((code >= 200) && (code != 407) && this->_response)
		// Вызываем функцию обратного вызова готовности головы ответа
		this->_response(code, this->_ctx->headers);
	// Продолжаем разбор
	return true;
}
/**
 * @brief Метод обработки фрагмента тела разобранного ответа прокси
 *
 * @param id     идентификатор потока
 * @param buffer буфер фрагмента тела
 * @param size   размер фрагмента тела
 * @param end    флаг завершения тела
 * @return       результат обработки
 *
 */
bool awh::http::proxy::Client::responseData(const uint32_t id, const void * buffer, const size_t size, const bool end) noexcept {
	// Если выдача тела не подавлена и функция обратного вызова установлена
	if(!this->_suppress && this->_data)
		// Вызываем функцию обратного вызова обработки тела ответа
		return this->_data(id, buffer, size, end);
	// Продолжаем разбор
	return true;
}
/**
 * @brief Метод обработки заголовков разобранного ответа прокси
 *
 * @param id    идентификатор потока
 * @param name  имя заголовка
 * @param value значение заголовка
 * @param part  часть сообщения (заголовки/трейлеры)
 * @return      результат обработки
 *
 */
bool awh::http::proxy::Client::responseHeader(const uint32_t, const string_view name, const string_view value, const parser_t::part_t part) noexcept {
	// Если активный контекст установлен и получены заголовки сообщения
	if((this->_ctx != nullptr) && (part == parser_t::part_t::HEADERS))
		// Сохраняем заголовок ответа прокси в контекст
		this->_ctx->headers.emplace_back(string(name), string(value));
	// Если активный контекст установлен и получены трейлеры сообщения
	else if((this->_ctx != nullptr) && (part == parser_t::part_t::TRAILER))
		// Сохраняем трейлер ответа прокси в контекст
		this->_ctx->trailers.emplace_back(string(name), string(value));
	// Продолжаем разбор
	return true;
}
/**
 * @brief Метод обработки провайдера разобранного запроса пользователя
 *
 * @param id       идентификатор потока
 * @param provider объект провайдера стартовой строки запроса
 * @param end      флаг завершения сообщения
 * @return         результат обработки
 *
 */
bool awh::http::proxy::Client::requestProvider(const uint32_t, const provider_t * provider, const bool) noexcept {
	// Если провайдер не установлен
	if(provider == nullptr)
		// Продолжаем разбор
		return true;
	// Получаем объект провайдера запроса пользователя
	const request_t * request = awh_cast <const request_t *> (provider);
	// Сохраняем метод запроса пользователя
	this->_method = request->method;
	// Сохраняем оригинальное написание метода запроса
	this->_methodName = request->methodName;
	// Сохраняем цель URI запроса пользователя
	this->_uri = request->uri;
	// Продолжаем разбор
	return true;
}
/**
 * @brief Метод обработки фрагмента тела разобранного запроса пользователя
 *
 * @param id     идентификатор потока
 * @param buffer буфер фрагмента тела
 * @param size   размер фрагмента тела
 * @param end    флаг завершения тела
 * @return       результат обработки
 *
 */
bool awh::http::proxy::Client::requestBody(const uint32_t, const void * buffer, const size_t size, const bool) noexcept {
	// Если фрагмент тела не пустой
	if((buffer != nullptr) && (size > 0))
		// Добавляем фрагмент тела к телу запроса пользователя
		this->_body.append(static_cast <const char *> (buffer), size);
	// Продолжаем разбор
	return true;
}
/**
 * @brief Метод обработки заголовков разобранного запроса пользователя
 *
 * @param id    идентификатор потока
 * @param name  имя заголовка
 * @param value значение заголовка
 * @param part  часть сообщения (заголовки/трейлеры)
 * @return      результат обработки
 *
 */
bool awh::http::proxy::Client::requestHeader(const uint32_t, const string_view name, const string_view value, const parser_t::part_t part) noexcept {
	// Если получен трейлер запроса пользователя
	if(part == parser_t::part_t::TRAILER)
		// Сохраняем трейлер запроса
		this->_trailers.emplace_back(string(name), string(value));
	// Если получен заголовок запроса пользователя
	else if(part == parser_t::part_t::HEADERS)
		// Сохраняем заголовок запроса
		this->_fields.emplace_back(string(name), string(value));
	// Продолжаем разбор
	return true;
}
/**
 * @brief Метод уведомления о завершении потока данных от прокси
 *
 */
void awh::http::proxy::Client::eof() noexcept {
	// Уведомляем парсер ответов о закрытии соединения прокси
	this->_rx.eof();
}
/**
 * @brief Метод уведомления о завершении потока данных от прокси с обработкой ответа
 *
 * @param ctx объект контекста для установки параметров сообщения
 *
 */
void awh::http::proxy::Client::eof(ctx_t & ctx) noexcept {
	// Уведомляем парсер ответов о закрытии соединения прокси
	this->_rx.eof();
	// Если ответ кадрируемый закрытием соединения дочитан полностью
	if(this->_rx.status() == parser_t::status_t::COMPLETE)
		// Обрабатываем полностью разобранный ответ прокси
		this->complete(ctx);
}
/**
 * @brief Метод полной очистки для нового соединения
 *
 * @param ctx объект контекста для сброса параметров сообщения
 *
 */
void awh::http::proxy::Client::clear(ctx_t & ctx) noexcept {
	// Очищаем парсер ответов полностью
	this->_rx.clear();
	// Очищаем парсер запросов полностью
	this->_tx.clear();
	// Восстанавливаем настройки парсеров и подписки на их события
	this->setup();
	// Сбрасываем флаг состоявшегося вызова авторизации прокси
	this->_challenged = false;
	// Сбрасываем состояние обмена и накопленные параметры
	this->reset(ctx);
}
/**
 * @brief Метод подготовки к следующему обмену в том же соединении
 *
 * @param ctx объект контекста для сброса параметров сообщения
 *
 */
void awh::http::proxy::Client::reset(ctx_t & ctx) noexcept {
	// Сбрасываем код состояния ответа прокси
	ctx.code = 0;
	// Сбрасываем счётчик повторов авторизации
	ctx.retries = 0;
	// Сбрасываем состояние разбора ответов прокси
	this->_rx.reset();
	// Сбрасываем состояние разбора и отправки запросов
	this->_tx.reset();
	// Очищаем цель URI запроса
	this->_uri.clear();
	// Очищаем накопленное тело запроса
	this->_body.clear();
	// Очищаем очередь конвейера пересылаемых запросов
	this->_queue.clear();
	// Очищаем накопленные заголовки запроса
	this->_fields.clear();
	// Очищаем накопленные трейлеры запроса
	this->_trailers.clear();
	// Очищаем оригинальное написание метода запроса
	this->_methodName.clear();
	// Сбрасываем метод запроса
	this->_method = method_t::NONE;
	// Сбрасываем активный контекст разбора
	this->_ctx = nullptr;
	// Сбрасываем флаг подавления выдачи тела
	this->_suppress = false;
	// Очищаем заголовки ответа прокси
	ctx.headers.clear();
	// Очищаем трейлеры ответа прокси
	ctx.trailers.clear();
	// Устанавливаем неопределённое состояние обмена
	ctx.state = state_t::NONE;
}
/**
 * @brief Метод получения названия текущей ошибки разбора
 *
 * @return название текущей ошибки разбора ответа прокси
 *
 */
string_view awh::http::proxy::Client::error() const noexcept {
	// Возвращаем название текущей ошибки разбора ответа прокси
	return this->_rx.errorName();
}
/**
 * @brief Метод освобождения отправленных байтов запроса
 *
 * @param size число отправленных байтов
 *
 */
void awh::http::proxy::Client::consume(const size_t size) noexcept {
	// Если очередь конвейера не пуста — режим пересылки
	if(!this->_queue.empty()){
		/**
		 * Помечаем первый неотправленный запрос отправленным
		 */
		for(auto & request : this->_queue){
			// Если запрос ещё не отправлен прокси
			if(!request.sent){
				// Если отправлены все байты запроса
				if(size >= request.wire.size()){
					// Помечаем запрос отправленным
					request.sent = true;
					// Освобождаем байты запроса
					request.wire.clear();
				// Если отправлена только часть байтов запроса
				} else request.wire.erase(0, size);
				// Завершаем поиск
				break;
			}
		}
	// Если очередь пуста — освобождаем отправленные байты отправителя (туннель)
	} else this->_tx.consumePending(size);
}
/**
 * @brief Метод установки режима превентивной авторизации
 *
 * @param mode флаг превентивной отправки учётных данных
 *
 */
void awh::http::proxy::Client::preemptive(const bool mode) noexcept {
	// Устанавливаем режим превентивной отправки учётных данных
	this->_preemptive = mode;
}
/**
 * @brief Метод установки токена доступа BEARER
 *
 * @param token токен доступа для авторизации на прокси
 *
 */
void awh::http::proxy::Client::setToken(const string & token) noexcept {
	// Сохраняем токен доступа для авторизации
	this->_token = token;
	// Устанавливаем токен доступа в объект авторизации
	this->_auth.token(token);
}
/**
 * @brief Метод установки параметров авторизации
 *
 * @param username имя пользователя для авторизации на прокси
 * @param password пароль пользователя для авторизации на прокси
 *
 */
void awh::http::proxy::Client::setUser(const string & username, const string & password) noexcept {
	// Сохраняем имя пользователя для авторизации
	this->_username = username;
	// Сохраняем пароль пользователя для авторизации
	this->_password = password;
	// Устанавливаем логин пользователя в объект авторизации
	this->_auth.user(username);
	// Устанавливаем пароль пользователя в объект авторизации
	this->_auth.pass(password);
}
/**
 * @brief Метод извлечения буфера запроса к прокси
 *
 * @param buffer указатель на буфер для извлечения данных
 * @param size   ссылка на размер буфера для извлечения данных
 * @param ctx    объект контекста для установки параметров сообщения
 * @return       результат извлечения данных в буфер
 *
 */
bool awh::http::proxy::Client::buffer(uint8_t ** buffer, size_t & size, ctx_t & ctx) noexcept {
	// Если указатель на буфер не установлен
	if(buffer == nullptr)
		// Буфер не извлечён
		return false;
	// Если режим работы — пересылка
	if(ctx.mode == mode_t::FORWARD){
		/**
		 * Ищем первый неотправленный запрос очереди конвейера
		 */
		for(auto & request : this->_queue){
			// Если запрос ещё не отправлен прокси
			if(!request.sent){
				// Устанавливаем указатель на буфер запроса
				*buffer = reinterpret_cast <uint8_t *> (const_cast <char *> (request.wire.data()));
				// Устанавливаем размер буфера запроса
				size = request.wire.size();
				// Буфер извлечён
				return true;
			}
		}
		// Неотправленных запросов нет
		return false;
	}
	/**
	 * Определяем текущее состояние клиента
	 */
	switch(static_cast <uint8_t> (ctx.state)){
		// Если состояние не определено — собираем первый запрос
		case static_cast <uint8_t> (state_t::NONE): {
			// Если режим работы — туннель
			if(ctx.mode == mode_t::TUNNEL){
				// Если сборка запроса CONNECT не выполнена
				if(!this->connect(ctx))
					// Буфер не извлечён
					return false;
			// Режим пересылки требует предварительного приёма запроса пользователя
			} else return false;
		} break;
		// Если требуется повторная авторизация по ответу 407
		case static_cast <uint8_t> (state_t::AUTH): {
			// Если сборка запроса CONNECT с учётными данными не выполнена
			if(!this->connect(ctx))
				// Буфер не извлечён
				return false;
		} break;
		// Если запрос уже собран пересылкой
		case static_cast <uint8_t> (state_t::REQUEST):
			// Переходим к извлечению буфера
			break;
		// В остальных случаях запрос не собирается
		default:
			// Буфер не извлечён
			return false;
	}
	// Представление ещё не отправленных исходящих байтов
	const string_view pending = this->_tx.pending();
	// Если исходящий буфер пуст
	if(pending.empty())
		// Буфер не извлечён
		return false;
	// Устанавливаем указатель на буфер запроса
	(* buffer) = const_cast <uint8_t *> (reinterpret_cast <const uint8_t *> (pending.data()));
	// Устанавливаем размер буфера запроса
	size = pending.size();
	// Буфер извлечён
	return true;
}
/**
 * @brief Метод парсинга входящих данных от прокси
 *
 * @param buffer бинарный буфер входящих данных
 * @param size   размер бинарного буфера входящих данных
 * @param ctx    объект для извлечения параметров сообщения
 * @return       количество потреблённых байт данных
 *
 */
size_t awh::http::proxy::Client::parse(const void * buffer, const size_t size, ctx_t & ctx) noexcept {
	// Если данные не переданы либо запрос к прокси ещё не отправлен
	if((buffer == nullptr) || (size == 0) || (ctx.state != state_t::REQUEST))
		// Данные не потребляются
		return 0;
	// Устанавливаем активный контекст разбора
	this->_ctx = &ctx;
	// Если предыдущий ответ разобран полностью — готовим парсер к следующему
	if(this->_rx.status() == parser_t::status_t::COMPLETE)
		// Сбрасываем состояние разбора ответа
		this->_rx.reset();
	// Если парсер находится в чистом состоянии — начинается новое сообщение
	if(this->_rx.status() == parser_t::status_t::NONE){
		// Очищаем заголовки предыдущего ответа
		ctx.headers.clear();
		// Очищаем трейлеры предыдущего ответа
		ctx.trailers.clear();
		// Сбрасываем флаг подавления выдачи тела
		this->_suppress = false;
	}
	// Устанавливаем метод запроса, которому соответствует ожидаемый ответ
	if(ctx.mode == mode_t::TUNNEL)
		// Ответ на CONNECT открывает туннель и тела не имеет
		this->_rx.method(method_t::CONNECT);
	// Режим пересылки: метод берётся из головы очереди конвейера (FIFO)
	else this->_rx.method(this->_queue.empty() ? method_t::NONE : this->_queue.front().method);
	// Разбираем данные ответа прокси
	const size_t consumed = this->_rx.parse(buffer, size);
	/**
	 * Определяем итоговый статус разбора ответа
	 */
	switch(static_cast <uint8_t> (this->_rx.status())){
		// Если разбор завершён ошибкой
		case static_cast <uint8_t> (parser_t::status_t::ERROR):
			// Устанавливаем состояние ошибки обмена
			ctx.state = state_t::BROKEN;
		break;
		// Если сообщение разобрано полностью
		case static_cast <uint8_t> (parser_t::status_t::COMPLETE):
			// Обрабатываем полностью разобранный ответ прокси
			this->complete(ctx);
		break;
	}
	// Сбрасываем активный контекст разбора
	this->_ctx = nullptr;
	// Возвращаем количество потреблённых байт данных
	return consumed;
}
/**
 * @brief Метод приёма запроса пользователя для пересылки через прокси
 *
 * @param buffer буфер данных запроса пользователя
 * @param size   размер данных запроса пользователя
 * @param ctx    объект контекста для установки параметров сообщения
 * @return       результат приёма данных запроса
 *
 */
bool awh::http::proxy::Client::forward(const void * buffer, const size_t size, ctx_t & ctx) noexcept {
	// Если режим работы не пересылка либо обмен сломан
	if((ctx.mode != mode_t::FORWARD) || (ctx.state == state_t::BROKEN))
		// Данные запроса не приняты
		return false;
	// Если данные запроса не переданы
	if((buffer == nullptr) || (size == 0))
		// Данные запроса не приняты
		return false;
	// Если парсер запросов находится в чистом состоянии — начинается новое сообщение
	if(this->_tx.status() == parser_t::status_t::NONE){
		// Очищаем цель URI запроса
		this->_uri.clear();
		// Очищаем накопленное тело запроса
		this->_body.clear();
		// Очищаем накопленные заголовки запроса
		this->_fields.clear();
		// Очищаем накопленные трейлеры запроса
		this->_trailers.clear();
		// Очищаем оригинальное написание метода запроса
		this->_methodName.clear();
		// Сбрасываем метод запроса
		this->_method = method_t::NONE;
	}
	// Буфер данных запроса пользователя
	const uint8_t * data = static_cast <const uint8_t *> (buffer);
	// Смещение потреблённых байт в буфере
	size_t offset = 0;
	/**
	 * Разбираем конвейер запросов пользователя по одному сообщению
	 */
	while(offset < size){
		// Разбираем очередную порцию данных запроса
		const size_t consumed = this->_tx.parse(data + offset, size - offset);
		// Продвигаем смещение на число потреблённых байт
		offset += consumed;
		// Если разбор завершён ошибкой
		if(this->_tx.status() == parser_t::status_t::ERROR){
			// Устанавливаем состояние ошибки обмена
			ctx.state = state_t::BROKEN;
			// Данные запроса не приняты
			return false;
		}
		// Если сообщение ещё не завершено — ожидаем продолжение
		if(this->_tx.status() != parser_t::status_t::COMPLETE)
			// Выходим из цикла разбора
			break;
		// Ставим разобранный запрос в очередь конвейера
		if(!this->enqueue(ctx))
			// Данные запроса не приняты
			return false;
		// Сбрасываем парсер для разбора следующего сообщения конвейера
		this->_tx.reset();
		// Очищаем цель URI запроса
		this->_uri.clear();
		// Очищаем накопленное тело запроса
		this->_body.clear();
		// Очищаем накопленные заголовки запроса
		this->_fields.clear();
		// Очищаем накопленные трейлеры запроса
		this->_trailers.clear();
		// Очищаем оригинальное написание метода запроса
		this->_methodName.clear();
		// Сбрасываем метод запроса
		this->_method = method_t::NONE;
	}
	// Устанавливаем состояние ожидания ответа, если очередь не пуста
	ctx.state = (this->_queue.empty() ? state_t::NONE : state_t::REQUEST);
	// Данные запроса приняты
	return true;
}
/**
 * @brief Метод установки функции обратного вызова готовности трейлеров ответа
 *
 * @param callback функция обратного вызова готовности трейлеров ответа прокси
 *
 */
void awh::http::proxy::Client::on(trailer_callback_t callback) noexcept {
	// Устанавливаем функцию обратного вызова готовности трейлеров ответа
	this->_trailer = ::move(callback);
}
/**
 * @brief Метод установки функции обратного вызова готовности головы ответа
 *
 * @param callback функция обратного вызова готовности головы ответа прокси
 *
 */
void awh::http::proxy::Client::on(response_callback_t callback) noexcept {
	// Устанавливаем функцию обратного вызова готовности головы ответа
	this->_response = ::move(callback);
}
/**
 * @brief Метод установки функции обратного вызова обработки тела ответа
 *
 * @param callback функция обратного вызова обработки фрагмента тела ответа
 *
 */
void awh::http::proxy::Client::on(parser_http_t::data_callback_t callback) noexcept {
	// Устанавливаем функцию обратного вызова обработки тела ответа
	this->_data = ::move(callback);
}
/**
 * @brief Конструктор
 *
 */
awh::http::proxy::Client::Client() noexcept :
 _suppress(false), _preemptive(false), _challenged(false),
 _auth(auth_t::owner_t::CLIENT), _method(method_t::NONE),
 _rx(direct_t::RESPONSE), _tx(direct_t::REQUEST),
 _uri{""}, _body{""}, _token{""},
 _username{""}, _password{""}, _methodName{""},
 _ctx(nullptr), _trailer(nullptr), _response(nullptr), _data(nullptr) {
	// Настраиваем парсеры и подписываемся на их события
	this->setup();
	// Устанавливаем режим работы через прокси для объекта авторизации
	this->_auth.proxy(true);
}

/**
 * @brief Деструктор
 *
 */
awh::http::proxy::Client::~Client() noexcept = default;
