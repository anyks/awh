/**
 * @file uri.hpp
 * @date 2026-03-28
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
 * \~russian
 * @brief Заголовочный файл модуля работы с универсальными идентификаторами ресурсов —
 *        класс Uniform_Resource_Identifier для разбора, сборки, нормализации и кодирования URI,
 *        работы с параметрами запроса, пользовательскими данными и относительными ссылками
 *
 * \~english
 * @brief Header file of the module of working with the uniform resource identifiers —
 *        the Uniform_Resource_Identifier class for the parsing, the assembly, the normalization and the encoding of the URIs,
 *        the work with the parameters of a query, with the user data and with the relative references
 *
 * \~
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Экранируем повторную инициализацию модуля
 */
#ifndef __AWH_NET_URI__
#define __AWH_NET_URI__

/**
 * Стандартные заголовочные файлы
 */
#include <string>
#include <vector>
#include <memory>
#include <cstdint>
#include <iostream>
#include <functional>
#include <unordered_map>

/**
 * Подключаем заголовочный файл проекта
 */
#include "addr.hpp"

/**
 * Снимаем на время объявлений макросы, чьи имена заняты
 * членами перечислений ниже (возвращает их macro_pop.hpp в конце файла)
 */
#include "../sys/macro_push.hpp"

/**
 * \~russian
 * @brief Основное пространство имён
 *
 *
 * \~english
 * @brief Main namespace
 *
 * \~
 */
namespace awh {
	/**
	 * Используем стандартное пространство имён
	 */
	using namespace std;

	/**
	 * \~russian
	 * @brief Класс для работы с универсальными идентификаторами ресурсов
	 *
	 * @details Разбирает адрес ресурса на составные части, собирает его обратно и
	 * сличает адреса между собой - по правилам RFC 3986. Держит **один**
	 * адрес и разбором его состояние меняет
	 *
	 * Отличается от `nwt_t` тем, что разбирает запись, уже признанную
	 * адресом ресурса, и делает это строго. Тот же, наоборот, сперва
	 * определяет вид записи и к строгости не стремится
	 *
	 * @note Разбор в ряде мест намеренно отступает от эталонных разборщиков -
	 * приводит запись к принятому виду, снимает точечные сегменты, читает
	 * незнакомую схему с числом за двоеточием как хост с портом. Отступления
	 * эти собраны с доводами в `tests/net/uri/DECISIONS.md`: прежде чем
	 * подавать наблюдение как дефект, следует поискать его там
	 *
	 * @warning Разбор ведётся **относительно уже разобранного**, а не с чистого
	 * листа. Так и задумано: сервер отдаёт в заголовке перенаправления
	 * ссылку, которая может быть и полным адресом, и одним лишь путём, и
	 * даже одними параметрами, - а собрать из неё нужно адрес, по которому
	 * идти дальше. Разбор второй записи поверх первой поэтому не заменяет
	 * её, а разрешает относительно неё. Чтобы разобрать запись
	 * самостоятельно, объект следует очистить
	 *
	 * @note Очистка сбрасывает сам адрес, но **не настройки**: режим
	 * разрешения ссылок и обратная связь для добавочного параметра её
	 * переживают. Заведены они один раз на объект и действуют на все
	 * последующие разборы
	 *
	 * @par Пример: разбор адреса и переход по перенаправлению
	 * @par Пример: самостоятельный разбор нескольких адресов
	 *
	 * @code{.cpp}
	 * awh::uri_t uri(&fmk, &log);
	 * // Разбираем исходный адрес
	 * uri.parse("https://anyks.com/api/v1/users?page=1");
	 * // Разрешаем перенаправление относительно него: выйдет https://anyks.com/api/v2/users
	 * uri.parse("../v2/users");
	 * // Собираем адрес обратно строкой
	 * const string address = uri.print();
	 * @endcode
	 *
	 * @code{.cpp}
	 * for(const auto & address : addresses){
	 *     // Очищаем объект, иначе запись разрешится относительно предыдущей
	 *     uri.clear();
	 *     // Разбираем очередной адрес
	 *     if(uri.parse(address) != awh::uri_t::type_t::NONE)
	 *         connect(uri.host(), uri.port());
	 * }
	 * @endcode
	 *
	 * \~english
	 * @brief Class for working with the uniform resource identifiers
	 * @details Parses an address of a resource into its constituent parts, assembles it back and
	 * matches the addresses with each other — by the rules of RFC 3986. Holds **one**
	 * address and changes its state by the parsing
	 * Differs from `nwt_t` in that it parses a record already recognized as
	 * an address of a resource, and does it strictly. That one, on the contrary, first
	 * determines the kind of the record and does not strive for the strictness
	 * @note The parsing in a number of places deliberately departs from the reference parsers —
	 * it brings a record to the accepted form, removes the dot segments, reads
	 * an unknown schema with a number behind a colon as a host with a port. These departures
	 * are gathered with the arguments in `tests/net/uri/DECISIONS.md`: before
	 * submitting an observation as a defect, one should look for it there
	 * @warning The parsing is performed **relative to what is already parsed**, and not from a clean
	 * slate. It is intended so: a server gives in the header of a redirection
	 * a reference, which may be both a full address, and a path alone, and
	 * even the parameters alone, — and an address to go further by needs to be assembled from it.
	 * The parsing of a second record over the first one therefore does not replace
	 * it, but resolves it relative to it. To parse a record
	 * on its own, the object should be cleared
	 * @note The clearing resets the address itself, but **not the settings**: the mode
	 * of the resolution of the references and the callback for the additional parameter
	 * outlive it. They are started once per object and are in force for all the
	 * subsequent parsings
	 * @par Example: the parsing of an address and the following of a redirection
	 * @par Example: the independent parsing of several addresses
	 *
	 * @code{.cpp}
	 * awh::uri_t uri(&fmk, &log);
	 * // Parsing the original address
	 * uri.parse("https://anyks.com/api/v1/users?page=1");
	 * // Resolving the redirection relative to it: https://anyks.com/api/v2/users will come out
	 * uri.parse("../v2/users");
	 * // Building the address back into a string
	 * const string address = uri.print();
	 * @endcode
	 *
	 * @code{.cpp}
	 * for(const auto & address : addresses){
	 *     // Clearing the object, otherwise the record will be resolved relative to the previous one
	 *     uri.clear();
	 *     // Parsing the next address
	 *     if(uri.parse(address) != awh::uri_t::type_t::NONE)
	 *         connect(uri.host(), uri.port());
	 * }
	 * @endcode
	 *
	 */
	typedef class __AWH_SHARED_EXPORT__ Uniform_Resource_Identifier {
		public:
			/**
			 * \~russian
			 * @brief Режим формата URI
			 *
			 * @details Задаёт, выписывать ли то, что и без того подразумевается. Полный
			 * вид выписывает схему и порт всегда, умный - лишь когда они заданы, а
			 * порт вдобавок отличается от принятого для схемы
			 *
			 * @note Умный вид короче и привычнее человеку, полный - однозначнее и
			 * удобнее сравнению как строк: у него записи "http://a.com" и
			 * "http://a.com:80" сойдутся
			 *
			 * \~english
			 * @brief Mode of the format of a URI
			 * @details Sets whether what is implied anyway should be written out. The full
			 * kind writes out the schema and the port always, the smart one — only when they are set, and
			 * the port in addition differs from the one accepted for the schema
			 * @note The smart kind is shorter and more customary to a human, the full one — more unambiguous and
			 * more convenient to the comparison as strings: at it the records "http://a.com" and
			 * "http://a.com:80" agree
			 *
			 * \~
			 */
			enum class format_t : uint8_t {
				NONE  = 0x00, // Режим формата URI не определён
				FULL  = 0x01, // Полный формат URI (с указанием схемы и порта)
				SMART = 0x02  // Умный формат URI (с указанием схемы и порта только при их наличии)
			};
			/**
			 * \~russian
			 * @brief Режим формата URI для печати
			 *
			 * @details Выбирает, какую часть адреса выписывать: весь его целиком либо
			 * одну составляющую - схему, учётные данные, хост, путь, параметры,
			 * якорь
			 *
			 * Два значения стоят особняком и составляющим не отвечают.
			 * Происхождение - это схема, хост и порт вместе, то самое сочетание,
			 * которым определяется, из одного ли места пришли два адреса. Запрос -
			 * путь вместе с параметрами и якорем, то есть всё, что уходит в
			 * запросе после имени узла
			 *
			 * @note Обратная связь для добавочного параметра при выписывании одних
			 * лишь параметров **не вызывается** - по ним она обычно и считает
			 * свою величину, и вызов ушёл бы в бесконечное повторение
			 *
			 * \~english
			 * @brief Mode of the format of a URI for the printing
			 * @details Chooses which part of the address should be written out: the whole of it or
			 * one constituent — the schema, the credentials, the host, the path, the parameters,
			 * the anchor
			 * Two values stand apart and answer to no constituents.
			 * The origin is the schema, the host and the port together, that very combination,
			 * by which it is determined whether two addresses have come from one place. The query is
			 * the path together with the parameters and the anchor, that is everything that goes in
			 * a request after the name of the node
			 * @note The callback for the additional parameter at the writing out of the parameters
			 * alone **is not called** — by them it usually counts
			 * its own value, and the call would go into an infinite repetition
			 *
			 * \~
			 */
			enum class item_t : uint8_t {
				NONE     = 0x00, // Режим формата URI для печати не определён
				URI      = 0x01, // Режим формата URI для печати полного URI
				SCHEME   = 0x02, // Режим формата URI для печати схемы URI
				USER     = 0x03, // Режим формата URI для печати параметров пользователя URI
				HOST     = 0x04, // Режим формата URI для печати хоста URI
				PATH     = 0x05, // Режим формата URI для печати пути URI
				QUERY    = 0x06, // Режим формата URI для печати параметров URI
				FRAGMENT = 0x07, // Режим формата URI для печати якоря URI
				ORIGIN   = 0x08, // Режим формата URI для печати происхождения URI
				REQUEST  = 0x09  // Режим формата URI для печати запроса URI
			};
			/**
			 * \~russian
			 * @brief Тип URI
			 *
			 * @details Разновидность адреса, выведенная из его схемы. Служит двум
			 * нуждам: по ней подбирается принятый для схемы порт, и по ней же
			 * разбор понимает, чего от записи ждать - у почты авторити отделяется
			 * от схемы иначе, чем у сетевых схем, а у местного сокета за хостом
			 * стоит путь файловой системы
			 *
			 * @note Значение `SCHEME` отведено записи, чья схема разбору незнакома.
			 * Ошибкой это не считается: схем существует много больше, чем
			 * перечислено здесь, и незнакомая разбирается по общим правилам, но
			 * принятого порта у неё нет
			 *
			 * \~english
			 * @brief Type of a URI
			 * @details The variety of an address, derived from its schema. Serves two
			 * needs: by it the port accepted for the schema is picked, and by it as well
			 * the parsing understands what to expect from the record — at the mail the authority is separated
			 * from the schema otherwise than at the network schemas, and at a local socket behind the host
			 * there stands a path of the file system
			 * @note The `SCHEME` value is given over to a record whose schema is unknown to the parsing.
			 * This is not considered an error: there exist far more schemas than are
			 * enumerated here, and an unknown one is parsed by the common rules, but
			 * has no accepted port
			 *
			 * \~
			 */
			enum class type_t : uint8_t {
				NONE       = 0x00, // Тип URI не определён
				WS         = 0x01, // URI для протокола WebSocket
				WSS        = 0x02, // URI для протокола WebSocket Secure
				FTP        = 0x03, // URI для протокола FTP
				SSH        = 0x04, // URI для протокола SSH
				UDS 	   = 0x05, // URI для Unix Domain Socket
				FILE 	   = 0x06, // URI для файловой системы
				MQTT       = 0x07, // URI для протокола MQTT
				HTTP       = 0x08, // URI для протокола HTTP
				HTTPS      = 0x09, // URI для протокола HTTPS
				REDIS      = 0x0A, // URI для протокола Redis
				MYSQL 	   = 0x0B, // URI для протокола MySQL
				EMAIL      = 0x0C, // URI для электронной почты
				SOCKS5     = 0x0D, // URI для протокола Socks5
				SCHEME     = 0x0E, // URI для схемы
				POSTGRESQL = 0x0F  // URI для протокола PostgreSQL
			};
			/**
			 * \~russian
			 * @brief Вид записи адреса
			 *
			 * @details Авторити отделяется от схемы двумя косыми чертами не у всякой схемы.
			 *          Записи "mailto:user@example.com", "acct:user@example.com",
			 *          "sip:user@example.com:5060" и "stun:example.com:3478" несут авторити
			 *          сразу за двоеточием, и косых черт у них нет вовсе. Схема же
			 *          "urn:isbn:0451450523" авторити не несёт совсем, а собачка в записи
			 *          "news:msgid@example.com" разделителем не является - она часть
			 *          обозначения сообщения.
			 *
			 *          Отдельно стоит запись довода команды копирования по сети
			 *          "git@github.com:group/repo.git": схемы у неё нет, а путь отделён от
			 *          хоста двоеточием. Адресом ресурса она не является, но встречается
			 *          повсеместно, и разбор её двоеточие принимал за разделитель порта -
			 *          первый сегмент пути съедался негодным номером
			 *
			 * \~english
			 * @brief Kind of the record of an address
			 * @details The authority is separated from the schema by two slashes not at every schema.
			 *          The records "mailto:user@example.com", "acct:user@example.com",
			 *          "sip:user@example.com:5060" and "stun:example.com:3478" carry the authority
			 *          right after the colon, and have no slashes at all. The schema
			 *          "urn:isbn:0451450523", though, carries no authority at all, and the at sign in the record
			 *          "news:msgid@example.com" is not a separator — it is a part of the designation
			 *          of the message.
			 *          Apart stands the record of an argument of the command of the copying over the network
			 *          "git@github.com:group/repo.git": it has no schema, and the path is separated from
			 *          the host by a colon. It is not an address of a resource, but occurs
			 *          everywhere, and the parsing took its colon for a separator of a port —
			 *          the first segment of the path was eaten up by an unfit number
			 *
			 * \~
			 */
			enum class form_t : uint8_t {
				NONE    = 0x00, // Авторити у записи нет
				BARE    = 0x01, // Авторити записана сразу за двоеточием схемы
				SLASHES = 0x02, // Авторити отделена от схемы двумя косыми чертами
				COMMAND = 0x03  // Запись копирования по сети: путь отделён от хоста двоеточием
			};
			/**
			 * \~russian
			 * @brief Режим разрешения относительных ссылок
			 *
			 * @details Ссылка со схемой сама по себе полный адрес, и строгий разбор
			 *          замещает ею адрес целиком (RFC 3986 5.2.2). Прежняя же
			 *          спецификация частичных адресов (RFC 1808 5.2) схему, совпавшую
			 *          со схемой основы, отбрасывала и разрешала остаток как
			 *          относительную ссылку: авторы писали "href=http:page.html",
			 *          подразумевая соседний файл. RFC 3986 называет это лазейкой и
			 *          советует её избегать, но допускает ради совместимости -
			 *          браузеры её поддерживают до сих пор и заголовок Location
			 *          разрешают вместе с ней.
			 *
			 *          Увести на чужой узел лазейка не способна: срабатывает она лишь
			 *          у ссылки без авторити, а значит хост неизбежно достаётся от
			 *          основы. Необязательности же двух косых черт у особых схем,
			 *          которую браузеры допускают сверх того, здесь нет: она хост как
			 *          раз меняет - "https:evil.com/x" читается ими как хост
			 *          "evil.com", - и RFC такого не знает вовсе
			 *
			 * \~english
			 * @brief Mode of the resolution of the relative references
			 * @details A reference with a schema is a full address by itself, and the strict parsing
			 *          replaces the address entirely by it (RFC 3986 5.2.2). The former
			 *          specification of the partial addresses (RFC 1808 5.2), though, discarded a schema that had coincided
			 *          with the schema of the base and resolved the remainder as
			 *          a relative reference: the authors wrote "href=http:page.html",
			 *          implying a neighbouring file. RFC 3986 calls this a loophole and
			 *          advises to avoid it, but allows it for the sake of the compatibility —
			 *          the browsers support it to this day and resolve the Location header
			 *          together with it.
			 *          The loophole is not capable of taking one away to a foreign node: it triggers only
			 *          at a reference without an authority, which means the host inevitably comes from
			 *          the base. The optionality of the two slashes at the special schemas,
			 *          which the browsers allow beyond that, is absent here: it changes the host
			 *          exactly — "https:evil.com/x" is read by them as the host
			 *          "evil.com", — and the RFC knows nothing of the kind
			 *
			 * \~
			 */
			enum class resolve_t : uint8_t {
				NONE       = 0x00, // Режим разрешения ссылок не установлен
				STRICT     = 0x01, // Строгий разбор: ссылка со схемой замещает адрес целиком
				COMPATIBLE = 0x02  // Совместимый разбор: схема, совпавшая со схемой основы, отбрасывается
			};
		public:
			/**
			 * \~russian
			 * @brief Структура пользователя URI
			 *
			 * \~english
			 * @brief Structure of the user of a URI
			 *
			 * \~
			 */
			typedef struct __AWH_SHARED_EXPORT__ User {
				// Имя пользователя
				string username;
				// Пароль пользователя
				string password;
				/**
				 * \~russian
				 * @brief Конструктор
				 *
				 *
				 * \~english
				 * @brief Constructor
				 *
				 * \~
				 */
				explicit User() noexcept;
			} user_t;
		private:
			// Тип URI
			type_t _type;
		private:
			/**
			 * \~russian
			 * Вид записи адреса
			 *
			 * @details Признаком наличия авторити служили заведённые атрибуты адреса.
			 *          Авторити, однако, бывает и пустой - записи "file:///etc/hosts" и
			 *          "http:///path" несут пустой хост с путём от корня, - и атрибутов ей
			 *          не заводится. Наличие её поэтому запоминается отдельно (RFC 3986 5.3).
			 *
			 *          Запоминается вместе с ним и вид самой записи: разделитель авторити
			 *          отбирался по разновидности адреса, и одинарное двоеточие доставалось
			 *          одной лишь почте. Схем же, записывающих авторити сразу за
			 *          двоеточием, полтора десятка, а произвольная схема сохранять свой вид
			 *          обязана в любом случае - "custom:user@host" собиралась как
			 *          "custom://user@host".
			 *
			 * \~english
			 * Kind of the record of an address
			 * @details The sign of the presence of the authority was the started attributes of the address.
			 *          The authority, however, happens to be an empty one as well — the records "file:///etc/hosts" and
			 *          "http:///path" carry an empty host with a path from the root, — and no attributes are
			 *          started for it. Its presence is therefore remembered separately (RFC 3986 5.3).
			 *          Together with it the kind of the record itself is remembered as well: the separator of the authority
			 *          was picked by the variety of the address, and a single colon went
			 *          to the mail alone. There are a dozen and a half of the schemas writing the authority right after
			 *          the colon, though, and an arbitrary schema is obliged to preserve its kind
			 *          in any case — "custom:user@host" was assembled as
			 *          "custom://user@host".
			 *
			 * \~
			 */
			form_t _form;
			/**
			 * \~russian
			 * Признак пути, ведущего от корня
			 *
			 * @details Ведущая косая черта пути в сегментах не хранится, и признак
			 *          ведения пути от корня отбирался по разновидности адреса:
			 *          иерархической он приписывался всегда, произвольной схеме -
			 *          никогда. Свойство это, однако, принадлежит самому пути, и
			 *          записи "custom:path" и "custom:/path" - разные адреса
			 *          (RFC 3986 3.3), как разными являются ссылки "path/to" и
			 *          "/path/to": разрешаются они относительно базового адреса
			 *          по-разному (RFC 3986 5.2). Записи теряли корень или получали
			 *          его без спроса - смотря по разновидности.
			 *
			 *          У адреса с авторити путь ведёт от корня всегда: пустым он ей
			 *          равнозначен, а непустой начинается с косой черты (RFC 3986 3.3)
			 *
			 * \~english
			 * Sign of a path leading from the root
			 * @details The leading slash of a path is not held in the segments, and the sign
			 *          of the leading of the path from the root was picked by the variety of the address:
			 *          to a hierarchical one it was ascribed always, to an arbitrary schema —
			 *          never. This property, however, belongs to the path itself, and
			 *          the records "custom:path" and "custom:/path" are different addresses
			 *          (RFC 3986 3.3), as different are the references "path/to" and
			 *          "/path/to": they are resolved relative to a base address
			 *          differently (RFC 3986 5.2). The records lost the root or received
			 *          it without asking — depending on the variety.
			 *          At an address with an authority the path leads from the root always: an empty one is equivalent to it,
			 *          and a non-empty one begins with a slash (RFC 3986 3.3)
			 *
			 * \~
			 */
			bool _rooted;
		private:
			// Режим разрешения относительных ссылок
			resolve_t _resolve;
		private:
			// Параметры пользователя URI
			user_t _user;
		private:
			// Схема URI
			string _scheme;
			// Якорь URI
			string _fragment;
		private:
			/**
			 * \~russian
			 * Зона IPv6-адреса хоста
			 *
			 * @details Зона обозначает область действия адреса локальной связи и частью
			 *          хоста является наравне с самим адресом (RFC 6874). Атрибуты
			 *          сетевого адреса, однако, несут одни лишь октеты адреса, и зона
			 *          оставалась лежать в объекте работы с сетевыми адресами - общем на
			 *          весь модуль и переживающем разбор. Оттого доставалась она
			 *          следующему разобранному адресу: хост "127.0.0.1", разобранный
			 *          вслед за "[fe80::1%25eth0]", печатался как "127.0.0.1%eth0"
			 *
			 * \~english
			 * Zone of the IPv6 address of the host
			 * @details The zone designates the scope of a link-local address and is a part
			 *          of the host on a par with the address itself (RFC 6874). The attributes
			 *          of a network address, however, carry the octets of the address alone, and the zone
			 *          remained lying in the object of the work with the network addresses — common for
			 *          the whole module and outliving the parsing. Therefore it went
			 *          to the next parsed address: the host "127.0.0.1", parsed
			 *          after "[fe80::1%25eth0]", was printed as "127.0.0.1%eth0"
			 *
			 * \~
			 */
			string _zone;
		private:
			// Объект работы с сетевыми адресами
			unique_ptr <net_addr_t> _addr;
			// Хост URI
			unique_ptr <net::attr_t> _attr;
		private:
			// Путь URI
			vector <string> _path;
			// Параметры URI
			unordered_multimap <string, string> _query;
		private:
			/**
			 * \~russian
			 * @brief Функция обратного вызова для генерации параметра URI (например, для генерации контрольной суммы)
			 *
			 * @details Функция обратного вызова вызывается при генерации URI и получает указатель на объект URI.
			 *          Функция должна вернуть строку, которая будет добавлена в конец URI.
			 *
			 * @param uri указатель на объект URI
			 * @return    строка, которая будет добавлена в конец URI
			 *
			 * \~english
			 * @brief Callback function for the generation of a parameter of a URI (for example, for the generation of a checksum)
			 * @details The callback function is called at the generation of a URI and receives a pointer to the object of the URI.
			 *          The function must return a string, which will be added at the end of the URI.
			 * @param uri pointer to the object of the URI
			 * @return    string, which will be added at the end of the URI
			 *
			 * \~
			 */
			function <string (const Uniform_Resource_Identifier *)> _callback;
		private:
			// Объект фреймворка
			const fmk_t * _fmk;
			// Объект для работы с логами
			const log_t * _log;
		private:
			/**
			 * \~russian
			 * @brief Метод определения стандартного порта для текущего типа URI
			 *
			 * @return стандартный порт или 0, если для типа URI он не определён
			 *
			 * \~english
			 * @brief Method of the determination of the standard port for the current type of a URI
			 * @return standard port or 0, if it is not defined for the type of the URI
			 *
			 * \~
			 */
			uint16_t defaultPort() const noexcept;
		private:
			/**
			 * \~russian
			 * @brief Метод добавления схемы URI в результат с разделителем, зависящим от типа URI
			 *
			 * @param result результат, в который добавляется схема URI
			 *
			 * \~english
			 * @brief Method of adding the schema of a URI into the result with a separator depending on the type of the URI
			 * @param result result the schema of the URI is added into
			 *
			 * \~
			 */
			void appendScheme(string & result) const noexcept;
			/**
			 * \~russian
			 * @brief Метод добавления параметров пользователя (логин и пароль) в результат
			 *
			 * @param result    результат, в который добавляются параметры пользователя
			 * @param delimiter флаг добавления разделителя "@" после параметров пользователя
			 *
			 * \~english
			 * @brief Method of adding the parameters of the user (the login and the password) into the result
			 * @param result    result the parameters of the user are added into
			 * @param delimiter flag of the addition of the separator "@" after the parameters of the user
			 *
			 * \~
			 */
			void appendUser(string & result, const bool delimiter) const noexcept;
		private:
			/**
			 * \~russian
			 * @brief Метод добавления хоста (и порта для сетевых адресов) в результат
			 *
			 * @param result результат, в который добавляется хост
			 * @param format режим формата URI для генерации
			 *
			 * \~english
			 * @brief Method of adding the host (and the port for the network addresses) into the result
			 * @param result result the host is added into
			 * @param format mode of the format of the URI for the generation
			 *
			 * \~
			 */
			void appendHost(string & result, const format_t format) const noexcept;
			/**
			 * \~russian
			 * @brief Метод добавления порта хоста в результат с учётом формата генерации
			 *
			 * @param result результат, в который добавляется порт
			 * @param port   порт хоста, заданный явно (0 — если не задан)
			 * @param format режим формата URI для генерации
			 *
			 * \~english
			 * @brief Method of adding the port of the host into the result with the format of the generation taken into account
			 * @param result result the port is added into
			 * @param port   port of the host set explicitly (0 — if it is not set)
			 * @param format mode of the format of the URI for the generation
			 *
			 * \~
			 */
			void appendPort(string & result, const uint16_t port, const format_t format) const noexcept;
		private:
			/**
			 * \~russian
			 * @brief Метод добавления сегментов пути URI в результат
			 *
			 * @param result  результат, в который добавляются сегменты пути
			 * @param leading флаг записи разделителя перед первым сегментом пути
			 *
			 * \~english
			 * @brief Method of adding the segments of the path of a URI into the result
			 * @param result  result the segments of the path are added into
			 * @param leading flag of the writing of the separator before the first segment of the path
			 *
			 * \~
			 */
			void appendPath(string & result, const bool leading) const noexcept;
			/**
			 * \~russian
			 * @brief Метод проверки наличия авторити у URI
			 *
			 * @return результат проверки
			 *
			 * \~english
			 * @brief Method of checking the presence of an authority at a URI
			 * @return result of the check
			 *
			 * \~
			 */
			bool hasAuthority() const noexcept;
			/**
			 * \~russian
			 * @brief Метод проверки пути URI на ведение от корня
			 *
			 * @return результат проверки
			 *
			 * \~english
			 * @brief Method of checking the path of a URI for the leading from the root
			 * @return result of the check
			 *
			 * \~
			 */
			bool rootedPath() const noexcept;
			/**
			 * \~russian
			 * @brief Метод проверки хоста URI на путь к доменному сокету
			 *
			 * @return результат проверки
			 *
			 * \~english
			 * @brief Method of checking the host of a URI for a path to a domain socket
			 * @return result of the check
			 *
			 * \~
			 */
			bool socketHost() const noexcept;
			/**
			 * \~russian
			 * @brief Метод проверки записи адреса на вид копирования по сети
			 *
			 * @return результат проверки
			 *
			 * \~english
			 * @brief Method of checking the record of an address for the kind of the copying over the network
			 * @return result of the check
			 *
			 * \~
			 */
			bool commandForm() const noexcept;
			/**
			 * \~russian
			 * @brief Метод определения вида записи, которым адрес выписывается
			 *
			 * @details Хранимый вид записи выписывается не всегда: вид копирования
			 * по сети порта не несёт, и адрес с портом записывается обычным
			 * иерархическим видом, а вид, авторити за одним двоеточием ставящий,
			 * читается обратно лишь тогда, когда её там выдаёт схема либо
			 * учётная запись. Метод выводит вид, который окажется в строке, - им
			 * адреса и сличаются, потому что хранимый вид из строки невосстановим
			 *
			 * @return вид записи, которым адрес выписывается
			 *
			 * \~english
			 * @brief Method of the determination of the kind of the record an address is written out by
			 * @details The held kind of the record is not written out always: the kind of the copying
			 * over the network carries no port, and an address with a port is written by the ordinary
			 * hierarchical kind, and the kind placing the authority behind a single colon,
			 * is read back only when it is given there by the schema or by
			 * the credentials. The method yields the kind that will turn out to be in the string — by it
			 * the addresses are matched, because the held kind is unrestorable from the string
			 * @return kind of the record an address is written out by
			 *
			 * \~
			 */
			form_t printForm() const noexcept;
			/**
			 * \~russian
			 * @brief Метод проверки наличия параметров URI для записи
			 *
			 * @return результат проверки
			 *
			 * \~english
			 * @brief Method of checking the presence of the parameters of a URI for the record
			 * @return result of the check
			 *
			 * \~
			 */
			bool hasQuery() const noexcept;
			/**
			 * \~russian
			 * @brief Метод проверки равнозначности пути URI корневому
			 *
			 * @return результат проверки
			 *
			 * \~english
			 * @brief Method of checking the equivalence of the path of a URI to the root one
			 * @return result of the check
			 *
			 * \~
			 */
			bool rootPath() const noexcept;
		private:
			/**
			 * \~russian
			 * @brief Метод сличения хостов URI
			 *
			 * @details Хост сличается по атрибутам, а не по напечатанной записи: у
			 *          адреса сети сличаются его октеты, у имени - строка, а разным
			 *          разновидностям хоста равными не быть никогда
			 *
			 * @note Зона IPv6-адреса здесь не сличается: принадлежит она хосту, но
			 *       хранится отдельно от атрибутов, и сличают её вызывающие
			 *
			 * @param uri объект URI для сличения
			 * @return    результат сличения
			 *
			 * \~english
			 * @brief Method of the matching of the hosts of the URIs
			 * @details The host is matched by the attributes, and not by the printed record: at
			 *          an address of a network its octets are matched, at a name — the string, and different
			 *          varieties of a host are never to be equal
			 * @note The zone of an IPv6 address is not matched here: it belongs to the host, but
			 *       is held separately from the attributes, and the callers match it
			 * @param uri object of the URI for the matching
			 * @return    result of the matching
			 *
			 * \~
			 */
			bool sameHost(const Uniform_Resource_Identifier & uri) const noexcept;
			/**
			 * \~russian
			 * @brief Метод добавления пар ключ-значение параметров URI в результат
			 *
			 * @param result    результат, в который добавляются параметры
			 * @param separator флаг записи разделителя перед строкой параметров
			 * @param callback  флаг вызова функции обратного вызова, дающей добавочный параметр
			 *
			 * \~english
			 * @brief Method of adding the key-value pairs of the parameters of a URI into the result
			 * @param result    result the parameters are added into
			 * @param separator flag of the writing of the separator before the string of the parameters
			 * @param callback  flag of the call of the callback function giving the additional parameter
			 *
			 * \~
			 */
			void appendQuery(string & result, const bool separator, const bool callback) const noexcept;
			/**
			 * \~russian
			 * @brief Метод добавления относительного URI-запроса в результат
			 *
			 * @param result результат, в который добавляется URI-запрос
			 *
			 * \~english
			 * @brief Method of adding a relative URI query into the result
			 * @param result result the URI query is added into
			 *
			 * \~
			 */
			void appendRequest(string & result) const noexcept;
		public:
			/**
			 * \~russian
			 * @brief Метод очистки URI
			 *
			 * @details Опустошает объект: сбрасывает все составляющие адреса, его тип,
			 * вид записи и признак пути от корня
			 *
			 * @warning Очищать объект следует **перед каждым самостоятельным
			 * разбором**: без очистки запись разрешается относительно прежней
			 *
			 * @note Настройки объекта очистку переживают - и режим разрешения
			 * ссылок, и обратная связь для добавочного параметра. Заведены они на
			 * объект, а не на разбираемую запись
			 *
			 * \~english
			 * @brief Method of clearing a URI
			 * @details Empties the object: resets all the constituents of the address, its type,
			 * the kind of the record and the sign of the path from the root
			 * @warning The object should be cleared **before every independent
			 * parsing**: without the clearing a record is resolved relative to the previous one
			 * @note The settings of the object outlive the clearing — both the mode of the resolution
			 * of the references, and the callback for the additional parameter. They are started per
			 * object, and not per parsed record
			 *
			 * \~
			 */
			void clear() noexcept;
		public:
			/**
			 * \~russian
			 * @brief Метод проверки на существование данных
			 *
			 * @return результат проверки
			 *
			 * \~english
			 * @brief Method of checking the existence of the data
			 * @return result of the check
			 *
			 * \~
			 */
			bool empty() const noexcept;
		public:
			/**
			 * \~russian
			 * @brief Метод получения типа URI
			 *
			 * @return тип URI
			 *
			 * \~english
			 * @brief Method of getting the type of a URI
			 * @return type of the URI
			 *
			 * \~
			 */
			type_t type() const noexcept;
		public:
			/**
			 * \~russian
			 * @brief Метод получения вида записи адреса
			 *
			 * @return вид записи адреса
			 *
			 * \~english
			 * @brief Method of getting the kind of the record of an address
			 * @return kind of the record of the address
			 *
			 * \~
			 */
			form_t form() const noexcept;
			/**
			 * \~russian
			 * @brief Метод установки вида записи адреса
			 *
			 * @details Вид записи отбирается по схеме и потому задаётся отдельно лишь тогда,
			 *          когда схема неизвестна: запись "custom:user@host" собирается видом
			 *          BARE, а "custom://user@host" - видом SLASHES
			 *
			 * @param form вид записи адреса для установки
			 *
			 * \~english
			 * @brief Method of setting the kind of the record of an address
			 * @details The kind of the record is picked by the schema and is therefore set separately only when
			 *          the schema is unknown: the record "custom:user@host" is assembled by the kind
			 *          BARE, and "custom://user@host" — by the kind SLASHES
			 * @param form kind of the record of the address to set
			 *
			 * \~
			 */
			void form(const form_t form) noexcept;
		public:
			/**
			 * \~russian
			 * @brief Метод получения признака пути, ведущего от корня
			 *
			 * @return признак пути, ведущего от корня
			 *
			 * \~english
			 * @brief Method of getting the sign of a path leading from the root
			 * @return sign of a path leading from the root
			 *
			 * \~
			 */
			bool rooted() const noexcept;
			/**
			 * \~russian
			 * @brief Метод установки признака пути, ведущего от корня
			 *
			 * @details У адреса с авторити путь ведёт от корня всегда, и признак этот
			 *          задаётся лишь записи, авторити не имеющей: путь адреса
			 *          "custom:path" от корня не ведёт, а путь адреса "custom:/path" -
			 *          ведёт
			 *
			 * @param rooted признак пути, ведущего от корня, для установки
			 *
			 * \~english
			 * @brief Method of setting the sign of a path leading from the root
			 * @details At an address with an authority the path leads from the root always, and this sign
			 *          is set only to a record having no authority: the path of the address
			 *          "custom:path" does not lead from the root, and the path of the address "custom:/path" —
			 *          does
			 * @param rooted sign of a path leading from the root, to set
			 *
			 * \~
			 */
			void rooted(const bool rooted) noexcept;
		public:
			/**
			 * \~russian
			 * @brief Метод получения режима разрешения относительных ссылок
			 *
			 * @return режим разрешения относительных ссылок
			 *
			 * \~english
			 * @brief Method of getting the mode of the resolution of the relative references
			 * @return mode of the resolution of the relative references
			 *
			 * \~
			 */
			resolve_t resolve() const noexcept;
			/**
			 * \~russian
			 * @brief Метод установки режима разрешения относительных ссылок
			 *
			 * @details Режим принадлежит объекту, а не разбираемой записи, и очисткой
			 *          объекта не сбрасывается: установленный однажды, он действует на
			 *          все последующие разборы
			 *
			 * @param resolve режим разрешения относительных ссылок для установки
			 *
			 * \~english
			 * @brief Method of setting the mode of the resolution of the relative references
			 * @details The mode belongs to the object, and not to the parsed record, and is not reset by
			 *          the clearing of the object: set once, it is in force for
			 *          all the subsequent parsings
			 * @param resolve mode of the resolution of the relative references to set
			 *
			 * \~
			 */
			void resolve(const resolve_t resolve) noexcept;
		public:
			/**
			 * \~russian
			 * @brief Метод получения схемы URI
			 *
			 * @return схема URI
			 *
			 * \~english
			 * @brief Method of getting the schema of a URI
			 * @return schema of the URI
			 *
			 * \~
			 */
			const string & scheme() const noexcept;
			/**
			 * \~russian
			 * @brief Метод установки схемы URI
			 *
			 * @param scheme схема URI для установки
			 *
			 * \~english
			 * @brief Method of setting the schema of a URI
			 * @param scheme schema of the URI to set
			 *
			 * \~
			 */
			void scheme(string_view scheme) noexcept;
		public:
			/**
			 * \~russian
			 * @brief Метод получения параметров пользователя URI
			 *
			 * @return параметры пользователя URI
			 *
			 * \~english
			 * @brief Method of getting the parameters of the user of a URI
			 * @return parameters of the user of the URI
			 *
			 * \~
			 */
			const user_t & user() const noexcept;
			/**
			 * \~russian
			 * @brief Метод установки параметров пользователя URI
			 *
			 * @param user параметры пользователя URI для установки
			 *
			 * \~english
			 * @brief Method of setting the parameters of the user of a URI
			 * @param user parameters of the user of the URI to set
			 *
			 * \~
			 */
			void user(const user_t & user) noexcept;
			/**
			 * \~russian
			 * @brief Метод установки логина и пароля пользователя URI
			 *
			 * @param username логин пользователя URI для установки
			 * @param password пароль пользователя URI для установки
			 *
			 * \~english
			 * @brief Method of setting the login and the password of the user of a URI
			 * @param username login of the user of the URI to set
			 * @param password password of the user of the URI to set
			 *
			 * \~
			 */
			void user(string_view username, string_view password) noexcept;
		public:
			/**
			 * \~russian
			 * @brief Метод получения якоря URI
			 *
			 * @return якорь URI
			 *
			 * \~english
			 * @brief Method of getting the anchor of a URI
			 * @return anchor of the URI
			 *
			 * \~
			 */
			const string & fragment() const noexcept;
			/**
			 * \~russian
			 * @brief Метод установки якоря URI
			 *
			 * @param fragment якорь URI для установки
			 *
			 * \~english
			 * @brief Method of setting the anchor of a URI
			 * @param fragment anchor of the URI to set
			 *
			 * \~
			 */
			void fragment(string_view fragment) noexcept;
		public:
			/**
			 * \~russian
			 * @brief Метод получения атрибутов URI
			 *
			 * @return атрибуты URI
			 *
			 * \~english
			 * @brief Method of getting the attributes of a URI
			 * @return attributes of the URI
			 *
			 * \~
			 */
			const net::attr_t * attr() const noexcept;
			/**
			 * \~russian
			 * @brief Метод установки атрибутов URI
			 *
			 * @param attr атрибуты URI для установки
			 *
			 * \~english
			 * @brief Method of setting the attributes of a URI
			 * @param attr attributes of the URI to set
			 *
			 * \~
			 */
			void attr(const net::attr_t * attr) noexcept;
		public:
			/**
			 * \~russian
			 * @brief Метод получения хоста URI
			 *
			 * @return хост URI
			 *
			 * \~english
			 * @brief Method of getting the host of a URI
			 * @return host of the URI
			 *
			 * \~
			 */
			string host() const noexcept;
			/**
			 * \~russian
			 * @brief Метод установки хоста URI
			 *
			 * @param host хост URI для установки
			 *
			 * \~english
			 * @brief Method of setting the host of a URI
			 * @param host host of the URI to set
			 *
			 * \~
			 */
			void host(string_view host) noexcept;
		public:
			/**
			 * \~russian
			 * @brief Метод получения порта URI
			 *
			 * @return порт URI
			 *
			 * \~english
			 * @brief Method of getting the port of a URI
			 * @return port of the URI
			 *
			 * \~
			 */
			uint16_t port() const noexcept;
			/**
			 * \~russian
			 * @brief Метод установки порта URI
			 *
			 * @param port порт URI для установки
			 *
			 * \~english
			 * @brief Method of setting the port of a URI
			 * @param port port of the URI to set
			 *
			 * \~
			 */
			void port(const uint16_t port) noexcept;
		public:
			/**
			 * \~russian
			 * @brief Метод получения пути URI
			 *
			 * @return путь URI
			 *
			 * \~english
			 * @brief Method of getting the path of a URI
			 * @return path of the URI
			 *
			 * \~
			 */
			const vector <string> & path() const noexcept;
			/**
			 * \~russian
			 * @brief Метод установки пути URI
			 *
			 * @param path путь URI для установки
			 *
			 * \~english
			 * @brief Method of setting the path of a URI
			 * @param path path of the URI to set
			 *
			 * \~
			 */
			void path(const vector <string> & path) noexcept;
		public:
			/**
			 * \~russian
			 * @brief Метод получения параметров URI
			 *
			 * @return параметры URI
			 *
			 * \~english
			 * @brief Method of getting the parameters of a URI
			 * @return parameters of the URI
			 *
			 * \~
			 */
			const unordered_multimap <string, string> & query() const noexcept;
			/**
			 * \~russian
			 * @brief Метод установки параметров URI
			 *
			 * @param query параметры URI для установки
			 *
			 * \~english
			 * @brief Method of setting the parameters of a URI
			 * @param query parameters of the URI to set
			 *
			 * \~
			 */
			void query(const unordered_multimap <string, string> & query) noexcept;
		public:
			/**
			 * \~russian
			 * @brief Метод парсинга URI-запроса
			 *
			 * @details Разбирает строку и **разрешает её относительно уже разобранного
			 * адреса** по правилам RFC 3986. У пустого объекта запись разбирается
			 * сама по себе, у заполненного - как ссылка относительно него: путь
			 * складывается с прежним, недостающие составляющие достаются от основы
			 *
			 * @warning Разбор поверх заполненного объекта прежний адрес **не
			 * заменяет**. Чтобы разобрать запись самостоятельно, объект следует
			 * очистить - иначе итог окажется склейкой двух записей
			 *
			 * @note Приметой наличия составляющей служит не пустота её, а само
			 * наличие: ссылка "?" задаёт параметры пустыми, а ссылка "g" не задаёт
			 * их вовсе, и разрешаются они по-разному
			 *
			 * @note Выводится тип того адреса, который объект несёт после разбора, а
			 * не признак удачи. У пустого объекта неразобранная запись даёт
			 * неопределённый тип, у заполненного - тип прежнего адреса: разбор при
			 * неудаче прежнего содержимого не трогает, и отличить отвергнутую
			 * запись от разрешённой по одному лишь типу нельзя
			 *
			 * @warning Отказ от негодной записи по типу не виден: ловить его следует
			 * методом `resolve()`, отдающим сам признак. Переход по
			 * перенаправлению, разбор которого отвергнут, иначе выглядел бы
			 * состоявшимся, а работа пошла бы по прежнему адресу
			 *
			 * @warning Негодное представление порта отменяет разбор целиком, а не
			 * снимается с адреса молча: порт входит в опознание ресурса, и запись
			 * "http://host:99999/" при отброшенном порте разбиралась бы как
			 * "http://host/" - то есть уводила бы обращение на другой узел. Пустое
			 * же представление ("http://host:/") задаёт порт стандартным и разбору
			 * не мешает (RFC 3986 3.2.3)
			 *
			 * @param uri строка URI-запроса для получения параметров
			 * @return    тип URI
			 *
			 * \~english
			 * @brief Method of the parsing of a URI query
			 * @details Parses a string and **resolves it relative to the already parsed
			 * address** by the rules of RFC 3986. At an empty object a record is parsed
			 * by itself, at a filled one — as a reference relative to it: the path
			 * is added to the previous one, the missing constituents come from the base
			 * @warning A parsing over a filled object **does not replace** the previous
			 * address. To parse a record on its own, the object should be
			 * cleared — otherwise the result will turn out to be a splice of two records
			 * @note The marker of the presence of a constituent is not its emptiness, but its very
			 * presence: the reference "?" sets the parameters as empty, and the reference "g" does not set
			 * them at all, and they are resolved differently
			 * @note What is yielded is the type of the address the object carries after the parsing, and
			 * not a sign of a success. At an empty object an unparsed record gives
			 * an undefined type, at a filled one — the type of the previous address: the parsing at
			 * a failure does not touch the previous content, and telling a rejected
			 * record from a resolved one by the type alone is not possible
			 * @warning A refusal from an unfit record is not seen by the type: it should be caught
			 * by the `resolve()` method giving back the sign itself. The following of
			 * a redirection whose parsing is rejected would otherwise look
			 * as having taken place, and the work would go by the previous address
			 * @warning An unfit representation of the port cancels the parsing entirely, and is not
			 * removed from the address silently: the port enters the identification of the resource, and the record
			 * "http://host:99999/" at a discarded port would be parsed as
			 * "http://host/" — that is would take the address to another node. An empty
			 * representation ("http://host:/"), though, sets the port as the standard one and does not hinder
			 * the parsing (RFC 3986 3.2.3)
			 * @param uri string of the URI query to get the parameters from
			 * @return    type of the URI
			 *
			 * \~
			 */
			type_t parse(string_view uri) noexcept;
			/**
			 * \~russian
			 * @brief Метод разрешения записи относительно разобранного адреса
			 *
			 * @details Делает то же, что и `parse()`, но отдаёт признак согласия с
			 * записью, а не разновидность получившегося адреса. Разбор ведёт
			 * именно он, а `parse()` лишь отдаёт разновидность поверх него
			 *
			 * @warning Отказ от записи означает, что прежний адрес остался
			 * нетронутым - и только это. Различить его по разновидности адреса
			 * нельзя: у заполненного объекта она остаётся прежней, то есть
			 * настоящей. Переход по перенаправлению следует делать этим методом:
			 * иначе отвергнутая ссылка выглядела бы применённой, а работа пошла
			 * бы по прежнему адресу
			 *
			 * @par Пример: переход по перенаправлению
			 * @param uri строка URI-запроса для разрешения
			 * @return    признак согласия с записью
			 *
			 * @code{.cpp}
			 * // Разрешаем ссылку заголовка перенаправления относительно адреса запроса
			 * if(!uri.resolve(location)){
			 *     // Ссылка негодна, прежний адрес цел - идти по нему нельзя
			 *     return;
			 * }
			 * // Ссылка применена, адрес перехода собран
			 * connect(uri.host(), uri.port());
			 * @endcode
			 *
			 * \~english
			 * @brief Method of the resolution of a record relative to a parsed address
			 * @details Does the same as `parse()`, but gives back the sign of the agreement with
			 * the record, and not the variety of the resulting address. The parsing is performed
			 * exactly by it, and `parse()` only gives back the variety on top of it
			 * @warning A refusal from a record means that the previous address has remained
			 * untouched — and only that. Telling it apart by the variety of the address
			 * is not possible: at a filled object it remains the previous one, that is
			 * the real one. The following of a redirection should be done by this method:
			 * otherwise a rejected reference would look as applied, and the work would go
			 * by the previous address
			 * @par Example: the following of a redirection
			 * @param uri string of the URI query to resolve
			 * @return    sign of the agreement with the record
			 *
			 * @code{.cpp}
			 * // Resolving the link of the redirection header relative to the address of the request
			 * if(!uri.resolve(location)){
			 *     // The link is unfit, the previous address is intact — it must not be followed
			 *     return;
			 * }
			 * // The link is applied, the address of the transition is built
			 * connect(uri.host(), uri.port());
			 * @endcode
			 *
			 */
			bool resolve(string_view uri) noexcept;
		public:
			/**
			 * \~russian
			 * @brief Метод генерации ETag хэша текста
			 *
			 * @details Считает краткую примету содержимого - ту, которой помечают
			 * ответ, чтобы при повторном запросе не пересылать неизменившееся
			 *
			 * @warning Примета эта **не криптографическая**: подобрать другой текст
			 * с той же приметой можно намеренно. Годится она для сверки версий, но
			 * не для подтверждения подлинности
			 *
			 * @note Укорочение записи повышает вероятность случайного совпадения
			 * двух разных текстов, а с ним и вероятность отдать несвежий ответ
			 * вместо изменившегося
			 *
			 * @param text текст для перевода в строку
			 * @param size длина записи хэша ETag в шестнадцатеричных цифрах, от 1 до 16 (по умолчанию 16)
			 * @return     хэш etag
			 *
			 * \~english
			 * @brief Method of the generation of the ETag hash of a text
			 * @details Counts a short marker of the content — the one an answer is marked by,
			 * so that at a repeated request the unchanged would not be resent
			 * @warning This marker is **not a cryptographic** one: another text
			 * with the same marker can be picked deliberately. It is fit for the checking of the versions, but
			 * not for the confirmation of the authenticity
			 * @note The shortening of the record raises the probability of an accidental coincidence
			 * of two different texts, and with it the probability of giving back a stale answer
			 * instead of a changed one
			 * @param text text to convert into a string
			 * @param size length of the record of the ETag hash in the hexadecimal digits, from 1 to 16 (16 by default)
			 * @return     etag hash
			 *
			 * \~
			 */
			string etag(string_view text, const uint8_t size = 16) const noexcept;
		public:
			/**
			 * \~russian
			 * @brief Метод генерации строки URI
			 *
			 * @details Собирает адрес обратно в строку - весь целиком либо одну его
			 * составляющую, смотря что запрошено. Сборка не есть точное
			 * восстановление исходной записи: она приводит адрес к принятому виду,
			 * и запись, разобранная и собранная обратно, может отличаться от
			 * исходной написанием, оставаясь равнозначной ей
			 *
			 * @note Добавочный параметр, задаваемый обратной связью, дописывается
			 * в конец параметров при сборке полного адреса и запроса - даже тогда,
			 * когда собственных параметров у адреса нет
			 *
			 * @param item   режим элемента URI для генерации
			 * @param format режим формата URI для генерации
			 * @return       строка URI
			 *
			 * \~english
			 * @brief Method of the generation of the string of a URI
			 * @details Assembles the address back into a string — the whole of it or one of its
			 * constituents, depending on what is requested. The assembly is not an exact
			 * restoration of the original record: it brings the address to the accepted form,
			 * and a record parsed and assembled back may differ from
			 * the original one by the spelling, remaining equivalent to it
			 * @note The additional parameter set by the callback is appended
			 * at the end of the parameters at the assembly of the full address and of the query — even when
			 * the address has no parameters of its own
			 * @param item   mode of the element of the URI for the generation
			 * @param format mode of the format of the URI for the generation
			 * @return       string of the URI
			 *
			 * \~
			 */
			string print(const item_t item = item_t::URI, const format_t format = format_t::SMART) const noexcept;
		public:
			/**
			 * \~russian
			 * @brief Метод установки функции обратного вызова для генерации параметра URI (например, для генерации контрольной суммы)
			 *
			 * @details Функция обратного вызова даёт добавочный параметр URI, и он
			 *          дописывается в конец строки параметров при генерации полного
			 *          URI и относительного URI-запроса - в том числе и тогда, когда
			 *          собственных параметров у URI нет.
			 *
			 * @note    Генерация строки параметров отдельным элементом функцию
			 *          обратного вызова не вызывает: по этой самой строке она обычно
			 *          и считает контрольную сумму, и вызов её оттуда уходил бы в
			 *          бесконечную рекурсию
			 *
			 * @param cb функция обратного вызова для генерации параметра URI
			 *
			 * \~english
			 * @brief Method of setting the callback function for the generation of a parameter of a URI (for example, for the generation of a checksum)
			 * @details The callback function gives an additional parameter of the URI, and it
			 *          is appended at the end of the string of the parameters at the generation of the full
			 *          URI and of the relative URI query — including when
			 *          the URI has no parameters of its own.
			 * @note    The generation of the string of the parameters as a separate element does not call the callback
			 *          function: by this very string it usually
			 *          counts the checksum, and its call from there would go into
			 *          an infinite recursion
			 * @param cb callback function for the generation of a parameter of the URI
			 *
			 * \~
			 */
			void callback(function <string (const Uniform_Resource_Identifier *)> cb) noexcept;
		public:
			/**
			 * \~russian
			 * @brief Оператор проверки на существование данных
			 *
			 * @return результат проверки
			 *
			 * \~english
			 * @brief Operator of the check of the existence of the data
			 * @return result of the check
			 *
			 * \~
			 */
			operator bool() const noexcept;
			/**
			 * \~russian
			 * @brief Оператор получения типа URI
			 *
			 * @return тип URI
			 *
			 * \~english
			 * @brief Operator of getting the type of a URI
			 * @return type of the URI
			 *
			 * \~
			 */
			operator type_t() const noexcept;
			/**
			 * \~russian
			 * @brief Оператор генерации строки URI
			 *
			 * @return строка URI
			 *
			 * \~english
			 * @brief Operator of the generation of the string of a URI
			 * @return string of the URI
			 *
			 * \~
			 */
			operator string() const noexcept;
			/**
			 * \~russian
			 * @brief Оператор получения параметров пользователя URI
			 *
			 * @return параметры пользователя URI
			 *
			 * \~english
			 * @brief Operator of getting the parameters of the user of a URI
			 * @return parameters of the user of the URI
			 *
			 * \~
			 */
			operator user_t() const noexcept;
		public:
			/**
			 * \~russian
			 * @brief Оператор получения атрибутов URI
			 *
			 * @return атрибуты URI
			 *
			 * \~english
			 * @brief Operator of getting the attributes of a URI
			 * @return attributes of the URI
			 *
			 * \~
			 */
			operator const net::attr_t * () const noexcept;
			/**
			 * \~russian
			 * @brief Оператор получения пути URI
			 *
			 * @return путь URI
			 *
			 * \~english
			 * @brief Operator of getting the path of a URI
			 * @return path of the URI
			 *
			 * \~
			 */
			operator const vector <string> & () const noexcept;
			/**
			 * \~russian
			 * @brief Оператор получения параметров URI
			 *
			 * @return параметры URI
			 *
			 * \~english
			 * @brief Operator of getting the parameters of a URI
			 * @return parameters of the URI
			 *
			 * \~
			 */
			operator const unordered_multimap <string, string> & () const noexcept;
		public:
		public:
			/**
			 * \~russian
			 * @brief Метод сличения происхождений URI
			 *
			 * @details Происхождение ресурса составляют схема, хост и порт (RFC 6454 4):
			 *          учётные данные, путь, параметры и якорь в него не входят, и записи
			 *          "http://user:pw@a.com/x?y#z" и "http://a.com" происходят из одного
			 *          места. Порт сличается по действующему его значению, отчего адрес
			 *          "http://a.com" равнозначен адресу "http://a.com:80".
			 *
			 *          Сличение это чисто адресное: разрешения имён оно не ведёт и
			 *          равнозначности сверх той, что определена самим RFC 3986, не знает.
			 *          Имя и адрес, в который оно разрешается, происходят из разных мест
			 *
			 * @note Запись без схемы либо без авторити происхождения не имеет вовсе, и
			 *       сличение таких записей даёт ложь - в том числе двух одинаковых
			 *       (RFC 6454 4). Решение это намеренно осторожное: ответ этой проверки
			 *       решает, нести ли учётные данные дальше
			 *
			 * @param uri объект URI для сличения
			 * @return    результат сличения
			 *
			 * \~english
			 * @brief Method of the matching of the origins of the URIs
			 * @details The origin of a resource is made up of the schema, the host and the port (RFC 6454 4):
			 *          the credentials, the path, the parameters and the anchor do not enter it, and the records
			 *          "http://user:pw@a.com/x?y#z" and "http://a.com" originate from one
			 *          place. The port is matched by its effective value, and therefore the address
			 *          "http://a.com" is equivalent to the address "http://a.com:80".
			 *          This matching is a purely address one: it performs no resolution of the names and
			 *          knows no equivalence beyond the one defined by RFC 3986 itself.
			 *          A name and the address it resolves into originate from different places
			 * @note A record without a schema or without an authority has no origin at all, and
			 *       the matching of such records gives falsehood — including of two identical ones
			 *       (RFC 6454 4). This decision is deliberately cautious: the answer of this check
			 *       decides whether the credentials should be carried further
			 * @param uri object of the URI for the matching
			 * @return    result of the matching
			 *
			 * \~
			 */
			bool sameOrigin(const Uniform_Resource_Identifier & uri) const noexcept;
			/**
			 * \~russian
			 * @brief Оператор сравнения
			 *
			 * @details Сличает адреса по всем их составляющим - схеме, учётным данным,
			 * хосту, порту, пути, параметрам и якорю
			 *
			 * @note Сличение ведётся по правилам RFC 3986, а те для разных
			 * составляющих разные: схема и хост сличаются без оглядки на
			 * написание прописными или строчными, а путь, параметры и якорь - с
			 * оглядкой
			 *
			 * @note Различать происхождение, а не адрес целиком, позволяет метод
			 * `sameOrigin()`: учётные данные, путь, параметры и якорь в
			 * происхождение не входят
			 *
			 * @param uri параметры URI для сравнения
			 * @return    результат сравнения
			 *
			 * \~english
			 * @brief Comparison operator
			 * @details Matches the addresses by all of their constituents — by the schema, the credentials,
			 * the host, the port, the path, the parameters and the anchor
			 * @note The matching is performed by the rules of RFC 3986, and those are different for the different
			 * constituents: the schema and the host are matched without a regard for
			 * the spelling in the capital or in the lowercase letters, and the path, the parameters and the anchor — with
			 * a regard
			 * @note Telling apart the origin, and not the address entirely, is made possible by the
			 * `sameOrigin()` method: the credentials, the path, the parameters and the anchor do not enter the
			 * origin
			 * @param uri parameters of the URI for the comparison
			 * @return    result of the comparison
			 *
			 * \~
			 */
			bool operator == (const Uniform_Resource_Identifier & uri) const noexcept;
			/**
			 * \~russian
			 * @brief Оператор неравенства
			 *
			 * @param uri параметры URI для сравнения
			 * @return    результат сравнения
			 *
			 * \~english
			 * @brief Inequality operator
			 * @param uri parameters of the URI for the comparison
			 * @return    result of the comparison
			 *
			 * \~
			 */
			bool operator != (const Uniform_Resource_Identifier & uri) const noexcept;
		public:
			/**
			 * \~russian
			 * @brief Оператор парсинга URI-запроса
			 *
			 * @param uri строка URI-запроса для получения параметров
			 * @return    текущий объект
			 *
			 * \~english
			 * @brief Operator of the parsing of a URI query
			 * @param uri string of the URI query to get the parameters from
			 * @return    the current object
			 *
			 * \~
			 */
			Uniform_Resource_Identifier & operator = (string_view uri) noexcept;
			/**
			 * \~russian
			 * @brief Оператор установки параметров пользователя URI
			 *
			 * @param user параметры пользователя URI для установки
			 * @return     текущий объект
			 *
			 * \~english
			 * @brief Operator of setting the parameters of the user of a URI
			 * @param user parameters of the user of the URI to set
			 * @return     the current object
			 *
			 * \~
			 */
			Uniform_Resource_Identifier & operator = (const user_t & user) noexcept;
			/**
			 * \~russian
			 * @brief Оператор установки атрибутов URI
			 *
			 * @param attr атрибуты URI для установки
			 * @return     текущий объект
			 *
			 * \~english
			 * @brief Operator of setting the attributes of a URI
			 * @param attr attributes of the URI to set
			 * @return     the current object
			 *
			 * \~
			 */
			Uniform_Resource_Identifier & operator = (const net::attr_t * attr) noexcept;
			/**
			 * \~russian
			 * @brief Оператор установки пути URI
			 *
			 * @param path путь URI для установки
			 * @return     текущий объект
			 *
			 * \~english
			 * @brief Operator of setting the path of a URI
			 * @param path path of the URI to set
			 * @return     the current object
			 *
			 * \~
			 */
			Uniform_Resource_Identifier & operator = (const vector <string> & path) noexcept;
			/**
			 * \~russian
			 * @brief Оператор установки параметров URI
			 *
			 * @param query параметры URI для установки
			 * @return      текущий объект
			 *
			 * \~english
			 * @brief Operator of setting the parameters of a URI
			 * @param query parameters of the URI to set
			 * @return      the current object
			 *
			 * \~
			 */
			Uniform_Resource_Identifier & operator = (const unordered_multimap <string, string> & query) noexcept;
		public:
			/**
			 * \~russian
			 * @brief Оператор перемещающего присваивания параметров URI
			 *
			 * @param uri объект URI для получения параметров
			 * @return    параметры URI
			 *
			 * \~english
			 * @brief Move assignment operator of the parameters of a URI
			 * @param uri object of the URI to get the parameters from
			 * @return    parameters of the URI
			 *
			 * \~
			 */
			Uniform_Resource_Identifier & operator = (Uniform_Resource_Identifier && uri) noexcept;
			/**
			 * \~russian
			 * @brief Оператор присваивания присваивания параметров URI
			 *
			 * @param uri объект URI для получения параметров
			 * @return    параметры URI
			 *
			 * \~english
			 * @brief Assignment operator of the assignment of the parameters of a URI
			 * @param uri object of the URI to get the parameters from
			 * @return    parameters of the URI
			 *
			 * \~
			 */
			Uniform_Resource_Identifier & operator = (const Uniform_Resource_Identifier & uri) noexcept;
		public:
			/**
			 * \~russian
			 * @brief Конструктор перемещения
			 *
			 * @details Явным конструктор копирования быть не может: инициализация
			 *          копированием - возврат объекта из функции, передача его по
			 *          значению, задание его знаком равенства - через явный конструктор
			 *          не проходит, и запись "return uri;" не собиралась
			 *
			 * @param uri параметры URI для перемещения
			 *
			 * \~english
			 * @brief Move constructor
			 * @details The copy constructor cannot be an explicit one: the initialization
			 *          by a copying — the return of an object from a function, its passing by
			 *          a value, its setting by an equals sign — does not pass through an explicit constructor,
			 *          and the record "return uri;" was not built
			 * @param uri parameters of the URI to move
			 *
			 * \~
			 */
			Uniform_Resource_Identifier(Uniform_Resource_Identifier && uri) noexcept;
			/**
			 * \~russian
			 * @brief Конструктор копирования
			 *
			 * @param uri параметры URI для копирования
			 *
			 * \~english
			 * @brief Copy constructor
			 * @param uri parameters of the URI to copy
			 *
			 * \~
			 */
			Uniform_Resource_Identifier(const Uniform_Resource_Identifier & uri) noexcept;
		public:
			/**
			 * \~russian
			 * @brief Конструктор
			 *
			 * @param fmk объект фреймворка
			 * @param log объект для работы с логами
			 *
			 * \~english
			 * @brief Constructor
			 * @param fmk framework object
			 * @param log object for working with logs
			 *
			 * \~
			 */
			explicit Uniform_Resource_Identifier(const fmk_t * fmk, const log_t * log) noexcept;
		public:
			/**
			 * \~russian
			 * @brief Деструктор
			 *
			 *
			 * \~english
			 * @brief Destructor
			 *
			 * \~
			 */
			~Uniform_Resource_Identifier() noexcept;
	} uri_t;
	/**
	 * \~russian
	 * @brief Оператор [>>] чтения из потока URI
	 *
	 * @param is  поток для чтения
	 * @param uri URI для присвоения
	 *
	 * \~english
	 * @brief The [>>] operator of the reading of a URI from a stream
	 * @param is  stream to read from
	 * @param uri URI to assign
	 *
	 * \~
	 */
	__AWH_SHARED_EXPORT__ istream & operator >> (istream & is, uri_t & uri) noexcept;
	/**
	 * \~russian
	 * @brief Оператор [<<] вывода в поток URI
	 *
	 * @param os  поток куда нужно вывести данные
	 * @param uri URI для присвоения
	 *
	 * \~english
	 * @brief The [<<] operator of the output of a URI into a stream
	 * @param os  stream the data needs to be yielded into
	 * @param uri URI to assign
	 *
	 * \~
	 */
	__AWH_SHARED_EXPORT__ ostream & operator << (ostream & os, const uri_t & uri) noexcept;
};

/**
 * Возвращаем макросы, снятые в начале файла
 */
#include "../sys/macro_pop.hpp"

#endif // __AWH_NET_URI__
