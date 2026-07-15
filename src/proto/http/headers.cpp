/**
 * @file: headers.cpp
 * @date: 2026-07-11
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
 * Подключаем заголовочный файл проекта
 */
#include <proto/http/headers.hpp>

/**
 * Используем стандартное пространство имён
 */
using namespace std;

/**
 * @brief Инкапсулируем статические параметры в пространство имён
 *
 */
namespace {
	/**
	 * Используем пространство имён AWH
	 */
	using namespace awh;

	/**
	 * @brief Функция приведения ASCII-символа к нижнему регистру
	 *
	 * @details Названия HTTP-заголовков и методов состоят исключительно из ASCII-символов,
	 *          поэтому используется быстрое ветвление вместо locale-зависимой ::tolower,
	 *          что исключает влияние текущей локали и ускоряет горячий путь сравнения/хеширования.
	 *
	 * @param letter символ для приведения к нижнему регистру
	 * @return       символ в нижнем регистре
	 */
	uint8_t toLower(const uint8_t letter) noexcept {
		// Если символ является заглавной латинской буквой - приводим его к нижнему регистру
		return (((letter >= 'A') && (letter <= 'Z')) ? static_cast <uint8_t> (letter + 32) : letter);
	}
	/**
	 * @brief Функция проверки, является ли ASCII-символ пробельным
	 *
	 * @param letter символ для проверки
	 * @return       результат проверки
	 */
	bool isSpace(const uint8_t letter) noexcept {
		// Проверяем принадлежность символа к набору пробельных символов ASCII
		return ((letter == ' ') || (letter == '\t') || (letter == '\n') || (letter == '\v') || (letter == '\f') || (letter == '\r'));
	}
	/**
	 * @brief Функция комбинирования хеш-кодов
	 *
	 * @param seed  исходный хеш-код
	 * @param value добавочный хеш-код
	 */
	void combine(size_t & seed, const size_t value) noexcept {
		// Комбинируем хеш-коды (константа - дробная часть золотого сечения в 64 битах)
		seed ^= (value + 0x9E3779B97F4A7C15ULL + (seed << 6) + (seed >> 2));
	}
	/**
	 * @brief Функция вычисления хеш-кода названия HTTP-заголовка
	 *
	 * @param name название заголовка для вычисления хеш-кода
	 * @return     хеш-код названия заголовка без учёта регистра
	 */
	size_t hashing(string_view name) noexcept {
		// Результат работы функции
		size_t result = 0;
		/**
		 * Вычисляем хеш названия заголовка без учёта регистра (согласуется с Header::operator ==)
		 */
		for(const auto letter : name)
			// Комбинируем хеш-код очередного символа названия заголовка (в нижнем регистре)
			::combine(result, hash <uint8_t> {}(toLower(static_cast <uint8_t> (letter))));
		// Возвращаем хеш-код
		return result;
	}
	/**
	 * @brief Функция регистронезависимого сравнения двух строк
	 *
	 * @details Названия HTTP-заголовков и методов запроса не чувствительны к регистру,
	 *          поэтому для их сравнения используется отдельная функция, а не строгое равенство строк
	 *
	 * @param first  первая строка для сравнения
	 * @param second вторая строка для сравнения
	 * @return       результат сравнения строк без учёта регистра
	 */
	bool equals(string_view first, string_view second) noexcept {
		// Если длины строк не совпадают - строки заведомо не равны
		if(first.size() != second.size())
			// Возвращаем значение по умолчанию
			return false;
		/**
		 * Выполняем последовательное побуквенное сравнение с приведением к нижнему регистру
		 */
		for(size_t i = 0; i < first.size(); i++){
			// Если очередная пара букв не совпадает - строки не равны
			if(toLower(static_cast <uint8_t> (first[i])) != toLower(static_cast <uint8_t> (second[i])))
				// Возвращаем значение по умолчанию
				return false;
		}
		// Строки полностью совпадают без учёта регистра
		return true;
	}
	/**
	 * @brief Шаблон функции поиска заголовка по названию без учёта регистра
	 *
	 * @tparam Iterator тип итератора для поиска
	 */
	template <typename Iterator>
	/**
	 * @brief Функция поиска заголовка по названию без учёта регистра
	 *
	 * @param begin начало диапазона поиска
	 * @param end   конец диапазона поиска
	 * @param name  название заголовка для поиска
	 * @return      итератор найденного заголовка либо end
	 */
	Iterator findByName(Iterator begin, Iterator end, string_view name) noexcept {
		/**
		 * Выполняем перебор заголовков для поиска совпадения по названию без учёта регистра
		 */
		for(auto i = begin; i != end; ++i){
			// Если название заголовка совпадает с указанным
			if(equals(i->name, name))
				// Возвращаем итератор найденного заголовка
				return i;
		}
		// Заголовок с указанным названием не найден
		return end;
	}
	/**
	 * @brief Функция определения метода HTTP-запроса по его текстовому названию
	 *
	 * @param method текстовое название метода HTTP-запроса
	 * @return       метод HTTP-запроса
	 */
	http::method_t method(string_view method) noexcept {
		// Если название метода соответствует GET
		if(equals(method, "GET"))
			// Возвращаем метод HTTP-запроса
			return http::method_t::GET;
		// Если название метода соответствует PUT
		else if(equals(method, "PUT"))
			// Возвращаем метод HTTP-запроса
			return http::method_t::PUT;
		// Если название метода соответствует DELETE
		else if(equals(method, "DELETE"))
			// Возвращаем метод HTTP-запроса
			return http::method_t::DEL;
		// Если название метода соответствует POST
		else if(equals(method, "POST"))
			// Возвращаем метод HTTP-запроса
			return http::method_t::POST;
		// Если название метода соответствует HEAD
		else if(equals(method, "HEAD"))
			// Возвращаем метод HTTP-запроса
			return http::method_t::HEAD;
		// Если название метода соответствует PATCH
		else if(equals(method, "PATCH"))
			// Возвращаем метод HTTP-запроса
			return http::method_t::PATCH;
		// Если название метода соответствует TRACE
		else if(equals(method, "TRACE"))
			// Возвращаем метод HTTP-запроса
			return http::method_t::TRACE;
		// Если название метода соответствует OPTIONS
		else if(equals(method, "OPTIONS"))
			// Возвращаем метод HTTP-запроса
			return http::method_t::OPTIONS;
		// Если название метода соответствует CONNECT
		else if(equals(method, "CONNECT"))
			// Возвращаем метод HTTP-запроса
			return http::method_t::CONNECT;
		// Метод не распознан
		return http::method_t::NONE;
	}
	/**
	 * @brief Функция получения текстового названия метода HTTP-запроса
	 *
	 * @param method метод HTTP-запроса
	 * @return       текстовое название метода HTTP-запроса
	 */
	string_view method(const http::method_t method) noexcept {
		/**
		 * Определяем текстовое название метода HTTP-запроса
		 */
		switch(static_cast <uint8_t> (method)){
			// Если метоод соответствует GET
			case static_cast <uint8_t> (http::method_t::GET):
				// Возвращаем текстовое название метода HTTP-запроса
				return "GET";
			// Если метоод соответствует PUT
			case static_cast <uint8_t> (http::method_t::PUT):
				// Возвращаем текстовое название метода HTTP-запроса
				return "PUT";
			// Если метоод соответствует DELETE
			case static_cast <uint8_t> (http::method_t::DEL):
				// Возвращаем текстовое название метода HTTP-запроса
				return "DELETE";
			// Если метоод соответствует POST
			case static_cast <uint8_t> (http::method_t::POST):
				// Возвращаем текстовое название метода HTTP-запроса
				return "POST";
			// Если метоод соответствует HEAD
			case static_cast <uint8_t> (http::method_t::HEAD):
				// Возвращаем текстовое название метода HTTP-запроса
				return "HEAD";
			// Если метоод соответствует PATCH
			case static_cast <uint8_t> (http::method_t::PATCH):
				// Возвращаем текстовое название метода HTTP-запроса
				return "PATCH";
			// Если метоод соответствует TRACE
			case static_cast <uint8_t> (http::method_t::TRACE):
				// Возвращаем текстовое название метода HTTP-запроса
				return "TRACE";
			// Если метоод соответствует OPTIONS
			case static_cast <uint8_t> (http::method_t::OPTIONS):
				// Возвращаем текстовое название метода HTTP-запроса
				return "OPTIONS";
			// Если метоод соответствует CONNECT
			case static_cast <uint8_t> (http::method_t::CONNECT):
				// Возвращаем текстовое название метода HTTP-запроса
				return "CONNECT";
		}
		// Возвращаем значение по умолчанию
		return "";
	}
	/**
	 * @brief Функция форматирования версии протокола HTTP в текстовый вид
	 *
	 * @param version версия протокола HTTP
	 * @return        версия протокола HTTP в текстовом виде (например "1.1")
	 */
	string version(const http::version_t version) noexcept {
		/**
		 * Определяем текстовое представление версии протокола HTTP
		 */
		switch(static_cast <uint8_t> (version)){
			// Если версия протокола соответствует HTTP/1.0
			case static_cast <uint8_t> (http::version_t::HTTP1_0):
				// Возвращаем текстовое представление версии протокола
				return "1.0";
			// Если версия протокола соответствует HTTP/1.1
			case static_cast <uint8_t> (http::version_t::HTTP1_1):
				// Возвращаем текстовое представление версии протокола
				return "1.1";
			// Если версия протокола соответствует HTTP/2
			case static_cast <uint8_t> (http::version_t::HTTP2):
				// Возвращаем текстовое представление версии протокола
				return "2.0";
			// Если версия протокола соответствует HTTP/3
			case static_cast <uint8_t> (http::version_t::HTTP3):
				// Возвращаем текстовое представление версии протокола
				return "3.0";
			// Если версия протокола соответствует HTTP/4
			case static_cast <uint8_t> (http::version_t::HTTP4):
				// Возвращаем текстовое представление версии протокола
				return "4.0";
			// Если версия протокола соответствует HTTP/5
			case static_cast <uint8_t> (http::version_t::HTTP5):
				// Возвращаем текстовое представление версии протокола
				return "5.0";
		}
		// Возвращаем значение по умолчанию
		return "0.0";
	}
	/**
	 * @brief Функция приведения названия HTTP-заголовка к канонической форме протокола по месту
	 *
	 * @details Преобразование выполняется посимвольно и не меняет длину строки, поэтому производится
	 *          непосредственно в переданной строке без выделения новой памяти. Это сохраняет внутренний
	 *          буфер строки (актуально при переносе названия заголовка, чтобы не терять семантику перемещения).
	 *
	 * @param name  название заголовка, приводимое к канонической форме
	 * @param proto версия протокола HTTP-запроса/ответа
	 */
	void applyCaseName(string & name, const http::proto_t proto) noexcept {
		/**
		 * Определяем версию протокола передачи данных
		 */
		switch(proto){
			// Для протоколов HTTP/2, HTTP/3 и их модификаций (Proxy, Websocket) названия заголовков обязаны быть в нижнем регистре
			case http::proto_t::HTTP2:
			case http::proto_t::HTTP3:
			case http::proto_t::PROXY2:
			case http::proto_t::PROXY3:
			case http::proto_t::WEBSOCKET2:
			case http::proto_t::WEBSOCKET3: {
				/**
				 * Приводим каждый символ названия заголовка к нижнему регистру по месту
				 */
				for(auto & letter : name)
					// Приводим очередной символ названия заголовка к нижнему регистру
					letter = static_cast <char> (toLower(static_cast <uint8_t> (letter)));
			} break;
			// Для остальных версий протокола название заголовка приводится к «умному» регистру
			default: {
				// Флаг детекции символа
				bool mode = true;
				/**
				 * Переходим по всем буквам названия и приводим их к канонической форме по месту
				 */
				for(size_t i = 0; i < name.length(); i++){
					// Получаем символ с которым ведётся работа в данный момент
					const char letter = name[i];
					// Если флаг перевода в верхний регистр активирован
					if(mode){
						// Приводим символ к нижнему регистру
						const uint8_t lower = toLower(static_cast <uint8_t> (letter));
						// Переводим строчную латинскую букву в верхний регистр, прочие символы оставляем без изменений
						name[i] = static_cast <char> (((lower >= 'a') && (lower <= 'z')) ? (lower - 32) : lower);
					// Переводим остальные символы в нижний регистр
					} else name[i] = static_cast <char> (toLower(static_cast <uint8_t> (letter)));
					// Если найден спецсимвол, устанавливаем флаг детекции
					mode = ((letter == '-') || (letter == '_') || isSpace(static_cast <uint8_t> (letter)));
				}
			}
		}
	}
	/**
	 * @brief Функция дописывания названия HTTP-заголовка с приведением регистра к канонической форме протокола
	 *
	 * @details Для протоколов семейства HTTP/2 (HTTP/2, HTTP/3 и их модификации) названия заголовков обязаны быть
	 *          в нижнем регистре (RFC 9113 §8.2), для остальных версий применяется «умный» регистр: первая буква
	 *          и буквы после разделителей ('-', '_', пробел) переводятся в верхний регистр, остальные - в нижний.
	 *          Название дописывается напрямую в результирующую строку без создания промежуточных временных строк.
	 *
	 * @param result результирующая строка, в которую дописывается название заголовка
	 * @param name   исходное название заголовка
	 * @param proto  версия протокола HTTP-запроса/ответа
	 */
	void appendCasedName(string & result, string_view name, const http::proto_t proto) noexcept {
		/**
		 * Определяем версию протокола передачи данных
		 */
		switch(proto){
			// Для протоколов HTTP/2, HTTP/3 и их модификаций (Proxy, Websocket) названия заголовков обязаны быть в нижнем регистре
			case http::proto_t::HTTP2:
			case http::proto_t::HTTP3:
			case http::proto_t::PROXY2:
			case http::proto_t::PROXY3:
			case http::proto_t::WEBSOCKET2:
			case http::proto_t::WEBSOCKET3: {
				/**
				 * Дописываем название заголовка, приводя каждый символ к нижнему регистру без создания промежуточной строки
				 */
				for(const auto & letter : name)
					// Дописываем очередной символ названия заголовка в нижнем регистре
					result.push_back(static_cast <char> (toLower(static_cast <uint8_t> (letter))));
			} break;
			// Для остальных версий протокола название заголовка приводится к «умному» регистру
			default: {
				// Символ с которым ведётся работа в данный момент
				char letter = 0;
				// Флаг детекции символа
				bool mode = true;
				/**
				 * Переходим по всем буквам слова и формируем новую строку
				 */
				for(size_t i = 0; i < name.length(); i++){
					// Получаем символ с которым ведётся работа в данный момент
					letter = name[i];
					// Если флаг перевода в верхний регистр активирован
					if(mode){
						// Приводим символ к нижнему регистру
						const uint8_t lower = toLower(static_cast <uint8_t> (letter));
						// Переводим строчную латинскую букву в верхний регистр, прочие символы оставляем без изменений
						result.push_back(static_cast <char> (((lower >= 'a') && (lower <= 'z')) ? (lower - 32) : lower));
					// Переводим остальные символы в нижний регистр
					} else result.push_back(static_cast <char> (toLower(static_cast <uint8_t> (letter))));
					// Если найден спецсимвол, устанавливаем флаг детекции
					mode = ((letter == '-') || (letter == '_') || isSpace(static_cast <uint8_t> (letter)));
				}
			}
		}
	}
	/**
	 * @brief Функция дописывания одной строки HTTP-заголовка с учётом версии протокола
	 *
	 * @details Заголовок дописывается напрямую в результирующую строку без создания промежуточных временных строк,
	 *          что сокращает количество аллокаций при печати всех заголовков.
	 *
	 * @param result результирующая строка, в которую дописывается заголовок
	 * @param header название и значение форматируемого заголовка
	 * @param proto  версия протокола HTTP-запроса/ответа
	 */
	void appendHeader(string & result, const http::Headers::header_t & header, const http::proto_t proto) noexcept {
		// Дописываем название заголовка в канонической для протокола форме
		appendCasedName(result, header.name, proto);
		// Дописываем разделитель названия и значения заголовка
		result.append(": ");
		// Дописываем значение заголовка
		result.append(header.value);
		// Дописываем завершающий перевод строки
		result.append("\r\n");
	}
	/**
	 * @brief Шаблон функции определения версии протокола HTTP на основе списка заголовков
	 *
	 * @tparam Range тип диапазона заголовков для анализа
	 */
	template <typename Range>
	/**
	 * @brief Функция определения версии протокола на основе списка заголовков
	 *
	 * @details Если среди заголовков присутствует хотя бы один псевдозаголовок (название которого
	 *          начинается с двоеточия, например ":method" или ":status") - заголовки соответствуют
	 *          семейству протоколов HTTP/2, в противном случае - протоколу HTTP/1.1
	 *
	 * @param headers набор заголовков для анализа
	 * @return        определённая версия протокола HTTP
	 */
	http::proto_t detectProto(const Range & headers) noexcept {
		// Флаг наличия хотя бы одного заголовка для анализа
		bool hasHeaders = false;
		/**
		 * Проходим по всем переданным заголовкам
		 */
		for(const auto & header : headers){
			// Отмечаем наличие хотя бы одного заголовка
			hasHeaders = true;
			// Если название заголовка является псевдозаголовком HTTP/2 (начинается с двоеточия)
			if(!header.name.empty() && (header.name.front() == ':'))
				// Заголовки соответствуют семейству протоколов HTTP/2
				return http::proto_t::HTTP2;
		}
		// Если заголовки отсутствуют - протокол не определён
		if(!hasHeaders)
			// Возвращаем значение по умолчанию
			return http::proto_t::NONE;
		// Псевдозаголовки не обнаружены - заголовки соответствуют протоколу HTTP/1.1
		return http::proto_t::HTTP1;
	}
	/**
	 * @brief Функция формирования списка псевдозаголовков протокола HTTP/2 на основе объекта провайдера
	 *
	 * @details У протокола HTTP/2 отсутствует стартовая строка, вместо неё передаются отдельные псевдозаголовки,
	 *          названия которых начинаются с двоеточия (":method", ":path", ":status" и т.д.).
	 *
	 * @param provider объект провайдера HTTP-запроса/ответа
	 * @return         список псевдозаголовков протокола HTTP/2
	 */
	vector <http::Headers::header_t> pseudoHeaders(const http::provider_t * provider) noexcept {
		// Результат работы функции - список псевдозаголовков
		vector <http::Headers::header_t> result;
		// Если объект провайдера установлен
		if(provider != nullptr){
			/**
			 * Определяем направление трафика (запрос/ответ), используя явный флаг вместо приведения типа через RTTI
			 */
			switch(static_cast <uint8_t> (provider->traffic)){
				// Если передан запрос клиента
				case static_cast <uint8_t> (http::traffic_t::REQUEST): {
					// Приводим провайдер к типу запроса клиента (безопасно, так как тип подтверждён флагом traffic)
					const auto * request = static_cast <const http::request_t *> (provider);
					// Получаем текстовое название метода запроса
					const string_view methodName = ::method(request->method);
					// Формируем псевдозаголовок метода запроса (:method) - обязателен и следует первым согласно RFC 9113
					if(!methodName.empty()){
						// Создаём псевдозаголовок метода запроса
						http::Headers::header_t header{};
						// Формируем псевдозаголовок метода запроса
						header.from(":method", methodName);
						// Если псевдозаголовок сформирован корректно - добавляем его в результат
						if(!header.name.empty())
							// Добавляем псевдозаголовок метода запроса в результат
							result.push_back(::move(header));
					}
					/**
					 * Для метода CONNECT формируется только псевдозаголовок :authority (RFC 9113 §8.5),
					 * а псевдозаголовки :scheme и :path должны отсутствовать
					 */
					if(request->method == http::method_t::CONNECT){
						// Формируем псевдозаголовок авторитета (:authority) из URI запроса (для CONNECT это цель "host:port")
						if(!request->uri.empty()){
							// Создаём псевдозаголовок авторитета
							http::Headers::header_t header{};
							// Формируем псевдозаголовок авторитета
							header.from(":authority", request->uri);
							// Если псевдозаголовок сформирован корректно - добавляем его в результат
							if(!header.name.empty())
								// Добавляем псевдозаголовок авторитета в результат
								result.push_back(::move(header));
						}
						// Прерываем формирование псевдозаголовков для метода CONNECT
						break;
					}
					// Значения псевдозаголовков схемы, авторитета и пути запроса
					string scheme = "", authority = "", path = request->uri;
					// Ищем разделитель схемы "://" - признак URI в абсолютной форме (scheme://authority/path)
					const size_t schemeEnd = path.find("://");
					// Если URI задан в абсолютной форме - разбираем его на составляющие
					if(schemeEnd != string::npos){
						// Извлекаем схему (часть до "://")
						scheme = path.substr(0, schemeEnd);
						// Определяем начало компонента авторитета (сразу после "://")
						const size_t authorityStart = (schemeEnd + 3);
						// Ищем начало пути - первый символ '/' после компонента авторитета
						const size_t pathStart = path.find('/', authorityStart);
						// Если путь присутствует в URI
						if(pathStart != string::npos){
							// Извлекаем компонент авторитета
							authority = path.substr(authorityStart, (pathStart - authorityStart));
							// Оставляем в пути только его часть (начиная с '/')
							path = path.substr(pathStart);
						// Если путь отсутствует - авторитет занимает остаток строки, а путь принимает значение по умолчанию
						} else {
							// Извлекаем компонент авторитета до конца строки
							authority = path.substr(authorityStart);
							// Устанавливаем путь по умолчанию
							path = "/";
						}
					}
					// Формируем псевдозаголовок схемы (:scheme), только если она определена
					if(!scheme.empty()){
						// Создаём псевдозаголовок схемы
						http::Headers::header_t header{};
						// Формируем псевдозаголовок схемы
						header.from(":scheme", scheme);
						// Если псевдозаголовок сформирован корректно - добавляем его в результат
						if(!header.name.empty())
							// Добавляем псевдозаголовок схемы в результат
							result.push_back(::move(header));
					}
					// Формируем псевдозаголовок авторитета (:authority), только если он определён
					if(!authority.empty()){
						// Создаём псевдозаголовок авторитета
						http::Headers::header_t header{};
						// Формируем псевдозаголовок авторитета
						header.from(":authority", authority);
						// Если псевдозаголовок сформирован корректно - добавляем его в результат
						if(!header.name.empty())
							// Добавляем псевдозаголовок авторитета в результат
							result.push_back(::move(header));
					}
					// Формируем псевдозаголовок пути запроса (:path) - обязателен для всех методов, кроме CONNECT
					if(!path.empty()){
						// Создаём псевдозаголовок пути запроса
						http::Headers::header_t header{};
						// Формируем псевдозаголовок пути запроса
						header.from(":path", path);
						// Если псевдозаголовок сформирован корректно - добавляем его в результат
						if(!header.name.empty())
							// Добавляем псевдозаголовок пути запроса в результат
							result.push_back(::move(header));
					}
				} break;
				// Если передан ответ сервера
				case static_cast <uint8_t> (http::traffic_t::RESPONSE): {
					// Приводим провайдер к типу ответа сервера (безопасно, так как тип подтверждён флагом traffic)
					const auto * response = static_cast <const http::response_t *> (provider);
					// Формируем псевдозаголовок кода ответа сервера
					http::Headers::header_t header{};
					// Формируем псевдозаголовок кода ответа сервера
					header.from(":status", ::to_string(response->code));
					// Если псевдозаголовок сформирован корректно - добавляем его в результат
					if(!header.name.empty())
						// Добавляем псевдозаголовок кода ответа сервера в результат
						result.push_back(::move(header));
				} break;
			}
		}
		// Выводим результат
		return result;
	}
	/**
	 * @brief Функция преобразования списка заголовков в набор заголовков
	 *
	 * @param headers список заголовков для преобразования
	 * @return        набор заголовков
	 */
	http::Headers::fields_t convert(const http::Headers::multimap_t & headers) noexcept {
		// Результирующий список заголовков
		http::Headers::fields_t result;
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Резервируем память под список заголовков во избежание частых перевыделений
			result.reserve(headers.size());
			/**
			 * Копируем все пары ключ-значение в результирующий список заголовков, сохраняя допустимые дубликаты имён
			 */
			for(const auto & item : headers){
				// Создаём новый заголовок на основе пары ключ-значение
				http::Headers::header_t header{};
				// Заполняем название и значение заголовка через фабричный метод
				header.from(item.first, item.second);
				// Если заголовок сформирован корректно (название не пустое) - добавляем его в результирующий список
				if(!header.name.empty())
					// Добавляем заголовок в результирующий список
					result.push_back(::move(header));
			}
		// Если возникает ошибка выделения памяти
		} catch(const exception &) {}
		// Выводим результат
		return result;
	}
	/**
	 * @brief Функция вывода сообщения об ошибке в лог
	 *
	 * @details Инкапсулирует общую для контейнера заголовков и его итераторов логику вывода ошибки:
	 *          при наличии объекта логирования запись выполняется через него, иначе - в поток ошибок
	 *
	 * @param log     объект для работы с логами
	 * @param func    название функции, в которой произошла ошибка
	 * @param message текст сообщения об ошибке
	 * @param flag    флаг важности сообщения
	 */
	void printError(const log_t * log, [[maybe_unused]] const char * func, const char * message, const log_t::flag_t flag = log_t::flag_t::CRITICAL) noexcept {
		// Если объект лога установлен
		if(log != nullptr){
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Записываем ошибку в лог
				log->debug("%s", func, {}, flag, message);
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в лог
				log->print("%s", flag, message);
			#endif
		// Если объект логирования не установлен
		} else {
			// Определяем текстовый префикс важности сообщения
			const char * prefix = ((flag == log_t::flag_t::WARNING) ? "WARNING" : "ERROR");
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Записываем ошибку в поток ошибок
				::fprintf(stderr, "%s! Called function:\n%s\n\nMessage:\n%s\n\n", prefix, func, message);
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Записываем ошибку в поток ошибок
				::fprintf(stderr, "%s! %s\n\n", prefix, message);
			#endif
		}
	}
};

/**
 * @brief Фабричный метод создания HTTP-заголовка
 *
 * @param name  название HTTP-заголовка
 * @param value значение HTTP-заголовка
 * @return      ссылка на текущий объект заголовка
 */
awh::http::Headers::Header & awh::http::Headers::Header::from(string_view name, string_view value) noexcept {
	// Если название заголовка передано (пустое значение допустимо согласно RFC 9110)
	if(!name.empty()){
		// Устанавливаем название заголовка
		this->name = name;
		// Устанавливаем значение заголовка
		this->value = value;
	// Если название заголовка не передано
	} else {
		// Сбрасываем название заголовка
		this->name.clear();
		// Сбрасываем значение заголовка
		this->value.clear();
	}
	// Возвращаем ссылку на текущий объект заголовка
	return (* this);
}
/**
 * @brief Оператор сравнения
 *
 * @param other другой объект для сравнения
 * @return      результат сравнения
 */
bool awh::http::Headers::Header::operator == (const header_t & other) const noexcept {
	/**
	 * Выполняем сравнение названия заголовка без учёта регистра (согласуется с хеш-функцией Header_Hash,
	 * которая вычисляется по приведённым к нижнему регистру символам названия)
	 */
	return ::equals(this->name, other.name);
}

/**
 * @brief Оператор вычисления хеш-кода
 *
 * @param header объект для вычисления хеш-кода
 * @return       хеш-код объекта
 */
size_t awh::http::Headers::Header_Hash::operator()(const header_t & header) const noexcept {
	// Вычисляем хеш-код названия заголовка без учёта регистра
	return ::hashing(header.name);
}

/**
 * @brief Оператор вычисления хеш-кода
 *
 * @param name название заголовка для вычисления хеш-кода
 * @return     хеш-код названия заголовка
 */
size_t awh::http::Headers::Header_Name_Hash::operator()(const string & name) const noexcept {
	// Вычисляем хеш-код названия заголовка без учёта регистра
	return ::hashing(name);
}

/**
 * @brief Оператор сравнения названий заголовков
 *
 * @param first  первое название заголовка
 * @param second второе название заголовка
 * @return       результат сравнения без учёта регистра
 */
bool awh::http::Headers::Header_Name_Equal::operator()(const string & first, const string & second) const noexcept {
	// Выполняем регистронезависимое сравнение названий заголовков
	return ::equals(first, second);
}

/**
 * @brief Метод вывода сообщения об ошибке в лог
 *
 * @param func    название функции, в которой произошла ошибка
 * @param message текст сообщения об ошибке
 * @param flag    флаг важности сообщения
 */
void awh::http::Headers::Iterator::_error(const char * func, const char * message, const log_t::flag_t flag) const noexcept {
	// Выводим сообщение об ошибке в лог через общую функцию логирования
	::printError(this->_log, func, message, flag);
}
/**
 * @brief Оператор преобразования в сырой итератор
 *
 * @return iterator итератор для преобразования
 */
awh::http::Headers::Iterator::operator iterator() noexcept {
	// Выводим текущее значение итератора
	return this->_it;
}
/**
 * @brief Оператор извлечения указателя заголовка
 *
 * @return указатель заголовка
 */
awh::http::Headers::Iterator::pointer awh::http::Headers::Iterator::operator -> () noexcept {
	// Выводим результат
	return &(* this->_it);
}
/**
 * @brief Оператор разыменования заголовка
 *
 * @return значение заголовка
 */
awh::http::Headers::Iterator::reference awh::http::Headers::Iterator::operator * () noexcept {
	// Выводим результат
	return (* this->_it);
}
/**
 * @brief Оператор смещения вперед
 *
 * @return значение текущего итератора
 */
awh::http::Headers::Iterator & awh::http::Headers::Iterator::operator ++ () noexcept {
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		// Выполняем смещение итератора
		++this->_it;
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		// Записываем ошибку в лог
		this->_error(__PRETTY_FUNCTION__, error.what());
	}
	// Выводим результат
	return (* this);
}
/**
 * @brief Оператор сравнения соответствия итератора
 *
 * @param other итератор для сравнения
 * @return      результат сравнения
 */
bool awh::http::Headers::Iterator::operator == (const iterator_t & other) const noexcept {
	// Выводим результат
	return (this->_it == other._it);
}
/**
 * @brief Оператора сравнения несоответствия итератора
 *
 * @param other итератор для сравнения
 * @return      результат сравнения
 */
bool awh::http::Headers::Iterator::operator != (const iterator_t & other) const noexcept {
	// Выводим результат
	return (this->_it != other._it);
}
/**
 * @brief Оператор сравнения соответствия итератора
 *
 * @param other константный итератор для сравнения
 * @return      результат сравнения
 */
bool awh::http::Headers::Iterator::operator == (const const_iterator_t & other) const noexcept {
	// Выводим результат
	return (this->_it == other._it);
}
/**
 * @brief Оператора сравнения несоответствия итератора
 *
 * @param other константный итератор для сравнения
 * @return      результат сравнения
 */
bool awh::http::Headers::Iterator::operator != (const const_iterator_t & other) const noexcept {
	// Выводим результат
	return (this->_it != other._it);
}
/**
 * @brief Конструктор
 *
 * @param it  итератор для установки
 * @param log объект для работы с логами
 */
awh::http::Headers::Iterator::Iterator(iterator it, const log_t * log) noexcept :
 _it(it), _log(log) {}

/**
 * @brief Метод вывода сообщения об ошибке в лог
 *
 * @param func    название функции, в которой произошла ошибка
 * @param message текст сообщения об ошибке
 * @param flag    флаг важности сообщения
 */
void awh::http::Headers::Const_Iterator::_error(const char * func, const char * message, const log_t::flag_t flag) const noexcept {
	// Выводим сообщение об ошибке в лог через общую функцию логирования
	::printError(this->_log, func, message, flag);
}
/**
 * @brief Оператор преобразования в сырой константный итератор
 *
 * @return const_iterator итератор для преобразования
 */
awh::http::Headers::Const_Iterator::operator const_iterator() const noexcept {
	// Выводим текущее значение итератора
	return this->_it;
}
/**
 * @brief Оператор извлечения указателя заголовка
 *
 * @return указатель заголовка
 */
awh::http::Headers::Const_Iterator::pointer awh::http::Headers::Const_Iterator::operator -> () const noexcept {
	// Выводим результат
	return &(* this->_it);
}
/**
 * @brief Оператор разыменования заголовка
 *
 * @return значение заголовка
 */
awh::http::Headers::Const_Iterator::reference awh::http::Headers::Const_Iterator::operator * () const noexcept {
	// Выводим результат
	return (* this->_it);
}
/**
 * @brief Оператор смещения вперед
 *
 * @return значение текущего итератора
 */
awh::http::Headers::Const_Iterator & awh::http::Headers::Const_Iterator::operator ++ () noexcept {
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		// Выполняем смещение итератора
		++this->_it;
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		// Записываем ошибку в лог
		this->_error(__PRETTY_FUNCTION__, error.what());
	}
	// Выводим результат
	return (* this);
}
/**
 * @brief Оператор сравнения соответствия итератора
 *
 * @param other итератор для сравнения
 * @return      результат сравнения
 */
bool awh::http::Headers::Const_Iterator::operator == (const iterator_t & other) const noexcept {
	// Выводим результат
	return (this->_it == other._it);
}
/**
 * @brief Оператора сравнения несоответствия итератора
 *
 * @param other итератор для сравнения
 * @return      результат сравнения
 */
bool awh::http::Headers::Const_Iterator::operator != (const iterator_t & other) const noexcept {
	// Выводим результат
	return (this->_it != other._it);
}
/**
 * @brief Оператор сравнения соответствия итератора
 *
 * @param other итератор для сравнения
 * @return      результат сравнения
 */
bool awh::http::Headers::Const_Iterator::operator == (const const_iterator_t & other) const noexcept {
	// Выводим результат
	return (this->_it == other._it);
}
/**
 * @brief Оператора сравнения несоответствия итератора
 *
 * @param other итератор для сравнения
 * @return      результат сравнения
 */
bool awh::http::Headers::Const_Iterator::operator != (const const_iterator_t & other) const noexcept {
	// Выводим результат
	return (this->_it != other._it);
}
/**
 * @brief Конструктор
 *
 * @param it  итератор для установки
 * @param log объект для работы с логами
 */
awh::http::Headers::Const_Iterator::Const_Iterator(const_iterator it, const log_t * log) noexcept :
 _it(it), _log(log) {}

/**
 * @brief Метод приведения названий всех заголовков к канонической форме текущего протокола
 *
 * @details Для протоколов семейства HTTP/2 названия приводятся к нижнему регистру, для остальных -
 *          к «умному» регистру. Вызывается при изменении протокола, чтобы единая семантика
 *          регистра соблюдалась при любом способе доступа к заголовкам (не только при печати).
 */
void awh::http::Headers::_recase() noexcept {
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		/**
		 * Приводим название каждого заголовка к канонической для текущего протокола форме
		 */
		for(auto & header : this->_headers)
			// Нормализуем регистр названия заголовка по месту (длина не меняется, поэтому счётчик памяти корректировать не требуется)
			::applyCaseName(header.name, this->_proto);
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		// Записываем ошибку в лог
		this->_error(__PRETTY_FUNCTION__, error.what());
	}
}
/**
 * @brief Шаблон добавления нового заголовка
 *
 * @tparam Name    тип названия добавляемого заголовка
 * @tparam Content тип содержимого добавляемого заголовка
 */
template <typename Name, typename Content>
/**
 * @brief Метод добавления нового заголовка
 *
 * @param name    название заголовка
 * @param content содержимое заголовка
 * @return        общее количество заголовков
 */
size_t awh::http::Headers::_emplace(Name && name, Content && content) noexcept {
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		// Формируем новый заголовок
		header_t header{};
		// Если название заголовка передано (пустое значение допустимо согласно RFC 9110; при пустом названии заголовок останется пустым)
		if(!name.empty()){
			// Устанавливаем название заголовка (перемещаем строку при передаче временного объекта, сохраняя её буфер)
			header.name = ::forward <Name> (name);
			// Приводим название заголовка к канонической для текущего протокола форме по месту (без перевыделения памяти)
			::applyCaseName(header.name, this->_proto);
			// Устанавливаем значение заголовка (перемещаем строку при передаче временного объекта)
			header.value = ::forward <Content> (content);
		}
		// Если заголовок сформирован корректно (название не пустое)
		if(!header.name.empty()){
			// Если максимальное количество заголовков ещё не превышено
			if(this->_headers.size() < this->_max.records){
				// Вычисляем объём полезной нагрузки добавляемого заголовка (название и значение)
				const size_t payload = (header.name.size() + header.value.size());
				// Если добавление заголовка не превысит максимальный объём потребляемой памяти
				if((this->_memory + payload) <= this->_max.memory){
					// Добавляем сформированный заголовок в список
					this->_headers.push_back(::move(header));
					// Увеличиваем счётчик потребляемой памяти на объём добавленного заголовка
					this->_memory += payload;
				}
			}
		}
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		// Записываем ошибку в лог
		this->_error(__PRETTY_FUNCTION__, error.what());
	}
	// Возвращаем общее количество заголовков после попытки добавления
	return this->_headers.size();
}
/**
 * Объявляем прототипы для метода добавления нового заголовка
 */
template size_t awh::http::Headers::_emplace <string &&, string &&> (string &&, string &&) noexcept;
template size_t awh::http::Headers::_emplace <string &&, const string &> (string &&, const string &) noexcept;
template size_t awh::http::Headers::_emplace <const string &, string &&> (const string &, string &&) noexcept;
template size_t awh::http::Headers::_emplace <const string &, const string &> (const string &, const string &) noexcept;
/**
 * @brief Метод вывода сообщения об ошибке в лог
 *
 * @param func    название функции, в которой произошла ошибка
 * @param message текст сообщения об ошибке
 * @param flag    флаг важности сообщения
 */
void awh::http::Headers::_error(const char * func, const char * message, const log_t::flag_t flag) const noexcept {
	// Выводим сообщение об ошибке в лог через общую функцию логирования
	::printError(this->_log, func, message, flag);
}
/**
 * @brief Метод очистки всех данных заголовков
 *
 */
void awh::http::Headers::clear() noexcept {
	// Сбрасываем счётчик потребляемой памяти
	this->_memory = 0;
	// Выполняем сброс списка заголовков
	this->_headers.clear();
}
/**
 * @brief Метод полной очистки памяти
 *
 */
void awh::http::Headers::reset() noexcept {
	// Сбрасываем счётчик потребляемой памяти
	this->_memory = 0;
	// Выполняем освобождение памяти индекса
	fields_t().swap(this->_headers);
}
/**
 * @brief Метод проверки на заполненность заголовков
 *
 * @return результат проверки
 */
bool awh::http::Headers::empty() const noexcept {
	// Выводим проверку на пустоту заголовков
	return this->_headers.empty();
}
/**
 * @brief Метод получения протокола HTTP-запроса/ответа
 *
 * @return протокол HTTP-запроса/ответа
 */
awh::http::proto_t awh::http::Headers::proto() const noexcept {
	// Выводим протокол HTTP-запроса/ответа
	return this->_proto;
}
/**
 * @brief Метод установки протокола HTTP-запроса/ответа
 *
 * @param proto протокол HTTP-запроса/ответа
 */
void awh::http::Headers::proto(const proto_t proto) noexcept {
	// Если протокол действительно изменился
	if(this->_proto != proto){
		// Устанавливаем протокол HTTP-запроса/ответа
		this->_proto = proto;
		// Приводим названия всех заголовков к канонической форме нового протокола
		this->_recase();
	}
}
/**
 * @brief Метод получения объекта провайдера HTTP-запроса/ответа
 *
 * @return объект провайдера HTTP-запроса/ответа
 */
const awh::http::provider_t * awh::http::Headers::provider() const noexcept {
	// Выводим объект провайдера HTTP-запроса/ответа
	return this->_provider.get();
}
/**
 * @brief Метод получения объекта провайдера HTTP-запроса/ответа
 *
 * @param provider объект провайдера HTTP-запроса/ответа
 * @return         результат выполнения операции
 */
bool awh::http::Headers::provider(unique_ptr <provider_t> & provider) const noexcept {
	// Результат работы функции
	bool result = false;
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		// Проверяем наличие объекта провайдера HTTP-запроса/ответа
		if(this->_provider == nullptr)
			// Если объект провайдера HTTP-запроса/ответа отсутствует
			return result;
		// Копируем объект провайдера HTTP-запроса/ответа без срезки производной части
		provider = this->_provider->clone();
		// Устанавливаем результат выполнения операции
		result = (provider != nullptr);
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		// Записываем ошибку в лог
		this->_error(__PRETTY_FUNCTION__, error.what());
	}
	// Выводим результат
	return result;
}
/**
 * @brief Метод установки объекта провайдера HTTP-запроса/ответа
 *
 * @param provider объект провайдера HTTP-запроса/ответа
 */
void awh::http::Headers::provider(const provider_t * provider) noexcept {
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		// Если объект провайдера HTTP-запроса/ответа передан
		if(provider != nullptr)
			// Копируем объект провайдера HTTP-запроса/ответа без срезки производной части
			this->_provider = provider->clone();
		// Если объект провайдера HTTP-запроса/ответа не передан
		else
			// Сбрасываем объект провайдера HTTP-запроса/ответа
			this->_provider.reset(nullptr);
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		// Записываем ошибку в лог
		this->_error(__PRETTY_FUNCTION__, error.what());
	}
}
/**
 * @brief Метод установки объекта провайдера HTTP-запроса/ответа
 *
 * @param provider объект провайдера HTTP-запроса/ответа
 */
void awh::http::Headers::provider(unique_ptr <provider_t> && provider) noexcept {
	// Устанавливаем объект провайдера HTTP-запроса/ответа
	this->_provider = ::move(provider);
}
/**
 * @brief Метод получения стартовой строки HTTP-запроса/ответа
 *
 * @return стартовая строка HTTP-запроса/ответа
 */
string awh::http::Headers::startline() const noexcept {
	// Результат работы функции - собранная стартовая строка HTTP-запроса/ответа
	string result = "";
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		// Если объект провайдера HTTP-запроса/ответа установлен
		if(this->_provider != nullptr){
			/**
			 * Определяем направление трафика (запрос/ответ) по явному флагу вместо приведения типа через RTTI (dynamic_cast),
			 * что позволяет избежать накладных расходов на обход таблиц виртуальных методов
			 */
			switch(static_cast <uint8_t> (this->_provider->traffic)){
				// Если установлен запрос клиента
				case static_cast <uint8_t> (traffic_t::REQUEST): {
					// Безопасно приводим провайдер к типу запроса клиента (тип подтверждён флагом traffic)
					const request_t * request = static_cast <const request_t *> (this->_provider.get());
					// Формируем стартовую строку запроса клиента в формате "Метод URI HTTP/Версия"
					result.append(::method(request->method));
					// Добавляем разделитель между методом и URI запроса
					result.append(1, ' ');
					// Добавляем URI запроса
					result.append(request->uri);
					// Добавляем разделитель между URI и версией протокола
					result.append(" HTTP/");
					// Формируем строку версии протокола в формате "HTTP/Версия"
					result.append(::version(request->version));
				} break;
				// Если установлен ответ сервера
				case static_cast <uint8_t> (traffic_t::RESPONSE): {
					// Безопасно приводим провайдер к типу ответа сервера (тип подтверждён флагом traffic)
					const response_t * response = static_cast <const response_t *> (this->_provider.get());
					// Получаем сообщение сервера, установленное пользователем (без копирования)
					string_view message = response->message;
					// Если сообщение сервера не установлено - подставляем стандартное сообщение по коду ответа (для неизвестного кода вернётся пустое представление)
					if(message.empty())
						// Устанавливаем стандартное сообщение, соответствующее коду ответа сервера
						message = statusMessage(response->code);
					// Формируем стартовую строку ответа сервера в формате "HTTP/Версия Код Сообщение"
					result.append("HTTP/");
					// Формируем строку версии протокола в формате "HTTP/Версия"
					result.append(::version(response->version));
					// Добавляем разделитель между версией протокола и кодом ответа
					result.append(1, ' ');
					// Добавляем код ответа сервера
					result.append(::to_string(response->code));
					// Добавляем разделитель между кодом ответа и сообщением сервера
					result.append(1, ' ');
					// Формируем сообщение сервера в формате "HTTP/Версия Код Сообщение"
					result.append(message);
				} break;
			}
		}
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		// Записываем ошибку в лог
		this->_error(__PRETTY_FUNCTION__, error.what());
	}
	// Выводим результат
	return result;
}
/**
 * @brief Метод установки стартовой строки HTTP-запроса/ответа
 *
 * @param startline стартовая строка HTTP-запроса/ответа
 */
void awh::http::Headers::startline(const string_view startline) noexcept {
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		// Если переданная стартовая строка пустая - сбрасываем текущий объект провайдера
		if(startline.empty()){
			// Сбрасываем объект провайдера HTTP-запроса/ответа
			this->_provider.reset(nullptr);
			// Прерываем выполнение метода
			return;
		}
		// Ищем пробел, отделяющий первый токен стартовой строки
		const size_t space1 = startline.find(' ');
		// Если пробел не найден - стартовая строка имеет некорректный формат
		if(space1 == string_view::npos)
			// Прерываем выполнение метода
			return;
		// Извлекаем первый токен стартовой строки (метод запроса либо версия протокола ответа)
		const string_view token1 = startline.substr(0, space1);
		// Ищем начало второго токена, пропуская повторяющиеся пробелы
		const size_t start2 = startline.find_first_not_of(' ', space1);
		// Если второй токен отсутствует - стартовая строка имеет некорректный формат
		if(start2 == string_view::npos)
			// Прерываем выполнение метода
			return;
		// Ищем пробел после второго токена
		const size_t space2 = startline.find(' ', start2);
		// Если первый токен начинается с "HTTP/" - перед нами строка состояния ответа сервера
		if((token1.size() > 5) && (token1.compare(0, 5, "HTTP/") == 0)){
			// Версия протокола по умолчанию
			version_t version = version_t::HTTP1_1;
			/**
			 * Если версия протокола указана в формате "HTTP/Версия"
			 */
			switch(token1.substr(5).front()){
				// Если версия протокола начинается с '1'
				case '1': {
					// Если версия протокола заканчивается на '1' - устанавливаем версию протокола HTTP/1.1, иначе - HTTP/1.0
					if(token1.substr(5).back() == '1')
						// Устанавливаем версию протокола HTTP/1.1
						version = version_t::HTTP1_1;
					// Если версия протокола заканчивается на '0' - устанавливаем версию протокола HTTP/1.0
					else version = version_t::HTTP1_0;
				} break;
				// Если версия протокола начинается с '2' - устанавливаем версию протокола HTTP/2.0
				case '2': version = version_t::HTTP2; break;
				// Если версия протокола начинается с '3' - устанавливаем версию протокола HTTP/3.0
				case '3': version = version_t::HTTP3; break;
				// Если версия протокола начинается с '4' - устанавливаем версию протокола HTTP/4.0
				case '4': version = version_t::HTTP4; break;
				// Если версия протокола начинается с '5' - устанавливаем версию протокола HTTP/5.0
				case '5': version = version_t::HTTP5; break;
			}
			// Сообщение сервера
			string_view message = "";
			// Текстовое представление кода ответа сервера
			string_view codeText = "";
			// Если пробел после кода ответа не найден - сообщение сервера отсутствует
			if(space2 == string_view::npos)
				// Код ответа занимает остаток строки
				codeText = startline.substr(start2);
			// Если пробел после кода ответа найден
			else {
				// Извлекаем код ответа сервера
				codeText = startline.substr(start2, space2 - start2);
				// Ищем начало сообщения сервера, пропуская повторяющиеся пробелы
				const size_t start3 = startline.find_first_not_of(' ', space2);
				// Если сообщение сервера присутствует в строке
				if(start3 != string_view::npos)
					// Извлекаем сообщение сервера
					message = startline.substr(start3);
			}
			// Формируем объект ответа сервера и устанавливаем его в качестве текущего провайдера
			this->_provider = make_unique <response_t> (version, static_cast <uint16_t> (::stoul(string(codeText))), string(message));
		// Если первый токен не является версией протокола - перед нами строка запроса клиента
		} else {
			// Если пробел после URI-адреса не найден - стартовая строка имеет некорректный формат
			if(space2 == string_view::npos)
				// Прерываем выполнение метода
				return;
			// Версия протокола запроса по умолчанию
			version_t version = version_t::HTTP1_1;
			// Извлекаем URI-адрес запроса
			const string_view uri = startline.substr(start2, space2 - start2);
			// Ищем начало версии протокола запроса, пропуская повторяющиеся пробелы
			const size_t start3 = startline.find_first_not_of(' ', space2);
			// Если версия протокола запроса присутствует в строке
			if(start3 != string_view::npos){
				// Извлекаем текстовое представление версии протокола запроса
				const string_view versionText = startline.substr(start3);
				// Если версия протокола запроса указана в формате "HTTP/Версия"
				if((versionText.size() > 5) && (versionText.compare(0, 5, "HTTP/") == 0)){
					/**
					 * Если версия протокола указана в формате "HTTP/Версия"
					 */
					switch(versionText.substr(5).front()){
						// Если версия протокола начинается с '1'
						case '1': {
							// Если версия протокола заканчивается на '1' - устанавливаем версию протокола HTTP/1.1, иначе - HTTP/1.0
							if(versionText.substr(5).back() == '1')
								// Устанавливаем версию протокола HTTP/1.1
								version = version_t::HTTP1_1;
							// Если версия протокола заканчивается на '0' - устанавливаем версию протокола HTTP/1.0, иначе - версия протокола не определена
							else version = version_t::HTTP1_0;
						} break;
						// Если версия протокола начинается с '2' - устанавливаем версию протокола HTTP/2.0
						case '2': version = version_t::HTTP2; break;
						// Если версия протокола начинается с '3' - устанавливаем версию протокола HTTP/3.0
						case '3': version = version_t::HTTP3; break;
						// Если версия протокола начинается с '4' - устанавливаем версию протокола HTTP/4.0
						case '4': version = version_t::HTTP4; break;
						// Если версия протокола начинается с '5' - устанавливаем версию протокола HTTP/5.0
						case '5': version = version_t::HTTP5; break;
					}
				}
			}
			// Формируем объект запроса клиента и устанавливаем его в качестве текущего провайдера
			this->_provider = make_unique <request_t> (version, ::method(token1), string(uri));
		}
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		// Записываем ошибку в лог
		this->_error(__PRETTY_FUNCTION__, error.what());
	}
}
/**
 * @brief Метод удаления заголовка
 *
 * @param name название удаляемого заголовка
 */
void awh::http::Headers::erase(string_view name) noexcept {
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		/**
		 * Выполняем перебор всех заголовков и удаляем совпадающие по названию без учёта регистра
		 */
		for(auto i = this->_headers.begin(); i != this->_headers.end();){
			// Если название заголовка совпадает с указанным
			if(::equals(i->name, name)){
				// Вычисляем объём полезной нагрузки удаляемого заголовка
				const size_t payload = (i->name.size() + i->value.size());
				// Уменьшаем счётчик потребляемой памяти, не допуская переполнения вниз при возможной внешней рассинхронизации
				this->_memory -= ((payload <= this->_memory) ? payload : this->_memory);
				// Удаляем текущий заголовок и переходим к следующему
				i = this->_headers.erase(i);
			// Переходим к следующему заголовку
			} else ++i;
		}
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		// Записываем ошибку в лог
		this->_error(__PRETTY_FUNCTION__, error.what());
	}
}
/**
 * @brief erase Метод удаления заголовка
 *
 * @param it идетартор заголовка для удаления
 * @return   следующий итератор
 */
awh::http::Headers::iterator_t awh::http::Headers::erase(const iterator_t & it) noexcept {
	// Результат работы функции - итератор следующий за удалённым элементом
	iterator_t result = this->end();
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		// Создаём копию входного итератора, поскольку операторы доступа класса Iterator объявлены не const
		Iterator copy = it;
		// Получаем сырой итератор удаляемого элемента
		const Iterator::iterator target = static_cast <Iterator::iterator> (copy);
		// Удаляем элемент только если итератор указывает на существующий заголовок (erase(end()) - неопределённое поведение)
		if(target != this->_headers.end()){
			// Вычисляем объём полезной нагрузки удаляемого заголовка
			const size_t payload = (target->name.size() + target->value.size());
			// Уменьшаем счётчик потребляемой памяти, не допуская переполнения вниз при возможной внешней рассинхронизации
			this->_memory -= ((payload <= this->_memory) ? payload : this->_memory);
			// Удаляем элемент, соответствующий переданному итератору, получая следующий сырой итератор
			const Iterator::iterator next = this->_headers.erase(target);
			// Оборачиваем полученный сырой итератор в класс-обёртку
			result = iterator_t(next, this->_log);
		}
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		// Записываем ошибку в лог
		this->_error(__PRETTY_FUNCTION__, error.what());
	}
	// Выводим результат
	return result;
}
/**
 * @brief Метод проверки существования заголовка
 *
 * @param name название заголовка для проверки
 * @return     результат выполнения проверки
 */
bool awh::http::Headers::has(string_view name) const noexcept {
	/**
	 * Выполняем перебор всех заголовков для проверки существования заголовка с указанным названием без учёта регистра (O(n) в худшем случае)
	 */
	for(auto & header : this->_headers){
		// Если название заголовка совпадает с указанным
		if(::equals(header.name, name))
			// Возвращаем положительный результат проверки
			return true;
	}
	// Выводим отрицательный результат проверки
	return false;
}
/**
 * @brief Метод получения общего количества заголовков
 *
 * @return общее количество заголовков
 */
size_t awh::http::Headers::size() const noexcept {
	// Выводим общее количество установленных заголовков
	return this->_headers.size();
}
/**
 * @brief Количество добавленных заголовков
 *
 * @param name название заголовка количество которых нужно определить
 * @return     количество добавленных заголовков
 */
size_t awh::http::Headers::count(string_view name) const noexcept {
	// Если название заголовка не указано - возвращаем общее количество всех заголовков
	if(name.empty())
		// Выводим общее количество установленных заголовков
		return this->_headers.size();
	// Результат работы функции
	size_t result = 0;
	/**
	 * Выполняем перебор всех заголовков для проверки существования заголовка с указанным названием без учёта регистра (O(n) в худшем случае)
	 */
	for(auto & header : this->_headers){
		// Если название заголовка совпадает с указанным
		if(::equals(header.name, name))
			// Увеличиваем счётчик совпадений
			result++;
	}
	// Выводим результат
	return result;
}
/**
 * @brief Метод извлечения содержимого заголовка
 *
 * @param name название заголовка
 * @return     содержимое заголовка
 */
const string & awh::http::Headers::at(string_view name) const noexcept {
	// Статическая пустая строка, возвращаемая если заголовок с указанным названием не найден
	static const string result = "";
	/**
	 * Выполняем перебор всех заголовков для проверки существования заголовка с указанным названием без учёта регистра (O(n) в худшем случае)
	 */
	for(auto & header : this->_headers){
		// Если название заголовка совпадает с указанным
		if(::equals(header.name, name))
			// Возвращаем значение найденного заголовка
			return header.value;
	}
	// Заголовок не найден - возвращаем пустую строку
	return result;
}
/**
 * @brief Метод извлечения названий заголовков
 *
 * @return список названий заголовков
 */
vector <string> awh::http::Headers::names() const noexcept {
	// Результат работы функции - список уникальных названий заголовков
	vector <string> result;
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		// Резервируем память под список названий заголовков
		result.reserve(this->_headers.size());
		// Набор уже добавленных названий для проверки уникальности без учёта регистра за амортизированное O(1)
		unordered_set <string, header_name_hash_t, header_name_equal_t> seen;
		// Резервируем память под набор проверенных названий
		seen.reserve(this->_headers.size());
		/**
		 * Проходим по всем установленным заголовкам
		 */
		for(const auto & header : this->_headers){
			// Если название заголовка ещё не было добавлено в список - добавляем его в результат
			if(seen.emplace(header.name).second)
				// Добавляем уникальное название заголовка в результат
				result.push_back(header.name);
		}
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		// Записываем ошибку в лог
		this->_error(__PRETTY_FUNCTION__, error.what());
	}
	// Выводим результат
	return result;
}
/**
 * @brief Метод вывода списка значений одинаковых заголовков
 *
 * @param name название заголовка
 * @return     список значений одинаковых заголовков
 */
vector <string> awh::http::Headers::range(string_view name) const noexcept {
	// Результат работы функции - список значений заголовков с указанным названием
	vector <string> result;
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		/**
		 * Выполняем перебор всех заголовков и собираем значения совпадающих по названию без учёта регистра
		 */
		for(const auto & header : this->_headers){
			// Если название заголовка совпадает с указанным
			if(::equals(header.name, name))
				// Добавляем значение заголовка в результирующий список
				result.push_back(header.value);
		}
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		// Записываем ошибку в лог
		this->_error(__PRETTY_FUNCTION__, error.what());
	}
	// Выводим результат
	return result;
}
/**
 * @brief Метод добавления или замены заголовка
 *
 * @note В режиме REPLACE все прежние вхождения заголовка с указанным именем заменяются новым
 *       значением, в режиме APPEND новый заголовок добавляется, сохраняя существующие одноимённые.
 *
 * @param name    название заголовка
 * @param content содержимое заголовка
 * @param mode    режим добавления заголовка (добавить/заменить)
 * @return        общее количество заголовков
 */
size_t awh::http::Headers::emplace(string && name, string && content, const mode_t mode) noexcept {
	// Если выбран режим замены и название заголовка указано - удаляем все его прежние вхождения перед заменой
	if((mode == mode_t::REPLACE) && !name.empty())
		// Удаляем все прежние вхождения заголовка с указанным названием
		this->erase(name);
	// Добавляем новый заголовок и возвращаем общее количество заголовков
	return this->_emplace(::move(name), ::move(content));
}
/**
 * @brief Метод добавления или замены заголовка
 *
 * @note В режиме REPLACE все прежние вхождения заголовка с указанным именем заменяются новым
 *       значением, в режиме APPEND новый заголовок добавляется, сохраняя существующие одноимённые.
 *
 * @param name    название заголовка (C-строка)
 * @param content содержимое заголовка (переносится)
 * @param mode    режим добавления заголовка (добавить/заменить)
 * @return        общее количество заголовков
 */
size_t awh::http::Headers::emplace(const char * name, string && content, const mode_t mode) noexcept {
	// Нормализуем название заголовка, защищаясь от нулевого указателя
	const char * key = ((name != nullptr) ? name : "");
	// Если выбран режим замены и название заголовка указано - удаляем все его прежние вхождения перед заменой
	if((mode == mode_t::REPLACE) && ((* key) != '\0'))
		// Удаляем все прежние вхождения заголовка с указанным названием
		this->erase(key);
	// Добавляем новый заголовок напрямую, избегая повторного удаления (название из C-строки, содержимое переносим)
	return this->_emplace(string(key), ::move(content));
}
/**
 * @brief Метод добавления или замены заголовка
 *
 * @note В режиме REPLACE все прежние вхождения заголовка с указанным именем заменяются новым
 *       значением, в режиме APPEND новый заголовок добавляется, сохраняя существующие одноимённые.
 *
 * @param name    название заголовка (переносится)
 * @param content содержимое заголовка (C-строка)
 * @param mode    режим добавления заголовка (добавить/заменить)
 * @return        общее количество заголовков
 */
size_t awh::http::Headers::emplace(string && name, const char * content, const mode_t mode) noexcept {
	// Если выбран режим замены и название заголовка указано - удаляем все его прежние вхождения перед заменой
	if((mode == mode_t::REPLACE) && !name.empty())
		// Удаляем все прежние вхождения заголовка с указанным названием
		this->erase(name);
	// Добавляем новый заголовок напрямую, избегая повторного удаления (название переносим, содержимое из C-строки с защитой от нулевого указателя)
	return this->_emplace(::move(name), string((content != nullptr) ? content : ""));
}
/**
 * @brief Метод добавления или замены заголовка
 *
 * @note В режиме REPLACE все прежние вхождения заголовка с указанным именем заменяются новым
 *       значением, в режиме APPEND новый заголовок добавляется, сохраняя существующие одноимённые.
 *
 * @param name    название заголовка
 * @param content содержимое заголовка
 * @param mode    режим добавления заголовка (добавить/заменить)
 * @return        общее количество заголовков
 */
size_t awh::http::Headers::emplace(string_view name, string_view content, const mode_t mode) noexcept {
	// Если выбран режим замены и название заголовка указано - удаляем все его прежние вхождения перед заменой
	if((mode == mode_t::REPLACE) && !name.empty())
		// Удаляем все прежние вхождения заголовка с указанным названием
		this->erase(name);
	// Добавляем новый заголовок и возвращаем общее количество заголовков
	return this->_emplace(string(name), string(content));
}
/**
 * @brief Метод добавления или замены заголовка
 *
 * @note В режиме REPLACE все прежние вхождения заголовка с указанным именем заменяются новым
 *       значением, в режиме APPEND новый заголовок добавляется, сохраняя существующие одноимённые.
 *
 * @param name    название заголовка (переносится)
 * @param content содержимое заголовка (копируется)
 * @param mode    режим добавления заголовка (добавить/заменить)
 * @return        общее количество заголовков
 */
size_t awh::http::Headers::emplace(string && name, const string & content, const mode_t mode) noexcept {
	// Если выбран режим замены и название заголовка указано - удаляем все его прежние вхождения перед заменой
	if((mode == mode_t::REPLACE) && !name.empty())
		// Удаляем все прежние вхождения заголовка с указанным названием
		this->erase(name);
	// Добавляем новый заголовок (название переносим, содержимое копируем) и возвращаем общее количество заголовков
	return this->_emplace(::move(name), content);
}
/**
 * @brief Метод добавления или замены заголовка
 *
 * @note В режиме REPLACE все прежние вхождения заголовка с указанным именем заменяются новым
 *       значением, в режиме APPEND новый заголовок добавляется, сохраняя существующие одноимённые.
 *
 * @param name    название заголовка (копируется)
 * @param content содержимое заголовка (переносится)
 * @param mode    режим добавления заголовка (добавить/заменить)
 * @return        общее количество заголовков
 */
size_t awh::http::Headers::emplace(const string & name, string && content, const mode_t mode) noexcept {
	// Если выбран режим замены и название заголовка указано - удаляем все его прежние вхождения перед заменой
	if((mode == mode_t::REPLACE) && !name.empty())
		// Удаляем все прежние вхождения заголовка с указанным названием
		this->erase(name);
	// Добавляем новый заголовок (название копируем, содержимое переносим) и возвращаем общее количество заголовков
	return this->_emplace(name, ::move(content));
}
/**
 * @brief Метод добавления или замены заголовка
 *
 * @note В режиме REPLACE все прежние вхождения заголовка с указанным именем заменяются новым
 *       значением, в режиме APPEND новый заголовок добавляется, сохраняя существующие одноимённые.
 *
 * @param name    название заголовка (C-строка)
 * @param content содержимое заголовка (C-строка)
 * @param mode    режим добавления заголовка (добавить/заменить)
 * @return        общее количество заголовков
 */
size_t awh::http::Headers::emplace(const char * name, const char * content, const mode_t mode) noexcept {
	// Нормализуем название заголовка, защищаясь от нулевого указателя
	const char * key = ((name != nullptr) ? name : "");
	// Если выбран режим замены и название заголовка указано - удаляем все его прежние вхождения перед заменой
	if((mode == mode_t::REPLACE) && ((* key) != '\0'))
		// Удаляем все прежние вхождения заголовка с указанным названием
		this->erase(key);
	// Добавляем новый заголовок напрямую, избегая повторного удаления (C-строки преобразуются во временные объекты с защитой от нулевого указателя)
	return this->_emplace(string(key), string((content != nullptr) ? content : ""));
}
/**
 * @brief Метод добавления или замены заголовка
 *
 * @note В режиме REPLACE все прежние вхождения заголовка с указанным именем заменяются новым
 *       значением, в режиме APPEND новый заголовок добавляется, сохраняя существующие одноимённые.
 *
 * @param name    название заголовка (C-строка)
 * @param content содержимое заголовка (копируется)
 * @param mode    режим добавления заголовка (добавить/заменить)
 * @return        общее количество заголовков
 */
size_t awh::http::Headers::emplace(const char * name, const string & content, const mode_t mode) noexcept {
	// Нормализуем название заголовка, защищаясь от нулевого указателя
	const char * key = ((name != nullptr) ? name : "");
	// Если выбран режим замены и название заголовка указано - удаляем все его прежние вхождения перед заменой
	if((mode == mode_t::REPLACE) && ((* key) != '\0'))
		// Удаляем все прежние вхождения заголовка с указанным названием
		this->erase(key);
	// Добавляем новый заголовок напрямую, избегая повторного удаления (название из C-строки, содержимое копируем)
	return this->_emplace(string(key), content);
}
/**
 * @brief Метод добавления или замены заголовка
 *
 * @note В режиме REPLACE все прежние вхождения заголовка с указанным именем заменяются новым
 *       значением, в режиме APPEND новый заголовок добавляется, сохраняя существующие одноимённые.
 *
 * @param name    название заголовка (копируется)
 * @param content содержимое заголовка (C-строка)
 * @param mode    режим добавления заголовка (добавить/заменить)
 * @return        общее количество заголовков
 */
size_t awh::http::Headers::emplace(const string & name, const char * content, const mode_t mode) noexcept {
	// Если выбран режим замены и название заголовка указано - удаляем все его прежние вхождения перед заменой
	if((mode == mode_t::REPLACE) && !name.empty())
		// Удаляем все прежние вхождения заголовка с указанным названием
		this->erase(name);
	// Добавляем новый заголовок напрямую, избегая повторного удаления (название копируем, содержимое из C-строки с защитой от нулевого указателя)
	return this->_emplace(name, string((content != nullptr) ? content : ""));
}
/**
 * @brief Метод добавления или замены заголовка
 *
 * @note В режиме REPLACE все прежние вхождения заголовка с указанным именем заменяются новым
 *       значением, в режиме APPEND новый заголовок добавляется, сохраняя существующие одноимённые.
 *
 * @param name    название заголовка (копируется)
 * @param content содержимое заголовка (копируется)
 * @param mode    режим добавления заголовка (добавить/заменить)
 * @return        общее количество заголовков
 */
size_t awh::http::Headers::emplace(const string & name, const string & content, const mode_t mode) noexcept {
	// Если выбран режим замены и название заголовка указано - удаляем все его прежние вхождения перед заменой
	if((mode == mode_t::REPLACE) && !name.empty())
		// Удаляем все прежние вхождения заголовка с указанным названием
		this->erase(name);
	// Добавляем новый заголовок (название и содержимое копируем) и возвращаем общее количество заголовков
	return this->_emplace(name, content);
}
/**
 * @brief Метод печати содержимого заголовков в формате HTTP
 *
 * @return заголовки в формате HTTP
 */
string awh::http::Headers::print(const http::proto_t proto) const noexcept {
	// Результат работы функции - все заголовки в текстовом виде
	string result = "";
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		// Резервируем память под результирующую строку с небольшим запасом на служебные символы, стартовую строку/псевдозаголовки и завершающий перевод строки
		result.reserve(this->memory() + ((this->_headers.size() + 4) * 4) + 128);
		/**
		 * Определяем принадлежность протокола к семейству HTTP/2 (бинарное фреймирование, отсутствие стартовой строки как таковой)
		 */
		switch(static_cast <uint8_t> (proto)){
			// Для протоколов HTTP/2, HTTP/3 и их модификаций (proxy/websocket) вместо стартовой строки формируются псевдозаголовки
			case static_cast <uint8_t> (http::proto_t::HTTP2):
			case static_cast <uint8_t> (http::proto_t::HTTP3):
			case static_cast <uint8_t> (http::proto_t::PROXY2):
			case static_cast <uint8_t> (http::proto_t::PROXY3):
			case static_cast <uint8_t> (http::proto_t::WEBSOCKET2):
			case static_cast <uint8_t> (http::proto_t::WEBSOCKET3): {
				// Формируем список псевдозаголовков на основе объекта провайдера (у HTTP/2 нет единой стартовой строки)
				const fields_t pseudo = ::pseudoHeaders(this->_provider.get());
				/**
				 * Проходим по всем сформированным псевдозаголовкам
				 */
				for(const auto & header : pseudo)
					// Дописываем отформатированную строку псевдозаголовка к результату
					::appendHeader(result, header, proto);
			} break;
			// Для протокола HTTP/1.1 и его модификаций (proxy/websocket) формируется классическая стартовая строка
			case static_cast <uint8_t> (http::proto_t::HTTP1):
			case static_cast <uint8_t> (http::proto_t::PROXY1):
			case static_cast <uint8_t> (http::proto_t::WEBSOCKET1): {
				// Формируем стартовую строку HTTP-запроса/ответа на основе объекта провайдера
				const string startline = this->startline();
				// Если стартовая строка сформирована корректно - добавляем её к результату
				if(!startline.empty())
					// Дописываем стартовую строку к результату с завершающим переводом строки
					result.append(startline + "\r\n");
			} break;
		}
		/**
		 * Проходим по всем установленным заголовкам
		 */
		for(const auto & header : this->_headers)
			// Дописываем отформатированную строку заголовка к результату
			::appendHeader(result, header, proto);
		// Добавляем завершающую пустую строку, отделяющую блок заголовков от тела сообщения
		result.append("\r\n");
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		// Записываем ошибку в лог
		this->_error(__PRETTY_FUNCTION__, error.what());
	}
	// Выводим результат
	return result;
}
/**
 * @brief Метод печати содержимого заголовка
 *
 * @param name  печать заголовка в формате HTTP
 * @param proto версия протокола
 * @return      распечатанный заголовок
 */
string awh::http::Headers::print(string_view name, const http::proto_t proto) const noexcept {
	// Результат работы функции - отформатированные строки заголовков с указанным названием
	string result = "";
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		/**
		 * Выполняем перебор всех заголовков и печатаем совпадающие по названию без учёта регистра
		 */
		for(const auto & header : this->_headers){
			// Если название заголовка совпадает с указанным
			if(::equals(header.name, name))
				// Дописываем отформатированную строку заголовка к результату
				::appendHeader(result, header, proto);
		}
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		// Записываем ошибку в лог
		this->_error(__PRETTY_FUNCTION__, error.what());
	}
	// Выводим результат
	return result;
}
/**
 * @brief Метод получения текущего размера потребляемой памяти
 *
 * @return текущий размер потребляемой памяти
 */
size_t awh::http::Headers::memory() const noexcept {
	// Выводим накопленный объём полезной нагрузки (payload) всех заголовков за время O(1)
	return this->_memory;
}
/**
 * @brief Метод получения максимального размера потребления памяти
 *
 * @return максимальный размер потребления памяти
 */
size_t awh::http::Headers::maxMemory() const noexcept {
	// Выводим максимальный размер потребления памяти под полезную нагрузку заголовков
	return this->_max.memory;
}
/**
 * @brief Метод установки максимального размера потребления памяти
 *
 * @param size максимальный размер потребления памяти
 */
void awh::http::Headers::maxMemory(const size_t size) noexcept {
	// Устанавливаем максимальный размер потребления памяти под полезную нагрузку заголовков
	this->_max.memory = size;
}
/**
 * @brief Метод получения максимального количества заголовков
 *
 * @return максимальное количество заголовков
 */
size_t awh::http::Headers::maxRecords() const noexcept {
	// Выводим максимальное количество допустимых заголовков
	return this->_max.records;
}
/**
 * @brief Метод установки максимального количества заголовков
 *
 * @param count максимальное количество заголовков
 */
void awh::http::Headers::maxRecords(const size_t count) noexcept {
	// Устанавливаем максимальное количество допустимых заголовков
	this->_max.records = count;
}
/**
 * @brief Метод обмена заголовками
 *
 * @param headers заголовки для обмена
 */
void awh::http::Headers::swap(Headers & headers) noexcept {
	// Обмениваем местами наборы заголовков
	this->_headers.swap(headers._headers);
	// Обмениваем местами объекты провайдера HTTP-запроса/ответа
	this->_provider.swap(headers._provider);
	// Обмениваем местами протокол HTTP-запроса/ответа
	const proto_t proto = this->_proto;
	// Устанавливаем протокол HTTP-запроса/ответа из переданного контейнера
	this->_proto = headers._proto;
	// Устанавливаем протокол HTTP-запроса/ответа в переданный контейнер
	headers._proto = proto;
	// Обмениваем местами ограничения по памяти и количеству заголовков
	const max_t max = this->_max;
	// Устанавливаем ограничения по памяти и количеству заголовков из переданного контейнера
	this->_max = headers._max;
	// Устанавливаем ограничения по памяти и количеству заголовков в переданный контейнер
	headers._max = max;
	// Обмениваем местами счётчики потребляемой памяти
	const size_t memory = this->_memory;
	// Устанавливаем счётчик потребляемой памяти из переданного контейнера
	this->_memory = headers._memory;
	// Устанавливаем счётчик потребляемой памяти в переданный контейнер
	headers._memory = memory;
}
/**
 * @brief Метод слияния заголовков
 *
 * @param headers заголовки для слияния
 */
void awh::http::Headers::merge(const Headers & headers) noexcept {
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		/**
		 * Проходим по всем заголовкам переданного контейнера и добавляем их в текущий набор с сохранением дубликатов
		 */
		for(const auto & header : headers._headers)
			// Добавляем заголовок в текущий набор с сохранением дубликатов
			this->_emplace(header.name, header.value);
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		// Записываем ошибку в лог
		this->_error(__PRETTY_FUNCTION__, error.what());
	}
}
/**
 * @brief Метод получения конечного итератора
 *
 * @return конечный итератор
 */
awh::http::Headers::iterator_t awh::http::Headers::end() noexcept {
	// Возвращаем обёртку конечного сырого итератора набора заголовков
	return iterator_t(this->_headers.end(), this->_log);
}
/**
 * @brief Метод получения конечного константного итератора
 *
 * @return конечный константный итератор
 */
awh::http::Headers::const_iterator_t awh::http::Headers::end() const noexcept {
	// Возвращаем обёртку конечного сырого константного итератора набора заголовков
	return const_iterator_t(this->_headers.cend(), this->_log);
}
/**
 * @brief Метод получения конечного константного итератора
 *
 * @return конечный константный итератор
 */
awh::http::Headers::const_iterator_t awh::http::Headers::cend() const noexcept {
	// Возвращаем обёртку конечного сырого константного итератора набора заголовков
	return const_iterator_t(this->_headers.cend(), this->_log);
}
/**
 * @brief Метод получение начального итератора
 *
 * @return начальный итератор
 */
awh::http::Headers::iterator_t awh::http::Headers::begin() noexcept {
	// Возвращаем обёртку начального сырого итератора набора заголовков
	return iterator_t(this->_headers.begin(), this->_log);
}
/**
 * @brief Метод получения начального константного итератора
 *
 * @return начальный константный итератор
 */
awh::http::Headers::const_iterator_t awh::http::Headers::begin() const noexcept {
	// Возвращаем обёртку начального сырого константного итератора набора заголовков
	return const_iterator_t(this->_headers.cbegin(), this->_log);
}
/**
 * @brief Метод получения начального константного итератора
 *
 * @return начальный константный итератор
 */
awh::http::Headers::const_iterator_t awh::http::Headers::cbegin() const noexcept {
	// Возвращаем обёртку начального сырого константного итератора набора заголовков
	return const_iterator_t(this->_headers.cbegin(), this->_log);
}
/**
 * @brief Метод поиска указанного заголовка
 *
 * @param name название заголовка для поиска
 * @return     итератор указанного заголовка
 */
awh::http::Headers::iterator_t awh::http::Headers::find(string_view name) noexcept {
	// Выполняем поиск заголовка линейным перебором без учёта регистра
	return iterator_t(::findByName(this->_headers.begin(), this->_headers.end(), name), this->_log);
}
/**
 * @brief Метод поиска указанного заголовка
 *
 * @param name название заголовка для поиска
 * @return     константный итератор указанного заголовка
 */
awh::http::Headers::const_iterator_t awh::http::Headers::find(string_view name) const noexcept {
	// Выполняем поиск заголовка линейным перебором без учёта регистра
	return const_iterator_t(::findByName(this->_headers.cbegin(), this->_headers.cend(), name), this->_log);
}
/**
 * @brief Оператор получения количество заголовков
 *
 * @return количество заголовков
 */
awh::http::Headers::operator size_t() const noexcept {
	// Возвращаем общее количество установленных заголовков
	return this->_headers.size();
}
/**
 * @brief Оператор печати содержимого заголовков в формате HTTP
 *
 * @return заголовки в формате HTTP
 */
awh::http::Headers::operator string() const noexcept {
	// Возвращаем текстовое представление всех заголовков с учётом текущего протокола
	return this->print(this->_proto);
}
/**
 * @brief Оператор получения протокола HTTP-запроса/ответа
 *
 * @return протокол HTTP-запроса/ответа
 */
awh::http::Headers::operator proto_t() const noexcept {
	// Возвращаем текущий протокол HTTP-запроса/ответа
	return this->_proto;
}
/**
 * @brief Оператор получения объекта провайдера HTTP-запроса/ответа
 *
 * @return объект провайдера HTTP-запроса/ответа
 */
awh::http::Headers::operator const provider_t * () const noexcept {
	// Возвращаем указатель на текущий объект провайдера HTTP-запроса/ответа
	return this->_provider.get();
}
/**
 * @brief Оператор получения объекта провайдера HTTP-запроса/ответа
 *
 * @return объект провайдера HTTP-запроса/ответа
 */
awh::http::Headers::operator unique_ptr <provider_t> () const noexcept {
	// Результат работы функции - копия объекта провайдера HTTP-запроса/ответа
	unique_ptr <provider_t> result = nullptr;
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		// Если объект провайдера установлен
		if(this->_provider != nullptr)
			// Создаём копию объекта провайдера без срезки производной части
			result = this->_provider->clone();
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		// Записываем ошибку в лог
		this->_error(__PRETTY_FUNCTION__, error.what());
	}
	// Выводим результат
	return result;
}
/**
 * @brief Оператор получения списка заголовков в том виде как они есть
 *
 * @return список всех добавленных заголовков
 */
awh::http::Headers::operator fields_t() const noexcept {
	// Результат работы функции - список всех добавленных заголовков
	fields_t result;
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		// Определяем принадлежность текущего протокола к семейству HTTP/2 (без стартовой строки, требующей псевдозаголовков)
		switch(this->_proto){
			// Для протоколов HTTP/2, HTTP/3 и их модификаций (proxy/websocket) вместо стартовой строки формируются псевдозаголовки
			case proto_t::HTTP2:
			case proto_t::HTTP3:
			case proto_t::PROXY2:
			case proto_t::PROXY3:
			case proto_t::WEBSOCKET2:
			case proto_t::WEBSOCKET3: {
				// Формируем список псевдозаголовков на основе объекта провайдера
				fields_t pseudo = ::pseudoHeaders(this->_provider.get());
				// Резервируем память с учётом псевдозаголовков
				result.reserve(pseudo.size() + this->_headers.size());
				/**
				 * Перемещаем сформированные псевдозаголовки в результат
				 */
				for(auto & header : pseudo)
					// Добавляем псевдозаголовки в результирующий список
					result.push_back(::move(header));
			} break;
			// Для остальных протоколов резервируем память только под обычные заголовки
			default:
				// Выделяем память под список заголовков с учётом их количества
				result.reserve(this->_headers.size());
		}
		// Копируем все заголовки из набора в список
		result.insert(result.end(), this->_headers.begin(), this->_headers.end());
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		// Записываем ошибку в лог
		this->_error(__PRETTY_FUNCTION__, error.what());
	}
	// Выводим результат
	return result;
}
/**
 * @brief Оператор получения списка заголовков
 *
 * @return список всех добавленных заголовков
 */
awh::http::Headers::operator entries_t() const noexcept {
	// Результат работы функции - набор заголовков
	entries_t result;
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		// Определяем принадлежность текущего протокола к семейству HTTP/2 (без стартовой строки, требующей псевдозаголовков)
		switch(this->_proto){
			// Для протоколов HTTP/2, HTTP/3 и их модификаций (proxy/websocket) вместо стартовой строки формируются псевдозаголовки
			case proto_t::HTTP2:
			case proto_t::HTTP3:
			case proto_t::PROXY2:
			case proto_t::PROXY3:
			case proto_t::WEBSOCKET2:
			case proto_t::WEBSOCKET3: {
				// Формируем список псевдозаголовков на основе объекта провайдера
				fields_t pseudo = ::pseudoHeaders(this->_provider.get());
				// Резервируем память с учётом псевдозаголовков
				result.reserve(pseudo.size() + this->_headers.size());
				/**
				 * Добавляем псевдозаголовки в результирующий набор
				 */
				for(auto & header : pseudo)
					// Добавляем псевдозаголовки в результирующий набор
					result.emplace(::move(header));
			} break;
			// Для остальных протоколов резервируем память только под обычные заголовки
			default:
				// Выделяем память под список заголовков с учётом их количества
				result.reserve(this->_headers.size());
		}
		/**
		 * Копируем все заголовки из списка в набор
		 */
		for(const auto & header : this->_headers)
			// Добавляем заголовки в результирующий набор
			result.emplace(header);
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		// Записываем ошибку в лог
		this->_error(__PRETTY_FUNCTION__, error.what());
	}
	// Выводим результат
	return result;
}
/**
 * @brief Оператор получения списка заголовков
 *
 * @return список всех добавленных заголовков
 */
awh::http::Headers::operator map_t() const noexcept {
	// Результат работы функции - карта заголовков (при дублирующихся именах сохраняется первое встретившееся значение)
	map_t result;
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		// Резервируем память под карту заголовков
		result.reserve(this->_headers.size());
		/**
		 * Проходим по всем установленным заголовкам
		 */
		for(const auto & header : this->_headers)
			// Добавляем пару ключ-значение в карту, если такой ключ ещё не добавлен
			result.emplace(header.name, header.value);
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		// Записываем ошибку в лог
		this->_error(__PRETTY_FUNCTION__, error.what());
	}
	// Выводим результат
	return result;
}
/**
 * @brief Оператор получения списка заголовков
 *
 * @return список всех добавленных заголовков
 */
awh::http::Headers::operator multimap_t() const noexcept {
	// Результат работы функции - множественная карта заголовков
	multimap_t result;
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		// Резервируем память под множественную карту заголовков
		result.reserve(this->_headers.size());
		/**
		 * Проходим по всем установленным заголовкам
		 */
		for(const auto & header : this->_headers)
			// Добавляем пару ключ-значение в множественную карту, сохраняя дубликаты имён
			result.emplace(header.name, header.value);
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		// Записываем ошибку в лог
		this->_error(__PRETTY_FUNCTION__, error.what());
	}
	// Выводим результат
	return result;
}
/**
 * @brief Оператор извлечения содержимого заголовка
 *
 * @details Намеренно только для чтения: запись через operator[] не поддерживается,
 *          так как контейнер допускает несколько заголовков с одним именем
 *          и однозначная семантика записи невозможна.
 *          Для добавления/замены используйте emplace().
 *
 * @param name название заголовка для извлечения
 * @return     содержимое заголовка
 */
const string & awh::http::Headers::operator[](string_view name) const noexcept {
	// Возвращаем значение заголовка через метод at()
	return this->at(name);
}
/**
 * @brief Оператор слияния заголовков
 *
 * @param headers заголовки для слияния
 * @return        текущий контейнер заголовков
 */
awh::http::Headers & awh::http::Headers::operator += (const Headers & headers) noexcept {
	// Выполняем слияние заголовков текущего контейнера с переданным
	this->merge(headers);
	// Возвращаем текущий контейнер заголовков
	return (* this);
}
/**
 * @brief Оператор сравнения двух заголовков
 *
 * @param headers заголовки для сравнения
 * @return        результат сравнения
 */
bool awh::http::Headers::operator == (const Headers & headers) const noexcept {
	// Если количество заголовков в списках не совпадает - контейнеры точно не равны
	if(this->_headers.size() != headers._headers.size())
		// Возвращаем результат сравнения
		return false;
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		// Тип карты частот значений заголовка (значение -> кратность), значения чувствительны к регистру
		using values_t = unordered_map <string, size_t>;
		// Тип группировки значений по названию заголовка без учёта регистра
		using groups_t = unordered_map <string, values_t, header_name_hash_t, header_name_equal_t>;
		// Группируем значения по названию заголовка для текущего контейнера
		groups_t self;
		// Резервируем память под группировку значений текущего контейнера
		self.reserve(this->_headers.size());
		/**
		 * Подсчитываем кратность значений по каждому названию заголовка текущего контейнера
		 */
		for(const auto & header : this->_headers)
			// Увеличиваем счётчик кратности значения для соответствующего названия заголовка
			++self[header.name][header.value];
		// Группируем значения по названию заголовка для сравниваемого контейнера
		groups_t other;
		// Резервируем память под группировку значений сравниваемого контейнера
		other.reserve(headers._headers.size());
		/**
		 * Подсчитываем кратность значений по каждому названию заголовка сравниваемого контейнера
		 */
		for(const auto & header : headers._headers)
			// Увеличиваем счётчик кратности значения для соответствующего названия заголовка
			++other[header.name][header.value];
		// Контейнеры равны, если совпадают группировки значений (сравнение unordered_map не зависит от порядка)
		return (self == other);
	/**
	 * Если возникает ошибка - считаем контейнеры не равными
	 */
	} catch(const exception &) {
		// Возвращаем результат сравнения
		return false;
	}
}
/**
 * @brief Оператор несравнения двух заголовков
 *
 * @param headers заголовки для сравнения
 * @return        результат сравнения
 */
bool awh::http::Headers::operator != (const Headers & headers) const noexcept {
	// Инвертируем результат сравнения на равенство
	return !(* this == headers);
}
/**
 * @brief Оператор перемещения
 *
 * @param headers заголовки для перемещения
 * @return        текущий контейнер заголовков
 */
awh::http::Headers & awh::http::Headers::operator = (Headers && headers) noexcept {
	// Если передан не текущий объект
	if(this != &headers){
		// Перемещаем набор заголовков
		this->_headers = ::move(headers._headers);
		// Перемещаем объект провайдера HTTP-запроса/ответа
		this->_provider = ::move(headers._provider);
		// Копируем объект фреймворка
		this->_fmk = headers._fmk;
		// Копируем объект логирования
		this->_log = headers._log;
		// Копируем ограничения по памяти и количеству заголовков
		this->_max = headers._max;
		// Копируем протокол HTTP-запроса/ответа
		this->_proto = headers._proto;
		// Перемещаем счётчик потребляемой памяти
		this->_memory = headers._memory;
		// Сбрасываем протокол перемещённого объекта на значение по умолчанию
		headers._proto = proto_t::NONE;
		// Сбрасываем счётчик потребляемой памяти перемещённого объекта
		headers._memory = 0;
	}
	// Возвращаем текущий контейнер заголовков
	return (* this);
}
/**
 * @brief Оператор копирования
 *
 * @param headers заголовки для копирования
 * @return        текущий контейнер заголовков
 */
awh::http::Headers & awh::http::Headers::operator = (const Headers & headers) noexcept {
	// Если передан не текущий объект
	if(this != &headers){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Копируем объект фреймворка
			this->_fmk = headers._fmk;
			// Копируем объект логирования
			this->_log = headers._log;
			// Копируем ограничения по памяти и количеству заголовков
			this->_max = headers._max;
			// Копируем протокол HTTP-запроса/ответа
			this->_proto = headers._proto;
			// Копируем счётчик потребляемой памяти
			this->_memory = headers._memory;
			// Копируем набор заголовков
			this->_headers = headers._headers;
			// Если объект провайдера установлен
			if(headers._provider != nullptr)
				// Создаём копию объекта провайдера без срезки производной части
				this->_provider = headers._provider->clone();
			// Если объект провайдера не установлен
			else
				// Сбрасываем объект провайдера
				this->_provider.reset(nullptr);
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			// Записываем ошибку в лог
			this->_error(__PRETTY_FUNCTION__, error.what());
		}
	}
	// Возвращаем текущий контейнер заголовков
	return (* this);
}
/**
 * @brief Оператор установки протокола HTTP-запроса/ответа
 *
 * @param proto протокол HTTP-запроса/ответа
 * @return      текущий контейнер заголовков
 */
awh::http::Headers & awh::http::Headers::operator = (const proto_t proto) noexcept {
	// Устанавливаем протокол HTTP-запроса/ответа
	this->_proto = proto;
	// Возвращаем текущий контейнер заголовков
	return (* this);
}
/**
 * @brief Оператор установки объекта провайдера HTTP-запроса/ответа
 *
 * @param provider объект провайдера HTTP-запроса/ответа
 * @return         текущий контейнер заголовков
 */
awh::http::Headers & awh::http::Headers::operator = (const provider_t * provider) noexcept {
	// Устанавливаем объект провайдера HTTP-запроса/ответа через соответствующий метод
	this->provider(provider);
	// Возвращаем текущий контейнер заголовков
	return (* this);
}
/**
 * @brief Оператор установки объекта провайдера HTTP-запроса/ответа
 *
 * @param provider объект провайдера HTTP-запроса/ответа
 * @return         текущий контейнер заголовков
 */
awh::http::Headers & awh::http::Headers::operator = (unique_ptr <provider_t> && provider) noexcept {
	// Перемещаем объект провайдера HTTP-запроса/ответа через соответствующий метод
	this->provider(::move(provider));
	// Возвращаем текущий контейнер заголовков
	return (* this);
}
/**
 * @brief Оператор копирования
 *
 * @param headers заголовки для копирования
 * @return        текущий контейнер заголовков
 */
awh::http::Headers & awh::http::Headers::operator = (const fields_t & headers) noexcept {
	// Очищаем текущий набор заголовков перед копированием
	this->clear();
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		// Флаг наличия хотя бы одного псевдозаголовка HTTP/2 среди переданных заголовков
		bool http2 = false;
		/**
		 * Добавляем все переданные заголовки, сохраняя допустимые дубликаты имён
		 */
		for(const auto & header : headers){
			// Добавляем заголовок в текущий набор с сохранением дубликатов
			this->_emplace(header.name, header.value);
			// Если название заголовка является псевдозаголовком HTTP/2 (начинается с двоеточия)
			if(!http2 && !header.name.empty() && (header.name.front() == ':'))
				// Устанавливаем флаг наличия псевдозаголовка HTTP/2
				http2 = true;
		}
		// Автоматически определяем протокол на основе состава переданных заголовков
		this->_proto = (headers.empty() ? proto_t::NONE : (http2 ? proto_t::HTTP2 : proto_t::HTTP1));
		// Приводим названия всех заголовков к канонической форме определённого протокола
		this->_recase();
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		// Записываем ошибку в лог
		this->_error(__PRETTY_FUNCTION__, error.what());
	}
	// Возвращаем текущий контейнер заголовков
	return (* this);
}
/**
 * @brief Оператор копирования
 *
 * @param headers заголовки для копирования
 * @return        текущий контейнер заголовков
 */
awh::http::Headers & awh::http::Headers::operator = (const entries_t & headers) noexcept {
	// Очищаем текущий набор заголовков перед копированием
	this->clear();
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		/**
		 * Добавляем все переданные заголовки через контролируемую вставку (с учётом ограничений), сохраняя допустимые дубликаты имён
		 */
		for(const auto & header : headers)
			// Добавляем заголовок в текущий набор с сохранением дубликатов
			this->_emplace(header.name, header.value);
		// Автоматически определяем протокол на основе состава переданных заголовков
		this->_proto = ::detectProto(headers);
		// Приводим названия всех заголовков к канонической форме определённого протокола
		this->_recase();
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		// Записываем ошибку в лог
		this->_error(__PRETTY_FUNCTION__, error.what());
	}
	// Возвращаем текущий контейнер заголовков
	return (* this);
}
/**
 * @brief Оператор копирования
 *
 * @param headers заголовки для копирования
 * @return        текущий контейнер заголовков
 */
awh::http::Headers & awh::http::Headers::operator = (const multimap_t & headers) noexcept {
	// Очищаем текущий набор заголовок перед копированием
	this->clear();
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		// Флаг наличия хотя бы одного псевдозаголовка HTTP/2 среди переданных заголовков
		bool http2 = false;
		/**
		 * Добавляем все переданные пары ключ-значение как заголовки, сохраняя допустимые дубликаты имён
		 */
		for(const auto & item : headers){
			// Добавляем заголовок в текущий набор с сохранением дубликатов
			this->_emplace(item.first, item.second);
			// Если название заголовка является псевдозаголовком HTTP/2 (начинается с двоеточия)
			if(!http2 && !item.first.empty() && (item.first.front() == ':'))
				// Устанавливаем флаг наличия псевдозаголовка HTTP/2
				http2 = true;
		}
		// Автоматически определяем протокол на основе состава переданных заголовков
		this->_proto = (headers.empty() ? proto_t::NONE : (http2 ? proto_t::HTTP2 : proto_t::HTTP1));
		// Приводим названия всех заголовков к канонической форме определённого протокола
		this->_recase();
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		// Записываем ошибку в лог
		this->_error(__PRETTY_FUNCTION__, error.what());
	}
	// Возвращаем текущий контейнер заголовков
	return (* this);
}
/**
 * @brief Оператор копирования
 *
 * @param headers заголовки для копирования
 * @return        текущий контейнер заголовков
 */
awh::http::Headers & awh::http::Headers::operator = (initializer_list <header_t> headers) noexcept {
	// Очищаем текущий набор заголовков перед копированием
	this->clear();
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		// Флаг наличия хотя бы одного псевдозаголовка HTTP/2 среди переданных заголовков
		bool http2 = false;
		/**
		 * Добавляем все переданные заголовки, сохраняя допустимые дубликаты имён
		 */
		for(const auto & header : headers){
			// Добавляем заголовок в текущий набор с сохранением дубликатов
			this->_emplace(header.name, header.value);
			// Если название заголовка является псевдозаголовком HTTP/2 (начинается с двоеточия)
			if(!http2 && !header.name.empty() && (header.name.front() == ':'))
				// Устанавливаем флаг наличия псевдозаголовка HTTP/2
				http2 = true;
		}
		// Автоматически определяем протокол на основе состава переданных заголовков
		this->_proto = ((headers.size() == 0) ? proto_t::NONE : (http2 ? proto_t::HTTP2 : proto_t::HTTP1));
		// Приводим названия всех заголовков к канонической форме определённого протокола
		this->_recase();
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		// Записываем ошибку в лог
		this->_error(__PRETTY_FUNCTION__, error.what());
	}
	// Возвращаем текущий контейнер заголовков
	return (* this);
}
/**
 * @brief Конструктор перемещения
 *
 * @param headers заголовки для перемещения
 */
awh::http::Headers::Headers(Headers && headers) noexcept :
 _max(headers._max), _headers(::move(headers._headers)),
 _memory(headers._memory), _proto(headers._proto),
 _provider(::move(headers._provider)), _fmk(headers._fmk), _log(headers._log) {
	// Сбрасываем счётчик потребляемой памяти перемещённого объекта
	headers._memory = 0;
	// Сбрасываем протокол перемещённого объекта на значение по умолчанию
	headers._proto = proto_t::NONE;
}
/**
 * @brief Конструктор копирования
 *
 * @param headers заголовки для копирования
 */
awh::http::Headers::Headers(const Headers & headers) noexcept :
 _max(headers._max), _headers(headers._headers),
 _memory(headers._memory), _proto(headers._proto),
 _provider((headers._provider != nullptr) ? headers._provider->clone() : nullptr),
 _fmk(headers._fmk), _log(headers._log) {}
/**
 * @brief Конструктор
 *
 * @param proto протокол HTTP-запроса/ответа
 */
awh::http::Headers::Headers(const proto_t proto) noexcept :
 Headers(proto, static_cast <const provider_t *> (nullptr), fields_t {}, nullptr, nullptr) {}
/**
 * @brief Конструктор
 *
 * @param provider объект провайдера HTTP-запроса/ответа
 */
awh::http::Headers::Headers(const provider_t * provider) noexcept :
 Headers(proto_t::NONE, provider, fields_t {}, nullptr, nullptr) {}
/**
 * @brief Конструктор
 *
 * @param provider объект провайдера HTTP-запроса/ответа
 */
awh::http::Headers::Headers(unique_ptr <provider_t> && provider) noexcept :
 Headers(proto_t::NONE, ::move(provider), fields_t {}, nullptr, nullptr) {}
/**
 * @brief Конструктор
 *
 * @param headers список заголовков инициализации
 */
awh::http::Headers::Headers(const fields_t & headers) noexcept :
 Headers(proto_t::NONE, static_cast <const provider_t *> (nullptr), headers, nullptr, nullptr) {}
/**
 * @brief Конструктор
 *
 * @param headers список заголовков инициализации
 */
awh::http::Headers::Headers(const entries_t & headers) noexcept :
 Headers(proto_t::NONE, static_cast <const provider_t *> (nullptr), headers, nullptr, nullptr) {}
/**
 * @brief Конструктор
 *
 * @param headers список заголовков инициализации
 */
awh::http::Headers::Headers(const multimap_t & headers) noexcept :
 Headers(proto_t::NONE, static_cast <const provider_t *> (nullptr), ::convert(headers), nullptr, nullptr) {}
/**
 * @brief Конструктор
 *
 * @param headers список заголовков инициализации
 */
awh::http::Headers::Headers(initializer_list <header_t> headers) noexcept :
 Headers(proto_t::NONE, static_cast <const provider_t *> (nullptr), fields_t (headers), nullptr, nullptr) {}
/**
 * @brief Конструктор
 *
 * @param proto   протокол HTTP-запроса/ответа
 * @param headers список заголовков инициализации
 */
awh::http::Headers::Headers(const proto_t proto, const fields_t & headers) noexcept :
 Headers(proto, static_cast <const provider_t *> (nullptr), headers, nullptr, nullptr) {}
/**
 * @brief Конструктор
 *
 * @param proto   протокол HTTP-запроса/ответа
 * @param headers список заголовков инициализации
 */
awh::http::Headers::Headers(const proto_t proto, const entries_t & headers) noexcept :
 Headers(proto, static_cast <const provider_t *> (nullptr), headers, nullptr, nullptr) {}
/**
 * @brief Конструктор
 *
 * @param proto   протокол HTTP-запроса/ответа
 * @param headers список заголовков инициализации
 */
awh::http::Headers::Headers(const proto_t proto, const multimap_t & headers) noexcept :
 Headers(proto, static_cast <const provider_t *> (nullptr), ::convert(headers), nullptr, nullptr) {}
/**
 * @brief Конструктор
 *
 * @param proto   протокол HTTP-запроса/ответа
 * @param headers список заголовков инициализации
 */
awh::http::Headers::Headers(const proto_t proto, initializer_list <header_t> headers) noexcept :
 Headers(proto, static_cast <const provider_t *> (nullptr), fields_t (headers), nullptr, nullptr) {}
/**
 * @brief Конструктор
 *
 * @param provider объект провайдера HTTP-запроса/ответа
 * @param headers  список заголовков инициализации
 */
awh::http::Headers::Headers(const provider_t * provider, const fields_t & headers) noexcept :
 Headers(proto_t::NONE, provider, headers, nullptr, nullptr) {}
/**
 * @brief Конструктор
 *
 * @param provider объект провайдера HTTP-запроса/ответа
 * @param headers  список заголовков инициализации
 */
awh::http::Headers::Headers(const provider_t * provider, const entries_t & headers) noexcept :
 Headers(proto_t::NONE, provider, headers, nullptr, nullptr) {}
/**
 * @brief Конструктор
 *
 * @param provider объект провайдера HTTP-запроса/ответа
 * @param headers  список заголовков инициализации
 */
awh::http::Headers::Headers(const provider_t * provider, const multimap_t & headers) noexcept :
 Headers(proto_t::NONE, provider, ::convert(headers), nullptr, nullptr) {}
/**
 * @brief Конструктор
 *
 * @param provider объект провайдера HTTP-запроса/ответа
 * @param headers  список заголовков инициализации
 */
awh::http::Headers::Headers(const provider_t * provider, initializer_list <header_t> headers) noexcept :
 Headers(proto_t::NONE, provider, fields_t (headers), nullptr, nullptr) {}
/**
 * @brief Конструктор
 *
 * @param provider объект провайдера HTTP-запроса/ответа
 * @param headers  список заголовков инициализации
 */
awh::http::Headers::Headers(unique_ptr <provider_t> && provider, const fields_t & headers) noexcept :
 Headers(proto_t::NONE, ::move(provider), headers, nullptr, nullptr) {}
/**
 * @brief Конструктор
 *
 * @param provider объект провайдера HTTP-запроса/ответа
 * @param headers  список заголовков инициализации
 */
awh::http::Headers::Headers(unique_ptr <provider_t> && provider, const entries_t & headers) noexcept :
 Headers(proto_t::NONE, ::move(provider), headers, nullptr, nullptr) {}
/**
 * @brief Конструктор
 *
 * @param provider объект провайдера HTTP-запроса/ответа
 * @param headers  список заголовков инициализации
 */
awh::http::Headers::Headers(unique_ptr <provider_t> && provider, const multimap_t & headers) noexcept :
 Headers(proto_t::NONE, ::move(provider), ::convert(headers), nullptr, nullptr) {}
/**
 * @brief Конструктор
 *
 * @param provider объект провайдера HTTP-запроса/ответа
 * @param headers  список заголовков инициализации
 */
awh::http::Headers::Headers(unique_ptr <provider_t> && provider, initializer_list <header_t> headers) noexcept :
 Headers(proto_t::NONE, ::move(provider), fields_t (headers), nullptr, nullptr) {}
/**
 * @brief Конструктор
 *
 * @param proto    протокол HTTP-запроса/ответа
 * @param provider объект провайдера HTTP-запроса/ответа
 * @param headers  список заголовков инициализации
 */
awh::http::Headers::Headers(const proto_t proto, const provider_t * provider, const fields_t & headers) noexcept :
 Headers(proto, provider, headers, nullptr, nullptr) {}
/**
 * @brief Конструктор
 *
 * @param proto    протокол HTTP-запроса/ответа
 * @param provider объект провайдера HTTP-запроса/ответа
 * @param headers  список заголовков инициализации
 */
awh::http::Headers::Headers(const proto_t proto, const provider_t * provider, const entries_t & headers) noexcept :
 Headers(proto, provider, headers, nullptr, nullptr) {}
/**
 * @brief Конструктор
 *
 * @param proto    протокол HTTP-запроса/ответа
 * @param provider объект провайдера HTTP-запроса/ответа
 * @param headers  список заголовков инициализации
 */
awh::http::Headers::Headers(const proto_t proto, const provider_t * provider, const multimap_t & headers) noexcept :
 Headers(proto, provider, ::convert(headers), nullptr, nullptr) {}
/**
 * @brief Конструктор
 *
 * @param proto    протокол HTTP-запроса/ответа
 * @param provider объект провайдера HTTP-запроса/ответа
 * @param headers  список заголовков инициализации
 */
awh::http::Headers::Headers(const proto_t proto, const provider_t * provider, initializer_list <header_t> headers) noexcept :
 Headers(proto, provider, fields_t (headers), nullptr, nullptr) {}
/**
 * @brief Конструктор
 *
 * @param proto    протокол HTTP-запроса/ответа
 * @param provider объект провайдера HTTP-запроса/ответа
 * @param headers  список заголовков инициализации
 */
awh::http::Headers::Headers(const proto_t proto, unique_ptr <provider_t> && provider, const fields_t & headers) noexcept :
 Headers(proto, ::move(provider), headers, nullptr, nullptr) {}
/**
 * @brief Конструктор
 *
 * @param proto    протокол HTTP-запроса/ответа
 * @param provider объект провайдера HTTP-запроса/ответа
 * @param headers  список заголовков инициализации
 */
awh::http::Headers::Headers(const proto_t proto, unique_ptr <provider_t> && provider, const entries_t & headers) noexcept :
 Headers(proto, ::move(provider), headers, nullptr, nullptr) {}
/**
 * @brief Конструктор
 *
 * @param proto    протокол HTTP-запроса/ответа
 * @param provider объект провайдера HTTP-запроса/ответа
 * @param headers  список заголовков инициализации
 */
awh::http::Headers::Headers(const proto_t proto, unique_ptr <provider_t> && provider, const multimap_t & headers) noexcept :
 Headers(proto, ::move(provider), ::convert(headers), nullptr, nullptr) {}
/**
 * @brief Конструктор
 *
 * @param proto    протокол HTTP-запроса/ответа
 * @param provider объект провайдера HTTP-запроса/ответа
 * @param headers  список заголовков инициализации
 */
awh::http::Headers::Headers(const proto_t proto, unique_ptr <provider_t> && provider, initializer_list <header_t> headers) noexcept :
 Headers(proto, ::move(provider), fields_t (headers), nullptr, nullptr) {}
/**
 * @brief Конструктор
 *
 * @param fmk объект фреймворка
 * @param log объект для работы с логами
 */
awh::http::Headers::Headers(const fmk_t * fmk, const log_t * log) noexcept :
 Headers(proto_t::NONE, static_cast <const provider_t *> (nullptr), fields_t {}, fmk, log) {}
/**
 * @brief Конструктор
 *
 * @param proto протокол HTTP-запроса/ответа
 */
awh::http::Headers::Headers(const proto_t proto, const fmk_t * fmk, const log_t * log) noexcept :
 Headers(proto, static_cast <const provider_t *> (nullptr), fields_t {}, fmk, log) {}
/**
 * @brief Конструктор
 *
 * @param provider объект провайдера HTTP-запроса/ответа
 */
awh::http::Headers::Headers(const provider_t * provider, const fmk_t * fmk, const log_t * log) noexcept :
 Headers(proto_t::NONE, provider, fields_t {}, fmk, log) {}
/**
 * @brief Конструктор
 *
 * @param provider объект провайдера HTTP-запроса/ответа
 */
awh::http::Headers::Headers(unique_ptr <provider_t> && provider, const fmk_t * fmk, const log_t * log) noexcept :
 Headers(proto_t::NONE, ::move(provider), fields_t {}, fmk, log) {}
/**
 * @brief Конструктор
 *
 * @param headers список заголовков инициализации
 * @param fmk     объект фреймворка
 * @param log     объект для работы с логами
 */
awh::http::Headers::Headers(const fields_t & headers, const fmk_t * fmk, const log_t * log) noexcept :
 Headers(proto_t::NONE, static_cast <const provider_t *> (nullptr), headers, fmk, log) {}
/**
 * @brief Конструктор
 *
 * @param headers список заголовков инициализации
 * @param fmk     объект фреймворка
 * @param log     объект для работы с логами
 */
awh::http::Headers::Headers(const entries_t & headers, const fmk_t * fmk, const log_t * log) noexcept :
 Headers(proto_t::NONE, static_cast <const provider_t *> (nullptr), headers, fmk, log) {}
/**
 * @brief Конструктор
 *
 * @param headers список заголовков инициализации
 * @param fmk     объект фреймворка
 * @param log     объект для работы с логами
 */
awh::http::Headers::Headers(const multimap_t & headers, const fmk_t * fmk, const log_t * log) noexcept :
 Headers(proto_t::NONE, static_cast <const provider_t *> (nullptr), ::convert(headers), fmk, log) {}
/**
 * @brief Конструктор
 *
 * @param headers список заголовков инициализации
 * @param fmk     объект фреймворка
 * @param log     объект для работы с логами
 */
awh::http::Headers::Headers(initializer_list <header_t> headers, const fmk_t * fmk, const log_t * log) noexcept :
 Headers(proto_t::NONE, static_cast <const provider_t *> (nullptr), fields_t (headers), fmk, log) {}
/**
 * @brief Конструктор
 *
 * @param proto   протокол HTTP-запроса/ответа
 * @param headers список заголовков инициализации
 * @param fmk     объект фреймворка
 * @param log     объект для работы с логами
 */
awh::http::Headers::Headers(const proto_t proto, const fields_t & headers, const fmk_t * fmk, const log_t * log) noexcept :
 Headers(proto, static_cast <const provider_t *> (nullptr), headers, fmk, log) {}
/**
 * @brief Конструктор
 *
 * @param proto   протокол HTTP-запроса/ответа
 * @param headers список заголовков инициализации
 * @param fmk     объект фреймворка
 * @param log     объект для работы с логами
 */
awh::http::Headers::Headers(const proto_t proto, const entries_t & headers, const fmk_t * fmk, const log_t * log) noexcept :
 Headers(proto, static_cast <const provider_t *> (nullptr), headers, fmk, log) {}
/**
 * @brief Конструктор
 *
 * @param proto   протокол HTTP-запроса/ответа
 * @param headers список заголовков инициализации
 * @param fmk     объект фреймворка
 * @param log     объект для работы с логами
 */
awh::http::Headers::Headers(const proto_t proto, const multimap_t & headers, const fmk_t * fmk, const log_t * log) noexcept :
 Headers(proto, static_cast <const provider_t *> (nullptr), ::convert(headers), fmk, log) {}
/**
 * @brief Конструктор
 *
 * @param proto   протокол HTTP-запроса/ответа
 * @param headers список заголовков инициализации
 * @param fmk     объект фреймворка
 * @param log     объект для работы с логами
 */
awh::http::Headers::Headers(const proto_t proto, initializer_list <header_t> headers, const fmk_t * fmk, const log_t * log) noexcept :
 Headers(proto, static_cast <const provider_t *> (nullptr), fields_t (headers), fmk, log) {}
/**
 * @brief Конструктор
 *
 * @param provider объект провайдера HTTP-запроса/ответа
 * @param headers  список заголовков инициализации
 * @param fmk      объект фреймворка
 * @param log      объект для работы с логами
 */
awh::http::Headers::Headers(const provider_t * provider, const fields_t & headers, const fmk_t * fmk, const log_t * log) noexcept :
 Headers(proto_t::NONE, provider, headers, fmk, log) {}
/**
 * @brief Конструктор
 *
 * @param provider объект провайдера HTTP-запроса/ответа
 * @param headers  список заголовков инициализации
 * @param fmk      объект фреймворка
 * @param log      объект для работы с логами
 */
awh::http::Headers::Headers(const provider_t * provider, const entries_t & headers, const fmk_t * fmk, const log_t * log) noexcept :
 Headers(proto_t::NONE, provider, headers, fmk, log) {}
/**
 * @brief Конструктор
 *
 * @param provider объект провайдера HTTP-запроса/ответа
 * @param headers  список заголовков инициализации
 * @param fmk      объект фреймворка
 * @param log      объект для работы с логами
 */
awh::http::Headers::Headers(const provider_t * provider, const multimap_t & headers, const fmk_t * fmk, const log_t * log) noexcept :
 Headers(proto_t::NONE, provider, ::convert(headers), fmk, log) {}
/**
 * @brief Конструктор
 *
 * @param provider объект провайдера HTTP-запроса/ответа
 * @param headers  список заголовков инициализации
 * @param fmk      объект фреймворка
 * @param log      объект для работы с логами
 */
awh::http::Headers::Headers(const provider_t * provider, initializer_list <header_t> headers, const fmk_t * fmk, const log_t * log) noexcept :
 Headers(proto_t::NONE, provider, fields_t (headers), fmk, log) {}
/**
 * @brief Конструктор
 *
 * @param provider объект провайдера HTTP-запроса/ответа
 * @param headers  список заголовков инициализации
 * @param fmk      объект фреймворка
 * @param log      объект для работы с логами
 */
awh::http::Headers::Headers(unique_ptr <provider_t> && provider, const fields_t & headers, const fmk_t * fmk, const log_t * log) noexcept :
 Headers(proto_t::NONE, ::move(provider), headers, fmk, log) {}
/**
 * @brief Конструктор
 *
 * @param provider объект провайдера HTTP-запроса/ответа
 * @param headers  список заголовков инициализации
 * @param fmk      объект фреймворка
 * @param log      объект для работы с логами
 */
awh::http::Headers::Headers(unique_ptr <provider_t> && provider, const entries_t & headers, const fmk_t * fmk, const log_t * log) noexcept :
 Headers(proto_t::NONE, ::move(provider), headers, fmk, log) {}
/**
 * @brief Конструктор
 *
 * @param provider объект провайдера HTTP-запроса/ответа
 * @param headers  список заголовков инициализации
 * @param fmk      объект фреймворка
 * @param log      объект для работы с логами
 */
awh::http::Headers::Headers(unique_ptr <provider_t> && provider, const multimap_t & headers, const fmk_t * fmk, const log_t * log) noexcept :
 Headers(proto_t::NONE, ::move(provider), ::convert(headers), fmk, log) {}
/**
 * @brief Конструктор
 *
 * @param provider объект провайдера HTTP-запроса/ответа
 * @param headers  список заголовков инициализации
 * @param fmk      объект фреймворка
 * @param log      объект для работы с логами
 */
awh::http::Headers::Headers(unique_ptr <provider_t> && provider, initializer_list <header_t> headers, const fmk_t * fmk, const log_t * log) noexcept :
 Headers(proto_t::NONE, ::move(provider), fields_t (headers), fmk, log) {}
/**
 * @brief Конструктор
 *
 * @param proto    протокол HTTP-запроса/ответа
 * @param provider объект провайдера HTTP-запроса/ответа
 * @param headers  список заголовков инициализации
 * @param fmk      объект фреймворка
 * @param log      объект для работы с логами
 */
awh::http::Headers::Headers(const proto_t proto, const provider_t * provider, const fields_t & headers, const fmk_t * fmk, const log_t * log) noexcept :
 _memory(0), _proto(proto), _provider(nullptr), _fmk(fmk), _log(log) {
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		// Если объект провайдера передан - создаём его копию без срезки производной части
		if(provider != nullptr)
			// Создаём копию объекта провайдера без срезки производной части
			this->_provider = provider->clone();
		/**
		 * Добавляем все переданные заголовки, сохраняя допустимые дубликаты имён
		 */
		for(const auto & header : headers)
			// Добавляем заголовок в текущий набор с сохранением дубликатов
			this->_emplace(header.name, header.value);
		// Если протокол явно не был указан - определяем его автоматически по составу переданных заголовков
		if(this->_proto == proto_t::NONE){
			// Автоматически определяем протокол на основе состава переданных заголовков
			this->_proto = ::detectProto(headers);
			// Приводим названия всех заголовков к канонической форме определённого протокола
			this->_recase();
		}
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		// Записываем ошибку в лог
		this->_error(__PRETTY_FUNCTION__, error.what());
	}
}
/**
 * @brief Конструктор
 *
 * @param proto    протокол HTTP-запроса/ответа
 * @param provider объект провайдера HTTP-запроса/ответа
 * @param headers  список заголовков инициализации
 * @param fmk      объект фреймворка
 * @param log      объект для работы с логами
 */
awh::http::Headers::Headers(const proto_t proto, const provider_t * provider, const entries_t & headers, const fmk_t * fmk, const log_t * log) noexcept :
 _memory(0), _proto(proto), _provider(nullptr), _fmk(fmk), _log(log) {
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		// Если объект провайдера передан - создаём его копию без срезки производной части
		if(provider != nullptr)
			// Создаём копию объекта провайдера без срезки производной части
			this->_provider = provider->clone();
		/**
		 * Добавляем все переданные заголовки, сохраняя допустимые дубликаты имён
		 */
		for(const auto & header : headers)
			// Добавляем заголовок в текущий набор с сохранением дубликатов
			this->_emplace(header.name, header.value);
		// Если протокол явно не был указан - определяем его автоматически по составу переданных заголовков
		if(this->_proto == proto_t::NONE){
			// Автоматически определяем протокол на основе состава переданных заголовков
			this->_proto = ::detectProto(headers);
			// Приводим названия всех заголовков к канонической форме определённого протокола
			this->_recase();
		}
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		// Записываем ошибку в лог
		this->_error(__PRETTY_FUNCTION__, error.what());
	}
}
/**
 * @brief Конструктор
 *
 * @param proto    протокол HTTP-запроса/ответа
 * @param provider объект провайдера HTTP-запроса/ответа
 * @param headers  список заголовков инициализации
 * @param fmk      объект фреймворка
 * @param log      объект для работы с логами
 */
awh::http::Headers::Headers(const proto_t proto, const provider_t * provider, const multimap_t & headers, const fmk_t * fmk, const log_t * log) noexcept :
 Headers(proto, provider, ::convert(headers), fmk, log) {}
/**
 * @brief Конструктор
 *
 * @param proto    протокол HTTP-запроса/ответа
 * @param provider объект провайдера HTTP-запроса/ответа
 * @param headers  список заголовков инициализации
 * @param fmk      объект фреймворка
 * @param log      объект для работы с логами
 */
awh::http::Headers::Headers(const proto_t proto, const provider_t * provider, initializer_list <header_t> headers, const fmk_t * fmk, const log_t * log) noexcept :
 Headers(proto, provider, fields_t (headers), fmk, log) {}
/**
 * @brief Конструктор
 *
 * @param proto    протокол HTTP-запроса/ответа
 * @param provider объект провайдера HTTP-запроса/ответа
 * @param headers  список заголовков инициализации
 * @param fmk      объект фреймворка
 * @param log      объект для работы с логами
 */
awh::http::Headers::Headers(const proto_t proto, unique_ptr <provider_t> && provider, const fields_t & headers, const fmk_t * fmk, const log_t * log) noexcept :
 _memory(0), _proto(proto), _provider(::move(provider)), _fmk(fmk), _log(log) {
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		/**
		 * Добавляем все переданные заголовки, сохраняя допустимые дубликаты имён
		 */
		for(const auto & header : headers)
			// Добавляем заголовок в текущий набор с сохранением дубликатов
			this->_emplace(header.name, header.value);
		// Если протокол явно не был указан - определяем его автоматически по составу переданных заголовков
		if(this->_proto == proto_t::NONE){
			// Автоматически определяем протокол на основе состава переданных заголовков
			this->_proto = ::detectProto(headers);
			// Приводим названия всех заголовков к канонической форме определённого протокола
			this->_recase();
		}
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		// Записываем ошибку в лог
		this->_error(__PRETTY_FUNCTION__, error.what());
	}
}
/**
 * @brief Конструктор
 *
 * @param proto    протокол HTTP-запроса/ответа
 * @param provider объект провайдера HTTP-запроса/ответа
 * @param headers  список заголовков инициализации
 * @param fmk      объект фреймворка
 * @param log      объект для работы с логами
 */
awh::http::Headers::Headers(const proto_t proto, unique_ptr <provider_t> && provider, const entries_t & headers, const fmk_t * fmk, const log_t * log) noexcept :
 _memory(0), _proto(proto), _provider(::move(provider)), _fmk(fmk), _log(log) {
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		/**
		 * Добавляем все переданные заголовки, сохраняя допустимые дубликаты имён
		 */
		for(const auto & header : headers)
			// Добавляем заголовок в текущий набор с сохранением дубликатов
			this->_emplace(header.name, header.value);
		// Если протокол явно не был указан - определяем его автоматически по составу переданных заголовков
		if(this->_proto == proto_t::NONE){
			// Автоматически определяем протокол на основе состава переданных заголовков
			this->_proto = ::detectProto(headers);
			// Приводим названия всех заголовков к канонической форме определённого протокола
			this->_recase();
		}
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception & error) {
		// Записываем ошибку в лог
		this->_error(__PRETTY_FUNCTION__, error.what());
	}
}
/**
 * @brief Конструктор
 *
 * @param proto    протокол HTTP-запроса/ответа
 * @param provider объект провайдера HTTP-запроса/ответа
 * @param headers  список заголовков инициализации
 * @param fmk      объект фреймворка
 * @param log      объект для работы с логами
 */
awh::http::Headers::Headers(const proto_t proto, unique_ptr <provider_t> && provider, const multimap_t & headers, const fmk_t * fmk, const log_t * log) noexcept :
 Headers(proto, ::move(provider), ::convert(headers), fmk, log) {}
/**
 * @brief Конструктор
 *
 * @param proto    протокол HTTP-запроса/ответа
 * @param provider объект провайдера HTTP-запроса/ответа
 * @param headers  список заголовков инициализации
 * @param fmk      объект фреймворка
 * @param log      объект для работы с логами
 */
awh::http::Headers::Headers(const proto_t proto, unique_ptr <provider_t> && provider, initializer_list <header_t> headers, const fmk_t * fmk, const log_t * log) noexcept :
 Headers(proto, ::move(provider), fields_t (headers), fmk, log) {}
/**
 * @brief Деструктор
 *
 */
awh::http::Headers::~Headers() noexcept {
	// Очищаем набор заголовков и сбрасываем объект провайдера HTTP-запроса/ответа
	this->_headers.clear();
	// Освобождаем память, выделенную под объект провайдера
	this->_provider.reset(nullptr);
}

/**
 * @brief Оператор [<<] вывода в поток буфера
 *
 * @param os      поток куда нужно вывести данные
 * @param headers контейнер заголовков
 */
ostream & awh::operator << (ostream & os, const http::headers_t & headers) noexcept {
	// Записываем в поток текстовое представление всех заголовков в формате протокола HTTP
	os << headers.print(static_cast <http::proto_t> (headers));
	// Выводим результат
	return os;
}
