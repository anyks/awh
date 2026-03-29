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
#include <sys/types.h>

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
					 * Иначе : является частью userinfo (например разделитель логина и пароля), продолжаем чтение хоста
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
						// Обработать разделитель в новом состоянии
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
						// Обработать разделитель в новом состоянии
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
					/**
					 * Не трогаем символы, разрешённые по RFC 3986 без кодирования:
					 * незарезервированные символы: буквенно-цифровые ASCII и - _ . ~
					 * Используем явные диапазонные проверки (locale-независимые),
					 * а не isalnum(), которая на UTF-8 локали трактует байты >127 как буквы.
					 * Символ @ является «gen-delim» и должен быть закодирован.
					 */
					if(((letter >= 'a') && (letter <= 'z')) ||
					   ((letter >= 'A') && (letter <= 'Z')) ||
					   ((letter >= '0') && (letter <= '9')) ||
					   (letter == '-') || (letter == '_') || (letter == '.') || (letter == '~')){
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
	// Очищаем путь URI
	this->_path.clear();
	// Очищаем параметры URI
	this->_query.clear();
	// Очищаем схему URI
	this->_scheme.clear();
	// Очищаем якорь URI
	this->_fragment.clear();
	// Очищаем логин пользователя URI
	this->_user.login.clear();
	// Очищаем пароль пользователя URI
	this->_user.pass.clear();
	// Сбрасываем тип URI
	this->_type = type_t::NONE;
	// Сбрасываем атрибуты URI
	this->_attr.reset(nullptr);
}
/**
 * @brief Метод проверки на существование данных
 *
 * @return результат проверки
 */
bool awh::Uniform_Resource_Identifier::empty() const noexcept {
	// Проверяем, что все компоненты URI пустые
	return (
		this->_path.empty() &&
		this->_query.empty() &&
		this->_scheme.empty() &&
		this->_fragment.empty() &&
		this->_user.pass.empty() &&
		this->_user.login.empty() &&
		(this->_attr == nullptr)
	);
}
/**
 * @brief Метод получения типа URI
 *
 * @return тип URI
 */
awh::Uniform_Resource_Identifier::type_t awh::Uniform_Resource_Identifier::type() const noexcept {
	// Выводим тип URI
	return this->_type;
}
/**
 * @brief Метод получения схемы URI
 *
 * @return схема URI
 */
const string & awh::Uniform_Resource_Identifier::scheme() const noexcept {
	// Выводим схему URI
	return this->_scheme;
}
/**
 * @brief Метод установки схемы URI
 *
 * @param scheme схема URI для установки
 */
void awh::Uniform_Resource_Identifier::scheme(string_view scheme) noexcept {
	// Устанавливаем схему URI
	this->_scheme = scheme;
}
/**
 * @brief Метод получения параметров пользователя URI
 *
 * @return параметры пользователя URI
 */
const awh::Uniform_Resource_Identifier::user_t & awh::Uniform_Resource_Identifier::user() const noexcept {
	// Выводим параметры пользователя URI
	return this->_user;
}
/**
 * @brief Метод установки параметров пользователя URI
 *
 * @param user параметры пользователя URI для установки
 */
void awh::Uniform_Resource_Identifier::user(const user_t & user) noexcept {
	// Устанавливаем параметры пользователя URI
	this->_user = user;
}
/**
 * @brief Метод установки логина и пароля пользователя URI
 *
 * @param login логин пользователя URI для установки
 * @param pass  пароль пользователя URI для установки
 */
void awh::Uniform_Resource_Identifier::user(string_view login, string_view pass) noexcept {
	// Устанавливаем пароль пользователя URI
	this->_user.pass = pass;
	// Устанавливаем логин пользователя URI
	this->_user.login = login;
}
/**
 * @brief Метод получения якоря URI
 *
 * @return якорь URI
 */
const string & awh::Uniform_Resource_Identifier::fragment() const noexcept {
	// Выводим якорь URI
	return this->_fragment;
}
/**
 * @brief Метод установки якоря URI
 *
 * @param fragment якорь URI для установки
 */
void awh::Uniform_Resource_Identifier::fragment(string_view fragment) noexcept {
	// Устанавливаем якорь URI
	this->_fragment = fragment;
}
/**
 * @brief Метод получения атрибутов URI
 *
 * @return атрибуты URI
 */
const awh::net::attr_t * awh::Uniform_Resource_Identifier::attr() const noexcept {
	// Выводим атрибуты URI
	return this->_attr.get();
}
/**
 * @brief Метод установки атрибутов URI
 *
 * @param attr атрибуты URI для установки
 */
void awh::Uniform_Resource_Identifier::attr(const net::attr_t * attr) noexcept {
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		// Если атрибуты URI не пустые
		if(attr != nullptr){
			/**
			 * Определяем тип атрибутов URI адреса
			 */
			switch(static_cast <uint8_t> (attr->type)){
				// Если атрибуты URI адреса являются адресом файловой системы
				case static_cast <uint8_t> (net::type_t::FS): {
					// Если атрибуты URI не инициализированы
					if(this->_attr == nullptr)
						// Инициализируем атрибуты URI
						this->_attr = make_unique <net::attr_uds_t> ();
					// Если путь к сокету в атрибутах URI адреса не инициализирован
					if(awh_cast <net::attr_uds_t *> (this->_attr.get())->path == nullptr)
						// Инициализируем путь к сокету в атрибутах URI адреса
						awh_cast <net::attr_uds_t *> (this->_attr.get())->path = make_unique <net::addr_fs_t> ();
					// Копируем путь к сокету из атрибутов URI адреса
					awh_cast <net::addr_fs_t *> (awh_cast <net::attr_uds_t *> (this->_attr.get())->path.get())->address = awh_cast <const net::addr_fs_t *> (awh_cast <const net::attr_uds_t *> (attr)->path.get())->address;
				} break;
				// Если атрибуты URI адреса являются FQDN-адресом
				case static_cast <uint8_t> (net::type_t::FQDN): {
					// Если атрибуты URI не инициализированы
					if(this->_attr == nullptr)
						// Инициализируем атрибуты URI
						this->_attr = make_unique <net::attr_fqdn_t> ();
					// Копируем порт хоста из атрибутов URI адреса
					awh_cast <net::attr_fqdn_t *> (this->_attr.get())->port = awh_cast <const net::attr_fqdn_t *> (attr)->port;
					// Копируем доменное имя хоста из атрибутов URI адреса
					awh_cast <net::attr_fqdn_t *> (this->_attr.get())->domain = awh_cast <const net::attr_fqdn_t *> (attr)->domain;
				} break;
				// Если атрибуты URI адреса являются IPv4-адресом
				case static_cast <uint8_t> (net::type_t::IPV4): {
					// Если атрибуты URI не инициализированы
					if(this->_attr == nullptr)
						// Инициализируем атрибуты URI
						this->_attr = make_unique <net::attr_net_t> ();
					// Если IP-адрес хоста в атрибутах URI адреса не инициализирован
					if(awh_cast <net::attr_net_t *> (this->_attr.get())->ip == nullptr)
						// Инициализируем IP-адрес хоста в атрибутах URI адреса
						awh_cast <net::attr_net_t *> (this->_attr.get())->ip = make_unique <net::addr_net_ipv4_t> ();
					// Копируем порт хоста из атрибутов URI адреса
					awh_cast <net::attr_net_t *> (this->_attr.get())->port = awh_cast <const net::attr_net_t *> (attr)->port;
					// Копируем IP-адрес хоста из атрибутов URI адреса
					awh_cast <net::addr_net_ipv4_t *> (awh_cast <net::attr_net_t *> (this->_attr.get())->ip.get())->address = awh_cast <const net::addr_net_ipv4_t *> (awh_cast <const net::attr_net_t *> (attr)->ip.get())->address;
				} break;
				// Если атрибуты URI адреса являются IPv6-адресом
				case static_cast <uint8_t> (net::type_t::IPV6): {
					// Если атрибуты URI не инициализированы
					if(this->_attr == nullptr)
						// Инициализируем атрибуты URI
						this->_attr = make_unique <net::attr_net_t> ();
					// Если IP-адрес хоста в атрибутах URI адреса не инициализирован
					if(awh_cast <net::attr_net_t *> (this->_attr.get())->ip == nullptr)
						// Инициализируем IP-адрес хоста в атрибутах URI адреса
						awh_cast <net::attr_net_t *> (this->_attr.get())->ip = make_unique <net::addr_net_ipv6_t> ();
					// Копируем порт хоста из атрибутов URI адреса
					awh_cast <net::attr_net_t *> (this->_attr.get())->port = awh_cast <const net::attr_net_t *> (attr)->port;
					// Копируем IP-адрес хоста из атрибутов URI адреса
					::memcpy(&awh_cast <net::addr_net_ipv6_t *> (awh_cast <net::attr_net_t *> (this->_attr.get())->ip.get())->address[0], &awh_cast <const net::addr_net_ipv6_t *> (awh_cast <const net::attr_net_t *> (attr)->ip.get())->address[0], 16);
				} break;
			}
			// Устанавливаем тип атрибутов URI адреса
			this->_attr->type = attr->type;
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
 * @brief Метод получения хоста URI
 *
 * @return хост URI
 */
string awh::Uniform_Resource_Identifier::host() const noexcept {
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		// Если атрибуты URI не пустые
		if(this->_attr != nullptr){
			/**
			 * Определяем тип атрибутов URI адреса
			 */
			switch(static_cast <uint8_t> (this->_attr->type)){
				// Если атрибуты URI адреса являются адресом файловой системы
				case static_cast <uint8_t> (net::type_t::FS):
					// Выводим путь к сокету из атрибутов URI адреса
					return awh_cast <net::addr_fs_t *> (awh_cast <net::attr_uds_t *> (this->_attr.get())->path.get())->address;
				// Если атрибуты URI адреса являются FQDN-адресом
				case static_cast <uint8_t> (net::type_t::FQDN):
					// Выводим доменное имя хоста из атрибутов URI адреса
					return awh_cast <net::attr_fqdn_t *> (this->_attr.get())->domain;
				// Если атрибуты URI адреса являются IPv4-адресом
				case static_cast <uint8_t> (net::type_t::IPV4):
				// Если атрибуты URI адреса являются IPv6-адресом
				case static_cast <uint8_t> (net::type_t::IPV6): {
					// Устанавливаем источник IP-адреса хоста в атрибутах URI адреса
					this->_addr->source(awh_cast <net::attr_net_t *> (this->_attr.get())->ip.get(), net_addr_t::endian_t::LITTLE);
					// Выводим IP-адрес хоста в виде строки
					return static_cast <string> (* this->_addr.get());
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
	// Выводим результат по умолчанию
	return "";
}
/**
 * @brief Метод установки хоста URI
 *
 * @param host хост URI для установки
 */
void awh::Uniform_Resource_Identifier::host(string_view host) noexcept {
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		// Выполняем парсинг переданного адреса
		if(this->_addr->parse(host)){
			// Порт хоста в атрибутах URI адреса
			uint16_t port = 0;
			/**
			 * Определяем тип полученного IP-адреса
			 */
			switch(static_cast <uint8_t> (this->_addr->type())){
				// Если хост URI является IPv4-адресом
				case static_cast <uint8_t> (net_addr_t::type_t::IPV4): {
					// Если атрибуты URI не инициализированы или уже инициализированы, но не являются IPv4-адресом
					if((this->_attr == nullptr) || (this->_attr->type != net::type_t::IPV4)){
						// Инициализируем атрибуты URI
						this->_attr = make_unique <net::attr_net_t> ();
						// Устанавливаем тип атрибутов URI адреса
						this->_attr->type = net::type_t::IPV4;
					// Если атрибуты URI уже инициализированы — сохраняем текущий порт, чтобы не потерять его при обновлении хоста
					} else if(this->_attr != nullptr)
						// Если атрибуты URI адреса уже инициализированы, то сохраняем порт хоста из атрибутов URI адреса
						port = awh_cast <net::attr_net_t *> (this->_attr.get())->port;
				} break;
				// Если хост URI является IPv6-адресом
				case static_cast <uint8_t> (net_addr_t::type_t::IPV6): {
					// Если атрибуты URI не инициализированы или уже инициализированы, но не являются IPv6-адресом
					if((this->_attr == nullptr) || (this->_attr->type != net::type_t::IPV6)){
						// Инициализируем атрибуты URI
						this->_attr = make_unique <net::attr_net_t> ();
						// Устанавливаем тип атрибутов URI адреса
						this->_attr->type = net::type_t::IPV6;
					// Если атрибуты URI уже инициализированы — сохраняем текущий порт, чтобы не потерять его при обновлении хоста
					} else if(this->_attr != nullptr)
						// Если атрибуты URI адреса уже инициализированы, то сохраняем порт хоста из атрибутов URI адреса
						port = awh_cast <net::attr_net_t *> (this->_attr.get())->port;
				} break;
			}
			// Устанавливаем порт хоста в атрибутах URI адреса
			awh_cast <net::attr_net_t *> (this->_attr.get())->port = port;
			// Инициализируем IP-адрес хоста в атрибутах URI адреса
			awh_cast <net::attr_net_t *> (this->_attr.get())->ip = ::move(this->_addr->source(net_addr_t::endian_t::LITTLE));
		// Если парсинг не выполнен
		} else {
			/**
			 * Определяем тип полученного хоста URI адреса
			 */
			switch(static_cast <uint8_t> (this->_addr->host(host))){
				// Если хост URI является файловой системой
				case static_cast <uint8_t> (net_addr_t::type_t::FS): {
					// Если атрибуты URI не инициализированы или уже инициализированы, но не являются FS-адресом
					if((this->_attr == nullptr) || (this->_attr->type != net::type_t::FS)){
						// Инициализируем атрибуты URI
						this->_attr = make_unique <net::attr_uds_t> ();
						// Устанавливаем тип атрибутов URI адреса
						this->_attr->type = net::type_t::FS;
					}
					// Если путь к сокету в атрибутах URI адреса не инициализирован
					if(awh_cast <net::attr_uds_t *> (this->_attr.get())->path == nullptr)
						// Инициализируем путь к сокету в атрибутах URI адреса
						awh_cast <net::attr_uds_t *> (this->_attr.get())->path = make_unique <net::addr_fs_t> ();
					// Копируем путь к сокету из атрибутов URI адреса
					awh_cast <net::addr_fs_t *> (awh_cast <net::attr_uds_t *> (this->_attr.get())->path.get())->address = host;
				} break;
				// Если хост URI является доменным именем
				default: {
					// Порт хоста в атрибутах URI адреса
					uint16_t port = 0;
					// Если атрибуты URI не инициализированы или уже инициализированы, но не являются FQDN-адресом
					if((this->_attr == nullptr) || (this->_attr->type != net::type_t::FQDN)){
						// Инициализируем атрибуты URI
						this->_attr = make_unique <net::attr_fqdn_t> ();
						// Устанавливаем тип атрибутов URI адреса
						this->_attr->type = net::type_t::FQDN;
					// Если атрибуты URI уже инициализированы — сохраняем текущий порт, чтобы не потерять его при обновлении хоста
					} else if(this->_attr != nullptr)
						// Если атрибуты URI адреса уже инициализированы, то сохраняем порт хоста из атрибутов URI адреса
						port = awh_cast <net::attr_fqdn_t *> (this->_attr.get())->port;
					// Устанавливаем порт хоста в атрибутах URI адреса
					awh_cast <net::attr_fqdn_t *> (this->_attr.get())->port = port;
					// Копируем доменное имя хоста из атрибутов URI адреса
					awh_cast <net::attr_fqdn_t *> (this->_attr.get())->domain = host;
				} break;
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
			this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(host), log_t::flag_t::CRITICAL, error.what());
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
 * @brief Метод получения порта URI
 *
 * @return порт URI
 */
uint16_t awh::Uniform_Resource_Identifier::port() const noexcept {
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		// Если атрибуты URI не пустые
		if(this->_attr != nullptr){
			/**
			 * Определяем тип атрибутов URI адреса
			 */
			switch(static_cast <uint8_t> (this->_attr->type)){
				// Если атрибуты URI адреса являются адресом файловой системы
				case static_cast <uint8_t> (net::type_t::FS):
					// Выводим порт хоста из атрибутов URI адреса (для файловой системы порт не используется, поэтому возвращаем 0)
					return 0;
				// Если атрибуты URI адреса являются FQDN-адресом
				case static_cast <uint8_t> (net::type_t::FQDN):
					// Выводим порт хоста из атрибутов URI адреса
					return awh_cast <net::attr_fqdn_t *> (this->_attr.get())->port;
				// Если атрибуты URI адреса являются IPv4-адресом
				case static_cast <uint8_t> (net::type_t::IPV4):
				// Если атрибуты URI адреса являются IPv6-адресом
				case static_cast <uint8_t> (net::type_t::IPV6):
					// Выводим порт хоста из атрибутов URI адреса
					return awh_cast <net::attr_net_t *> (this->_attr.get())->port;
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
	// Выводим результат по умолчанию
	return 0;
}
/**
 * @brief Метод установки порта URI
 *
 * @param port порт URI для установки
 */
void awh::Uniform_Resource_Identifier::port(const uint16_t port) noexcept {
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		// Если атрибуты URI не пустые
		if(this->_attr != nullptr){
			/**
			 * Определяем тип атрибутов URI адреса
			 */
			switch(static_cast <uint8_t> (this->_attr->type)){
				// Если атрибуты URI адреса являются FQDN-адресом
				case static_cast <uint8_t> (net::type_t::FQDN):
					// Устанавливаем порт хоста в атрибутах URI адреса
					awh_cast <net::attr_fqdn_t *> (this->_attr.get())->port = port;
				break;
				// Если атрибуты URI адреса являются IPv4-адресом
				case static_cast <uint8_t> (net::type_t::IPV4):
				// Если атрибуты URI адреса являются IPv6-адресом
				case static_cast <uint8_t> (net::type_t::IPV6):
					// Устанавливаем порт хоста в атрибутах URI адреса
					awh_cast <net::attr_net_t *> (this->_attr.get())->port = port;
				break;
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
			this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(port), log_t::flag_t::CRITICAL, error.what());
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
 * @brief Метод получения пути URI
 *
 * @return путь URI
 */
const vector <string> & awh::Uniform_Resource_Identifier::path() const noexcept {
	// Выводим путь URI
	return this->_path;
}
/**
 * @brief Метод установки пути URI
 *
 * @param path путь URI для установки
 */
void awh::Uniform_Resource_Identifier::path(const vector <string> & path) noexcept {
	// Устанавливаем путь URI
	this->_path = path;
}
/**
 * @brief Метод получения параметров URI
 *
 * @return параметры URI
 */
const unordered_map <string, string> & awh::Uniform_Resource_Identifier::query() const noexcept {
	// Выводим параметры URI
	return this->_query;
}
/**
 * @brief Метод установки параметров URI
 *
 * @param query параметры URI для установки
 */
void awh::Uniform_Resource_Identifier::query(const unordered_map <string, string> & query) noexcept {
	// Устанавливаем параметры URI
	this->_query = query;
}
/**
 * @brief Метод парсинга URI-запроса
 *
 * @param uri строка URI-запроса для получения параметров
 * @return    тип URI
 */
awh::Uniform_Resource_Identifier::type_t awh::Uniform_Resource_Identifier::parse(string_view uri) noexcept {
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		// Результат парсинга URI
		string_view scheme, userinfo, host, port, path, query, fragment;
		// Выполняем парсинг URI
		if(uri::parse(uri, scheme, userinfo, host, port, path, query, fragment)){
			// Если схема URI не пустая
			if(!scheme.empty()){
				// Очищаем все предыдущие данные URI
				this->clear();
				// Устанавливаем схему URI
				this->_scheme = scheme;
				// Если протокол является HTTP
				if(this->_fmk->compare(scheme, "http"))
					// Устанавливаем тип URI как HTTP
					this->_type = type_t::HTTP;
				// Если протокол является HTTPS
				else if(this->_fmk->compare(scheme, "https"))
					// Устанавливаем тип URI как HTTPS
					this->_type = type_t::HTTPS;
				// Если протокол является WS
				else if(this->_fmk->compare(scheme, "ws"))
					// Устанавливаем тип URI как WS
					this->_type = type_t::WS;
				// Если протокол является WSS
				else if(this->_fmk->compare(scheme, "wss"))
					// Устанавливаем тип URI как WSS
					this->_type = type_t::WSS;
				// Если протокол является SSH
				else if(this->_fmk->compare(scheme, "ssh"))
					// Устанавливаем тип URI как SSH
					this->_type = type_t::SSH;
				// Если протокол является FTP
				else if(this->_fmk->compare(scheme, "ftp"))
					// Устанавливаем тип URI как FTP
					this->_type = type_t::FTP;
				// Если протокол является MQTT
				else if(this->_fmk->compare(scheme, "mqtt"))
					// Устанавливаем тип URI как MQTT
					this->_type = type_t::MQTT;
				// Если протокол является REDIS
				else if(this->_fmk->compare(scheme, "redis"))
					// Устанавливаем тип URI как REDIS
					this->_type = type_t::REDIS;
				// Если протокол является E-mail
				else if(this->_fmk->compare(scheme, "mailto"))
					// Устанавливаем тип URI как E-mail
					this->_type = type_t::EMAIL;
				// Если протокол является Socks5
				else if(this->_fmk->compare(scheme, "socks5"))
					// Устанавливаем тип URI как Socks5
					this->_type = type_t::SOCKS5;
				// Если протокол является File
				else if(this->_fmk->compare(scheme, "file"))
					// Устанавливаем тип URI как File
					this->_type = type_t::FILE;
				// Если протокол является Unix Socket
				else if(this->_fmk->compare(scheme, "unix"))
					// Устанавливаем тип URI как Unix Socket
					this->_type = type_t::UDS;
				// Если протокол является MySQL
				else if(this->_fmk->compare(scheme, "mysql"))
					// Устанавливаем тип URI как MySQL
					this->_type = type_t::MYSQL;
				// Если протокол является PostgreSQL
				else if(this->_fmk->compare(scheme, "postgresql"))
					// Устанавливаем тип URI как PostgreSQL
					this->_type = type_t::POSTGRESQL;
				// Если это просто произвольная схема URI, то устанавливаем тип URI как SCHEME
				else this->_type = type_t::SCHEME;
			}
			// Если параметры пользователя URI не пустые
			if(!userinfo.empty()){
				// Выполняем поиск символа ":" в параметрах пользователя URI
				const size_t pos = userinfo.find(':');
				// Если в параметрах пользователя URI есть символ ":", то разделяем логин и пароль
				if(pos != string_view::npos){
					// Устанавливаем логин пользователя URI
					this->_user.login = userinfo.substr(0, pos);
					// Устанавливаем пароль пользователя URI
					this->_user.pass = userinfo.substr(pos + 1);
				// Если в параметрах пользователя URI нет символа ":", то устанавливаем только логин, а пароль оставляем пустым
				} else this->_user.login = userinfo;
			}
			// Если хост URI не пустой
			if(!host.empty())
				// Устанавливаем хост URI
				this->host(host);
			// Если порт URI не пустой и является числом
			if(!port.empty() && this->_fmk->is(port, fmk_t::check_t::NUMBER)){
				// Устанавливаем порт URI, преобразуя строку порта в число
				this->port(this->_fmk->atoi <uint16_t> (port));
				// Если тип URI не определён, то пытаемся определить его по схеме URI
				if(this->_type == type_t::NONE){
					/**
					 * Определяем тип порта URI адреса
					 */
					switch(this->port()){
						// Если порт URI является 443, то это может быть HTTPS
						case 443:
							// Устанавливаем тип URI как HTTPS
							this->_type = type_t::HTTPS;
						break;
						// Если порт URI является 80, то это может быть HTTP
						case 80:
							// Устанавливаем тип URI как HTTP
							this->_type = type_t::HTTP;
						break;
						// Если порт URI является 22, то это может быть SSH
						case 22:
							// Устанавливаем тип URI как SSH
							this->_type = type_t::SSH;
						break;
						// Если порт URI является 21, то это может быть FTP
						case 21:
							// Устанавливаем тип URI как FTP
							this->_type = type_t::FTP;
						break;
						// Если порт URI является 1883, то это может быть MQTT
						case 1883:
							// Устанавливаем тип URI как MQTT
							this->_type = type_t::MQTT;
						break;
						// Если порт URI является 25, то это может быть E-mail
						case 25:
							// Устанавливаем тип URI как E-mail
							this->_type = type_t::EMAIL;
						break;
						// Если порт URI является 1080, то это может быть Socks5
						case 1080:
							// Устанавливаем тип URI как Socks5
							this->_type = type_t::SOCKS5;
						break;
						// Если порт URI является 6379, то это может быть REDIS
						case 6379:
							// Устанавливаем тип URI как REDIS
							this->_type = type_t::REDIS;
						break;
						// Если порт URI является 3306, то это может быть MySQL
						case 3306:
							// Устанавливаем тип URI как MySQL
							this->_type = type_t::MYSQL;
						break;
						// Если порт URI является 5432, то это может быть PostgreSQL
						case 5432:
							// Устанавливаем тип URI как PostgreSQL
							this->_type = type_t::POSTGRESQL;
						break;
					}
				}
			// Если порт пустой и установленный порт URI равен 0
			} else if(port.empty() && (this->port() == 0)) {
				/**
				 * Определяем тип URI адреса по схеме URI, если порт не указан
				 */
				switch(static_cast <uint8_t> (this->_type)){
					// Если тип URI является WS или HTTP, то устанавливаем порт URI как 80
					case static_cast <uint8_t> (type_t::WS):
					case static_cast <uint8_t> (type_t::HTTP):
						// Устанавливаем порт URI как 80
						this->port(80);
					break;
					// Если тип URI является WSS или HTTPS, то устанавливаем порт URI как 443
					case static_cast <uint8_t> (type_t::WSS):
					case static_cast <uint8_t> (type_t::HTTPS):
						// Устанавливаем порт URI как 443
						this->port(443);
					break;
					// Если тип URI является SSH, то устанавливаем порт URI как 22
					case static_cast <uint8_t> (type_t::SSH):
						// Устанавливаем порт URI как 22
						this->port(22);
					break;
					// Если тип URI является FTP, то устанавливаем порт URI как 21
					case static_cast <uint8_t> (type_t::FTP):
						// Устанавливаем порт URI как 21
						this->port(21);
					break;
					// Если тип URI является MQTT, то устанавливаем порт URI как 1883
					case static_cast <uint8_t> (type_t::MQTT):
						// Устанавливаем порт URI как 1883
						this->port(1883);
					break;
					// Если тип URI является E-mail, то устанавливаем порт URI как 25
					case static_cast <uint8_t> (type_t::EMAIL):
						// Устанавливаем порт URI как 25
						this->port(25);
					break;
					// Если тип URI является Socks5, то устанавливаем порт URI как 1080
					case static_cast <uint8_t> (type_t::SOCKS5):
						// Устанавливаем порт URI как 1080
						this->port(1080);
					break;
					// Если тип URI является REDIS, то устанавливаем порт URI как 6379
					case static_cast <uint8_t> (type_t::REDIS):
						// Устанавливаем порт URI как 6379
						this->port(6379);
					break;
					// Если тип URI является MySQL, то устанавливаем порт URI как 3306
					case static_cast <uint8_t> (type_t::MYSQL):
						// Устанавливаем порт URI как 3306
						this->port(3306);
					break;
					// Если тип URI является PostgreSQL, то устанавливаем порт URI как 5432
					case static_cast <uint8_t> (type_t::POSTGRESQL):
						// Устанавливаем порт URI как 5432
						this->port(5432);
					break;
				}
			}
			// Если путь URI не пустой
			if(!path.empty()){
				// Очищаем все предыдущие сегменты пути URI
				this->_path.clear();
				// Разделяем путь URI на сегменты по символу "/"
				string_view segment;
				// Позиция начала и конца сегмента пути URI
				size_t start = 0, end = 0;
				/**
				 * Пока в пути URI есть символ "/", разделяем путь на сегменты и добавляем их в путь URI
				 */
				while((end = path.find('/', start)) != string_view::npos){
					// Получаем сегмент пути URI
					segment = path.substr(start, end - start);
					// Если сегмент пути URI не пустой, то добавляем его в путь URI
					if(!segment.empty())
						// Добавляем сегмент пути URI в путь URI
						this->_path.push_back(uri::decode(segment, this->_log));
					// Обновляем позицию начала следующего сегмента
					start = end + 1;
				}
				// Добавляем последний сегмент пути URI, если он не пустой
				segment = path.substr(start);
				// Если последний сегмент пути URI не пустой, то добавляем его в путь URI
				if(!segment.empty())
					// Добавляем последний сегмент пути URI в путь URI
					this->_path.push_back(uri::decode(segment, this->_log));
			}
			// Если параметры URI не пустые
			if(!query.empty()){
				// Очищаем все предыдущие параметры URI
				this->_query.clear();
				// Разделяем параметры URI на пары ключ-значение по символу "&"
				string_view pair;
				// Позиция начала и конца пары ключ-значение параметров URI
				size_t start = 0, end = 0;
				/**
				 * Пока в параметрах URI есть символ "&", разделяем параметры на пары ключ-значение и добавляем их в параметры URI
				 */
				while((end = query.find('&', start)) != string_view::npos){
					// Получаем пару ключ-значение параметров URI
					pair = query.substr(start, end - start);
					// Если пара ключ-значение параметров URI не пустая, то разделяем её на ключ и значение по символу "="
					if(!pair.empty()){
						// Позиция символа "=" в паре ключ-значение параметров URI
						const size_t pos = pair.find('=');
						// Если в паре ключ-значение параметров URI есть символ "=", то разделяем её на ключ и значение
						if(pos != string_view::npos)
							// Добавляем пару ключ-значение параметров URI в параметры URI
							this->_query.emplace(uri::decode(pair.substr(0, pos), this->_log), uri::decode(pair.substr(pos + 1), this->_log));
						// Если в паре ключ-значение параметров URI нет символа "=", то добавляем её как ключ с пустым значением
						else this->_query.emplace(uri::decode(pair, this->_log), "");
					}
					// Обновляем позицию начала следующей пары ключ-значение параметров URI
					start = (end + 1);
				}
				// Добавляем последнюю пару ключ-значение параметров URI, если она не пустая
				pair = query.substr(start);
				// Если последняя пара ключ-значение параметров URI не пустая, то разделяем её на ключ и значение по символу "=" и добавляем её в параметры URI
				if(!pair.empty()){
					// Позиция символа "=" в последней паре ключ-значение параметров URI
					const size_t pos = pair.find('=');
					// Если в последней паре ключ-значение параметров URI есть символ "=", то разделяем её на ключ и значение
					if(pos != string_view::npos)
						// Добавляем последнюю пару ключ-значение параметров URI в параметры URI
						this->_query.emplace(uri::decode(pair.substr(0, pos), this->_log), uri::decode(pair.substr(pos + 1), this->_log));
					// Если в последней паре ключ-значение параметров URI нет символа "=", то добавляем её как ключ с пустым значением
					else this->_query.emplace(uri::decode(pair, this->_log), "");
				}
			}
			// Если якорь URI не пустой
			if(!fragment.empty())
				// Устанавливаем якорь URI
				this->_fragment = uri::decode(fragment, this->_log);
			// Если якорь URI пустой, то очищаем его
			else this->_fragment.clear();
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
	return this->_type;
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
 * @brief Метод генерации строки URI
 *
 * @param item   режим элемента URI для генерации
 * @param format режим формата URI для генерации
 * @return       строка URI
 */
string awh::Uniform_Resource_Identifier::print(const item_t item, const format_t format) const noexcept {
	// Результат работы функции
	string result = "";
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		/**
		 * Определяем режим элемента URI для генерации
		 */
		switch(static_cast <uint8_t> (item)){
			// Если режим элемента URI для генерации является полным, то генерируем полный URI
			case static_cast <uint8_t> (item_t::URI): {
				// Если схема URI не пустая, то добавляем её в результат
				if(!this->_scheme.empty()){
					/**
					 * Определяем тип URI адреса по схеме URI
					 */
					switch(static_cast <uint8_t> (this->_type)){
						// Если тип URI является WS, WSS, FTP, UDS, FILE, MQTT, REDIS, MySQL, SOCKS5, PostgreSQL, HTTP или HTTPS, то добавляем схему URI в результат с "://"
						case static_cast <uint8_t> (type_t::WS):
						case static_cast <uint8_t> (type_t::WSS):
						case static_cast <uint8_t> (type_t::FTP):
						case static_cast <uint8_t> (type_t::UDS):
						case static_cast <uint8_t> (type_t::FILE):
						case static_cast <uint8_t> (type_t::MQTT):
						case static_cast <uint8_t> (type_t::HTTP):
						case static_cast <uint8_t> (type_t::HTTPS):
						case static_cast <uint8_t> (type_t::REDIS):
						case static_cast <uint8_t> (type_t::MYSQL):
						case static_cast <uint8_t> (type_t::SOCKS5):
						case static_cast <uint8_t> (type_t::POSTGRESQL):
							// Добавляем схему URI в результат
							result.append(this->_fmk->format("%s://", this->_scheme.c_str()));
						break;
						// Если тип URI является SSH, E-mail или Scheme, то добавляем схему URI в результат без "://"
						case static_cast <uint8_t> (type_t::SSH):
						case static_cast <uint8_t> (type_t::EMAIL):
						case static_cast <uint8_t> (type_t::SCHEME):
							// Добавляем схему URI в результат
							result.append(this->_fmk->format("%s:", this->_scheme.c_str()));
						break;
					}
				}
				// Если пользователь URI не пустой, то добавляем его в результат
				if(!this->_user.login.empty()){
					// Добавляем логин пользователя URI в результат
					result.append(this->_user.login);
					// Если пароль пользователя URI не пустой, то добавляем его в результат
					if(!this->_user.pass.empty())
						// Добавляем пароль пользователя URI в результат
						result.append(this->_fmk->format(":%s", this->_user.pass.c_str()));
					// Добавляем символ "@" после параметров пользователя URI
					result.append("@");
				}
				// Если атрибуты URI не пустые
				if(this->_attr != nullptr){
					/**
					 * Определяем режим формата URI для генерации
					 */
					switch(static_cast <uint8_t> (format)){
						// Если режим формата URI для генерации является полным, то генерируем полный URI
						case static_cast <uint8_t> (format_t::FULL): {
							/**
							 * Определяем тип атрибутов URI адреса
							 */
							switch(static_cast <uint8_t> (this->_attr->type)){
								// Если атрибуты URI адреса являются адресом файловой системы
								case static_cast <uint8_t> (net::type_t::FS):
									// Выводим путь к сокету из атрибутов URI адреса
									result.append(awh_cast <net::addr_fs_t *> (awh_cast <net::attr_uds_t *> (this->_attr.get())->path.get())->address);
								break;
								// Если атрибуты URI адреса являются FQDN-адресом
								case static_cast <uint8_t> (net::type_t::FQDN): {
									// Извлекаем атрибуты URI адреса как FQDN-адрес
									net::attr_fqdn_t * attr = awh_cast <net::attr_fqdn_t *> (this->_attr.get());
									// Выводим доменное имя хоста из атрибутов URI адреса
									result.append(attr->domain);
									// Если порт хоста в атрибутах URI адреса не равен 0, то добавляем его в результат
									if(attr->port != 0)
										// Добавляем порт хоста в результат, если он не равен 0
										result.append(this->_fmk->format(":%u", attr->port));
								} break;
								// Если атрибуты URI адреса являются IPv4-адресом
								case static_cast <uint8_t> (net::type_t::IPV4): {
									// Извлекаем атрибуты URI адреса как сетевой адрес
									net::attr_net_t * attr = awh_cast <net::attr_net_t *> (this->_attr.get());
									// Устанавливаем источник IP-адреса хоста в атрибутах URI адреса
									this->_addr->source(attr->ip.get(), net_addr_t::endian_t::LITTLE);
									// Выводим IP-адрес хоста в виде строки
									result.append(static_cast <string> (* this->_addr.get()));
									// Если порт хоста в атрибутах URI адреса не равен 0, то добавляем его в результат
									if(attr->port != 0)
										// Добавляем порт хоста в результат, если он не равен 0
										result.append(this->_fmk->format(":%u", attr->port));
								} break;
								// Если атрибуты URI адреса являются IPv6-адресом
								case static_cast <uint8_t> (net::type_t::IPV6): {
									// Извлекаем атрибуты URI адреса как сетевой адрес
									net::attr_net_t * attr = awh_cast <net::attr_net_t *> (this->_attr.get());
									// Устанавливаем источник IP-адреса хоста в атрибутах URI адреса
									this->_addr->source(attr->ip.get(), net_addr_t::endian_t::LITTLE);
									// Выводим IP-адрес хоста в виде строки
									result.append(this->_fmk->format("[%s]", static_cast <string> (* this->_addr.get()).c_str()));
									// Если порт хоста в атрибутах URI адреса не равен 0, то добавляем его в результат
									if(attr->port != 0)
										// Добавляем порт хоста в результат, если он не равен 0
										result.append(this->_fmk->format(":%u", attr->port));
								} break;
							}
						} break;
						// Если режим формата URI для генерации является умным, то генерируем краткий URI
						case static_cast <uint8_t> (format_t::SMART): {
							/**
							 * Определяем тип атрибутов URI адреса
							 */
							switch(static_cast <uint8_t> (this->_attr->type)){
								// Если атрибуты URI адреса являются адресом файловой системы
								case static_cast <uint8_t> (net::type_t::FS):
									// Выводим путь к сокету из атрибутов URI адреса
									result.append(awh_cast <net::addr_fs_t *> (awh_cast <net::attr_uds_t *> (this->_attr.get())->path.get())->address);
								break;
								// Если атрибуты URI адреса являются FQDN-адресом
								case static_cast <uint8_t> (net::type_t::FQDN): {
									// Извлекаем атрибуты URI адреса как FQDN-адрес
									net::attr_fqdn_t * attr = awh_cast <net::attr_fqdn_t *> (this->_attr.get());
									// Выводим доменное имя хоста из атрибутов URI адреса
									result.append(attr->domain);
									// Если порт хоста в атрибутах URI адреса не равен 0, то добавляем его в результат
									if(attr->port != 0){
										/**
										 * Определяем тип URI адреса по схеме URI
										 */
										switch(static_cast <uint8_t> (this->_type)){
											// Если тип URI является WS или HTTP, то устанавливаем порт URI как 80
											case static_cast <uint8_t> (type_t::WS):
											case static_cast <uint8_t> (type_t::HTTP):
												// Если порт URI является 80, то это может быть HTTP, иначе добавляем порт в результат
												if(attr->port != 80)
													// Добавляем порт хоста в результат, если он не равен 0
													result.append(this->_fmk->format(":%u", attr->port));
											break;
											// Если тип URI является WSS или HTTPS, то устанавливаем порт URI как 443
											case static_cast <uint8_t> (type_t::WSS):
											case static_cast <uint8_t> (type_t::HTTPS):
												// Если порт URI является 443, то это может быть HTTPS, иначе добавляем порт в результат
												if(attr->port != 443)
													// Добавляем порт хоста в результат, если он не равен 0
													result.append(this->_fmk->format(":%u", attr->port));
											break;
											// Если тип URI является SSH, то устанавливаем порт URI как 22
											case static_cast <uint8_t> (type_t::SSH):
												// Если порт URI является 22, то это может быть SSH, иначе добавляем порт в результат
												if(attr->port != 22)
													// Добавляем порт хоста в результат, если он не равен 0
													result.append(this->_fmk->format(":%u", attr->port));
											break;
											// Если тип URI является FTP, то устанавливаем порт URI как 21
											case static_cast <uint8_t> (type_t::FTP):
												// Если порт URI является 21, то это может быть FTP, иначе добавляем порт в результат
												if(attr->port != 21)
													// Добавляем порт хоста в результат, если он не равен 0
													result.append(this->_fmk->format(":%u", attr->port));
											break;
											// Если тип URI является MQTT, то устанавливаем порт URI как 1883
											case static_cast <uint8_t> (type_t::MQTT):
												// Если порт URI является 1883, то это может быть MQTT, иначе добавляем порт в результат
												if(attr->port != 1883)
													// Добавляем порт хоста в результат, если он не равен 0
													result.append(this->_fmk->format(":%u", attr->port));
											break;
											// Если тип URI является E-mail, то устанавливаем порт URI как 25
											case static_cast <uint8_t> (type_t::EMAIL):
												// Если порт URI является 25, то это может быть E-mail, иначе добавляем порт в результат
												if(attr->port != 25)
													// Добавляем порт хоста в результат, если он не равен 0
													result.append(this->_fmk->format(":%u", attr->port));
											break;
											// Если тип URI является Socks5, то устанавливаем порт URI как 1080
											case static_cast <uint8_t> (type_t::SOCKS5):
												// Если порт URI является 1080, то это может быть Socks5, иначе добавляем порт в результат
												if(attr->port != 1080)
													// Добавляем порт хоста в результат, если он не равен 0
													result.append(this->_fmk->format(":%u", attr->port));
											break;
											// Если тип URI является REDIS, то устанавливаем порт URI как 6379
											case static_cast <uint8_t> (type_t::REDIS):
												// Если порт URI является 6379, то это может быть REDIS, иначе добавляем порт в результат
												if(attr->port != 6379)
													// Добавляем порт хоста в результат, если он не равен 0
													result.append(this->_fmk->format(":%u", attr->port));
											break;
											// Если тип URI является MySQL, то устанавливаем порт URI как 3306
											case static_cast <uint8_t> (type_t::MYSQL):
												// Если порт URI является 3306, то это может быть MySQL, иначе добавляем порт в результат
												if(attr->port != 3306)
													// Добавляем порт хоста в результат, если он не равен 0
													result.append(this->_fmk->format(":%u", attr->port));
											break;
											// Если тип URI является PostgreSQL, то устанавливаем порт URI как 5432
											case static_cast <uint8_t> (type_t::POSTGRESQL):
												// Если порт URI является 5432, то это может быть PostgreSQL, иначе добавляем порт в результат
												if(attr->port != 5432)
													// Добавляем порт хоста в результат, если он не равен 0
													result.append(this->_fmk->format(":%u", attr->port));
											break;
										}
									}
								} break;
								// Если атрибуты URI адреса являются IPv4-адресом
								case static_cast <uint8_t> (net::type_t::IPV4): {
									// Извлекаем атрибуты URI адреса как сетевой адрес
									net::attr_net_t * attr = awh_cast <net::attr_net_t *> (this->_attr.get());
									// Устанавливаем источник IP-адреса хоста в атрибутах URI адреса
									this->_addr->source(attr->ip.get(), net_addr_t::endian_t::LITTLE);
									// Выводим IP-адрес хоста в виде строки
									result.append(static_cast <string> (* this->_addr.get()));
									// Если порт хоста в атрибутах URI адреса не равен 0, то добавляем его в результат
									if(attr->port != 0){
										/**
										 * Определяем тип URI адреса по схеме URI
										 */
										switch(static_cast <uint8_t> (this->_type)){
											// Если тип URI является WS или HTTP, то устанавливаем порт URI как 80
											case static_cast <uint8_t> (type_t::WS):
											case static_cast <uint8_t> (type_t::HTTP):
												// Если порт URI является 80, то это может быть HTTP, иначе добавляем порт в результат
												if(attr->port != 80)
													// Добавляем порт хоста в результат, если он не равен 0
													result.append(this->_fmk->format(":%u", attr->port));
											break;
											// Если тип URI является WSS или HTTPS, то устанавливаем порт URI как 443
											case static_cast <uint8_t> (type_t::WSS):
											case static_cast <uint8_t> (type_t::HTTPS):
												// Если порт URI является 443, то это может быть HTTPS, иначе добавляем порт в результат
												if(attr->port != 443)
													// Добавляем порт хоста в результат, если он не равен 0
													result.append(this->_fmk->format(":%u", attr->port));
											break;
											// Если тип URI является SSH, то устанавливаем порт URI как 22
											case static_cast <uint8_t> (type_t::SSH):
												// Если порт URI является 22, то это может быть SSH, иначе добавляем порт в результат
												if(attr->port != 22)
													// Добавляем порт хоста в результат, если он не равен 0
													result.append(this->_fmk->format(":%u", attr->port));
											break;
											// Если тип URI является FTP, то устанавливаем порт URI как 21
											case static_cast <uint8_t> (type_t::FTP):
												// Если порт URI является 21, то это может быть FTP, иначе добавляем порт в результат
												if(attr->port != 21)
													// Добавляем порт хоста в результат, если он не равен 0
													result.append(this->_fmk->format(":%u", attr->port));
											break;
											// Если тип URI является MQTT, то устанавливаем порт URI как 1883
											case static_cast <uint8_t> (type_t::MQTT):
												// Если порт URI является 1883, то это может быть MQTT, иначе добавляем порт в результат
												if(attr->port != 1883)
													// Добавляем порт хоста в результат, если он не равен 0
													result.append(this->_fmk->format(":%u", attr->port));
											break;
											// Если тип URI является E-mail, то устанавливаем порт URI как 25
											case static_cast <uint8_t> (type_t::EMAIL):
												// Если порт URI является 25, то это может быть E-mail, иначе добавляем порт в результат
												if(attr->port != 25)
													// Добавляем порт хоста в результат, если он не равен 0
													result.append(this->_fmk->format(":%u", attr->port));
											break;
											// Если тип URI является Socks5, то устанавливаем порт URI как 1080
											case static_cast <uint8_t> (type_t::SOCKS5):
												// Если порт URI является 1080, то это может быть Socks5, иначе добавляем порт в результат
												if(attr->port != 1080)
													// Добавляем порт хоста в результат, если он не равен 0
													result.append(this->_fmk->format(":%u", attr->port));
											break;
											// Если тип URI является REDIS, то устанавливаем порт URI как 6379
											case static_cast <uint8_t> (type_t::REDIS):
												// Если порт URI является 6379, то это может быть REDIS, иначе добавляем порт в результат
												if(attr->port != 6379)
													// Добавляем порт хоста в результат, если он не равен 0
													result.append(this->_fmk->format(":%u", attr->port));
											break;
											// Если тип URI является MySQL, то устанавливаем порт URI как 3306
											case static_cast <uint8_t> (type_t::MYSQL):
												// Если порт URI является 3306, то это может быть MySQL, иначе добавляем порт в результат
												if(attr->port != 3306)
													// Добавляем порт хоста в результат, если он не равен 0
													result.append(this->_fmk->format(":%u", attr->port));
											break;
											// Если тип URI является PostgreSQL, то устанавливаем порт URI как 5432
											case static_cast <uint8_t> (type_t::POSTGRESQL):
												// Если порт URI является 5432, то это может быть PostgreSQL, иначе добавляем порт в результат
												if(attr->port != 5432)
													// Добавляем порт хоста в результат, если он не равен 0
													result.append(this->_fmk->format(":%u", attr->port));
											break;
										}
									}
								} break;
								// Если атрибуты URI адреса являются IPv6-адресом
								case static_cast <uint8_t> (net::type_t::IPV6): {
									// Извлекаем атрибуты URI адреса как сетевой адрес
									net::attr_net_t * attr = awh_cast <net::attr_net_t *> (this->_attr.get());
									// Устанавливаем источник IP-адреса хоста в атрибутах URI адреса
									this->_addr->source(attr->ip.get(), net_addr_t::endian_t::LITTLE);
									// Выводим IP-адрес хоста в виде строки
									result.append(this->_fmk->format("[%s]", static_cast <string> (* this->_addr.get()).c_str()));
									// Если порт хоста в атрибутах URI адреса не равен 0, то добавляем его в результат
									if(attr->port != 0){
										/**
										 * Определяем тип URI адреса по схеме URI
										 */
										switch(static_cast <uint8_t> (this->_type)){
											// Если тип URI является WS или HTTP, то устанавливаем порт URI как 80
											case static_cast <uint8_t> (type_t::WS):
											case static_cast <uint8_t> (type_t::HTTP):
												// Если порт URI является 80, то это может быть HTTP, иначе добавляем порт в результат
												if(attr->port != 80)
													// Добавляем порт хоста в результат, если он не равен 0
													result.append(this->_fmk->format(":%u", attr->port));
											break;
											// Если тип URI является WSS или HTTPS, то устанавливаем порт URI как 443
											case static_cast <uint8_t> (type_t::WSS):
											case static_cast <uint8_t> (type_t::HTTPS):
												// Если порт URI является 443, то это может быть HTTPS, иначе добавляем порт в результат
												if(attr->port != 443)
													// Добавляем порт хоста в результат, если он не равен 0
													result.append(this->_fmk->format(":%u", attr->port));
											break;
											// Если тип URI является SSH, то устанавливаем порт URI как 22
											case static_cast <uint8_t> (type_t::SSH):
												// Если порт URI является 22, то это может быть SSH, иначе добавляем порт в результат
												if(attr->port != 22)
													// Добавляем порт хоста в результат, если он не равен 0
													result.append(this->_fmk->format(":%u", attr->port));
											break;
											// Если тип URI является FTP, то устанавливаем порт URI как 21
											case static_cast <uint8_t> (type_t::FTP):
												// Если порт URI является 21, то это может быть FTP, иначе добавляем порт в результат
												if(attr->port != 21)
													// Добавляем порт хоста в результат, если он не равен 0
													result.append(this->_fmk->format(":%u", attr->port));
											break;
											// Если тип URI является MQTT, то устанавливаем порт URI как 1883
											case static_cast <uint8_t> (type_t::MQTT):
												// Если порт URI является 1883, то это может быть MQTT, иначе добавляем порт в результат
												if(attr->port != 1883)
													// Добавляем порт хоста в результат, если он не равен 0
													result.append(this->_fmk->format(":%u", attr->port));
											break;
											// Если тип URI является E-mail, то устанавливаем порт URI как 25
											case static_cast <uint8_t> (type_t::EMAIL):
												// Если порт URI является 25, то это может быть E-mail, иначе добавляем порт в результат
												if(attr->port != 25)
													// Добавляем порт хоста в результат, если он не равен 0
													result.append(this->_fmk->format(":%u", attr->port));
											break;
											// Если тип URI является Socks5, то устанавливаем порт URI как 1080
											case static_cast <uint8_t> (type_t::SOCKS5):
												// Если порт URI является 1080, то это может быть Socks5, иначе добавляем порт в результат
												if(attr->port != 1080)
													// Добавляем порт хоста в результат, если он не равен 0
													result.append(this->_fmk->format(":%u", attr->port));
											break;
											// Если тип URI является REDIS, то устанавливаем порт URI как 6379
											case static_cast <uint8_t> (type_t::REDIS):
												// Если порт URI является 6379, то это может быть REDIS, иначе добавляем порт в результат
												if(attr->port != 6379)
													// Добавляем порт хоста в результат, если он не равен 0
													result.append(this->_fmk->format(":%u", attr->port));
											break;
											// Если тип URI является MySQL, то устанавливаем порт URI как 3306
											case static_cast <uint8_t> (type_t::MYSQL):
												// Если порт URI является 3306, то это может быть MySQL, иначе добавляем порт в результат
												if(attr->port != 3306)
													// Добавляем порт хоста в результат, если он не равен 0
													result.append(this->_fmk->format(":%u", attr->port));
											break;
											// Если тип URI является PostgreSQL, то устанавливаем порт URI как 5432
											case static_cast <uint8_t> (type_t::POSTGRESQL):
												// Если порт URI является 5432, то это может быть PostgreSQL, иначе добавляем порт в результат
												if(attr->port != 5432)
													// Добавляем порт хоста в результат, если он не равен 0
													result.append(this->_fmk->format(":%u", attr->port));
											break;
										}
									}
								} break;
							}
						} break;
					}
				}
				// Если путь URI не пустой, то добавляем его в результат
				if(!this->_path.empty()){
					/**
					 * Определяем тип URI адреса по схеме URI
					 */
					switch(static_cast <uint8_t> (this->_type)){
						// Если тип URI является WS, WSS, UDS, FTP, FILE, REDIS, MySQL, SOCKS5, PostgreSQL, HTTP или HTTPS
						case static_cast <uint8_t> (type_t::NONE):
						case static_cast <uint8_t> (type_t::WS):
						case static_cast <uint8_t> (type_t::WSS):
						case static_cast <uint8_t> (type_t::UDS):
						case static_cast <uint8_t> (type_t::FTP):
						case static_cast <uint8_t> (type_t::FILE):
						case static_cast <uint8_t> (type_t::HTTP):
						case static_cast <uint8_t> (type_t::HTTPS):
						case static_cast <uint8_t> (type_t::REDIS):
						case static_cast <uint8_t> (type_t::MYSQL):
						case static_cast <uint8_t> (type_t::SOCKS5):
						case static_cast <uint8_t> (type_t::POSTGRESQL): {
							// Добавляем символ "/" перед каждым сегментом пути URI и добавляем сегменты пути URI в результат
							for(const string & segment : this->_path){
								// Добавляем символ "/" перед сегментом пути URI
								result.append(1, '/');
								// Добавляем сегмент пути URI в результат
								result.append(uri::encode(segment, this->_log));
							}
						} break;
						// Если тип URI является SSH
						case static_cast <uint8_t> (type_t::SSH): {
							// Добавляем символ ":" перед сегментом пути URI
							result.append(1, ':');
							// Добавляем символ "/" перед каждым сегментом пути URI и добавляем сегменты пути URI в результат
							for(auto i = this->_path.begin(); i != this->_path.end(); ++i){
								// Если это не первый сегмент пути URI, то добавляем символ "/" перед ним
								if(i != this->_path.begin())
									// Добавляем символ "/" перед сегментом пути URI
									result.append(1, '/');
								// Добавляем сегмент пути URI в результат
								result.append(uri::encode(* i, this->_log));
							}
						} break;
						// Если тип URI является Scheme
						case static_cast <uint8_t> (type_t::SCHEME): {
							// Добавляем символ "/" перед каждым сегментом пути URI и добавляем сегменты пути URI в результат
							for(auto i = this->_path.begin(); i != this->_path.end(); ++i){
								// Если это не первый сегмент пути URI, то добавляем символ "/" перед ним
								if(i != this->_path.begin())
									// Добавляем символ "/" перед сегментом пути URI
									result.append(1, '/');
								// Добавляем сегмент пути URI в результат
								result.append(uri::encode(* i, this->_log));
							}
						} break;
					}
				// Если путь URI пустой, но параметры URI не пустые, то добавляем символ "/" в результат перед параметрами URI
				} else if(!this->_query.empty() || !this->_fragment.empty()) {
					/**
					 * Определяем тип URI адреса по схеме URI
					 */
					switch(static_cast <uint8_t> (this->_type)){
						// Если тип URI является WS, WSS, UDS, FTP, FILE, REDIS, MySQL, SOCKS5, PostgreSQL, HTTP или HTTPS
						case static_cast <uint8_t> (type_t::NONE):
						case static_cast <uint8_t> (type_t::WS):
						case static_cast <uint8_t> (type_t::WSS):
						case static_cast <uint8_t> (type_t::UDS):
						case static_cast <uint8_t> (type_t::FTP):
						case static_cast <uint8_t> (type_t::FILE):
						case static_cast <uint8_t> (type_t::HTTP):
						case static_cast <uint8_t> (type_t::HTTPS):
						case static_cast <uint8_t> (type_t::REDIS):
						case static_cast <uint8_t> (type_t::MYSQL):
						case static_cast <uint8_t> (type_t::SOCKS5):
						case static_cast <uint8_t> (type_t::POSTGRESQL):
							// Добавляем символ "/" в результат перед параметрами URI
							result.append(1, '/');
						break;
					}
				}
				// Если параметры URI не пустые, то добавляем их в результат
				if(!this->_query.empty()){
					/**
					 * Определяем тип URI адреса по схеме URI
					 */
					switch(static_cast <uint8_t> (this->_type)){
						// Если тип URI является WS, WSS, FTP, SSH, REDIS, MySQL, Scheme, SOCKS5, PostgreSQL, HTTP или HTTPS
						case static_cast <uint8_t> (type_t::NONE):
						case static_cast <uint8_t> (type_t::WS):
						case static_cast <uint8_t> (type_t::WSS):
						case static_cast <uint8_t> (type_t::FTP):
						case static_cast <uint8_t> (type_t::SSH):
						case static_cast <uint8_t> (type_t::HTTP):
						case static_cast <uint8_t> (type_t::HTTPS):
						case static_cast <uint8_t> (type_t::REDIS):
						case static_cast <uint8_t> (type_t::MYSQL):
						case static_cast <uint8_t> (type_t::SCHEME):
						case static_cast <uint8_t> (type_t::SOCKS5):
						case static_cast <uint8_t> (type_t::POSTGRESQL): {
							// Добавляем символ "?" перед первой парой ключ-значение параметров URI
							result.append("?");
							// Добавляем пары ключ-значение параметров URI в результат, разделяя их символом "&"
							for(const auto & [key, value] : this->_query)
								// Добавляем пару ключ-значение параметров URI в результат, разделяя их символом "=" и добавляя символ "&" после каждой пары
								result.append(this->_fmk->format("%s=%s&", uri::encode(key, this->_log).c_str(), uri::encode(value, this->_log).c_str()));
							// Если функция обратного вызова установлена
							if(this->_callback != nullptr){
								// Выполняем генерацию параметров URI с помощью функции обратного вызова
								const string & item = this->_callback(this);
								// Если параметры URI, сгенерированные удачно
								if(!item.empty())
									// Вызываем функцию обратного вызова и добавляем её результат в результат
									result.append(item);
							// Удаляем последний символ "&" из результата
							} else result.pop_back();
						} break;
					}
				}
				// Если якорь URI не пустой, то добавляем его в результат
				if(!this->_fragment.empty()){
					/**
					 * Определяем тип URI адреса по схеме URI
					 */
					switch(static_cast <uint8_t> (this->_type)){
						// Если тип URI является WS, WSS, Scheme, HTTP или HTTPS
						case static_cast <uint8_t> (type_t::NONE):
						case static_cast <uint8_t> (type_t::WS):
						case static_cast <uint8_t> (type_t::WSS):
						case static_cast <uint8_t> (type_t::HTTP):
						case static_cast <uint8_t> (type_t::HTTPS):
						case static_cast <uint8_t> (type_t::SCHEME):
							// Добавляем якорь URI в результат, предваряя его символом "#"
							result.append(this->_fmk->format("#%s", uri::encode(this->_fragment, this->_log).c_str()));
						break;
					}
				}
			} break;
			// Если режим элемента URI для генерации является схемой, то генерируем схему URI
			case static_cast <uint8_t> (item_t::SCHEME): {
				// Если схема URI не пустая, то добавляем её в результат
				if(!this->_scheme.empty()){
					/**
					 * Определяем тип URI адреса по схеме URI
					 */
					switch(static_cast <uint8_t> (this->_type)){
						// Если тип URI является WS, WSS, FTP, UDS, FILE, MQTT, REDIS, MySQL, SOCKS5, PostgreSQL, HTTP или HTTPS, то добавляем схему URI в результат с "://"
						case static_cast <uint8_t> (type_t::WS):
						case static_cast <uint8_t> (type_t::WSS):
						case static_cast <uint8_t> (type_t::FTP):
						case static_cast <uint8_t> (type_t::UDS):
						case static_cast <uint8_t> (type_t::FILE):
						case static_cast <uint8_t> (type_t::MQTT):
						case static_cast <uint8_t> (type_t::HTTP):
						case static_cast <uint8_t> (type_t::HTTPS):
						case static_cast <uint8_t> (type_t::REDIS):
						case static_cast <uint8_t> (type_t::MYSQL):
						case static_cast <uint8_t> (type_t::SOCKS5):
						case static_cast <uint8_t> (type_t::POSTGRESQL):
							// Добавляем схему URI в результат
							result.append(this->_fmk->format("%s://", this->_scheme.c_str()));
						break;
						// Если тип URI является SSH, E-mail или Scheme, то добавляем схему URI в результат без "://"
						case static_cast <uint8_t> (type_t::SSH):
						case static_cast <uint8_t> (type_t::EMAIL):
						case static_cast <uint8_t> (type_t::SCHEME):
							// Добавляем схему URI в результат
							result.append(this->_fmk->format("%s:", this->_scheme.c_str()));
						break;
					}
				}
			} break;
			// Если режим элемента URI для генерации является пользователем, то генерируем параметры пользователя URI
			case static_cast <uint8_t> (item_t::USER): {
				// Если пользователь URI не пустой, то добавляем его в результат
				if(!this->_user.login.empty()){
					// Добавляем логин пользователя URI в результат
					result.append(this->_user.login);
					// Если пароль пользователя URI не пустой, то добавляем его в результат
					if(!this->_user.pass.empty())
						// Добавляем пароль пользователя URI в результат
						result.append(this->_fmk->format(":%s", this->_user.pass.c_str()));
					// Добавляем символ "@" после параметров пользователя URI
					result.append("@");
				}
			} break;
			// Если режим элемента URI для генерации является хостом, то генерируем хост URI
			case static_cast <uint8_t> (item_t::HOST): {
				// Если атрибуты URI не пустые
				if(this->_attr != nullptr){
					/**
					 * Определяем режим формата URI для генерации
					 */
					switch(static_cast <uint8_t> (format)){
						// Если режим формата URI для генерации является полным, то генерируем полный URI
						case static_cast <uint8_t> (format_t::FULL): {
							/**
							 * Определяем тип атрибутов URI адреса
							 */
							switch(static_cast <uint8_t> (this->_attr->type)){
								// Если атрибуты URI адреса являются адресом файловой системы
								case static_cast <uint8_t> (net::type_t::FS):
									// Выводим путь к сокету из атрибутов URI адреса
									result.append(awh_cast <net::addr_fs_t *> (awh_cast <net::attr_uds_t *> (this->_attr.get())->path.get())->address);
								break;
								// Если атрибуты URI адреса являются FQDN-адресом
								case static_cast <uint8_t> (net::type_t::FQDN): {
									// Извлекаем атрибуты URI адреса как FQDN-адрес
									net::attr_fqdn_t * attr = awh_cast <net::attr_fqdn_t *> (this->_attr.get());
									// Выводим доменное имя хоста из атрибутов URI адреса
									result.append(attr->domain);
									// Если порт хоста в атрибутах URI адреса не равен 0, то добавляем его в результат
									if(attr->port != 0)
										// Добавляем порт хоста в результат, если он не равен 0
										result.append(this->_fmk->format(":%u", attr->port));
								} break;
								// Если атрибуты URI адреса являются IPv4-адресом
								case static_cast <uint8_t> (net::type_t::IPV4): {
									// Извлекаем атрибуты URI адреса как сетевой адрес
									net::attr_net_t * attr = awh_cast <net::attr_net_t *> (this->_attr.get());
									// Устанавливаем источник IP-адреса хоста в атрибутах URI адреса
									this->_addr->source(attr->ip.get(), net_addr_t::endian_t::LITTLE);
									// Выводим IP-адрес хоста в виде строки
									result.append(static_cast <string> (* this->_addr.get()));
									// Если порт хоста в атрибутах URI адреса не равен 0, то добавляем его в результат
									if(attr->port != 0)
										// Добавляем порт хоста в результат, если он не равен 0
										result.append(this->_fmk->format(":%u", attr->port));
								} break;
								// Если атрибуты URI адреса являются IPv6-адресом
								case static_cast <uint8_t> (net::type_t::IPV6): {
									// Извлекаем атрибуты URI адреса как сетевой адрес
									net::attr_net_t * attr = awh_cast <net::attr_net_t *> (this->_attr.get());
									// Устанавливаем источник IP-адреса хоста в атрибутах URI адреса
									this->_addr->source(attr->ip.get(), net_addr_t::endian_t::LITTLE);
									// Выводим IP-адрес хоста в виде строки
									result.append(this->_fmk->format("[%s]", static_cast <string> (* this->_addr.get()).c_str()));
									// Если порт хоста в атрибутах URI адреса не равен 0, то добавляем его в результат
									if(attr->port != 0)
										// Добавляем порт хоста в результат, если он не равен 0
										result.append(this->_fmk->format(":%u", attr->port));
								} break;
							}
						} break;
						// Если режим формата URI для генерации является умным, то генерируем краткий URI
						case static_cast <uint8_t> (format_t::SMART): {
							/**
							 * Определяем тип атрибутов URI адреса
							 */
							switch(static_cast <uint8_t> (this->_attr->type)){
								// Если атрибуты URI адреса являются адресом файловой системы
								case static_cast <uint8_t> (net::type_t::FS):
									// Выводим путь к сокету из атрибутов URI адреса
									result.append(awh_cast <net::addr_fs_t *> (awh_cast <net::attr_uds_t *> (this->_attr.get())->path.get())->address);
								break;
								// Если атрибуты URI адреса являются FQDN-адресом
								case static_cast <uint8_t> (net::type_t::FQDN): {
									// Извлекаем атрибуты URI адреса как FQDN-адрес
									net::attr_fqdn_t * attr = awh_cast <net::attr_fqdn_t *> (this->_attr.get());
									// Выводим доменное имя хоста из атрибутов URI адреса
									result.append(attr->domain);
									// Если порт хоста в атрибутах URI адреса не равен 0, то добавляем его в результат
									if(attr->port != 0){
										/**
										 * Определяем тип URI адреса по схеме URI
										 */
										switch(static_cast <uint8_t> (this->_type)){
											// Если тип URI является WS или HTTP, то устанавливаем порт URI как 80
											case static_cast <uint8_t> (type_t::WS):
											case static_cast <uint8_t> (type_t::HTTP):
												// Если порт URI является 80, то это может быть HTTP, иначе добавляем порт в результат
												if(attr->port != 80)
													// Добавляем порт хоста в результат, если он не равен 0
													result.append(this->_fmk->format(":%u", attr->port));
											break;
											// Если тип URI является WSS или HTTPS, то устанавливаем порт URI как 443
											case static_cast <uint8_t> (type_t::WSS):
											case static_cast <uint8_t> (type_t::HTTPS):
												// Если порт URI является 443, то это может быть HTTPS, иначе добавляем порт в результат
												if(attr->port != 443)
													// Добавляем порт хоста в результат, если он не равен 0
													result.append(this->_fmk->format(":%u", attr->port));
											break;
											// Если тип URI является SSH, то устанавливаем порт URI как 22
											case static_cast <uint8_t> (type_t::SSH):
												// Если порт URI является 22, то это может быть SSH, иначе добавляем порт в результат
												if(attr->port != 22)
													// Добавляем порт хоста в результат, если он не равен 0
													result.append(this->_fmk->format(":%u", attr->port));
											break;
											// Если тип URI является FTP, то устанавливаем порт URI как 21
											case static_cast <uint8_t> (type_t::FTP):
												// Если порт URI является 21, то это может быть FTP, иначе добавляем порт в результат
												if(attr->port != 21)
													// Добавляем порт хоста в результат, если он не равен 0
													result.append(this->_fmk->format(":%u", attr->port));
											break;
											// Если тип URI является MQTT, то устанавливаем порт URI как 1883
											case static_cast <uint8_t> (type_t::MQTT):
												// Если порт URI является 1883, то это может быть MQTT, иначе добавляем порт в результат
												if(attr->port != 1883)
													// Добавляем порт хоста в результат, если он не равен 0
													result.append(this->_fmk->format(":%u", attr->port));
											break;
											// Если тип URI является E-mail, то устанавливаем порт URI как 25
											case static_cast <uint8_t> (type_t::EMAIL):
												// Если порт URI является 25, то это может быть E-mail, иначе добавляем порт в результат
												if(attr->port != 25)
													// Добавляем порт хоста в результат, если он не равен 0
													result.append(this->_fmk->format(":%u", attr->port));
											break;
											// Если тип URI является Socks5, то устанавливаем порт URI как 1080
											case static_cast <uint8_t> (type_t::SOCKS5):
												// Если порт URI является 1080, то это может быть Socks5, иначе добавляем порт в результат
												if(attr->port != 1080)
													// Добавляем порт хоста в результат, если он не равен 0
													result.append(this->_fmk->format(":%u", attr->port));
											break;
											// Если тип URI является REDIS, то устанавливаем порт URI как 6379
											case static_cast <uint8_t> (type_t::REDIS):
												// Если порт URI является 6379, то это может быть REDIS, иначе добавляем порт в результат
												if(attr->port != 6379)
													// Добавляем порт хоста в результат, если он не равен 0
													result.append(this->_fmk->format(":%u", attr->port));
											break;
											// Если тип URI является MySQL, то устанавливаем порт URI как 3306
											case static_cast <uint8_t> (type_t::MYSQL):
												// Если порт URI является 3306, то это может быть MySQL, иначе добавляем порт в результат
												if(attr->port != 3306)
													// Добавляем порт хоста в результат, если он не равен 0
													result.append(this->_fmk->format(":%u", attr->port));
											break;
											// Если тип URI является PostgreSQL, то устанавливаем порт URI как 5432
											case static_cast <uint8_t> (type_t::POSTGRESQL):
												// Если порт URI является 5432, то это может быть PostgreSQL, иначе добавляем порт в результат
												if(attr->port != 5432)
													// Добавляем порт хоста в результат, если он не равен 0
													result.append(this->_fmk->format(":%u", attr->port));
											break;
										}
									}
								} break;
								// Если атрибуты URI адреса являются IPv4-адресом
								case static_cast <uint8_t> (net::type_t::IPV4): {
									// Извлекаем атрибуты URI адреса как сетевой адрес
									net::attr_net_t * attr = awh_cast <net::attr_net_t *> (this->_attr.get());
									// Устанавливаем источник IP-адреса хоста в атрибутах URI адреса
									this->_addr->source(attr->ip.get(), net_addr_t::endian_t::LITTLE);
									// Выводим IP-адрес хоста в виде строки
									result.append(static_cast <string> (* this->_addr.get()));
									// Если порт хоста в атрибутах URI адреса не равен 0, то добавляем его в результат
									if(attr->port != 0){
										/**
										 * Определяем тип URI адреса по схеме URI
										 */
										switch(static_cast <uint8_t> (this->_type)){
											// Если тип URI является WS или HTTP, то устанавливаем порт URI как 80
											case static_cast <uint8_t> (type_t::WS):
											case static_cast <uint8_t> (type_t::HTTP):
												// Если порт URI является 80, то это может быть HTTP, иначе добавляем порт в результат
												if(attr->port != 80)
													// Добавляем порт хоста в результат, если он не равен 0
													result.append(this->_fmk->format(":%u", attr->port));
											break;
											// Если тип URI является WSS или HTTPS, то устанавливаем порт URI как 443
											case static_cast <uint8_t> (type_t::WSS):
											case static_cast <uint8_t> (type_t::HTTPS):
												// Если порт URI является 443, то это может быть HTTPS, иначе добавляем порт в результат
												if(attr->port != 443)
													// Добавляем порт хоста в результат, если он не равен 0
													result.append(this->_fmk->format(":%u", attr->port));
											break;
											// Если тип URI является SSH, то устанавливаем порт URI как 22
											case static_cast <uint8_t> (type_t::SSH):
												// Если порт URI является 22, то это может быть SSH, иначе добавляем порт в результат
												if(attr->port != 22)
													// Добавляем порт хоста в результат, если он не равен 0
													result.append(this->_fmk->format(":%u", attr->port));
											break;
											// Если тип URI является FTP, то устанавливаем порт URI как 21
											case static_cast <uint8_t> (type_t::FTP):
												// Если порт URI является 21, то это может быть FTP, иначе добавляем порт в результат
												if(attr->port != 21)
													// Добавляем порт хоста в результат, если он не равен 0
													result.append(this->_fmk->format(":%u", attr->port));
											break;
											// Если тип URI является MQTT, то устанавливаем порт URI как 1883
											case static_cast <uint8_t> (type_t::MQTT):
												// Если порт URI является 1883, то это может быть MQTT, иначе добавляем порт в результат
												if(attr->port != 1883)
													// Добавляем порт хоста в результат, если он не равен 0
													result.append(this->_fmk->format(":%u", attr->port));
											break;
											// Если тип URI является E-mail, то устанавливаем порт URI как 25
											case static_cast <uint8_t> (type_t::EMAIL):
												// Если порт URI является 25, то это может быть E-mail, иначе добавляем порт в результат
												if(attr->port != 25)
													// Добавляем порт хоста в результат, если он не равен 0
													result.append(this->_fmk->format(":%u", attr->port));
											break;
											// Если тип URI является Socks5, то устанавливаем порт URI как 1080
											case static_cast <uint8_t> (type_t::SOCKS5):
												// Если порт URI является 1080, то это может быть Socks5, иначе добавляем порт в результат
												if(attr->port != 1080)
													// Добавляем порт хоста в результат, если он не равен 0
													result.append(this->_fmk->format(":%u", attr->port));
											break;
											// Если тип URI является REDIS, то устанавливаем порт URI как 6379
											case static_cast <uint8_t> (type_t::REDIS):
												// Если порт URI является 6379, то это может быть REDIS, иначе добавляем порт в результат
												if(attr->port != 6379)
													// Добавляем порт хоста в результат, если он не равен 0
													result.append(this->_fmk->format(":%u", attr->port));
											break;
											// Если тип URI является MySQL, то устанавливаем порт URI как 3306
											case static_cast <uint8_t> (type_t::MYSQL):
												// Если порт URI является 3306, то это может быть MySQL, иначе добавляем порт в результат
												if(attr->port != 3306)
													// Добавляем порт хоста в результат, если он не равен 0
													result.append(this->_fmk->format(":%u", attr->port));
											break;
											// Если тип URI является PostgreSQL, то устанавливаем порт URI как 5432
											case static_cast <uint8_t> (type_t::POSTGRESQL):
												// Если порт URI является 5432, то это может быть PostgreSQL, иначе добавляем порт в результат
												if(attr->port != 5432)
													// Добавляем порт хоста в результат, если он не равен 0
													result.append(this->_fmk->format(":%u", attr->port));
											break;
										}
									}
								} break;
								// Если атрибуты URI адреса являются IPv6-адресом
								case static_cast <uint8_t> (net::type_t::IPV6): {
									// Извлекаем атрибуты URI адреса как сетевой адрес
									net::attr_net_t * attr = awh_cast <net::attr_net_t *> (this->_attr.get());
									// Устанавливаем источник IP-адреса хоста в атрибутах URI адреса
									this->_addr->source(attr->ip.get(), net_addr_t::endian_t::LITTLE);
									// Выводим IP-адрес хоста в виде строки
									result.append(this->_fmk->format("[%s]", static_cast <string> (* this->_addr.get()).c_str()));
									// Если порт хоста в атрибутах URI адреса не равен 0, то добавляем его в результат
									if(attr->port != 0){
										/**
										 * Определяем тип URI адреса по схеме URI
										 */
										switch(static_cast <uint8_t> (this->_type)){
											// Если тип URI является WS или HTTP, то устанавливаем порт URI как 80
											case static_cast <uint8_t> (type_t::WS):
											case static_cast <uint8_t> (type_t::HTTP):
												// Если порт URI является 80, то это может быть HTTP, иначе добавляем порт в результат
												if(attr->port != 80)
													// Добавляем порт хоста в результат, если он не равен 0
													result.append(this->_fmk->format(":%u", attr->port));
											break;
											// Если тип URI является WSS или HTTPS, то устанавливаем порт URI как 443
											case static_cast <uint8_t> (type_t::WSS):
											case static_cast <uint8_t> (type_t::HTTPS):
												// Если порт URI является 443, то это может быть HTTPS, иначе добавляем порт в результат
												if(attr->port != 443)
													// Добавляем порт хоста в результат, если он не равен 0
													result.append(this->_fmk->format(":%u", attr->port));
											break;
											// Если тип URI является SSH, то устанавливаем порт URI как 22
											case static_cast <uint8_t> (type_t::SSH):
												// Если порт URI является 22, то это может быть SSH, иначе добавляем порт в результат
												if(attr->port != 22)
													// Добавляем порт хоста в результат, если он не равен 0
													result.append(this->_fmk->format(":%u", attr->port));
											break;
											// Если тип URI является FTP, то устанавливаем порт URI как 21
											case static_cast <uint8_t> (type_t::FTP):
												// Если порт URI является 21, то это может быть FTP, иначе добавляем порт в результат
												if(attr->port != 21)
													// Добавляем порт хоста в результат, если он не равен 0
													result.append(this->_fmk->format(":%u", attr->port));
											break;
											// Если тип URI является MQTT, то устанавливаем порт URI как 1883
											case static_cast <uint8_t> (type_t::MQTT):
												// Если порт URI является 1883, то это может быть MQTT, иначе добавляем порт в результат
												if(attr->port != 1883)
													// Добавляем порт хоста в результат, если он не равен 0
													result.append(this->_fmk->format(":%u", attr->port));
											break;
											// Если тип URI является E-mail, то устанавливаем порт URI как 25
											case static_cast <uint8_t> (type_t::EMAIL):
												// Если порт URI является 25, то это может быть E-mail, иначе добавляем порт в результат
												if(attr->port != 25)
													// Добавляем порт хоста в результат, если он не равен 0
													result.append(this->_fmk->format(":%u", attr->port));
											break;
											// Если тип URI является Socks5, то устанавливаем порт URI как 1080
											case static_cast <uint8_t> (type_t::SOCKS5):
												// Если порт URI является 1080, то это может быть Socks5, иначе добавляем порт в результат
												if(attr->port != 1080)
													// Добавляем порт хоста в результат, если он не равен 0
													result.append(this->_fmk->format(":%u", attr->port));
											break;
											// Если тип URI является REDIS, то устанавливаем порт URI как 6379
											case static_cast <uint8_t> (type_t::REDIS):
												// Если порт URI является 6379, то это может быть REDIS, иначе добавляем порт в результат
												if(attr->port != 6379)
													// Добавляем порт хоста в результат, если он не равен 0
													result.append(this->_fmk->format(":%u", attr->port));
											break;
											// Если тип URI является MySQL, то устанавливаем порт URI как 3306
											case static_cast <uint8_t> (type_t::MYSQL):
												// Если порт URI является 3306, то это может быть MySQL, иначе добавляем порт в результат
												if(attr->port != 3306)
													// Добавляем порт хоста в результат, если он не равен 0
													result.append(this->_fmk->format(":%u", attr->port));
											break;
											// Если тип URI является PostgreSQL, то устанавливаем порт URI как 5432
											case static_cast <uint8_t> (type_t::POSTGRESQL):
												// Если порт URI является 5432, то это может быть PostgreSQL, иначе добавляем порт в результат
												if(attr->port != 5432)
													// Добавляем порт хоста в результат, если он не равен 0
													result.append(this->_fmk->format(":%u", attr->port));
											break;
										}
									}
								} break;
							}
						} break;
					}
				}
			} break;
			// Если режим элемента URI для генерации является путём, то генерируем путь URI
			case static_cast <uint8_t> (item_t::PATH): {
				// Если путь URI не пустой, то добавляем его в результат
				if(!this->_path.empty()){
					/**
					 * Определяем тип URI адреса по схеме URI
					 */
					switch(static_cast <uint8_t> (this->_type)){
						// Если тип URI является WS, WSS, UDS, FTP, FILE, REDIS, MySQL, SOCKS5, PostgreSQL, HTTP или HTTPS
						case static_cast <uint8_t> (type_t::NONE):
						case static_cast <uint8_t> (type_t::WS):
						case static_cast <uint8_t> (type_t::WSS):
						case static_cast <uint8_t> (type_t::UDS):
						case static_cast <uint8_t> (type_t::FTP):
						case static_cast <uint8_t> (type_t::FILE):
						case static_cast <uint8_t> (type_t::HTTP):
						case static_cast <uint8_t> (type_t::HTTPS):
						case static_cast <uint8_t> (type_t::REDIS):
						case static_cast <uint8_t> (type_t::MYSQL):
						case static_cast <uint8_t> (type_t::SOCKS5):
						case static_cast <uint8_t> (type_t::POSTGRESQL): {
							// Добавляем символ "/" перед каждым сегментом пути URI и добавляем сегменты пути URI в результат
							for(const string & segment : this->_path){
								// Добавляем символ "/" перед сегментом пути URI
								result.append(1, '/');
								// Добавляем сегмент пути URI в результат
								result.append(uri::encode(segment, this->_log));
							}
						} break;
						// Если тип URI является SSH
						case static_cast <uint8_t> (type_t::SSH): {
							// Добавляем символ ":" перед сегментом пути URI
							result.append(1, ':');
							// Добавляем символ "/" перед каждым сегментом пути URI и добавляем сегменты пути URI в результат
							for(auto i = this->_path.begin(); i != this->_path.end(); ++i){
								// Если это не первый сегмент пути URI, то добавляем символ "/" перед ним
								if(i != this->_path.begin())
									// Добавляем символ "/" перед сегментом пути URI
									result.append(1, '/');
								// Добавляем сегмент пути URI в результат
								result.append(uri::encode(* i, this->_log));
							}
						} break;
						// Если тип URI является Scheme
						case static_cast <uint8_t> (type_t::SCHEME): {
							// Добавляем символ "/" перед каждым сегментом пути URI и добавляем сегменты пути URI в результат
							for(auto i = this->_path.begin(); i != this->_path.end(); ++i){
								// Если это не первый сегмент пути URI, то добавляем символ "/" перед ним
								if(i != this->_path.begin())
									// Добавляем символ "/" перед сегментом пути URI
									result.append(1, '/');
								// Добавляем сегмент пути URI в результат
								result.append(uri::encode(* i, this->_log));
							}
						} break;
					}
				// Если путь URI пустой, но параметры URI не пустые, то добавляем символ "/" в результат перед параметрами URI
				} else if(!this->_query.empty() || !this->_fragment.empty()) {
					/**
					 * Определяем тип URI адреса по схеме URI
					 */
					switch(static_cast <uint8_t> (this->_type)){
						// Если тип URI является WS, WSS, UDS, FTP, FILE, REDIS, MySQL, SOCKS5, PostgreSQL, HTTP или HTTPS
						case static_cast <uint8_t> (type_t::NONE):
						case static_cast <uint8_t> (type_t::WS):
						case static_cast <uint8_t> (type_t::WSS):
						case static_cast <uint8_t> (type_t::UDS):
						case static_cast <uint8_t> (type_t::FTP):
						case static_cast <uint8_t> (type_t::FILE):
						case static_cast <uint8_t> (type_t::HTTP):
						case static_cast <uint8_t> (type_t::HTTPS):
						case static_cast <uint8_t> (type_t::REDIS):
						case static_cast <uint8_t> (type_t::MYSQL):
						case static_cast <uint8_t> (type_t::SOCKS5):
						case static_cast <uint8_t> (type_t::POSTGRESQL):
							// Добавляем символ "/" в результат перед параметрами URI
							result.append(1, '/');
						break;
					}
				}
			} break;
			// Если режим элемента URI для генерации является параметрами, то генерируем параметры URI
			case static_cast <uint8_t> (item_t::QUERY): {
				// Если параметры URI не пустые, то добавляем их в результат
				if(!this->_query.empty()){
					/**
					 * Определяем тип URI адреса по схеме URI
					 */
					switch(static_cast <uint8_t> (this->_type)){
						// Если тип URI является WS, WSS, FTP, SSH, REDIS, MySQL, Scheme, SOCKS5, PostgreSQL, HTTP или HTTPS
						case static_cast <uint8_t> (type_t::NONE):
						case static_cast <uint8_t> (type_t::WS):
						case static_cast <uint8_t> (type_t::WSS):
						case static_cast <uint8_t> (type_t::FTP):
						case static_cast <uint8_t> (type_t::SSH):
						case static_cast <uint8_t> (type_t::HTTP):
						case static_cast <uint8_t> (type_t::HTTPS):
						case static_cast <uint8_t> (type_t::REDIS):
						case static_cast <uint8_t> (type_t::MYSQL):
						case static_cast <uint8_t> (type_t::SCHEME):
						case static_cast <uint8_t> (type_t::SOCKS5):
						case static_cast <uint8_t> (type_t::POSTGRESQL): {
							// Добавляем символ "?" перед первой парой ключ-значение параметров URI
							result.append("?");
							// Добавляем пары ключ-значение параметров URI в результат, разделяя их символом "&"
							for(const auto & [key, value] : this->_query)
								// Добавляем пару ключ-значение параметров URI в результат, разделяя их символом "=" и добавляя символ "&" после каждой пары
								result.append(this->_fmk->format("%s=%s&", uri::encode(key, this->_log).c_str(), uri::encode(value, this->_log).c_str()));
							// Удаляем последний символ "&" из результата
							result.pop_back();
						} break;
					}
				}
			} break;
			// Если режим элемента URI для генерации является якорем, то генерируем якорь URI
			case static_cast <uint8_t> (item_t::FRAGMENT): {
				// Если якорь URI не пустой, то добавляем его в результат
				if(!this->_fragment.empty()){
					/**
					 * Определяем тип URI адреса по схеме URI
					 */
					switch(static_cast <uint8_t> (this->_type)){
						// Если тип URI является WS, WSS, Scheme, HTTP или HTTPS
						case static_cast <uint8_t> (type_t::NONE):
						case static_cast <uint8_t> (type_t::WS):
						case static_cast <uint8_t> (type_t::WSS):
						case static_cast <uint8_t> (type_t::HTTP):
						case static_cast <uint8_t> (type_t::HTTPS):
						case static_cast <uint8_t> (type_t::SCHEME):
							// Добавляем якорь URI в результат, предваряя его символом "#"
							result.append(this->_fmk->format("#%s", uri::encode(this->_fragment, this->_log).c_str()));
						break;
					}
				}
			} break;
			// Если режим элемента URI для генерации является происхождением, то генерируем происхождение URI
			case static_cast <uint8_t> (item_t::ORIGIN): {
				// Если схема URI не пустая, то добавляем её в результат
				if(!this->_scheme.empty()){
					/**
					 * Определяем тип URI адреса по схеме URI
					 */
					switch(static_cast <uint8_t> (this->_type)){
						// Если тип URI является WS, WSS, FTP, UDS, FILE, MQTT, REDIS, MySQL, SOCKS5, PostgreSQL, HTTP или HTTPS, то добавляем схему URI в результат с "://"
						case static_cast <uint8_t> (type_t::WS):
						case static_cast <uint8_t> (type_t::WSS):
						case static_cast <uint8_t> (type_t::FTP):
						case static_cast <uint8_t> (type_t::UDS):
						case static_cast <uint8_t> (type_t::FILE):
						case static_cast <uint8_t> (type_t::MQTT):
						case static_cast <uint8_t> (type_t::HTTP):
						case static_cast <uint8_t> (type_t::HTTPS):
						case static_cast <uint8_t> (type_t::REDIS):
						case static_cast <uint8_t> (type_t::MYSQL):
						case static_cast <uint8_t> (type_t::SOCKS5):
						case static_cast <uint8_t> (type_t::POSTGRESQL):
							// Добавляем схему URI в результат
							result.append(this->_fmk->format("%s://", this->_scheme.c_str()));
						break;
						// Если тип URI является SSH, E-mail или Scheme, то добавляем схему URI в результат без "://"
						case static_cast <uint8_t> (type_t::SSH):
						case static_cast <uint8_t> (type_t::EMAIL):
						case static_cast <uint8_t> (type_t::SCHEME):
							// Добавляем схему URI в результат
							result.append(this->_fmk->format("%s:", this->_scheme.c_str()));
						break;
					}
				}
				// Если пользователь URI не пустой, то добавляем его в результат
				if(!this->_user.login.empty()){
					// Добавляем логин пользователя URI в результат
					result.append(this->_user.login);
					// Если пароль пользователя URI не пустой, то добавляем его в результат
					if(!this->_user.pass.empty())
						// Добавляем пароль пользователя URI в результат
						result.append(this->_fmk->format(":%s", this->_user.pass.c_str()));
					// Добавляем символ "@" после параметров пользователя URI
					result.append("@");
				}
				// Если атрибуты URI не пустые
				if(this->_attr != nullptr){
					/**
					 * Определяем режим формата URI для генерации
					 */
					switch(static_cast <uint8_t> (format)){
						// Если режим формата URI для генерации является полным, то генерируем полный URI
						case static_cast <uint8_t> (format_t::FULL): {
							/**
							 * Определяем тип атрибутов URI адреса
							 */
							switch(static_cast <uint8_t> (this->_attr->type)){
								// Если атрибуты URI адреса являются адресом файловой системы
								case static_cast <uint8_t> (net::type_t::FS):
									// Выводим путь к сокету из атрибутов URI адреса
									result.append(awh_cast <net::addr_fs_t *> (awh_cast <net::attr_uds_t *> (this->_attr.get())->path.get())->address);
								break;
								// Если атрибуты URI адреса являются FQDN-адресом
								case static_cast <uint8_t> (net::type_t::FQDN): {
									// Извлекаем атрибуты URI адреса как FQDN-адрес
									net::attr_fqdn_t * attr = awh_cast <net::attr_fqdn_t *> (this->_attr.get());
									// Выводим доменное имя хоста из атрибутов URI адреса
									result.append(attr->domain);
									// Если порт хоста в атрибутах URI адреса не равен 0, то добавляем его в результат
									if(attr->port != 0)
										// Добавляем порт хоста в результат, если он не равен 0
										result.append(this->_fmk->format(":%u", attr->port));
								} break;
								// Если атрибуты URI адреса являются IPv4-адресом
								case static_cast <uint8_t> (net::type_t::IPV4): {
									// Извлекаем атрибуты URI адреса как сетевой адрес
									net::attr_net_t * attr = awh_cast <net::attr_net_t *> (this->_attr.get());
									// Устанавливаем источник IP-адреса хоста в атрибутах URI адреса
									this->_addr->source(attr->ip.get(), net_addr_t::endian_t::LITTLE);
									// Выводим IP-адрес хоста в виде строки
									result.append(static_cast <string> (* this->_addr.get()));
									// Если порт хоста в атрибутах URI адреса не равен 0, то добавляем его в результат
									if(attr->port != 0)
										// Добавляем порт хоста в результат, если он не равен 0
										result.append(this->_fmk->format(":%u", attr->port));
								} break;
								// Если атрибуты URI адреса являются IPv6-адресом
								case static_cast <uint8_t> (net::type_t::IPV6): {
									// Извлекаем атрибуты URI адреса как сетевой адрес
									net::attr_net_t * attr = awh_cast <net::attr_net_t *> (this->_attr.get());
									// Устанавливаем источник IP-адреса хоста в атрибутах URI адреса
									this->_addr->source(attr->ip.get(), net_addr_t::endian_t::LITTLE);
									// Выводим IP-адрес хоста в виде строки
									result.append(this->_fmk->format("[%s]", static_cast <string> (* this->_addr.get()).c_str()));
									// Если порт хоста в атрибутах URI адреса не равен 0, то добавляем его в результат
									if(attr->port != 0)
										// Добавляем порт хоста в результат, если он не равен 0
										result.append(this->_fmk->format(":%u", attr->port));
								} break;
							}
						} break;
						// Если режим формата URI для генерации является умным, то генерируем краткий URI
						case static_cast <uint8_t> (format_t::SMART): {
							/**
							 * Определяем тип атрибутов URI адреса
							 */
							switch(static_cast <uint8_t> (this->_attr->type)){
								// Если атрибуты URI адреса являются адресом файловой системы
								case static_cast <uint8_t> (net::type_t::FS):
									// Выводим путь к сокету из атрибутов URI адреса
									result.append(awh_cast <net::addr_fs_t *> (awh_cast <net::attr_uds_t *> (this->_attr.get())->path.get())->address);
								break;
								// Если атрибуты URI адреса являются FQDN-адресом
								case static_cast <uint8_t> (net::type_t::FQDN): {
									// Извлекаем атрибуты URI адреса как FQDN-адрес
									net::attr_fqdn_t * attr = awh_cast <net::attr_fqdn_t *> (this->_attr.get());
									// Выводим доменное имя хоста из атрибутов URI адреса
									result.append(attr->domain);
									// Если порт хоста в атрибутах URI адреса не равен 0, то добавляем его в результат
									if(attr->port != 0){
										/**
										 * Определяем тип URI адреса по схеме URI
										 */
										switch(static_cast <uint8_t> (this->_type)){
											// Если тип URI является WS или HTTP, то устанавливаем порт URI как 80
											case static_cast <uint8_t> (type_t::WS):
											case static_cast <uint8_t> (type_t::HTTP):
												// Если порт URI является 80, то это может быть HTTP, иначе добавляем порт в результат
												if(attr->port != 80)
													// Добавляем порт хоста в результат, если он не равен 0
													result.append(this->_fmk->format(":%u", attr->port));
											break;
											// Если тип URI является WSS или HTTPS, то устанавливаем порт URI как 443
											case static_cast <uint8_t> (type_t::WSS):
											case static_cast <uint8_t> (type_t::HTTPS):
												// Если порт URI является 443, то это может быть HTTPS, иначе добавляем порт в результат
												if(attr->port != 443)
													// Добавляем порт хоста в результат, если он не равен 0
													result.append(this->_fmk->format(":%u", attr->port));
											break;
											// Если тип URI является SSH, то устанавливаем порт URI как 22
											case static_cast <uint8_t> (type_t::SSH):
												// Если порт URI является 22, то это может быть SSH, иначе добавляем порт в результат
												if(attr->port != 22)
													// Добавляем порт хоста в результат, если он не равен 0
													result.append(this->_fmk->format(":%u", attr->port));
											break;
											// Если тип URI является FTP, то устанавливаем порт URI как 21
											case static_cast <uint8_t> (type_t::FTP):
												// Если порт URI является 21, то это может быть FTP, иначе добавляем порт в результат
												if(attr->port != 21)
													// Добавляем порт хоста в результат, если он не равен 0
													result.append(this->_fmk->format(":%u", attr->port));
											break;
											// Если тип URI является MQTT, то устанавливаем порт URI как 1883
											case static_cast <uint8_t> (type_t::MQTT):
												// Если порт URI является 1883, то это может быть MQTT, иначе добавляем порт в результат
												if(attr->port != 1883)
													// Добавляем порт хоста в результат, если он не равен 0
													result.append(this->_fmk->format(":%u", attr->port));
											break;
											// Если тип URI является E-mail, то устанавливаем порт URI как 25
											case static_cast <uint8_t> (type_t::EMAIL):
												// Если порт URI является 25, то это может быть E-mail, иначе добавляем порт в результат
												if(attr->port != 25)
													// Добавляем порт хоста в результат, если он не равен 0
													result.append(this->_fmk->format(":%u", attr->port));
											break;
											// Если тип URI является Socks5, то устанавливаем порт URI как 1080
											case static_cast <uint8_t> (type_t::SOCKS5):
												// Если порт URI является 1080, то это может быть Socks5, иначе добавляем порт в результат
												if(attr->port != 1080)
													// Добавляем порт хоста в результат, если он не равен 0
													result.append(this->_fmk->format(":%u", attr->port));
											break;
											// Если тип URI является REDIS, то устанавливаем порт URI как 6379
											case static_cast <uint8_t> (type_t::REDIS):
												// Если порт URI является 6379, то это может быть REDIS, иначе добавляем порт в результат
												if(attr->port != 6379)
													// Добавляем порт хоста в результат, если он не равен 0
													result.append(this->_fmk->format(":%u", attr->port));
											break;
											// Если тип URI является MySQL, то устанавливаем порт URI как 3306
											case static_cast <uint8_t> (type_t::MYSQL):
												// Если порт URI является 3306, то это может быть MySQL, иначе добавляем порт в результат
												if(attr->port != 3306)
													// Добавляем порт хоста в результат, если он не равен 0
													result.append(this->_fmk->format(":%u", attr->port));
											break;
											// Если тип URI является PostgreSQL, то устанавливаем порт URI как 5432
											case static_cast <uint8_t> (type_t::POSTGRESQL):
												// Если порт URI является 5432, то это может быть PostgreSQL, иначе добавляем порт в результат
												if(attr->port != 5432)
													// Добавляем порт хоста в результат, если он не равен 0
													result.append(this->_fmk->format(":%u", attr->port));
											break;
										}
									}
								} break;
								// Если атрибуты URI адреса являются IPv4-адресом
								case static_cast <uint8_t> (net::type_t::IPV4): {
									// Извлекаем атрибуты URI адреса как сетевой адрес
									net::attr_net_t * attr = awh_cast <net::attr_net_t *> (this->_attr.get());
									// Устанавливаем источник IP-адреса хоста в атрибутах URI адреса
									this->_addr->source(attr->ip.get(), net_addr_t::endian_t::LITTLE);
									// Выводим IP-адрес хоста в виде строки
									result.append(static_cast <string> (* this->_addr.get()));
									// Если порт хоста в атрибутах URI адреса не равен 0, то добавляем его в результат
									if(attr->port != 0){
										/**
										 * Определяем тип URI адреса по схеме URI
										 */
										switch(static_cast <uint8_t> (this->_type)){
											// Если тип URI является WS или HTTP, то устанавливаем порт URI как 80
											case static_cast <uint8_t> (type_t::WS):
											case static_cast <uint8_t> (type_t::HTTP):
												// Если порт URI является 80, то это может быть HTTP, иначе добавляем порт в результат
												if(attr->port != 80)
													// Добавляем порт хоста в результат, если он не равен 0
													result.append(this->_fmk->format(":%u", attr->port));
											break;
											// Если тип URI является WSS или HTTPS, то устанавливаем порт URI как 443
											case static_cast <uint8_t> (type_t::WSS):
											case static_cast <uint8_t> (type_t::HTTPS):
												// Если порт URI является 443, то это может быть HTTPS, иначе добавляем порт в результат
												if(attr->port != 443)
													// Добавляем порт хоста в результат, если он не равен 0
													result.append(this->_fmk->format(":%u", attr->port));
											break;
											// Если тип URI является SSH, то устанавливаем порт URI как 22
											case static_cast <uint8_t> (type_t::SSH):
												// Если порт URI является 22, то это может быть SSH, иначе добавляем порт в результат
												if(attr->port != 22)
													// Добавляем порт хоста в результат, если он не равен 0
													result.append(this->_fmk->format(":%u", attr->port));
											break;
											// Если тип URI является FTP, то устанавливаем порт URI как 21
											case static_cast <uint8_t> (type_t::FTP):
												// Если порт URI является 21, то это может быть FTP, иначе добавляем порт в результат
												if(attr->port != 21)
													// Добавляем порт хоста в результат, если он не равен 0
													result.append(this->_fmk->format(":%u", attr->port));
											break;
											// Если тип URI является MQTT, то устанавливаем порт URI как 1883
											case static_cast <uint8_t> (type_t::MQTT):
												// Если порт URI является 1883, то это может быть MQTT, иначе добавляем порт в результат
												if(attr->port != 1883)
													// Добавляем порт хоста в результат, если он не равен 0
													result.append(this->_fmk->format(":%u", attr->port));
											break;
											// Если тип URI является E-mail, то устанавливаем порт URI как 25
											case static_cast <uint8_t> (type_t::EMAIL):
												// Если порт URI является 25, то это может быть E-mail, иначе добавляем порт в результат
												if(attr->port != 25)
													// Добавляем порт хоста в результат, если он не равен 0
													result.append(this->_fmk->format(":%u", attr->port));
											break;
											// Если тип URI является Socks5, то устанавливаем порт URI как 1080
											case static_cast <uint8_t> (type_t::SOCKS5):
												// Если порт URI является 1080, то это может быть Socks5, иначе добавляем порт в результат
												if(attr->port != 1080)
													// Добавляем порт хоста в результат, если он не равен 0
													result.append(this->_fmk->format(":%u", attr->port));
											break;
											// Если тип URI является REDIS, то устанавливаем порт URI как 6379
											case static_cast <uint8_t> (type_t::REDIS):
												// Если порт URI является 6379, то это может быть REDIS, иначе добавляем порт в результат
												if(attr->port != 6379)
													// Добавляем порт хоста в результат, если он не равен 0
													result.append(this->_fmk->format(":%u", attr->port));
											break;
											// Если тип URI является MySQL, то устанавливаем порт URI как 3306
											case static_cast <uint8_t> (type_t::MYSQL):
												// Если порт URI является 3306, то это может быть MySQL, иначе добавляем порт в результат
												if(attr->port != 3306)
													// Добавляем порт хоста в результат, если он не равен 0
													result.append(this->_fmk->format(":%u", attr->port));
											break;
											// Если тип URI является PostgreSQL, то устанавливаем порт URI как 5432
											case static_cast <uint8_t> (type_t::POSTGRESQL):
												// Если порт URI является 5432, то это может быть PostgreSQL, иначе добавляем порт в результат
												if(attr->port != 5432)
													// Добавляем порт хоста в результат, если он не равен 0
													result.append(this->_fmk->format(":%u", attr->port));
											break;
										}
									}
								} break;
								// Если атрибуты URI адреса являются IPv6-адресом
								case static_cast <uint8_t> (net::type_t::IPV6): {
									// Извлекаем атрибуты URI адреса как сетевой адрес
									net::attr_net_t * attr = awh_cast <net::attr_net_t *> (this->_attr.get());
									// Устанавливаем источник IP-адреса хоста в атрибутах URI адреса
									this->_addr->source(attr->ip.get(), net_addr_t::endian_t::LITTLE);
									// Выводим IP-адрес хоста в виде строки
									result.append(this->_fmk->format("[%s]", static_cast <string> (* this->_addr.get()).c_str()));
									// Если порт хоста в атрибутах URI адреса не равен 0, то добавляем его в результат
									if(attr->port != 0){
										/**
										 * Определяем тип URI адреса по схеме URI
										 */
										switch(static_cast <uint8_t> (this->_type)){
											// Если тип URI является WS или HTTP, то устанавливаем порт URI как 80
											case static_cast <uint8_t> (type_t::WS):
											case static_cast <uint8_t> (type_t::HTTP):
												// Если порт URI является 80, то это может быть HTTP, иначе добавляем порт в результат
												if(attr->port != 80)
													// Добавляем порт хоста в результат, если он не равен 0
													result.append(this->_fmk->format(":%u", attr->port));
											break;
											// Если тип URI является WSS или HTTPS, то устанавливаем порт URI как 443
											case static_cast <uint8_t> (type_t::WSS):
											case static_cast <uint8_t> (type_t::HTTPS):
												// Если порт URI является 443, то это может быть HTTPS, иначе добавляем порт в результат
												if(attr->port != 443)
													// Добавляем порт хоста в результат, если он не равен 0
													result.append(this->_fmk->format(":%u", attr->port));
											break;
											// Если тип URI является SSH, то устанавливаем порт URI как 22
											case static_cast <uint8_t> (type_t::SSH):
												// Если порт URI является 22, то это может быть SSH, иначе добавляем порт в результат
												if(attr->port != 22)
													// Добавляем порт хоста в результат, если он не равен 0
													result.append(this->_fmk->format(":%u", attr->port));
											break;
											// Если тип URI является FTP, то устанавливаем порт URI как 21
											case static_cast <uint8_t> (type_t::FTP):
												// Если порт URI является 21, то это может быть FTP, иначе добавляем порт в результат
												if(attr->port != 21)
													// Добавляем порт хоста в результат, если он не равен 0
													result.append(this->_fmk->format(":%u", attr->port));
											break;
											// Если тип URI является MQTT, то устанавливаем порт URI как 1883
											case static_cast <uint8_t> (type_t::MQTT):
												// Если порт URI является 1883, то это может быть MQTT, иначе добавляем порт в результат
												if(attr->port != 1883)
													// Добавляем порт хоста в результат, если он не равен 0
													result.append(this->_fmk->format(":%u", attr->port));
											break;
											// Если тип URI является E-mail, то устанавливаем порт URI как 25
											case static_cast <uint8_t> (type_t::EMAIL):
												// Если порт URI является 25, то это может быть E-mail, иначе добавляем порт в результат
												if(attr->port != 25)
													// Добавляем порт хоста в результат, если он не равен 0
													result.append(this->_fmk->format(":%u", attr->port));
											break;
											// Если тип URI является Socks5, то устанавливаем порт URI как 1080
											case static_cast <uint8_t> (type_t::SOCKS5):
												// Если порт URI является 1080, то это может быть Socks5, иначе добавляем порт в результат
												if(attr->port != 1080)
													// Добавляем порт хоста в результат, если он не равен 0
													result.append(this->_fmk->format(":%u", attr->port));
											break;
											// Если тип URI является REDIS, то устанавливаем порт URI как 6379
											case static_cast <uint8_t> (type_t::REDIS):
												// Если порт URI является 6379, то это может быть REDIS, иначе добавляем порт в результат
												if(attr->port != 6379)
													// Добавляем порт хоста в результат, если он не равен 0
													result.append(this->_fmk->format(":%u", attr->port));
											break;
											// Если тип URI является MySQL, то устанавливаем порт URI как 3306
											case static_cast <uint8_t> (type_t::MYSQL):
												// Если порт URI является 3306, то это может быть MySQL, иначе добавляем порт в результат
												if(attr->port != 3306)
													// Добавляем порт хоста в результат, если он не равен 0
													result.append(this->_fmk->format(":%u", attr->port));
											break;
											// Если тип URI является PostgreSQL, то устанавливаем порт URI как 5432
											case static_cast <uint8_t> (type_t::POSTGRESQL):
												// Если порт URI является 5432, то это может быть PostgreSQL, иначе добавляем порт в результат
												if(attr->port != 5432)
													// Добавляем порт хоста в результат, если он не равен 0
													result.append(this->_fmk->format(":%u", attr->port));
											break;
										}
									}
								} break;
							}
						} break;
					}
				}
			} break;
			// Если режим элемента URI для генерации является запросом, то генерируем относительный URI-запрос
			case static_cast <uint8_t> (item_t::REQUEST): {
				/**
				 * Определяем тип URI адреса по схеме URI
				 */
				switch(static_cast <uint8_t> (this->_type)){
					// Если тип URI является WS, WSS, FTP, UDS, FILE, MQTT, REDIS, MySQL, SOCKS5, PostgreSQL, HTTP или HTTPS, то добавляем схему URI в результат с "://"
					case static_cast <uint8_t> (type_t::NONE):
					case static_cast <uint8_t> (type_t::WS):
					case static_cast <uint8_t> (type_t::WSS):
					case static_cast <uint8_t> (type_t::FTP):
					case static_cast <uint8_t> (type_t::UDS):
					case static_cast <uint8_t> (type_t::FILE):
					case static_cast <uint8_t> (type_t::MQTT):
					case static_cast <uint8_t> (type_t::HTTP):
					case static_cast <uint8_t> (type_t::HTTPS):
					case static_cast <uint8_t> (type_t::REDIS):
					case static_cast <uint8_t> (type_t::MYSQL):
					case static_cast <uint8_t> (type_t::SOCKS5):
					case static_cast <uint8_t> (type_t::POSTGRESQL): {
						// Если путь URI не пустой, то добавляем его в результат
						if(!this->_path.empty()){
							// Добавляем символ "/" перед каждым сегментом пути URI и добавляем сегменты пути URI в результат
							for(const string & segment : this->_path){
								// Добавляем символ "/" перед сегментом пути URI
								result.append(1, '/');
								// Добавляем сегмент пути URI в результат
								result.append(uri::encode(segment, this->_log));
							}
						// Если путь URI пустой, но параметры URI не пустые, то добавляем символ "/" в результат перед параметрами URI
						} else result.append(1, '/');
						// Если параметры URI не пустые, то добавляем их в результат
						if(!this->_query.empty()){
							// Добавляем символ "?" перед первой парой ключ-значение параметров URI
							result.append("?");
							// Добавляем пары ключ-значение параметров URI в результат, разделяя их символом "&"
							for(const auto & [key, value] : this->_query)
								// Добавляем пару ключ-значение параметров URI в результат, разделяя их символом "=" и добавляя символ "&" после каждой пары
								result.append(this->_fmk->format("%s=%s&", uri::encode(key, this->_log).c_str(), uri::encode(value, this->_log).c_str()));
							// Если функция обратного вызова установлена
							if(this->_callback != nullptr){
								// Выполняем генерацию параметров URI с помощью функции обратного вызова
								const string & item = this->_callback(this);
								// Если параметры URI, сгенерированные удачно
								if(!item.empty())
									// Вызываем функцию обратного вызова и добавляем её результат в результат
									result.append(item);
							// Удаляем последний символ "&" из результата
							} else result.pop_back();
						}
						// Если якорь URI не пустой, то добавляем его в результат
						if(!this->_fragment.empty())
							// Добавляем якорь URI в результат, предваряя его символом "#"
							result.append(this->_fmk->format("#%s", uri::encode(this->_fragment, this->_log).c_str()));
					} break;
					// Если тип URI является E-mail или Scheme, то добавляем схему URI в результат без "://"
					case static_cast <uint8_t> (type_t::EMAIL): {
						// Если пользователь URI не пустой, то добавляем его в результат
						if(!this->_user.login.empty()){
							// Добавляем логин пользователя URI в результат
							result.append(this->_user.login);
							// Если пароль пользователя URI не пустой, то добавляем его в результат
							if(!this->_user.pass.empty())
								// Добавляем пароль пользователя URI в результат
								result.append(this->_fmk->format(":%s", this->_user.pass.c_str()));
							// Добавляем символ "@" после параметров пользователя URI
							result.append("@");
						}
						/**
						 * Определяем тип атрибутов URI адреса
						 */
						switch(static_cast <uint8_t> (this->_attr->type)){
							// Если атрибуты URI адреса являются FQDN-адресом
							case static_cast <uint8_t> (net::type_t::FQDN): {
								// Извлекаем атрибуты URI адреса как FQDN-адрес
								net::attr_fqdn_t * attr = awh_cast <net::attr_fqdn_t *> (this->_attr.get());
								// Выводим доменное имя хоста из атрибутов URI адреса
								result.append(attr->domain);
								// Если порт хоста в атрибутах URI адреса не равен 0 и не равен 25, то добавляем его в результат
								if((attr->port != 0) && (attr->port != 25))
									// Добавляем порт хоста в результат, если он не равен 0
									result.append(this->_fmk->format(":%u", attr->port));
							} break;
							// Если атрибуты URI адреса являются IPv4-адресом
							case static_cast <uint8_t> (net::type_t::IPV4): {
								// Извлекаем атрибуты URI адреса как сетевой адрес
								net::attr_net_t * attr = awh_cast <net::attr_net_t *> (this->_attr.get());
								// Устанавливаем источник IP-адреса хоста в атрибутах URI адреса
								this->_addr->source(attr->ip.get(), net_addr_t::endian_t::LITTLE);
								// Выводим IP-адрес хоста в виде строки
								result.append(static_cast <string> (* this->_addr.get()));
								// Если порт хоста в атрибутах URI адреса не равен 0 и не равен 25, то добавляем его в результат
								if((attr->port != 0) && (attr->port != 25))
									// Добавляем порт хоста в результат, если он не равен 0
									result.append(this->_fmk->format(":%u", attr->port));
							} break;
							// Если атрибуты URI адреса являются IPv6-адресом
							case static_cast <uint8_t> (net::type_t::IPV6): {
								// Извлекаем атрибуты URI адреса как сетевой адрес
								net::attr_net_t * attr = awh_cast <net::attr_net_t *> (this->_attr.get());
								// Устанавливаем источник IP-адреса хоста в атрибутах URI адреса
								this->_addr->source(attr->ip.get(), net_addr_t::endian_t::LITTLE);
								// Выводим IP-адрес хоста в виде строки
								result.append(this->_fmk->format("[%s]", static_cast <string> (* this->_addr.get()).c_str()));
								// Если порт хоста в атрибутах URI адреса не равен 0 и не равен 25, то добавляем его в результат
								if((attr->port != 0) && (attr->port != 25))
									// Добавляем порт хоста в результат, если он не равен 0
									result.append(this->_fmk->format(":%u", attr->port));
							} break;
						}
					} break;
					// Если тип URI является SSH или Scheme
					case static_cast <uint8_t> (type_t::SSH):
					case static_cast <uint8_t> (type_t::SCHEME): {
						// Если путь URI не пустой, то добавляем его в результат
						if(!this->_path.empty()){
							// Добавляем символ "/" перед каждым сегментом пути URI и добавляем сегменты пути URI в результат
							for(auto i = this->_path.begin(); i != this->_path.end(); ++i){
								// Если это не первый сегмент пути URI, то добавляем символ "/" перед ним
								if(i != this->_path.begin())
									// Добавляем символ "/" перед сегментом пути URI
									result.append(1, '/');
								// Добавляем сегмент пути URI в результат
								result.append(uri::encode(* i, this->_log));
							}
						}
					} break;
				}
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
			// Выводим сообщение об ошибке
			this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(static_cast <uint16_t> (format)), log_t::flag_t::CRITICAL, error.what());
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
 * @brief Метод установки функции обратного вызова для генерации параметра URI (например, для генерации контрольной суммы)
 *
 * @param cb функция обратного вызова для генерации параметра URI
 */
void awh::Uniform_Resource_Identifier::callback(function <string (const Uniform_Resource_Identifier *)> cb) noexcept {
	// Устанавливаем функцию обратного вызова для генерации параметра URI
	this->_callback = ::move(cb);
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
				// Выполняем сравнение размеров параметров URI
				result = (this->_query.size() == uri._query.size());
				// Если размеры параметров URI равны
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
				// Выполняем сравнение размеров параметров URI
				result = (this->_query.size() != uri._query.size());
				// Если размеры параметров URI равны
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
 * @brief Оператор установки параметров пользователя URI
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
 * @brief Оператор [=] присваивания параметров URI
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
 _type(type_t::NONE), _scheme{""}, _fragment{""},
 _addr(nullptr), _attr(nullptr), _callback(nullptr), _fmk(nullptr), _log(nullptr) {
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
 _type(type_t::NONE), _scheme{""}, _fragment{""},
 _addr(nullptr), _attr(nullptr), _callback(nullptr), _fmk(nullptr), _log(nullptr) {
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
 * @brief Конструктор
 *
 * @param fmk объект фреймворка
 * @param log объект для работы с логами
 */
awh::Uniform_Resource_Identifier::Uniform_Resource_Identifier(const fmk_t * fmk, const log_t * log) noexcept :
 _type(type_t::NONE), _scheme{""}, _fragment{""},
 _addr(nullptr), _attr(nullptr), _callback(nullptr), _fmk(fmk), _log(log) {
	// Инициализируем объект работы с сетевыми адресами
	this->_addr = make_unique <net_addr_t> (fmk, log);
}
/**
 * @brief Деструктор
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
