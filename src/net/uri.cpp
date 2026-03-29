/**
 * @file: uri.cpp
 * @date: 2026-03-28
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
 * Стандартные модули
 */
#include <cctype>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <string_view>

/**
 * Подключаем заголовочный файл
 */
#include <cstdint>
#include <net/uri.hpp>

/**
 * Подписываемся на стандартное пространство имён
 */
using namespace std;

/**
 * Инкапсулируем парсинг URI в пространство имён
 */
namespace uri {
	/**
	 * Подписываемся на пространство имён AWH
	 */
	using namespace awh;

	/**
	 * @brief Состояния парсинга URI
	 *
	 */
	enum class state_t : uint8_t {
		NONE      = 0x00, // Состояние парсинга URI не определено
		HOST      = 0x01, // Внутри авторити: хост
		PORT      = 0x02, // Внутри авторити: порт (после :)
		PATH      = 0x03, // Путь (до ? или #)
		QUERY     = 0x04, // Запрос (до #)
		SCHEME    = 0x05, // Читаем схему (до :)
		FRAGMENT  = 0x06, // Фрагмент (до конца)
		AUTHORITY = 0x07  // Читаем авторити (// ... до / ? #)
	};

	/**
	 * @brief Функция проверки допустимых символов в схеме URI
	 *
	 * @param letter символ для проверки
	 * @return       результат проверки
	 */
	[[nodiscard]] static inline bool isSchemeLetter(const char letter) noexcept {
		/**
		 * Проверяем, является ли символ допустимым для схемы URI (буква, цифра, +, -, .)
		 */
		return (
			((letter >= 'a') && (letter <= 'z')) ||
			((letter >= 'A') && (letter <= 'Z')) ||
			((letter >= '0') && (letter <= '9')) ||
			(letter == '+') || (letter == '-') || (letter == '.')
		);
	}

	/**
	 * @brief Парсинг URI в один проход (Single Pass)
	 *
	 * @param uri      строка URI для парсинга
	 * @param scheme   ссылка для сохранения схемы URI
	 * @param userinfo ссылка для сохранения параметров пользователя URI
	 * @param host     ссылка для сохранения хоста URI
	 * @param port     ссылка для сохранения порта URI
	 * @param path     ссылка для сохранения пути URI
	 * @param query    ссылка для сохранения параметров URI
	 * @param fragment ссылка для сохранения якоря URI
	 * @return         результат парсинга
	 */
	[[nodiscard]] static bool parse(
		string_view uri,
		string_view & scheme,
		string_view & userinfo,
		string_view & host,
		string_view & port,
		string_view & path,
		string_view & query,
		string_view & fragment
	) noexcept {
		// Инициализируем все выходные параметры пустыми строками
		scheme = userinfo = host = port = path = query = fragment = {};
		// Если URI пустой
		if(uri.empty())
			// Выводим результат парсинга
			return false;
		// Итератор на начало строки
		const char * begin = uri.data();
		// Итератор на текущую позицию в строке
		const char * ptr = begin;
		// Итератор на конец строки для оптимизации сравнения
		const char * end = (begin + uri.size());
		// Указатели на начало текущей лексемы
		const char * tokenBegin = begin;
		// Флаг для обработки [IPv6]
		bool inIPv6 = false;
		// Флаг для проверки валидности схемы URI
		bool schemeValid = false;
		// Флаг для определения наличия авторити (//)
		bool hasAuthority = false;
		// Текущее состояние парсинга URI
		state_t state = state_t::SCHEME;
		// Вспомогательный маркер для хоста URI
		const char * hostBegin = nullptr;
		// Вспомогательный маркер для userinfo
		const char * userinfoBegin = nullptr;
		/**
		 * Проходим по каждому символу URI и обрабатываем его в зависимости от текущего состояния парсинга
		 */
		while(ptr < end){
			// Получаем текущий символ
			char letter = (* ptr);
			/**
			 * В зависимости от текущего состояния парсинга URI, обрабатываем символ и при необходимости меняем состояние
			 */
			switch(static_cast <uint8_t> (state)){
				// Если мы читаем схему URI
				case static_cast <uint8_t> (state_t::SCHEME): {
					// Если встретили разделитель схемы
					if(letter == ':'){
						// Если до разделителя были символы, и схема валидна
						if((ptr > tokenBegin) && schemeValid){
							/**
							 * Проверяем, содержит ли кандидат точку.
							 * Настоящие схемы URI (http, ftp, mailto, ...) точек не содержат.
							 * Если точка есть — это доменное имя (www.example.com:443/...),
							 * а не схема, и мы переходим к разбору хоста.
							 */
							bool hasDot = false;
							// Ищем точку в кандидате на схему
							for(const char * s = tokenBegin; s < ptr; ++s){
								// Если нашли точку
								if(* s == '.'){
									// Устанавливаем флаг
									hasDot = true;
									// Прерываем поиск
									break;
								}
							}
							// Если точка найдена — это домен, а не схема
							if(hasDot){
								// Помечаем наличие authority без //
								hasAuthority = true;
								// Переходим к чтению хоста
								state = state_t::HOST;
								// Если хост начинается с [ — мы внутри IPv6-адреса
								inIPv6 = (* tokenBegin == '[');
								// Начало хоста — начало строки
								hostBegin = tokenBegin;
								// Начало userinfo — то же место
								userinfoBegin = tokenBegin;
								// Обработать : в состоянии HOST (оно найдёт порт)
								continue;
							}
							// Иначе сохраняем схему URI
							scheme = string_view(tokenBegin, ptr - tokenBegin);
						/**
						 * Если схема не валидна (например токен начинается с цифры: 127.0.0.1,
						 * или с [: IPv6-адрес [2001:db8::1]),
						 * проверяем наличие точки или [ — тогда это IP/домен, а не путь
						 */
						} else if(ptr > tokenBegin) {
							// Флаг: токен является хостом (есть точка или это IPv6 в [])
							bool isHost = (* tokenBegin == '[');
							// Если не IPv6 — ищем точку
							if(!isHost){
								// Ищем точку в кандидате
								for(const char * s = tokenBegin; s < ptr; ++s){
									// Если нашли точку
									if(* s == '.'){
										// Устанавливаем флаг
										isHost = true;
										// Прерываем поиск
										break;
									}
								}
							}
							// Если точка найдена или IPv6 — это host:port (например 127.0.0.1:443 или [::1]:443)
							if(isHost){
								// Помечаем наличие authority без //
								hasAuthority = true;
								// Переходим к чтению хоста
								state = state_t::HOST;
								// Если хост начинается с [ — мы внутри IPv6-адреса
								inIPv6 = (* tokenBegin == '[');
								// Начало хоста — начало строки
								hostBegin = tokenBegin;
								// Начало userinfo — то же место
								userinfoBegin = tokenBegin;
								// Обработать : в состоянии HOST (оно найдёт порт)
								continue;
							}
							// Точки нет — это не хост, идём в путь
							tokenBegin = begin;
							// Сбрасываем состояние
							state = state_t::PATH;
							// Обработать текущий символ в контексте PATH
							continue;
						// Если до разделителя не было символов, и это не начало схемы — это может быть путь, начинающийся с : (например :/path)
						} else {
							// До разделителя не было символов — путь
							tokenBegin = begin; 
							// Сбрасываем состояние, идем в путь с начала строки
							state = state_t::PATH;
							// Не делаем ptr++, чтобы обработать текущий символ в контексте PATH
							continue; 
						}
						// Переходим к состоянию чтения авторити (ожидаем // или путь)
						state = state_t::AUTHORITY;
						// Устанавливаем начало токена на следующий символ после ":"
						tokenBegin = (ptr + 1);
					// Если встретили разделитель пути, запроса или фрагмента до разделителя схемы
					} else if((letter == '/') || (letter == '?') || (letter == '#')) {
						/**
						 * Проверяем, есть ли точка в части до разделителя, или начинается с [.
						 * Если есть — это домен/IP без схемы (www.example.com/path, 127.0.0.1/path, [::1]/path).
						 */
						// Флаг: токен является хостом (есть точка или это IPv6 в [])
						bool isHost = ((ptr > tokenBegin) && (* tokenBegin == '['));
						// Если не IPv6 — ищем точку
						if(!isHost && (ptr > tokenBegin)){
							// Ищем точку в кандидате
							for(const char * s = tokenBegin; s < ptr; ++s){
								// Если нашли точку
								if(* s == '.'){
									// Устанавливаем флаг
									isHost = true;
									// Прерываем поиск
									break;
								}
							}
						}
						// Если точка найдена или IPv6 — это хост, переходим к разбору хоста
						if(isHost){
							// Помечаем наличие authority без //
							hasAuthority = true;
							// Переходим к чтению хоста
							state = state_t::HOST;
							// Если хост начинается с [ — мы внутри IPv6-адреса
							inIPv6 = (* tokenBegin == '[');
							// Начало хоста и userinfo — начало строки
							hostBegin = tokenBegin;
							// Начало userinfo — то же место
							userinfoBegin = tokenBegin;
							// Обработать / в состоянии HOST (оно сохранит хост и уйдёт в PATH)
							continue;
						}
						// Устанавливаем начало токена на начало строки, чтобы обработать весь URI как путь
						tokenBegin = begin;
						// Схема не найдена, это относительный URI или путь
						state = state_t::PATH;
						// Обработать символ в новом состоянии
						continue;
					// Если встретили @ — это userinfo@host без схемы (например user@example.com)
					} else if(letter == '@') {
						// Всё до @ — userinfo
						userinfo = string_view(tokenBegin, ptr - tokenBegin);
						// Помечаем наличие authority
						hasAuthority = true;
						// Переходим к чтению хоста
						state = state_t::HOST;
						// Начало хоста — символ после @
						tokenBegin = (ptr + 1);
						// Устанавливаем начало хоста
						hostBegin = tokenBegin;
						// userinfoBegin уже не нужен, но на всякий случай
						userinfoBegin = tokenBegin;
					// Если это не разделитель, то проверяем валидность символов схемы
					} else {
						// Если это первый символ схемы, проверяем его на допустимость для первой позиции (должна быть буква)
						if(ptr == begin)
							// Устанавливаем флаг валидности схемы, если первый символ - допустимая буква
							schemeValid = (((letter >= 'a') && (letter <= 'z')) || ((letter >= 'A') && (letter <= 'Z')));
						// Если уже определили что схема не валидна, то не проверяем символы схемы, просто ждем разделителя или конец строки
						else if(!schemeValid) {
							/**
							 * Уже поняли что схема бита, но ждем разделителя
							 */
						// Если схема пока валидна, проверяем текущий символ на допустимость для схемы
						} else {
							// Если символ не допустим для схемы, устанавливаем флаг валидности схемы в false
							if(!isSchemeLetter(letter))
								// Схема не валидна, ждем разделителя или конец строки
								schemeValid = false;
						}
					}
				} break;
				// Если мы читаем авторити URI (после схемы и : )
				case static_cast <uint8_t> (state_t::AUTHORITY): {
					// Проверяем на наличие // для определения наличия авторити
					if(((ptr - tokenBegin) == 0) && (letter == '/')){
						/**
						 * Это начало //
						 * Ничего не делаем, ждем второй слэш или конец
						 */
					// Если встретили второй слэш, то подтверждаем наличие авторити и переходим к чтению хоста
					} else if(((ptr - tokenBegin) == 1) && (letter == '/')) {
						// Подтвердили //
						hasAuthority = true;
						// Устанавливаем состояние на чтение хоста
						state = state_t::HOST;
						// Устанавливаем начало токена на следующий символ после //
						tokenBegin = (ptr + 1);
						// Устанавливаем начало хоста на следующий символ после //
						hostBegin = tokenBegin;
						// Устанавливаем начало userinfo на начало хоста, так как userinfo может быть до @ в хосте
						userinfoBegin = tokenBegin;
					/**
					 * Если это не было //, проверяем, есть ли @ впереди до первого /, ?, # -
					 * это opaque URI вида scheme:user@host (например mailto:user@example.com)
					 */
					} else {
						// Ищем @ до первого /, ?, # или конца строки
						bool hasAtInOpaque = false;
						// Перебираем символы начиная с текущего
						for(const char * look = ptr; look < end; ++look){
							// Если нашли @ - это userinfo@host без //
							if(* look == '@'){
								// Нашли @ в opaque URI
								hasAtInOpaque = true;
								// Прерываем поиск
								break;
							}
							// Если нашли /, ?, # раньше @ - это обычный путь без userinfo
							if((* look == '/') || (* look == '?') || (* look == '#'))
								// Прерываем поиск
								break;
						}
						// Если @ найден — парсим как userinfo@host (например mailto:user@example.com)
						if(hasAtInOpaque){
							// Помечаем, что у нас есть authority (нужно для корректной финализации хоста)
							hasAuthority = true;
							// Переходим к чтению хоста
							state = state_t::HOST;
							// Устанавливаем начало токена на текущую позицию (после scheme:)
							tokenBegin = (begin + scheme.size() + 1);
							// Устанавливаем начало хоста и userinfo
							hostBegin = tokenBegin;
							// Устанавливаем начало userinfo
							userinfoBegin = tokenBegin;
							// Обработать текущий символ в состоянии HOST
							continue;
						}
						// Иначе — это обычный opaque URI (например scheme:path-only), идём в путь
						// Устанавливаем итератор на начало пути (после scheme:)
						tokenBegin = (begin + scheme.size() + 1);
						// Устанавливаем состояние на чтение пути
						state = state_t::PATH;
						// Обработать символ в новом состоянии
						continue;
					}
				} break;
				// Если мы читаем хост URI (внутри авторити)
				case static_cast <uint8_t> (state_t::HOST): {
					// Если встретили [ или ], то это может быть IPv6-адрес внутри авторити, и @ для userinfo, но только если мы не внутри IPv6-адреса
					if(letter == '[')
						// Начало IPv6-адреса, устанавливаем флаг
						inIPv6 = true;
					// Если встретили ], то это может быть конец IPv6-адреса внутри авторити, сбрасываем флаг
					else if(letter == ']')
						// Конец IPv6-адреса, сбрасываем флаг
						inIPv6 = false;
					// Если встретили @ и мы не внутри IPv6-адреса, то это разделитель userinfo и хоста
					else if((letter == '@') && !inIPv6) {
						// Userinfo найден
						userinfo = string_view(userinfoBegin, ptr - userinfoBegin);
						// Переходим к чтению хоста
						tokenBegin = (ptr + 1);
						// Устанавливаем начало хоста на следующий символ после @
						hostBegin = tokenBegin;
					/**
					 * Если встретили : и мы не внутри IPv6-адреса, то это может быть разделитель хоста и порта
					 * или разделитель логина и пароля в userinfo (если впереди есть @)
					 */
					} else if((letter == ':') && !inIPv6) {
						/**
						 * Проверяем, есть ли @ впереди (вне скобок IPv6) до первого /, ?, # или конца строки.
						 * Если @ найден раньше — текущий : является частью userinfo (разделитель пароля),
						 * и переходить в PORT рано. Если @ не найден — это host:port разделитель.
						 */
						bool hasAtAhead = false;
						{
							// Глубина вложенности скобок IPv6
							uint16_t depth = 0;
							// Перебираем символы после текущего :
							for(const char * look = (ptr + 1); look < end; ++look){
								// Если встретили [, увеличиваем глубину
								if(* look == '[')
									// Входим в IPv6-адрес
									++depth;
								// Если встретили ], уменьшаем глубину
								else if(* look == ']')
									// Выходим из IPv6-адреса
									--depth;
								// Если встретили @ вне скобок IPv6, то это разделитель userinfo и хоста
								else if((* look == '@') && (depth == 0)){
									// Нашли @ вне IPv6
									hasAtAhead = true;
									// Прерываем поиск
									break;
								// Если встретили /, ?, # вне скобок, то @ дальше не будет (конец авторити)
								} else if(((* look == '/') || (* look == '?') || (* look == '#')) && (depth == 0))
									// Прерываем поиск
									break;
							}
						}
						// Если @ впереди не найден — текущий : является разделителем хоста и порта
						if(!hasAtAhead){
							// Разделитель хоста и порта
							host = string_view(hostBegin, ptr - hostBegin);
							// Переходим к чтению порта
							state = state_t::PORT;
							// Устанавливаем начало токена на следующий символ после :
							tokenBegin = (ptr + 1);
						}
					/**
					 * Иначе : является частью userinfo (пароль), продолжаем чтение хоста
					 * Если встретили /, ?, # и мы не внутри IPv6-адреса, то это конец авторити и начало пути, запроса или фрагмента
					 */
					} else if((letter == '/') || (letter == '?') || (letter == '#')) {
						// Конец авторити
						if(!host.data())
							// Если хост еще не сохранен (не было порта)
							host = string_view(hostBegin, ptr - hostBegin);
						// Переходим к чтению пути, запроса или фрагмента в зависимости от разделителя
						state = (
							((letter == '/') ? state_t::PATH : 
							((letter == '?') ? state_t::QUERY : state_t::FRAGMENT))
						);
						// Если это был разделитель пути, то путь начинается с разделителя, иначе путь начинается после разделителя
						if(letter == '/')
							// Путь включает /
							tokenBegin = ptr;
						// Если это был разделитель запроса или фрагмента, то путь начинается после разделителя
						else tokenBegin = (ptr + 1);
						// Обработать разделитель в новом стейте
						continue;
					}
				} break;
				// Если мы читаем порт URI (внутри авторити после : )
				case static_cast <uint8_t> (state_t::PORT): {
					// Если встретили /, ?, #, то это конец порта и начало пути, запроса или фрагмента
					if((letter == '/') || (letter == '?') || (letter == '#')){
						// Сохраняем порт URI
						port = string_view(tokenBegin, ptr - tokenBegin);
						// Устанавливаем состояние на чтение пути, запроса или фрагмента в зависимости от разделителя
						state = (
							((letter == '/') ? state_t::PATH : 
							((letter == '?') ? state_t::QUERY : state_t::FRAGMENT))
						);
						// Если это был разделитель пути, то путь начинается с разделителя, иначе путь начинается после разделителя
						if(letter == '/')
							// Путь включает /
							tokenBegin = ptr;
						// Если это был разделитель запроса или фрагмента, то путь начинается после разделителя
						else tokenBegin = (ptr + 1);
						// Обработать разделитель в новом стейте
						continue;
					}
				// Иначе просто продолжаем копить порт (валидацию цифр можно сделать позже)
				} break;
				// Если мы читаем путь URI
				case static_cast <uint8_t> (state_t::PATH): {
					// Если встретили ? или #, то это конец пути и начало запроса или фрагмента
					if(letter == '?'){
						// Сохраняем путь URI
						path = string_view(tokenBegin, ptr - tokenBegin);
						// Устанавливаем состояние на чтение запроса
						state = state_t::QUERY;
						// Устанавливаем начало токена на следующий символ после ?
						tokenBegin = (ptr + 1);
					// Если встретили #, то это конец пути и начало фрагмента
					} else if(letter == '#') {
						// Сохраняем путь URI
						path = string_view(tokenBegin, ptr - tokenBegin);
						// Устанавливаем состояние на чтение фрагмента
						state = state_t::FRAGMENT;
						// Устанавливаем начало токена на следующий символ после #
						tokenBegin = (ptr + 1);
					}
				} break;
				// Если мы читаем параметры URI
				case static_cast <uint8_t> (state_t::QUERY): {
					// Если встретили #, то это конец параметров и начало фрагмента
					if(letter == '#'){
						// Сохраняем параметры URI
						query = string_view(tokenBegin, ptr - tokenBegin);
						// Устанавливаем состояние на чтение фрагмента
						state = state_t::FRAGMENT;
						// Устанавливаем начало токена на следующий символ после #
						tokenBegin = (ptr + 1);
					}
				} break;
				// Если мы читаем якорь URI
				case static_cast <uint8_t> (state_t::FRAGMENT):
					// До конца строки ничего не меняем
					break;
			}
			// Переходим к следующему символу
			ptr++;
		}
		/**
		 * Финализация (конец строки)
		 */
		switch(static_cast <uint8_t> (state)){
			// Если мы закончили на чтении схемы, то это может быть схема без : или путь без схемы
			case static_cast <uint8_t> (state_t::SCHEME): {
				/**
				 * Строка без разделителей (нет ни : ни / ни ? ни # ни @).
				 * Проверяем, содержит ли строка @: тогда это userinfo@host без схемы.
				 * Иначе — относительный путь.
				 */
				const char * atSign = nullptr;
				// Ищем @ в строке
				for(const char * s = tokenBegin; s < end; ++s){
					// Если нашли @
					if(* s == '@'){
						// Сохраняем указатель
						atSign = s;
						// Прерываем поиск
						break;
					}
				}
				// Если @ найден — это userinfo@host
				if(atSign != nullptr){
					// Сохраняем userinfo
					userinfo = string_view(tokenBegin, atSign - tokenBegin);
					// Сохраняем host (всё после @)
					host = string_view(atSign + 1, end - (atSign + 1));
				// Устанавливаем путь URI
				} else path = string_view(begin, end - begin);
			} break;
			// Если мы закончили на чтении авторити, то это может быть авторити без пути или портом, или просто путь
			case static_cast <uint8_t> (state_t::AUTHORITY):
				// Было scheme: но не было //
				path = string_view(tokenBegin, end - tokenBegin);
			break;
			// Если мы закончили на чтении хоста, то сохраняем хост (если был авторити) или путь (если не было авторити)
			case static_cast <uint8_t> (state_t::HOST): {
				// Если был авторити, сохраняем хост, иначе сохраняем путь
				if(hasAuthority)
					// Устанавливаем хост URI
					host = string_view(hostBegin, end - hostBegin);
				// Устанавливаем путь URI
				else path = string_view(begin, end - begin);
			} break;
			// Если мы закончили на чтении порта, сохраняем порт
			case static_cast <uint8_t> (state_t::PORT):
				// Устанавливаем порт URI
				port = string_view(tokenBegin, end - tokenBegin);
			break;
			// Если мы закончили на чтении пути, сохраняем путь
			case static_cast <uint8_t> (state_t::PATH):
				// Устанавливаем путь URI
				path = string_view(tokenBegin, end - tokenBegin);
			break;
			// Если мы закончили на чтении параметров, сохраняем параметры
			case static_cast <uint8_t> (state_t::QUERY):
				// Устанавливаем параметры URI
				query = string_view(tokenBegin, end - tokenBegin);
			break;
			// Если мы закончили на чтении якоря, сохраняем якорь
			case static_cast <uint8_t> (state_t::FRAGMENT):
				// Устанавливаем якорь URI
				fragment = string_view(tokenBegin, end - tokenBegin);
			break;
		}
		// Выводим результат парсинга
		return true;
	}

	/**
	 * @brief Функция кодирования строки в URL-адресе
	 *
	 * @param text строка текста для кодирования
	 * @param log  объект работы с логами
	 * @return     результат кодирования
	 */
	static string encode(string_view text, const log_t * log) noexcept {
		// Результат работы функции
		string result = "";
		// Если строка передана
		if(!text.empty()){
			/**
			 * Выполняем отлов ошибок
			 */
			try {
				// Создаём поток
				ostringstream ss;
				// Заполняем поток нулями
				ss.fill('0');
				// Переключаемся на 16-ю систему счисления
				ss << std::hex;
				// Перебираем все символы
				for(char letter : text){
					// Не трогаем буквенно-цифровые и другие допустимые символы.
					if(::isalnum(letter) || (letter == '-') || (letter == '_') || (letter == '.') || (letter == '~') || (letter == '@') ||
					  ((letter >= '0') && (letter <= '9')) || ((letter >= 'A') && (letter <= 'Z')) || ((letter >= 'a') && (letter <= 'z'))){
						// Записываем в поток символ, как он есть
						ss << letter;
						// Пропускаем итерацию
						continue;
					}
					/**
					 * Любые другие символы закодированы в процентах
					 */
					// Переводим символы в верхний регистр
					ss << std::uppercase;
					// Записываем в поток, код символа
					ss << '%' << std::setw(2) << static_cast <int16_t> (static_cast <uint8_t> (letter));
					// Убираем верхний регистр
					ss << std::nouppercase;
				}
				// Получаем результат
				result = ss.str();
			/**
			 * Если возникает ошибка
			 */
			} catch(const exception & error) {
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Выводим сообщение об ошибке
					log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(text), log_t::flag_t::WARNING, error.what());
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Выводим сообщение об ошибке
					log->print("%s", log_t::flag_t::WARNING, error.what());
				#endif
			}
		}
		// Выводим результат
		return result;
	}

	/**
	 * @brief Функция декодирования строки в URL-адресе
	 *
	 * @param text строка текста для декодирования
	 * @param log  объект работы с логами
	 * @return     результат декодирования
	 */
	static string decode(string_view text, const log_t * log) noexcept {
		// Результат работы функции
		string result = "";
		// Если строка передана
		if(!text.empty()){
			/**
			 * Выполняем отлов ошибок
			 */
			try {
				// Создаём бинарный буфер данных
				char buffer[3];
				// Устанавливаем завершение строки
				buffer[2] = '\0';
				// Код символа в 16-м виде
				uint16_t hex = 0;
				// Смещение в текстовом буфере
				const char * offset = nullptr;
				// Выделяем память для строки
				result.reserve(text.length());
				// Переходим по всей длине строки
				for(size_t i = 0; i < text.length(); i++){
					// Получаем текущее смещение в текстовом буфере
					offset = (text.data() + i);
					// Если это не проценты
					if(offset[0] != '%'){
						// Если это объединение двух слов
						if(offset[0] == '+')
							// Выполняем добавление разделителя
							result.append(1, ' ');
						// Иначе копируем букву как она есть
						else result.append(1, offset[0]);
					// Если же это проценты
					} else {
						// Выполняем копирование в бинарный буфер полученных байт
						::memcpy(buffer, offset + 1, 2);
						// Извлекаем из 16-х символов наш код числа
						::sscanf(buffer, "%hx", &hex);
						// Запоминаем полученный символ
						result.append(1, static_cast <char> (hex));
						// Смещаем итератор
						i += 2;
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
					// Выводим сообщение об ошибке
					log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(text), log_t::flag_t::WARNING, error.what());
				/**
				 * Если режим отладки не включён
				 */
				#else
					// Выводим сообщение об ошибке
					log->print("%s", log_t::flag_t::WARNING, error.what());
				#endif
			}
		}
		// Выводим результат
		return result;
	}
};

/**
 * @brief Метод очистки URI
 *
 */
void awh::Uniform_Resource_Identifier::clear() noexcept {

}
/**
 * @brief Метод проверки на существование данных
 *
 * @return результат проверки
 */
bool awh::Uniform_Resource_Identifier::empty() const noexcept {

}
/**
 * @brief Метод получения типа URI
 *
 * @return тип URI
 */
awh::Uniform_Resource_Identifier::type_t awh::Uniform_Resource_Identifier::type() const noexcept {

}
/**
 * @brief Метод получения схемы URI
 *
 * @return схема URI
 */
const string & awh::Uniform_Resource_Identifier::scheme() const noexcept {

}
/**
 * @brief Метод установки схемы URI
 *
 * @param scheme схема URI для установки
 */
void awh::Uniform_Resource_Identifier::scheme(string_view scheme) noexcept {

}
/**
 * @brief Метод получения параметров пользователя URI
 *
 * @return параметры пользователя URI
 */
const awh::Uniform_Resource_Identifier::user_t & awh::Uniform_Resource_Identifier::user() const noexcept {

}
/**
 * @brief Метод установки параметров пользователя URI
 *
 * @param user параметры пользователя URI для установки
 */
void awh::Uniform_Resource_Identifier::user(const user_t & user) noexcept {

}
/**
 * @brief Метод установки логина и пароля пользователя URI
 *
 * @param login логин пользователя URI для установки
 * @param pass  пароль пользователя URI для установки
 */
void awh::Uniform_Resource_Identifier::user(string_view login, string_view pass) noexcept {

}
/**
 * @brief Метод получения якоря URI
 *
 * @return якорь URI
 */
const string & awh::Uniform_Resource_Identifier::fragment() const noexcept {

}
/**
 * @brief Метод установки якоря URI
 *
 * @param fragment якорь URI для установки
 */
void awh::Uniform_Resource_Identifier::fragment(string_view fragment) noexcept {

}
/**
 * @brief Метод получения атрибутов URI
 *
 * @return атрибуты URI
 */
const awh::net::attr_t * awh::Uniform_Resource_Identifier::attr() const noexcept {

}
/**
 * @brief Метод установки атрибутов URI
 *
 * @param attr атрибуты URI для установки
 */
void awh::Uniform_Resource_Identifier::attr(const net::attr_t * attr) noexcept {

}
/**
 * @brief Метод получения хоста URI
 *
 * @return хост URI
 */
string awh::Uniform_Resource_Identifier::host() const noexcept {

}
/**
 * @brief Метод установки хоста URI
 *
 * @param host хост URI для установки
 */
void awh::Uniform_Resource_Identifier::host(string_view host) noexcept {

}
/**
 * @brief Метод получения пути URI
 *
 * @return путь URI
 */
const vector <string> & awh::Uniform_Resource_Identifier::path() const noexcept {

}
/**
 * @brief Метод установки пути URI
 *
 * @param path путь URI для установки
 */
void awh::Uniform_Resource_Identifier::path(const vector <string> & path) noexcept {

}
/**
 * @brief Метод получения параметров URI
 *
 * @return параметры URI
 */
const unordered_map <string, string> & awh::Uniform_Resource_Identifier::query() const noexcept {

}
/**
 * @brief Метод установки параметров URI
 *
 * @param query параметры URI для установки
 */
void awh::Uniform_Resource_Identifier::query(const unordered_map <string, string> & query) noexcept {

}
/**
 * @brief Метод парсинга URI-запроса
 *
 * @param uri строка URI-запроса для получения параметров
 * @return    тип URI
 */
awh::Uniform_Resource_Identifier::type_t awh::Uniform_Resource_Identifier::parse(string_view uri) const noexcept {
	// Результат работы функции
	type_t result = type_t::NONE;
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		// Результат парсинга URI
		string_view scheme, userinfo, host, port, path, query, fragment;
		// Выполняем парсинг URI
		if(uri::parse(uri, scheme, userinfo, host, port, path, query, fragment)){

			if(!scheme.empty())
				cout << "Scheme: " << scheme << endl;
			if(!userinfo.empty())
				cout << "Userinfo: " << userinfo << endl;
			if(!host.empty())
				cout << "Host: " << host << endl;
			if(!port.empty())
				cout << "Port: " << port << endl;
			if(!path.empty())
				cout << "Path: " << path << endl;
			if(!query.empty())
				cout << "Query: " << query << endl;
			if(!fragment.empty())
				cout << "Fragment: " << fragment << endl;
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
			this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(uri), log_t::flag_t::CRITICAL, error.what());
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
 * @brief Метод генерации ETag хэша текста
 *
 * @param text текст для перевода в строку
 * @param size размер хэша ETag для генерации (по умолчанию 16 байт)
 * @return     хэш etag
 */
string awh::Uniform_Resource_Identifier::etag(string_view text, const uint8_t size) const noexcept {
	// Если текст передан и размер хэша больше 0
	if(!text.empty() && (size > 0)){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Получаем 64-битный FNV-1a хэш строки
			uint64_t hash = 0xcbf29ce484222325ULL;
			// Перебираем все символы
			for(char letter : text){
				// Вычисляем FNV-1a хэш для каждого символа
				hash ^= static_cast <uint8_t> (letter);
				// Умножаем на простое число для 64-битного хэша
				hash *= 0x100000001b3ULL;
			}
			// Маска в зависимости от запрошенной длины
			uint64_t mask = 0;
			// Если размер хэша 8 байт или меньше
			if(size <= 8)
				// 8 hex символов (32 бита)
				mask = 0xFFFFFFFFULL;
			// Если размер хэша 16 байт или меньше
			else if(size <= 16)
				// 16 hex символов (64 бита)
				mask = 0xFFFFFFFFFFFFFFFFULL;
			// Если размер хэша больше 16 байт, используем максимум
			else {
				// Максимум для 64-битного хэша
				const_cast <uint8_t &> (size) = 16;
				// 16 hex символов (64 бита)
				mask = 0xFFFFFFFFFFFFFFFFULL;
			}
			// Формируем ETag в виде "hexhash"
			stringstream ss;
			// Записываем хэш в шестнадцатеричном виде, с ведущими нулями, в кавычках
			ss << "\"" << std::hex << std::setw(size) << std::setfill('0') << (hash & mask) << "\"";
			// Получаем строку ETag
			return ss.str();
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Выводим сообщение об ошибке
				this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(text), log_t::flag_t::WARNING, error.what());
			/**
			 * Если режим отладки не включён
			 */
			#else
				// Выводим сообщение об ошибке
				this->_log->print("%s", log_t::flag_t::WARNING, error.what());
			#endif
		}
	}
	// Выводим результат по умолчанию (пустой ETag)
	return "";
}
/**
 * @brief Метод создания заголовка [origin], для HTTP запроса
 *
 * @return заголовок [origin]
 */
string awh::Uniform_Resource_Identifier::origin() const noexcept {

}
/**
 * @brief Метод получения относительного URI-запроса
 *
 * @return относительный URI-запрос
 */
string awh::Uniform_Resource_Identifier::request() const noexcept {

}
/**
 * @brief Метод генерации строки URI
 *
 * @param format режим формата URI для генерации
 * @return       строка URI
 */
string awh::Uniform_Resource_Identifier::print(const format_t format) const noexcept {

}
/**
 * @brief Оператор проверки на существование данных
 *
 * @return результат проверки
 */
awh::Uniform_Resource_Identifier::operator bool() const noexcept {
	// Выводим результат проверки
	return !this->empty();
}
/**
 * @brief Оператор получения типа URI
 *
 * @return тип URI
 */
awh::Uniform_Resource_Identifier::operator awh::Uniform_Resource_Identifier::type_t() const noexcept {
	// Выводим тип URI
	return this->type();
}
/**
 * @brief Оператор генерации строки URI
 *
 * @return строка URI
 */
awh::Uniform_Resource_Identifier::operator string() const noexcept {
	// Выводим строку URI в кратком формате
	return this->print();
}
/**
 * @brief Оператор получения параметров пользователя URI
 *
 * @return параметры пользователя URI
 */
awh::Uniform_Resource_Identifier::operator awh::Uniform_Resource_Identifier::user_t() const noexcept {
	// Выводим параметры пользователя URI
	return this->user();
}
/**
 * @brief Оператор получения атрибутов URI
 *
 * @return атрибуты URI
 */
awh::Uniform_Resource_Identifier::operator const awh::net::attr_t * () const noexcept {
	// Выводим атрибуты URI
	return this->attr();
}
/**
 * @brief Оператор получения пути URI
 *
 * @return путь URI
 */
awh::Uniform_Resource_Identifier::operator const vector <string> & () const noexcept {
	// Выводим путь URI
	return this->path();
}
/**
 * @brief Оператор получения параметров URI
 *
 * @return параметры URI
 */
awh::Uniform_Resource_Identifier::operator const unordered_map <string, string> & () const noexcept {
	// Выводим параметры URI
	return this->query();
}
/**
 * @brief Оператор сравнения
 *
 * @param uri параметры URI для сравнения
 * @return    результат сравнения
 */
bool awh::Uniform_Resource_Identifier::operator == (const Uniform_Resource_Identifier & uri) noexcept {
	// Результат работы функции
	bool result = true;
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		// Выполняем сравнение типов URI
		result = (this->_type == uri._type);
		// Если типы URI равны
		if(result){
			// Выполняем сравнение размеров схем URI
			result = (this->_scheme.size() == uri._scheme.size());
			// Если схемы URI равны
			if(result)
				// Выполняем сравнение схем URI
				result = this->_fmk->compare(this->_scheme, uri._scheme);
		}
		// Если типы URI равны
		if(result){
			// Выполняем сравнение размеров якорей URI
			result = (this->_fragment.size() == uri._fragment.size());
			// Если якоря URI равны
			if(result)
				// Выполняем сравнение якорей URI
				result = this->_fmk->compare(this->_fragment, uri._fragment);
		}
		// Если типы URI равны
		if(result){
			// Выполняем сравнение размеров логинов пользователя URI
			result = (this->_user.login.size() == uri._user.login.size());
			// Если параметры пользователя URI равны
			if(result)
				// Выполняем сравнение параметров пользователя URI
				result = this->_fmk->compare(this->_user.login, uri._user.login);
		}
		// Если типы URI равны
		if(result){
			// Выполняем сравнение размеров параметров пользователя URI
			result = (this->_user.pass.size() == uri._user.pass.size());
			// Если параметры пользователя URI равны
			if(result)
				// Выполняем сравнение параметров пользователя URI
				result = this->_fmk->compare(this->_user.pass, uri._user.pass);
		}
		// Если типы URI равны
		if(result){
			// Выполняем сравнение атрибутов URI
			result = (((this->_attr != nullptr) && (uri._attr != nullptr)) || ((this->_attr == nullptr) && (uri._attr == nullptr)));
			// Если атрибуты URI равны
			if(result){
				// Выполняем сравнение типов атрибутов URI
				if((this->_attr != nullptr) && (uri._attr != nullptr)){
					// Выполняем сравнение типов атрибутов URI
					result = (this->_attr->type == uri._attr->type);
					// Если типы атрибутов URI равны
					if(result){
						/**
						 * Определяем тип атрибутов URI адреса
						 */
						switch(static_cast <uint8_t> (uri._attr->type)){
							// Если атрибуты URI адреса являются адресом файловой системы
							case static_cast <uint8_t> (net::type_t::FS):
								// Выполняем сравнение адресов файловой системы в атрибутах URI адреса
								result = this->_fmk->compare(awh_cast <net::addr_fs_t *> (awh_cast <net::attr_uds_t *> (this->_attr.get())->path.get())->address, awh_cast <const net::addr_fs_t *> (awh_cast <const net::attr_uds_t *> (uri._attr.get())->path.get())->address);
							break;
							// Если атрибуты URI адреса являются FQDN-адресом
							case static_cast <uint8_t> (net::type_t::FQDN): {
								// Выполняем сравнение портов хоста в атрибутах URI адреса
								result = (awh_cast <net::attr_fqdn_t *> (this->_attr.get())->port == awh_cast <const net::attr_fqdn_t *> (uri._attr.get())->port);
								// Если порты хоста в атрибутах URI адреса равны
								if(result)
									// Выполняем сравнение доменов в атрибутах URI адреса
									result = this->_fmk->compare(awh_cast <net::attr_fqdn_t *> (this->_attr.get())->domain, awh_cast <const net::attr_fqdn_t *> (uri._attr.get())->domain);
							} break;
							// Если атрибуты URI адреса являются IPv4-адресом
							case static_cast <uint8_t> (net::type_t::IPV4): {
								// Выполняем сравнение портов хоста в атрибутах URI адреса
								result = (awh_cast <net::attr_net_t *> (this->_attr.get())->port == awh_cast <const net::attr_net_t *> (uri._attr.get())->port);
								// Если порты хоста в атрибутах URI адреса равны
								if(result)
									// Выполняем сравнение IP-адресов хоста в атрибутах URI адреса
									result = (awh_cast <net::addr_net_ipv4_t *> (awh_cast <net::attr_net_t *> (this->_attr.get())->ip.get())->address == awh_cast <const net::addr_net_ipv4_t *> (awh_cast <const net::attr_net_t *> (uri._attr.get())->ip.get())->address);
							} break;
							// Если атрибуты URI адреса являются IPv6-адресом
							case static_cast <uint8_t> (net::type_t::IPV6): {
								// Выполняем сравнение портов хоста в атрибутах URI адреса
								result = (awh_cast <net::attr_net_t *> (this->_attr.get())->port == awh_cast <const net::attr_net_t *> (uri._attr.get())->port);
								// Если порты хоста в атрибутах URI адреса равны
								if(result)
									// Выполняем сравнение IP-адресов хоста в атрибутах URI адреса
									result = (::memcmp(&awh_cast <net::addr_net_ipv6_t *> (awh_cast <net::attr_net_t *> (this->_attr.get())->ip.get())->address[0], &awh_cast <const net::addr_net_ipv6_t *> (awh_cast <const net::attr_net_t *> (uri._attr.get())->ip.get())->address[0], 16) == 0);
							} break;
						}
					}
				}
			}
		}
		// Если типы URI равны
		if(result){
			// Выполняем сравнение размеров путей URI
			result = (this->_path.size() == uri._path.size());
			// Если размеры путей URI равны
			if(result && !this->_path.empty()){
				// Выполняем сравнение путей URI
				for(size_t i = 0; i < this->_path.size(); ++i){
					// Выполняем сравнение сегментов путей URI
					if(!(result = this->_fmk->compare(this->_path[i], uri._path[i])))
						// Если сегменты путей URI не равны, то прекращаем сравнение
						break;
				}
			}
			// Если пути URI равны
			if(result){
				// Выполняем сравнение размеров путей URI
				result = (this->_path.size() == uri._path.size());
				// Если пути URI равны
				if(result && !this->_query.empty()){
					// Выполняем сравнение размеров параметров URI
					for(auto & [key, value] : this->_query){
						// Выполняем сравнение параметров URI
						if(!(result = ((uri._query.find(key) != uri._query.end()) && this->_fmk->compare(uri._query.at(key), value))))
							// Если параметры URI не равны, то прекращаем сравнение
							break;
					}
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
	// Выводим результат
	return result;
}
/**
 * @brief Оператор неравенства
 *
 * @param uri параметры URI для сравнения
 * @return    результат сравнения
 */
bool awh::Uniform_Resource_Identifier::operator != (const Uniform_Resource_Identifier & uri) noexcept {
	// Результат работы функции
	bool result = true;
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		// Выполняем сравнение типов URI
		result = (this->_type != uri._type);
		// Если типы URI равны
		if(!result){
			// Выполняем сравнение размеров схем URI
			result = (this->_scheme.size() != uri._scheme.size());
			// Если схемы URI равны
			if(!result)
				// Выполняем сравнение схем URI
				result = !this->_fmk->compare(this->_scheme, uri._scheme);
		}
		// Если типы URI равны
		if(!result){
			// Выполняем сравнение размеров якорей URI
			result = (this->_fragment.size() != uri._fragment.size());
			// Если якоря URI равны
			if(!result)
				// Выполняем сравнение якорей URI
				result = !this->_fmk->compare(this->_fragment, uri._fragment);
		}
		// Если типы URI равны
		if(!result){
			// Выполняем сравнение размеров логинов пользователя URI
			result = (this->_user.login.size() != uri._user.login.size());
			// Если параметры пользователя URI равны
			if(!result)
				// Выполняем сравнение параметров пользователя URI
				result = !this->_fmk->compare(this->_user.login, uri._user.login);
		}
		// Если типы URI равны
		if(!result){
			// Выполняем сравнение размеров параметров пользователя URI
			result = (this->_user.pass.size() != uri._user.pass.size());
			// Если параметры пользователя URI равны
			if(!result)
				// Выполняем сравнение параметров пользователя URI
				result = !this->_fmk->compare(this->_user.pass, uri._user.pass);
		}
		// Если типы URI равны
		if(!result){
			// Выполняем сравнение атрибутов URI
			result = (((this->_attr != nullptr) && (uri._attr == nullptr)) || ((this->_attr == nullptr) && (uri._attr != nullptr)));
			// Если атрибуты URI равны
			if(!result){
				// Выполняем сравнение типов атрибутов URI
				if((this->_attr != nullptr) && (uri._attr != nullptr)){
					// Выполняем сравнение типов атрибутов URI
					result = (this->_attr->type != uri._attr->type);
					// Если типы атрибутов URI равны
					if(!result){
						/**
						 * Определяем тип атрибутов URI адреса
						 */
						switch(static_cast <uint8_t> (uri._attr->type)){
							// Если атрибуты URI адреса являются адресом файловой системы
							case static_cast <uint8_t> (net::type_t::FS):
								// Выполняем сравнение адресов файловой системы в атрибутах URI адреса
								result = !this->_fmk->compare(awh_cast <net::addr_fs_t *> (awh_cast <net::attr_uds_t *> (this->_attr.get())->path.get())->address, awh_cast <const net::addr_fs_t *> (awh_cast <const net::attr_uds_t *> (uri._attr.get())->path.get())->address);
							break;
							// Если атрибуты URI адреса являются FQDN-адресом
							case static_cast <uint8_t> (net::type_t::FQDN): {
								// Выполняем сравнение портов хоста в атрибутах URI адреса
								result = (awh_cast <net::attr_fqdn_t *> (this->_attr.get())->port != awh_cast <const net::attr_fqdn_t *> (uri._attr.get())->port);
								// Если порты хоста в атрибутах URI адреса равны
								if(!result)
									// Выполняем сравнение доменов в атрибутах URI адреса
									result = !this->_fmk->compare(awh_cast <net::attr_fqdn_t *> (this->_attr.get())->domain, awh_cast <const net::attr_fqdn_t *> (uri._attr.get())->domain);
							} break;
							// Если атрибуты URI адреса являются IPv4-адресом
							case static_cast <uint8_t> (net::type_t::IPV4): {
								// Выполняем сравнение портов хоста в атрибутах URI адреса
								result = (awh_cast <net::attr_net_t *> (this->_attr.get())->port != awh_cast <const net::attr_net_t *> (uri._attr.get())->port);
								// Если порты хоста в атрибутах URI адреса равны
								if(!result)
									// Выполняем сравнение IP-адресов хоста в атрибутах URI адреса
									result = (awh_cast <net::addr_net_ipv4_t *> (awh_cast <net::attr_net_t *> (this->_attr.get())->ip.get())->address != awh_cast <const net::addr_net_ipv4_t *> (awh_cast <const net::attr_net_t *> (uri._attr.get())->ip.get())->address);
							} break;
							// Если атрибуты URI адреса являются IPv6-адресом
							case static_cast <uint8_t> (net::type_t::IPV6): {
								// Выполняем сравнение портов хоста в атрибутах URI адреса
								result = (awh_cast <net::attr_net_t *> (this->_attr.get())->port != awh_cast <const net::attr_net_t *> (uri._attr.get())->port);
								// Если порты хоста в атрибутах URI адреса равны
								if(!result)
									// Выполняем сравнение IP-адресов хоста в атрибутах URI адреса
									result = (::memcmp(&awh_cast <net::addr_net_ipv6_t *> (awh_cast <net::attr_net_t *> (this->_attr.get())->ip.get())->address[0], &awh_cast <const net::addr_net_ipv6_t *> (awh_cast <const net::attr_net_t *> (uri._attr.get())->ip.get())->address[0], 16) != 0);
							} break;
						}
					}
				}
			}
		}
		// Если типы URI равны
		if(!result){
			// Выполняем сравнение размеров путей URI
			result = (this->_path.size() != uri._path.size());
			// Если размеры путей URI равны
			if(!result && !this->_path.empty()){
				// Выполняем сравнение путей URI
				for(size_t i = 0; i < this->_path.size(); ++i){
					// Выполняем сравнение сегментов путей URI
					if((result = !this->_fmk->compare(this->_path[i], uri._path[i])))
						// Если сегменты путей URI не равны, то прекращаем сравнение
						break;
				}
			}
			// Если пути URI равны
			if(!result){
				// Выполняем сравнение размеров путей URI
				result = (this->_path.size() != uri._path.size());
				// Если пути URI равны
				if(!result && !this->_query.empty()){
					// Выполняем сравнение размеров параметров URI
					for(auto & [key, value] : this->_query){
						// Выполняем сравнение параметров URI
						if((result = ((uri._query.find(key) == uri._query.end()) || !this->_fmk->compare(uri._query.at(key), value))))
							// Если параметры URI не равны, то прекращаем сравнение
							break;
					}
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
	// Выводим результат
	return result;
}
/**
 * @brief Оператор парсинга URI-запроса
 *
 * @param uri строка URI-запроса для получения параметров
 * @return    текущий объект
 */
awh::Uniform_Resource_Identifier & awh::Uniform_Resource_Identifier::operator = (string_view uri) noexcept {
	// Парсим URI-запрос
	this->parse(uri);
	// Выводим результат
	return (* this);
}
/**
 * @brief Оператор  установки параметров пользователя URI
 *
 * @param user параметры пользователя URI для установки
 * @return     текущий объект
 */
awh::Uniform_Resource_Identifier & awh::Uniform_Resource_Identifier::operator = (const user_t & user) noexcept {
	// Устанавливаем параметры пользователя URI
	this->user(user);
	// Выводим результат
	return (* this);
}
/**
 * @brief Оператор установки атрибутов URI
 *
 * @param attr атрибуты URI для установки
 * @return     текущий объект
 */
awh::Uniform_Resource_Identifier & awh::Uniform_Resource_Identifier::operator = (const net::attr_t * attr) noexcept {
	// Устанавливаем атрибуты URI
	this->attr(attr);
	// Выводим результат
	return (* this);
}
/**
 * @brief Оператор установки пути URI
 *
 * @param path путь URI для установки
 * @return     текущий объект
 */
awh::Uniform_Resource_Identifier & awh::Uniform_Resource_Identifier::operator = (const vector <string> & path) noexcept {
	// Устанавливаем путь URI
	this->path(path);
	// Выводим результат
	return (* this);
}
/**
 * @brief Оператор установки параметров URI
 *
 * @param query параметры URI для установки
 * @return      текущий объект
 */
awh::Uniform_Resource_Identifier & awh::Uniform_Resource_Identifier::operator = (const unordered_map <string, string> & query) noexcept {
	// Устанавливаем параметры URI
	this->query(query);
	// Выводим результат
	return (* this);

}
/**
 * @brief Оператор [=] перемещения параметров URI
 *
 * @param uri объект URI для получения параметров
 * @return    параметры URI
 */
awh::Uniform_Resource_Identifier & awh::Uniform_Resource_Identifier::operator = (awh::Uniform_Resource_Identifier && uri) noexcept {
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		// Устанавливаем тип URI
		this->_type = uri._type;
		// Перемещаем атрибуты URI
		this->_attr = ::move(uri._attr);
		// Перемещаем хост URI
		this->_path = ::move(uri._path);
		// Перемещаем параметры URI
		this->_query = ::move(uri._query);
		// Перемещаем схему URI
		this->_scheme = ::move(uri._scheme);
		// Перемещаем якорь URI
		this->_fragment = ::move(uri._fragment);
		// Перемещаем параметры пользователя URI
		this->_user.pass = ::move(uri._user.pass);
		// Перемещаем логин пользователя URI
		this->_user.login = ::move(uri._user.login);
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
	// Выводим результат
	return (* this);
}
/**
 * @brief Оператор [=] присванивания параметров URI
 *
 * @param uri объект URI для получения параметров
 * @return    параметры URI
 */
awh::Uniform_Resource_Identifier & awh::Uniform_Resource_Identifier::operator = (const awh::Uniform_Resource_Identifier & uri) noexcept {
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		// Устанавливаем тип URI
		this->_type = uri._type;
		// Копируем хост URI
		this->_path = uri._path;
		// Копируем параметры URI
		this->_query = uri._query;
		// Копируем схему URI
		this->_scheme = uri._scheme;
		// Копируем якорь URI
		this->_fragment = uri._fragment;
		// Копируем параметры пользователя URI
		this->_user.pass = uri._user.pass;
		// Копируем логин пользователя URI
		this->_user.login = uri._user.login;
		// Если атрибуты URI не пустые
		if(uri._attr != nullptr){
			/**
			 * Определяем тип атрибутов URI адреса
			 */
			switch(static_cast <uint8_t> (uri._attr->type)){
				// Если атрибуты URI адреса являются адресом файловой системы
				case static_cast <uint8_t> (net::type_t::FS): {
					// Если атрибуты URI не инициализированы
					if(this->_attr == nullptr)
						// Инициализируем атрибуты URI
						this->_attr = make_unique <net::attr_uds_t> ();
					// Получаем атрибуты URI адреса
					net::attr_uds_t * attr = awh_cast <net::attr_uds_t *> (this->_attr.get());
					// Если путь к сокету в атрибутах URI адреса не инициализирован
					if(attr->path == nullptr)
						// Инициализируем путь к сокету в атрибутах URI адреса
						attr->path = make_unique <net::addr_fs_t> ();
					// Копируем путь к сокету из атрибутов URI адреса
					awh_cast <net::addr_fs_t *> (attr->path.get())->address = awh_cast <const net::addr_fs_t *> (awh_cast <const net::attr_uds_t *> (uri._attr.get())->path.get())->address;
				} break;
				// Если атрибуты URI адреса являются FQDN-адресом
				case static_cast <uint8_t> (net::type_t::FQDN): {
					// Если атрибуты URI не инициализированы
					if(this->_attr == nullptr)
						// Инициализируем атрибуты URI
						this->_attr = make_unique <net::attr_fqdn_t> ();
					// Получаем атрибуты URI адреса
					net::attr_fqdn_t * attr = awh_cast <net::attr_fqdn_t *> (this->_attr.get());
					// Копируем порт хоста из атрибутов URI адреса
					attr->port = awh_cast <const net::attr_fqdn_t *> (uri._attr.get())->port;
					// Копируем доменное имя хоста из атрибутов URI адреса
					attr->domain = awh_cast <const net::attr_fqdn_t *> (uri._attr.get())->domain;
				} break;
				// Если атрибуты URI адреса являются IPv4-адресом
				case static_cast <uint8_t> (net::type_t::IPV4): {
					// Если атрибуты URI не инициализированы
					if(this->_attr == nullptr)
						// Инициализируем атрибуты URI
						this->_attr = make_unique <net::attr_net_t> ();
					// Получаем атрибуты URI адреса
					net::attr_net_t * attr = awh_cast <net::attr_net_t *> (this->_attr.get());
					// Если IP-адрес хоста в атрибутах URI адреса не инициализирован
					if(attr->ip == nullptr)
						// Инициализируем IP-адрес хоста в атрибутах URI адреса
						attr->ip = make_unique <net::addr_net_ipv4_t> ();
					// Копируем порт хоста из атрибутов URI адреса
					attr->port = awh_cast <const net::attr_net_t *> (uri._attr.get())->port;
					// Копируем IP-адрес хоста из атрибутов URI адреса
					awh_cast <net::addr_net_ipv4_t *> (attr->ip.get())->address = awh_cast <const net::addr_net_ipv4_t *> (awh_cast <const net::attr_net_t *> (uri._attr.get())->ip.get())->address;
				} break;
				// Если атрибуты URI адреса являются IPv6-адресом
				case static_cast <uint8_t> (net::type_t::IPV6): {
					// Если атрибуты URI не инициализированы
					if(this->_attr == nullptr)
						// Инициализируем атрибуты URI
						this->_attr = make_unique <net::attr_net_t> ();
					// Получаем атрибуты URI адреса
					net::attr_net_t * attr = awh_cast <net::attr_net_t *> (this->_attr.get());
					// Если IP-адрес хоста в атрибутах URI адреса не инициализирован
					if(attr->ip == nullptr)
						// Инициализируем IP-адрес хоста в атрибутах URI адреса
						attr->ip = make_unique <net::addr_net_ipv6_t> ();
					// Копируем порт хоста из атрибутов URI адреса
					attr->port = awh_cast <const net::attr_net_t *> (uri._attr.get())->port;
					// Копируем IP-адрес хоста из атрибутов URI адреса
					::memcpy(&awh_cast <net::addr_net_ipv6_t *> (attr->ip.get())->address[0], &awh_cast <const net::addr_net_ipv6_t *> (awh_cast <const net::attr_net_t *> (uri._attr.get())->ip.get())->address[0], 16);
				} break;
			}
			// Устанавливаем тип атрибутов URI адреса
			this->_attr->type = uri._attr->type;
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
	// Выводим результат
	return (* this);
}
/**
 * @brief Конструктор перемещения
 *
 * @param uri параметры URI для перемещения
 */
awh::Uniform_Resource_Identifier::Uniform_Resource_Identifier(Uniform_Resource_Identifier && uri) noexcept :
 _type(type_t::NONE), _scheme{""}, _fragment{""}, _addr(nullptr), _attr(nullptr), _fmk(nullptr), _log(nullptr) {
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		// Устанавливаем объект фреймворка
		this->_fmk = uri._fmk;
		// Устанавливаем объект для работы с логами
		this->_log = uri._log;
		// Устанавливаем тип URI
		this->_type = uri._type;
		// Перемещаем атрибуты URI
		this->_attr = ::move(uri._attr);
		// Перемещаем хост URI
		this->_path = ::move(uri._path);
		// Перемещаем параметры URI
		this->_query = ::move(uri._query);
		// Перемещаем схему URI
		this->_scheme = ::move(uri._scheme);
		// Перемещаем якорь URI
		this->_fragment = ::move(uri._fragment);
		// Перемещаем параметры пользователя URI
		this->_user.pass = ::move(uri._user.pass);
		// Перемещаем логин пользователя URI
		this->_user.login = ::move(uri._user.login);
		// Инициализируем объект работы с сетевыми адресами
		this->_addr = make_unique <net_addr_t> (this->_fmk, this->_log);
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
/**
 * @brief Конструктор копирования
 *
 * @param uri параметры URI для копирования
 */
awh::Uniform_Resource_Identifier::Uniform_Resource_Identifier(const Uniform_Resource_Identifier & uri) noexcept :
 _type(type_t::NONE), _scheme{""}, _fragment{""}, _addr(nullptr), _attr(nullptr), _fmk(nullptr), _log(nullptr) {
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		// Устанавливаем объект фреймворка
		this->_fmk = uri._fmk;
		// Устанавливаем объект для работы с логами
		this->_log = uri._log;
		// Устанавливаем тип URI
		this->_type = uri._type;
		// Копируем хост URI
		this->_path = uri._path;
		// Копируем параметры URI
		this->_query = uri._query;
		// Копируем схему URI
		this->_scheme = uri._scheme;
		// Копируем якорь URI
		this->_fragment = uri._fragment;
		// Копируем параметры пользователя URI
		this->_user.pass = uri._user.pass;
		// Копируем логин пользователя URI
		this->_user.login = uri._user.login;
		// Инициализируем объект работы с сетевыми адресами
		this->_addr = make_unique <net_addr_t> (this->_fmk, this->_log);
		// Если атрибуты URI не пустые
		if(uri._attr != nullptr){
			/**
			 * Определяем тип атрибутов URI адреса
			 */
			switch(static_cast <uint8_t> (uri._attr->type)){
				// Если атрибуты URI адреса являются адресом файловой системы
				case static_cast <uint8_t> (net::type_t::FS): {
					// Если атрибуты URI не инициализированы
					if(this->_attr == nullptr)
						// Инициализируем атрибуты URI
						this->_attr = make_unique <net::attr_uds_t> ();
					// Получаем атрибуты URI адреса
					net::attr_uds_t * attr = awh_cast <net::attr_uds_t *> (this->_attr.get());
					// Если путь к сокету в атрибутах URI адреса не инициализирован
					if(attr->path == nullptr)
						// Инициализируем путь к сокету в атрибутах URI адреса
						attr->path = make_unique <net::addr_fs_t> ();
					// Копируем путь к сокету из атрибутов URI адреса
					awh_cast <net::addr_fs_t *> (attr->path.get())->address = awh_cast <const net::addr_fs_t *> (awh_cast <const net::attr_uds_t *> (uri._attr.get())->path.get())->address;
				} break;
				// Если атрибуты URI адреса являются FQDN-адресом
				case static_cast <uint8_t> (net::type_t::FQDN): {
					// Если атрибуты URI не инициализированы
					if(this->_attr == nullptr)
						// Инициализируем атрибуты URI
						this->_attr = make_unique <net::attr_fqdn_t> ();
					// Получаем атрибуты URI адреса
					net::attr_fqdn_t * attr = awh_cast <net::attr_fqdn_t *> (this->_attr.get());
					// Копируем порт хоста из атрибутов URI адреса
					attr->port = awh_cast <const net::attr_fqdn_t *> (uri._attr.get())->port;
					// Копируем доменное имя хоста из атрибутов URI адреса
					attr->domain = awh_cast <const net::attr_fqdn_t *> (uri._attr.get())->domain;
				} break;
				// Если атрибуты URI адреса являются IPv4-адресом
				case static_cast <uint8_t> (net::type_t::IPV4): {
					// Если атрибуты URI не инициализированы
					if(this->_attr == nullptr)
						// Инициализируем атрибуты URI
						this->_attr = make_unique <net::attr_net_t> ();
					// Получаем атрибуты URI адреса
					net::attr_net_t * attr = awh_cast <net::attr_net_t *> (this->_attr.get());
					// Если IP-адрес хоста в атрибутах URI адреса не инициализирован
					if(attr->ip == nullptr)
						// Инициализируем IP-адрес хоста в атрибутах URI адреса
						attr->ip = make_unique <net::addr_net_ipv4_t> ();
					// Копируем порт хоста из атрибутов URI адреса
					attr->port = awh_cast <const net::attr_net_t *> (uri._attr.get())->port;
					// Копируем IP-адрес хоста из атрибутов URI адреса
					awh_cast <net::addr_net_ipv4_t *> (attr->ip.get())->address = awh_cast <const net::addr_net_ipv4_t *> (awh_cast <const net::attr_net_t *> (uri._attr.get())->ip.get())->address;
				} break;
				// Если атрибуты URI адреса являются IPv6-адресом
				case static_cast <uint8_t> (net::type_t::IPV6): {
					// Если атрибуты URI не инициализированы
					if(this->_attr == nullptr)
						// Инициализируем атрибуты URI
						this->_attr = make_unique <net::attr_net_t> ();
					// Получаем атрибуты URI адреса
					net::attr_net_t * attr = awh_cast <net::attr_net_t *> (this->_attr.get());
					// Если IP-адрес хоста в атрибутах URI адреса не инициализирован
					if(attr->ip == nullptr)
						// Инициализируем IP-адрес хоста в атрибутах URI адреса
						attr->ip = make_unique <net::addr_net_ipv6_t> ();
					// Копируем порт хоста из атрибутов URI адреса
					attr->port = awh_cast <const net::attr_net_t *> (uri._attr.get())->port;
					// Копируем IP-адрес хоста из атрибутов URI адреса
					::memcpy(&awh_cast <net::addr_net_ipv6_t *> (attr->ip.get())->address[0], &awh_cast <const net::addr_net_ipv6_t *> (awh_cast <const net::attr_net_t *> (uri._attr.get())->ip.get())->address[0], 16);
				} break;
			}
			// Устанавливаем тип атрибутов URI адреса
			this->_attr->type = uri._attr->type;
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
/**
 * @brief конструктор
 *
 * @param fmk объект фреймворка
 * @param log объект для работы с логами
 */
awh::Uniform_Resource_Identifier::Uniform_Resource_Identifier(const fmk_t * fmk, const log_t * log) noexcept :
 _type(type_t::NONE), _scheme{""}, _fragment{""}, _addr(nullptr), _attr(nullptr), _fmk(fmk), _log(log) {
	// Инициализируем объект работы с сетевыми адресами
	this->_addr = make_unique <net_addr_t> (fmk, log);
}
/**
 * @brief деструктор
 *
 */
awh::Uniform_Resource_Identifier::~Uniform_Resource_Identifier() noexcept {}
/**
 * @brief Оператор [>>] чтения из потока URI
 *
 * @param is  поток для чтения
 * @param uri URI для присвоения
 */
istream & awh::operator >> (istream & is, uri_t & uri) noexcept {
	// Адрес URI в виде строки
	string addr = "";
	// Считываем адрес URI из потока
	is >> addr;
	// Если адрес URI не пустой
	if(!addr.empty())
		// Парсим URI-запрос
		uri.parse(addr);
	// Выводим результат
	return is;
}
/**
 * @brief Оператор [<<] вывода в поток URI
 *
 * @param os  поток куда нужно вывести данные
 * @param uri URI для присвоения
 */
ostream & awh::operator << (ostream & os, const uri_t & uri) noexcept {
	// Записываем в поток URI адрес в виде строки
	os << uri.print();
	// Выводим результат
	return os;
}
