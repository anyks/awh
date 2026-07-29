/**
 * @file: http.cpp
 * @date: 2026-07-27
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @brief Реализация парсера сессии HTTP/3 (RFC 9114) — разбор потоков запросов и однонаправленных
 *        потоков соединения, управляющий поток, семантика HTTP, лимиты безопасности
 *
 * @copyright: Copyright © 2026
 *
 */

/**
 * Подключаем заголовочный файл проекта
 */
#include <proto/http/parser/http3/http.hpp>

/**
 * Стандартные заголовочные файлы
 */
#include <algorithm>

/**
 * Используем стандартное пространство имён
 */
using namespace std;
using namespace awh;

/**
 * @brief Пространство имён внутренних функций парсера
 *
 */
namespace {
	/**
	 * @brief Пространство имён названий полей, участвующих в сравнениях
	 *
	 * @details Названия вынесены константами намеренно. Сравнение вида
	 *          `name.compare("upgrade")` строит представление из указателя,
	 *          а значит вычисляет длину литерала вызовом strlen во время
	 *          выполнения - на каждое поле каждой секции. У константы
	 *          длина известна на этапе компиляции
	 *
	 */
	namespace header {
		// Псевдо-заголовки запроса и ответа (RFC 9114 §4.3)
		static constexpr string_view METHOD = ":method";
		static constexpr string_view SCHEME = ":scheme";
		static constexpr string_view PATH = ":path";
		static constexpr string_view AUTHORITY = ":authority";
		static constexpr string_view PROTOCOL = ":protocol";
		static constexpr string_view STATUS = ":status";
		// Поля, запрещённые в HTTP/3 (RFC 9114 §4.2)
		static constexpr string_view UPGRADE = "upgrade";
		static constexpr string_view KEEP_ALIVE = "keep-alive";
		static constexpr string_view CONNECTION = "connection";
		static constexpr string_view PROXY_CONNECTION = "proxy-connection";
		static constexpr string_view TRANSFER_ENCODING = "transfer-encoding";
		// Поля, запрещённые в секции трейлеров
		static constexpr string_view TE = "te";
		static constexpr string_view HOST = "host";
		static constexpr string_view RANGE = "range";
		static constexpr string_view EXPECT = "expect";
		static constexpr string_view TRAILER = "trailer";
		static constexpr string_view CONTENT_TYPE = "content-type";
		static constexpr string_view CACHE_CONTROL = "cache-control";
		static constexpr string_view CONTENT_RANGE = "content-range";
		static constexpr string_view MAX_FORWARDS = "max-forwards";
		static constexpr string_view AUTHORIZATION = "authorization";
		static constexpr string_view CONTENT_LENGTH = "content-length";
		static constexpr string_view CONTENT_ENCODING = "content-encoding";
		static constexpr string_view PROXY_AUTHORIZATION = "proxy-authorization";
		// Заголовки, запрещённые в трейлерах (RFC 9110 §6.5.1)
		static constexpr string_view AGE = "age";
		static constexpr string_view AUTHENTICATION_INFO = "authentication-info";
		static constexpr string_view COOKIE = "cookie";
		static constexpr string_view DATE = "date";
		static constexpr string_view EXPIRES = "expires";
		static constexpr string_view IF_MATCH = "if-match";
		static constexpr string_view IF_MODIFIED_SINCE = "if-modified-since";
		static constexpr string_view IF_NONE_MATCH = "if-none-match";
		static constexpr string_view IF_RANGE = "if-range";
		static constexpr string_view IF_UNMODIFIED_SINCE = "if-unmodified-since";
		static constexpr string_view LOCATION = "location";
		static constexpr string_view PRAGMA = "pragma";
		static constexpr string_view PROXY_AUTHENTICATE = "proxy-authenticate";
		static constexpr string_view PROXY_AUTHENTICATION_INFO = "proxy-authentication-info";
		static constexpr string_view RETRY_AFTER = "retry-after";
		static constexpr string_view SET_COOKIE = "set-cookie";
		static constexpr string_view VARY = "vary";
		static constexpr string_view WARNING = "warning";
		static constexpr string_view WWW_AUTHENTICATE = "www-authenticate";
		// Заголовок приоритета запроса (RFC 9218 §5)
		static constexpr string_view PRIORITY = "priority";
	};
	/**
	 * @brief Пространство имён значений полей, участвующих в сравнениях
	 *
	 */
	namespace value {
		// Методы запроса, меняющие обработку тела
		static constexpr string_view CONNECT = "CONNECT";
		static constexpr string_view HEAD = "HEAD";
		// Метод, которому допустима звёздочка вместо пути (RFC 9110 §7.1)
		static constexpr string_view OPTIONS = "OPTIONS";
		// Единственное допустимое значение поля [te] (RFC 9114 §4.2)
		static constexpr string_view TRAILERS = "trailers";
		// Коды состояния, при которых тело ответа отсутствует
		static constexpr string_view NO_CONTENT = "204";
		static constexpr string_view NOT_MODIFIED = "304";
		// Элементы структурированного поля приоритета (RFC 9218 §4)
		static constexpr string_view INCREMENTAL_ON = "i=?1";
		static constexpr string_view INCREMENTAL_OFF = "i=?0";
	};

	/**
	 * @brief Функция проверки принадлежности символа к набору token (RFC 9110 §5.6.2)
	 *
	 * @param letter проверяемый символ
	 * @return       результат проверки
	 *
	 */
	bool isToken(const char letter) noexcept {
		/**
		 * Таблица допустимых символов строится один раз: посимвольная проверка
		 * набором сравнений обошлась бы дороже обращения к таблице
		 *
		 * @note Прописные буквы в таблице отсутствуют намеренно: названия полей
		 *       в HTTP/3 обязаны быть в нижнем регистре (RFC 9114 §4.2), и
		 *       отдельной проверки регистра не требуется
		 */
		static const array <bool, 256> TABLE = [](){
			// Собираемая таблица допустимых символов
			array <bool, 256> table{};
			/**
			 * Выполняем перебор всех специальных символов набора token
			 */
			for(const uint8_t item : string_view("!#$%&'*+-.^_`|~"))
				// Помечаем символ допустимым
				table[item] = true;
			/**
			 * Выполняем перебор всех цифр
			 */
			for(uint8_t item = '0'; item <= '9'; item++)
				// Помечаем символ допустимым
				table[item] = true;
			/**
			 * Выполняем перебор всех строчных букв
			 */
			for(uint8_t item = 'a'; item <= 'z'; item++)
				// Помечаем символ допустимым
				table[item] = true;
			// Выводим собранную таблицу
			return table;
		}();
		// Выводим результат проверки
		return TABLE[static_cast <uint8_t> (letter)];
	}
	/**
	 * @brief Функция проверки названия поля (RFC 9114 §4.2)
	 *
	 * @details Название обязано состоять из символов набора token и не содержать
	 *          прописных букв. Пустое название недопустимо
	 *
	 * @param name проверяемое название поля
	 * @return     результат проверки
	 *
	 */
	/**
	 * @brief Функция проверки принадлежности схемы запроса протоколу HTTP
	 *
	 * @details Форму цели запроса задают только схемы [http] и [https]: для прочих
	 *          её задаёт не HTTP, и проверять путь по правилам HTTP нельзя.
	 *          Схема в URI регистронезависима (RFC 3986 §3.1)
	 *
	 * @param scheme значение псевдо-заголовка схемы
	 * @return       результат проверки
	 *
	 */
	bool httpScheme(string_view scheme) noexcept {
		// Если длина схемы не совпадает ни с одной из проверяемых
		if((scheme.size() != 4) && (scheme.size() != 5))
			// Схема протоколу HTTP не принадлежит
			return false;
		// Собираемая схема в нижнем регистре
		char letters[5] = {0};
		/**
		 * Выполняем приведение схемы к нижнему регистру
		 */
		for(size_t i = 0; i < scheme.size(); i++)
			// Приводим очередной символ схемы к нижнему регистру
			letters[i] = static_cast <char> (((scheme[i] >= 'A') && (scheme[i] <= 'Z')) ? (scheme[i] + 32) : scheme[i]);
		// Выводим результат сравнения схемы с проверяемыми
		return ((string_view(letters, scheme.size()) == "http") || (string_view(letters, scheme.size()) == "https"));
	}
	bool validName(string_view name) noexcept {
		// Если название поля пустое
		if(name.empty())
			// Выводим отрицательный результат
			return false;
		/**
		 * Псевдо-заголовок начинается с двоеточия, которое в набор token не входит:
		 * проверяем остаток названия, а само двоеточие пропускаем
		 */
		const size_t offset = ((name.front() == ':') ? 1 : 0);
		// Если у псевдо-заголовка нет ничего, кроме двоеточия
		if(name.size() == offset)
			// Выводим отрицательный результат
			return false;
		/**
		 * Выполняем перебор всех символов названия поля
		 */
		for(size_t i = offset; i < name.size(); i++){
			// Если символ не принадлежит набору token
			if(!isToken(name[i]))
				// Выводим отрицательный результат
				return false;
		}
		// Выводим положительный результат
		return true;
	}
	/**
	 * @brief Функция проверки значения поля (RFC 9114 §4.2)
	 *
	 * @details Значение не должно содержать управляющих символов NUL, CR и LF,
	 *          а также обрамляющих пробелов: без этой проверки трансляция
	 *          сообщения в HTTP/1.1 позволяла бы расщепить его на два
	 *
	 * @param value проверяемое значение поля
	 * @return      результат проверки
	 *
	 */
	bool validValue(string_view value) noexcept {
		// Если значение поля пустое - это допустимо
		if(value.empty())
			// Выводим положительный результат
			return true;
		// Если значение обрамлено пробелом либо табуляцией
		if((value.front() == ' ') || (value.front() == '\t') || (value.back() == ' ') || (value.back() == '\t'))
			// Выводим отрицательный результат
			return false;
		/**
		 * Выполняем перебор всех символов значения поля
		 */
		for(const char letter : value){
			// Если символ является запрещённым управляющим
			if((letter == '\0') || (letter == '\r') || (letter == '\n'))
				// Выводим отрицательный результат
				return false;
		}
		// Выводим положительный результат
		return true;
	}
	/**
	 * @brief Функция проверки пригодности значения поля к трансляции в другую версию HTTP (RFC 9114 §10.3)
	 *
	 * @details Минимальная проверка отсеивает только NUL, CR и LF: конечному
	 *          получателю прочие управляющие символы безразличны, он передаёт
	 *          значение приложению как есть. Узлу, транслирующему сообщение в
	 *          другую версию протокола, они безразличны быть не могут: грамматика
	 *          field-content (RFC 9110 §5.5) управляющих символов не допускает,
	 *          а вертикальную табуляцию и перевод страницы часть реализаций
	 *          HTTP/1.1 разбирает как разделители строк. Звено, прочитавшее их
	 *          именно так, определит границу поля иначе
	 *
	 * @param value проверяемое значение поля
	 * @return      результат проверки
	 *
	 */
	bool translatable(string_view value) noexcept {
		/**
		 * Выполняем перебор всех символов значения поля
		 */
		for(const char item : value){
			// Получаем очередной символ значения поля
			const uint8_t letter = static_cast <uint8_t> (item);
			// Управляющие символы (кроме табуляции) и DEL в значение поля не входят (RFC 9110 §5.5)
			if(((letter < 0x20) && (letter != 0x09)) || (letter == 0x7F))
				// Выводим отрицательный результат
				return false;
		}
		// Выводим положительный результат
		return true;
	}
	/**
	 * @brief Функция проверки пригодности значения псевдо-заголовка к сборке стартовой строки (RFC 9114 §10.3)
	 *
	 * @details Из [:scheme], [:authority] и [:path] следующее звено собирает цель
	 *          запроса, а из неё вместе с методом - стартовую строку HTTP/1.1.
	 *          Пробел, табуляция и управляющие символы расщепляют эту строку на
	 *          элементы, которых отправитель туда не помещал
	 *
	 * @param value проверяемое значение псевдо-заголовка
	 * @return      результат проверки
	 *
	 */
	bool translatablePseudo(string_view value) noexcept {
		/**
		 * Выполняем перебор всех символов значения псевдо-заголовка
		 */
		for(const char item : value){
			// Получаем очередной символ значения псевдо-заголовка
			const uint8_t letter = static_cast <uint8_t> (item);
			// Пробел, табуляция, управляющие символы и DEL стартовую строку расщепляют
			if((letter <= 0x20) || (letter == 0x7F))
				// Выводим отрицательный результат
				return false;
		}
		// Выводим положительный результат
		return true;
	}
	/**
	 * @brief Функция проверки принадлежности значения к набору token (RFC 9110 §5.6.2)
	 *
	 * @note От isToken отличается тем, что заглавные латинские буквы допускает:
	 *       ограничение нижним регистром относится к названиям полей HTTP/3,
	 *       а метод запроса регистрозависим и заглавные буквы несут как раз
	 *       все стандартные методы
	 *
	 * @param value проверяемое значение
	 * @return      результат проверки
	 *
	 */
	bool tokenValue(string_view value) noexcept {
		// Пустое значение токеном не является
		if(value.empty())
			// Выводим отрицательный результат
			return false;
		/**
		 * Выполняем перебор всех символов значения
		 */
		for(const char letter : value){
			// Если символ является заглавной латинской буквой - он допустим
			if((letter >= 'A') && (letter <= 'Z'))
				// Переходим к следующему символу значения
				continue;
			// Если символ не принадлежит набору token
			if(!isToken(letter))
				// Выводим отрицательный результат
				return false;
		}
		// Выводим положительный результат
		return true;
	}
	/**
	 * @brief Функция проверки названия поля на псевдо-заголовок
	 *
	 * @param name проверяемое название поля
	 * @return     результат проверки
	 *
	 */
	bool isPseudo(string_view name) noexcept {
		// Выводим признак псевдо-заголовка
		return (!name.empty() && (name.front() == ':'));
	}
	/**
	 * @brief Функция проверки названия поля на запрещённое в HTTP/3 (RFC 9114 §4.2)
	 *
	 * @details Поля управления соединением принадлежат HTTP/1.1 и в HTTP/3 смысла
	 *          не имеют: соединением распоряжается транспорт. Их пропуск позволил бы
	 *          протащить управляющую семантику через шлюз
	 *
	 * @param name проверяемое название поля
	 * @return     результат проверки
	 *
	 */
	bool isForbidden(string_view name) noexcept {
		// Выводим признак запрещённого поля
		return (
			(name == header::CONNECTION) || (name == header::KEEP_ALIVE) ||
			(name == header::UPGRADE) || (name == header::PROXY_CONNECTION) ||
			(name == header::TRANSFER_ENCODING)
		);
	}
	/**
	 * @brief Функция проверки названия поля на недопустимое в секции трейлеров
	 *
	 * @details Перечень взят из RFC 9110 §6.5.1: это поля, влияющие на обработку
	 *          сообщения, маршрутизацию либо аутентификацию. Их появление после
	 *          тела означало бы, что получатель обязан пересмотреть уже принятое
	 *
	 * @param name проверяемое название поля
	 * @return     результат проверки
	 *
	 */
	bool isForbiddenTrailer(string_view name) noexcept {
		/**
		 * Выполняем проверку названия по списку запрещённых (диспетчер по первой букве):
		 * названия полей в этой версии протокола обязаны быть в нижнем регистре,
		 * поэтому регистр приводить не нужно, а перебор всего списка на каждое поле
		 * секции обходился бы тем дороже, чем полнее список
		 */
		switch(name.empty() ? '\0' : name.front()){
			// Поля управляющих данных ответа и аутентификации
			case 'a': return ((name == header::AGE) || (name == header::AUTHORIZATION) || (name == header::AUTHENTICATION_INFO));
			// Поля обработки содержимого, управления кэшированием и состояния сессии
			case 'c': return (
				(name == header::COOKIE) || (name == header::CONTENT_TYPE) || (name == header::CACHE_CONTROL) ||
				(name == header::CONTENT_RANGE) || (name == header::CONTENT_LENGTH) || (name == header::CONTENT_ENCODING)
			);
			// Поле отметки времени сообщения
			case 'd': return (name == header::DATE);
			// Поля ожидания промежуточного ответа и срока годности ответа
			case 'e': return ((name == header::EXPECT) || (name == header::EXPIRES));
			// Поле целевого узла
			case 'h': return (name == header::HOST);
			// Условные поля запроса
			case 'i': return (
				(name == header::IF_MATCH) || (name == header::IF_RANGE) || (name == header::IF_NONE_MATCH) ||
				(name == header::IF_MODIFIED_SINCE) || (name == header::IF_UNMODIFIED_SINCE)
			);
			// Поле перенаправления
			case 'l': return (name == header::LOCATION);
			// Поле ограничения числа промежуточных узлов
			case 'm': return (name == header::MAX_FORWARDS);
			// Поля аутентификации на прокси и совместимости с HTTP/1.0
			case 'p': return (
				(name == header::PRAGMA) || (name == header::PROXY_AUTHENTICATE) ||
				(name == header::PROXY_AUTHORIZATION) || (name == header::PROXY_AUTHENTICATION_INFO)
			);
			// Поля запроса диапазона и указания времени повтора
			case 'r': return ((name == header::RANGE) || (name == header::RETRY_AFTER));
			// Поле установки сессионных данных
			case 's': return (name == header::SET_COOKIE);
			// Поля объявления трейлеров и расширений передачи
			case 't': return ((name == header::TE) || (name == header::TRAILER));
			// Поле ключа вариативности кэша
			case 'v': return (name == header::VARY);
			// Поля запроса аутентификации и предупреждения о содержимом
			case 'w': return ((name == header::WARNING) || (name == header::WWW_AUTHENTICATE));
		}
		// Поле в секции трейлеров разрешено
		return false;
	}
	/**
	 * @brief Функция разбора значения поля [content-length]
	 *
	 * @param value  значение поля
	 * @param output разобранное значение
	 * @return       результат разбора
	 *
	 */
	bool parseLength(string_view value, uint64_t & output) noexcept {
		// Если значение поля пустое
		if(value.empty())
			// Выводим отрицательный результат
			return false;
		// Накапливаемое значение
		uint64_t result = 0;
		/**
		 * Выполняем перебор всех символов значения поля
		 */
		for(const char letter : value){
			// Если символ не является цифрой
			if((letter < '0') || (letter > '9'))
				// Выводим отрицательный результат
				return false;
			// Если накопление переполнит разрядность
			if(result > ((UINT64_MAX - static_cast <uint64_t> (letter - '0')) / 10))
				// Выводим отрицательный результат
				return false;
			// Накапливаем значение
			result = ((result * 10) + static_cast <uint64_t> (letter - '0'));
		}
		// Устанавливаем разобранное значение
		output = result;
		// Выводим положительный результат
		return true;
	}
	/**
	 * @brief Функция разбора значения псевдо-заголовка [:status]
	 *
	 * @param value  значение псевдо-заголовка
	 * @param output разобранный код ответа сервера
	 * @return       результат разбора
	 *
	 */
	bool parseStatus(string_view value, uint16_t & output) noexcept {
		// Код ответа сервера обязан состоять ровно из трёх цифр (RFC 9110 §15)
		if(value.size() != 3)
			// Выводим отрицательный результат
			return false;
		// Накапливаемый код ответа сервера
		uint16_t result = 0;
		/**
		 * Выполняем перебор всех цифр кода ответа сервера
		 */
		for(const char letter : value){
			// Если символ не является цифрой
			if((letter < '0') || (letter > '9'))
				// Выводим отрицательный результат
				return false;
			// Накапливаем код ответа сервера
			result = static_cast <uint16_t> ((result * 10) + (letter - '0'));
		}
		// Устанавливаем разобранный код ответа сервера
		output = result;
		// Выводим положительный результат
		return true;
	}
	/**
	 * @brief Класс временного изъятия списка идентификаторов потоков из накопителя
	 *
	 * @details Обходы карты потоков сначала собирают список идентификаторов, а затем
	 *          выходят в пользовательские функции обратного вызова. Те вправе вызвать
	 *          разбор повторно, и вложенный обход собрал бы свой список поверх нашего:
	 *          внешний цикл продолжился бы по чужим потокам, а свои недообработал.
	 *          Список поэтому изымается из накопителя на время обхода, а по выходу
	 *          возвращается туда - переиспользование ёмкости сохраняется, вложенный
	 *          обход получает пустой накопитель и заводит собственный список
	 *
	 */
	/**
	 * @brief Класс временного изъятия буфера нагрузки принятого кадра
	 *
	 * @details Нагрузка, собранная по кускам, обрабатывается вне буфера накопления:
	 *          обработчик вправе реентрантно продолжить разбор того же потока
	 *          и переиспользовать буфер. Ёмкость при этом теряться не должна,
	 *          поэтому буфер изымается из накопителя, а не создаётся заново.
	 *          Вложенный вызов застаёт накопитель пустым и работает
	 *          на собственной ёмкости
	 *
	 */
	class Scratch {
		private:
			// Накопитель, из которого изъят буфер
			string & _pool;
		public:
			// Изъятый буфер нагрузки кадра
			string buffer;
		public:
			/**
			 * @brief Конструктор
			 *
			 * @param pool накопитель ёмкости буфера нагрузки
			 *
			 */
			explicit Scratch(string & pool) noexcept : _pool(pool) {
				// Забираем накопитель вместе с его ёмкостью
				this->buffer.swap(pool);
				// Выполняем очистку изъятого буфера
				this->buffer.clear();
			}
			/**
			 * @brief Деструктор
			 *
			 */
			~Scratch() noexcept {
				// Выполняем очистку изъятого буфера
				this->buffer.clear();
				// Возвращаем в накопитель ту ёмкость, которая больше
				if(this->_pool.capacity() < this->buffer.capacity())
					// Возвращаем изъятый буфер в накопитель
					this->buffer.swap(this->_pool);
			}
	};
	class Borrowed {
		private:
			// Накопитель, из которого изъят список
			vector <uint64_t> & _pool;
		public:
			// Изъятый список идентификаторов потоков
			vector <uint64_t> list;
		public:
			/**
			 * @brief Конструктор
			 *
			 * @param pool накопитель списка идентификаторов потоков
			 *
			 */
			explicit Borrowed(vector <uint64_t> & pool) noexcept : _pool(pool) {
				// Забираем накопитель вместе с его ёмкостью
				this->list.swap(pool);
				// Выполняем очистку изъятого списка
				this->list.clear();
			}
			/**
			 * @brief Деструктор
			 *
			 */
			~Borrowed() noexcept {
				// Выполняем очистку изъятого списка
				this->list.clear();
				// Возвращаем в накопитель ту ёмкость, которая больше
				if(this->_pool.capacity() < this->list.capacity())
					// Возвращаем изъятый список в накопитель
					this->list.swap(this->_pool);
			}
	};
}


/**
 * @brief Конструктор
 *
 */
awh::http::Parser_HTTP3::Ring::Ring() noexcept : items(PUSH_HISTORY_CACHE, UINT64_MAX), cursor(0) {}
/**
 * @brief Метод проверки наличия идентификатора в кольце
 *
 * @param value искомый идентификатор
 * @return      результат проверки
 *
 */
bool awh::http::Parser_HTTP3::Ring::has(const uint64_t value) const noexcept {
	// Пустая ячейка идентификатором не является
	if(value == UINT64_MAX)
		// Выводим отрицательный результат
		return false;
	/**
	 * Перебор, а не поиск по множеству: кольцо короткое и лежит одним куском памяти,
	 * а обращаются к нему только события push - на порядки реже кадров сообщений
	 */
	for(const uint64_t item : this->items){
		// Если идентификатор найден в кольце
		if(item == value)
			// Выводим положительный результат
			return true;
	}
	// Выводим отрицательный результат
	return false;
}
/**
 * @brief Метод записи идентификатора в кольцо
 *
 * @param value записываемый идентификатор
 *
 */
void awh::http::Parser_HTTP3::Ring::put(const uint64_t value) noexcept {
	// Пустая ячейка идентификатором не является
	if(value == UINT64_MAX)
		// Выходим из метода
		return;
	/**
	 * Повторная запись того же идентификатора вытеснила бы из кольца чужой,
	 * события которого ещё могут прийти
	 */
	if(this->has(value))
		// Выходим из метода
		return;
	// Записываем идентификатор в текущую ячейку кольца
	this->items[this->cursor] = value;
	// Продвигаем позицию записи по кольцу
	this->cursor = ((this->cursor + 1) % this->items.size());
}
/**
 * @brief Метод удаления идентификатора из кольца
 *
 * @param value удаляемый идентификатор
 *
 */
void awh::http::Parser_HTTP3::Ring::drop(const uint64_t value) noexcept {
	// Пустая ячейка идентификатором не является
	if(value == UINT64_MAX)
		// Выходим из метода
		return;
	/**
	 * Выполняем перебор всех ячеек кольца
	 */
	for(uint64_t & item : this->items){
		// Если идентификатор найден в кольце
		if(item == value){
			// Освобождаем ячейку кольца
			item = UINT64_MAX;
			// Выходим из метода
			return;
		}
	}
}
/**
 * @brief Метод очистки кольца
 *
 */
void awh::http::Parser_HTTP3::Ring::clear() noexcept {
	// Освобождаем все ячейки кольца
	::std::fill(this->items.begin(), this->items.end(), UINT64_MAX);
	// Сбрасываем позицию записи в кольце
	this->cursor = 0;
}
/**
 * @brief Конструктор
 *
 */
awh::http::Parser_HTTP3::Limits::Limits() noexcept :
 parser_t::limits_t(), maxHeaderSection(64 * 1024), maxControlFrame(16 * 1024),
 maxBlockedTail(256 * 1024), maxStreams(128),
 ctrlLimitRate(CTRL_LIMIT_RATE), ctrlLimitBurst(CTRL_LIMIT_BURST),
 prioLimitRate(PRIORITY_LIMIT_RATE), prioLimitBurst(PRIORITY_LIMIT_BURST) {}
/**
 * @brief Конструктор
 *
 */
awh::http::Parser_HTTP3::Settings::Settings() noexcept :
 qpackMaxTableCapacity(h3::proto::QPACK_TABLE_CAPACITY),
 qpackBlockedStreams(h3::proto::QPACK_BLOCKED_STREAMS),
 maxFieldSectionSize(h3::proto::MAX_FIELD_SECTION_SIZE),
 enableConnectProtocol(false) {}
/**
 * @brief Конструктор
 *
 */
awh::http::Parser_HTTP3::Ratelim::Ratelim() noexcept : rate(0), burst(0), value(0), stamp(0) {}
/**
 * @brief Метод инициализации лимита
 *
 * @param burst стартовый запас токенов
 * @param rate  пополнение токенов в секунду
 *
 */
void awh::http::Parser_HTTP3::Ratelim::init(const uint64_t burst, const uint64_t rate) noexcept {
	// Устанавливаем пополнение токенов в секунду
	this->rate = rate;
	// Устанавливаем максимум токенов
	this->burst = burst;
	// Устанавливаем текущее число токенов
	this->value = burst;
	// Сбрасываем последний момент обновления
	this->stamp = 0;
}
/**
 * @brief Метод обновления момента времени
 *
 * @param stamp текущий момент времени в секундах
 *
 */
void awh::http::Parser_HTTP3::Ratelim::update(const uint64_t stamp) noexcept {
	// Если момент времени ещё не устанавливался
	if(this->stamp == 0){
		// Запоминаем момент времени
		this->stamp = stamp;
		// Выходим из метода
		return;
	}
	// Если время не сдвинулось вперёд
	if(stamp <= this->stamp)
		// Выходим из метода
		return;
	// Вычисляем прошедшее время в секундах
	const uint64_t elapsed = (stamp - this->stamp);
	// Запоминаем момент времени
	this->stamp = stamp;
	/**
	 * Если пополнение переполнит разрядность - наполняем корзину до предела
	 */
	if((this->rate > 0) && (elapsed > (UINT64_MAX / this->rate))){
		// Наполняем корзину до предела
		this->value = this->burst;
		// Выходим из метода
		return;
	}
	// Вычисляем пополнение корзины
	const uint64_t refill = (elapsed * this->rate);
	// Наполняем корзину, не превышая предела
	this->value = ((refill > (this->burst - ::std::min(this->value, this->burst))) ? this->burst : (this->value + refill));
}
/**
 * @brief Метод списания токенов
 *
 * @param value число списываемых токенов
 * @return      результат списания (false - токенов не хватает)
 *
 */
bool awh::http::Parser_HTTP3::Ratelim::drain(const uint64_t value) noexcept {
	// Если токенов в корзине не хватает
	if(this->value < value)
		// Выводим признак превышения лимита
		return false;
	// Списываем токены из корзины
	this->value -= value;
	// Выводим признак успешного списания
	return true;
}
/**
 * @brief Конструктор
 *
 */
awh::http::Parser_HTTP3::Framing::Framing() noexcept : active(false), type(0), length(0), remain(0) {}
/**
 * @brief Метод сброса состояния разбора
 *
 */
void awh::http::Parser_HTTP3::Framing::clear() noexcept {
	// Сбрасываем признак разбора нагрузки
	this->active = false;
	// Сбрасываем тип разбираемого кадра
	this->type = 0;
	// Сбрасываем длину нагрузки разбираемого кадра
	this->length = 0;
	// Сбрасываем остаток нагрузки разбираемого кадра
	this->remain = 0;
	// Выполняем очистку буфера накопления
	this->buffer.clear();
}
/**
 * @brief Конструктор
 *
 */
awh::http::Parser_HTTP3::Stream::Stream() noexcept :
 state(h3::stream_state_t::IDLE), generation(0),
 headers(false), trailers(false), body(false),
 completed(false), headless(false), headlessSend(false), trailerless(false), trailerlessSend(false), localFin(false), length(0), declared(UINT64_MAX),
 urgency(h3::proto::DEFAULT_URGENCY), incremental(false), prioritized(false),
 blockedActive(false), blockedType(0), blockedPushId(UINT64_MAX), blockedFin(false),
 sendOffset(0), sourceEof(false), endStreamPending(false), writableNotified(false), trailersPending(false), headersSent(false) {}
/**
 * @brief Метод получения объёма ещё не обёрнутого в кадры тела
 *
 * @return объём данных буфера отправки
 *
 */
size_t awh::http::Parser_HTTP3::Stream::pending() const noexcept {
	// Выводим объём данных буфера отправки за вычетом уже обёрнутого
	return (this->sendBuffer.size() - this->sendOffset);
}
/**
 * @brief Метод амортизированного уплотнения буфера отправки
 *
 * @details Обёрнутый в кадры префикс не вырезается на каждой отправке: вырезание
 *          сдвигает весь остаток и даёт квадратичную стоимость на длинном теле.
 *          Буфер уплотняется, когда обёрнутая часть занимает половину и больше
 *
 */
void awh::http::Parser_HTTP3::Stream::compactSendBuffer() noexcept {
	// Если буфер выдан целиком - очищаем его без сдвига остатка
	if(this->sendOffset >= this->sendBuffer.size()){
		// Выполняем очистку буфера отправки
		this->sendBuffer.clear();
		// Сбрасываем количество обёрнутых октетов
		this->sendOffset = 0;
	// Если обёрнутая часть занимает половину буфера и больше - уплотняем
	} else if(this->sendOffset >= (this->sendBuffer.size() / 2)) {
		// Вырезаем обёрнутый префикс буфера отправки
		this->sendBuffer.erase(0, this->sendOffset);
		// Сбрасываем количество обёрнутых октетов
		this->sendOffset = 0;
	}
}
/**
 * @brief Конструктор
 *
 */
awh::http::Parser_HTTP3::Outgoing::Outgoing() noexcept : consumed(0), fin(false) {}
/**
 * @brief Конструктор
 *
 */
awh::http::Parser_HTTP3::Unistream::Unistream() noexcept :
 known(false), type(0), pushId(UINT64_MAX), identified(false) {}
/**
 * @brief Конструктор
 *
 */
awh::http::Parser_HTTP3::Callbacks::Callbacks() noexcept {}

/**
 * @brief Метод фиксации ошибки уровня соединения
 *
 * @param code    код ошибки протокола
 * @param message текстовое описание ошибки
 * @return        результат обработки (всегда ERROR)
 *
 */
awh::http::h3::status_t awh::http::Parser_HTTP3::fail(const error_t code, const char * message) noexcept {
	// Если соединение уже завершено - повторно не сообщаем
	if(this->_closed)
		// Выводим результат обработки
		return h3::status_t::ERROR;
	// Запоминаем код последней ошибки протокола
	this->_error = code;
	// Запоминаем завершённость соединения
	this->_closed = true;
	// Устанавливаем итоговый статус разбора
	this->_status = status_t::ERROR;
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		// Если функция обратного вызова ошибки установлена
		if(this->_callbacks.error)
			/**
			 * Извещаем обвязку об ошибке: своего кадра для этого у HTTP/3 нет,
			 * поэтому обвязка обязана закрыть соединение QUIC с этим кодом
			 */
			this->_callbacks.error(code, (message != nullptr ? string_view(message) : string_view()));
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception &) {
		/**
		 * Исключение из пользовательской функции обратного вызова перехватывается
		 * на месте вызова: соединение и так уже признано нерабочим
		 */
	}
	// Выводим результат обработки
	return h3::status_t::ERROR;
}
/**
 * @brief Метод записи исходящих байтов потока
 *
 * @param sid    идентификатор потока
 * @param buffer буфер исходящих данных
 * @param size   размер исходящих данных
 * @param fin    признак завершения потока в исходящем направлении
 *
 */
void awh::http::Parser_HTTP3::emit(const uint64_t sid, const void * buffer, const size_t size, const bool fin) noexcept {
	// Если функция обратного вызова записи установлена
	if(this->_callbacks.write){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Отдаём исходящие байты транспорту
			this->_callbacks.write(sid, buffer, size, fin);
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception &) {
			// Исключение из пользовательской функции обратного вызова гасим на месте
		}
		// Выходим из метода
		return;
	}
	// Получаем буфер исходящих данных потока
	outgoing_t & output = this->_pending[sid];
	// Если исходящие данные переданы
	if((buffer != nullptr) && (size > 0))
		// Дописываем исходящие данные в буфер потока
		output.buffer.append(reinterpret_cast <const char *> (buffer), size);
	// Если поток завершается в исходящем направлении
	if(fin)
		// Запоминаем завершение потока
		output.fin = true;
}
/**
 * @brief Метод выгрузки накопленных инструкций кодека QPACK
 *
 */
void awh::http::Parser_HTTP3::flushQpack() noexcept {
	/**
	 * Инструкции снимаются с кодека ДО отправки, а не после, и отправляются
	 * из собственной копии.
	 *
	 * Отправка выходит в пользовательскую функцию обратного вызова, а та вправе
	 * подать разбор повторно - именно так поступает синхронная обвязка, замыкающая
	 * два парсера друг на друга. Разбор дойдёт до этого же места, увидит те же
	 * неснятые инструкции и отправит их второй раз, потом третий, и переписка
	 * не закончится никогда. Копия невелика: инструкции QPACK коротки и в типичном
	 * случае умещаются в саму строку без выделения памяти
	 */
	// Если наш поток инструкций кодера открыт
	if(this->_encoderLocal != UINT64_MAX){
		// Получаем накопленные инструкции потока кодера
		const string_view instructions = this->_encoder.pending();
		// Если инструкции потока кодера накоплены
		if(!instructions.empty()){
			// Забираем инструкции потока кодера в собственную копию
			const string outgoing(instructions);
			// Отмечаем инструкции потока кодера отправленными
			this->_encoder.consumePending(outgoing.size());
			// Записываем инструкции в поток кодера
			this->emit(this->_encoderLocal, outgoing.data(), outgoing.size(), false);
		}
	}
	// Если наш поток инструкций декодера открыт
	if(this->_decoderLocal != UINT64_MAX){
		// Получаем накопленные инструкции потока декодера
		const string_view instructions = this->_decoder.pending();
		// Если инструкции потока декодера накоплены
		if(!instructions.empty()){
			// Забираем инструкции потока декодера в собственную копию
			const string outgoing(instructions);
			// Отмечаем инструкции потока декодера отправленными
			this->_decoder.consumePending(outgoing.size());
			// Записываем инструкции в поток декодера
			this->emit(this->_decoderLocal, outgoing.data(), outgoing.size(), false);
		}
	}
}
/**
 * @brief Метод открытия служебных однонаправленных потоков
 *
 * @return признак готовности служебных потоков
 *
 */
bool awh::http::Parser_HTTP3::prepare() noexcept {
	// Если служебные потоки уже открыты
	if((this->_controlLocal != UINT64_MAX) && (this->_encoderLocal != UINT64_MAX) && (this->_decoderLocal != UINT64_MAX))
		// Выводим признак готовности служебных потоков
		return true;
	// Если функция обратного вызова открытия потока не установлена
	if(!this->_callbacks.open)
		// Выводим признак неготовности служебных потоков
		return false;
	/**
	 * @brief Функция открытия одного служебного потока
	 *
	 * @param target идентификатор открываемого потока
	 * @param type   тип однонаправленного потока
	 * @return       признак успешного открытия
	 *
	 */
	auto open = [this](uint64_t & target, const h3::unistream_t type) noexcept -> bool {
		// Если поток уже открыт
		if(target != UINT64_MAX)
			// Выводим признак готовности потока
			return true;
		// Идентификатор открытого потока
		int64_t sid = -1;
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Запрашиваем у обвязки открытие однонаправленного потока
			sid = this->_callbacks.open();
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception &) {
			// Исключение из пользовательской функции обратного вызова гасим на месте
			sid = -1;
		}
		// Если транспорт открыть поток не смог
		if(sid < 0)
			// Выводим признак неготовности потока
			return false;
		// Запоминаем идентификатор открытого потока
		target = static_cast <uint64_t> (sid);
		// Выполняем очистку буфера сборки кадра
		this->_frame.clear();
		// Записываем тип однонаправленного потока в его начало
		h3::frame::serialize::unistream(this->_frame, static_cast <uint64_t> (type));
		// Отправляем тип однонаправленного потока
		this->emit(target, this->_frame.data(), this->_frame.size(), false);
		// Выводим признак готовности потока
		return true;
	};
	/**
	 * Управляющий поток открывается первым: пир вправе ожидать SETTINGS раньше
	 * любых других инструкций
	 */
	if(!open(this->_controlLocal, h3::unistream_t::CONTROL))
		// Выводим признак неготовности служебных потоков
		return false;
	// Открываем поток инструкций кодера QPACK
	if(!open(this->_encoderLocal, h3::unistream_t::QPACK_ENCODER))
		// Выводим признак неготовности служебных потоков
		return false;
	// Открываем поток инструкций декодера QPACK
	if(!open(this->_decoderLocal, h3::unistream_t::QPACK_DECODER))
		// Выводим признак неготовности служебных потоков
		return false;
	// Выводим признак готовности служебных потоков
	return true;
}
/**
 * @brief Метод проверки двунаправленности потока
 *
 * @param sid идентификатор потока
 * @return    признак двунаправленного потока
 *
 */
bool awh::http::Parser_HTTP3::bidirectional(const uint64_t sid) noexcept {
	/**
	 * Второй младший бит идентификатора потока QUIC различает двунаправленные
	 * и однонаправленные потоки (RFC 9000 §2.1)
	 */
	return ((sid & 0x02) == 0);
}
/**
 * @brief Метод проверки принадлежности потока инициатору
 *
 * @param sid идентификатор потока
 * @return    признак того, что поток инициирован пиром
 *
 */
bool awh::http::Parser_HTTP3::peerInitiated(const uint64_t sid) const noexcept {
	/**
	 * Младший бит идентификатора потока QUIC различает инициатора: ноль - клиент,
	 * единица - сервер (RFC 9000 §2.1)
	 */
	const bool client = ((sid & 0x01) == 0);
	// Выводим признак инициирования потока пиром
	return (client == (this->_endpoint == h3::endpoint_t::SERVER));
}
/**
 * @brief Метод получения состояния потока запроса
 *
 * @param sid идентификатор потока
 * @return    состояние потока запроса
 *
 */
awh::http::Parser_HTTP3::stream_t & awh::http::Parser_HTTP3::stream(const uint64_t sid) noexcept {
	// Выполняем поиск состояния потока запроса, создавая его при необходимости
	stream_t & result = this->_streams[sid];
	// Если состояние потока только что создано - выдаём ему новое поколение
	if(result.generation == 0)
		// Выдаём состоянию потока очередное поколение
		result.generation = (++this->_generation);
	// Выводим состояние потока запроса
	return result;
}
/**
 * @brief Метод поиска состояния потока запроса
 *
 * @param sid идентификатор потока
 * @return    состояние потока запроса либо nullptr
 *
 */
awh::http::Parser_HTTP3::stream_t * awh::http::Parser_HTTP3::findStream(const uint64_t sid) noexcept {
	// Выполняем поиск состояния потока запроса
	auto i = this->_streams.find(sid);
	// Выводим найденное состояние потока запроса
	return ((i != this->_streams.end()) ? &i->second : nullptr);
}
/**
 * @brief Метод проверки того, что поток пережил выход в пользовательскую функцию
 *
 * @details Сверки адреса состояния мало: обработчик вправе закрыть поток и тут же
 *          открыть поток с тем же идентификатором, а узловая карта способна вернуть
 *          под него тот же адрес. Байты прежнего сообщения ушли бы тогда в состояние
 *          нового потока, поэтому сверяется выданное при создании поколение
 *
 * @param sid        идентификатор потока
 * @param generation поколение состояния потока до выхода наружу
 * @return           признак того, что поток жив и не пересоздан
 *
 */
bool awh::http::Parser_HTTP3::aliveStream(const uint64_t sid, const uint64_t generation) noexcept {
	// Выполняем поиск состояния потока запроса
	const stream_t * stream = this->findStream(sid);
	// Поток жив, только если он существует и остался тем же самым
	return ((stream != nullptr) && (stream->generation == generation));
}
/**
 * @brief Метод закрытия потока запроса
 *
 * @param sid  идентификатор потока
 * @param code код ошибки закрытия
 *
 */
void awh::http::Parser_HTTP3::closeStream(const uint64_t sid, const error_t code) noexcept {
	// Выполняем поиск состояния потока запроса
	auto i = this->_streams.find(sid);
	// Если поток уже закрыт
	if(i == this->_streams.end())
		// Выходим из метода
		return;
	/**
	 * Удаляем поток из карты до извещения: закрытие связанных потоков из обработчика
	 * иначе вызывало бы друг друга бесконечно
	 */
	this->_streams.erase(i);
	/**
	 * Ссылки отправленных секций на записи таблицы здесь НЕ снимаются, и это
	 * принципиально: подтверждение секции приходит от декодера пира тогда, когда
	 * он её разобрал, а это законно происходит уже после закрытия потока.
	 * Снятие ссылок на закрытии привело бы к подтверждению секции, о которой
	 * кодер уже забыл, - то есть к ошибке уровня соединения на ровном месте.
	 * Освобождают ссылки только сами инструкции потока декодера: подтверждение
	 * секции либо отмена потока (RFC 9204 §4.4.1, §4.4.2)
	 */
	if(code != error_t::H3_NO_ERROR){
		/**
		 * Поток оборван, а значит его секции разобраны не будут: извещаем об этом
		 * кодер пира, чтобы он освободил удерживаемые ими записи
		 */
		this->_decoder.cancel(sid);
		// Выгружаем накопленные инструкции кодека
		this->flushQpack();
	}
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		// Если функция обратного вызова закрытия потока установлена
		if(this->_callbacks.close)
			// Извещаем обвязку о закрытии потока
			this->_callbacks.close(sid, code);
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception &) {
		// Исключение из пользовательской функции обратного вызова гасим на месте
	}
}

/**
 * @brief Метод разбора данных потока
 *
 * @param sid    идентификатор потока QUIC
 * @param buffer буфер данных потока
 * @param size   размер данных потока
 * @param fin    признак завершения потока пиром
 * @return       результат разбора (OK/ERROR)
 *
 */
awh::http::h3::status_t awh::http::Parser_HTTP3::parse(const uint64_t sid, const void * buffer, const size_t size, const bool fin) noexcept {
	// Если соединение уже завершено
	if(this->_closed)
		// Выводим результат разбора
		return h3::status_t::ERROR;
	// Устанавливаем итоговый статус разбора
	this->_status = status_t::PARTIAL;
	// Получаем указатель на разбираемые данные
	const uint8_t * data = reinterpret_cast <const uint8_t *> (buffer);
	/**
	 * Двунаправленные потоки несут запросы и ответы, однонаправленные - служебные
	 * потоки соединения и потоки push (RFC 9114 §6)
	 */
	if(bidirectional(sid))
		// Выполняем разбор потока запроса
		return this->parseRequest(sid, data, size, fin);
	// Выполняем разбор однонаправленного потока
	return this->parseUnistream(sid, data, size, fin);
}
/**
 * @brief Метод разбора байтов потока запроса
 *
 * @param sid  идентификатор потока
 * @param data входной буфер
 * @param size доступно байт
 * @param fin  признак завершения потока пиром
 * @return     результат разбора
 *
 */
awh::http::h3::status_t awh::http::Parser_HTTP3::parseRequest(const uint64_t sid, const uint8_t * data, const size_t size, const bool fin) noexcept {
	// Выполняем поиск состояния потока
	stream_t * found = this->findStream(sid);
	// Если поток встречен впервые
	if(found == nullptr){
		/**
		 * Двунаправленные потоки в HTTP/3 открывает только клиент (RFC 9114 §6.1):
		 * серверный идентификатор у такого потока незаконен независимо от нашей роли.
		 * Проверка стоит внутри ветки нового потока: разбор потока push приходит сюда
		 * с серверным однонаправленным идентификатором, но его состояние уже создано
		 */
		if(bidirectional(sid) && ((sid & 0x01) != 0))
			// Фиксируем ошибку уровня соединения
			return this->fail(error_t::H3_STREAM_CREATION_ERROR, "двунаправленный поток открыт сервером");
		/**
		 * Поток, инициированный пиром сверх объявленного нами в GOAWAY предела,
		 * отвергается: пир вправе повторить его на новом соединении (RFC 9114 §5.2)
		 */
		if(this->peerInitiated(sid) && (sid >= this->_goawayLocal)){
			// Обрываем поток с кодом отклонения
			this->sendReset(sid, error_t::H3_REQUEST_REJECTED);
			// Выводим результат разбора
			return h3::status_t::OK;
		}
		// Если лимит одновременно живых потоков исчерпан
		if(this->_streams.size() >= this->_limits.maxStreams){
			// Обрываем поток с кодом отклонения
			this->sendReset(sid, error_t::H3_REQUEST_REJECTED);
			// Выводим результат разбора
			return h3::status_t::OK;
		}
		// Создаём состояние потока
		found = &this->stream(sid);
		// Переводим поток в открытое состояние
		found->state = h3::stream_state_t::OPEN;
		// Применяем приоритет, объявленный кадром до открытия потока (RFC 9218 §7.2)
		this->applyPendingPriority(sid, * found);
		// Если поток инициирован пиром и обработчик его отверг
		if(this->peerInitiated(sid) && !this->fireBegin(sid)){
			// Обрываем поток с кодом отклонения
			this->sendReset(sid, error_t::H3_REQUEST_REJECTED);
			// Выводим результат разбора
			return h3::status_t::OK;
		}
		// Обновляем состояние потока: обработчик мог его закрыть
		found = this->findStream(sid);
		// Если поток закрыт из обработчика
		if(found == nullptr)
			// Выводим результат разбора
			return h3::status_t::OK;
	}
	/**
	 * Поток, чья секция полей ждёт вставок QPACK, разбирать нельзя: тело до
	 * разобранной секции недопустимо, а вторая секция затёрла бы отложенную.
	 * Всё пришедшее откладывается до разблокировки потока
	 */
	if(found->blockedActive)
		// Откладываем принятые байты до разблокировки потока
		return this->deferTail(sid, data, size, fin);
	// Позиция разбора во входном буфере
	size_t offset = 0;
	/**
	 * Выполняем разбор всех кадров входного буфера
	 */
	while(offset < size){
		// Обновляем состояние потока: обработчик мог его закрыть
		stream_t * current = this->findStream(sid);
		// Если поток закрыт из обработчика
		if(current == nullptr)
			// Выводим результат разбора
			return h3::status_t::OK;
		// Получаем состояние разбора кадров потока
		framing_t & framing = current->framing;
		// Запоминаем поколение состояния соединения перед выходами в обработчики
		const uint64_t epoch = this->_epoch;
		// Запоминаем поколение состояния потока перед выходами в обработчики
		const uint64_t generation = current->generation;
		/**
		 * Накопление заголовка кадра: тип и длина кодируются целыми переменной
		 * длины, поэтому размер заголовка заранее неизвестен и определяется
		 * по мере накопления
		 */
		if(!framing.active){
			// Разбираемый заголовок кадра
			h3::frame::header_t head;
			// Количество разобранных октетов заголовка кадра
			size_t used = 0;
			/**
			 * Заголовок пробуем разобрать прямо из входного буфера: в типичном случае
			 * он лежит там целиком, и накопление по октету стоило бы дописывания
			 * в строку и полного переразбора на каждом принятом октете
			 */
			if(framing.buffer.empty()){
				// Выполняем разбор заголовка кадра из входного буфера
				used = h3::frame::parser::header((data + offset), (size - offset), head);
				// Если заголовок кадра разобран целиком
				if(used > 0)
					// Пропускаем разобранный заголовок кадра
					offset += used;
			}
			/**
			 * Заголовок кадра разорван границей принятой порции: накапливаем его
			 * по октету, пока целое переменной длины не соберётся полностью
			 */
			if(used == 0){
				// Дописываем очередной октет заголовка кадра
				framing.buffer.push_back(static_cast <char> (data[offset++]));
				// Выполняем разбор заголовка кадра
				used = h3::frame::parser::header(reinterpret_cast <const uint8_t *> (framing.buffer.data()), framing.buffer.size(), head);
				// Если заголовок кадра ещё не разобран
				if(used == 0){
					/**
					 * Заголовок кадра не может занимать больше шестнадцати октетов:
					 * два целых переменной длины по восемь октетов каждое
					 */
					if(framing.buffer.size() > 16)
						// Фиксируем ошибку уровня соединения
						return this->fail(error_t::H3_FRAME_ERROR, "заголовок кадра превысил допустимый размер");
					// Переходим к следующему октету
					continue;
				}
				// Выполняем очистку буфера накопления заголовка кадра
				framing.buffer.clear();
			}
			// Устанавливаем тип разбираемого кадра
			framing.type = head.type;
			// Устанавливаем длину нагрузки разбираемого кадра
			framing.length = head.length;
			// Устанавливаем остаток нагрузки разбираемого кадра
			framing.remain = head.length;
			// Переводим разбор в состояние приёма нагрузки
			framing.active = true;
			/**
			 * Выполняем проверку допустимости кадра в потоке сообщения
			 */
			switch(framing.type){
				// Данные тела сообщения
				case static_cast <uint64_t> (h3::frame_t::DATA): {
					/**
					 * Данные до секции полей и после секции трейлеров недопустимы:
					 * это нарушение порядка частей сообщения (RFC 9114 §4.1)
					 */
					if(!current->headers || current->trailers)
						// Фиксируем ошибку уровня соединения
						return this->fail(error_t::H3_FRAME_UNEXPECTED, "кадр DATA вне тела сообщения");
					/**
					 * Ответы 204 и 304, а равно ответ на запрос HEAD, содержимого
					 * не несут по определению (RFC 9110 §8.6, §9.3.2). Тело в них
					 * делает сообщение искажённым, а это причина отвергнуть поток,
					 * а не соединение: остальные потоки к нему отношения не имеют.
					 * Пустой кадр DATA содержимого не добавляет и сообщение
					 * искажённым не делает - это выбор нарезки, а не семантика,
					 * и HTTP/2 трактует его так же
					 */
					if(current->headless && (framing.length > 0)){
						// Обрываем поток с кодом ошибки сообщения
						this->sendReset(sid, error_t::H3_MESSAGE_ERROR);
						// Выводим результат разбора
						return h3::status_t::OK;
					}
				} break;
				// Секция полей заголовков либо трейлеров
				case static_cast <uint64_t> (h3::frame_t::HEADERS): {
					// Если секция трейлеров уже получена
					if(current->trailers)
						// Фиксируем ошибку уровня соединения
						return this->fail(error_t::H3_FRAME_UNEXPECTED, "кадр HEADERS после секции трейлеров");
					/**
					 * Ответы 204 и 304 завершаются концом секции полей и не несут
					 * ни содержимого, ни трейлеров (RFC 9110 §15.3.5, §15.4.5).
					 * Вторая секция по такому потоку - это и есть трейлеры, и она
					 * делает сообщение искажённым: причина отвергнуть поток,
					 * а не соединение
					 */
					if(current->headers && current->trailerless){
						// Обрываем поток с кодом ошибки сообщения
						this->sendReset(sid, error_t::H3_MESSAGE_ERROR);
						// Выводим результат разбора
						return h3::status_t::OK;
					}
					/**
					 * Длина кадра протоколом не ограничена, поэтому границу ставит
					 * лимит парсера: иначе отправитель одним кадром задавал бы
					 * потребление памяти получателем
					 */
					if(framing.length > this->_limits.maxHeaderSection){
						// Обрываем поток с кодом чрезмерной нагрузки
						this->sendReset(sid, error_t::H3_EXCESSIVE_LOAD);
						// Выводим результат разбора
						return h3::status_t::OK;
					}
				} break;
				// Обещание server push
				case static_cast <uint64_t> (h3::frame_t::PUSH_PROMISE): {
					// Обещание push вправе отправлять только сервер (RFC 9114 §7.2.5)
					if(this->_endpoint == h3::endpoint_t::SERVER)
						// Фиксируем ошибку уровня соединения
						return this->fail(error_t::H3_FRAME_UNEXPECTED, "кадр PUSH_PROMISE получен сервером");
					// Если размер обещания превысил лимит секции полей
					if(framing.length > this->_limits.maxHeaderSection){
						// Обрываем поток с кодом чрезмерной нагрузки
						this->sendReset(sid, error_t::H3_EXCESSIVE_LOAD);
						// Выводим результат разбора
						return h3::status_t::OK;
					}
				} break;
				// Кадры, допустимые только в управляющем потоке
				case static_cast <uint64_t> (h3::frame_t::SETTINGS):
				case static_cast <uint64_t> (h3::frame_t::GOAWAY):
				case static_cast <uint64_t> (h3::frame_t::MAX_PUSH_ID):
				case static_cast <uint64_t> (h3::frame_t::CANCEL_PUSH):
				case static_cast <uint64_t> (h3::frame_t::PRIORITY_UPDATE_REQUEST):
				case static_cast <uint64_t> (h3::frame_t::PRIORITY_UPDATE_PUSH):
					// Фиксируем ошибку уровня соединения
					return this->fail(error_t::H3_FRAME_UNEXPECTED, "управляющий кадр в потоке сообщения");
				// Кадр неизвестного типа
				default: {
					/**
					 * Типы, изъятые из употребления вместе с кадрами HTTP/2, обязаны
					 * обрывать соединение: иначе пир, ошибочно отправляющий кадры
					 * HTTP/2, остался бы незамеченным (RFC 9114 §11.2.1)
					 */
					if(h3::retired(framing.type))
						// Фиксируем ошибку уровня соединения
						return this->fail(error_t::H3_FRAME_UNEXPECTED, "кадр, изъятый из употребления в HTTP/3");
					/**
					 * Остальные неизвестные типы кадров игнорируются целиком: именно
					 * это делает совместимыми будущие расширения (RFC 9114 §9)
					 */
				}
			}
			// Если нагрузка кадра пуста
			if(framing.remain == 0){
				// Выполняем обработку кадра с пустой нагрузкой
				const h3::status_t status = this->dispatchMessage(sid, framing.type, nullptr, 0, (fin && (offset == size)));
				// Если обработка кадра не удалась
				if(status != h3::status_t::OK)
					// Выводим результат разбора
					return status;
				// Если состояние потока не пережило обработчик
				if((epoch != this->_epoch) || !this->aliveStream(sid, generation))
					// Выводим результат разбора
					return h3::status_t::OK;
				// Сбрасываем состояние разбора кадра
				framing.clear();
				// Если секция полей осталась ждать вставок QPACK
				if(current->blockedActive)
					// Откладываем неразобранный хвост потока до разблокировки
					return this->deferTail(sid, (data + offset), (size - offset), fin);
			}
			// Переходим к разбору следующего кадра
			continue;
		}
		// Вычисляем размер доступной части нагрузки кадра
		const size_t chunk = static_cast <size_t> (::std::min <uint64_t> (static_cast <uint64_t> (size - offset), framing.remain));
		// Признак того, что нагрузка кадра завершается и поток закрывается пиром
		const bool last = (fin && ((offset + chunk) == size) && (chunk == framing.remain));
		/**
		 * Данные тела отдаются потребителю по частям и в буфере не накапливаются:
		 * длина кадра DATA протоколом не ограничена
		 */
		if(framing.type == static_cast <uint64_t> (h3::frame_t::DATA)){
			// Наращиваем суммарный размер принятого тела потока
			current->length += chunk;
			// Если суммарный размер тела превысил лимит
			if((this->_limits.maxBodySize > 0) && (current->length > this->_limits.maxBodySize)){
				// Обрываем поток с кодом чрезмерной нагрузки
				this->sendReset(sid, error_t::H3_EXCESSIVE_LOAD);
				// Выводим результат разбора
				return h3::status_t::OK;
			}
			/**
			 * Тело сверх объявленной длины делает сообщение искажённым (RFC 9110 §8.6).
			 * Расхождение видно уже здесь, и ждать завершения потока незачем: лишние
			 * октеты иначе прошли бы через обработчик тела как часть сообщения
			 */
			if((current->declared != UINT64_MAX) && !current->headless && (current->length > current->declared)){
				// Обрываем поток с кодом ошибки сообщения
				this->sendReset(sid, error_t::H3_MESSAGE_ERROR);
				// Выводим результат разбора
				return h3::status_t::OK;
			}
			/**
			 * Состояние разбора кадра живёт внутри состояния потока, а обработчик вправе
			 * закрыть поток либо сбросить весь парсер прямо изнутри. Поэтому после каждого
			 * выхода наружу проверяются оба поколения: смена поколения соединения означает
			 * сброс парсера, смена поколения потока - закрытие именно этого потока. И то
			 * и другое делает framing недействительным, и разбор буфера обязан свернуться
			 */
			// Если фаза приёма тела ещё не начата
			if(!current->body){
				// Запоминаем начало фазы приёма тела
				current->body = true;
				// Извещаем обработчик о начале фазы приёма тела
				if(!this->firePhase(sid, phase_t::BEGIN, part_t::BODY)){
					// Если состояние потока пережило обработчик
					if((epoch == this->_epoch) && this->aliveStream(sid, generation))
						// Обрываем поток с кодом отмены запроса
						this->sendReset(sid, error_t::H3_REQUEST_CANCELLED);
					// Выводим результат разбора
					return h3::status_t::OK;
				}
				// Если состояние потока не пережило обработчик
				if((epoch != this->_epoch) || !this->aliveStream(sid, generation))
					// Выводим результат разбора
					return h3::status_t::OK;
			}
			// Если данные тела есть - отдаём их потребителю
			if(chunk > 0){
				// Отдаём данные тела потребителю
				const bool proceed = this->fireData(sid, (data + offset), chunk, last);
				// Если состояние потока не пережило обработчик
				if((epoch != this->_epoch) || !this->aliveStream(sid, generation))
					// Выводим результат разбора
					return h3::status_t::OK;
				// Если обработчик отказался принимать данные тела
				if(!proceed){
					// Обрываем поток с кодом отмены запроса
					this->sendReset(sid, error_t::H3_REQUEST_CANCELLED);
					// Выводим результат разбора
					return h3::status_t::OK;
				}
			}
		/**
		 * Нагрузка секции полей и обещания push накапливается целиком: разобрать
		 * её по частям нельзя, а размер уже ограничен лимитом
		 */
		} else if((framing.type == static_cast <uint64_t> (h3::frame_t::HEADERS)) ||
		          (framing.type == static_cast <uint64_t> (h3::frame_t::PUSH_PROMISE))){
			/**
			 * Накопление начинается с ёмкости, оставшейся от прежних кадров: буфер
			 * состояния потока живёт ровно один кадр, а состояние потока - ровно один
			 * запрос, и без переиспользования ёмкости каждая секция полей растила бы
			 * буфер с нуля
			 */
			if(framing.buffer.empty())
				// Забираем ёмкость накопителя нагрузки
				framing.buffer.swap(this->_payload);
			// Дописываем очередную часть нагрузки кадра
			framing.buffer.append(reinterpret_cast <const char *> (data + offset), chunk);
		}
		/**
		 * Нагрузка кадра неизвестного типа отбрасывается, не накапливаясь
		 */
		// Выполняем смещение разбора
		offset += chunk;
		// Уменьшаем остаток нагрузки кадра
		framing.remain -= chunk;
		// Если нагрузка кадра принята целиком
		if(framing.remain == 0){
			// Запоминаем тип обработанного кадра
			const uint64_t type = framing.type;
			/**
			 * Изымаем накопленную нагрузку кадра вместе с ёмкостью буфера: обработчик
			 * вправе реентрантно продолжить разбор этого же потока, поэтому обрабатывать
			 * нагрузку прямо в буфере накопления нельзя. Изъятие вместо перемещения
			 * возвращает ёмкость назад, иначе каждый собранный по кускам кадр
			 * начинал бы накопление с пустого буфера
			 */
			::Scratch payload(this->_payload);
			// Забираем накопленную нагрузку вместе с ёмкостью буфера
			payload.buffer.swap(framing.buffer);
			// Сбрасываем состояние разбора кадра
			framing.clear();
			// Выполняем обработку кадра
			const h3::status_t status = this->dispatchMessage(
				sid, type, reinterpret_cast <const uint8_t *> (payload.buffer.data()), payload.buffer.size(), last
			);
			// Если обработка кадра не удалась
			if(status != h3::status_t::OK)
				// Выводим результат разбора
				return status;
			/**
			 * Выполняем поиск состояния потока: обработчик мог его закрыть, а следом
			 * открыть поток с тем же идентификатором - тогда указатель жив, но ведёт
			 * уже в другое состояние, и откладывать в него хвост прежнего нельзя
			 */
			const stream_t * blocked = (this->aliveStream(sid, generation) ? this->findStream(sid) : nullptr);
			// Если секция полей осталась ждать вставок QPACK
			if((blocked != nullptr) && blocked->blockedActive)
				// Откладываем неразобранный хвост потока до разблокировки
				return this->deferTail(sid, (data + offset), (size - offset), fin);
		}
	}
	// Если пир завершил поток
	if(fin)
		// Выполняем применение завершения потока
		return this->applyFin(sid);
	// Выводим результат разбора
	return h3::status_t::OK;
}

/**
 * @brief Метод обработки принятого кадра потока сообщения
 *
 * @param sid     идентификатор потока
 * @param type    тип кадра
 * @param payload нагрузка кадра
 * @param size    размер нагрузки кадра
 * @param last    признак завершения потока вместе с этим кадром
 * @return        результат обработки
 *
 */
awh::http::h3::status_t awh::http::Parser_HTTP3::dispatchMessage(const uint64_t sid, const uint64_t type, const uint8_t * payload, const size_t size, const bool last) noexcept {
	/**
	 * Выполняем обработку принятого кадра
	 */
	switch(type){
		// Секция полей заголовков либо трейлеров
		case static_cast <uint64_t> (h3::frame_t::HEADERS):
			// Выполняем обработку принятой секции полей
			return this->commitSection(sid, string_view(reinterpret_cast <const char *> (payload), size), last);
		// Обещание server push
		case static_cast <uint64_t> (h3::frame_t::PUSH_PROMISE): {
			// Разобранная нагрузка обещания push
			h3::frame::push_promise_t promise;
			// Код ошибки протокола
			error_t error = error_t::H3_NO_ERROR;
			// Выполняем разбор нагрузки обещания push
			if(h3::frame::parser::pushPromise(payload, size, promise, error) != h3::status_t::OK)
				// Фиксируем ошибку уровня соединения
				return this->fail(error, "нагрузка кадра PUSH_PROMISE не разобрана");
			/**
			 * Идентификатор push обязан лежать в границах, которые мы сами и объявили
			 * кадром MAX_PUSH_ID (RFC 9114 §7.2.5)
			 */
			if((this->_localMaxPushId == UINT64_MAX) || (promise.pushId > this->_localMaxPushId))
				// Фиксируем ошибку уровня соединения
				return this->fail(error_t::H3_ID_ERROR, "идентификатор push вне объявленных границ");
			// Код ошибки декодирования секции полей
			error_t reason = error_t::H3_NO_ERROR;
			// Выполняем декодирование секции полей обещанного запроса
			const h3::status_t status = this->_decoder.decode(sid, promise.block, this->_fields, this->_limits.maxHeadersTotal, reason);
			// Если поток заблокирован ожиданием пополнения таблицы
			if(status == h3::status_t::BLOCKED){
				// Выполняем поиск состояния потока
				stream_t * stream = this->findStream(sid);
				// Если поток уже закрыт
				if(stream == nullptr)
					// Выводим результат обработки
					return h3::status_t::OK;
				// Запоминаем отложенную секцию обещания
				stream->blocked.assign(promise.block);
				// Запоминаем тип кадра отложенной секции
				stream->blockedType = static_cast <uint64_t> (h3::frame_t::PUSH_PROMISE);
				// Запоминаем идентификатор отложенного обещания
				stream->blockedPushId = promise.pushId;
				// Запоминаем завершение потока вместе с отложенной секцией
				stream->blockedFin = last;
				// Запоминаем наличие отложенной секции
				stream->blockedActive = true;
				// Выводим результат обработки
				return h3::status_t::OK;
			}
			// Если декодирование секции полей не удалось
			if(status != h3::status_t::OK)
				// Фиксируем ошибку уровня соединения
				return this->fail(reason, "секция полей обещания push не разобрана");
			// Выгружаем накопленные инструкции кодека
			this->flushQpack();
			// Если секция полей превысила лимит распакованного списка
			if(this->_decoder.overflowed()){
				// Отменяем обещанный push
				this->sendCancelPush(promise.pushId);
				// Выводим результат обработки
				return h3::status_t::OK;
			}
			// Выполняем доставку декодированной секции обещанного запроса
			return this->deliverPromise(sid, promise.pushId);
		}
	}
	/**
	 * Нагрузка кадра DATA уже выдана потребителю по частям, а кадры неизвестных
	 * типов отброшены: обрабатывать здесь нечего
	 */
	return h3::status_t::OK;
}
/**
 * @brief Метод закрытия потока по завершении обоих направлений
 *
 * @param sid идентификатор потока
 *
 */
void awh::http::Parser_HTTP3::maybeClose(const uint64_t sid) noexcept {
	// Выполняем поиск состояния потока
	stream_t * stream = this->findStream(sid);
	// Если поток уже закрыт
	if(stream == nullptr)
		// Выходим из метода
		return;
	/**
	 * Поток удаляется только после завершения обоих направлений: сервер отвечает
	 * на том же потоке, на котором принял запрос
	 */
	if(stream->completed && stream->localFin)
		// Закрываем поток штатно
		this->closeStream(sid, error_t::H3_NO_ERROR);
}
/**
 * @brief Метод применения завершения потока пиром
 *
 * @param sid идентификатор потока
 * @return    результат применения
 *
 */
awh::http::h3::status_t awh::http::Parser_HTTP3::applyFin(const uint64_t sid) noexcept {
	// Выполняем поиск состояния потока
	stream_t * stream = this->findStream(sid);
	// Если поток уже закрыт либо приём уже завершён
	if((stream == nullptr) || stream->completed)
		// Выводим результат применения
		return h3::status_t::OK;
	/**
	 * Поток, завершившийся посреди кадра, - ошибка уровня соединения: нагрузка
	 * оборвалась до объявленной длины (RFC 9114 §7.1)
	 */
	if(stream->framing.active || !stream->framing.buffer.empty())
		// Фиксируем ошибку уровня соединения
		return this->fail(error_t::H3_FRAME_ERROR, "поток завершён посреди кадра");
	/**
	 * Если поток ждёт пополнения таблицы QPACK - завершение применится после
	 * разбора отложенной секции
	 */
	if(stream->blockedActive){
		// Запоминаем завершение потока вместе с отложенной секцией
		stream->blockedFin = true;
		// Выводим результат применения
		return h3::status_t::OK;
	}
	/**
	 * Поток, завершившийся без секции полей, не несёт сообщения вовсе
	 * (RFC 9114 §4.1)
	 */
	if(!stream->headers){
		// Обрываем поток с кодом неполного сообщения
		this->sendReset(sid, error_t::H3_REQUEST_INCOMPLETE);
		// Выводим результат применения
		return h3::status_t::OK;
	}
	/**
	 * Сверяем принятую длину тела с объявленной: расхождение означает, что
	 * сообщение искажено (RFC 9110 §8.6)
	 */
	if((stream->declared != UINT64_MAX) && !stream->headless && (stream->length != stream->declared)){
		// Обрываем поток с кодом ошибки сообщения
		this->sendReset(sid, error_t::H3_MESSAGE_ERROR);
		// Выводим результат применения
		return h3::status_t::OK;
	}
	// Запоминаем завершённость приёма сообщения
	stream->completed = true;
	// Переводим поток в состояние закрытого приёма
	stream->state = (stream->localFin ? h3::stream_state_t::CLOSED : h3::stream_state_t::HALF_CLOSED_REMOTE);
	/**
	 * Обработчик вправе закрыть поток либо сбросить весь парсер прямо изнутри,
	 * поэтому после каждого выхода наружу состояние потока перепроверяется:
	 * смена поколения соединения означает сброс парсера, смена поколения потока -
	 * закрытие именно этого потока, и в обоих случаях stream уже недействителен
	 */
	// Запоминаем поколение состояния соединения перед выходами в обработчики
	const uint64_t epoch = this->_epoch;
	// Запоминаем поколение состояния потока перед выходами в обработчики
	const uint64_t generation = stream->generation;
	// Если фаза приёма тела была начата
	if(stream->body && !this->firePhase(sid, phase_t::END, part_t::BODY)){
		// Если состояние потока пережило обработчик
		if((epoch == this->_epoch) && this->aliveStream(sid, generation))
			// Обрываем поток с кодом отмены запроса
			this->sendReset(sid, error_t::H3_REQUEST_CANCELLED);
		// Выводим результат применения
		return h3::status_t::OK;
	}
	// Если состояние потока не пережило обработчик
	if((epoch != this->_epoch) || !this->aliveStream(sid, generation))
		// Выводим результат применения
		return h3::status_t::OK;
	// Извещаем обработчик о полном приёме сообщения потока
	if(!this->firePhase(sid, phase_t::END, part_t::NONE)){
		// Если состояние потока пережило обработчик
		if((epoch == this->_epoch) && this->aliveStream(sid, generation))
			// Обрываем поток с кодом отмены запроса
			this->sendReset(sid, error_t::H3_REQUEST_CANCELLED);
		// Выводим результат применения
		return h3::status_t::OK;
	}
	// Если состояние потока не пережило обработчик
	if((epoch != this->_epoch) || !this->aliveStream(sid, generation))
		// Выводим результат применения
		return h3::status_t::OK;
	// Закрываем поток, если оба направления завершены
	this->maybeClose(sid);
	// Выводим результат применения
	return h3::status_t::OK;
}
/**
 * @brief Метод разбора байтов однонаправленного потока
 *
 * @param sid  идентификатор потока
 * @param data входной буфер
 * @param size доступно байт
 * @param fin  признак завершения потока пиром
 * @return     результат разбора
 *
 */
awh::http::h3::status_t awh::http::Parser_HTTP3::parseUnistream(const uint64_t sid, const uint8_t * data, const size_t size, const bool fin) noexcept {
	// Получаем состояние однонаправленного потока
	unistream_t & stream = this->_unistreams[sid];
	// Запоминаем поколение состояния соединения перед выходами в обработчики
	const uint64_t epoch = this->_epoch;
	// Позиция разбора во входном буфере
	size_t offset = 0;
	/**
	 * Первым целым переменной длины в однонаправленном потоке идёт его тип
	 * (RFC 9114 §6.2)
	 */
	if(!stream.known){
		/**
		 * Выполняем накопление типа потока по одному октету
		 */
		while((offset < size) && !stream.known){
			// Дописываем очередной октет типа потока
			stream.buffer.push_back(static_cast <char> (data[offset++]));
			// Тип однонаправленного потока
			uint64_t type = 0;
			// Выполняем чтение типа однонаправленного потока
			const size_t used = quic::varint::read(reinterpret_cast <const uint8_t *> (stream.buffer.data()), stream.buffer.size(), type);
			// Если тип потока прочитан целиком
			if((used > 0) && (used == stream.buffer.size())){
				// Устанавливаем тип однонаправленного потока
				stream.type = type;
				// Запоминаем прочитанность типа потока
				stream.known = true;
				// Выполняем очистку буфера накопления
				stream.buffer.clear();
			// Если тип потока не помещается в целое переменной длины
			} else if(stream.buffer.size() >= 8)
				// Фиксируем ошибку уровня соединения
				return this->fail(error_t::H3_STREAM_CREATION_ERROR, "тип однонаправленного потока не разобран");
		}
		// Если тип потока ещё не прочитан
		if(!stream.known)
			// Выводим результат разбора
			return h3::status_t::OK;
		/**
		 * Выполняем регистрацию однонаправленного потока по его типу
		 */
		switch(stream.type){
			// Управляющий поток соединения
			case static_cast <uint64_t> (h3::unistream_t::CONTROL): {
				/**
				 * Управляющий поток в соединении единственный: второй означает
				 * ошибку пира (RFC 9114 §6.2.1)
				 */
				if(this->_controlRemote != UINT64_MAX)
					// Фиксируем ошибку уровня соединения
					return this->fail(error_t::H3_STREAM_CREATION_ERROR, "второй управляющий поток");
				// Запоминаем идентификатор управляющего потока пира
				this->_controlRemote = sid;
			} break;
			// Поток инструкций кодера QPACK
			case static_cast <uint64_t> (h3::unistream_t::QPACK_ENCODER): {
				// Если поток инструкций кодера уже открыт
				if(this->_encoderRemote != UINT64_MAX)
					// Фиксируем ошибку уровня соединения
					return this->fail(error_t::H3_STREAM_CREATION_ERROR, "второй поток инструкций кодера QPACK");
				// Запоминаем идентификатор потока инструкций кодера пира
				this->_encoderRemote = sid;
			} break;
			// Поток инструкций декодера QPACK
			case static_cast <uint64_t> (h3::unistream_t::QPACK_DECODER): {
				// Если поток инструкций декодера уже открыт
				if(this->_decoderRemote != UINT64_MAX)
					// Фиксируем ошибку уровня соединения
					return this->fail(error_t::H3_STREAM_CREATION_ERROR, "второй поток инструкций декодера QPACK");
				// Запоминаем идентификатор потока инструкций декодера пира
				this->_decoderRemote = sid;
			} break;
			// Поток server push
			case static_cast <uint64_t> (h3::unistream_t::PUSH): {
				/**
				 * Поток push открывает только сервер: его получение сервером означает,
				 * что клиент выдаёт себя за сервер (RFC 9114 §6.2.2)
				 */
				if(this->_endpoint == h3::endpoint_t::SERVER)
					// Фиксируем ошибку уровня соединения
					return this->fail(error_t::H3_STREAM_CREATION_ERROR, "поток push получен сервером");
			} break;
			// Однонаправленный поток неизвестного типа
			default: {
				/**
				 * Неизвестный тип потока ошибкой не является: его содержимое
				 * отбрасывается, а отправителю сообщается, что читать поток
				 * мы не будем (RFC 9114 §6.2)
				 */
				if(this->_callbacks.abort){
					/**
					 * Выполняем отлов ошибок
					 */
					try {
						// Просим отправителя прекратить передачу
						this->_callbacks.abort(sid, error_t::H3_STREAM_CREATION_ERROR, true);
					/**
					 * Если возникает ошибка
					 */
					} catch(const exception &) {
						// Исключение из пользовательской функции обратного вызова гасим на месте
					}
					/**
					 * Обработчик вправе сбросить парсер прямо изнутри и снести карту
					 * однонаправленных потоков целиком, поэтому после выхода наружу
					 * проверяется поколение состояния: его смена означает, что объект,
					 * на который ссылается stream, уже уничтожен
					 */
					if(epoch != this->_epoch)
						// Выводим результат разбора
						return h3::status_t::OK;
				}
			}
		}
	}
	/**
	 * Выполняем разбор содержимого однонаправленного потока
	 */
	switch(stream.type){
		// Управляющий поток соединения
		case static_cast <uint64_t> (h3::unistream_t::CONTROL): {
			// Выполняем разбор кадров управляющего потока
			const h3::status_t status = this->parseControl(sid, stream, (data + offset), (size - offset));
			// Если разбор не удался
			if(status != h3::status_t::OK)
				// Выводим результат разбора
				return status;
		} break;
		// Поток инструкций кодера QPACK
		case static_cast <uint64_t> (h3::unistream_t::QPACK_ENCODER): {
			// Дописываем принятые инструкции к неразобранному остатку
			stream.buffer.append(reinterpret_cast <const char *> (data + offset), (size - offset));
			// Количество разобранных октетов
			size_t consumed = 0;
			// Код ошибки протокола
			error_t error = error_t::H3_NO_ERROR;
			// Выполняем обработку инструкций потока кодера
			if(this->_decoder.decodeEncoderStream(stream.buffer, consumed, error) != h3::status_t::OK)
				// Фиксируем ошибку уровня соединения
				return this->fail(error, "инструкция потока кодера QPACK не разобрана");
			// Отбрасываем разобранную часть инструкций
			stream.buffer.erase(0, consumed);
			/**
			 * Неразобранный остаток - это незавершённая инструкция: её размер
			 * ограничен, иначе пир задавал бы потребление памяти получателем
			 */
			if(stream.buffer.size() > this->_limits.maxControlFrame)
				// Фиксируем ошибку уровня соединения
				return this->fail(error_t::QPACK_ENCODER_STREAM_ERROR, "инструкция потока кодера превысила лимит");
			// Выгружаем накопленные инструкции кодека
			this->flushQpack();
			// Выполняем повторный разбор заблокированных потоков
			const h3::status_t status = this->retryBlocked();
			// Если разбор не удался
			if(status != h3::status_t::OK)
				// Выводим результат разбора
				return status;
		} break;
		// Поток инструкций декодера QPACK
		case static_cast <uint64_t> (h3::unistream_t::QPACK_DECODER): {
			// Дописываем принятые инструкции к неразобранному остатку
			stream.buffer.append(reinterpret_cast <const char *> (data + offset), (size - offset));
			// Количество разобранных октетов
			size_t consumed = 0;
			// Код ошибки протокола
			error_t error = error_t::H3_NO_ERROR;
			// Выполняем обработку инструкций потока декодера
			if(this->_encoder.decodeDecoderStream(stream.buffer, consumed, error) != h3::status_t::OK)
				// Фиксируем ошибку уровня соединения
				return this->fail(error, "инструкция потока декодера QPACK не разобрана");
			// Отбрасываем разобранную часть инструкций
			stream.buffer.erase(0, consumed);
			// Если неразобранный остаток превысил лимит
			if(stream.buffer.size() > this->_limits.maxControlFrame)
				// Фиксируем ошибку уровня соединения
				return this->fail(error_t::QPACK_DECODER_STREAM_ERROR, "инструкция потока декодера превысила лимит");
		} break;
		// Поток server push
		case static_cast <uint64_t> (h3::unistream_t::PUSH): {
			/**
			 * Вслед за типом потока идёт идентификатор push, которому принадлежит
			 * ответ (RFC 9114 §4.6)
			 */
			if(!stream.identified){
				/**
				 * Выполняем накопление идентификатора push по одному октету
				 */
				while((offset < size) && !stream.identified){
					// Дописываем очередной октет идентификатора push
					stream.buffer.push_back(static_cast <char> (data[offset++]));
					// Идентификатор push
					uint64_t pushId = 0;
					// Выполняем чтение идентификатора push
					const size_t used = quic::varint::read(reinterpret_cast <const uint8_t *> (stream.buffer.data()), stream.buffer.size(), pushId);
					// Если идентификатор push прочитан целиком
					if((used > 0) && (used == stream.buffer.size())){
						// Устанавливаем идентификатор push
						stream.pushId = pushId;
						// Запоминаем прочитанность идентификатора push
						stream.identified = true;
						// Выполняем очистку буфера накопления
						stream.buffer.clear();
					// Если идентификатор push не помещается в целое переменной длины
					} else if(stream.buffer.size() >= 8)
						// Фиксируем ошибку уровня соединения
						return this->fail(error_t::H3_ID_ERROR, "идентификатор push не разобран");
				}
				// Если идентификатор push ещё не прочитан
				if(!stream.identified)
					// Выводим результат разбора
					return h3::status_t::OK;
				/**
				 * Идентификатор push обязан лежать в границах, объявленных нами
				 * кадром MAX_PUSH_ID
				 */
				if((this->_localMaxPushId == UINT64_MAX) || (stream.pushId > this->_localMaxPushId))
					// Фиксируем ошибку уровня соединения
					return this->fail(error_t::H3_ID_ERROR, "поток push вне объявленных границ");
				/**
				 * Идентификатор обещания используется ровно одним потоком: второй поток
				 * с тем же идентификатором - ошибка соединения (RFC 9114 §4.6)
				 */
				if(this->_openedPush.has(stream.pushId))
					// Фиксируем ошибку уровня соединения
					return this->fail(error_t::H3_ID_ERROR, "повторный поток обещания push");
				// Запоминаем пришедший идентификатор обещания push
				this->_openedPush.put(stream.pushId);
				/**
				 * Поток обещания сверх объявленного нами в GOAWAY предела отвергается
				 * так же, как поток запроса сверх него (RFC 9114 §5.2). Проверка стоит
				 * после учёта идентификатора: повторный поток остаётся ошибкой соединения
				 * независимо от того, завершаем мы соединение или нет
				 */
				if(stream.pushId >= this->_goawayLocal){
					// Удаляем состояние однонаправленного потока: читать его мы не будем
					this->_unistreams.erase(sid);
					// Если функция обратного вызова обрыва потока установлена
					if(this->_callbacks.abort){
						/**
						 * Выполняем отлов ошибок
						 */
						try {
							// Просим отправителя прекратить передачу
							this->_callbacks.abort(sid, error_t::H3_REQUEST_REJECTED, true);
						/**
						 * Если возникает ошибка
						 */
						} catch(const exception &) {
							// Исключение из пользовательской функции обратного вызова гасим на месте
						}
					}
					// Выводим результат разбора
					return h3::status_t::OK;
				}
				/**
				 * Отмена обещания обгоняет его поток: CANCEL_PUSH идёт управляющим
				 * потоком, а сам push - своим. Пришедший следом за отменой поток
				 * читать незачем - его содержимое уже никому не нужно (RFC 9114 §7.2.3)
				 */
				if(this->_cancelledPush.has(stream.pushId)){
					// Снимаем отмену: она отработала приходом потока
					this->_cancelledPush.drop(stream.pushId);
					// Удаляем состояние однонаправленного потока: читать его мы не будем
					this->_unistreams.erase(sid);
					// Если функция обратного вызова обрыва потока установлена
					if(this->_callbacks.abort){
						/**
						 * Выполняем отлов ошибок
						 */
						try {
							// Просим отправителя прекратить передачу
							this->_callbacks.abort(sid, error_t::H3_REQUEST_CANCELLED, true);
						/**
						 * Если возникает ошибка
						 */
						} catch(const exception &) {
							// Исключение из пользовательской функции обратного вызова гасим на месте
						}
					}
					// Выводим результат разбора
					return h3::status_t::OK;
				}
				/**
				 * Поток push несёт сообщение и живёт в той же карте, что и потоки запросов,
				 * поэтому считается тем же лимитом: иначе сервер обходил бы его потоками push
				 */
				if(this->_streams.size() >= this->_limits.maxStreams){
					// Удаляем состояние однонаправленного потока: читать его мы не будем
					this->_unistreams.erase(sid);
					// Если функция обратного вызова обрыва потока установлена
					if(this->_callbacks.abort){
						/**
						 * Выполняем отлов ошибок
						 */
						try {
							// Просим отправителя прекратить передачу
							this->_callbacks.abort(sid, error_t::H3_REQUEST_REJECTED, true);
						/**
						 * Если возникает ошибка
						 */
						} catch(const exception &) {
							// Исключение из пользовательской функции обратного вызова гасим на месте
						}
					}
					// Выводим результат разбора
					return h3::status_t::OK;
				}
				// Создаём состояние потока сообщения для потока push
				stream_t & message = this->stream(sid);
				// Переводим поток в открытое состояние
				message.state = h3::stream_state_t::OPEN;
				/**
				 * Поток push однонаправленный, поэтому нашего направления у него нет
				 * и оно считается завершённым сразу
				 */
				message.localFin = true;
			}
			// Выполняем разбор потока push как потока сообщения
			return this->parseRequest(sid, (data + offset), (size - offset), fin);
		}
	}
	/**
	 * Разбор содержимого выходит в пользовательские функции обратного вызова:
	 * управляющие кадры извещают о параметрах и завершении соединения, а инструкции
	 * кодера разблокируют отложенные секции полей. Любая из них вправе сбросить
	 * парсер, и тогда обращаться к stream уже нельзя
	 */
	if(epoch != this->_epoch)
		// Выводим результат разбора
		return h3::status_t::OK;
	// Если пир завершил однонаправленный поток
	if(fin){
		/**
		 * Управляющий поток и потоки QPACK обязаны жить всё соединение: их закрытие
		 * лишает соединение управления (RFC 9114 §6.2.1, RFC 9204 §4.2)
		 */
		if((stream.type == static_cast <uint64_t> (h3::unistream_t::CONTROL)) ||
		   (stream.type == static_cast <uint64_t> (h3::unistream_t::QPACK_ENCODER)) ||
		   (stream.type == static_cast <uint64_t> (h3::unistream_t::QPACK_DECODER)))
			// Фиксируем ошибку уровня соединения
			return this->fail(error_t::H3_CLOSED_CRITICAL_STREAM, "закрыт поток, обязанный жить всё соединение");
		// Удаляем состояние завершённого однонаправленного потока
		this->_unistreams.erase(sid);
	}
	// Выводим результат разбора
	return h3::status_t::OK;
}

/**
 * @brief Метод разбора кадров управляющего потока
 *
 * @param sid    идентификатор потока
 * @param stream состояние однонаправленного потока
 * @param data   входной буфер
 * @param size   доступно байт
 * @return       результат разбора
 *
 */
awh::http::h3::status_t awh::http::Parser_HTTP3::parseControl(const uint64_t sid, unistream_t & stream, const uint8_t * data, const size_t size) noexcept {
	/**
	 * Обработка управляющего кадра выходит в пользовательские функции обратного
	 * вызова (SETTINGS и GOAWAY), а те вправе сбросить парсер прямо изнутри
	 * и снести карту однонаправленных потоков целиком. Поэтому после каждой
	 * обработки проверяется поколение состояния: его смена означает, что объект,
	 * на который ссылается stream, уже уничтожен
	 */
	// Запоминаем поколение состояния соединения перед выходами в обработчики
	const uint64_t epoch = this->_epoch;
	// Идентификатор потока в разборе управляющих кадров не участвует
	(void) sid;
	// Получаем состояние разбора кадров потока
	framing_t & framing = stream.framing;
	// Позиция разбора во входном буфере
	size_t offset = 0;
	/**
	 * Выполняем разбор всех кадров входного буфера
	 */
	while(offset < size){
		/**
		 * Накопление заголовка кадра по одному октету: размер заголовка заранее
		 * неизвестен, так как тип и длина кодируются целыми переменной длины
		 */
		if(!framing.active){
			// Дописываем очередной октет заголовка кадра
			framing.buffer.push_back(static_cast <char> (data[offset++]));
			// Разбираемый заголовок кадра
			h3::frame::header_t head;
			// Выполняем разбор заголовка кадра
			const size_t used = h3::frame::parser::header(reinterpret_cast <const uint8_t *> (framing.buffer.data()), framing.buffer.size(), head);
			// Если заголовок кадра ещё не разобран
			if(used == 0){
				// Если заголовок кадра превысил допустимый размер
				if(framing.buffer.size() > 16)
					// Фиксируем ошибку уровня соединения
					return this->fail(error_t::H3_FRAME_ERROR, "заголовок управляющего кадра превысил допустимый размер");
				// Переходим к следующему октету
				continue;
			}
			// Выполняем очистку буфера накопления заголовка кадра
			framing.buffer.clear();
			// Устанавливаем тип разбираемого кадра
			framing.type = head.type;
			// Устанавливаем длину нагрузки разбираемого кадра
			framing.length = head.length;
			// Устанавливаем остаток нагрузки разбираемого кадра
			framing.remain = head.length;
			// Переводим разбор в состояние приёма нагрузки
			framing.active = true;
			/**
			 * Управляющий поток обязан начинаться кадром SETTINGS: без него
			 * неизвестны ни ёмкость таблицы QPACK, ни лимиты пира (RFC 9114 §6.2.1)
			 */
			if(!this->_settingsReceived && (framing.type != static_cast <uint64_t> (h3::frame_t::SETTINGS)))
				// Фиксируем ошибку уровня соединения
				return this->fail(error_t::H3_MISSING_SETTINGS, "управляющий поток начат не кадром SETTINGS");
			/**
			 * Выполняем проверку допустимости кадра в управляющем потоке
			 */
			switch(framing.type){
				// Кадры сообщения в управляющем потоке недопустимы (RFC 9114 §7.1)
				case static_cast <uint64_t> (h3::frame_t::DATA):
				case static_cast <uint64_t> (h3::frame_t::HEADERS):
				case static_cast <uint64_t> (h3::frame_t::PUSH_PROMISE):
					// Фиксируем ошибку уровня соединения
					return this->fail(error_t::H3_FRAME_UNEXPECTED, "кадр сообщения в управляющем потоке");
				// Параметры соединения
				case static_cast <uint64_t> (h3::frame_t::SETTINGS): {
					/**
					 * Кадр SETTINGS в соединении единственный: повторный означает
					 * попытку изменить уже согласованное (RFC 9114 §7.2.4)
					 */
					if(this->_settingsReceived)
						// Фиксируем ошибку уровня соединения
						return this->fail(error_t::H3_FRAME_UNEXPECTED, "повторный кадр SETTINGS");
				} break;
				// Остальные типы кадров
				default: {
					// Если тип кадра изъят из употребления вместе с кадрами HTTP/2
					if(h3::retired(framing.type))
						// Фиксируем ошибку уровня соединения
						return this->fail(error_t::H3_FRAME_UNEXPECTED, "кадр, изъятый из употребления в HTTP/3");
				}
			}
			/**
			 * Нагрузка управляющего кадра накапливается целиком, поэтому её размер
			 * обязан быть ограничен: длина кадра протоколом не ограничена
			 */
			if(framing.length > this->_limits.maxControlFrame)
				// Фиксируем ошибку уровня соединения
				return this->fail(error_t::H3_EXCESSIVE_LOAD, "нагрузка управляющего кадра превысила лимит");
			// Если нагрузка кадра пуста
			if(framing.remain == 0){
				// Запоминаем тип обработанного кадра
				const uint64_t type = framing.type;
				// Сбрасываем состояние разбора кадра
				framing.clear();
				// Выполняем обработку кадра с пустой нагрузкой
				const h3::status_t status = this->dispatchControl(type, nullptr, 0);
				// Если обработка кадра не удалась
				if(status != h3::status_t::OK)
					// Выводим результат разбора
					return status;
				// Если состояние потока не пережило обработчик
				if(epoch != this->_epoch)
					// Выводим результат разбора
					return h3::status_t::OK;
			}
			// Переходим к разбору следующего кадра
			continue;
		}
		// Вычисляем размер доступной части нагрузки кадра
		const size_t chunk = static_cast <size_t> (::std::min <uint64_t> (static_cast <uint64_t> (size - offset), framing.remain));
		// Если накопление нагрузки только начинается
		if(framing.buffer.empty())
			// Забираем ёмкость накопителя нагрузки
			framing.buffer.swap(this->_payload);
		// Дописываем очередную часть нагрузки кадра
		framing.buffer.append(reinterpret_cast <const char *> (data + offset), chunk);
		// Выполняем смещение разбора
		offset += chunk;
		// Уменьшаем остаток нагрузки кадра
		framing.remain -= chunk;
		// Если нагрузка кадра принята целиком
		if(framing.remain == 0){
			// Запоминаем тип обработанного кадра
			const uint64_t type = framing.type;
			/**
			 * Изымаем накопленную нагрузку кадра вместе с ёмкостью буфера: обработчик
			 * вправе реентрантно продолжить разбор этого же потока, поэтому обрабатывать
			 * нагрузку прямо в буфере накопления нельзя. Изъятие вместо перемещения
			 * возвращает ёмкость назад, иначе каждый собранный по кускам кадр
			 * начинал бы накопление с пустого буфера
			 */
			::Scratch payload(this->_payload);
			// Забираем накопленную нагрузку вместе с ёмкостью буфера
			payload.buffer.swap(framing.buffer);
			// Сбрасываем состояние разбора кадра
			framing.clear();
			// Выполняем обработку кадра
			const h3::status_t status = this->dispatchControl(type, reinterpret_cast <const uint8_t *> (payload.buffer.data()), payload.buffer.size());
			// Если обработка кадра не удалась
			if(status != h3::status_t::OK)
				// Выводим результат разбора
				return status;
			// Если состояние потока не пережило обработчик
			if(epoch != this->_epoch)
				// Выводим результат разбора
				return h3::status_t::OK;
		}
	}
	// Выводим результат разбора
	return h3::status_t::OK;
}
/**
 * @brief Метод обработки кадра управляющего потока
 *
 * @param type    тип кадра
 * @param payload нагрузка кадра
 * @param size    размер нагрузки кадра
 * @return        результат обработки
 *
 */
awh::http::h3::status_t awh::http::Parser_HTTP3::dispatchControl(const uint64_t type, const uint8_t * payload, const size_t size) noexcept {
	// Код ошибки протокола
	error_t error = error_t::H3_NO_ERROR;
	/**
	 * Выполняем обработку принятого управляющего кадра
	 */
	switch(type){
		// Параметры соединения
		case static_cast <uint64_t> (h3::frame_t::SETTINGS): {
			// Если лимит частоты управляющих кадров исчерпан
			if(!this->_ctrlLimit.drain(1))
				// Фиксируем ошибку уровня соединения
				return this->fail(error_t::H3_EXCESSIVE_LOAD, "превышен лимит частоты управляющих кадров");
			// Разобранный набор параметров
			vector <h3::frame::setting_entry_t> items;
			// Выполняем разбор нагрузки кадра
			if(h3::frame::parser::settings(payload, size, items, error) != h3::status_t::OK)
				// Фиксируем ошибку уровня соединения
				return this->fail(error, "нагрузка кадра SETTINGS не разобрана");
			// Выполняем применение полученного набора параметров
			return this->applySettings(items);
		}
		// Завершение соединения
		case static_cast <uint64_t> (h3::frame_t::GOAWAY): {
			// Если лимит частоты управляющих кадров исчерпан
			if(!this->_ctrlLimit.drain(1))
				// Фиксируем ошибку уровня соединения
				return this->fail(error_t::H3_EXCESSIVE_LOAD, "превышен лимит частоты управляющих кадров");
			// Идентификатор, объявленный пиром
			uint64_t identifier = 0;
			// Выполняем разбор нагрузки кадра
			if(h3::frame::parser::identifier(payload, size, identifier, error) != h3::status_t::OK)
				// Фиксируем ошибку уровня соединения
				return this->fail(error, "нагрузка кадра GOAWAY не разобрана");
			/**
			 * Сервер объявляет в GOAWAY идентификатор потока запроса, и потоки запросов
			 * бывают только двунаправленными и только клиентскими. Идентификатор любого
			 * другого класса означает расхождение в учёте потоков (RFC 9114 §5.2).
			 * Клиент объявляет идентификатор push, и класса у того нет вовсе
			 */
			if((this->_endpoint == h3::endpoint_t::CLIENT) && ((identifier & 0x03) != 0))
				// Фиксируем ошибку уровня соединения
				return this->fail(error_t::H3_ID_ERROR, "идентификатор в кадре GOAWAY не принадлежит потоку запроса");
			/**
			 * Объявленный идентификатор обязан не возрастать: возрастание означало бы
			 * отзыв уже данного обещания не обрабатывать потоки (RFC 9114 §5.2)
			 */
			if(identifier > this->_goawayRemote)
				// Фиксируем ошибку уровня соединения
				return this->fail(error_t::H3_ID_ERROR, "идентификатор в кадре GOAWAY возрос");
			// Запоминаем объявленный пиром идентификатор
			this->_goawayRemote = identifier;
			/**
			 * Выполняем отлов ошибок
			 */
			try {
				// Если функция обратного вызова завершения соединения установлена
				if(this->_callbacks.goaway)
					// Извещаем обвязку о завершении соединения пиром
					this->_callbacks.goaway(identifier);
			/**
			 * Если возникает ошибка
			 */
			} catch(const exception &) {
				// Исключение из пользовательской функции обратного вызова гасим на месте
			}
		} break;
		// Верхняя граница идентификаторов push
		case static_cast <uint64_t> (h3::frame_t::MAX_PUSH_ID): {
			// Если лимит частоты управляющих кадров исчерпан
			if(!this->_ctrlLimit.drain(1))
				// Фиксируем ошибку уровня соединения
				return this->fail(error_t::H3_EXCESSIVE_LOAD, "превышен лимит частоты управляющих кадров");
			/**
			 * Границу выдачи push назначает клиент: её получение клиентом означает,
			 * что сервер разрешает push сам себе (RFC 9114 §7.2.7)
			 */
			if(this->_endpoint == h3::endpoint_t::CLIENT)
				// Фиксируем ошибку уровня соединения
				return this->fail(error_t::H3_FRAME_UNEXPECTED, "кадр MAX_PUSH_ID получен клиентом");
			// Наибольший разрешённый идентификатор push
			uint64_t identifier = 0;
			// Выполняем разбор нагрузки кадра
			if(h3::frame::parser::identifier(payload, size, identifier, error) != h3::status_t::OK)
				// Фиксируем ошибку уровня соединения
				return this->fail(error, "нагрузка кадра MAX_PUSH_ID не разобрана");
			/**
			 * Граница обязана не убывать: её снижение отозвало бы уже данное
			 * разрешение (RFC 9114 §7.2.7)
			 */
			if((this->_maxPushId != UINT64_MAX) && (identifier < this->_maxPushId))
				// Фиксируем ошибку уровня соединения
				return this->fail(error_t::H3_ID_ERROR, "граница идентификаторов push снижена");
			// Запоминаем наибольший разрешённый идентификатор push
			this->_maxPushId = identifier;
		} break;
		// Отмена обещанного push
		case static_cast <uint64_t> (h3::frame_t::CANCEL_PUSH): {
			// Если лимит частоты управляющих кадров исчерпан
			if(!this->_ctrlLimit.drain(1))
				// Фиксируем ошибку уровня соединения
				return this->fail(error_t::H3_EXCESSIVE_LOAD, "превышен лимит частоты управляющих кадров");
			// Идентификатор отменяемого push
			uint64_t identifier = 0;
			// Выполняем разбор нагрузки кадра
			if(h3::frame::parser::identifier(payload, size, identifier, error) != h3::status_t::OK)
				// Фиксируем ошибку уровня соединения
				return this->fail(error, "нагрузка кадра CANCEL_PUSH не разобрана");
			/**
			 * Отмена push, который никогда не мог быть выдан, означает расхождение
			 * в учёте идентификаторов (RFC 9114 §7.2.3)
			 */
			if(this->_endpoint == h3::endpoint_t::SERVER){
				// Если идентификатор выходит за выданные нами границы
				if(identifier >= this->_nextPushId)
					// Фиксируем ошибку уровня соединения
					return this->fail(error_t::H3_ID_ERROR, "отменён невыданный push");
			// Если идентификатор выходит за объявленные нами границы
			} else if((this->_localMaxPushId == UINT64_MAX) || (identifier > this->_localMaxPushId))
				// Фиксируем ошибку уровня соединения
				return this->fail(error_t::H3_ID_ERROR, "отменён push вне объявленных границ");
			/**
			 * Отмена обещания, поток которого уже пришёл, эффекта не имеет вовсе
			 * (RFC 9114 §7.2.3): запоминать нечего, а запись, которую никто уже
			 * не снимет, только вытеснила бы из кольца полезную
			 */
			if(!this->_openedPush.has(identifier))
				// Запоминаем отменённый идентификатор push
				this->_cancelledPush.put(identifier);
		} break;
		// Обновление расширенного приоритета потока запроса либо потока push
		case static_cast <uint64_t> (h3::frame_t::PRIORITY_UPDATE_REQUEST):
		case static_cast <uint64_t> (h3::frame_t::PRIORITY_UPDATE_PUSH): {
			/**
			 * Лимит кадров приоритета отдельный и заметно щедрее: переустановка
			 * приоритетов на каждый загружаемый ресурс страницы - штатное поведение
			 */
			if(!this->_priorityLimit.drain(1))
				// Фиксируем ошибку уровня соединения
				return this->fail(error_t::H3_EXCESSIVE_LOAD, "превышен лимит частоты кадров приоритета");
			/**
			 * Кадр обоих видов отправляют только клиенты: сервер приоритетами
			 * не распоряжается вовсе, и принявший такой кадр клиент обязан
			 * считать это ошибкой соединения (RFC 9218 §7.2)
			 */
			if(this->_endpoint == h3::endpoint_t::CLIENT)
				// Фиксируем ошибку уровня соединения
				return this->fail(error_t::H3_FRAME_UNEXPECTED, "кадр PRIORITY_UPDATE принят клиентом");
			// Разобранная нагрузка кадра приоритета
			h3::frame::priority_update_t priority;
			// Выполняем разбор нагрузки кадра
			if(h3::frame::parser::priorityUpdate(type, payload, size, priority, error) != h3::status_t::OK)
				// Фиксируем ошибку уровня соединения
				return this->fail(error, "нагрузка кадра PRIORITY_UPDATE не разобрана");
			// Если приоритет назначается потоку запроса
			if(!priority.push){
				/**
				 * Приоритет назначается только двунаправленному потоку, инициированному
				 * клиентом: остальные потоки запросов не несут (RFC 9218 §7.2)
				 */
				if(!bidirectional(priority.id) || ((priority.id & 0x01) != 0))
					// Фиксируем ошибку уровня соединения
					return this->fail(error_t::H3_ID_ERROR, "приоритет назначен не потоку запроса");
				// Выполняем поиск состояния потока
				stream_t * stream = this->findStream(priority.id);
				// Если поток найден - применяем к нему приоритет
				if(stream != nullptr){
					// Применяем значение поля приоритета
					this->applyPriority(* stream, priority.value);
					// Помечаем что приоритет потока задан кадром
					stream->prioritized = true;
				/**
				 * Кадр вправе опередить секцию полей: он идёт по управляющему потоку,
				 * а порядок между потоками QUIC не гарантирует вовсе (RFC 9218 §7).
				 * Состояния потока под сигнал не создаём - до прихода секции это
				 * позволило бы пиру наполнить карту потоков даром; запись ложится
				 * в кольцо и применяется при открытии потока
				 */
				} else this->deferPriority(priority.id, priority.value);
			/**
			 * Приоритет обещания push адресуется идентификатором обещания, а не потока:
			 * поток push откроется позже и может не открыться вовсе (RFC 9218 §7.2)
			 */
			} else {
				/**
				 * Приоритизировать можно только уже обещанный push: идентификатор сверх
				 * выданных нами означает, что клиент распоряжается тем, чего мы ему
				 * не обещали, и это ошибка соединения (RFC 9218 §7.2). Предел,
				 * разрешённый клиентом, отдельной проверки не требует: сверх него
				 * мы обещаний и не выдаём
				 */
				if(priority.id >= this->_nextPushId)
					// Фиксируем ошибку уровня соединения
					return this->fail(error_t::H3_ID_ERROR, "приоритет назначен необещанному push");
				// Применяем приоритет обещания push
				this->applyPushPriority(priority.id, priority.value);
			}
		} break;
		// Остальные типы кадров
		default: {
			/**
			 * Неизвестные и зарезервированные типы кадров игнорируются целиком:
			 * именно это делает совместимыми будущие расширения (RFC 9114 §9)
			 */
		}
	}
	// Выводим результат обработки
	return h3::status_t::OK;
}
/**
 * @brief Метод применения полученного набора параметров SETTINGS
 *
 * @param items набор параметров
 * @return      результат применения
 *
 */
awh::http::h3::status_t awh::http::Parser_HTTP3::applySettings(const vector <h3::frame::setting_entry_t> & items) noexcept {
	/**
	 * Выполняем применение всех полученных параметров
	 */
	for(const auto & item : items){
		/**
		 * Параметры, изъятые из употребления вместе с параметрами HTTP/2, обязаны
		 * обрывать соединение: иначе пир, ошибочно отправляющий SETTINGS от HTTP/2,
		 * остался бы незамеченным (RFC 9114 §7.2.4.1)
		 */
		if(h3::retiredSetting(item.id))
			// Фиксируем ошибку уровня соединения
			return this->fail(error_t::H3_SETTINGS_ERROR, "параметр, изъятый из употребления в HTTP/3");
		/**
		 * Выполняем применение очередного параметра
		 */
		switch(item.id){
			// Размер динамической таблицы QPACK
			case static_cast <uint64_t> (h3::setting_t::QPACK_MAX_TABLE_CAPACITY):
				// Запоминаем размер динамической таблицы QPACK пира
				this->_remote.qpackMaxTableCapacity = item.value;
			break;
			// Число потоков, которым разрешено ожидать пополнения таблицы
			case static_cast <uint64_t> (h3::setting_t::QPACK_BLOCKED_STREAMS):
				// Запоминаем число потоков, которым пир разрешил ожидание
				this->_remote.qpackBlockedStreams = item.value;
			break;
			// Максимальный размер секции полей
			case static_cast <uint64_t> (h3::setting_t::MAX_FIELD_SECTION_SIZE):
				// Запоминаем максимальный размер секции полей пира
				this->_remote.maxFieldSectionSize = item.value;
			break;
			// Разрешение расширенного CONNECT
			case static_cast <uint64_t> (h3::setting_t::ENABLE_CONNECT_PROTOCOL): {
				// Параметр логический, поэтому иных значений не принимает (RFC 9220 §3)
				if(item.value > 1)
					// Фиксируем ошибку уровня соединения
					return this->fail(error_t::H3_SETTINGS_ERROR, "недопустимое значение ENABLE_CONNECT_PROTOCOL");
				/**
				 * Разрешение расширенного CONNECT выдаёт сервер: его получение
				 * сервером означает, что клиент разрешает туннели сам себе
				 */
				if(this->_endpoint == h3::endpoint_t::SERVER)
					// Фиксируем ошибку уровня соединения
					return this->fail(error_t::H3_SETTINGS_ERROR, "ENABLE_CONNECT_PROTOCOL получен сервером");
				// Запоминаем разрешение расширенного CONNECT
				this->_remote.enableConnectProtocol = (item.value == 1);
			} break;
			// Остальные параметры
			default: {
				/**
				 * Неизвестные и зарезервированные параметры игнорируются: именно
				 * это делает совместимыми будущие расширения (RFC 9114 §7.2.4.1)
				 */
			}
		}
	}
	// Запоминаем получение параметров пира
	this->_settingsReceived = true;
	/**
	 * Ёмкость таблицы кодера ограничена и анонсом пира, и нашим собственным
	 * выбором: держать таблицу больше, чем нужно нам, незачем
	 */
	this->_encoder.maxCapacity(::std::min <uint64_t> (this->_remote.qpackMaxTableCapacity, h3::proto::QPACK_TABLE_CAPACITY));
	// Устанавливаем число потоков, которым пир разрешил ожидание пополнения таблицы
	this->_encoder.maxBlocked(this->_remote.qpackBlockedStreams);
	// Выгружаем накопленные инструкции кодека
	this->flushQpack();
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		// Если функция обратного вызова применённых параметров установлена
		if(this->_callbacks.settings)
			// Извещаем обвязку о применённых параметрах пира
			this->_callbacks.settings();
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception &) {
		// Исключение из пользовательской функции обратного вызова гасим на месте
	}
	// Выводим результат применения
	return h3::status_t::OK;
}
/**
 * @brief Метод повторного разбора заблокированных потоков
 *
 * @return результат разбора
 *
 */
awh::http::h3::status_t awh::http::Parser_HTTP3::retryBlocked() noexcept {
	// Изымаем список разбираемых потоков из накопителя на время обхода
	::Borrowed outgoing(this->_outgoing);
	/**
	 * Собираем список заблокированных потоков заранее: разбор секции способен
	 * закрыть поток из обработчика, а это разрушило бы перебор карты
	 */
	for(const auto & item : this->_streams){
		// Если поток ждёт пополнения таблицы
		if(item.second.blockedActive)
			// Дописываем поток в список разбираемых
			outgoing.list.push_back(item.first);
	}
	// Запоминаем поколение состояния соединения перед выходами в обработчики
	const uint64_t epoch = this->_epoch;
	/**
	 * Выполняем повторный разбор всех заблокированных потоков
	 *
	 * Перебор идёт по индексу, а не по итератору: доставка секции выходит
	 * в пользовательские функции обратного вызова, а те вправе закрыть поток
	 * и тем разрушить итератор списка
	 */
	for(size_t i = 0; i < outgoing.list.size(); i++){
		// Получаем идентификатор очередного разбираемого потока
		const uint64_t sid = outgoing.list[i];
		// Выполняем поиск состояния потока
		stream_t * stream = this->findStream(sid);
		// Если поток закрыт либо уже разобран
		if((stream == nullptr) || !stream->blockedActive)
			// Переходим к следующему потоку
			continue;
		// Запоминаем поколение состояния потока перед выходами в обработчики
		const uint64_t generation = stream->generation;
		/**
		 * Отложенная секция и хвост забираются из состояния потока, а не копируются:
		 * хвост доходит до сотен килобайт, и копия его была бы самой дорогой операцией
		 * разблокировки. Держать их снаружи обязательно - разбор способен закрыть поток,
		 * и представление указывало бы в освобождённую память. Если секция окажется
		 * заблокированной снова, оба возвращаются на место
		 */
		string section = ::std::move(stream->blocked);
		// Забираем неразобранный хвост потока, накопленный за время блокировки
		string tail = ::std::move(stream->blockedTail);
		// Запоминаем тип кадра отложенной секции
		const uint64_t type = stream->blockedType;
		// Запоминаем идентификатор отложенного обещания
		const uint64_t pushId = stream->blockedPushId;
		/**
		 * Завершение потока применяется к хвосту, а не к секции: пока за секцией
		 * стоят неразобранные кадры, приём сообщения ими не закончен
		 */
		const bool last = (stream->blockedFin && tail.empty());
		// Код ошибки протокола
		error_t error = error_t::H3_NO_ERROR;
		// Выполняем декодирование отложенной секции полей
		const h3::status_t status = this->_decoder.decode(sid, section, this->_fields, this->_limits.maxHeadersTotal, error);
		// Если поток всё ещё заблокирован
		if(status == h3::status_t::BLOCKED){
			// Обновляем состояние потока
			stream = this->findStream(sid);
			// Если поток не закрыт
			if(stream != nullptr){
				// Возвращаем отложенную секцию: поток по-прежнему ждёт вставок
				stream->blocked = ::std::move(section);
				// Возвращаем неразобранный хвост потока
				stream->blockedTail = ::std::move(tail);
			}
			// Переходим к следующему потоку
			continue;
		}
		// Если декодирование секции полей не удалось
		if(status != h3::status_t::OK)
			// Фиксируем ошибку уровня соединения
			return this->fail(error, "отложенная секция полей не разобрана");
		// Обновляем состояние потока
		stream = this->findStream(sid);
		// Если поток закрыт
		if(stream == nullptr)
			// Переходим к следующему потоку
			continue;
		// Снимаем блокировку потока
		stream->blockedActive = false;
		// Выполняем очистку отложенной секции
		stream->blocked.clear();
		// Выполняем очистку неразобранного хвоста потока
		stream->blockedTail.clear();
		// Выгружаем накопленные инструкции кодека
		this->flushQpack();
		// Если секция полей превысила лимит распакованного списка
		if(this->_decoder.overflowed()){
			// Если превышение допустило обещание push
			if(type == static_cast <uint64_t> (h3::frame_t::PUSH_PROMISE))
				// Отменяем обещанный push
				this->sendCancelPush(pushId);
			// Если превышение допустила секция полей потока
			else this->sendReset(sid, error_t::H3_EXCESSIVE_LOAD);
			// Переходим к следующему потоку
			continue;
		}
		// Результат доставки декодированной секции
		h3::status_t result = h3::status_t::OK;
		// Если разобрано обещание push
		if(type == static_cast <uint64_t> (h3::frame_t::PUSH_PROMISE))
			// Выполняем доставку декодированной секции обещанного запроса
			result = this->deliverPromise(sid, pushId);
		// Если разобрана секция полей потока
		else result = this->deliverSection(sid, last);
		// Если доставка не удалась
		if(result != h3::status_t::OK)
			// Выводим результат разбора
			return result;
		// Если поток завершился вместе с отложенной секцией
		if(last){
			// Выполняем применение завершения потока
			result = this->applyFin(sid);
			// Если применение не удалось
			if(result != h3::status_t::OK)
				// Выводим результат разбора
				return result;
		}
		// Если состояние соединения не пережило обработчики доставки
		if(epoch != this->_epoch)
			// Выводим результат разбора
			return h3::status_t::OK;
		/**
		 * Кадры, накопленные за время блокировки, разбираются сразу за секцией,
		 * которая их держала: порядок частей сообщения обязан быть тем же, в каком
		 * их отправил пир
		 */
		if(!tail.empty()){
			// Если поток не пережил обработчики доставки
			if(!this->aliveStream(sid, generation))
				// Переходим к следующему потоку
				continue;
			// Обновляем состояние потока
			stream = this->findStream(sid);
			// Запоминаем завершение потока пиром
			const bool finished = stream->blockedFin;
			// Снимаем признак завершения потока: его применит разбор хвоста
			stream->blockedFin = false;
			// Выполняем разбор накопленного хвоста потока
			result = this->parseRequest(sid, reinterpret_cast <const uint8_t *> (tail.data()), tail.size(), finished);
			// Если разбор хвоста не удался
			if(result != h3::status_t::OK)
				// Выводим результат разбора
				return result;
			// Если состояние соединения не пережило разбор хвоста
			if(epoch != this->_epoch)
				// Выводим результат разбора
				return h3::status_t::OK;
		}
	}
	// Выводим результат разбора
	return h3::status_t::OK;
}
/**
 * @brief Метод применения значения заголовка приоритета (RFC 9218 §5)
 *
 * @param stream состояние потока
 * @param value  значение заголовка приоритета
 *
 */
void awh::http::Parser_HTTP3::applyPriority(stream_t & stream, string_view value) noexcept {
	// Разбираем сигнал приоритета прямо в признаки потока
	this->parsePriority(value, stream.urgency, stream.incremental);
}
/**
 * @brief Метод разбора значения поля расширенного приоритета (RFC 9218 §4)
 *
 * @param value       значение поля приоритета
 * @param urgency     срочность (выходной параметр)
 * @param incremental признак инкрементальной доставки (выходной параметр)
 *
 */
void awh::http::Parser_HTTP3::parsePriority(const string_view value, uint8_t & urgency, bool & incremental) const noexcept {
	/**
	 * Сигнал приоритета задаёт его целиком: параметр, в сигнале отсутствующий,
	 * принимает значение по умолчанию, а не сохраняет прежнее (RFC 9218 §4).
	 * Без сброса [u=1] после прежнего [i] оставлял бы поток инкрементальным
	 */
	urgency = h3::proto::DEFAULT_URGENCY;
	// Снимаем признак инкрементальной доставки потока
	incremental = false;
	// Позиция разбора значения
	size_t offset = 0;
	/**
	 * Значение записано синтаксисом структурированных полей и представляет собой
	 * словарь: разбираем его поэлементно, разделителем служит запятая
	 */
	while(offset < value.size()){
		// Выполняем поиск конца текущего элемента словаря
		size_t end = value.find(',', offset);
		// Если разделитель не найден - элемент последний
		if(end == string_view::npos)
			// Устанавливаем конец элемента на конец значения
			end = value.size();
		// Выделяем текущий элемент словаря
		string_view item = value.substr(offset, (end - offset));
		// Переходим к следующему элементу словаря
		offset = (end + 1);
		/**
		 * Снимаем обрамляющие пробелы: синтаксис структурированных полей допускает
		 * их вокруг разделителя
		 */
		while(!item.empty() && ((item.front() == ' ') || (item.front() == '\t')))
			// Снимаем ведущий пробел
			item.remove_prefix(1);
		/**
		 * Снимаем завершающие пробелы элемента
		 */
		while(!item.empty() && ((item.back() == ' ') || (item.back() == '\t')))
			// Снимаем завершающий пробел
			item.remove_suffix(1);
		/**
		 * Отсекаем параметры члена словаря (RFC 8941 §3.1.2): они уточняют значение,
		 * но самого значения не отменяют, а неизвестные обязаны игнорироваться.
		 * Без этого [u=1;q=0.5] отбрасывался целиком, и срочность оставалась
		 * значением по умолчанию. Обрезка по первой точке с запятой безопасна:
		 * значения ключей [u] и [i] - целое и логическое, точки с запятой в них нет
		 */
		const size_t params = item.find(';');
		// Если параметры члена словаря заданы
		if(params != string_view::npos)
			// Отсекаем параметры члена словаря
			item = item.substr(0, params);
		/**
		 * Снимаем пробелы, оставшиеся перед разделителем параметров
		 */
		while(!item.empty() && ((item.back() == ' ') || (item.back() == '\t')))
			// Снимаем завершающий пробел
			item.remove_suffix(1);
		// Если элемент задаёт срочность потока
		if((item.size() == 3) && (item[0] == 'u') && (item[1] == '=')){
			// Если значение срочности является цифрой допустимого диапазона
			if((item[2] >= '0') && (item[2] <= ('0' + static_cast <char> (h3::proto::MAX_URGENCY))))
				// Устанавливаем срочность потока
				urgency = static_cast <uint8_t> (item[2] - '0');
		/**
		 * Логический элемент словаря записывается либо одним именем, либо именем
		 * с явным значением (RFC 8941 §3.2)
		 */
		} else if((item == "i") || (item == value::INCREMENTAL_ON))
			// Помечаем поток инкрементальным
			incremental = true;
		// Если инкрементальность потока явно снята
		else if(item == value::INCREMENTAL_OFF)
			// Снимаем признак инкрементального потока
			incremental = false;
	}
}
/**
 * @brief Метод запоминания приоритета ещё не открытого потока (RFC 9218 §7.2)
 *
 * @param sid   идентификатор приоритизируемого потока
 * @param value значение поля приоритета
 *
 */
void awh::http::Parser_HTTP3::deferPriority(const uint64_t sid, const string_view value) noexcept {
	// Формируем запись отложенного приоритета
	signal_t signal;
	// Запоминаем идентификатор приоритизируемого потока
	signal.id = sid;
	// Разбираем сигнал приоритета в поля записи
	this->parsePriority(value, signal.urgency, signal.incremental);
	/**
	 * Выполняем поиск прежней записи по этому же потоку
	 */
	for(signal_t & item : this->_pendingPriorities){
		// Если запись по этому потоку уже есть
		if(item.id == sid){
			// Новый сигнал заменяет прежний целиком (RFC 9218 §4)
			item = signal;
			// Выходим из метода
			return;
		}
	}
	// Если кольцо заполнено - вытесняем самую старую запись
	if(this->_pendingPriorities.size() >= PENDING_PRIORITIES_CACHE)
		// Снимаем самую старую запись кольца
		this->_pendingPriorities.erase(this->_pendingPriorities.begin());
	// Запоминаем приоритет до открытия потока
	this->_pendingPriorities.push_back(signal);
}
/**
 * @brief Метод применения приоритета, отложенного до открытия потока
 *
 * @param sid    идентификатор открываемого потока
 * @param stream состояние открываемого потока
 *
 */
void awh::http::Parser_HTTP3::applyPendingPriority(const uint64_t sid, stream_t & stream) noexcept {
	// Если отложенных приоритетов нет - применять нечего
	if(this->_pendingPriorities.empty())
		// Выходим из метода
		return;
	// Число записей, сохраняемых в кольце
	size_t count = 0;
	/**
	 * Уплотняем кольцо на месте: записи потоков с идентификатором не выше
	 * открываемого сохранению не подлежат. Идентификаторы потоков запросов
	 * строго возрастают, поэтому такие потоки открыты уже не будут, и без
	 * снятия их сигналы вытесняли бы из кольца актуальные
	 */
	for(const signal_t & item : this->_pendingPriorities){
		// Если запись относится к открываемому потоку
		if(item.id == sid){
			// Применяем срочность потока
			stream.urgency = item.urgency;
			// Применяем признак инкрементальной доставки потока
			stream.incremental = item.incremental;
			// Помечаем что приоритет потока задан кадром
			stream.prioritized = true;
		// Если запись относится к потоку, который ещё может быть открыт
		} else if(item.id > sid)
			// Сохраняем запись в кольце
			this->_pendingPriorities[count++] = item;
	}
	// Усекаем кольцо до числа сохранённых записей
	this->_pendingPriorities.resize(count);
}
/**
 * @brief Метод применения приоритета обещания push (RFC 9218 §7.2)
 *
 * @param pushId идентификатор обещания push
 * @param value  значение поля приоритета
 *
 */
void awh::http::Parser_HTTP3::applyPushPriority(const uint64_t pushId, const string_view value) noexcept {
	// Формируем запись приоритета обещания push
	signal_t signal;
	// Запоминаем идентификатор обещания push
	signal.id = pushId;
	// Разбираем сигнал приоритета в поля записи
	this->parsePriority(value, signal.urgency, signal.incremental);
	/**
	 * Выполняем поиск прежней записи по этому же обещанию
	 */
	for(signal_t & item : this->_pushPriorities){
		// Если запись по этому обещанию уже есть
		if(item.id == pushId){
			// Новый сигнал заменяет прежний целиком (RFC 9218 §4)
			item = signal;
			// Выходим из метода
			return;
		}
	}
	// Если кольцо заполнено - вытесняем самую старую запись
	if(this->_pushPriorities.size() >= PUSH_HISTORY_CACHE)
		// Снимаем самую старую запись кольца
		this->_pushPriorities.erase(this->_pushPriorities.begin());
	// Запоминаем приоритет обещания push
	this->_pushPriorities.push_back(signal);
}

/**
 * @brief Метод накопления неразобранного хвоста заблокированного потока
 *
 * @param sid  идентификатор потока
 * @param data неразобранный хвост
 * @param size размер неразобранного хвоста
 * @param fin  признак завершения потока пиром
 * @return     результат накопления
 *
 */
awh::http::h3::status_t awh::http::Parser_HTTP3::deferTail(const uint64_t sid, const uint8_t * data, const size_t size, const bool fin) noexcept {
	// Выполняем поиск состояния потока
	stream_t * stream = this->findStream(sid);
	// Если поток уже закрыт
	if(stream == nullptr)
		// Выводим результат накопления
		return h3::status_t::OK;
	/**
	 * Завершение потока применяется не к секции, а к хвосту: кадры, пришедшие
	 * следом за ней, ещё не разобраны, и приём сообщения ими не закончен
	 */
	if(fin)
		// Запоминаем завершение потока пиром
		stream->blockedFin = true;
	// Если накапливать нечего
	if(size == 0)
		// Выводим результат накопления
		return h3::status_t::OK;
	/**
	 * Хвост копится, пока пир не пришлёт инструкции потока кодера. Пир, который
	 * их не присылает вовсе, иначе задавал бы потребление памяти получателем,
	 * поэтому размер хвоста ограничен - и это причина отвергнуть один поток,
	 * а не соединение
	 */
	if((stream->blockedTail.size() + size) > this->_limits.maxBlockedTail){
		// Снимаем блокировку потока
		stream->blockedActive = false;
		// Выполняем очистку отложенной секции
		stream->blocked.clear();
		// Выполняем очистку неразобранного хвоста
		stream->blockedTail.clear();
		// Обрываем поток с кодом чрезмерной нагрузки
		this->sendReset(sid, error_t::H3_EXCESSIVE_LOAD);
		// Выводим результат накопления
		return h3::status_t::OK;
	}
	// Дописываем неразобранный хвост потока
	stream->blockedTail.append(reinterpret_cast <const char *> (data), size);
	// Выводим результат накопления
	return h3::status_t::OK;
}
/**
 * @brief Метод обработки принятой секции полей
 *
 * @param sid     идентификатор потока
 * @param section секция полей
 * @param fin     признак завершения потока
 * @return        результат обработки
 *
 */
awh::http::h3::status_t awh::http::Parser_HTTP3::commitSection(const uint64_t sid, string_view section, const bool fin) noexcept {
	// Код ошибки протокола
	error_t error = error_t::H3_NO_ERROR;
	// Выполняем декодирование секции полей
	const h3::status_t status = this->_decoder.decode(sid, section, this->_fields, this->_limits.maxHeadersTotal, error);
	/**
	 * Секция, пришедшая раньше нужных вставок QPACK, откладывается: инструкции
	 * кодека идут отдельным потоком и обгоняют секцию (RFC 9204 §2.1.2)
	 */
	if(status == h3::status_t::BLOCKED){
		// Выполняем поиск состояния потока
		stream_t * stream = this->findStream(sid);
		// Если поток уже закрыт
		if(stream == nullptr)
			// Выводим результат обработки
			return h3::status_t::OK;
		// Запоминаем отложенную секцию полей
		stream->blocked.assign(section);
		// Запоминаем тип кадра отложенной секции
		stream->blockedType = static_cast <uint64_t> (h3::frame_t::HEADERS);
		// Запоминаем завершение потока вместе с отложенной секцией
		stream->blockedFin = fin;
		// Запоминаем наличие отложенной секции
		stream->blockedActive = true;
		// Выводим результат обработки
		return h3::status_t::OK;
	}
	// Если декодирование секции полей не удалось
	if(status != h3::status_t::OK)
		// Фиксируем ошибку уровня соединения
		return this->fail(error, "секция полей не разобрана");
	// Выгружаем накопленные инструкции кодека
	this->flushQpack();
	/**
	 * Секция сверх лимита распакованного списка разобрана целиком, но наружу
	 * не отдана: динамическая таблица осталась синхронной с кодером пира,
	 * поэтому отвергается один поток, а соединение живёт
	 */
	if(this->_decoder.overflowed()){
		// Обрываем поток с кодом чрезмерной нагрузки
		this->sendReset(sid, error_t::H3_EXCESSIVE_LOAD);
		// Выводим результат обработки
		return h3::status_t::OK;
	}
	// Выполняем доставку декодированной секции полей
	return this->deliverSection(sid, fin);
}
/**
 * @brief Метод доставки декодированной секции полей
 *
 * @param sid идентификатор потока
 * @param fin признак завершения потока
 * @return    результат доставки
 *
 */
awh::http::h3::status_t awh::http::Parser_HTTP3::deliverSection(const uint64_t sid, const bool fin) noexcept {
	// Выполняем поиск состояния потока
	stream_t * stream = this->findStream(sid);
	// Если поток уже закрыт
	if(stream == nullptr)
		// Выводим результат доставки
		return h3::status_t::OK;
	/**
	 * Поля секции ссылаются в арену декодера, а состояние потока живёт в карте
	 * потоков: обработчик вправе снести и то и другое, вызвав clear() либо reset()
	 * прямо изнутри. Поэтому после каждого выхода наружу проверяются оба поколения,
	 * а состояние потока перечитывается заново. Поколение потока, а не его наличие:
	 * закрытие потока с последующим открытием потока с тем же идентификатором
	 * вернуло бы живой указатель на уже другое состояние
	 */
	// Запоминаем поколение состояния соединения перед выходами в обработчики
	const uint64_t epoch = this->_epoch;
	// Запоминаем поколение состояния потока перед выходами в обработчики
	const uint64_t generation = stream->generation;
	// Секция считается трейлерами, если финальная секция полей уже принята
	const bool trailer = stream->headers;
	// Код ошибки семантики сообщения
	error_t error = error_t::H3_MESSAGE_ERROR;
	// Если семантика секции полей нарушена
	if(!this->validateSection(sid, trailer, error)){
		// Обрываем поток с кодом ошибки сообщения
		this->sendReset(sid, error);
		// Выводим результат доставки
		return h3::status_t::OK;
	}
	// Признак информационного ответа сервера
	bool informational = false;
	/**
	 * Информационный ответ (1xx) промежуточный: он доставляется, но сообщения
	 * не завершает и фазу приёма не начинает (RFC 9110 §15.2)
	 */
	if(!trailer && (this->_direct == direct_t::RESPONSE)){
		/**
		 * Выполняем поиск псевдо-заголовка статуса
		 */
		for(const auto & field : this->_fields){
			// Если найден псевдо-заголовок статуса
			if(field.name == header::STATUS){
				// Код ответа сервера
				uint16_t code = 0;
				// Выполняем разбор кода ответа сервера
				if(::parseStatus(field.value, code))
					// Запоминаем признак информационного ответа
					informational = ((code >= 100) && (code < 200));
				// Прекращаем поиск
				break;
			}
		}
	}
	// Определяем часть сообщения, которой принадлежит секция
	const part_t part = (trailer ? part_t::TRAILER : part_t::HEADERS);
	// Если принята первая финальная секция полей сообщения
	if(!trailer && !informational){
		// Извещаем обработчик о начале приёма сообщения потока
		if(!this->firePhase(sid, phase_t::BEGIN, part_t::NONE)){
			// Если состояние соединения пережило обработчик
			if(epoch == this->_epoch)
				// Обрываем поток с кодом отмены запроса
				this->sendReset(sid, error_t::H3_REQUEST_CANCELLED);
			// Выводим результат доставки
			return h3::status_t::OK;
		}
		// Если состояние соединения не пережило обработчик
		if(epoch != this->_epoch)
			// Выводим результат доставки
			return h3::status_t::OK;
	}
	// Если принята секция трейлеров
	if(trailer){
		// Если поток не пережил обработчик
		if(!this->aliveStream(sid, generation))
			// Выводим результат доставки
			return h3::status_t::OK;
		// Обновляем состояние потока
		stream = this->findStream(sid);
		/**
		 * Секция трейлеров завершает тело сообщения, поэтому фаза приёма тела
		 * закрывается до неё: иначе события пришли бы в обратном порядке
		 */
		if(stream->body){
			// Снимаем признак незавершённой фазы приёма тела
			stream->body = false;
			// Извещаем обработчик о завершении фазы приёма тела
			if(!this->firePhase(sid, phase_t::END, part_t::BODY)){
				// Если состояние соединения пережило обработчик
				if(epoch == this->_epoch)
					// Обрываем поток с кодом отмены запроса
					this->sendReset(sid, error_t::H3_REQUEST_CANCELLED);
				// Выводим результат доставки
				return h3::status_t::OK;
			}
			// Если состояние соединения не пережило обработчик
			if(epoch != this->_epoch)
				// Выводим результат доставки
				return h3::status_t::OK;
		}
		// Извещаем обработчик о начале приёма секции трейлеров
		if(!this->firePhase(sid, phase_t::BEGIN, part_t::TRAILER)){
			// Если состояние соединения пережило обработчик
			if(epoch == this->_epoch)
				// Обрываем поток с кодом отмены запроса
				this->sendReset(sid, error_t::H3_REQUEST_CANCELLED);
			// Выводим результат доставки
			return h3::status_t::OK;
		}
		// Если состояние соединения не пережило обработчик
		if(epoch != this->_epoch)
			// Выводим результат доставки
			return h3::status_t::OK;
	}
	/**
	 * Выполняем доставку всех полей секции
	 */
	for(const auto & field : this->_fields){
		// Отдаём поле потребителю
		if(!this->fireHeader(sid, field.name, field.value, part)){
			// Если состояние соединения пережило обработчик
			if(epoch == this->_epoch)
				// Обрываем поток с кодом отмены запроса
				this->sendReset(sid, error_t::H3_REQUEST_CANCELLED);
			// Выводим результат доставки
			return h3::status_t::OK;
		}
		/**
		 * Список полей и арена декодера уничтожены сбросом парсера, продолжать
		 * перебор нельзя: следующий шаг обратился бы к освобождённой памяти
		 */
		if(epoch != this->_epoch)
			// Выводим результат доставки
			return h3::status_t::OK;
		// Если поле задаёт расширенный приоритет потока (RFC 9218 §5)
		if(!trailer && (field.name == header::PRIORITY)){
			// Обновляем состояние потока
			stream = this->findStream(sid);
			/**
			 * Если поток пережил обработчики - применяем приоритет. Кадр
			 * PRIORITY_UPDATE перекрывает любой другой сигнал приоритета (§7),
			 * причём независимо от порядка прихода: он вправе опередить секцию
			 * полей, и решать эту гонку RFC предписывает в его пользу
			 */
			if(this->aliveStream(sid, generation) && !stream->prioritized)
				// Применяем значение заголовка приоритета
				this->applyPriority(* stream, field.value);
		}
	}
	// Собираем провайдер полей потока: для секции трейлеров провайдера нет
	unique_ptr <provider_t> provider = (trailer ? nullptr : this->buildProvider(this->_direct == direct_t::REQUEST));
	// Отдаём провайдер полей потребителю
	if(!this->fireProvider(sid, provider.get(), fin)){
		// Если состояние соединения пережило обработчик
		if(epoch == this->_epoch)
			// Обрываем поток с кодом отмены запроса
			this->sendReset(sid, error_t::H3_REQUEST_CANCELLED);
		// Выводим результат доставки
		return h3::status_t::OK;
	}
	// Если состояние соединения не пережило обработчик
	if(epoch != this->_epoch)
		// Выводим результат доставки
		return h3::status_t::OK;
	// Если поток не пережил обработчик
	if(!this->aliveStream(sid, generation))
		// Выводим результат доставки
		return h3::status_t::OK;
	// Обновляем состояние потока
	stream = this->findStream(sid);
	// Если доставлена секция трейлеров
	if(trailer){
		// Запоминаем приём секции трейлеров
		stream->trailers = true;
		// Извещаем обработчик о завершении приёма секции трейлеров
		if(!this->firePhase(sid, phase_t::END, part_t::TRAILER)){
			// Если состояние соединения пережило обработчик
			if(epoch == this->_epoch)
				// Обрываем поток с кодом отмены запроса
				this->sendReset(sid, error_t::H3_REQUEST_CANCELLED);
			// Выводим результат доставки
			return h3::status_t::OK;
		}
	// Если доставлена финальная секция полей
	} else if(!informational) {
		// Запоминаем приём финальной секции полей
		stream->headers = true;
		/**
		 * Признак безтелесности снимается до выхода наружу: обработчик фазы вправе
		 * закрыть поток, и объект потока к проверке ниже уже не существует. Значение
		 * при этом не устареет - оно выведено из секции полей, которая уже разобрана
		 */
		const bool headless = stream->headless;
		// Извещаем обработчик о завершении приёма секции полей
		if(!this->firePhase(sid, phase_t::END, part_t::HEADERS)){
			// Если состояние соединения пережило обработчик
			if(epoch == this->_epoch)
				// Обрываем поток с кодом отмены запроса
				this->sendReset(sid, error_t::H3_REQUEST_CANCELLED);
			// Выводим результат доставки
			return h3::status_t::OK;
		}
		// Если состояние соединения не пережило обработчик
		if(epoch != this->_epoch)
			// Выводим результат доставки
			return h3::status_t::OK;
		/**
		 * Фаза приёма тела открывается, когда тело ещё возможно: пир оставил поток
		 * открытым и кадрирование тела не исключает. По факту прихода данных её
		 * открывать нельзя - последовательность событий зависела бы от того, прислал
		 * ли пир пустой кадр DATA, а это его выбор нарезки, а не свойство сообщения.
		 * Безтелесные сообщения (204, 304, ответ на HEAD) исключение: там тела не
		 * будет независимо от кадрирования, и фаза приёма тела попросту солгала бы.
		 * Ровно так же поступает HTTP/1: на chunked без единого чанка фазу открывает,
		 * а на 204 с тем же chunked - нет
		 */
		if(!fin && !headless){
			// Если поток не пережил обработчик
			if(!this->aliveStream(sid, generation))
				// Выводим результат доставки
				return h3::status_t::OK;
			// Обновляем состояние потока
			stream = this->findStream(sid);
			// Запоминаем начало фазы приёма тела
			stream->body = true;
			// Извещаем обработчик о начале фазы приёма тела
			if(!this->firePhase(sid, phase_t::BEGIN, part_t::BODY)){
				// Если состояние соединения пережило обработчик
				if(epoch == this->_epoch)
					// Обрываем поток с кодом отмены запроса
					this->sendReset(sid, error_t::H3_REQUEST_CANCELLED);
				// Выводим результат доставки
				return h3::status_t::OK;
			}
		}
	}
	// Выводим результат доставки
	return h3::status_t::OK;
}
/**
 * @brief Метод доставки декодированной секции обещанного запроса
 *
 * @param sid    идентификатор ассоциированного потока
 * @param pushId идентификатор обещанного push
 * @return       результат доставки
 *
 */
/**
 * @brief Метод сверки секции полей повторно обещанного push
 *
 * @param pushId идентификатор обещанного push
 * @return       результат сверки
 *
 */
awh::http::h3::status_t awh::http::Parser_HTTP3::checkPromise(const uint64_t pushId) noexcept {
	// Отпечаток секции полей обещания
	size_t digest = this->_fields.size();
	/**
	 * Собираем отпечаток секции по её декодированным полям: одинаковые обещания
	 * дают одинаковую последовательность полей, а значит и одинаковый отпечаток.
	 * Длины подмешиваются вместе с содержимым: без них перенос символов через
	 * границу названия и значения отпечатка не меняет
	 */
	for(const auto & field : this->_fields){
		// Подмешиваем длину названия поля в отпечаток
		digest = (digest * 31 + field.name.size());
		// Подмешиваем название поля в отпечаток
		digest = (digest * 31 + ::std::hash <string_view> {}(field.name));
		// Подмешиваем длину значения поля в отпечаток
		digest = (digest * 31 + field.value.size());
		// Подмешиваем значение поля в отпечаток
		digest = (digest * 31 + ::std::hash <string_view> {}(field.value));
	}
	/**
	 * Выполняем поиск обещания в кольце
	 */
	for(auto & item : this->_promisedPush){
		// Если обещание с таким идентификатором не встречалось - идём дальше
		if(item.id != pushId)
			// Переходим к следующей ячейке кольца
			continue;
		/**
		 * Повтор идентификатора допустим: один push сервер вправе пообещать
		 * на нескольких потоках запросов. Недопустимо расхождение секций
		 */
		if(item.digest != digest)
			// Фиксируем ошибку уровня соединения
			return this->fail(error_t::H3_GENERAL_PROTOCOL_ERROR, "секции повторного обещания push расходятся");
		// Выводим результат сверки
		return h3::status_t::OK;
	}
	// Записываем идентификатор обещания в текущую ячейку кольца
	this->_promisedPush[this->_promisedCursor].id = pushId;
	// Записываем отпечаток секции обещания в текущую ячейку кольца
	this->_promisedPush[this->_promisedCursor].digest = digest;
	// Продвигаем позицию записи по кольцу
	this->_promisedCursor = ((this->_promisedCursor + 1) % this->_promisedPush.size());
	// Выводим результат сверки
	return h3::status_t::OK;
}
/**
 * @brief Метод доставки декодированной секции полей обещанного запроса
 *
 * @param sid    идентификатор потока
 * @param pushId идентификатор обещанного push
 * @return       результат доставки
 *
 */
awh::http::h3::status_t awh::http::Parser_HTTP3::deliverPromise(const uint64_t sid, const uint64_t pushId) noexcept {
	/**
	 * Поля обещания ссылаются в арену декодера, а обработчик вправе снести её,
	 * вызвав clear() либо reset() прямо изнутри. Поэтому после каждого выхода
	 * наружу проверяется поколение состояния соединения
	 */
	// Код ошибки семантики секции обещания
	error_t error = error_t::H3_MESSAGE_ERROR;
	/**
	 * Обещание несёт запрос, и секция его обязана быть корректным запросом
	 * (RFC 9114 §4.6): искажённое обещание - ошибка сообщения, а не повод
	 * отдать наружу мусор. Проверяется оно как запрос при любом направлении
	 * разбора: на клиенте, который обещания и принимает, направление разбора -
	 * ответ. Вместе с семантикой на этом пути включаются и лимиты секции:
	 * без вызова из защит оставался только лимит распакованного списка
	 */
	if(!this->validateSection(sid, false, error, true)){
		// Отменяем обещанный push
		this->sendCancelPush(pushId);
		// Выводим результат доставки
		return h3::status_t::OK;
	}
	/**
	 * Секции повторных обещаний одного push обязаны совпадать: расхождение -
	 * ошибка соединения, и проверяется оно до выхода наружу (RFC 9114 §7.2.5)
	 */
	if(this->checkPromise(pushId) != h3::status_t::OK)
		// Выводим результат доставки
		return h3::status_t::ERROR;
	// Запоминаем поколение состояния соединения перед выходами в обработчики
	const uint64_t epoch = this->_epoch;
	// Если обработчик отклонил обещанный push
	if(!this->firePush(sid, pushId)){
		// Если состояние соединения пережило обработчик
		if(epoch == this->_epoch)
			// Отменяем обещанный push
			this->sendCancelPush(pushId);
		// Выводим результат доставки
		return h3::status_t::OK;
	}
	// Если состояние соединения не пережило обработчик
	if(epoch != this->_epoch)
		// Выводим результат доставки
		return h3::status_t::OK;
	/**
	 * Выполняем доставку всех полей обещанного запроса
	 */
	for(const auto & field : this->_fields){
		// Отдаём поле потребителю
		if(!this->fireHeader(sid, field.name, field.value, part_t::HEADERS)){
			// Если состояние соединения пережило обработчик
			if(epoch == this->_epoch)
				// Отменяем обещанный push
				this->sendCancelPush(pushId);
			// Выводим результат доставки
			return h3::status_t::OK;
		}
		/**
		 * Список полей и арена декодера уничтожены сбросом парсера, продолжать
		 * перебор нельзя: следующий шаг обратился бы к освобождённой памяти
		 */
		if(epoch != this->_epoch)
			// Выводим результат доставки
			return h3::status_t::OK;
	}
	/**
	 * Обещание push всегда несёт запрос, каким бы ни было направление разбора:
	 * его отправляет сервер от имени клиента (RFC 9114 §4.6)
	 */
	unique_ptr <provider_t> provider = this->buildProvider(true);
	// Отдаём провайдер полей обещанного запроса потребителю
	if(!this->fireProvider(sid, provider.get(), true) && (epoch == this->_epoch))
		// Отменяем обещанный push
		this->sendCancelPush(pushId);
	// Выводим результат доставки
	return h3::status_t::OK;
}
/**
 * @brief Метод проверки семантики секции полей (RFC 9114 §4.1, §4.2)
 *
 * @param sid     идентификатор потока
 * @param trailer признак секции трейлеров
 * @param error   код ошибки протокола
 * @param promise признак секции обещанного запроса
 * @return        результат проверки
 *
 */
bool awh::http::Parser_HTTP3::validateSection(const uint64_t sid, const bool trailer, error_t & error, const bool promise) noexcept {
	// Устанавливаем код ошибки семантики сообщения
	error = error_t::H3_MESSAGE_ERROR;
	/**
	 * Обещание push всегда несёт запрос, каким бы ни было направление разбора:
	 * его отправляет сервер от имени клиента (RFC 9114 §4.6)
	 */
	const bool request = (promise || (this->_direct == direct_t::REQUEST));
	// Признак работы парсера промежуточным узлом (RFC 9114 §10.3)
	const bool proxy = (this->_proto == proto_t::PROXY3);
	// Признак соединения, несущего WebSocket поверх расширенного CONNECT (RFC 9220 §3)
	const bool websocket = (this->_proto == proto_t::WEBSOCKET3);
	/**
	 * Состояние потока обещанию не принадлежит: кадр приходит на поток запроса,
	 * ответом на который он является, а собственный поток push откроется позже.
	 * Писать в чужое состояние объявленную длину тела и безтелесность нельзя
	 */
	stream_t * stream = (promise ? nullptr : this->findStream(sid));
	// Если поток уже закрыт
	if(!promise && (stream == nullptr))
		// Выводим отрицательный результат
		return false;
	// Признак того, что обычные поля уже начались
	bool ordinary = false;
	// Количество проверенных полей
	size_t count = 0;
	// Признаки наличия псевдо-заголовков
	bool hasMethod = false, hasScheme = false, hasPath = false;
	// Признаки наличия остальных псевдо-заголовков
	bool hasAuthority = false, hasProtocol = false, hasStatus = false;
	// Признак наличия поля адресата HTTP/1.1
	bool hasHost = false;
	// Значения псевдо-заголовков, участвующих в перекрёстных проверках
	string_view method, authority, status, path, host, scheme, protocol;
	// Объявленная длина тела сообщения
	uint64_t declared = UINT64_MAX;
	/**
	 * Выполняем проверку всех полей секции
	 */
	for(const auto & field : this->_fields){
		// Если количество полей превысило лимит
		if((++count) > this->_limits.maxHeaderCount)
			// Выводим отрицательный результат
			return false;
		// Если длина названия либо значения поля превысила лимит
		if((field.name.size() > this->_limits.maxHeaderName) || (field.value.size() > this->_limits.maxHeaderValue))
			// Выводим отрицательный результат
			return false;
		// Если название либо значение поля синтаксически некорректно
		if(!::validName(field.name) || !::validValue(field.value))
			// Выводим отрицательный результат
			return false;
		/**
		 * Узел, передающий сообщение дальше по цепочке, проверяет значение по
		 * грамматике field-content целиком: конечному получателю достаточно
		 * минимальной проверки, а транслирующему - нет (RFC 9114 §10.3)
		 */
		if(proxy && !::translatable(field.value))
			// Выводим отрицательный результат
			return false;
		// Если поле является псевдо-заголовком
		if(::isPseudo(field.name)){
			/**
			 * Псевдо-заголовки обязаны идти строго перед обычными полями и
			 * отсутствовать в секции трейлеров (RFC 9114 §4.3)
			 */
			if(ordinary || trailer)
				// Выводим отрицательный результат
				return false;
			// Если разбирается запрос клиента либо обещанный сервером запрос
			if(request){
				// Если получен псевдо-заголовок метода
				if(field.name == header::METHOD){
					// Повторный псевдо-заголовок недопустим
					if(hasMethod)
						// Выводим отрицательный результат
						return false;
					// Запоминаем наличие псевдо-заголовка метода
					hasMethod = true;
					// Пустой метод запроса недопустим - это токен (RFC 9110 §9.1)
					if(field.value.empty())
						// Выводим отрицательный результат
						return false;
					// Запоминаем значение метода запроса
					method = field.value;
				// Если получен псевдо-заголовок схемы
				} else if(field.name == header::SCHEME) {
					// Повторный псевдо-заголовок недопустим
					if(hasScheme)
						// Выводим отрицательный результат
						return false;
					// Запоминаем наличие псевдо-заголовка схемы
					hasScheme = true;
					// Пустая схема запроса недопустима (RFC 9114 §4.3.1)
					if(field.value.empty())
						// Выводим отрицательный результат
						return false;
					// Запоминаем значение схемы запроса
					scheme = field.value;
				// Если получен псевдо-заголовок пути
				} else if(field.name == header::PATH) {
					// Повторный псевдо-заголовок недопустим
					if(hasPath)
						// Выводим отрицательный результат
						return false;
					// Запоминаем наличие псевдо-заголовка пути
					hasPath = true;
					// Запоминаем значение пути запроса
					path = field.value;
				// Если получен псевдо-заголовок адресата
				} else if(field.name == header::AUTHORITY) {
					// Повторный псевдо-заголовок недопустим
					if(hasAuthority)
						// Выводим отрицательный результат
						return false;
					// Запоминаем наличие псевдо-заголовка адресата
					hasAuthority = true;
					// Запоминаем значение адресата запроса
					authority = field.value;
				// Если получен псевдо-заголовок протокола туннеля (RFC 9220 §4)
				} else if(field.name == header::PROTOCOL) {
					// Повторный псевдо-заголовок недопустим
					if(hasProtocol)
						// Выводим отрицательный результат
						return false;
					// Запоминаем наличие псевдо-заголовка протокола туннеля
					hasProtocol = true;
					// Запоминаем значение протокола туннеля
					protocol = field.value;
					// Пустой протокол туннеля недопустим (RFC 9220 §4)
					if(field.value.empty())
						// Выводим отрицательный результат
						return false;
				/**
				 * Неизвестный псевдо-заголовок недопустим: перечень закрыт,
				 * а расширения обязаны его пополнять явно (RFC 9114 §4.3)
				 */
				} else return false;
			// Если разбирается ответ сервера
			} else {
				// Если получен псевдо-заголовок статуса
				if(field.name == header::STATUS){
					// Повторный псевдо-заголовок недопустим
					if(hasStatus)
						// Выводим отрицательный результат
						return false;
					// Запоминаем наличие псевдо-заголовка статуса
					hasStatus = true;
					// Запоминаем значение статуса ответа
					status = field.value;
				// Неизвестный псевдо-заголовок ответа недопустим
				} else return false;
			}
			// Переходим к следующему полю секции
			continue;
		}
		// Запоминаем начало обычных полей
		ordinary = true;
		/**
		 * Поля управления соединением принадлежат HTTP/1.1: в HTTP/3 соединением
		 * распоряжается транспорт (RFC 9114 §4.2)
		 */
		if(::isForbidden(field.name))
			// Выводим отрицательный результат
			return false;
		/**
		 * Единственное допустимое значение поля [te] - trailers: остальные
		 * описывают кодирование передачи, которого в HTTP/3 нет
		 */
		if((field.name == header::TE) && (field.value != value::TRAILERS))
			// Выводим отрицательный результат
			return false;
		// Если поле недопустимо в секции трейлеров (RFC 9110 §6.5.1)
		if(trailer && ::isForbiddenTrailer(field.name))
			// Выводим отрицательный результат
			return false;
		// Если получено поле адресата HTTP/1.1
		if(field.name == header::HOST){
			// Запоминаем наличие поля адресата
			hasHost = true;
			// Запоминаем значение поля адресата
			host = field.value;
		}
		// Если получено поле объявленной длины тела
		if(field.name == header::CONTENT_LENGTH){
			// Разобранная длина тела сообщения
			uint64_t length = 0;
			// Если значение поля не является числом
			if(!::parseLength(field.value, length))
				// Выводим отрицательный результат
				return false;
			// Повторное поле с иным значением недопустимо (RFC 9110 §8.6)
			if((declared != UINT64_MAX) && (declared != length))
				// Выводим отрицательный результат
				return false;
			// Запоминаем объявленную длину тела сообщения
			declared = length;
		}
	}
	// Секция трейлеров дальнейшим проверкам не подлежит
	if(trailer)
		// Выводим положительный результат
		return true;
	// Если разбирается запрос клиента либо обещанный сервером запрос
	if(request){
		// Если обязательный псевдо-заголовок метода отсутствует
		if(!hasMethod)
			// Выводим отрицательный результат
			return false;
		/**
		 * Из псевдо-заголовков запроса следующее звено собирает стартовую строку,
		 * а отдельных правил их проверки протокол не содержит: RFC 9114 §10.3
		 * оставляет её тому, кто значения использует. Пробел либо управляющий
		 * символ внутри значения расщепляет стартовую строку на элементы, которых
		 * отправитель туда не помещал, а метод сверх того обязан быть токеном
		 * (RFC 9110 §9.1)
		 */
		if(proxy){
			// Если метод запроса токеном не является
			if(!::tokenValue(method))
				// Выводим отрицательный результат
				return false;
			// Если схема, адресат либо путь запроса к сборке стартовой строки непригодны
			if(!::translatablePseudo(scheme) || !::translatablePseudo(authority) || !::translatablePseudo(path))
				// Выводим отрицательный результат
				return false;
		}
		// Признак метода установления туннеля
		const bool connect = (method == value::CONNECT);
		/**
		 * Классический CONNECT адресует узел, а не ресурс: схема и путь у него
		 * отсутствуют, а адресат обязателен (RFC 9114 §4.4)
		 */
		if(connect && !hasProtocol){
			/**
			 * Схема либо путь присутствовать не должны, а адресат обязан быть
			 * и непустым: пустая строка не описывает узел назначения туннеля
			 */
			if(hasScheme || hasPath || !hasAuthority || authority.empty())
				// Выводим отрицательный результат
				return false;
		// Если запрос адресует ресурс
		} else {
			// Схема и путь обязательны для любого запроса, кроме классического CONNECT
			if(!hasScheme || !hasPath)
				// Выводим отрицательный результат
				return false;
			// Пустой путь недопустим (RFC 9114 §4.3.1)
			if(path.empty())
				// Выводим отрицательный результат
				return false;
			// Если запрос несёт псевдо-заголовок протокола туннеля
			if(hasProtocol){
				// Расширенный CONNECT существует только для метода CONNECT (RFC 9220 §4)
				if(!connect)
					// Выводим отрицательный результат
					return false;
				/**
				 * Расширенный CONNECT допустим, только если мы сами его анонсировали:
				 * иначе клиент пользуется разрешением, которого не получал
				 */
				if(!this->connectProtocol())
					// Выводим отрицательный результат
					return false;
			}
		}
		/**
		 * Схемы [http] и [https] задают форму цели запроса (RFC 9114 §4.3.1):
		 * путь либо начинается с косой черты, либо равен звёздочке, и звёздочка
		 * допустима только методу OPTIONS - она адресует сервер целиком, а не
		 * ресурс. Прочие схемы проверке не подлежат: их форму задаёт не HTTP
		 */
		if(::httpScheme(scheme) && hasPath){
			// Если путь не начинается с косой черты
			if(path.empty() || (path.front() != '/')){
				// Звёздочка допустима только методу OPTIONS
				if((path != "*") || (method != value::OPTIONS))
					// Выводим отрицательный результат
					return false;
			}
		}
		/**
		 * У схем с обязательным адресатом (http/https) запрос обязан нести
		 * :authority либо Host, и присутствующее поле не может быть пустым
		 * (RFC 9114 §4.3.1)
		 */
		if(::httpScheme(scheme)){
			// Если адресат не задан ни псевдо-заголовком, ни полем Host
			if(!hasAuthority && !hasHost)
				// Выводим отрицательный результат
				return false;
			// Если псевдо-заголовок адресата присутствует пустым
			if(hasAuthority && authority.empty())
				// Выводим отрицательный результат
				return false;
			// Если поле Host присутствует пустым
			if(hasHost && host.empty())
				// Выводим отрицательный результат
				return false;
		}
		/**
		 * Устаревший подкомпонент userinfo в адресате запрещён для схем [http]
		 * и [https] (RFC 9114 §4.3.1): он переносил бы в запрос учётные данные,
		 * которым место в заголовке авторизации. Отделяет его символ [@]
		 */
		if(::httpScheme(scheme) && (authority.find('@') != string_view::npos))
			// Выводим отрицательный результат
			return false;
		/**
		 * Схему цели запроса WebSocket задаёт RFC 8441 §5, и RFC 9220 §3 переносит
		 * его правила в HTTP/3 без изменений: [https] для адресов [wss] и [http]
		 * для адресов [ws]. Схема самого адреса WebSocket в псевдо-заголовок не
		 * переносится - [ws] и [wss] задают форму адреса, а не цели запроса, и узел,
		 * принявший их за схему цели, соберёт из псевдо-заголовков не тот URI,
		 * который имел в виду отправитель. Проверка привязана к роли: на обычном
		 * соединении расширенный CONNECT поднимает туннель любого зарегистрированного
		 * протокола, и правила WebSocket к нему не относятся
		 */
		if(websocket && hasProtocol && this->_fmk->compare("websocket", protocol) && !::httpScheme(scheme))
			// Выводим отрицательный результат
			return false;
		/**
		 * Поле адресата HTTP/1.1 обязано совпадать с псевдо-заголовком адресата:
		 * расхождение позволило бы протащить через шлюз два разных адресата
		 * в одном запросе (RFC 9114 §4.3.1)
		 */
		if(hasHost && hasAuthority && (host != authority))
			// Выводим отрицательный результат
			return false;
	// Если разбирается ответ сервера
	} else {
		// Если обязательный псевдо-заголовок статуса отсутствует
		if(!hasStatus)
			// Выводим отрицательный результат
			return false;
		// Код ответа сервера
		uint16_t code = 0;
		// Если код ответа сервера синтаксически некорректен
		if(!::parseStatus(status, code))
			// Выводим отрицательный результат
			return false;
		/**
		 * Смена протокола принадлежит HTTP/1.1 и в HTTP/3 смысла не имеет:
		 * туннели поднимаются расширенным CONNECT (RFC 9114 §4.1)
		 */
		if(code == 101)
			// Выводим отрицательный результат
			return false;
		/**
		 * Ответы 204 и 304 тела не несут по определению, поэтому объявленная
		 * длина тела к ним не применяется (RFC 9110 §8.6)
		 */
		if((status == value::NO_CONTENT) || (status == value::NOT_MODIFIED)){
			// Запоминаем безтелесность сообщения
			stream->headless = true;
			// Запоминаем что сообщение не может нести и секцию трейлеров
			stream->trailerless = true;
		}
		/**
		 * Ответы 1xx и [204 No Content] тела не несут по определению, и в HTTP/3
		 * объявленная у них длина попросту игнорируется (RFC 9114 §4.1.2 разрешает
		 * ненулевой Content-Length прямо). Но узел, передающий такой ответ дальше
		 * по цепочке, отправить это поле следующему звену не вправе (RFC 9112 §6.1),
		 * а звено, всё же его получившее и уважившее, прочитает следующий ответ как
		 * тело этого - граница сообщений в цепочке разойдётся. Ответу
		 * [304 Not Modified] и ответу на HEAD поле, напротив, разрешено: оно
		 * описывает тело, которое было бы отправлено в ответ на такой же запрос GET
		 */
		if(proxy && (declared != UINT64_MAX) && (((code >= 100) && (code < 200)) || (code == 204)))
			// Выводим отрицательный результат
			return false;
	}
	// Если проверялась секция потока, а не обещания
	if(!promise){
		// Запоминаем объявленную длину тела сообщения
		stream->declared = declared;
		/**
		 * Ответ на запрос методом HEAD содержимого не несёт (RFC 9110 §9.3.2).
		 * У обещания push проверять нечего: секцию несёт поток запроса, а ответ
		 * на само обещание придёт по отдельному потоку push
		 */
		if(request && (method == value::HEAD))
			// Запоминаем безтелесность отправляемого нами ответа
			stream->headlessSend = true;
	}
	// Выводим положительный результат
	return true;
}
/**
 * @brief Метод сборки провайдера полей потока
 *
 * @param request признак сборки провайдера запроса клиента
 * @return        собранный провайдер полей потока
 *
 */
unique_ptr <awh::http::provider_t> awh::http::Parser_HTTP3::buildProvider(const bool request) const noexcept {
	// Собираемый провайдер полей потока
	unique_ptr <provider_t> result = nullptr;
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		// Если собирается запрос клиента
		if(request){
			// Создаём объект провайдера запроса клиента
			unique_ptr <request_t> provider(new request_t(version_t::HTTP3));
			/**
			 * Выполняем перебор всех полей секции
			 */
			for(const auto & field : this->_fields){
				// Если получен псевдо-заголовок метода запроса
				if(field.name == header::METHOD){
					// Выполняем классификацию метода запроса по его имени
					provider->method = awh::http::classifyMethod(field.value);
					// Если метод запроса синтаксически корректен, но не распознан
					if(provider->method == method_t::NONE){
						// Помечаем метод запроса как нераспознанный
						provider->method = method_t::UNKNOWN;
						// Сохраняем оригинальное написание метода
						provider->methodName = field.value;
					}
				// Если получен псевдо-заголовок пути запроса
				} else if(field.name == header::PATH)
					// Устанавливаем параметры URI-запроса
					provider->uri = field.value;
				// Если получен псевдо-заголовок протокола туннеля
				else if(field.name == header::PROTOCOL)
					// Устанавливаем протокол туннеля
					provider->protocol = field.value;
			}
			// Устанавливаем собранный провайдер как результат
			result = ::std::move(provider);
		// Если собирается ответ сервера
		} else {
			// Создаём объект провайдера ответа сервера
			unique_ptr <response_t> provider(new response_t(version_t::HTTP3));
			/**
			 * Выполняем перебор всех полей секции
			 */
			for(const auto & field : this->_fields){
				// Если получен псевдо-заголовок статуса ответа
				if(field.name == header::STATUS){
					// Код ответа сервера
					uint16_t code = 0;
					// Выполняем разбор кода ответа сервера
					if(::parseStatus(field.value, code)){
						// Устанавливаем код ответа сервера
						provider->code = code;
						// Устанавливаем стандартное сообщение сервера
						provider->message = statusMessage(code);
					}
					// Прекращаем перебор полей секции
					break;
				}
			}
			// Устанавливаем собранный провайдер как результат
			result = ::std::move(provider);
		}
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception &) {
		// Провайдер собрать не удалось
		result = nullptr;
	}
	// Выводим собранный провайдер полей потока
	return result;
}

/**
 * @brief Метод вызова функции обратного вызова фазы приёма сообщения потока
 *
 * @param sid   идентификатор потока
 * @param phase фаза приёма сообщения потока
 * @param part  часть сообщения
 * @return      результат вызова (false - поток обрывается)
 *
 */
bool awh::http::Parser_HTTP3::firePhase(const uint64_t sid, const phase_t phase, const part_t part) noexcept {
	// Если функция обратного вызова не установлена
	if(!this->_callbacks.phase)
		// Разбор продолжается
		return true;
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		// Извещаем обработчик о фазе приёма сообщения потока
		return this->_callbacks.phase(sid, phase, part);
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception &) {
		/**
		 * Исключение из пользовательской функции обратного вызова перехватывается
		 * на месте вызова: обрывается поток, а соединение и процесс живут
		 */
		return false;
	}
}
/**
 * @brief Метод вызова функции обратного вызова открытия нового потока
 *
 * @param sid идентификатор потока
 * @return    результат вызова (false - поток обрывается)
 *
 */
bool awh::http::Parser_HTTP3::fireBegin(const uint64_t sid) noexcept {
	// Если функция обратного вызова не установлена
	if(!this->_callbacks.begin)
		// Разбор продолжается
		return true;
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		// Извещаем обработчик об открытии нового потока
		return this->_callbacks.begin(sid);
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception &) {
		// Поток обрывается
		return false;
	}
}
/**
 * @brief Метод вызова функции обратного вызова анонса server push
 *
 * @param sid    идентификатор ассоциированного потока
 * @param pushId идентификатор обещанного push
 * @return       результат вызова (false - push отклоняется)
 *
 */
bool awh::http::Parser_HTTP3::firePush(const uint64_t sid, const uint64_t pushId) noexcept {
	// Если функция обратного вызова не установлена - push отклоняется
	if(!this->_callbacks.push)
		// Обещанный push отклоняется
		return false;
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		// Извещаем обработчик об анонсе server push
		return this->_callbacks.push(sid, pushId);
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception &) {
		// Обещанный push отклоняется
		return false;
	}
}
/**
 * @brief Метод вызова функции обратного вызова провайдера полей потока
 *
 * @param sid       идентификатор потока
 * @param provider  провайдер полей потока
 * @param endStream признак завершения потока
 * @return          результат вызова (false - поток обрывается)
 *
 */
bool awh::http::Parser_HTTP3::fireProvider(const uint64_t sid, const provider_t * provider, const bool endStream) noexcept {
	// Если функция обратного вызова не установлена
	if(!this->_callbacks.provider)
		// Разбор продолжается
		return true;
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		// Извещаем обработчик о провайдере полей потока
		return this->_callbacks.provider(sid, provider, endStream);
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception &) {
		// Поток обрывается
		return false;
	}
}
/**
 * @brief Метод вызова функции обратного вызова поля секции
 *
 * @param sid   идентификатор потока
 * @param name  название поля
 * @param value значение поля
 * @param part  часть сообщения
 * @return      результат вызова (false - поток обрывается)
 *
 */
bool awh::http::Parser_HTTP3::fireHeader(const uint64_t sid, const string_view name, const string_view value, const part_t part) noexcept {
	// Если функция обратного вызова не установлена
	if(!this->_callbacks.header)
		// Разбор продолжается
		return true;
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		// Извещаем обработчик о поле секции
		return this->_callbacks.header(sid, name, value, part);
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception &) {
		// Поток обрывается
		return false;
	}
}
/**
 * @brief Метод вызова функции обратного вызова фрагмента тела потока
 *
 * @param sid       идентификатор потока
 * @param buffer    буфер данных тела
 * @param size      размер данных тела
 * @param endStream признак завершения потока
 * @return          результат вызова (false - поток обрывается)
 *
 */
bool awh::http::Parser_HTTP3::fireData(const uint64_t sid, const void * buffer, const size_t size, const bool endStream) noexcept {
	// Если функция обратного вызова не установлена
	if(!this->_callbacks.data)
		// Разбор продолжается
		return true;
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		// Извещаем обработчик о фрагменте тела потока
		return this->_callbacks.data(sid, buffer, size, endStream);
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception &) {
		// Поток обрывается
		return false;
	}
}
/**
 * @brief Метод разбора данных соединения
 *
 * @param buffer буфер данных
 * @param size   размер данных
 * @return       количество разобранных байт (всегда 0)
 *
 */
size_t awh::http::Parser_HTTP3::parse(const void * buffer, const size_t size) noexcept {
	// Буфер и его размер в разборе не участвуют
	(void) buffer;
	(void) size;
	/**
	 * Единого байтового потока у соединения HTTP/3 нет: данные всегда принадлежат
	 * конкретному потоку QUIC. Молчаливый отказ выглядел бы как исправная работа,
	 * поэтому обращение по неприменимой сигнатуре фиксируется ошибкой
	 */
	this->fail(error_t::H3_INTERNAL_ERROR, "HTTP/3 разбирает данные потоков: используйте parse(sid, ...)");
	// Выводим количество разобранных байт
	return 0;
}
/**
 * @brief Метод обработки завершения ввода
 *
 */
void awh::http::Parser_HTTP3::eof() noexcept {
	/**
	 * Завершение ввода означает обрыв соединения транспортом: потоки, не успевшие
	 * принять сообщение целиком, закрываются с признаком неполноты
	 */
	::Borrowed outgoing(this->_outgoing);
	/**
	 * Собираем список потоков заранее: закрытие потока из обработчика разрушило бы
	 * перебор карты
	 */
	for(const auto & item : this->_streams)
		// Дописываем поток в список закрываемых
		outgoing.list.push_back(item.first);
	/**
	 * Выполняем закрытие всех незавершённых потоков
	 *
	 * Перебор идёт по индексу, а не по итератору: закрытие потока выходит
	 * в пользовательскую функцию обратного вызова, а та вправе закрыть
	 * ещё один поток и тем разрушить итератор списка
	 */
	for(size_t i = 0; i < outgoing.list.size(); i++){
		// Получаем идентификатор очередного закрываемого потока
		const uint64_t sid = outgoing.list[i];
		// Выполняем поиск состояния потока
		stream_t * stream = this->findStream(sid);
		// Если поток уже закрыт
		if(stream == nullptr)
			// Переходим к следующему потоку
			continue;
		// Закрываем поток с признаком неполноты сообщения
		this->closeStream(sid, (stream->completed ? error_t::H3_NO_ERROR : error_t::H3_REQUEST_INCOMPLETE));
	}
	// Запоминаем завершённость соединения
	this->_closed = true;
	// Устанавливаем итоговый статус разбора
	this->_status = ((this->_error == error_t::H3_NO_ERROR) ? status_t::COMPLETE : status_t::ERROR);
}
/**
 * @brief Метод сброса состояния парсера
 *
 */
void awh::http::Parser_HTTP3::reset() noexcept {
	/**
	 * Сдвигаем поколение состояния: если сброс пришёл из пользовательской функции,
	 * это признак для всех идущих разборов немедленно свернуться, не обращаясь
	 * к уже освобождённым состояниям потоков и спискам полей
	 */
	++this->_epoch;
	// Выполняем сброс базового состояния разбора
	parser_t::reset();
	// Выполняем очистку карты потоков запросов
	this->_streams.clear();
	// Выполняем очистку карты однонаправленных потоков
	this->_unistreams.clear();
	// Выполняем очистку буферов исходящих данных
	this->_pending.clear();
	// Выполняем сброс состояния кодера QPACK
	this->_encoder.clear();
	// Выполняем сброс состояния декодера QPACK
	this->_decoder.clear();
	// Сбрасываем идентификатор нашего управляющего потока
	this->_controlLocal = UINT64_MAX;
	// Сбрасываем идентификатор управляющего потока пира
	this->_controlRemote = UINT64_MAX;
	// Сбрасываем идентификатор нашего потока инструкций кодера
	this->_encoderLocal = UINT64_MAX;
	// Сбрасываем идентификатор нашего потока инструкций декодера
	this->_decoderLocal = UINT64_MAX;
	// Сбрасываем идентификатор потока инструкций кодера пира
	this->_encoderRemote = UINT64_MAX;
	// Сбрасываем идентификатор потока инструкций декодера пира
	this->_decoderRemote = UINT64_MAX;
	// Сбрасываем признак получения параметров пира
	this->_settingsReceived = false;
	// Сбрасываем признак отправки наших параметров
	this->_settingsSent = false;
	// Сбрасываем признак завершённости соединения
	this->_closed = false;
	// Сбрасываем границу идентификаторов push, разрешённую пиром
	this->_maxPushId = UINT64_MAX;
	// Сбрасываем границу идентификаторов push, разрешённую нами
	this->_localMaxPushId = UINT64_MAX;
	// Сбрасываем идентификатор следующего выдаваемого push
	this->_nextPushId = 0;
	// Выполняем очистку списка отменённых push
	this->_cancelledPush.clear();
	// Очищаем кольцо пришедших обещаний push
	this->_openedPush.clear();
	// Очищаем кольцо приоритетов ещё не открытых потоков
	this->_pendingPriorities.clear();
	// Очищаем кольцо приоритетов обещаний push
	this->_pushPriorities.clear();
	// Очищаем кольцо отпечатков секций обещаний push
	::std::fill(this->_promisedPush.begin(), this->_promisedPush.end(), promise_t());
	// Сбрасываем позицию записи в кольце обещаний push
	this->_promisedCursor = 0;
	// Сбрасываем идентификатор, объявленный нами в GOAWAY
	this->_goawayLocal = h3::proto::MAX_VARINT;
	// Сбрасываем идентификатор, объявленный пиром в GOAWAY
	this->_goawayRemote = h3::proto::MAX_VARINT;
	// Сбрасываем параметры пира к значениям по умолчанию протокола
	this->_remote = settings_t();
	// Устанавливаем размер таблицы QPACK пира по умолчанию протокола
	this->_remote.qpackMaxTableCapacity = h3::proto::DEFAULT_QPACK_TABLE_CAPACITY;
	// Устанавливаем число ожидающих потоков пира по умолчанию протокола
	this->_remote.qpackBlockedStreams = h3::proto::DEFAULT_QPACK_BLOCKED_STREAMS;
	// Устанавливаем ёмкость таблицы декодера по нашему анонсу
	this->_decoder.maxCapacity(this->_settings.qpackMaxTableCapacity);
	// Устанавливаем число ожидающих потоков декодера по нашему анонсу
	this->_decoder.maxBlocked(this->_settings.qpackBlockedStreams);
	// Инициализируем лимит частоты управляющих кадров
	this->_ctrlLimit.init(this->_limits.ctrlLimitBurst, this->_limits.ctrlLimitRate);
	// Инициализируем лимит частоты кадров приоритета
	this->_priorityLimit.init(this->_limits.prioLimitBurst, this->_limits.prioLimitRate);
	// Сбрасываем код последней ошибки протокола
	this->_error = error_t::H3_NO_ERROR;
}
/**
 * @brief Метод очистки состояния парсера
 *
 */
void awh::http::Parser_HTTP3::clear() noexcept {
	// Выполняем очистку базового состояния разбора
	parser_t::clear();
	// Возвращаем протокол работы парсера к значению по умолчанию
	this->_proto = proto_t::HTTP3;
	// Возвращаем лимиты безопасности к значениям по умолчанию
	this->_limits = limits_t();
	// Возвращаем наши параметры к значениям по умолчанию
	this->_settings = settings_t();
	// Выполняем очистку набора функций обратного вызова
	this->_callbacks = callbacks_t();
	// Выполняем сброс состояния парсера
	this->reset();
}
/**
 * @brief Метод создания копии парсера
 *
 * @return копия парсера
 *
 */
unique_ptr <awh::http::parser_t> awh::http::Parser_HTTP3::clone() const noexcept {
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		// Создаём копию парсера с теми же направлением и настройками
		unique_ptr <Parser_HTTP3> result(new Parser_HTTP3(this->_direct, this->_fmk, this->_log));
		// Переносим протокол работы парсера: роль узла на соединении - такая же настройка, как лимиты
		result->_proto = this->_proto;
		// Переносим лимиты безопасности
		result->_limits = this->_limits;
		// Переносим наши параметры
		result->_settings = this->_settings;
		/**
		 * Состояние соединения не копируется намеренно: динамические таблицы QPACK,
		 * карта потоков и счётчики принадлежат конкретному соединению
		 */
		return result;
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception &) {
		// Копию парсера создать не удалось
		return nullptr;
	}
}
/**
 * @brief Метод получения названия кода последней ошибки
 *
 * @return название кода последней ошибки
 *
 */
string_view awh::http::Parser_HTTP3::errorName() const noexcept {
	// Выводим название кода последней ошибки
	return h3::errorName(this->_error);
}
/**
 * @brief Метод получения названия кода ошибки протокола
 *
 * @param error код ошибки протокола
 * @return      название кода ошибки протокола
 *
 */
string_view awh::http::Parser_HTTP3::errorName(const error_t error) noexcept {
	// Выводим название кода ошибки протокола
	return h3::errorName(error);
}
/**
 * @brief Метод получения кода последней ошибки протокола
 *
 * @return код последней ошибки протокола
 *
 */
awh::http::Parser_HTTP3::error_t awh::http::Parser_HTTP3::error() const noexcept {
	// Выводим код последней ошибки протокола
	return this->_error;
}
/**
 * @brief Метод обработки обрыва потока пиром
 *
 * @param sid  идентификатор потока
 * @param code код ошибки, с которым поток оборван
 *
 */
void awh::http::Parser_HTTP3::aborted(const uint64_t sid, const uint64_t code) noexcept {
	/**
	 * Обрыв управляющего потока либо потока QPACK лишает соединение управления
	 * (RFC 9114 §6.2.1, RFC 9204 §4.2)
	 */
	if((sid == this->_controlRemote) || (sid == this->_encoderRemote) || (sid == this->_decoderRemote)){
		// Фиксируем ошибку уровня соединения
		this->fail(error_t::H3_CLOSED_CRITICAL_STREAM, "оборван поток, обязанный жить всё соединение");
		// Выходим из метода
		return;
	}
	// Удаляем состояние однонаправленного потока
	this->_unistreams.erase(sid);
	// Закрываем поток с кодом, полученным от транспорта
	this->closeStream(sid, static_cast <error_t> (code));
}

/**
 * @brief Метод проверки того, что расширенный CONNECT разрешён нами
 *
 * @details Разрешение выдаётся параметром SETTINGS_ENABLE_CONNECT_PROTOCOL либо
 *          подразумевается ролью узла: соединение, объявленное несущим WebSocket,
 *          иначе отвергало бы единственный запрос, ради которого заведено
 *          (RFC 9220 §3)
 *
 * @return признак разрешения расширенного CONNECT
 *
 */
bool awh::http::Parser_HTTP3::connectProtocol() const noexcept {
	// Выводим признак разрешения расширенного CONNECT
	return (this->_settings.enableConnectProtocol || (this->_proto == proto_t::WEBSOCKET3));
}
/**
 * @brief Метод получения протокола, с которым работает парсер
 *
 * @return протокол работы парсера
 *
 */
awh::http::proto_t awh::http::Parser_HTTP3::proto() const noexcept {
	// Выводим протокол работы парсера
	return this->_proto;
}
/**
 * @brief Метод установки протокола, с которым работает парсер
 *
 * @param proto протокол работы парсера
 *
 */
void awh::http::Parser_HTTP3::proto(const proto_t proto) noexcept {
	/**
	 * Определяем принадлежность указанного протокола к семейству HTTP/3
	 */
	switch(static_cast <uint8_t> (proto)){
		// Прямое соединение с узлом, работа промежуточным узлом либо туннель WebSocket
		case static_cast <uint8_t> (proto_t::HTTP3):
		case static_cast <uint8_t> (proto_t::PROXY3):
		case static_cast <uint8_t> (proto_t::WEBSOCKET3):
			// Устанавливаем протокол работы парсера
			this->_proto = proto;
		break;
		/**
		 * Протокол другого семейства этот парсер разобрать не может, и принять такое
		 * указание молча означало бы оставить вызывающую сторону в уверенности, что
		 * оно учтено
		 */
		default: this->_log->print(
			"HTTP/3 parser speaks HTTP/3 only: the protocol has not been changed",
			log_t::flag_t::CRITICAL
		);
	}
}
/**
 * @brief Метод получения лимитов безопасности парсера
 *
 * @return лимиты безопасности парсера
 *
 */
const awh::http::Parser_HTTP3::limits_t & awh::http::Parser_HTTP3::limits() const noexcept {
	// Выводим лимиты безопасности парсера
	return this->_limits;
}
/**
 * @brief Метод установки лимитов безопасности парсера
 *
 * @param limits лимиты безопасности парсера
 *
 */
void awh::http::Parser_HTTP3::limits(const limits_t & limits) noexcept {
	// Устанавливаем лимиты безопасности парсера
	this->_limits = limits;
	// Переинициализируем лимит частоты управляющих кадров
	this->_ctrlLimit.init(this->_limits.ctrlLimitBurst, this->_limits.ctrlLimitRate);
	// Переинициализируем лимит частоты кадров приоритета
	this->_priorityLimit.init(this->_limits.prioLimitBurst, this->_limits.prioLimitRate);
}
/**
 * @brief Метод получения наших параметров SETTINGS
 *
 * @return наши параметры SETTINGS
 *
 */
const awh::http::Parser_HTTP3::settings_t & awh::http::Parser_HTTP3::settings() const noexcept {
	// Выводим наши параметры SETTINGS
	return this->_settings;
}
/**
 * @brief Метод установки наших параметров SETTINGS
 *
 * @param settings наши параметры SETTINGS
 *
 */
void awh::http::Parser_HTTP3::settings(const settings_t & settings) noexcept {
	/**
	 * Кадр SETTINGS в соединении единственный, поэтому менять параметры после
	 * его отправки поздно: пир согласовал уже отправленные (RFC 9114 §7.2.4)
	 */
	if(this->_settingsSent)
		// Выходим из метода
		return;
	// Устанавливаем наши параметры SETTINGS
	this->_settings = settings;
	/**
	 * Разрешение расширенного CONNECT выдаёт только сервер: анонс клиента
	 * не имел бы адресата (RFC 9220 §3)
	 */
	if(this->_endpoint == h3::endpoint_t::CLIENT)
		// Снимаем разрешение расширенного CONNECT
		this->_settings.enableConnectProtocol = false;
	// Устанавливаем ёмкость таблицы декодера по нашему анонсу
	this->_decoder.maxCapacity(this->_settings.qpackMaxTableCapacity);
	// Устанавливаем число ожидающих потоков декодера по нашему анонсу
	this->_decoder.maxBlocked(this->_settings.qpackBlockedStreams);
}
/**
 * @brief Метод получения параметров SETTINGS пира
 *
 * @return параметры SETTINGS пира
 *
 */
const awh::http::Parser_HTTP3::settings_t & awh::http::Parser_HTTP3::remoteSettings() const noexcept {
	// Выводим параметры SETTINGS пира
	return this->_remote;
}
/**
 * @brief Метод проверки получения SETTINGS от пира
 *
 * @return признак получения SETTINGS от пира
 *
 */
bool awh::http::Parser_HTTP3::isSettingsReceived() const noexcept {
	// Выводим признак получения параметров пира
	return this->_settingsReceived;
}
/**
 * @brief Метод проверки завершённости соединения
 *
 * @return признак завершённости соединения
 *
 */
bool awh::http::Parser_HTTP3::isClosed() const noexcept {
	// Выводим признак завершённости соединения
	return this->_closed;
}
/**
 * @brief Метод обновления момента времени для частотных лимитов
 *
 * @param seconds текущий момент времени в секундах
 *
 */
void awh::http::Parser_HTTP3::updateTime(const uint64_t seconds) noexcept {
	// Обновляем момент времени лимита управляющих кадров
	this->_ctrlLimit.update(seconds);
	// Обновляем момент времени лимита кадров приоритета
	this->_priorityLimit.update(seconds);
}
/**
 * @brief Метод отправки параметров соединения
 *
 */
void awh::http::Parser_HTTP3::sendSettings() noexcept {
	// Если параметры уже отправлены либо соединение завершено
	if(this->_settingsSent || this->_closed)
		// Выходим из метода
		return;
	// Если служебные потоки открыть не удалось
	if(!this->prepare())
		// Выходим из метода
		return;
	// Собираемый набор параметров
	vector <h3::frame::setting_entry_t> items;
	// Собираемый параметр
	h3::frame::setting_entry_t item;
	// Устанавливаем идентификатор параметра размера таблицы QPACK
	item.id = static_cast <uint64_t> (h3::setting_t::QPACK_MAX_TABLE_CAPACITY);
	// Устанавливаем значение параметра размера таблицы QPACK
	item.value = this->_settings.qpackMaxTableCapacity;
	// Дописываем параметр в набор
	items.push_back(item);
	// Устанавливаем идентификатор параметра числа ожидающих потоков
	item.id = static_cast <uint64_t> (h3::setting_t::QPACK_BLOCKED_STREAMS);
	// Устанавливаем значение параметра числа ожидающих потоков
	item.value = this->_settings.qpackBlockedStreams;
	// Дописываем параметр в набор
	items.push_back(item);
	// Если максимальный размер секции полей анонсируется
	if(this->_settings.maxFieldSectionSize > 0){
		// Устанавливаем идентификатор параметра размера секции полей
		item.id = static_cast <uint64_t> (h3::setting_t::MAX_FIELD_SECTION_SIZE);
		/**
		 * Анонсировать больше, чем соблюдаем, нельзя: секцию сверх лимита
		 * распакованного списка мы всё равно оборвём кодом H3_EXCESSIVE_LOAD,
		 * и пир получил бы сброс за размер, который сами же ему и разрешили.
		 * Отсечка стоит на отправке, а не в settings(): лимиты безопасности
		 * задаются отдельным методом, и порядок вызовов произволен. Обратное
		 * соотношение допустимо - анонс строже соблюдаемого лишь бережёт пира
		 */
		item.value = this->_settings.maxFieldSectionSize;
		/**
		 * Нулевой лимит распакованного списка означает его отсутствие, а не запрет
		 * полей: sentinel-значения для этого в самом протоколе нет, и урезание анонса
		 * по нему объявило бы пиру предельный размер секции в ноль октет
		 */
		if(this->_limits.maxHeadersTotal > 0)
			// Урезаем анонс до соблюдаемого лимита распакованного списка
			item.value = ::std::min(item.value, static_cast <uint64_t> (this->_limits.maxHeadersTotal));
		// Если анонс пришлось урезать до соблюдаемого лимита
		if(item.value != this->_settings.maxFieldSectionSize)
			// Записываем сообщение об урезании анонса в лог
			this->_log->print(
				"HTTP/3 announced SETTINGS_MAX_FIELD_SECTION_SIZE is capped to enforced header list limit of %llu octets",
				log_t::flag_t::WARNING, static_cast <unsigned long long> (item.value)
			);
		// Дописываем параметр в набор
		items.push_back(item);
	}
	/**
	 * Расширенный CONNECT анонсирует сервер: разрешение выдаётся параметром либо
	 * подразумевается ролью узла. Соединение, объявленное несущим WebSocket, без
	 * этого анонса бессмысленно: клиент не вправе слать расширенный CONNECT,
	 * не получив разрешения, а мы обязаны такой запрос отвергать
	 */
	if((this->_endpoint == h3::endpoint_t::SERVER) && this->connectProtocol()){
		// Устанавливаем идентификатор параметра расширенного CONNECT
		item.id = static_cast <uint64_t> (h3::setting_t::ENABLE_CONNECT_PROTOCOL);
		// Устанавливаем значение параметра расширенного CONNECT
		item.value = 1;
		// Дописываем параметр в набор
		items.push_back(item);
	}
	/**
	 * Дописываем зарезервированный параметр: он обязан игнорироваться пиром,
	 * и его отправка проверяет, что пир не считает набор параметров закрытым
	 * (RFC 9114 §7.2.4.1)
	 */
	item.id = (h3::proto::GREASE_BASE + h3::proto::GREASE_STEP);
	// Устанавливаем произвольное значение зарезервированного параметра
	item.value = 0;
	// Дописываем параметр в набор
	items.push_back(item);
	// Выполняем очистку буфера сборки кадра
	this->_frame.clear();
	// Собираем кадр параметров соединения
	h3::frame::serialize::settings(this->_frame, items.data(), items.size());
	// Отправляем кадр параметров в управляющий поток
	this->emit(this->_controlLocal, this->_frame.data(), this->_frame.size(), false);
	// Запоминаем отправку наших параметров
	this->_settingsSent = true;
}
/**
 * @brief Метод отправки секции полей потока
 *
 * @param sid       идентификатор потока
 * @param fields    поля секции
 * @param endStream признак завершения потока
 *
 */
void awh::http::Parser_HTTP3::sendHeaders(const uint64_t sid, const vector <h3::qpack::field_t> & fields, const bool endStream) noexcept {
	// Если соединение завершено
	if(this->_closed)
		// Выходим из метода
		return;
	/**
	 * Ответы 204 и 304 завершаются концом секции полей и не несут ни содержимого,
	 * ни трейлеров (RFC 9110 §15.3.5, §15.4.5). Признак ставится в момент отправки
	 * самой такой секции, поэтому любая следующая по этому потоку - уже трейлеры
	 */
	const stream_t * target = this->findStream(sid);
	/**
	 * После отправленного FIN наше направление потока закрыто, и слать в него нечего
	 * (RFC 9114 §4.1). Признак отложенного завершения здесь не проверяется: он означает
	 * ровно то, что тело дочитано до конца, а это и есть момент отправки секции
	 * трейлеров - завершение потока переезжает с последнего кадра тела на них
	 */
	if((target != nullptr) && target->localFin){
		// Записываем сообщение об отказе в лог
		this->_log->print(
			"HTTP/3 stream %llu is already finished in the local direction",
			log_t::flag_t::WARNING, static_cast <unsigned long long> (sid)
		);
		// Выходим из метода
		return;
	}
	// Если по потоку уже отправлен ответ, трейлеров не допускающий
	if((target != nullptr) && target->trailerlessSend){
		// Записываем сообщение об отказе в лог
		this->_log->print(
			"HTTP/3 response on stream %llu cannot carry trailers",
			log_t::flag_t::WARNING, static_cast <unsigned long long> (sid)
		);
		// Выходим из метода
		return;
	}
	/**
	 * Секция полей, идущая по потоку следом за уже отправленной, - это трейлеры,
	 * и уйти они обязаны строго после тела, которое завершают. Пока в буфере
	 * отправки остаётся тело либо не исчерпан источник, секция откладывается
	 */
	if((target != nullptr) && target->headersSent && this->deferTrailers(sid, fields, endStream))
		// Выходим из метода
		return;
	/**
	 * После GOAWAY сервера клиент не открывает новых потоков запроса (RFC 9114 §5.2):
	 * сервер объявил, что не станет их обрабатывать. Проверяется именно открытие -
	 * секции уже открытого потока, включая трейлеры, отправляются штатно. Сравнение
	 * с идентификатором из GOAWAY имеет смысл только на клиенте: от клиента тот же
	 * кадр несёт идентификатор обещания push, а не потока
	 */
	if((this->_endpoint == h3::endpoint_t::CLIENT) && (sid >= this->_goawayRemote) && (this->findStream(sid) == nullptr)){
		// Записываем сообщение об отказе в лог
		this->_log->print(
			"HTTP/3 peer sent GOAWAY, request for stream %llu is not sent",
			log_t::flag_t::WARNING, static_cast <unsigned long long> (sid)
		);
		// Выходим из метода
		return;
	}
	// Если служебные потоки открыть не удалось
	if(!this->prepare())
		// Выходим из метода
		return;
	// Если параметры соединения ещё не отправлены
	if(!this->_settingsSent)
		// Отправляем параметры соединения
		this->sendSettings();
	// Выполняем очистку буфера сборки секции полей
	this->_section.clear();
	// Выполняем кодирование секции полей
	this->_encoder.encode(sid, fields, this->_section);
	/**
	 * Секция сверх анонсированного пиром лимита не отправляется: пир отверг бы
	 * её целиком, а поток остался бы без сообщения (RFC 9114 §7.2.4.1)
	 */
	if((this->_remote.maxFieldSectionSize > 0) && (this->_encoder.listSize() > this->_remote.maxFieldSectionSize)){
		// Выгружаем накопленные инструкции кодека
		this->flushQpack();
		// Откатываем ровно ту секцию, которая не ушла в сеть
		this->_encoder.rollback(sid);
		// Записываем сообщение об отказе в лог
		this->_log->print(
			"HTTP/3 field section for stream %llu exceeds peer SETTINGS_MAX_FIELD_SECTION_SIZE, not sent",
			log_t::flag_t::WARNING, static_cast <unsigned long long> (sid)
		);
		// Выходим из метода
		return;
	}
	// Получаем состояние потока
	stream_t & stream = this->stream(sid);
	// Помечаем что секция полей по этому потоку нами отправлена
	stream.headersSent = true;
	/**
	 * Ответ на запрос HEAD тела не несёт, поэтому объявленная в нём длина тела
	 * к принятому телу не применяется (RFC 9110 §9.3.2), а отправляемые нами
	 * ответы 204 и 304 не несут ни содержимого, ни трейлеров (§15.3.5, §15.4.5).
	 * Признаки выставляются до выхода наружу: emit() отдаёт байты обвязке
	 * синхронно, и та вправе прямо оттуда продолжить сообщение - тело либо
	 * трейлеры ушли бы на провод раньше, чем запрет на них начал действовать
	 */
	for(const auto & field : fields){
		// Если запрос отправляется методом HEAD
		if((field.name == header::METHOD) && (field.value == value::HEAD)){
			// Запоминаем безтелесность ожидаемого ответа
			stream.headless = true;
			// Прекращаем перебор полей секции
			break;
		// Если отправляется ответ, не несущий ни тела, ни трейлеров
		} else if((field.name == header::STATUS) && ((field.value == value::NO_CONTENT) || (field.value == value::NOT_MODIFIED))) {
			// Запоминаем что отправляемое сообщение тела нести не может
			stream.headlessSend = true;
			// Запоминаем что отправляемое сообщение трейлеров нести не может
			stream.trailerlessSend = true;
			// Прекращаем перебор полей секции
			break;
		}
	}
	// Выгружаем накопленные инструкции кодека
	this->flushQpack();
	// Выполняем очистку буфера сборки кадра
	this->_frame.clear();
	// Собираем кадр секции полей
	h3::frame::serialize::headers(this->_frame, this->_section);
	// Отправляем кадр секции полей
	this->emit(sid, this->_frame.data(), this->_frame.size(), endStream);
	// Если поток завершается вместе с секцией полей
	if(endStream){
		// Запоминаем завершение потока в нашем направлении
		stream.localFin = true;
		// Закрываем поток, если оба направления завершены
		this->maybeClose(sid);
	}
}
/**
 * @brief Метод отправки секции полей потока из провайдера
 *
 * @param sid       идентификатор потока
 * @param headers   набор заголовков сообщения
 * @param endStream признак завершения потока
 * @param scheme    схема запроса для псевдо-заголовка [:scheme]
 *
 */
void awh::http::Parser_HTTP3::sendHeaders(const uint64_t sid, const headers_t & headers, const bool endStream, string_view scheme) noexcept {
	// Если соединение завершено
	if(this->_closed)
		// Выходим из метода
		return;
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		// Собираемый список полей секции
		vector <h3::qpack::field_t> fields;
		// Выделяем место под поля секции
		fields.reserve(headers.size() + 5);
		// Получаем объект провайдера контейнера заголовков
		const provider_t * provider = headers.provider();
		// Если провайдер контейнера установлен - формируем псевдо-заголовки (RFC 9114 §4.3)
		if(provider != nullptr){
			// Если провайдер является запросом клиента
			if(provider->direct == direct_t::REQUEST){
				// Получаем объект провайдера запроса клиента
				const request_t * request = static_cast <const request_t *> (provider);
				// Признак поднятия туннеля расширенным методом CONNECT
				const bool extended = (!request->protocol.empty() && (request->method == method_t::CONNECT));
				/**
				 * Расширенный CONNECT допустим, только если пир анонсировал его
				 * параметром SETTINGS_ENABLE_CONNECT_PROTOCOL (RFC 9220 §3)
				 */
				if(extended && !this->_remote.enableConnectProtocol){
					// Записываем сообщение об отказе в лог
					this->_log->print(
						"HTTP/3 peer does not support extended CONNECT (RFC 9220), request for stream %llu is not sent",
						log_t::flag_t::WARNING, static_cast <unsigned long long> (sid)
					);
					// Выходим из метода
					return;
				}
				/**
				 * Схему цели запроса WebSocket задаёт RFC 8441 §5: [https] для адресов
				 * [wss] и [http] для адресов [ws]. Отправить [ws] либо [wss] схемой цели
				 * значило бы собрать у принимающей стороны не тот URI, который имелся
				 * в виду, поэтому запрос не отправляется целиком: подменять схему за
				 * приложение парсер не вправе - оно адресовало запрос осознанно
				 */
				if((this->_proto == proto_t::WEBSOCKET3) && extended &&
				   this->_fmk->compare("websocket", request->protocol) && !::httpScheme(scheme)){
					// Записываем сообщение об отказе в лог
					this->_log->print(
						"HTTP/3 WebSocket target URI requires http or https scheme (RFC 8441), request for stream %llu is not sent",
						log_t::flag_t::WARNING, static_cast <unsigned long long> (sid)
					);
					// Выходим из метода
					return;
				}
				// Дописываем псевдо-заголовок метода запроса
				fields.emplace_back(string(header::METHOD), string(awh::http::methodName(request)));
				// Если псевдо-заголовки схемы и пути допустимы для этого запроса
				if((request->method != method_t::CONNECT) || extended)
					// Дописываем псевдо-заголовок схемы запроса
					fields.emplace_back(string(header::SCHEME), string(scheme));
				// Если контейнер содержит заголовок адресата HTTP/1.1
				if(headers.has("host"))
					// Дописываем псевдо-заголовок адресата
					fields.emplace_back(string(header::AUTHORITY), headers.at("host"));
				/**
				 * Для метода CONNECT псевдо-заголовок адресата обязателен, а заголовка
				 * Host в таком запросе может не быть - берём цель из URI (RFC 9114 §4.4)
				 */
				else if((request->method == method_t::CONNECT) && !request->uri.empty())
					// Дописываем псевдо-заголовок адресата
					fields.emplace_back(string(header::AUTHORITY), request->uri);
				// Если псевдо-заголовки схемы и пути допустимы для этого запроса
				if((request->method != method_t::CONNECT) || extended)
					// Дописываем псевдо-заголовок пути запроса
					fields.emplace_back(string(header::PATH), (request->uri.empty() ? string("/") : request->uri));
				// Если запрос поднимает туннель расширенным методом CONNECT
				if(extended)
					// Дописываем псевдо-заголовок протокола туннеля
					fields.emplace_back(string(header::PROTOCOL), request->protocol);
			// Если провайдер является ответом сервера
			} else {
				// Получаем объект провайдера ответа сервера
				const response_t * response = static_cast <const response_t *> (provider);
				// Собираемое значение статус-кода ответа сервера
				char code[4] = {0};
				// Формируем значение статус-кода ответа сервера
				code[0] = static_cast <char> ('0' + ((response->code / 100) % 10));
				// Формируем вторую цифру статус-кода ответа сервера
				code[1] = static_cast <char> ('0' + ((response->code / 10) % 10));
				// Формируем третью цифру статус-кода ответа сервера
				code[2] = static_cast <char> ('0' + (response->code % 10));
				// Дописываем псевдо-заголовок статуса ответа
				fields.emplace_back(string(header::STATUS), string(code, 3));
			}
		}
		// Переиспользуемый буфер названия поля в нижнем регистре
		string buffer;
		/**
		 * Выполняем перебор всех заголовков контейнера
		 */
		for(const headers_t::header_t & item : headers){
			// Выполняем очистку буфера названия поля
			buffer.clear();
			// Выделяем место под название поля
			buffer.reserve(item.name.size());
			/**
			 * Названия полей в HTTP/3 обязаны быть в нижнем регистре (RFC 9114 §4.2)
			 */
			for(const char letter : item.name)
				// Дописываем символ названия поля в нижнем регистре
				buffer.push_back(static_cast <char> (((letter >= 'A') && (letter <= 'Z')) ? (letter + 32) : letter));
			/**
			 * Поля управления соединением в HTTP/3 запрещены, а заголовок адресата
			 * уже перенесён в псевдо-заголовок (RFC 9114 §4.2)
			 */
			if(::isForbidden(buffer) || (buffer == header::HOST))
				// Переходим к следующему заголовку
				continue;
			// Единственное допустимое значение поля [te] - trailers
			if((buffer == header::TE) && (item.value != value::TRAILERS))
				// Переходим к следующему заголовку
				continue;
			// Дописываем поле в список секции
			fields.emplace_back(buffer, item.value);
		}
		// Отправляем собранную секцию полей
		this->sendHeaders(sid, fields, endStream);
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception &) {
		// Секцию полей собрать не удалось
	}
}
/**
 * @brief Метод отправки данных тела потока
 *
 * @param sid       идентификатор потока
 * @param buffer    буфер данных тела
 * @param size      размер данных тела
 * @param endStream признак завершения потока
 * @return          количество принятых к отправке байт
 *
 */
size_t awh::http::Parser_HTTP3::sendData(const uint64_t sid, const void * buffer, const size_t size, const bool endStream) noexcept {
	// Результат работы функции - число принятых байт
	size_t result = 0;
	// Если соединение завершено
	if(this->_closed)
		// Выводим число принятых байт
		return result;
	// Выполняем поиск состояния потока
	stream_t * stream = this->findStream(sid);
	// Если поток закрыт - данные не принимаются
	if(stream == nullptr)
		// Выводим число принятых байт
		return result;
	// Нельзя слать тело после уже поставленного завершения потока
	if(stream->endStreamPending || stream->localFin)
		// Выводим число принятых байт
		return result;
	// Если секция трейлеров уже отложена - тело после неё не принимается
	if(stream->trailersPending)
		// Выводим число принятых байт
		return result;
	/**
	 * Тело потока, которому назначен pull-источник, приложение не досылает само:
	 * два писателя в один буфер перемешали бы тело
	 */
	if(stream->source != nullptr){
		// Записываем сообщение об ошибке в лог
		this->_log->print(
			"HTTP/3 stream %llu is fed by a data source, direct body is not accepted",
			log_t::flag_t::WARNING, static_cast <unsigned long long> (sid)
		);
		// Выводим число принятых байт
		return result;
	}
	/**
	 * Ответ на запрос методом HEAD содержимого не несёт (RFC 9110 §9.3.2): тело
	 * принимается и отбрасывается, а не отвергается. Отказ приёмом нуля байт
	 * приложение прочло бы как заполненный буфер и ждало бы сигнала готовности,
	 * которого при пустом буфере не будет
	 */
	if(stream->headlessSend){
		// Признаём принятым весь фрагмент, не отправляя из него ничего
		result = size;
		// Если фрагмент финальный - поток всё равно обязан завершиться
		if(endStream)
			// Помечаем что поток обязан завершиться на последнем фрагменте
			stream->endStreamPending = true;
		// Выполняем выдачу накопленного тела потока
		this->pumpStream(sid);
		// Выводим число принятых байт
		return result;
	}
	// Вычисляем свободное место в буфере отправки до верхней метки
	const size_t room = ((stream->pending() < this->_sendHighWater) ? (this->_sendHighWater - stream->pending()) : 0);
	// Принимаем столько байт, сколько влезает (частичный приём)
	result = ::std::min(size, room);
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		// Если есть что принимать - дописываем данные в буфер отправки потока
		if((result > 0) && (buffer != nullptr))
			// Дописываем данные в буфер отправки потока
			stream->sendBuffer.append(static_cast <const char *> (buffer), result);
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception &) {
		// Данные принять не удалось
		result = 0;
	}
	// Завершение потока помечаем только когда принят весь финальный фрагмент
	if(endStream && (result == size))
		// Помечаем что поток обязан завершиться на последнем фрагменте
		stream->endStreamPending = true;
	// Если буфер отправки поднялся выше нижней метки - взводим сигнал готовности снова
	if(stream->pending() > this->_sendLowWater)
		// Взводим сигнал готовности для следующего опустошения буфера
		stream->writableNotified = false;
	// Выполняем выдачу накопленного тела потока
	this->pumpStream(sid);
	// Выводим число принятых байт
	return result;
}
/**
 * @brief Метод назначения pull-источника данных тела потока
 *
 * @param sid    идентификатор потока
 * @param source pull-источник данных тела
 *
 */
void awh::http::Parser_HTTP3::dataSource(const uint64_t sid, data_source_callback_t source) noexcept {
	// Выполняем поиск состояния потока
	stream_t * stream = this->findStream(sid);
	// Если поток закрыт - назначать источник некуда
	if(stream == nullptr)
		// Выходим из метода
		return;
	// Запоминаем источник данных тела потока
	stream->source = ::move(source);
	// Снимаем признак достижения конца тела: источник назначен заново
	stream->sourceEof = false;
	// Выполняем выдачу накопленного тела потока
	this->pumpStream(sid);
}
/**
 * @brief Метод возобновления выдачи тела потока
 *
 * @param sid идентификатор потока
 *
 */
void awh::http::Parser_HTTP3::resume(const uint64_t sid) noexcept {
	// Если соединение завершено - возобновлять нечего
	if(this->_closed)
		// Выходим из метода
		return;
	// Выполняем выдачу накопленного тела потока
	this->pumpStream(sid);
}
/**
 * @brief Метод настройки порогов буфера отправки потока
 *
 * @param high ёмкость буфера отправки потока (high-water)
 * @param low  порог сигнала готовности (low-water)
 *
 */
void awh::http::Parser_HTTP3::sendWaterMarks(const size_t high, const size_t low) noexcept {
	// Ёмкость буфера отправки не может быть нулевой: приём тела встал бы совсем
	this->_sendHighWater = ((high > 0) ? high : SEND_HIGH_WATER);
	// Порог сигнала готовности не может превышать ёмкость буфера
	this->_sendLowWater = ::std::min(low, this->_sendHighWater);
}
/**
 * @brief Метод настройки порога накопленных исходящих данных потока
 *
 * @param high порог накопленных исходящих данных потока
 *
 */
void awh::http::Parser_HTTP3::outputHighWater(const size_t high) noexcept {
	// Порог накопленных исходящих данных не может быть нулевым: выдача встала бы совсем
	this->_outputHighWater = ((high > 0) ? high : OUTPUT_HIGH_WATER);
}
/**
 * @brief Метод откладывания секции трейлеров до конца отправки тела
 *
 * @details Секция трейлеров уходит строго после тела, которое завершает. Пока
 *          в буфере отправки остаётся тело либо не исчерпан источник, секция
 *          запоминается и выдаётся выдачей тела
 *
 * @param sid       идентификатор потока
 * @param fields    поля секции трейлеров
 * @param endStream признак завершения потока
 * @return          признак откладывания секции
 *
 */
bool awh::http::Parser_HTTP3::deferTrailers(const uint64_t sid, const vector <h3::qpack::field_t> & fields, const bool endStream) noexcept {
	// Выполняем поиск состояния потока
	stream_t * stream = this->findStream(sid);
	// Если поток закрыт либо его секция полей ещё не отправлена - откладывать нечего
	if(stream == nullptr)
		// Откладывание не требуется
		return false;
	// Если тело потока отправлено полностью - трейлеры уходят сразу за ним
	if((stream->pending() == 0) && this->sourceDone(* stream) && !stream->endStreamPending)
		// Откладывание не требуется
		return false;
	// Повторная секция трейлеров недопустима
	if(stream->trailersPending){
		// Записываем сообщение об ошибке в лог
		this->_log->print(
			"HTTP/3 trailers for stream %llu are already pending",
			log_t::flag_t::WARNING, static_cast <unsigned long long> (sid)
		);
		// Отправка отложена (повторную секцию отбрасываем)
		return true;
	}
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		// Запоминаем поля отложенной секции трейлеров
		stream->sendTrailers = fields;
		// Помечаем что секция трейлеров отложена
		stream->trailersPending = true;
		// Помечаем что поток обязан завершиться вместе с секцией
		stream->endStreamPending = true;
		// Если трейлеры не завершают поток - это нарушение порядка частей сообщения
		if(!endStream)
			// Записываем сообщение об ошибке в лог
			this->_log->print(
				"HTTP/3 trailers for stream %llu must finish the stream",
				log_t::flag_t::WARNING, static_cast <unsigned long long> (sid)
			);
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception &) {
		// Секцию трейлеров запомнить не удалось
		stream->trailersPending = false;
	}
	// Выполняем выдачу накопленного тела потока
	this->pumpStream(sid);
	// Отправка отложена
	return true;
}
/**
 * @brief Метод отправки отложенной секции трейлеров потока
 *
 * @param sid идентификатор потока
 *
 */
void awh::http::Parser_HTTP3::flushTrailers(const uint64_t sid) noexcept {
	// Выполняем поиск состояния потока
	stream_t * stream = this->findStream(sid);
	// Если поток закрыт либо секция трейлеров не откладывалась
	if((stream == nullptr) || !stream->trailersPending)
		// Выходим из метода
		return;
	// Забираем поля отложенной секции трейлеров
	const vector <h3::qpack::field_t> fields = ::move(stream->sendTrailers);
	// Снимаем признак отложенной секции трейлеров
	stream->trailersPending = false;
	// Выполняем очистку полей отложенной секции
	stream->sendTrailers.clear();
	// Отправляем секцию трейлеров, завершая ею поток
	this->sendHeaders(sid, fields, true);
}
/**
 * @brief Метод получения объёма накопленных исходящих данных потока
 *
 * @param sid идентификатор потока
 * @return    объём ещё не выданных наружу октетов
 *
 */
size_t awh::http::Parser_HTTP3::outputPending(const uint64_t sid) const noexcept {
	// Выполняем поиск буфера исходящих данных потока
	auto i = this->_pending.find(sid);
	// Если буфер исходящих данных не найден - копить нечего
	if(i == this->_pending.end())
		// Выводим нулевой объём
		return 0;
	// Выводим объём ещё не выданных наружу октетов
	return (i->second.buffer.size() - i->second.consumed);
}
/**
 * @brief Метод проверки того, что всё тело потока для отправки получено
 *
 * @param stream объект потока
 * @return       результат проверки (источника нет либо достигнут его конец)
 *
 */
bool awh::http::Parser_HTTP3::sourceDone(const stream_t & stream) const noexcept {
	// Выводим признак отсутствия источника либо достижения его конца
	return ((stream.source == nullptr) || stream.sourceEof);
}
/**
 * @brief Метод вызова функции обратного вызова готовности потока
 *
 * @param sid  идентификатор потока
 * @param room свободное место в буфере отправки потока
 *
 */
void awh::http::Parser_HTTP3::fireWritable(const uint64_t sid, const size_t room) noexcept {
	// Если функция обратного вызова готовности не установлена - сообщать некому
	if(this->_callbacks.writable == nullptr)
		// Выходим из метода
		return;
	/**
	 * Выполняем отлов ошибок
	 */
	try {
		// Уведомляем о готовности потока принимать данные тела
		this->_callbacks.writable(sid, room);
	/**
	 * Если возникает ошибка
	 */
	} catch(const exception &) {
		// Исключение из пользовательской функции обратного вызова гасим на месте
	}
}
/**
 * @brief Метод сигнализации о готовности потока принимать данные
 *
 * @param sid    идентификатор потока
 * @param stream объект потока
 *
 */
void awh::http::Parser_HTTP3::maybeNotifyWritable(const uint64_t sid, stream_t & stream) noexcept {
	/**
	 * Сигнал предназначен push-модели: при назначенном источнике парсер опрашивает
	 * его сам, и приложению нечего досылать по сигналу
	 */
	if((stream.source != nullptr) || (this->_callbacks.writable == nullptr))
		// Выходим из метода
		return;
	// Если сигнал ещё не подан и буфер отправки опустился ниже нижней метки
	if(!stream.writableNotified && (stream.pending() <= this->_sendLowWater)){
		// Помечаем что сигнал для текущего заполнения буфера подан
		stream.writableNotified = true;
		// Уведомляем о готовности потока принимать данные тела
		this->fireWritable(sid, (this->_sendHighWater - stream.pending()));
	}
}
/**
 * @brief Метод дозагрузки буфера отправки потока из pull-источника
 *
 * @param sid    идентификатор потока
 * @param stream объект потока (ссылка может стать недействительной)
 *
 */
void awh::http::Parser_HTTP3::refillFromSource(const uint64_t sid, stream_t & stream) noexcept {
	// Если источник данных не задан либо его тело уже закончилось - дозагружать нечего
	if((stream.source == nullptr) || stream.sourceEof)
		// Выходим из метода
		return;
	/**
	 * Ответ на запрос методом HEAD содержимого не несёт (RFC 9110 §9.3.2), поэтому
	 * источник данных не опрашивается вовсе: иначе HEAD по большому ресурсу заставил
	 * бы приложение вычитать его целиком - ради того, чтобы всё вычитанное отбросить
	 */
	if(stream.headlessSend){
		// Помечаем что конец тела источника достигнут
		stream.sourceEof = true;
		// Помечаем что поток обязан завершиться на последнем фрагменте
		stream.endStreamPending = true;
		// Выходим из метода
		return;
	}
	// Запоминаем идентификатор потока
	const uint64_t id = sid;
	// Указатель на объект потока (источник данных вправе закрыть поток)
	stream_t * sp = &stream;
	/**
	 * Держим буфер наполненным до верхней метки, запрашивая источник порциями
	 */
	while((sp->pending() < this->_sendHighWater) && !sp->sourceEof){
		// Вычисляем ёмкость запрашиваемой порции
		const size_t cap = ::std::min(SOURCE_CHUNK, (this->_sendHighWater - sp->pending()));
		// Запоминаем текущий размер буфера отправки
		const size_t offset = sp->sendBuffer.size();
		// Резервируем место под порцию данных прямо в буфере отправки (без промежуточной копии)
		sp->sendBuffer.resize(offset + cap);
		// Флаг достижения конца тела
		bool eof = false;
		// Результат запроса данных у источника
		int64_t bytes = -1;
		/**
		 * Забираем источник данных на время вызова: приложение вправе сбросить поток
		 * прямо из источника, а уничтожение вызываемого объекта под собственным вызовом
		 * недопустимо
		 */
		data_source_callback_t source = ::move(sp->source);
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Запрашиваем порцию данных у источника (источник пишет напрямую в буфер отправки)
			bytes = source(id, reinterpret_cast <uint8_t *> (&sp->sendBuffer[offset]), cap, eof);
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception &) {
			// Исключение из пользовательского источника гасим на месте
			bytes = -1;
		}
		// Перечитываем указатель на поток (источник мог его закрыть либо сбросить парсер)
		sp = this->findStream(id);
		// Если поток удалён - буфер отправки уничтожен вместе с ним, дозагружать некуда
		if(sp == nullptr)
			// Выходим из метода
			return;
		// Возвращаем источник данных потоку, если из самого источника не назначен новый
		if(sp->source == nullptr)
			// Возвращаем источник данных обратно потоку
			sp->source = ::move(source);
		// Обрезаем буфер отправки до фактически записанного источником размера
		sp->sendBuffer.resize(offset + static_cast <size_t> (((bytes > 0) && (bytes <= static_cast <int64_t> (cap))) ? bytes : 0));
		// Если источник сообщил об ошибке данных либо нарушил контракт (записал больше ёмкости)
		if((bytes < 0) || (bytes > static_cast <int64_t> (cap))){
			// Обрываем поток с кодом внутренней ошибки
			this->sendReset(id, error_t::H3_INTERNAL_ERROR);
			// Выходим из метода
			return;
		}
		// Если достигнут конец тела источника
		if(eof){
			// Помечаем что конец тела источника достигнут
			sp->sourceEof = true;
			// Помечаем что поток обязан завершиться на последнем фрагменте
			sp->endStreamPending = true;
		}
		// Если источник временно без данных - прерываем дозагрузку
		if((bytes == 0) && !eof)
			// Прерываем дозагрузку до следующей прокачки
			break;
	}
}
/**
 * @brief Метод выдачи накопленного тела потока кадрами DATA
 *
 * @param sid идентификатор потока
 *
 */
void awh::http::Parser_HTTP3::pumpStream(const uint64_t sid) noexcept {
	/**
	 * Выполняем выдачу, пока у потока есть что отправить и есть куда: в push-модели
	 * место наружу неограниченно, в pull-модели его освобождает consumePending()
	 */
	for(;;){
		// Выполняем поиск состояния потока
		stream_t * stream = this->findStream(sid);
		// Если поток закрыт - выдавать некуда
		if(stream == nullptr)
			// Выходим из метода
			return;
		/**
		 * Наше направление потока уже завершено признаком FIN, либо завершено само
		 * соединение: слать больше нечего и некуда (RFC 9114 §4.1). Проверка стоит
		 * до обращения к источнику данных - тянуть тело в поток, который отправлять
		 * его уже не может, значит вычитать приложение впустую, а затем ещё и
		 * выпустить вычитанное на провод следом за FIN
		 */
		if(this->_closed || stream->localFin)
			// Выходим из метода
			return;
		// Дозагружаем буфер отправки из источника данных
		this->refillFromSource(sid, * stream);
		// Перечитываем состояние потока (источник вправе его закрыть)
		stream = this->findStream(sid);
		// Если поток закрыт источником
		if(stream == nullptr)
			// Выходим из метода
			return;
		/**
		 * Накопленное наружу не вычитано до порога: в pull-модели это и есть обратное
		 * давление - новые кадры тела не собираются, пока обвязка не заберёт прежние
		 */
		if((this->_callbacks.write == nullptr) && (this->outputPending(sid) >= this->_outputHighWater))
			// Выходим из метода
			return;
		// Получаем объём ещё не обёрнутого в кадры тела
		const size_t remaining = stream->pending();
		// Если тело кончилось
		if(remaining == 0){
			// Если поток обязан завершиться, а всё тело уже выдано
			if(stream->endStreamPending && this->sourceDone(* stream)){
				// Снимаем признак отложенного завершения потока
				stream->endStreamPending = false;
				// Если секция трейлеров отложена - поток завершает она
				if(stream->trailersPending)
					// Отправляем отложенную секцию трейлеров
					this->flushTrailers(sid);
				// Иначе завершаем поток признаком транспорта
				else {
					// Отправляем признак завершения потока
					this->emit(sid, nullptr, 0, true);
					// Выполняем поиск состояния потока
					stream = this->findStream(sid);
					// Если поток жив
					if(stream != nullptr){
						// Запоминаем завершение потока в нашем направлении
						stream->localFin = true;
						// Закрываем поток, если оба направления завершены
						this->maybeClose(sid);
					}
				}
			}
			// Выходим из метода
			return;
		}
		// Вычисляем размер выдаваемого фрагмента тела
		const size_t size = ::std::min(remaining, SOURCE_CHUNK);
		// Выполняем очистку буфера сборки кадра
		this->_frame.clear();
		// Собираем кадр данных тела из буфера отправки потока
		h3::frame::serialize::data(this->_frame, string_view(stream->sendBuffer.data() + stream->sendOffset, size));
		// Отмечаем обёрнутый префикс без сдвига всего буфера
		stream->sendOffset += size;
		// Выполняем амортизированное уплотнение буфера отправки
		stream->compactSendBuffer();
		// Отправляем кадр данных тела
		this->emit(sid, this->_frame.data(), this->_frame.size(), false);
		// Перечитываем состояние потока (выдача выходит в обвязку)
		stream = this->findStream(sid);
		// Если поток закрыт обвязкой
		if(stream == nullptr)
			// Выходим из метода
			return;
		// Сигнализируем о готовности потока принимать данные (если буфер просел)
		this->maybeNotifyWritable(sid, * stream);
	}
}
/**
 * @brief Метод анонса server push (только сервер)
 *
 * @param sid    идентификатор ассоциированного потока запроса
 * @param fields поля обещанного запроса
 * @return       идентификатор обещанного push либо UINT64_MAX при отказе
 *
 */
uint64_t awh::http::Parser_HTTP3::sendPushPromise(const uint64_t sid, const vector <h3::qpack::field_t> & fields) noexcept {
	// Если соединение завершено либо мы не сервер
	if(this->_closed || (this->_endpoint != h3::endpoint_t::SERVER))
		// Выводим признак отказа
		return UINT64_MAX;
	/**
	 * Выдавать push можно только в границах, разрешённых клиентом кадром
	 * MAX_PUSH_ID (RFC 9114 §7.2.7)
	 */
	if((this->_maxPushId == UINT64_MAX) || (this->_nextPushId > this->_maxPushId))
		// Выводим признак отказа
		return UINT64_MAX;
	/**
	 * После GOAWAY клиента сервер не обещает push с идентификатором не меньше
	 * объявленного (RFC 9114 §5.2). От клиента GOAWAY несёт именно идентификатор
	 * обещания push, поэтому сравнивается он, а не идентификатор потока
	 */
	if(this->_nextPushId >= this->_goawayRemote)
		// Выводим признак отказа
		return UINT64_MAX;
	// Если служебные потоки открыть не удалось
	if(!this->prepare())
		// Выводим признак отказа
		return UINT64_MAX;
	// Выполняем очистку буфера сборки секции полей
	this->_section.clear();
	// Выполняем кодирование секции полей обещанного запроса
	this->_encoder.encode(sid, fields, this->_section);
	/**
	 * Секция сверх анонсированного клиентом лимита не отправляется: он отверг бы
	 * её целиком, а идентификатор обещания оказался бы истрачен впустую
	 * (RFC 9114 §7.2.4.1). Откатывается ровно эта секция - прежние секции потока
	 * уже отправлены, и подтверждения на них придут
	 */
	if((this->_remote.maxFieldSectionSize > 0) && (this->_encoder.listSize() > this->_remote.maxFieldSectionSize)){
		// Выгружаем накопленные инструкции кодека
		this->flushQpack();
		// Откатываем ровно ту секцию, которая не ушла в сеть
		this->_encoder.rollback(sid);
		// Записываем сообщение об отказе в лог
		this->_log->print(
			"HTTP/3 push promise field section for stream %llu exceeds peer SETTINGS_MAX_FIELD_SECTION_SIZE, not sent",
			log_t::flag_t::WARNING, static_cast <unsigned long long> (sid)
		);
		// Выводим признак отказа
		return UINT64_MAX;
	}
	// Выделяем идентификатор обещанного push
	const uint64_t pushId = this->_nextPushId++;
	// Выгружаем накопленные инструкции кодека
	this->flushQpack();
	// Выполняем очистку буфера сборки кадра
	this->_frame.clear();
	// Собираем кадр обещания push
	h3::frame::serialize::pushPromise(this->_frame, pushId, this->_section);
	// Отправляем кадр обещания push
	this->emit(sid, this->_frame.data(), this->_frame.size(), false);
	// Выводим идентификатор обещанного push
	return pushId;
}
/**
 * @brief Метод отмены обещанного push
 *
 * @param pushId идентификатор отменяемого push
 *
 */
void awh::http::Parser_HTTP3::sendCancelPush(const uint64_t pushId) noexcept {
	// Если соединение завершено либо управляющий поток не открыт
	if(this->_closed || (this->_controlLocal == UINT64_MAX))
		// Выходим из метода
		return;
	// Выполняем очистку буфера сборки кадра
	this->_frame.clear();
	// Собираем кадр отмены обещанного push
	h3::frame::serialize::cancelPush(this->_frame, pushId);
	// Отправляем кадр отмены в управляющий поток
	this->emit(this->_controlLocal, this->_frame.data(), this->_frame.size(), false);
	/**
	 * Отменённое нами обещание запоминается: сервер мог отправить его поток раньше,
	 * чем получил отмену, и тот придёт следом - читать его уже незачем
	 */
	if((this->_endpoint == h3::endpoint_t::CLIENT) && !this->_openedPush.has(pushId))
		// Запоминаем отменённый идентификатор push
		this->_cancelledPush.put(pushId);
}
/**
 * @brief Метод проверки отменённости обещанного push
 *
 * @param pushId идентификатор обещанного push
 * @return       признак отменённости обещания
 *
 */
bool awh::http::Parser_HTTP3::pushCancelled(const uint64_t pushId) const noexcept {
	// Выводим признак отменённости обещания
	return this->_cancelledPush.has(pushId);
}
/**
 * @brief Конструктор
 *
 */
awh::http::Parser_HTTP3::Priority::Priority() noexcept :
 urgency(h3::proto::DEFAULT_URGENCY), incremental(false) {}
/**
 * @brief Метод получения расширенного приоритета потока (RFC 9218 §4)
 *
 * @param sid идентификатор потока
 * @return    расширенный приоритет потока
 *
 */
awh::http::Parser_HTTP3::priority_t awh::http::Parser_HTTP3::priority(const uint64_t sid) const noexcept {
	// Результат работы функции - расширенный приоритет потока
	priority_t result;
	// Выполняем поиск состояния потока
	auto i = this->_streams.find(sid);
	// Если состояние потока найдено
	if(i != this->_streams.end()){
		// Устанавливаем срочность потока
		result.urgency = i->second.urgency;
		// Устанавливаем признак инкрементальной доставки потока
		result.incremental = i->second.incremental;
	}
	// Выводим расширенный приоритет потока
	return result;
}
/**
 * @brief Метод получения расширенного приоритета обещания push (RFC 9218 §4)
 *
 * @param pushId идентификатор обещания push
 * @return       расширенный приоритет обещания push
 *
 */
awh::http::Parser_HTTP3::priority_t awh::http::Parser_HTTP3::pushPriority(const uint64_t pushId) const noexcept {
	// Результат работы функции - расширенный приоритет обещания push
	priority_t result;
	/**
	 * Выполняем поиск записи приоритета обещания
	 */
	for(const signal_t & item : this->_pushPriorities){
		// Если запись относится к искомому обещанию
		if(item.id == pushId){
			// Устанавливаем срочность обещания
			result.urgency = item.urgency;
			// Устанавливаем признак инкрементальной доставки обещания
			result.incremental = item.incremental;
			// Прекращаем поиск
			break;
		}
	}
	// Выводим расширенный приоритет обещания push
	return result;
}
/**
 * @brief Метод разрешения пиру выдавать push (только клиент)
 *
 * @param pushId наибольший разрешённый идентификатор push
 *
 */
void awh::http::Parser_HTTP3::sendMaxPushId(const uint64_t pushId) noexcept {
	// Если соединение завершено либо мы не клиент
	if(this->_closed || (this->_endpoint != h3::endpoint_t::CLIENT))
		// Выходим из метода
		return;
	/**
	 * Граница обязана не убывать: её снижение отозвало бы уже данное разрешение
	 * (RFC 9114 §7.2.7)
	 */
	if((this->_localMaxPushId != UINT64_MAX) && (pushId < this->_localMaxPushId))
		// Выходим из метода
		return;
	// Если служебные потоки открыть не удалось
	if(!this->prepare())
		// Выходим из метода
		return;
	// Если параметры соединения ещё не отправлены
	if(!this->_settingsSent)
		// Отправляем параметры соединения
		this->sendSettings();
	// Запоминаем объявленную нами границу идентификаторов push
	this->_localMaxPushId = pushId;
	// Выполняем очистку буфера сборки кадра
	this->_frame.clear();
	// Собираем кадр границы идентификаторов push
	h3::frame::serialize::maxPushId(this->_frame, pushId);
	// Отправляем кадр границы в управляющий поток
	this->emit(this->_controlLocal, this->_frame.data(), this->_frame.size(), false);
}
/**
 * @brief Метод отправки приоритета потока (RFC 9218 §7.2)
 *
 * @param sid         идентификатор потока
 * @param urgency     срочность потока
 * @param incremental признак инкрементального потока
 *
 */
void awh::http::Parser_HTTP3::sendPriority(const uint64_t sid, const uint8_t urgency, const bool incremental) noexcept {
	// Если соединение завершено
	if(this->_closed)
		// Выходим из метода
		return;
	/**
	 * Кадр отправляет только клиент: серверу запрещены обе его разновидности,
	 * а получивший его клиент обязан оборвать соединение (RFC 9218 §7.2).
	 * Приоритет своего ответа сервер объявляет заголовком priority, а не кадром
	 */
	if(this->_endpoint == h3::endpoint_t::SERVER){
		// Записываем сообщение об ошибке в лог
		this->_log->print("HTTP/3 server is not allowed to send PRIORITY_UPDATE", log_t::flag_t::WARNING);
		// Выходим из метода
		return;
	}
	// Если служебные потоки открыть не удалось
	if(!this->prepare())
		// Выходим из метода
		return;
	// Если параметры соединения ещё не отправлены
	if(!this->_settingsSent)
		// Отправляем параметры соединения
		this->sendSettings();
	// Собираемое значение поля приоритета
	string value;
	// Формируем срочность потока в синтаксисе структурированных полей
	value.append("u=");
	// Дописываем значение срочности потока
	value.push_back(static_cast <char> ('0' + ::std::min <uint8_t> (urgency, h3::proto::MAX_URGENCY)));
	// Если поток объявляется инкрементальным
	if(incremental)
		// Дописываем признак инкрементального потока
		value.append(", i");
	// Выполняем очистку буфера сборки кадра
	this->_frame.clear();
	// Собираем кадр обновления приоритета потока
	h3::frame::serialize::priorityUpdate(this->_frame, false, sid, value);
	// Отправляем кадр приоритета в управляющий поток
	this->emit(this->_controlLocal, this->_frame.data(), this->_frame.size(), false);
}
/**
 * @brief Метод отправки приоритета обещания push (RFC 9218 §7.2)
 *
 * @param pushId      идентификатор обещания push
 * @param urgency     срочность (0 - наивысшая, 7 - наименьшая)
 * @param incremental признак инкрементальной доставки
 *
 */
void awh::http::Parser_HTTP3::sendPushPriority(const uint64_t pushId, const uint8_t urgency, const bool incremental) noexcept {
	// Если соединение завершено
	if(this->_closed)
		// Выходим из метода
		return;
	/**
	 * Приоритизировать обещание push вправе только клиент, которому оно выдано:
	 * серверу отправка кадра запрещена прямо (RFC 9218 §7.2)
	 */
	if(this->_endpoint == h3::endpoint_t::SERVER){
		// Записываем сообщение об ошибке в лог
		this->_log->print("HTTP/3 server is not allowed to send PRIORITY_UPDATE", log_t::flag_t::WARNING);
		// Выходим из метода
		return;
	}
	// Если служебные потоки открыть не удалось
	if(!this->prepare())
		// Выходим из метода
		return;
	// Если параметры соединения ещё не отправлены
	if(!this->_settingsSent)
		// Отправляем параметры соединения
		this->sendSettings();
	// Собираемое значение поля приоритета
	string value;
	// Формируем срочность обещания в синтаксисе структурированных полей
	value.append("u=");
	// Дописываем значение срочности обещания
	value.push_back(static_cast <char> ('0' + ::std::min <uint8_t> (urgency, h3::proto::MAX_URGENCY)));
	// Если обещание объявляется инкрементальным
	if(incremental)
		// Дописываем признак инкрементальной доставки
		value.append(", i");
	// Выполняем очистку буфера сборки кадра
	this->_frame.clear();
	// Собираем кадр обновления приоритета обещания push
	h3::frame::serialize::priorityUpdate(this->_frame, true, pushId, value);
	// Отправляем кадр приоритета в управляющий поток
	this->emit(this->_controlLocal, this->_frame.data(), this->_frame.size(), false);
}
/**
 * @brief Метод обрыва потока
 *
 * @param sid  идентификатор потока
 * @param code код ошибки, с которым обрывается поток
 *
 */
void awh::http::Parser_HTTP3::sendReset(const uint64_t sid, const error_t code) noexcept {
	// Если функция обратного вызова обрыва потока установлена
	if(this->_callbacks.abort){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Просим транспорт прекратить отправку в поток
			this->_callbacks.abort(sid, code, false);
			/**
			 * У двунаправленного потока обрывается и приём: без этого отправитель
			 * продолжал бы слать данные в поток, который мы больше не читаем
			 * (RFC 9114 §4.1)
			 */
			if(bidirectional(sid))
				// Просим транспорт прекратить приём из потока
				this->_callbacks.abort(sid, code, true);
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception &) {
			// Исключение из пользовательской функции обратного вызова гасим на месте
		}
	}
	// Закрываем поток с указанным кодом
	this->closeStream(sid, code);
}
/**
 * @brief Метод завершения соединения (RFC 9114 §5.2)
 *
 * @param id идентификатор потока запроса (от сервера) либо push (от клиента)
 *
 */
void awh::http::Parser_HTTP3::sendGoaway(const uint64_t id) noexcept {
	// Если соединение завершено
	if(this->_closed)
		// Выходим из метода
		return;
	// Если служебные потоки открыть не удалось
	if(!this->prepare())
		// Выходим из метода
		return;
	// Если параметры соединения ещё не отправлены
	if(!this->_settingsSent)
		// Отправляем параметры соединения
		this->sendSettings();
	/**
	 * Объявленный идентификатор обязан не возрастать: возрастание отозвало бы
	 * уже данное обещание не обрабатывать потоки (RFC 9114 §5.2)
	 */
	if(id > this->_goawayLocal)
		// Выходим из метода
		return;
	// Запоминаем объявленный нами идентификатор
	this->_goawayLocal = id;
	// Выполняем очистку буфера сборки кадра
	this->_frame.clear();
	// Собираем кадр завершения соединения
	h3::frame::serialize::goaway(this->_frame, id);
	// Отправляем кадр завершения в управляющий поток
	this->emit(this->_controlLocal, this->_frame.data(), this->_frame.size(), false);
}
/**
 * @brief Метод плавного завершения соединения
 *
 */
void awh::http::Parser_HTTP3::sendShutdown() noexcept {
	/**
	 * Предупреждающий GOAWAY с предельным идентификатором прекращает открытие
	 * новых потоков, оставляя уже открытые дожить штатно (RFC 9114 §5.2)
	 */
	this->sendGoaway(h3::proto::MAX_VARINT);
}
/**
 * @brief Метод получения списка потоков с накопленными исходящими данными
 *
 * @param output список идентификаторов потоков
 *
 */
void awh::http::Parser_HTTP3::outgoing(vector <uint64_t> & output) noexcept {
	// Выполняем очистку списка потоков
	output.clear();
	/**
	 * Выполняем перебор всех буферов исходящих данных
	 */
	for(const auto & item : this->_pending){
		// Если у потока есть неотправленные данные либо неотправленный признак завершения
		if((item.second.consumed < item.second.buffer.size()) || item.second.fin)
			// Дописываем поток в список
			output.push_back(item.first);
	}
}
/**
 * @brief Метод получения накопленных исходящих данных потока
 *
 * @param sid идентификатор потока
 * @return    представление накопленных исходящих данных
 *
 */
string_view awh::http::Parser_HTTP3::pending(const uint64_t sid) noexcept {
	// Выполняем поиск буфера исходящих данных потока
	auto i = this->_pending.find(sid);
	// Если буфер исходящих данных не найден
	if(i == this->_pending.end())
		// Выводим пустое представление
		return string_view();
	// Выводим представление накопленных исходящих данных
	return string_view(i->second.buffer).substr(i->second.consumed);
}
/**
 * @brief Метод отметки исходящих данных потока как отправленных
 *
 * @param sid  идентификатор потока
 * @param size количество отправленных октетов
 *
 */
void awh::http::Parser_HTTP3::consumePending(const uint64_t sid, const size_t size) noexcept {
	// Выполняем поиск буфера исходящих данных потока
	auto i = this->_pending.find(sid);
	// Если буфер исходящих данных не найден
	if(i == this->_pending.end())
		// Выходим из метода
		return;
	// Наращиваем количество выданных наружу октетов
	i->second.consumed += ::std::min(size, (i->second.buffer.size() - i->second.consumed));
	// Если буфер выдан наружу целиком
	if(i->second.consumed >= i->second.buffer.size()){
		// Выполняем очистку буфера исходящих данных
		i->second.buffer.clear();
		// Сбрасываем количество выданных наружу октетов
		i->second.consumed = 0;
		// Если поток завершён в исходящем направлении
		if(i->second.fin)
			// Удаляем буфер исходящих данных завершённого потока
			this->_pending.erase(i);
	}
	/**
	 * Вычитанное обвязкой освободило место наружу: продолжаем выдачу тела и
	 * опрашиваем источник. Это и есть точка, в которой обратное давление
	 * pull-модели превращается обратно в разрешение отправлять
	 */
	this->pumpStream(sid);
}
/**
 * @brief Метод проверки завершения потока в исходящем направлении
 *
 * @param sid идентификатор потока
 * @return    признак того, что поток закрыт нами и данных больше не будет
 *
 */
bool awh::http::Parser_HTTP3::finished(const uint64_t sid) noexcept {
	// Выполняем поиск буфера исходящих данных потока
	auto i = this->_pending.find(sid);
	// Если буфер исходящих данных не найден - данных больше не будет
	if(i == this->_pending.end())
		// Выводим признак завершения потока
		return true;
	// Выводим признак завершения потока в исходящем направлении
	return (i->second.fin && (i->second.consumed >= i->second.buffer.size()));
}
/**
 * @brief Метод установки функции обратного вызова открытия однонаправленного потока
 *
 * @param callback функция обратного вызова
 *
 */
void awh::http::Parser_HTTP3::on(open_callback_t callback) noexcept {
	// Устанавливаем функцию обратного вызова открытия однонаправленного потока
	this->_callbacks.open = ::std::move(callback);
}
/**
 * @brief Метод установки функции обратного вызова записи исходящих байтов
 *
 * @param callback функция обратного вызова
 *
 */
void awh::http::Parser_HTTP3::on(write_callback_t callback) noexcept {
	// Устанавливаем функцию обратного вызова записи исходящих байтов
	this->_callbacks.write = ::std::move(callback);
}
/**
 * @brief Метод установки функции обратного вызова обрыва потока
 *
 * @param callback функция обратного вызова
 *
 */
void awh::http::Parser_HTTP3::on(abort_callback_t callback) noexcept {
	// Устанавливаем функцию обратного вызова обрыва потока
	this->_callbacks.abort = ::std::move(callback);
}
/**
 * @brief Метод установки функции обратного вызова применённого SETTINGS пира
 *
 * @param callback функция обратного вызова
 *
 */
void awh::http::Parser_HTTP3::on(settings_callback_t callback) noexcept {
	// Устанавливаем функцию обратного вызова применённых параметров пира
	this->_callbacks.settings = ::std::move(callback);
}
/**
 * @brief Метод установки функции обратного вызова открытия нового потока
 *
 * @param callback функция обратного вызова
 *
 */
void awh::http::Parser_HTTP3::on(begin_callback_t callback) noexcept {
	// Устанавливаем функцию обратного вызова открытия нового потока
	this->_callbacks.begin = ::std::move(callback);
}
/**
 * @brief Метод установки функции обратного вызова закрытия потока
 *
 * @param callback функция обратного вызова
 *
 */
void awh::http::Parser_HTTP3::on(close_callback_t callback) noexcept {
	// Устанавливаем функцию обратного вызова закрытия потока
	this->_callbacks.close = ::std::move(callback);
}
/**
 * @brief Метод установки функции обратного вызова ошибки уровня соединения
 *
 * @param callback функция обратного вызова
 *
 */
void awh::http::Parser_HTTP3::on(error_callback_t callback) noexcept {
	// Устанавливаем функцию обратного вызова ошибки уровня соединения
	this->_callbacks.error = ::std::move(callback);
}
/**
 * @brief Метод установки функции обратного вызова полученного GOAWAY
 *
 * @param callback функция обратного вызова
 *
 */
void awh::http::Parser_HTTP3::on(goaway_callback_t callback) noexcept {
	// Устанавливаем функцию обратного вызова завершения соединения пиром
	this->_callbacks.goaway = ::std::move(callback);
}
/**
 * @brief Метод установки функции обратного вызова анонса server push
 *
 * @param callback функция обратного вызова
 *
 */
void awh::http::Parser_HTTP3::on(push_callback_t callback) noexcept {
	// Устанавливаем функцию обратного вызова анонса server push
	this->_callbacks.push = ::std::move(callback);
}
/**
 * @brief Метод установки функции обратного вызова фазы приёма сообщения потока
 *
 * @param callback функция обратного вызова
 *
 */
void awh::http::Parser_HTTP3::on(phase_callback_t callback) noexcept {
	// Устанавливаем функцию обратного вызова фазы приёма сообщения потока
	this->_callbacks.phase = ::std::move(callback);
}
/**
 * @brief Метод установки функции обратного вызова провайдера полей потока
 *
 * @param callback функция обратного вызова
 *
 */
void awh::http::Parser_HTTP3::on(provider_callback_t callback) noexcept {
	// Устанавливаем функцию обратного вызова провайдера полей потока
	this->_callbacks.provider = ::std::move(callback);
}
/**
 * @brief Метод установки функции обратного вызова поля секции
 *
 * @param callback функция обратного вызова
 *
 */
void awh::http::Parser_HTTP3::on(header_callback_t callback) noexcept {
	// Устанавливаем функцию обратного вызова поля секции
	this->_callbacks.header = ::std::move(callback);
}
/**
 * @brief Метод установки функции обратного вызова фрагмента тела потока
 *
 * @param callback функция обратного вызова
 *
 */
void awh::http::Parser_HTTP3::on(data_callback_t callback) noexcept {
	// Устанавливаем функцию обратного вызова фрагмента тела потока
	this->_callbacks.data = ::std::move(callback);
}
/**
 * @brief Метод установки функции обратного вызова готовности потока принимать данные
 *
 * @param callback функция обратного вызова
 *
 */
void awh::http::Parser_HTTP3::on(writable_callback_t callback) noexcept {
	// Устанавливаем функцию обратного вызова готовности потока
	this->_callbacks.writable = ::move(callback);
}
/**
 * @brief Конструктор
 *
 * @param direct направление разбора сообщений
 * @param fmk    объект фреймворка
 * @param log    объект для работы с логами
 *
 */
awh::http::Parser_HTTP3::Parser_HTTP3(const direct_t direct, const fmk_t * fmk, const log_t * log) noexcept :
 parser_t(direct, fmk, log),
 _endpoint((direct == direct_t::REQUEST) ? h3::endpoint_t::SERVER : h3::endpoint_t::CLIENT),
 _encoder(0, 0), _decoder(h3::proto::QPACK_TABLE_CAPACITY, h3::proto::QPACK_BLOCKED_STREAMS),
 _proto(proto_t::HTTP3),
 _sendLowWater(SEND_LOW_WATER), _sendHighWater(SEND_HIGH_WATER), _outputHighWater(OUTPUT_HIGH_WATER),
 _controlLocal(UINT64_MAX), _controlRemote(UINT64_MAX), _encoderLocal(UINT64_MAX),
 _decoderLocal(UINT64_MAX), _encoderRemote(UINT64_MAX), _decoderRemote(UINT64_MAX),
 _settingsReceived(false), _settingsSent(false), _closed(false),
 _maxPushId(UINT64_MAX), _localMaxPushId(UINT64_MAX), _nextPushId(0),
 _promisedPush(PUSH_HISTORY_CACHE), _promisedCursor(0),
 _goawayLocal(h3::proto::MAX_VARINT), _goawayRemote(h3::proto::MAX_VARINT),
 _error(error_t::H3_NO_ERROR), _epoch(0), _generation(0) {
	/**
	 * Разрешение расширенного CONNECT выдаёт только сервер: анонс клиента
	 * не имел бы адресата (RFC 9220 §3)
	 */
	if(this->_endpoint == h3::endpoint_t::CLIENT)
		// Снимаем разрешение расширенного CONNECT
		this->_settings.enableConnectProtocol = false;
	// Выполняем приведение состояния парсера к исходному
	this->reset();
}
