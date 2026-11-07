/**
 * @file: engine.cpp
 * @date: 2022-08-03
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
 * Максимальный номер порта
 */
#define AWH_MAX_PORT 0xFFFF

/**
 * @brief Устанавливаем наибольшее число частей накопителя за один шаг перекачки в 64
 *
 * @note Ограничение от зависания: сокет, принимающий байты без перерыва, не должен
 *       держать в одном шаге дольше нескольких записей рукопожатия
 *
 */
#define AWH_ENGINE_FLUSH_LIMIT 0x40
/**
 * @brief Устанавливаем размер буфера приёма потокового канала в 16384 байта
 *
 * @note Наибольшая открытая запись TLS по разделу 5.2 RFC 8446 — 2^14 байт, поэтому
 *       один приём покрывает любую запись целиком
 *
 */
#define AWH_ENGINE_CHUNK_SIZE 0x4000
/**
 * @brief Устанавливаем размер буфера приёма дейтаграммного канала в 65536 байт
 *
 * @note 65535 — предельная длина дейтаграммы IPv6 заголовка UDP, байт запаса держим
 *       на случай обрезки, чтобы приём не разрезал запись рукопожатия
 *
 */
#define AWH_ENGINE_DATAGRAM_SIZE 0x10000
/**
 * @brief Устанавливаем наибольший размер накопленного приветствия в 65536 байт
 *
 * @note Приветствие клиента с накопленным запасом сертификатов на порядок меньше, а
 *       за этим пределом ждать продолжения нечего: байты подаём связи как есть
 *
 */
#define AWH_ENGINE_HELLO_LIMIT 0x10000
/**
 * @brief Устанавливаем наибольшее число приёмов канала за один шаг перекачки в 64
 *
 * @note Событие чтения краевое (EV_CLEAR), поэтому шаг обязан выбрать весь пришедший
 *       канал; предел держим на случай собеседника, не перестающего слать байты
 *
 */
#define AWH_ENGINE_FEED_LIMIT 0x40

/**
 * Отключаем Deprecated для Apple
 */
#if __APPLE__ && __clang__
	#pragma clang diagnostic ignored "-Wdeprecated-declarations"
#endif

/**
 * Подключаем заголовочный файл
 */
#include <net/engine.hpp>

/**
 * Модули внешнего модуля шифрования A-NTS
 */
#include <nts/pem/pem.hpp>
#include <nts/x509/x509.hpp>
#include <nts/trust/trust.hpp>
#include <nts/hello/hello.hpp>
#include <nts/client/client.hpp>
#include <nts/server/server.hpp>
#include <nts/context/context.hpp>
#include <nts/protect/protect.hpp>
#include <nts/extension/extension.hpp>

/**
 * Подписываемся на стандартное пространство имён
 */
using namespace std;

/**
 * Код издания протокола TLS 1.2
 */
static constexpr uint16_t TLS_VERSION_12 = 0x0303;
/**
 * Код издания протокола TLS 1.3
 */
static constexpr uint16_t TLS_VERSION_13 = 0x0304;
/**
 * Тип записи протокола с сообщениями рукопожатия (раздел 5.1 RFC 8446)
 */
static constexpr uint8_t RECORD_TYPE_HANDSHAKE = 0x16;
/**
 * Длина обвязки записи протокола: тип, издание и размер
 */
static constexpr size_t RECORD_HEADER_SIZE = 5;
/**
 * Длина обвязки сообщения рукопожатия: тип и три байта размера
 */
static constexpr size_t MESSAGE_HEADER_SIZE = 4;

/**
 * @brief Функция получения названия набора шифрования для журнала
 *
 * Названия приводятся к виду OpenSSL, чтобы журналы прежних и нынешних сборок читались одинаково
 *
 * @param suite согласованный набор шифрования
 * @return        название набора
 *
 */
static const char * suiteName(const nts::protect::suite_t suite) noexcept {
	// Результат работы функции
	const char * result = "UNKNOWN";
	/**
	 * Определяем согласованный набор
	 */
	switch(static_cast <uint16_t> (suite)){
		case static_cast <uint16_t> (nts::protect::suite_t::AES_128_GCM_SHA256):                 result = "TLS_AES_128_GCM_SHA256";           break;
		case static_cast <uint16_t> (nts::protect::suite_t::AES_256_GCM_SHA384):                 result = "TLS_AES_256_GCM_SHA384";           break;
		case static_cast <uint16_t> (nts::protect::suite_t::CHACHA20_POLY1305_SHA256):           result = "TLS_CHACHA20_POLY1305_SHA256";     break;
		case static_cast <uint16_t> (nts::protect::suite_t::ECDHE_ECDSA_AES_128_GCM_SHA256):     result = "ECDHE-ECDSA-AES128-GCM-SHA256";   break;
		case static_cast <uint16_t> (nts::protect::suite_t::ECDHE_ECDSA_AES_256_GCM_SHA384):     result = "ECDHE-ECDSA-AES256-GCM-SHA384";   break;
		case static_cast <uint16_t> (nts::protect::suite_t::ECDHE_RSA_AES_128_GCM_SHA256):       result = "ECDHE-RSA-AES128-GCM-SHA256";      break;
		case static_cast <uint16_t> (nts::protect::suite_t::ECDHE_RSA_AES_256_GCM_SHA384):       result = "ECDHE-RSA-AES256-GCM-SHA384";      break;
		case static_cast <uint16_t> (nts::protect::suite_t::ECDHE_RSA_CHACHA20_POLY1305_SHA256): result = "ECDHE-RSA-CHACHA20-POLY1305";      break;
		case static_cast <uint16_t> (nts::protect::suite_t::ECDHE_ECDSA_CHACHA20_POLY1305_SHA256):result = "ECDHE-ECDSA-CHACHA20-POLY1305";   break;
		case static_cast <uint16_t> (nts::protect::suite_t::ECDHE_ECDSA_AES_128_CBC_SHA256):     result = "ECDHE-ECDSA-AES128-CBC-SHA256";   break;
		case static_cast <uint16_t> (nts::protect::suite_t::ECDHE_RSA_AES_128_CBC_SHA256):       result = "ECDHE-RSA-AES128-CBC-SHA256";      break;
		case static_cast <uint16_t> (nts::protect::suite_t::ECDHE_ECDSA_AES_128_CBC_SHA):        result = "ECDHE-ECDSA-AES128-CBC-SHA";      break;
		case static_cast <uint16_t> (nts::protect::suite_t::ECDHE_ECDSA_AES_256_CBC_SHA):        result = "ECDHE-ECDSA-AES256-CBC-SHA";      break;
		case static_cast <uint16_t> (nts::protect::suite_t::ECDHE_RSA_AES_128_CBC_SHA):          result = "ECDHE-RSA-AES128-CBC-SHA";         break;
		case static_cast <uint16_t> (nts::protect::suite_t::ECDHE_RSA_AES_256_CBC_SHA):          result = "ECDHE-RSA-AES256-CBC-SHA";         break;
		case static_cast <uint16_t> (nts::protect::suite_t::RSA_AES_128_GCM_SHA256):             result = "AES128-GCM-SHA256";                break;
		case static_cast <uint16_t> (nts::protect::suite_t::RSA_AES_256_GCM_SHA384):             result = "AES256-GCM-SHA384";                break;
		case static_cast <uint16_t> (nts::protect::suite_t::RSA_AES_128_CBC_SHA):                result = "AES128-SHA";                       break;
		case static_cast <uint16_t> (nts::protect::suite_t::RSA_AES_256_CBC_SHA):                result = "AES256-SHA";                       break;
		case static_cast <uint16_t> (nts::protect::suite_t::RSA_AES_128_CBC_SHA256):             result = "AES128-SHA256";                    break;
		case static_cast <uint16_t> (nts::protect::suite_t::RSA_AES_256_CBC_SHA256):             result = "AES256-SHA256";                    break;
		case static_cast <uint16_t> (nts::protect::suite_t::KUZNYECHIK_MGM_L):                   result = "KUZNYECHIK-CBC-MAGMA-MGM-L";       break;
		case static_cast <uint16_t> (nts::protect::suite_t::KUZNYECHIK_MGM_S):                   result = "KUZNYECHIK-CBC-MAGMA-MGM-S";       break;
		case static_cast <uint16_t> (nts::protect::suite_t::MAGMA_MGM_L):                        result = "MAGMA-CBC-MAGMA-MGM-L";            break;
		case static_cast <uint16_t> (nts::protect::suite_t::MAGMA_MGM_S):                        result = "MAGMA-CBC-MAGMA-MGM-S";            break;
		case static_cast <uint16_t> (nts::protect::suite_t::SM4_GCM_SM3):                        result = "SM4-GCM-SM3";                      break;
		case static_cast <uint16_t> (nts::protect::suite_t::SM4_CCM_SM3):                        result = "SM4-CCM-SM3";                      break;
	}
	// Выводим название набора
	return result;
}
/**
 * @brief Функция разбора названия набора шифрования из списка настроек
 *
 * Принимаются и названия OpenSSL («ECDHE-RSA-AES256-GCM-SHA384»), и правила IANA
 * («TLS_ECDHE_RSA_WITH_AES_256_GCM_SHA384»), и шестнадцатеричный код («0xC030»). Названия CBC-
 * наборов ведутся двумя написаниями: OpenSSL понимает только каноническое
 * («ECDHE-RSA-AES128-SHA256»), а список настроек прежних сборок держал написание через «CBC»
 * («ECDHE-RSA-AES128-CBC-SHA256») — без обоих строк из конфигурации набор терялся молча
 *
 * @param name  название набора шифрования
 * @param suite место, в которое помещается разобранный набор
 * @return      результат разбора
 *
 */
static bool suiteByName(const string & name, nts::protect::suite_t & suite) noexcept {
	// Набор названий OpenSSL и правил IANA, сопоставленных с наборами A-NTS
	static const pair <const char *, nts::protect::suite_t> names[] = {
		{"ECDHE-ECDSA-AES128-GCM-SHA256",   nts::protect::suite_t::ECDHE_ECDSA_AES_128_GCM_SHA256},
		{"ECDHE-ECDSA-AES256-GCM-SHA384",   nts::protect::suite_t::ECDHE_ECDSA_AES_256_GCM_SHA384},
		{"ECDHE-RSA-AES128-GCM-SHA256",     nts::protect::suite_t::ECDHE_RSA_AES_128_GCM_SHA256},
		{"ECDHE-RSA-AES256-GCM-SHA384",     nts::protect::suite_t::ECDHE_RSA_AES_256_GCM_SHA384},
		{"ECDHE-RSA-CHACHA20-POLY1305",     nts::protect::suite_t::ECDHE_RSA_CHACHA20_POLY1305_SHA256},
		{"ECDHE-ECDSA-CHACHA20-POLY1305",   nts::protect::suite_t::ECDHE_ECDSA_CHACHA20_POLY1305_SHA256},
		{"ECDHE-ECDSA-AES128-CBC-SHA256",   nts::protect::suite_t::ECDHE_ECDSA_AES_128_CBC_SHA256},
		{"ECDHE-RSA-AES128-CBC-SHA256",     nts::protect::suite_t::ECDHE_RSA_AES_128_CBC_SHA256},
		{"ECDHE-ECDSA-AES128-CBC-SHA",      nts::protect::suite_t::ECDHE_ECDSA_AES_128_CBC_SHA},
		{"ECDHE-ECDSA-AES256-CBC-SHA",      nts::protect::suite_t::ECDHE_ECDSA_AES_256_CBC_SHA},
		{"ECDHE-RSA-AES128-CBC-SHA",        nts::protect::suite_t::ECDHE_RSA_AES_128_CBC_SHA},
		{"ECDHE-RSA-AES256-CBC-SHA",        nts::protect::suite_t::ECDHE_RSA_AES_256_CBC_SHA},
		{"ECDHE-ECDSA-AES128-SHA256",       nts::protect::suite_t::ECDHE_ECDSA_AES_128_CBC_SHA256},
		{"ECDHE-RSA-AES128-SHA256",         nts::protect::suite_t::ECDHE_RSA_AES_128_CBC_SHA256},
		{"ECDHE-ECDSA-AES128-SHA",          nts::protect::suite_t::ECDHE_ECDSA_AES_128_CBC_SHA},
		{"ECDHE-ECDSA-AES256-SHA",          nts::protect::suite_t::ECDHE_ECDSA_AES_256_CBC_SHA},
		{"ECDHE-RSA-AES128-SHA",            nts::protect::suite_t::ECDHE_RSA_AES_128_CBC_SHA},
		{"ECDHE-RSA-AES256-SHA",            nts::protect::suite_t::ECDHE_RSA_AES_256_CBC_SHA},
		{"AES128-GCM-SHA256",               nts::protect::suite_t::RSA_AES_128_GCM_SHA256},
		{"AES256-GCM-SHA384",               nts::protect::suite_t::RSA_AES_256_GCM_SHA384},
		{"AES128-SHA",                      nts::protect::suite_t::RSA_AES_128_CBC_SHA},
		{"AES256-SHA",                      nts::protect::suite_t::RSA_AES_256_CBC_SHA},
		{"AES128-SHA256",                   nts::protect::suite_t::RSA_AES_128_CBC_SHA256},
		{"AES256-SHA256",                   nts::protect::suite_t::RSA_AES_256_CBC_SHA256},
		{"TLS_AES_128_GCM_SHA256",          nts::protect::suite_t::AES_128_GCM_SHA256},
		{"TLS_AES_256_GCM_SHA384",          nts::protect::suite_t::AES_256_GCM_SHA384},
		{"TLS_CHACHA20_POLY1305_SHA256",    nts::protect::suite_t::CHACHA20_POLY1305_SHA256},
		{"TLS_ECDHE_ECDSA_WITH_AES_128_GCM_SHA256",   nts::protect::suite_t::ECDHE_ECDSA_AES_128_GCM_SHA256},
		{"TLS_ECDHE_ECDSA_WITH_AES_256_GCM_SHA384",   nts::protect::suite_t::ECDHE_ECDSA_AES_256_GCM_SHA384},
		{"TLS_ECDHE_RSA_WITH_AES_128_GCM_SHA256",     nts::protect::suite_t::ECDHE_RSA_AES_128_GCM_SHA256},
		{"TLS_ECDHE_RSA_WITH_AES_256_GCM_SHA384",     nts::protect::suite_t::ECDHE_RSA_AES_256_GCM_SHA384},
		{"TLS_ECDHE_RSA_WITH_CHACHA20_POLY1305_SHA256", nts::protect::suite_t::ECDHE_RSA_CHACHA20_POLY1305_SHA256},
		{"TLS_ECDHE_ECDSA_WITH_CHACHA20_POLY1305_SHA256", nts::protect::suite_t::ECDHE_ECDSA_CHACHA20_POLY1305_SHA256},
		{"TLS_ECDHE_ECDSA_WITH_AES_128_CBC_SHA256", nts::protect::suite_t::ECDHE_ECDSA_AES_128_CBC_SHA256},
		{"TLS_ECDHE_RSA_WITH_AES_128_CBC_SHA256", nts::protect::suite_t::ECDHE_RSA_AES_128_CBC_SHA256},
		{"TLS_ECDHE_ECDSA_WITH_AES_128_CBC_SHA", nts::protect::suite_t::ECDHE_ECDSA_AES_128_CBC_SHA},
		{"TLS_ECDHE_ECDSA_WITH_AES_256_CBC_SHA", nts::protect::suite_t::ECDHE_ECDSA_AES_256_CBC_SHA},
		{"TLS_ECDHE_RSA_WITH_AES_128_CBC_SHA", nts::protect::suite_t::ECDHE_RSA_AES_128_CBC_SHA},
		{"TLS_ECDHE_RSA_WITH_AES_256_CBC_SHA", nts::protect::suite_t::ECDHE_RSA_AES_256_CBC_SHA},
	};
	// Разбираемое название в верхнем регистре
	string upper = name;
	// Выполняем перевод названия в верхний регистр
	transform(upper.begin(), upper.end(), upper.begin(), ::toupper);
	// Набор для разбора шестнадцатеричного кода
	unsigned long code = 0;
	// Если название похоже на шестнадцатеричный код набора, разбираем его
	if((upper.compare(0, 2, "0X") == 0) && (upper.size() > 2)){
		// Выполняем разбор кода
		code = ::strtoul(upper.c_str(), nullptr, 0);
		// Выводим разобранный набор
		suite = static_cast <nts::protect::suite_t> (code & 0xFFFF);
		// Сообщаем, что набор разобран
		return true;
	}
	/**
	 * Переходим по таблице названий
	 */
	for(const auto & pair : names){
		// Если название совпало, запоминаем набор
		if(upper == pair.first){
			// Выполняем установку найденного набора
			suite = pair.second;
			// Выводим признак того, что набор разобран
			return true;
		}
	}
	// Выводим признак того, что название набора не разобрано
	return false;
}
/**
 * @brief Функция разбора перечня прикладных протоколов из проволочного вида
 *
 * Прошлый вид перечня оставлен публичным свойством Context::protocols: каждый протокол идёт
 * своим длиной и байтами. Для наружного модуля нужен перечень строк
 *
 * @param wire   перечень протоколов проволочным видом
 * @param result место, в которое помещается разобранный перечень
 *
 */
static void protocolList(const vector <uint8_t> & wire, vector <string> & result) noexcept {
	/**
	 * Переход по всему перечню
	 */
	for(size_t offset = 0; offset < wire.size();){
		// Размер текущего названия протокола
		const size_t size = static_cast <size_t> (wire[offset]);
		// Если запись выходит за границы перечня, выходим
		if((offset + 1 + size) > wire.size())
			// Выходим из цикла
			break;
		// Добавляем разобранные названия протокола
		result.push_back(string(reinterpret_cast <const char *> (&wire[offset + 1]), size));
		// Выполняем переход к следующей записи
		offset += (size + 1);
	}
}
/**
 * @brief Функция разбора IP-адреса, переданного в качестве хоста (IPv4 / IPv6)
 *
 * @param host хост удалённого сервера
 * @return       размер бинарного адреса (4 / 16) либо 0, если хост не является IP-адресом
 */
static size_t ipLiteral(const string & host) noexcept {
	// Буфер бинарного адреса
	uint8_t addr[16];
	// Адрес без квадратных скобок IPv6
	const string ip = (((host.size() > 2) && (host.front() == '[') && (host.back() == ']')) ? host.substr(1, host.size() - 2) : host);
	// Если хост является адресом IPv4
	if(::inet_pton(AF_INET, ip.c_str(), addr) == 1)
		// Выводим размер адреса
		return 4;
	// Если хост является адресом IPv6
	if(::inet_pton(AF_INET6, ip.c_str(), addr) == 1)
		// Выводим размер адреса
		return 16;
	// Хост не является IP-адресом
	return 0;
}
/**
 * @brief Функция установки серверных наборов шифрования по умолчанию
 *
 * Набор Mozilla «intermediate»: только ECDHE с AEAD. Прежний список держал ещё и DHE-RSA,
 * но наборов TLS 1.2 с простым DHE у наружного модуля нет — механика конечного поля ffdhe
 * работает лишь как key_share издания 1.3
 *
 * Наборы издания 1.3 ставят первыми: перечень отдаётся от предпочтительного к запасному,
 * и без них сервер, отвечая на modern-приветствие, выбрал бы издание 1.2, тогда как
 * прежний OpenSSL держал 1.3 по умолчанию
 *
 * @param result место, в которое помещается перечень наборов
 *
 */
static void serverSuites(vector <nts::protect::suite_t> & result) noexcept {
	// Выполняем установку наборов TLS 1.3 как предпочтительных
	result.assign({
		nts::protect::suite_t::AES_128_GCM_SHA256,
		nts::protect::suite_t::AES_256_GCM_SHA384,
		nts::protect::suite_t::CHACHA20_POLY1305_SHA256,
		// Выполняем установку наборов TLS 1.2 с ECDHE и AEAD как запасных
		nts::protect::suite_t::ECDHE_ECDSA_AES_128_GCM_SHA256,
		nts::protect::suite_t::ECDHE_RSA_AES_128_GCM_SHA256,
		nts::protect::suite_t::ECDHE_ECDSA_AES_256_GCM_SHA384,
		nts::protect::suite_t::ECDHE_RSA_AES_256_GCM_SHA384,
		nts::protect::suite_t::ECDHE_ECDSA_CHACHA20_POLY1305_SHA256,
		nts::protect::suite_t::ECDHE_RSA_CHACHA20_POLY1305_SHA256
	});
}
/**
 * @brief Функция разбора перечня наборов шифрования из строки настроек
 *
 * Прежний `SSL_CTX_set_cipher_list()` касался только TLS 1.2, наборы TLS 1.3 задавала сама
 * библиотека. Чтобы настройка не молча отрезала издание 1.3, наборы TLS 1.3 дополняем, если
 * в списке не назван ни один из них
 *
 * @param ciphers строка настроек списка алгоритмов шифрования
 * @param result  место, в которое помещается разобранный перечень наборов
 * @param log     объект работы с логами
 */
static void suitesList(const string & ciphers, vector <nts::protect::suite_t> & result, const awh::log_t * log) noexcept {
	// Если список настроек не задан, наружный модуль берёт свои наборы по умолчанию
	if(ciphers.empty())
		// Выходим из функции
		return;
	// Разбираемое название набора
	string name = "";
	// Признак того, что в списке назван набор TLS 1.3
	bool modern = false;
	/**
	 * Переход по строке настроек, разделяя список двоеточием
	 */
	for(size_t begin = 0; begin <= ciphers.size();){
		// Позиция следующего разделителя
		const size_t pos = ciphers.find(':', begin);
		// Выполняем извлечение названия набора
		name = ciphers.substr(begin, ((pos == string::npos) ? string::npos : pos - begin));
		// Разобранный набор шифрования
		nts::protect::suite_t suite = nts::protect::suite_t::NONE;
		// Если название набора разобрано, добавляем его в перечень
		if(!name.empty() && suiteByName(name, suite)){
			// Если набор принадлежит изданию TLS 1.3, запоминаем это
			if((static_cast <uint16_t> (suite) >= 0x1301) && (static_cast <uint16_t> (suite) <= 0x1303))
				// Запоминаем, что издание 1.3 в списке назван
				modern = true;
			// Добавляем разобранный набор
			result.push_back(suite);
		// Если название не разобралось, выводим сообщение: молча терять заказ нельзя
		} else if(!name.empty()) log->print("Unknown cipher is skipped: %s", awh::log_t::flag_t::WARNING, name.c_str());
		// Если разделителей больше нет, выходим
		if(pos == string::npos)
			// Выходим из цикла
			break;
		// Выполняем переход к следующему названию
		begin = pos + 1;
	}
	/**
	 * Дополняем наборы TLS 1.3, если в списке не назван ни один: прежний OpenSSL список
	 * cipher_list к изданию 1.3 не применял, и без дополнения связь с modern-клиентом
	 * перестала бы заводиться
	 */
	if(!modern){
		// Добавляем наборы TLS 1.3 по умолчанию
		result.push_back(nts::protect::suite_t::AES_128_GCM_SHA256);
		// Добавляем следующий набор издания 1.3
		result.push_back(nts::protect::suite_t::AES_256_GCM_SHA384);
		// Добавляем последний набор издания 1.3
		result.push_back(nts::protect::suite_t::CHACHA20_POLY1305_SHA256);
	}
}
/**
 * @brief Функция получения перечня групп обмена ключами по умолчанию
 *
 * X25519 предлагают все современные клиенты, без неё рукопожатие шло только на P-256.
 * Гибридная постквантовая группа X25519MLKEM768 идёт первой, как это делал OpenSSL 3.5
 *
 * @param result место, в которое помещается перечень групп
 */
static void groupList(vector <nts::extension::group_t> & result) noexcept {
	// Выполняем установку перечня групп в порядке предпочтения
	result.assign({
		nts::extension::group_t::X25519MLKEM768,
		nts::extension::group_t::X25519,
		nts::extension::group_t::SECP256R1,
		nts::extension::group_t::SECP384R1,
		nts::extension::group_t::SECP521R1
	});
}
/**
 * @brief Функция сборки тела сообщения рукопожатия из принятых байтов канала
 *
 * Раздел 5.1 RFC 8446: сообщения рукопожатия лежат внутри записей, а одно сообщение может
 * занять несколько записей. Собираем тела записей рукопожатия, пока не придёт первое
 * сообщение целиком
 *
 * @param bytes  принятые байты канала
 * @param result место, в которое помещается тело сообщения без его обвязки
 * @return       результат сборки (false - байтов пока не хватает)
 */
static bool helloBody(const vector <uint8_t> & bytes, vector <uint8_t> & result) noexcept {
	// Тела записей рукопожатия, собранные в одно сообщение
	vector <uint8_t> message;
	/**
	 * Переход по записям протокола
	 */
	for(size_t offset = 0; (offset + RECORD_HEADER_SIZE) <= bytes.size();){
		// Размер тела текущей записи протокола
		const size_t size = ((static_cast <size_t> (bytes[offset + 3]) << 8) | static_cast <size_t> (bytes[offset + 4]));
		// Если тело записи ещё не пришло целиком, выходим
		if((offset + RECORD_HEADER_SIZE + size) > bytes.size())
			// Выводим из цикла
			break;
		/**
		 * Тип записи 0x16 — сообщения рукопожатия (раздел 5.1 RFC 8446); прочие записи
		 * здесь не встречаются, но и рвать из-за них разбор не станем
		 */
		if(bytes[offset] == RECORD_TYPE_HANDSHAKE){
			// Добавляем тело записи к собираемому сообщению
			message.insert(message.end(), &bytes[offset + RECORD_HEADER_SIZE], &bytes[offset + RECORD_HEADER_SIZE + size]);
			// Если обвязки сообщения ещё нет, ждём продолжения
			if(message.size() >= MESSAGE_HEADER_SIZE){
				// Размер тела сообщения по его обвязке
				const size_t total = ((static_cast <size_t> (message[1]) << 16) | (static_cast <size_t> (message[2]) << 8) | static_cast <size_t> (message[3]));
				// Если тело сообщения собрано, забираем его без обвязки и выходим
				if(message.size() >= (MESSAGE_HEADER_SIZE + total)){
					// Выполняем копирование собранного тела сообщения
					result.assign(message.begin() + MESSAGE_HEADER_SIZE, message.begin() + MESSAGE_HEADER_SIZE + total);
					// Выходим из цикла
					break;
				}
			}
		}
		// Выполняем переход к следующей записи
		offset += (RECORD_HEADER_SIZE + size);
	}
	// Выводим результат сборки
	return !result.empty();
}
/**
 * @brief Функция разбора имени узла, запрошенного клиентом, из байтов приветствия
 *
 * Раздел 3 RFC 6066: имя лежит в расширении 0 приветствия клиента и при TLS 1.2, и при TLS 1.3
 * открытым текстом. Наружное имя под ECH (раздел 4 RFC 9849) разбирается так же, внутреннее —
 * только библиотеке
 *
 * @param body тело сообщения приветствия клиента без его обвязки
 * @param name место, в которое помещается разобранное имя
 * @return     результат разбора
 */
static bool helloHostname(const vector <uint8_t> & body, string & name) noexcept {
	// Разобранные параметры приветствия клиента
	nts::hello::client_t hello;
	// Если приветствие не разобралось, выходим
	if(nts::hello::parse(nts::view_t(body.data(), body.size()), hello) != nts::hello::error_t::NONE)
		// Выводим признак неудачи
		return false;
	// Расширение server_name имеет тип 0 (раздел 4.3.1 RFC 6066)
	const nts::hello::extension_t * ext = nts::hello::extension(hello.extensions, 0);
	// Если расширение не предъявлено либо оно короткое, выходим
	if((ext == nullptr) || (ext->data.size < 5))
		// Выводим признак неудачи
		return false;
	// Байты тела расширения
	const uint8_t * data = ext->data.data;
	// Если имя предъявлено не как узел (тип имени 0), выходим
	if(data[2] != 0)
		// Выводим признак неудачи
		return false;
	// Размер имени узла
	const size_t size = ((static_cast <size_t> (data[3]) << 8) | static_cast <size_t> (data[4]));
	// Если имя выходит за границы расширения, выходим
	if((size == 0) || (5 + size) > ext->data.size)
		// Выводим признак неудачи
		return false;
	// Выполняем установку разобранного имени
	name.assign(reinterpret_cast <const char *> (data + 5), size);
	// Выводим признак того, что имя разобрано
	return true;
}
/**
 * @brief Класс состояния связи внешнего модуля шифрования
 *
 * Оба фасада A-NTS живут рядом: роль связи известна лишь при обёртывании сокета, а сами фасады
 * не копируются и весят по указателю на своё ядро
 *
 */
class awh::Engine::Context::Tls {
	public:
		// Флаг того, что рукопожатие прервано: подлинность по имени клиента не годится
		bool fatal;
		// Флаг проверки соответствия сертификата собеседника предъявленному имени
		bool verify;
		// Флаг того, что сверка имени сертификата выполнена
		bool checked;
		// Флаг отправки приветствия своей стороной связи
		bool started;
		// Флаг того, что приветствие клиента разобрано и подлинность выбрана
		bool sniffed;
		// Запас доверенных сертификатов
		nts::trust::store_t store;
		// Связь стороны клиента
		nts::client::context_t client;
		// Связь стороны сервера
		nts::server::context_t server;
		// Хост удалённого сервера, за которым пришли (для SNI и сверки имени сертификата)
		string host;
		// Непроданные собеседнику байты открытого текста
		vector <uint8_t> plain;
		// Байты канала, разобранные до подачи их связи (подбор подлинности по SNI)
		vector <uint8_t> peek;
		// Движок, которому принадлежит связь: по нему выбираем подлинность для имени клиента
		awh::Engine * engine;
		/**
		 * @brief Конструктор
		 *
		 */
		Tls() noexcept : 
		 fatal(false), verify(false), checked(false),
		 started(false), sniffed(false), engine(nullptr) {}
};
/**
 * @brief Класс запаса подготовленных подлинностей
 *
 * Подготовка ключа RSA стоит вдвое дороже самой подписи, а ключа ECDSA — вдвое дороже всего
 * рукопожатия, поэтому подготовленный ключ разделяют все связи одной подлинности
 *
 */
class awh::Engine::Keys {
	public:
		// Подготовленные подлинности, ключом запаса служит адрес файла цепочки вместе с адресом ключа
		unordered_map <string, nts::context::credential_t> held;
};

/**
 * @brief Метод закрытия подключения
 *
 * @return результат выполнения операции
 */
bool awh::Engine::Address::close() noexcept {
	// Результат работы функции
	bool result = false;
	// Если сетевой сокет подключён
	if((result = (this->sock != INVALID_SOCKET))){
		/**
		 * Для операционной системы MS Windows
		 */
		#if _WIN32 || _WIN64
			// Если тип сокета не диграммы
			if(this->_type != SOCK_DGRAM)
				// Запрещаем работу с сокетом
				::shutdown(this->sock, SD_BOTH);
			// Выполняем закрытие сокета
			::closesocket(this->sock);
		/**
		 * Для операционной системы не являющейся MS Windows
		 */
		#else
			// Если тип сокета не диграммы
			if(this->_type != SOCK_DGRAM)
				// Запрещаем работу с сокетом
				::shutdown(this->sock, SHUT_RDWR);
			// Выполняем закрытие сокета
			::close(this->sock);
		#endif
		// Выполняем сброс сетевого сокета
		this->sock = INVALID_SOCKET;
	}
	// Выводим результат
	return result;
}
/**
 * @brief Метод извлечения данных клиента
 *
 */
void awh::Engine::Address::client() noexcept {
	/**
	 * Определяем тип подключения
	 */
	switch(this->_peer.client.ss_family){
		// Для протокола IPv4
		case AF_INET:
		// Для протокола IPv6
		case AF_INET6: {
			// Буфер для получения IP-адреса
			char buffer[INET6_ADDRSTRLEN];
			// Выполняем зануление буфера данных
			::memset(buffer, 0, sizeof(buffer));
			/**
			 * Определяем тип протокола интернета
			 */
			switch(this->_peer.client.ss_family){
				// Если протокол интернета IPv4
				case AF_INET: {
					// Получаем порт клиента
					this->port = ntohs(reinterpret_cast <struct sockaddr_in *> (&this->_peer.client)->sin_port);
					// Получаем IP-адрес
					this->ip = ::inet_ntop(AF_INET, &(reinterpret_cast <struct sockaddr_in *> (&this->_peer.client)->sin_addr), buffer, sizeof(buffer));
				} break;
				// Если протокол интернета IPv6
				case AF_INET6: {
					// Получаем порт клиента
					this->port = ntohs(reinterpret_cast <struct sockaddr_in6 *> (&this->_peer.client)->sin6_port);
					// Получаем IP-адрес
					this->ip = ::inet_ntop(AF_INET6, &(reinterpret_cast <struct sockaddr_in6 *> (&this->_peer.client)->sin6_addr), buffer, sizeof(buffer));
				} break;
			}
			// Получаем данные подключившегося клиента
			string ip = this->_ifnet.ip(reinterpret_cast <struct sockaddr *> (&this->_peer.client), this->_peer.client.ss_family);
			// Если IP-адрес получен пустой
			if((ip.compare("0.0.0.0") == 0) || (ip.compare("::") == 0)){
				// Получаем IP-адрес локального сервера
				ip = this->_ifnet.ip(this->_peer.client.ss_family);
				// Получаем данные MAC адреса внутреннего клиента
				this->mac = this->_ifnet.mac(ip, this->_peer.client.ss_family);
			// Получаем данные MAC адреса внешнего клиента
			} else this->mac = this->_ifnet.mac(this->ip, this->_peer.client.ss_family);
		} break;
		/**
		 * Для операционной системы не являющейся MS Windows
		 */
		#if !_WIN32 && !_WIN64
			// Для протокола unix-сокета
			case AF_UNIX: {
				// Устанавливаем адрес сервера
				this->ip = this->_ifnet.ip(AF_INET);
				// Получаем данные мак адреса клиента
				this->mac = this->_ifnet.mac(this->ip, AF_INET);
			} break;
		#endif
	}
}
/**
 * @brief Метод активации прослушивания сокета
 *
 * @return результат выполнения операции
 */
bool awh::Engine::Address::list() noexcept {
	// Результат работы функции
	bool result = false;
	/**
	 * Определяем тип сокета
	 */
	switch(this->_type){
		// Если сокет установлен TCP/IP
		case SOCK_STREAM: {
			// Выполняем слушать порт сервера
			if(!(result = (::listen(this->sock, SOMAXCONN) == 0))){
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Выводим сообщение об ошибке
					this->_log->debug("%s", __PRETTY_FUNCTION__, {}, log_t::flag_t::CRITICAL, this->_socket.message(AWH_ERROR()).c_str());
				/**
				* Если режим отладки не включён
				*/
				#else
					// Выводим сообщение об ошибке
					this->_log->print("%s", log_t::flag_t::CRITICAL, this->_socket.message(AWH_ERROR()).c_str());
				#endif
				// Выходим из функции
				return result;
			}
			/**
			 * Для операционной системы Linux или FreeBSD
			 */
			#if __linux__ || __FreeBSD__
				// Если протокол интернета установлен как SCTP
				if(this->_protocol == IPPROTO_SCTP){
					// Выполняем инициализацию SCTP протокола
					this->_socket.events(this->sock);
				}
			#endif
		} break;
		// Если сокет установлен UDP
		case SOCK_DGRAM: return true;
	}
	// Выводим результат
	return result;
}
/**
 * @brief Метод очистки параметров подключения
 *
 * @return результат выполнения операции
 */
bool awh::Engine::Address::clear() noexcept {
	// Результат работы функции
	bool result = false;
	// Сбрасываем флаг инициализации
	this->_encrypted = false;
	// Зануляем структуру клиента
	::memset(&this->_peer.client, 0, sizeof(this->_peer.client));
	// Зануляем структуру сервера
	::memset(&this->_peer.server, 0, sizeof(this->_peer.server));
	// Закрываем основной сокет
	return this->close();
}
/**
 * @brief Метод выполнения подключения
 *
 * @return результат выполнения операции
 */
bool awh::Engine::Address::connect() noexcept {
	// Устанавливаем статус отключения
	this->status = status_t::DISCONNECTED;
	// Если сокет установлен TCP/IP
	if(this->_type == SOCK_STREAM){
		/**
		 * Определяем тип подключения
		 */
		switch(this->_peer.server.ss_family){
			// Для протокола IPv4
			case AF_INET:
				// Запоминаем размер структуры
				this->_peer.size = sizeof(struct sockaddr_in);
			break;
			// Для протокола IPv6
			case AF_INET6:
				// Запоминаем размер структуры
				this->_peer.size = sizeof(struct sockaddr_in6);
			break;
			/**
			 * Для операционной системы не являющейся MS Windows
			 */
			#if !_WIN32 && !_WIN64
				// Для протокола unix-сокета
				case AF_UNIX:
					// Получаем размер объекта сокета
					this->_peer.size = (
						offsetof(struct sockaddr_un, sun_path) +
						strlen(reinterpret_cast <struct sockaddr_un *> (&this->_peer.server)->sun_path)
					);
				break;
			#endif
		}
		/**
		 * Для операционной системы Linux или FreeBSD
		 */
		#if __linux__ || __FreeBSD__
			// Если протокол интернета установлен как SCTP
			if(this->_protocol == IPPROTO_SCTP)
				// Выполняем инициализацию SCTP протокола
				this->_socket.events(this->sock);
		#endif
		// Если размер структуры адреса получен
		if(this->_peer.size > 0){
			// Выполняем подключение к удаленному серверу
			if(::connect(this->sock, reinterpret_cast <struct sockaddr *> (&this->_peer.server), this->_peer.size) == 0)
				// Устанавливаем статус подключения
				this->status = status_t::CONNECTED;
			/**
			 * Неблокирующее подключение ещё выполняется: считаем его начатым, итог
			 * подключения вызывающий получает по готовности сокета к записи из SO_ERROR
			 */
			else {
				/**
				 * Для операционной системы MS Windows
				 */
				#if _WIN32 || _WIN64
					// Если подключение выполняется в фоне
					if(AWH_ERROR() == WSAEWOULDBLOCK)
						// Устанавливаем статус подключения
						this->status = status_t::CONNECTED;
				/**
				 * Для операционной системы не являющейся MS Windows
				 */
				#else
					// Если подключение выполняется в фоне
					if(AWH_ERROR() == EINPROGRESS)
						// Устанавливаем статус подключения
						this->status = status_t::CONNECTED;
				#endif
			}
		}
	// Если сокет установлен UDP
	} else if(this->_type == SOCK_DGRAM) {
		/**
		 * Сокет DTLS поверх unix-сокета не подключаем (connect): сервер отвечает не со
		 * слушающего сокета, а с отдельного сокета клиента по своему пути (два сокета не
		 * могут занять один путь), и неподключённый BIO датаграмм переключает адрес
		 * назначения на источник полученных датаграмм. Подключённый сокет продолжал бы
		 * слать на слушающий сокет сервера, а в Linux и не принимал бы ответы с другого пути.
		 */
		if(this->_encrypted && (this->_peer.server.ss_family == AF_UNIX))
			// Устанавливаем статус подключения
			this->status = status_t::CONNECTED;
		// Если подключение зашифрованно, значит мы должны использовать DTLS
		else if(this->_encrypted){
			/**
			 * Определяем тип подключения
			 */
			switch(this->_peer.server.ss_family){
				// Для протокола IPv4
				case AF_INET:
					// Запоминаем размер структуры
					this->_peer.size = sizeof(struct sockaddr_in);
				break;
				// Для протокола IPv6
				case AF_INET6:
					// Запоминаем размер структуры
					this->_peer.size = sizeof(struct sockaddr_in6);
				break;
			}
			// Выполняем подключение к удаленному серверу
			if((this->_peer.size > 0) && (::connect(this->sock, reinterpret_cast <struct sockaddr *> (&this->_peer.server), this->_peer.size) == 0))
				// Устанавливаем статус подключения
				this->status = status_t::CONNECTED;
		// Устанавливаем статус подключения
		} else this->status = status_t::CONNECTED;
	}
	// Если подключение выполнено
	if(this->status == status_t::CONNECTED)
		// Выполняем извлечение данных клиента
		this->client();
	// Выводим результат
	return (this->status == status_t::CONNECTED);
}
/**
 * @brief Метод извлечения хоста компьютера
 *
 * @param family семейство сокета (AF_INET / AF_INET6)
 * @return       хост компьютера с которого производится запрос
 */
string awh::Engine::Address::host(const int32_t family) const noexcept {
	// Результат работы функции
	string result = "";
	// Если список сетей установлен
	if(!this->network.empty()){
		/**
		 * Выполняем отлов ошибок
		 */
		try {
			// Если количество элементов больше 1
			if(this->network.size() > 1){
				// Подключаем устройство генератора
				mt19937 random(const_cast <addr_t *> (this)->_randev());
				// Выполняем генерирование случайного числа
				uniform_int_distribution <mt19937::result_type> dist6(0, this->network.size() - 1);
				// Получаем IP-адрес
				result = this->network.at(dist6(random));
			// Выводим только первый элемент
			} else result = this->network.front();
		/**
		 * Если возникает ошибка
		 */
		} catch(const exception & error) {
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Выводим сообщение об ошибке
				this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(family), log_t::flag_t::WARNING, error.what());
			/**
			* Если режим отладки не включён
			*/
			#else
				// Выводим сообщение об ошибке
				this->_log->print("%s", log_t::flag_t::WARNING, error.what());
			#endif
			// Выводим только первый элемент
			result = this->network.front();
		}
	}
	// Если IP-адрес не установлен
	if(result.empty()){
		/**
		 * Определяем тип подключения
		 */
		switch(family){
			// Для протокола IPv6
			case AF_INET6: return "::";
			// Для протокола IPv4
			case AF_INET: return "0.0.0.0";
		}
	}
	// Выводим результат
	return result;
}
/**
 * @brief Метод прикрепления клиента к серверу
 *
 * @param addr объект подключения сервера
 */
bool awh::Engine::Address::attach(Address & addr) noexcept {
	// Устанавливаем статус отключения
	this->status = status_t::DISCONNECTED;
	/**
	 * Определяем тип подключения
	 */
	switch(addr._peer.server.ss_family){
		// Для протокола IPv4
		case AF_INET:
			// Запоминаем размер структуры
			this->_peer.size = sizeof(struct sockaddr_in);
		break;
		// Для протокола IPv6
		case AF_INET6:
			// Запоминаем размер структуры
			this->_peer.size = sizeof(struct sockaddr_in6);
		break;
		/**
		 * Для операционной системы не являющейся MS Windows
		 */
		#if !_WIN32 && !_WIN64
			// Для протокола unix-сокета
			case AF_UNIX:
				// Запоминаем размер структуры
				this->_peer.size = sizeof(struct sockaddr_un);
			break;
		#endif
	}
	// Создаем сокет подключения
	this->sock = ::socket(addr._peer.server.ss_family, addr._type, 0);
	// Устанавливаем разрешение на повторное использование сокета
	this->_socket.reuseable(this->sock);
	// Выполняем копирование объекта подключения клиента
	::memcpy(&this->_peer.client, &addr._peer.client, sizeof(struct sockaddr_storage));
	// Выполняем копирование объекта подключения сервера
	::memcpy(&this->_peer.server, &addr._peer.server, sizeof(struct sockaddr_storage));
	/**
	 * Для операционной системы не являющейся MS Windows
	 */
	#if !_WIN32 && !_WIN64
		/**
		 * Путь unix-сокета не может занять второй сокет (SO_REUSEPORT для него не
		 * действует), поэтому сокет клиента сервера DTLS привязываем к собственному
		 * пути «<путь сервера>_<сокет>». Клиент узнаёт этот путь из первой полученной
		 * датаграммы ответа. Файл пути удаляет сервер при закрытии подключения.
		 */
		if((this->_peer.server.ss_family == AF_UNIX) && (this->sock != INVALID_SOCKET)){
			// Получаем объект адреса сервера
			struct sockaddr_un * server = reinterpret_cast <struct sockaddr_un *> (&this->_peer.server);
			// Формируем путь сокета клиента на сервере
			const string & path = this->_fmk->format("%s_%d", server->sun_path, this->sock);
			// Если путь не помещается в адрес unix-сокета
			if(path.size() >= sizeof(server->sun_path)){
				// Выводим сообщение об ошибке
				this->_log->print("Unix socket path \"%s\" is too long", log_t::flag_t::CRITICAL, path.c_str());
				// Выходим
				return false;
			}
			// Если сокет в файловой системе уже существует, удаляем его
			if(this->_fs.isSock(path))
				// Удаляем файл сокета
				::unlink(path.c_str());
			// Очищаем путь сокета сервера
			::memset(server->sun_path, 0, sizeof(server->sun_path));
			// Копируем путь сокета клиента на сервере
			::strncpy(server->sun_path, path.c_str(), sizeof(server->sun_path) - 1);
		}
	#endif
	// Выполняем бинд на сокет
	if((this->_peer.size > 0) && (::bind(this->sock, reinterpret_cast <struct sockaddr *> (&this->_peer.server), this->_peer.size) < 0)){
		// Получаем код ошибки
		const int32_t error = AWH_ERROR();
		// Хост подключённого клиента
		const string & client = this->_ifnet.ip(reinterpret_cast <struct sockaddr *> (&this->_peer.client), this->_peer.client.ss_family);
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Выводим сообщение об ошибке
			this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(client), log_t::flag_t::CRITICAL, this->_socket.message(error).c_str());
		/**
		* Если режим отладки не включён
		*/
		#else
			// Выводим сообщение об ошибке
			this->_log->print("%s", log_t::flag_t::CRITICAL, this->_socket.message(error).c_str());
		#endif
		// Выходим
		return false;
	}
	// Если подключение не выполненно то сообщаем об этом, выполняем подключение к удаленному серверу
	if(::connect(this->sock, reinterpret_cast <struct sockaddr *> (&this->_peer.client), this->_peer.size) == 0){
		// Выполняем извлечение данных клиента
		this->client();
		// Устанавливаем статус подключения
		this->status = status_t::ATTACHED;
	}
	// Выводим результат
	return (this->status == status_t::ATTACHED);
}
/**
 * @brief Метод согласования подключения
 *
 * @param addr объект подключения сервера
 * @return     результат выполнения операции
 */
bool awh::Engine::Address::accept(Address & addr) noexcept {
	// Выполняем вызов метода согласование
	return this->accept(addr.sock, addr._peer.server.ss_family);
}
/**
 * @brief Метод согласования подключения
 *
 * @param sock   сетевой сокет сервера
 * @param family семейство сокета (AF_INET / AF_INET6 / AF_UNIX)
 * @return       результат выполнения операции
 */
bool awh::Engine::Address::accept(const SOCKET sock, const int32_t family) noexcept {
	// Устанавливаем статус отключения
	this->status = status_t::DISCONNECTED;
	// Заполняем структуру клиента нулями
	::memset(&this->_peer.client, 0, sizeof(this->_peer.client));
	/**
	 * Определяем тип сокета
	 */
	switch(this->_type){
		// Если сокет установлен TCP/IP
		case SOCK_STREAM: {
			/**
			 * Определяем тип подключения
			 */
			switch(family){
				// Для протокола IPv4
				case AF_INET: {
					// Создаём объект клиента
					struct sockaddr_in client;
					// Очищаем всю структуру для клиента
					::memset(&client, 0, sizeof(client));
					// Устанавливаем протокол интернета
					client.sin_family = family;
					// Запоминаем размер структуры
					this->_peer.size = sizeof(client);
					// Выполняем копирование объекта подключения клиента
					::memcpy(&this->_peer.client, &client, this->_peer.size);
				} break;
				// Для протокола IPv6
				case AF_INET6: {
					// Создаём объект клиента
					struct sockaddr_in6 client;
					// Очищаем всю структуру для клиента
					::memset(&client, 0, sizeof(client));
					// Устанавливаем протокол интернета
					client.sin6_family = family;
					// Запоминаем размер структуры
					this->_peer.size = sizeof(client);
					// Выполняем копирование объекта подключения клиента
					::memcpy(&this->_peer.client, &client, this->_peer.size);
				} break;
				/**
				 * Для операционной системы не являющейся MS Windows
				 */
				#if !_WIN32 && !_WIN64
					// Для протокола unix-сокета
					case AF_UNIX: {
						// Создаём объект подключения для клиента
						struct sockaddr_un client;
						// Очищаем всю структуру для клиента
						::memset(&client, 0, sizeof(client));
						// Устанавливаем протокол интернета
						client.sun_family = family;
						// Запоминаем размер структуры
						this->_peer.size = sizeof(client);
						// Выполняем копирование объект подключения клиента в сторейдж
						::memcpy(&this->_peer.client, &client, sizeof(client));
					} break;
				#endif
			}
			// Определяем разрешено ли подключение к прокси серверу
			this->sock = ::accept(sock, reinterpret_cast <struct sockaddr *> (&this->_peer.client), &this->_peer.size);
			// Если сокет не создан тогда выходим
			if(this->sock == INVALID_SOCKET)
				// Выходим из функции
				return false;
		} break;
		// Если сокет установлен UDP
		case SOCK_DGRAM: this->sock = sock; break;
	}
	/**
	 * Определяем тип подключения
	 */
	switch(this->_peer.client.ss_family){
		// Для протокола IPv4
		case AF_INET:
		// Для протокола IPv6
		case AF_INET6: {
			/**
			 * Для операционной системы не являющейся MS Windows
			 */
			#if !_WIN32 && !_WIN64
				// Выполняем игнорирование сигнала неверной инструкции процессора
				this->_socket.noSigILL();
				// Если сокет установлен TCP/IP
				if(this->_type == SOCK_STREAM){
					// Отключаем сигнал записи в оборванное подключение
					this->_socket.noSigPIPE(this->sock);
					/**
					 * Для операционной системы Linux или FreeBSD
					 */
					#if __linux__ || __FreeBSD__
						// Если протокол интернета установлен как SCTP
						if(this->_protocol != IPPROTO_SCTP)
							// Активируем KeepAlive
							this->_socket.keepAlive(this->sock, this->alive.cnt, this->alive.idle, this->alive.intvl);
					/**
					 * Для операционной системы Linux
					 */
					#else
						// Активируем KeepAlive
						this->_socket.keepAlive(this->sock, this->alive.cnt, this->alive.idle, this->alive.intvl);
					#endif
				}
			/**
			 * Для операционной системы MS Windows
			 */
			#else
				// Если сокет установлен TCP/IP
				if(this->_type == SOCK_STREAM)
					// Активируем KeepAlive
					this->_socket.keepAlive(this->sock);
			#endif
			// Если сокет установлен TCP/IP
			if(this->_type == SOCK_STREAM){
				// Устанавливаем разрешение на повторное использование сокета
				this->_socket.reuseable(this->sock);
				// Переводим сокет в не блокирующий режим
				this->_socket.blocking(this->sock, socket_t::mode_t::DISABLED);
			}
		} break;
		/**
		 * Для операционной системы не являющейся MS Windows
		 */
		#if !_WIN32 && !_WIN64
			// Для протокола unix-сокета
			case AF_UNIX: {
				// Выполняем игнорирование сигнала неверной инструкции процессора
				this->_socket.noSigILL();
				// Если сокет установлен TCP/IP
				if(this->_type == SOCK_STREAM){
					// Отключаем сигнал записи в оборванное подключение
					this->_socket.noSigPIPE(this->sock);
					// Устанавливаем разрешение на повторное использование сокета
					this->_socket.reuseable(this->sock);
					// Переводим сокет в не блокирующий режим
					this->_socket.blocking(this->sock, socket_t::mode_t::DISABLED);
				}
			} break;
		#endif
	}
	// Выполняем извлечение данных клиента
	this->client();
	// Устанавливаем статус подключения
	this->status = status_t::ACCEPTED;
	// Выводим результат
	return true;
}
/**
 * @brief Метод установки параметров сокета
 *
 * @param type     тип сокета (SOCK_STREAM / SOCK_DGRAM)
 * @param protocol протокол сокета (IPPROTO_TCP / IPPROTO_UDP / IPPROTO_SCTP)
 */
void awh::Engine::Address::sonet(const int32_t type, const int32_t protocol) noexcept {
	// Устанавливаем тип сокета
	this->_type = type;
	// Устанавливаем протокол сокета
	this->_protocol = protocol;
}
/**
 * @brief Метод инициализации адресного пространства сокета
 *
 * @param unixsocket адрес unxi-сокета в файловой системе
 * @param type       тип приложения (клиент или сервер)
 */
void awh::Engine::Address::init(const string & unixsocket, const type_t type) noexcept {
	// Если unix-сокет передан
	if(!unixsocket.empty()){
		/**
		 * Для операционной системы не являющейся MS Windows
		 */
		#if !_WIN32 && !_WIN64
			// Создаем сокет подключения
			this->sock = ::socket(AF_UNIX, this->_type, 0);
			// Если сокет не создан то выходим
			if(this->sock == INVALID_SOCKET){
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Выводим сообщение об ошибке
					this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(unixsocket, static_cast <uint16_t> (type)), log_t::flag_t::CRITICAL, this->_socket.message(AWH_ERROR()).c_str());
				/**
				* Если режим отладки не включён
				*/
				#else
					// Выводим сообщение об ошибке
					this->_log->print("%s", log_t::flag_t::CRITICAL, this->_socket.message(AWH_ERROR()).c_str());
				#endif
				// Выходим
				return;
			}
			// Выполняем игнорирование сигнала неверной инструкции процессора
			this->_socket.noSigILL();
			// Устанавливаем разрешение на повторное использование сокета
			this->_socket.reuseable(this->sock);
			// Если сокет установлен TCP/IP
			if(this->_type == SOCK_STREAM)
				// Отключаем сигнал записи в оборванное подключение
				this->_socket.noSigPIPE(this->sock);
			// Если приложение является сервером
			if(type == type_t::SERVER)
				// Переводим сокет в не блокирующий режим
				this->_socket.blocking(this->sock, socket_t::mode_t::DISABLED);
			// Создаём объект подключения для клиента
			struct sockaddr_un client;
			// Создаём объект подключения для сервера
			struct sockaddr_un server;
			// Зануляем изначальную структуру данных клиента
			::memset(&client, 0, sizeof(client));
			// Зануляем изначальную структуру данных сервера
			::memset(&server, 0, sizeof(server));
			// Устанавливаем размер объекта подключения
			this->_peer.size = sizeof(struct sockaddr_un);
			/**
			 * Определяем тип сокета
			 */
			switch(this->_type){
				// Если сокет установлен TCP/IP
				case SOCK_STREAM: {
					// Устанавливаем протокол интернета
					server.sun_family = AF_UNIX;
					// Очищаем всю структуру для сервера
					::memset(&server.sun_path, 0, sizeof(server.sun_path));
					// Если приложение является сервером
					if(type == type_t::SERVER){
						// Если сокет в файловой системе уже существует, удаляем его
						if(this->_fs.isSock(unixsocket))
							// Удаляем файл сокета
							::unlink(unixsocket.c_str());
					}
					// Копируем адрес сокета сервера
					::strncpy(server.sun_path, unixsocket.c_str(), sizeof(server.sun_path));
					// Выполняем копирование объект подключения сервера в сторейдж
					::memcpy(&this->_peer.server, &server, sizeof(server));
				} break;
				// Если сокет установлен UDP
				case SOCK_DGRAM: {
					// Устанавливаем протокол интернета для клиента
					client.sun_family = AF_UNIX;
					// Устанавливаем протокол интернета для сервера
					server.sun_family = AF_UNIX;
					// Очищаем всю структуру для клиента
					::memset(&client.sun_path, 0, sizeof(client.sun_path));
					// Очищаем всю структуру для сервера
					::memset(&server.sun_path, 0, sizeof(server.sun_path));
					// Выполняем поиск последнего слеша разделителя
					const size_t pos = unixsocket.rfind("/");
					// Если разделитель найден
					if(pos != string::npos){
						// Получаем название файла unix-сокета
						const string name = unixsocket.substr(pos + 1);
						// Получаем путь к файлу unix-сокета
						const string path = unixsocket.substr(0, pos + 1);
						// Создаём адрес unix-сокета клиента
						const string & clientName = this->_fmk->format("%sc_%s", path.c_str(), name.c_str());
						// Создаём адрес unix-сокета сервера
						const string & serverName = this->_fmk->format("%ss_%s", path.c_str(), name.c_str());
						/**
						 * Определяем тип приложения
						 */
						switch(static_cast <uint8_t> (type)){
							// Если приложение является клиентом
							case static_cast <uint8_t> (type_t::CLIENT): {
								// Если сокет в файловой системе уже существует, удаляем его
								if(this->_fs.isSock(clientName))
									// Удаляем файл сокета
									::unlink(clientName.c_str());
							} break;
							// Если приложение является сервером
							case static_cast <uint8_t> (type_t::SERVER): {
								// Если сокет в файловой системе уже существует, удаляем его
								if(this->_fs.isSock(serverName))
									// Удаляем файл сокета
									::unlink(serverName.c_str());
							} break;
						}
						// Копируем адрес сокета сервера
						::strncpy(server.sun_path, serverName.c_str(), sizeof(server.sun_path));
						// Выполняем копирование объект подключения сервера в сторейдж
						::memcpy(&this->_peer.server, &server, sizeof(server));
						// Если приложение является клиентом
						if(type == type_t::CLIENT){
							// Копируем адрес сокета клиента
							::strncpy(client.sun_path, clientName.c_str(), sizeof(client.sun_path));
							// Выполняем копирование объект подключения клиента в сторейдж
							::memcpy(&this->_peer.client, &client, sizeof(client));
							// Получаем размер объекта сокета
							const socklen_t size = (offsetof(struct sockaddr_un, sun_path) + strlen(client.sun_path));
							// Выполняем бинд на сокет
							if(::bind(this->sock, reinterpret_cast <struct sockaddr *> (&this->_peer.client), size) < 0){
								/**
								 * Если включён режим отладки
								 */
								#if DEBUG_MODE
									// Выводим сообщение об ошибке
									this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(unixsocket, static_cast <uint16_t> (type), clientName), log_t::flag_t::CRITICAL, this->_socket.message(AWH_ERROR()).c_str());
								/**
								* Если режим отладки не включён
								*/
								#else
									// Выводим сообщение об ошибке
									this->_log->print("%s", log_t::flag_t::CRITICAL, this->_socket.message(AWH_ERROR()).c_str());
								#endif
							}
						}
					}
				} break;
			}
			// Если приложение является сервером
			if(type == type_t::SERVER){
				// Получаем размер объекта сокета
				const socklen_t size = (offsetof(struct sockaddr_un, sun_path) + strlen(server.sun_path));
				// Выполняем бинд на сокет
				if(::bind(this->sock, reinterpret_cast <struct sockaddr *> (&this->_peer.server), size) < 0){
					/**
					 * Если включён режим отладки
					 */
					#if DEBUG_MODE
						// Выводим сообщение об ошибке
						this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(unixsocket, static_cast <uint16_t> (type)), log_t::flag_t::CRITICAL, this->_socket.message(AWH_ERROR()).c_str());
					/**
					* Если режим отладки не включён
					*/
					#else
						// Выводим сообщение об ошибке
						this->_log->print("%s", log_t::flag_t::CRITICAL, this->_socket.message(AWH_ERROR()).c_str());
					#endif
					// Выходим из приложения
					::exit(EXIT_FAILURE);
				}
			}
		#endif
	}
}
/**
 * @brief Метод инициализации адресного пространства сокета
 *
 * @param ip     адрес для которого нужно создать сокет
 * @param port   порт сервера для которого нужно создать сокет
 * @param family семейство сокета (AF_INET / AF_INET6 / AF_UNIX)
 * @param type   тип приложения (клиент или сервер)
 * @param onlyV6 флаг разрешающий использовать только IPv6 подключение
 * @return       параметры подключения к серверу
 */
void awh::Engine::Address::init(const string & ip, const uint32_t port, const int32_t family, const type_t type, const bool onlyV6) noexcept {
	// Если IP-адрес передан
	if(!ip.empty() && (port <= AWH_MAX_PORT) && !this->network.empty()){
		// Если список сетевых интерфейсов установлен
		if((family == AF_INET) || (family == AF_INET6)){
			// Получаем хост текущего компьютера
			const string & host = this->host(family);
			/**
			 * Определяем тип подключения
			 */
			switch(family){
				// Для протокола IPv4
				case AF_INET: {
					/**
					 * Определяем тип приложения
					 */
					switch(static_cast <uint8_t> (type)){
						// Если приложение является клиентом
						case static_cast <uint8_t> (type_t::CLIENT): {
							// Создаём объект клиента
							struct sockaddr_in client;
							// Очищаем всю структуру для клиента
							::memset(&client, 0, sizeof(client));
							// Устанавливаем протокол интернета
							client.sin_family = family;
							// Устанавливаем произвольный порт для локального подключения
							client.sin_port = htons(0);
							// Устанавливаем адрес для локальго подключения
							client.sin_addr.s_addr = ::inet_addr(host.c_str());
							// Запоминаем размер структуры
							this->_peer.size = sizeof(client);
							// Выполняем копирование объекта подключения клиента
							::memcpy(&this->_peer.client, &client, this->_peer.size);
						} break;
						// Если приложение является сервером
						case static_cast <uint8_t> (type_t::SERVER): {
							// Создаём объект клиента
							struct sockaddr_in client;
							// Очищаем всю структуру для клиента
							::memset(&client, 0, sizeof(client));
							// Устанавливаем протокол интернета
							client.sin_family = family;
							// Выполняем копирование объекта подключения клиента
							::memcpy(&this->_peer.client, &client, sizeof(client));
						} break;
					}
					// Создаём объект сервера
					struct sockaddr_in server;
					// Очищаем всю структуру для сервера
					::memset(&server, 0, sizeof(server));
					// Устанавливаем протокол интернета
					server.sin_family = family;
					// Устанавливаем порт для локального подключения
					server.sin_port = htons(port);
					// Устанавливаем адрес для удаленного подключения
					server.sin_addr.s_addr = ::inet_addr(ip.c_str());
					// Если ядро является сервером
					if(type == type_t::SERVER)
						// Запоминаем размер структуры
						this->_peer.size = sizeof(server);
					// Выполняем копирование объекта подключения сервера
					::memcpy(&this->_peer.server, &server, this->_peer.size);
					// Обнуляем серверную структуру
					::memset(&(reinterpret_cast <struct sockaddr_in *> (&this->_peer.server)->sin_zero), 0, sizeof(server.sin_zero));
				} break;
				// Для протокола IPv6
				case AF_INET6: {
					/**
					 * Определяем тип приложения
					 */
					switch(static_cast <uint8_t> (type)){
						// Если приложение является клиентом
						case static_cast <uint8_t> (type_t::CLIENT): {
							// Создаём объект клиента
							struct sockaddr_in6 client;
							// Очищаем всю структуру для клиента
							::memset(&client, 0, sizeof(client));
							// Устанавливаем протокол интернета
							client.sin6_family = family;
							// Устанавливаем произвольный порт для локального подключения
							client.sin6_port = htons(0);
							// Указываем адрес IPv6 для клиента
							::inet_pton(family, host.c_str(), &client.sin6_addr);
							// ::inet_ntop(family, &client.sin6_addr, hostClient, sizeof(hostClient));
							// Запоминаем размер структуры
							this->_peer.size = sizeof(client);
							// Выполняем копирование объекта подключения клиента
							::memcpy(&this->_peer.client, &client, this->_peer.size);
						} break;
						// Если приложение является сервером
						case static_cast <uint8_t> (type_t::SERVER): {
							// Создаём объект клиента
							struct sockaddr_in6 client;
							// Очищаем всю структуру для клиента
							::memset(&client, 0, sizeof(client));
							// Устанавливаем протокол интернета
							client.sin6_family = family;
							// Выполняем копирование объекта подключения клиента
							::memcpy(&this->_peer.client, &client, sizeof(client));
						} break;
					}
					// Создаём объект сервера
					struct sockaddr_in6 server;
					// Очищаем всю структуру для сервера
					::memset(&server, 0, sizeof(server));
					// Устанавливаем протокол интернета
					server.sin6_family = family;
					// Устанавливаем порт для локального подключения
					server.sin6_port = htons(port);
					// Указываем адрес IPv6 для сервера
					::inet_pton(family, ip.c_str(), &server.sin6_addr);
					// ::inet_ntop(family, &server.sin6_addr, hostServer, sizeof(hostServer));
					// Если приложение является сервером
					if(type == type_t::SERVER)
						// Запоминаем размер структуры
						this->_peer.size = sizeof(server);
					// Выполняем копирование объекта подключения сервера
					::memcpy(&this->_peer.server, &server, this->_peer.size);
				} break;
				// Если тип сети не определен
				default: {
					// Выводим сообщение в консоль
					this->_log->print("Network not allow from server [%s:%u]", log_t::flag_t::CRITICAL, ip.c_str(), port);
					// Выходим
					return;
				}
			}
			// Создаем сокет подключения
			this->sock = ::socket(family, this->_type, this->_protocol);
			// Если сокет не создан то выходим
			if(this->sock == INVALID_SOCKET){
				/**
				 * Если включён режим отладки
				 */
				#if DEBUG_MODE
					// Выводим сообщение об ошибке
					this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(ip, port, family, static_cast <uint16_t> (type), onlyV6), log_t::flag_t::CRITICAL, this->_socket.message(AWH_ERROR()).c_str());
				/**
				* Если режим отладки не включён
				*/
				#else
					// Выводим сообщение об ошибке
					this->_log->print("%s", log_t::flag_t::CRITICAL, this->_socket.message(AWH_ERROR()).c_str());
				#endif
				// Выходим
				return;
			}
			/**
			 * Для операционной системы не являющейся MS Windows
			 */
			#if !_WIN32 && !_WIN64
				// Выполняем игнорирование сигнала неверной инструкции процессора
				this->_socket.noSigILL();
				// Если сокет установлен TCP/IP
				if(this->_type == SOCK_STREAM)
					// Отключаем сигнал записи в оборванное подключение
					this->_socket.noSigPIPE(this->sock);
				// Если приложение является сервером
				if(type == type_t::SERVER){
					// Включаем отображение сети IPv4 в IPv6
					if(family == AF_INET6)
						// Выполняем активацию работы только IPv6 сети
						this->_socket.onlyIPv6(this->sock, onlyV6 ? socket_t::mode_t::ENABLED : socket_t::mode_t::DISABLED);
				// Если приложение является клиентом и сокет установлен TCP/IP
				} else if(this->_type == SOCK_STREAM) {
					/**
					 * Для операционной системы Linux или FreeBSD
					 */
					#if __linux__ || __FreeBSD__
						// Если протокол интернета установлен как SCTP
						if(this->_protocol != IPPROTO_SCTP)
							// Активируем KeepAlive
							this->_socket.keepAlive(this->sock, this->alive.cnt, this->alive.idle, this->alive.intvl);
					/**
					 * Для операционной системы Linux
					 */
					#else
						// Активируем KeepAlive
						this->_socket.keepAlive(this->sock, this->alive.cnt, this->alive.idle, this->alive.intvl);
					#endif
				}
			/**
			 * Для операционной системы MS Windows
			 */
			#else
				// Если приложение является сервером
				if(type == type_t::SERVER){
					// Включаем отображение сети IPv4 в IPv6
					if(family == AF_INET6)
						// Выполняем активацию работы только IPv6 сети
						this->_socket.onlyIPv6(this->sock, onlyV6 ? socket_t::mode_t::ENABLED : socket_t::mode_t::DISABLED);
				// Если приложение является клиентом и сокет установлен TCP/IP
				} else if(this->_type == SOCK_STREAM)
					// Активируем KeepAlive
					this->_socket.keepAlive(this->sock);
			#endif
			// Если приложение является сервером
			if(type == type_t::SERVER)
				// Переводим сокет в не блокирующий режим
				this->_socket.blocking(this->sock, socket_t::mode_t::DISABLED);
			// Устанавливаем разрешение на повторное использование сокета
			this->_socket.reuseable(this->sock);
			/**
			 * Определяем тип запускаемого приложения
			 */
			switch(static_cast <uint8_t> (type)){
				// Если приложение является сервером
				case static_cast <uint8_t> (type_t::SERVER): {
					// Выполняем бинд на сокет
					if(::bind(this->sock, reinterpret_cast <struct sockaddr *> (&this->_peer.server), this->_peer.size) < 0){
						/**
						 * Если включён режим отладки
						 */
						#if DEBUG_MODE
							// Выводим сообщение об ошибке
							this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(host, ip, port, family, static_cast <uint16_t> (type), onlyV6), log_t::flag_t::CRITICAL, this->_socket.message(AWH_ERROR()).c_str());
						/**
						* Если режим отладки не включён
						*/
						#else
							// Выводим сообщение об ошибке
							this->_log->print("%s", log_t::flag_t::CRITICAL, this->_socket.message(AWH_ERROR()).c_str());
						#endif
						// Выходим из приложения
						::exit(EXIT_FAILURE);
					}
				} break;
				// Если приложение является клиентом
				case static_cast <uint8_t> (type_t::CLIENT): {
					// Выполняем бинд на сокет
					if(::bind(this->sock, reinterpret_cast <struct sockaddr *> (&this->_peer.client), this->_peer.size) < 0){
						/**
						 * Если включён режим отладки
						 */
						#if DEBUG_MODE
							// Выводим сообщение об ошибке
							this->_log->debug("%s", __PRETTY_FUNCTION__, std::make_tuple(host, ip, port, family, static_cast <uint16_t> (type), onlyV6), log_t::flag_t::CRITICAL, this->_socket.message(AWH_ERROR()).c_str());
						/**
						* Если режим отладки не включён
						*/
						#else
							// Выводим сообщение об ошибке
							this->_log->print("%s", log_t::flag_t::CRITICAL, this->_socket.message(AWH_ERROR()).c_str());
						#endif
					}
				} break;
			}
		}
	}
}
/**
 * @brief Деструктор
 *
 */
awh::Engine::Address::~Address() noexcept {
	// Выполняем отключение
	this->clear();
}
/**
 * @brief Метод вывода информации об ошибке
 *
 * @param status статус ошибки
 * @return       результат работы фукнции
 */
int32_t awh::Engine::Context::error(const int32_t status) const noexcept {
	// Результат работы функции
	int32_t result = -1;
	// Если защищённый режим работы разрешён
	if(this->_encrypted && (this->_tls != nullptr)){
		// Отказ, которым связь оборвана
		const nts::context::error_t error = ((this->_type == type_t::SERVER) ? this->_tls->server.error() : this->_tls->client.error());
		// Состояние связи
		const nts::context::state_t state = ((this->_type == type_t::SERVER) ? this->_tls->server.state() : this->_tls->client.state());
		// Извещение, полученное или отправленное связью
		const nts::alert::alert_t & alert = ((this->_type == type_t::SERVER) ? this->_tls->server.alert() : this->_tls->client.alert());
		/**
		 * Состояние CLOSED наступает по любой из сторон, поэтому о том, кончился ли
		 * приём, говорит не оно, а отсутствие принятых данных после подачи
		 */
		if(error != nts::context::error_t::NONE){
			// Устанавливаем результат отключения подключения
			result = 0;
			// Выводим в лог сообщение об отказе связи
			this->_log->print("Engine: connection failed: code=%hhu state=%hhu alert=%s", log_t::flag_t::CRITICAL,
				static_cast <uint8_t> (error), static_cast <uint8_t> (state), nts::alert::name(alert.description));
		// Если отказа связи нет, решение принимаем по её состоянию
		} else {
			/**
			 * Определяем состояние связи
			 */
			switch(static_cast <uint8_t> (state)){
				// Если связь закрыта одной из сторон
				case static_cast <uint8_t> (nts::context::state_t::CLOSED): {
					// Устанавливаем результат отключения подключения
					result = 0;
					// Выводим в лог сообщение о закрытии связи собеседником
					this->_log->print("Engine: connection closed by peer: alert=%s", log_t::flag_t::INFO, nts::alert::name(alert.description));
				} break;
				// Если связь оборвана отказом
				case static_cast <uint8_t> (nts::context::state_t::FAILED): {
					// Устанавливаем результат отключения подключения
					result = 0;
					// Выводим в лог сообщение об отказе рукопожатия
					this->_log->print("Engine: handshake failed: alert=%s", log_t::flag_t::CRITICAL, nts::alert::name(alert.description));
				} break;
			}
		}
	// Если произошла ошибка
	} else if(status < 0) {
		/**
		 * Определяем тип ошибки
		 */
		switch(AWH_ERROR()){
			// Если ошибка не обнаружена, выходим
			case 0: break;
			/**
			 * Для операционной системы не являющейся MS Windows
			 */
			#if !_WIN32 && !_WIN64
				// Если произведена неудачная запись в PIPE
				case EPIPE:
				// Если произведён сброс подключения
				case ECONNRESET: {
					// Устанавливаем результат отключения подключения
					result = 0;
					// Выводим в лог сообщение
					this->_log->print("Engine: %s", log_t::flag_t::WARNING, this->_addr->_socket.message().c_str());
				} break;
			/**
			 * Для операционной системы MS Windows
			 */
			#else
				// Если произведён сброс подключения
				case WSAECONNRESET: {
					// Устанавливаем результат отключения подключения
					result = 0;
					// Выводим в лог сообщение
					this->_log->print("Engine: %s", log_t::flag_t::WARNING, this->_addr->_socket.message().c_str());
				} break;
			#endif
			// Для остальных ошибок
			default: {
				// Устанавливаем результат отключения подключения
				result = 0;
				// Выводим в лог сообщение
				this->_log->print("Engine: %s", log_t::flag_t::CRITICAL, this->_addr->_socket.message().c_str());
			}
		}
	}
	// Выводим значение результата
	return result;
}
/**
 * @brief Метод проверки временной ошибки отправки датаграммы
 *
 * @return результат проверки (true - отправку нужно повторить позже)
 */
bool awh::Engine::Context::retry() const noexcept {
	/**
	 * Для операционной системы MS Windows
	 */
	#if _WIN32 || _WIN64
		// Выводим результат проверки
		return ((AWH_ERROR() == WSAEWOULDBLOCK) || (AWH_ERROR() == WSAENOBUFS) || (AWH_ERROR() == WSAEINTR));
	/**
	 * Для операционной системы не являющейся MS Windows
	 */
	#else
		// Выводим результат проверки
		return ((AWH_ERROR() == EWOULDBLOCK) || (AWH_ERROR() == EAGAIN) || (AWH_ERROR() == ENOBUFS) || (AWH_ERROR() == EINTR));
	#endif
}
/**
 * @brief Метод проверки пройденности рукопожатия обеими сторонами
 *
 * @return результат проверки
 *
 */
bool awh::Engine::Context::established() const noexcept {
	// Если связь создана, выводим результат проверки
	if(this->_tls != nullptr)
		// Выводим признак пройденного рукопожатия
		return ((this->_type == type_t::SERVER) ? this->_tls->server.established() : this->_tls->client.established());
	// Сообщаем, что связь не создана
	return false;
}
/**
 * @brief Метод проверки наличия байтов, которые связь должна отправить
 *
 * @return результат проверки (true - в накопителе связи лежат неотправленные байты)
 *
 */
bool awh::Engine::Context::pending() const noexcept {
	// Если связь не создана, отправлять нечего
	if(this->_tls == nullptr)
		// Сообщаем, что неотправленных байтов нет
		return false;
	// Байты, накопленные связью для отправки собеседнику
	const nts::view_t data = ((this->_type == type_t::SERVER) ? this->_tls->server.output() : this->_tls->client.output());
	// Выводим признак наличия неотправленных байтов
	return (data.size > 0);
}
/**
 * @brief Метод отправки накопленных связи байтов в сокет
 *
 * Снятие байтов происходит ровно в том объёме, в котором сокет их принял: связь не знает,
 * дошло ли до собеседника, и повторной генерации записей не будет
 *
 * @return результат работы функции (false - сокет не принял всё, повторить позже)
 *
 */
bool awh::Engine::Context::flush() noexcept {
	// Результат работы функции
	bool result = true;
	// Если связь и сокет инициализированы
	if((this->_tls != nullptr) && (this->_addr != nullptr) && (this->_addr->sock != INVALID_SOCKET)){
		/**
		 * Переход по накопленным связи байтам	
		 */
		for(size_t count = 0; count < AWH_ENGINE_FLUSH_LIMIT; count++){
			// Байты, отправляемые собеседнику
			const nts::view_t data = ((this->_type == type_t::SERVER) ? this->_tls->server.output() : this->_tls->client.output());
			// Если связи нечего отправить, выходим
			if(data.size == 0)
				// Выходим из цикла
				break;
			// Количество отправленных байтов
			ssize_t sent = 0;
			/**
			 * Определяем тип сокета
			 */
			if(this->_addr->_type == SOCK_DGRAM){
				// Адрес собеседника, которому предназначена дейтаграмма
				struct sockaddr * addr = reinterpret_cast <struct sockaddr *> (((this->_type == type_t::SERVER) ? &this->_addr->_peer.client : &this->_addr->_peer.server));
				// Выполняем отправку дейтаграммы: накопитель связи отдаёт ровно одну готовую дейтаграмму
				sent = ::sendto(this->_addr->sock, data.data, data.size, 0, addr, sizeof(struct sockaddr_storage));
			// Если сокет установлен как потоковый, отправляем байты как есть
			} else sent = ::send(this->_addr->sock, data.data, data.size, 0);
			// Если сокет принял байты
			if(sent > 0){
				// Снимаем отправленные байты с накопителя связи
				if(this->_type == type_t::SERVER)
					// Выполняем снятие байтов у связи сервера
					this->_tls->server.consume(static_cast <size_t> (sent));
				else this->_tls->client.consume(static_cast <size_t> (sent));
				// Если сокет принял не всё, повторяем отправку позже
				if(static_cast <size_t> (sent) < data.size)
					// Выводим признак того, что нужно дождаться готовности к записи
					return false;
				// Выполняем переход к следующей части накопителя
				continue;
			// Если сокет временно не готов принять байты, выводим признак ожидания
			} else if(this->retry()) return false;
			// Выводим в лог сообщение об ошибке отправки
			this->_log->print("Engine: write failed: %s", log_t::flag_t::WARNING, this->_addr->_socket.message().c_str());
			// Выходим из цикла
			break;
		}
	}
	// Выводим результат
	return result;
}
/**
 * @brief Метод приёма байтов из сокета и подачи их связи
 *
 * Сервер разбирает приветствие клиента до подачи его байтов связи: подлинность по имени (SNI)
 * нужно успеть выставить, пока ответ не сложен
 *
 * @return количество принятых байт, 0 - собеседник закрылся, -1 - данных нет, -2 - связь отвергла байты
 *
 */
int64_t awh::Engine::Context::feed() noexcept {
	// Результат работы функции
	int64_t result = -1;
	// Если связь и сокет инициализированы
	if((this->_tls != nullptr) && (this->_addr != nullptr) && (this->_addr->sock != INVALID_SOCKET)){
		// Буфер принятых байтов канала
		vector <uint8_t> buffer;
		// Размер буфера приёма: дейтаграмма либо кусок потока
		const size_t max = ((this->_addr->_type == SOCK_DGRAM) ? static_cast <size_t> (AWH_ENGINE_DATAGRAM_SIZE) : static_cast <size_t> (AWH_ENGINE_CHUNK_SIZE));
		// Выполняем выделение памяти под буфер приёма
		buffer.resize(max);
		// Количество принятых байтов
		ssize_t got = 0;
		/**
		 * Определяем тип сокета
		 */
		if(this->_addr->_type == SOCK_DGRAM){
			// Адрес собеседника, от которого ждём дейтаграмму
			struct sockaddr * addr = reinterpret_cast <struct sockaddr *> (((this->_type == type_t::SERVER) ? &this->_addr->_peer.client : &this->_addr->_peer.server));
			// Выполняем приём дейтаграммы
			got = ::recvfrom(this->_addr->sock, buffer.data(), max, 0, addr, &this->_addr->_peer.size);
		// Если сокет установлен как потоковый, читаем поток байтов
		} else got = ::recv(this->_addr->sock, buffer.data(), max, 0);
		// Если байты приняты
		if(got > 0){
			// Байты, которые связи ещё не показаны
			const uint8_t * data = buffer.data();
			// Размер байтов, которые связи показываются этим шагом
			size_t size = static_cast <size_t> (got);
			/**
			 * Серверу нужно выбрать подлинность по предъявленному имени до того, как
			 * приветствие будет разобрано связью: окно «разобрано, но не отвечено» у
			 * наружного модуля отсутствует
			 */
			if((this->_type == type_t::SERVER) && !this->_tls->sniffed){
				// Добавляем принятые байты к накопленному для разбора приветствию
				this->_tls->peek.insert(this->_tls->peek.end(), data, data + size);
				// Тело сообщения рукопожатия, собранное из принятых записей
				vector <uint8_t> body;
				// Если тело приветствия ещё не собрано
				if(!helloBody(this->_tls->peek, body)){
					// Пока байтов не больше разбираемого предела, ждём их продолжения
					if(this->_tls->peek.size() <= static_cast <size_t> (AWH_ENGINE_HELLO_LIMIT))
						// Выводим количество принятых байтов, связи их ещё не показаны
						return static_cast <int64_t> (got);
					// За пределом ждать нечего: подаём накопленное как есть и перестаём разбирать
					this->_tls->sniffed = true;
				// Тело приветствия собрано: подлинность выбираем по предъявленному имени
				} else {
					// Имя узла, за которым пришёл клиент
					string name;
					// Если имя разобрано и движок доступен, выбираем подлинность по нему
					if(helloHostname(body, name) && (this->_tls->engine != nullptr)){
						/**
						 * Смена подлинности до разбора приветствия связью возможна:
						 * ответ ещё не сложен
						 */
						this->_tls->engine->sniSelect(*this, name);
					}
					// Запоминаем, что разбор приветствия сделан
					this->_tls->sniffed = true;
				}
				// Байты, показываемые связи, берутся из накопленного для разбора
				data = this->_tls->peek.data();
				// Устанавливаем размер показываемых байтов
				size = this->_tls->peek.size();
			}
			// Байты подаются связи как пришли: границы записей связь восстанавливает сама
			const nts::context::error_t error = ((this->_type == type_t::SERVER) ? this->_tls->server.push(nts::view_t(data, size)) : this->_tls->client.push(nts::view_t(data, size)));
			// Очистка накопленного для разбора приветствия
			this->_tls->peek.clear();
			// Если связь отвергла принятые байты
			if(error != nts::context::error_t::NONE){
				// Выводим в лог сообщение об отказе разбора
				this->_log->print("Engine: received bytes refused: code=%hhu", log_t::flag_t::CRITICAL, static_cast <uint8_t> (error));
				// Выводим признак отказа связи
				return -2;
			}
			// Выводим количество принятых байтов
			result = static_cast <int64_t> (got);
		// Если собеседник закрыл соединение
		} else if(got == 0) result = 0;
		// Если приём не выполнен по временной причине, выводим ожидание данных
		else if(!this->retry()){
			// Выводим в лог сообщение об ошибке приёма
			this->_log->print("Engine: read failed: %s", log_t::flag_t::WARNING, this->_addr->_socket.message().c_str());
			// Выводим признак отказа связи
			result = -2;
		}
	}
	// Выводим результат
	return result;
}
/**
 * @brief Метод извлечения прочитанных связью байтов в буфер чтения
 *
 * Раздел 5.4 RFC 8446: принятые записи открытым текстом выдаются накопителем целиком,
 * поэтому неотданный остаток держится в самом буфере чтения
 *
 */
void awh::Engine::Context::drain() noexcept {
	// Если связь инициализирована
	if(this->_tls != nullptr){
		// Принятые связью байты открытого текста
		vector <uint8_t> data;
		// Отказ извлечения принятых байтов
		const nts::context::error_t error = ((this->_type == type_t::SERVER) ? this->_tls->server.read(data) : this->_tls->client.read(data));
		// Если отказ не связан с состоянием связи, выводим сообщение
		if((error != nts::context::error_t::NONE) && (error != nts::context::error_t::STATE)){
			// Выводим в лог сообщение об отказе извлечения
			this->_log->print("Engine: received data refused: code=%hhu", log_t::flag_t::CRITICAL, static_cast <uint8_t> (error));
		// Если открытые байты получены, добавляем их к неотданному остатку
		} else if(!data.empty()) this->_tls->plain.insert(this->_tls->plain.end(), data.begin(), data.end());
	}
}
/**
 * @brief Метод перекачки байтов связи одним шагом
 *
 * @param want флаг, что прикладные данные нужны сейчас
 * @return     результат работы функции (1 - рукопожатие завершено, -1 - ждёт данных, -2 - ждёт записи, 0 - не удалось)
 */
int32_t awh::Engine::Context::pump(const bool want) noexcept {
	// Результат работы функции
	int32_t result = -1;
	// Если связь не создана, сообщаем об отказе
	if(this->_tls == nullptr)
		// Выводим признак того, что рукопожатие не удалось
		return 0;
	// Если приветствие своей стороны ещё не отправлено
	if(!this->_tls->started){
		// Отказ заведения связи
		const nts::context::error_t error = ((this->_type == type_t::SERVER) ? this->_tls->server.start() : this->_tls->client.start());
		// Если связь не удалось завести
		if(error != nts::context::error_t::NONE){
			// Выводим в лог сообщение об отказе заведения связи
			this->_log->print("Engine: connection start failed: code=%hhu", log_t::flag_t::CRITICAL, static_cast <uint8_t> (error));
			// Выводим признак того, что рукопожатие не удалось
			return 0;
		}
		// Запоминаем, что приветствие отправлено
		this->_tls->started = true;
	}
	// Байты, которые связь должна отправить, уходят первыми
	if(!this->flush())
		// Сообщаем, что нужно дождаться готовности сокета к записи
		return -2;
	/**
	 * Событие чтения ставится на краевой режим (EV_CLEAR), поэтому за один шаг
	 * выбираем весь пришедший канал: оставшиеся в сокете байты не дождутся
	 * следующего изменения состояния, и рукопожатие встанет
	 */
	int64_t size = -1;
	/**
	 * Переход по принятым байтам канала
	 */
	for(size_t count = 0; count < AWH_ENGINE_FEED_LIMIT; count++){
		// Результат приёма байтов канала
		size = this->feed();
		// Если связь отвергла принятые байты
		if(size == -2)
			// Сообщаем, что рукопожатие не удалось
			return 0;
		// Если читать больше нечего либо собеседник закрыл связь, останавливаемся
		if(size <= 0)
			// Выходим из цикла
			break;
		// Открытый текст принятых записей забираем в буфер чтения
		this->drain();
	}
	// Если подлинность по предъявленному имени не годится, рукопожатие прерываем
	if(this->_tls->fatal)
		// Сообщаем, что рукопожатие не удалось
		return 0;
	/**
	 * Записи приложения и close_notify могут прийти одним куском: открытый текст уже в
	 * буфере, поэтому закрытие связи объявляем только когда отдавать нечего. Иначе
	 * последнее сообщение собеседника пропало бы, не дойдя до вызывающего
	 */
	if(size == 0 && this->_tls->plain.empty())
		// Сообщаем, что рукопожатие не удалось
		return 0;
	/**
	 * Прежний OpenSSL проверял цепочку доверенных и соответствие сертификата доменному
	 * имени колбэками внутри рукопожатия. Наружный модуль цепочку собеседника не судит —
	 * запас доверенных нужен ему лишь для разбора ответа OCSP, — поэтому обе проверки
	 * делает насос: сразу по завершении рукопожатия и ровно один раз на связь
	 */
	if(this->established() && !this->_tls->checked){
		// Отметка о том, что проверки выполнены
		this->_tls->checked = true;
		// Цепочка сертификатов собеседника
		const vector <vector <uint8_t>> & chain = ((this->_type == type_t::SERVER) ? this->_tls->server.peer() : this->_tls->client.peer());
		// Если проверка доверия цепочке разрешена и цепочка получена
		if(this->_tls->verify && !chain.empty()){
			// Миг проверки соответствия сроков сертификатов
			const int64_t moment = static_cast <int64_t> (::time(nullptr));
			// Роль собеседника: у клиента проверяем сервер, у сервера — клиента
			const nts::trust::role_t role = ((this->_type == type_t::SERVER) ? nts::trust::role_t::CLIENT : nts::trust::role_t::SERVER);
			// Отказ доверия цепочки сертификатов запасом доверенных
			const nts::trust::error_t failure = nts::trust::verify(chain, this->_tls->store, moment, role);
			// Если цепочка запасу не доверена, рукопожатие не удалось
			if(failure != nts::trust::error_t::NONE){
				// Выводим в лог сообщение о недоверенной цепочке сертификатов
				this->_log->print("Certificate chain validation failed: code=%hhu", log_t::flag_t::CRITICAL, static_cast <uint8_t> (failure));
				// Сообщаем, что рукопожатие не удалось
				return 0;
			}
			/**
			 * Раздел 6.4 RFC 6125: имя узла сверяется с именами первого звена цепочки —
			 * дополнительных имён владельца, а при их отсутствии — с общим именем
			 */
			if(!this->_tls->host.empty()){
				// Отказ сличения имени узла с именами сертификата
				const nts::trust::error_t error = nts::trust::belongs(nts::view_t(chain.front().data(), chain.front().size()), this->_tls->host);
				// Если имя узла не отвечает именам сертификата, рукопожатие не удалось
				if(error != nts::trust::error_t::NONE){
					// Выводим в лог сообщение о несоответствии имени сертификата
					this->_log->print("Certificate does not match the host '%s': code=%hhu", log_t::flag_t::CRITICAL, this->_tls->host.c_str(), static_cast <uint8_t> (error));
					// Сообщаем, что рукопожатие не удалось
					return 0;
				}
			}
		}
	}
	// Если рукопожатие завершено обеими сторонами
	if(this->established()){
		/**
		 * Завершающий полёт рукопожатия (ChangeCipherSpec и Finished у TLS 1.2) связь
		 * складывает в накопитель тем же шагом, каким признаёт рукопожатие пройденным.
		 * Собеседник ждёт эти байты, поэтому завершённым рукопожатие объявляем только
		 * после их отправки: без этого ответ ушёл бы лишь по следующему событию этой
		 * стороны, а на краевых событиях это ожидание до таймаута подключения
		 */
		if(!this->flush())
			// Сообщаем, что нужно дождаться готовности сокета к записи
			return -2;
		// Упреждающие данные и данные конца рукопожатия приходят тем же шагом
		if(want)
			// Выполняем повторное извлечение принятых байтов
			this->drain();
		// Сообщаем, что рукопожатие завершено
		return 1;
	}
	// Ответ на принятые байты отправляем сразу, не дожидаясь следующего события
	if(!this->flush())
		// Сообщаем, что нужно дождаться готовности сокета к записи
		return -2;
	/**
	 * Открытые байты, принятые одним куском с закрытием связи, обязана получить
	 * вызывающая сторона: рукопожатие уже пройдено, и признак наличия данных отдаём
	 * до того, как отказ будет объявлен на следующем шаге, когда буфер опустеет
	 */
	if(!this->_tls->plain.empty())
		// Сообщаем, что данные приняты
		return 1;
	// Выводим результат шага: завершено либо ждём данных
	return (this->established() ? 1 : result);
}
/**
 * @brief Метод выполнения шага рукопожатия без передачи данных
 *
 * @return результат работы функции (1 - рукопожатие завершено, -1 - ждёт данных, -2 - ждёт записи, 0 - не удалось)
 */
int32_t awh::Engine::Context::negotiate() noexcept {
	// Если шифрование не активно либо связь не создана
	if(!this->_encrypted || (this->_tls == nullptr))
		// Сообщаем, что рукопожатие завершено
		return 1;
	// Если сокет не инициализирован
	if((this->_addr == nullptr) || (this->_addr->sock == INVALID_SOCKET))
		// Сообщаем, что рукопожатие не удалось
		return 0;
	/**
	 * Рукопожатие засчитывается пройденным только вместе с отправкой его завершающего
	 * полёта, поэтому неотправленные байты обязаны пойти шаг за шагом
	 */
	if(this->established() && !this->pending())
		// Сообщаем, что рукопожатие завершено
		return 1;
	// Выполняем шаг перекачки байтов связи
	const int32_t result = this->pump(false);
	// Если рукопожатие не удалось, выводим его причину
	if(result == 0)
		// Выводим информацию об ошибке связи
		static_cast <void> (this->error(0));
	// Выводим результат
	return result;
}
/**
 * @brief Метод получения времени до перепосылки рукопожатия DTLS
 *
 * @return время в миллисекундах до срабатывания таймера DTLS (0 - таймер не запущен)
 */
uint32_t awh::Engine::Context::retransmission() const noexcept {
	/**
	 * Потерянные сообщения рукопожатия DTLS перепосылает тот, кто связь ведёт: наружный
	 * модуль лишь считает срок ожидания, часы у него отсутствуют (раздел 5.8.2 RFC 9147)
	 */
	if(this->_encrypted && (this->_tls != nullptr) && (this->_addr != nullptr) && (this->_addr->_type == SOCK_DGRAM)){
		// Время до срабатывания таймера в миллисекундах
		const uint32_t msec = ((this->_type == type_t::SERVER) ? this->_tls->server.timeout() : this->_tls->client.timeout());
		// Выводим время ожидания
		return msec;
	}
	// Таймер не запущен
	return 0;
}
/**
 * @brief Метод перепосылки сообщений рукопожатия DTLS по истечении таймера
 *
 * @return результат работы функции (false - рукопожатие не удалось)
 */
bool awh::Engine::Context::retransmit() noexcept {
	// Если это подключение DTLS
	if(this->_encrypted && (this->_tls != nullptr) && (this->_addr != nullptr) && (this->_addr->_type == SOCK_DGRAM)){
		// Отказ перепосылки по истечении срока ожидания
		const nts::context::error_t error = ((this->_type == type_t::SERVER) ? this->_tls->server.elapsed() : this->_tls->client.elapsed());
		// Если попытки перепосылки исчерпаны
		if(error != nts::context::error_t::NONE){
			// Выводим в лог сообщение о перепосылке
			this->_log->print("DTLS handshake retransmission failed: code=%hhu", log_t::flag_t::WARNING, static_cast <uint8_t> (error));
			// Сообщаем, что рукопожатие не удалось
			return false;
		}
		// Если рукопожатие ещё не завершено либо остались неотправленные байты, продвигаем его: сообщения могли прийти без события
		if(!this->established() || this->pending()){
			// Результат шага перекачки байтов связи
			const int32_t result = this->pump(false);
			// Если рукопожатие не удалось
			if(result == 0)
				// Сообщаем, что рукопожатие не удалось
				return false;
		}
	}
	// Сообщаем, что всё хорошо
	return true;
}
/**
 * @brief Метод очистки контекста
 *
 */
void awh::Engine::Context::clear() noexcept {
	// Если связь создана
	if(this->_tls != nullptr){
		/**
		 * Извещение о закрытии передачи складывается в накопитель связи: отправить его
		 * собеседнику успевает тот, кто связь ведёт, — сокет ещё не закрыт
		 */
		if(this->_encrypted && this->established()){
			// Выполняем закрытие связи
			static_cast <void> ((this->_type == type_t::SERVER) ? this->_tls->server.close() : this->_tls->client.close());
			// Выполняем отправку накопленных байтов
			static_cast <void> (this->flush());
		}
		// Выполняем очистку состояния связи
		this->_tls.reset(nullptr);
	}
	// Если сокет активен
	if(this->_addr != nullptr)
		// Выполняем закрытие подключения
		this->_addr->clear();
	// Сбрасываем флаг вывода подробной информации
	this->_verb = false;
	// Сбрасываем флаг инициализации
	this->_encrypted = false;
}
/**
 * @brief Метод вывода информации о сертификате
 *
 */
void awh::Engine::Context::info() const noexcept {
	// Код издания протокола, согласованного связью
	const uint16_t version = ((this->_type == type_t::SERVER) ? this->_tls->server.version() : this->_tls->client.version());
	// Если связь уже что-то согласовала, выводим её параметры
	if(version > 0)
		// Выводим в лог информационное сообщение о согласованных параметрах связи
		this->_log->print("A-NTS: protocol 0x%04X cipher %s", log_t::flag_t::INFO, version, suiteName(((this->_type == type_t::SERVER) ? this->_tls->server.suite() : this->_tls->client.suite())));
	// Если связь инициализирована
	if(this->_tls != nullptr){
		// Цепочка сертификатов собеседника
		const vector <vector <uint8_t>> & chain = ((this->_type == type_t::SERVER) ? this->_tls->server.peer() : this->_tls->client.peer());
		// Набор шифрования, согласованный связью
		const nts::protect::suite_t suite = ((this->_type == type_t::SERVER) ? this->_tls->server.suite() : this->_tls->client.suite());
		// Буфер данных для получения названий сертификата
		string subject, issuer;
		// Если сертификат собеседника получен
		if(!chain.empty()){
			// Разбираемый сертификат собеседника
			nts::x509::certificate_t cert;
			// Если сертификат разобрался, выводим его названия
			if(nts::x509::error_t::NONE == cert.parse(nts::view_t(chain.front().data(), chain.front().size()))){
				// Выводим начальный разделитель
				printf ("------------------------------------------------------------\n\n");
				// Выводим заголовок
				printf("Peer certificates:\n");
				// Выполняем извлечение названия владельца сертификата
				nts::x509::text(cert.subjects(), subject);
				// Выводим название сертификата
				printf("Subject: %s\n", subject.c_str());
				// Выполняем извлечение названия эмитента сертификата
				nts::x509::text(cert.issuers(), issuer);
				// Выводим эмитента сертификата
				printf("Issuer: %s\n", issuer.c_str());
				// Выводим параметры шифрования
				printf("Cipher: %s\n", suiteName(suite));
				// Выводим конечный разделитель
				printf ("\n------------------------------------------------------------\n\n");
			}
		}
	}
}
/**
 * @brief Метод чтения данных из сокета
 *
 * @param buffer буфер данных для чтения
 * @param size   размер буфера данных
 * @return       количество считанных байт
 */
int64_t awh::Engine::Context::read(char * buffer, const size_t size) noexcept {
	// Результат работы функции
	int64_t result = 0;
	// Если буфер данных передан
	if((buffer != nullptr) && (this->_addr != nullptr) && (size > 0) &&
	   (this->_type != type_t::NONE) && (this->_addr->sock != INVALID_SOCKET)){
		// Если защищённый режим работы разрешён
		if(this->_encrypted && (this->_tls != nullptr)){
			// Результат шага перекачки байтов связи
			const int32_t step = this->pump(true);
			// Если связь не допустила байтов
			if(step <= 0){
				/**
				 * Связь жива, но байтов нет: ждём следующего события. Решение принимаем
				 * по шагу перекачки, а не по errno — у неблокирующего сокета errno может
				 * остаться от прежнего системного вызова
				 */
				if((step == -1) || (step == -2))
					// Выполняем пропуск попытки
					return -1;
				// Если нужно отключиться
				if((result = this->error(0)) == 0)
					// Устанавливаем статус отключён
					this->_addr->status = addr_t::status_t::DISCONNECTED;
				// Выводим полученный результат
				return result;
			}
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Если подробная информация ещё не выведена
				if(!this->_verb){
					// Запоминаем, что подробная информация уже была выведена
					this->_verb = true;
					// Выводим информацию о связи
					this->info();
				}
			#endif
			// Если неотданных байтов открытого текста нет, отдавать нечего
			if(this->_tls->plain.empty())
				// Выполняем пропуск попытки
				return -1;
			// Количество байтов, отдаваемых вызывающему
			result = static_cast <int64_t> ((this->_tls->plain.size() < size) ? this->_tls->plain.size() : size);
			// Выполняем копирование прочитанных байтов в буфер вызывающего
			::memcpy(buffer, this->_tls->plain.data(), static_cast <size_t> (result));
			// Снимаем отданные байты с остатка открытого текста
			this->_tls->plain.erase(this->_tls->plain.begin(), this->_tls->plain.begin() + static_cast <size_t> (result));
			// Выводим полученный результат
			return result;
		// Выполняем чтение из буфера данных стандартным образом
		} else {
			// Если сокет установлен как TCP/IP
			if(this->_addr->_type == SOCK_STREAM)
				// Выполняем чтение данных из TCP/IP сокета
				result = ::recv(this->_addr->sock, buffer, size, 0);
			// Если сокет установлен UDP
			else if(this->_addr->_type == SOCK_DGRAM) {
				// Создаём объект подключения
				struct sockaddr * addr = nullptr;
				/**
				 * Определяем тип подключения
				 */
				switch(static_cast <uint8_t> (this->_addr->status)){
					// Если статус установлен как подключение клиентом
					case static_cast <uint8_t> (addr_t::status_t::CONNECTED):
						// Запоминаем полученную структуру
						addr = reinterpret_cast <struct sockaddr *> (&this->_addr->_peer.server);
					break;
					// Если статус установлен как разрешение подключения к серверу
					case static_cast <uint8_t> (addr_t::status_t::ACCEPTED):
						// Запоминаем полученную структуру
						addr = reinterpret_cast <struct sockaddr *> (&this->_addr->_peer.client);
					break;
				}
				/**
				 * Выполняем чтение данных из сокета.
				 * Повтор чтения при EWOULDBLOCK для клиента намеренно убран: сокет клиента UDP
				 * блокирующий с таймаутом SO_RCVTIMEO, и при отсутствии данных повтор крутился
				 * бесконечно, подвешивая цикл событий. Теперь EWOULDBLOCK/EAGAIN возвращает -1
				 * (ниже по коду), и вызывающий выходит из цикла чтения до следующего события.
				 */
				result = ::recvfrom(this->_addr->sock, buffer, size, 0, addr, &this->_addr->_peer.size);
			}
		}
		// Если данные прочитать не удалось
		if(result <= 0){
			/**
			 * Определяем тип ошибки
			 */
			switch(AWH_ERROR()){
				// Если ошибка не обнаружена, выходим
				case 0: break;
				/**
				 * Если мы работаем в MacOS X
				 */
				#ifdef __APPLE__
					// Если операция не поддерживается сокетом
					case EOPNOTSUPP:
				#endif
				/**
				 * Для операционной системы не являющейся MS Windows
				 */
				#if !_WIN32 && !_WIN64
					// Если сработало событие таймаута
					case ETIME:
					// Если ошибка протокола
					case EPROTO:
					// Если в буфере закончились данные
					case ENOBUFS:
					// Если операция не поддерживается сокетом
					case ENOTSUP:
					// Если сеть отключена
					case ENETDOWN:
					// Если сокет не является сокетом
					case ENOTSOCK:
					// Если сокет не подключён
					case ENOTCONN:
					// Если хост не существует
					case EHOSTDOWN:
					// Если сокет отключён
					case ESHUTDOWN:
					// Если произведён сброс подключения
					case ETIMEDOUT:
					// Если подключение сброшено
					case ENETRESET:
					// Если операция отменена
					case ECANCELED:
					// Если подключение сброшено
					case ECONNRESET:
					// Если протокол повреждён для сокета
					case EPROTOTYPE:
					// Если протокол не поддерживается
					case ENOPROTOOPT:
					// Если сеть недоступна
					case ENETUNREACH:
					// Если протокол не поддерживается
					case EPFNOSUPPORT:
					// Если протокол подключения не поддерживается
					case EAFNOSUPPORT:
					// Если роутинг к хосту не существует
					case EHOSTUNREACH:
					// Если подключение оборванно
					case ECONNABORTED:
					// Если требуется адрес назначения
					case EDESTADDRREQ:
					// Если в соединении отказанно
					case ECONNREFUSED:
					// Если тип сокета не поддерживается
					case ESOCKTNOSUPPORT:
					// Если протокол не поддерживается
					case EPROTONOSUPPORT: {
				/**
				 * Для операционной системы MS Windows
				 */
				#else
					// Если в буфере закончились данные
					case WSAENOBUFS:
					// Если сеть отключена
					case WSAENETDOWN:
					// Если сокет не является сокетом
					case WSAENOTSOCK:
					// Если сокет не подключён
					case WSAENOTCONN:
					// Если произведён сброс подключения
					case WSAETIMEDOUT:
					// Если сокет отключён
					case WSAESHUTDOWN:
					// Если хост не существует
					case WSAEHOSTDOWN:
					// Если подключение сброшено
					case WSAENETRESET:
					// Если протокол повреждён для сокета
					case WSAEPROTOTYPE:
					// Если подключение сброшено
					case WSAECONNRESET:
					// Если операция не поддерживается сокетом
					case WSAEOPNOTSUPP:
					// Если протокол не поддерживается
					case WSAENOPROTOOPT:
					// Если сеть недоступна
					case WSAENETUNREACH:
					// Если протокол не поддерживается
					case WSAEPFNOSUPPORT:
					// Если протокол подключения не поддерживается
					case WSAEAFNOSUPPORT:
					// Если роутинг к хосту не существует
					case WSAEHOSTUNREACH:
					// Если подключение оборванно
					case WSAECONNABORTED:
					// Если требуется адрес назначения
					case WSAEDESTADDRREQ:
					// Если в соединении отказанно
					case WSAECONNREFUSED:
					// Если тип сокета не поддерживается
					case WSAESOCKTNOSUPPORT:
					// Если протокол не поддерживается
					case WSAEPROTONOSUPPORT: {
				#endif
					// Выводим в лог сообщение
					this->_log->print("READ: %s", log_t::flag_t::WARNING, this->_addr->_socket.message().c_str());
					// Требуем завершения работы
					result = 0;
				} break;
				// Для остальных ошибок
				default: {
					/**
					 * Для операционной системы MS Windows
					 */
					#if _WIN32 || _WIN64
						// Если данные не приняты по временной причине
						if((AWH_ERROR() == WSAEWOULDBLOCK) || (AWH_ERROR() == WSAEINTR))
							// Выполняем пропуск попытки
							return -1;
					/**
					 * Для операционной системы не являющейся MS Windows
					 */
					#else
						// Если данные не приняты по временной причине
						if((AWH_ERROR() == EWOULDBLOCK) || (AWH_ERROR() == EINTR))
							// Выполняем пропуск попытки
							return -1;
					#endif
					// Иначе выводим сообщение об ошибке
					result = this->error(result);
				}
			}
			// Если произошло отключение
			if(result == 0)
				// Устанавливаем статус отключён
				this->_addr->status = addr_t::status_t::DISCONNECTED;
		}
	}
	// Выводим результат
	return result;
}
/**
 * @brief Метод записи данных в сокет
 *
 * @param buffer буфер данных для записи
 * @param size   размер буфера данных
 * @return       количество записанных байт
 */
int64_t awh::Engine::Context::write(const char * buffer, const size_t size) noexcept {
	// Результат работы функции
	int64_t result = 0;
	// Если буфер данных передан
	if((buffer != nullptr) && (this->_addr != nullptr) && (size > 0) &&
	   (this->_type != type_t::NONE) && (this->_addr->sock != INVALID_SOCKET)){
		// Если защищённый режим работы разрешён
		if(this->_encrypted && (this->_tls != nullptr)){
			// Результат шага перекачки байтов связи
			const int32_t step = this->pump(false);
			// Если рукопожатие не выполнено
			if(step <= 0){
				/**
				 * Связь жива, но рукопожатие не сошлось: вызывающий ставит данные в
				 * очередь отправки и повторит попытку по событии, а не рвёт подключение
				 */
				if((step == -1) || (step == -2))
					// Выполняем пропуск попытки
					return -1;
				// Если нужно отключиться
				if((result = this->error(0)) == 0)
					// Устанавливаем статус отключён
					this->_addr->status = addr_t::status_t::DISCONNECTED;
				// Выводим полученный результат
				return result;
			}
			/**
			 * Если включён режим отладки
			 */
			#if DEBUG_MODE
				// Если подробная информация ещё не выведена
				if(!this->_verb){
					// Запоминаем, что подробная информация уже была выведена
					this->_verb = true;
					// Выводим информацию о связи
					this->info();
				}
			#endif
			/**
			 * Пока сокет не принял то, что сложила связь, новые прикладные байты в связь
			 * не кладём: повтор пойдёт из очереди вызывающего по событии записи, и
			 * принятые ранее байты не окажутся записаны дважды
			 */
			if(!this->flush())
				// Выполняем пропуск попытки
				return -1;
			// Байты прикладных данных, передаваемые связи
			const nts::view_t data(reinterpret_cast <const uint8_t *> (buffer), size);
			// Отказ передачи прикладных данных связи
			const nts::context::error_t error = ((this->_type == type_t::SERVER) ? this->_tls->server.write(data) : this->_tls->client.write(data));
			// Если связь отвергла прикладные байты
			if(error != nts::context::error_t::NONE){
				// Выводим в лог сообщение об отказе передачи
				this->_log->print("Engine: send refused: code=%hhu", log_t::flag_t::CRITICAL, static_cast <uint8_t> (error));
				// Устанавливаем статус отключён
				this->_addr->status = addr_t::status_t::DISCONNECTED;
				// Выводим признак того, что отправка не выполнена
				return 0;
			}
			// Отправляем то, что сложила связь: неотосланное уйдёт следующим шагом
			static_cast <void> (this->flush());
			// Выводим количество принятых связью байтов
			return static_cast <int64_t> (size);
		// Выполняем отправку сообщения в сокет
		} else {
			// Если сокет установлен как TCP/IP
			if(this->_addr->_type == SOCK_STREAM)
				// Выполняем отправку данных в TCP/IP сокет
				result = ::send(this->_addr->sock, buffer, size, 0);
			// Если сокет установлен UDP
			else if(this->_addr->_type == SOCK_DGRAM) {
				// Создаём объект подключения
				struct sockaddr * addr = nullptr;
				/**
				 * Определяем тип подключения
				 */
				switch(static_cast <uint8_t> (this->_addr->status)){
					// Если статус установлен как подключение клиентом
					case static_cast <uint8_t> (addr_t::status_t::CONNECTED): {
						// Запоминаем полученную структуру
						addr = reinterpret_cast <struct sockaddr *> (&this->_addr->_peer.server);
						/**
						 * Для операционной системы не являющейся MS Windows
						 */
						#if !_WIN32 && !_WIN64
							// Для протокола unix-сокета
							if(this->_addr->_peer.server.ss_family == AF_UNIX)
								// Получаем размер объекта сокета
								this->_addr->_peer.size = (
									offsetof(struct sockaddr_un, sun_path) +
									strlen(reinterpret_cast <struct sockaddr_un *> (&this->_addr->_peer.server)->sun_path)
								);
						#endif
					} break;
					// Если статус установлен как разрешение подключения к серверу
					case static_cast <uint8_t> (addr_t::status_t::ACCEPTED): {
						// Запоминаем полученную структуру
						addr = reinterpret_cast <struct sockaddr *> (&this->_addr->_peer.client);
						/**
						 * Для операционной системы не являющейся MS Windows
						 */
						#if !_WIN32 && !_WIN64
							// Для протокола unix-сокета
							if(this->_addr->_peer.client.ss_family == AF_UNIX)
								// Получаем размер объекта сокета
								this->_addr->_peer.size = (
									offsetof(struct sockaddr_un, sun_path) +
									strlen(reinterpret_cast <struct sockaddr_un *> (&this->_addr->_peer.client)->sun_path)
								);
						#endif
					} break;
				}
				// Выполняем запись данных в сокет
				result = ::sendto(this->_addr->sock, buffer, size, 0, addr, this->_addr->_peer.size);
				/**
				 * Повтор на месте при EWOULDBLOCK убран: очередь отправки брокера хранит границы
				 * датаграмм (node_t::datagram), поэтому отложенная датаграмма ставится в очередь
				 * целиком (-1) и уходит отдельно по готовности сокета. ENOBUFS (так unix-сокет
				 * датаграмм в macOS сообщает о переполнении буфера получателя) тоже не разрывает
				 * подключение, а откладывает отправку
				 */
				if((result < 0) && this->retry())
					// Выполняем пропуск попытки
					return -1;
			}
		}
		// Если данные записать не удалось
		if(result <= 0){
			/**
			 * Определяем тип ошибки
			 */
			switch(AWH_ERROR()){
				// Если ошибка не обнаружена, выходим
				case 0: break;
				/**
				 * Если мы работаем в MacOS X
				 */
				#ifdef __APPLE__
					// Если операция не поддерживается сокетом
					case EOPNOTSUPP:
				#endif
				/**
				 * Для операционной системы не являющейся MS Windows
				 */
				#if !_WIN32 && !_WIN64
					// Если сработало событие таймаута
					case ETIME:
					// Если ошибка протокола
					case EPROTO:
					// Если в буфере закончились данные
					case ENOBUFS:
					// Если операция не поддерживается сокетом
					case ENOTSUP:
					// Если сеть отключена
					case ENETDOWN:
					// Если сокет не является сокетом
					case ENOTSOCK:
					// Если сокет не подключён
					case ENOTCONN:
					// Если хост не существует
					case EHOSTDOWN:
					// Если сокет отключён
					case ESHUTDOWN:
					// Если произведён сброс подключения
					case ETIMEDOUT:
					// Если подключение сброшено
					case ENETRESET:
					// Если операция отменена
					case ECANCELED:
					// Если подключение сброшено
					case ECONNRESET:
					// Если протокол повреждён для сокета
					case EPROTOTYPE:
					// Если протокол не поддерживается
					case ENOPROTOOPT:
					// Если сеть недоступна
					case ENETUNREACH:
					// Если протокол не поддерживается
					case EPFNOSUPPORT:
					// Если протокол подключения не поддерживается
					case EAFNOSUPPORT:
					// Если роутинг к хосту не существует
					case EHOSTUNREACH:
					// Если подключение оборванно
					case ECONNABORTED:
					// Если требуется адрес назначения
					case EDESTADDRREQ:
					// Если в соединении отказанно
					case ECONNREFUSED:
					// Если тип сокета не поддерживается
					case ESOCKTNOSUPPORT:
					// Если протокол не поддерживается
					case EPROTONOSUPPORT: {
				/**
				 * Для операционной системы MS Windows
				 */
				#else
					// Если в буфере закончились данные
					case WSAENOBUFS:
					// Если сеть отключена
					case WSAENETDOWN:
					// Если сокет не является сокетом
					case WSAENOTSOCK:
					// Если сокет не подключён
					case WSAENOTCONN:
					// Если произведён сброс подключения
					case WSAETIMEDOUT:
					// Если сокет отключён
					case WSAESHUTDOWN:
					// Если хост не существует
					case WSAEHOSTDOWN:
					// Если подключение сброшено
					case WSAENETRESET:
					// Если протокол повреждён для сокета
					case WSAEPROTOTYPE:
					// Если подключение сброшено
					case WSAECONNRESET:
					// Если операция не поддерживается сокетом
					case WSAEOPNOTSUPP:
					// Если протокол не поддерживается
					case WSAENOPROTOOPT:
					// Если сеть недоступна
					case WSAENETUNREACH:
					// Если протокол не поддерживается
					case WSAEPFNOSUPPORT:
					// Если протокол подключения не поддерживается
					case WSAEAFNOSUPPORT:
					// Если роутинг к хосту не существует
					case WSAEHOSTUNREACH:
					// Если подключение оборванно
					case WSAECONNABORTED:
					// Если требуется адрес назначения
					case WSAEDESTADDRREQ:
					// Если в соединении отказанно
					case WSAECONNREFUSED:
					// Если тип сокета не поддерживается
					case WSAESOCKTNOSUPPORT:
					// Если протокол не поддерживается
					case WSAEPROTONOSUPPORT: {
				#endif
					// Выводим в лог сообщение
					this->_log->print("WRITE: %s", log_t::flag_t::WARNING, this->_addr->_socket.message().c_str());
					// Требуем завершения работы
					result = 0;
				} break;
				// Для остальных ошибок
				default: {
					/**
					 * Для операционной системы MS Windows
					 */
					#if _WIN32 || _WIN64
						// Если данные не отправлены по временной причине
						if((AWH_ERROR() == WSAEWOULDBLOCK) || (AWH_ERROR() == WSAEINTR))
							// Выполняем пропуск попытки
							return -1;
					/**
					 * Для операционной системы не являющейся MS Windows
					 */
					#else
						// Если данные не отправлены по временной причине
						if((AWH_ERROR() == EWOULDBLOCK) || (AWH_ERROR() == EINTR))
							// Выполняем пропуск попытки
							return -1;
					#endif
					// Иначе выводим сообщение об ошибке
					result = this->error(result);
				}
			}
			// Если произошло отключение
			if(result == 0)
				// Устанавливаем статус отключён
				this->_addr->status = addr_t::status_t::DISCONNECTED;
		}
	}
	// Выводим результат
	return result;
}
/**
 * @brief Метод проверки на то, является ли сокет заблокированным
 *
 * @return результат работы функции
 */
bool awh::Engine::Context::blocking() noexcept {
	// Если адрес присвоен
	if(this->_addr != nullptr)
		// Выводим результат проверки
		return !this->_addr->_async;
	// Сообщаем что сокет в блокирующем режиме
	return true;
}
/**
 * @brief Метод активации/деактивации блокирующего сокета
 *
 * @param mode режим применимой операции
 * @return     результат работы функции
 */
bool awh::Engine::Context::blocking(const mode_t mode) noexcept {
	// Если адрес присвоен
	if(this->_addr != nullptr){
		// Если сокет инициализирован
		if(this->_addr->sock != INVALID_SOCKET){
			/**
			 * Определяем режим применяемой операции
			 */
			switch(static_cast <uint8_t> (mode)){
				// Если необходимо перевести сокет в блокирующий режим
				case static_cast <uint8_t> (mode_t::ENABLED):
					// Переводим сокет в блокирующий режим
					this->_addr->_async = !this->_addr->_socket.blocking(this->_addr->sock, socket_t::mode_t::ENABLED);
				break;
				// Если необходимо перевести сокет в неблокирующий режим
				case static_cast <uint8_t> (mode_t::DISABLED):
					// Переводим сокет в не блокирующий режим
					this->_addr->_async = this->_addr->_socket.blocking(this->_addr->sock, socket_t::mode_t::DISABLED);
				break;
			}
		}
		// Выводим результат
		return !this->_addr->_async;
	}
	// Сообщаем что сокет в блокирующем режиме
	return false;
}
/**
 * @brief Метод отключения/включения алгоритма TCP/CORK
 *
 * @param mode режим применимой операции
 * @return     результат выполенния операции
 */
bool awh::Engine::Context::cork(const mode_t mode) noexcept {
	// Результат работы функции
	bool result = false;
	// Если адрес присвоен
	if(this->_addr != nullptr){
		// Если сокет инициализирован
		if(this->_addr->sock != INVALID_SOCKET){
			// Если сокет установлен TCP/IP
			if(this->_addr->_type == SOCK_STREAM){
				/**
				 * Определяем режим применяемой операции
				 */
				switch(static_cast <uint8_t> (mode)){
					// Если необходимо активировать алгоритм TCP/CORK
					case static_cast <uint8_t> (mode_t::ENABLED):
						// Выполняем активирование алгоритма TCP/CORK
						result = this->_addr->_socket.cork(this->_addr->sock, socket_t::mode_t::ENABLED);
					break;
					// Если необходимо деактивировать алгоритм TCP/CORK
					case static_cast <uint8_t> (mode_t::DISABLED):
						// Выполняем деактивирование алгоритма TCP/CORK
						result = this->_addr->_socket.cork(this->_addr->sock, socket_t::mode_t::DISABLED);
					break;
				}
			}
		}
	}
	// Сообщаем что сокет в блокирующем режиме
	return result;
}
/**
 * @brief Метод отключения/включения алгоритма Нейгла
 *
 * @param mode режим применимой операции
 * @return     результат выполенния операции
 */
bool awh::Engine::Context::nodelay(const mode_t mode) noexcept {
	// Результат работы функции
	bool result = false;
	// Если адрес присвоен
	if(this->_addr != nullptr){
		// Если сокет инициализирован
		if(this->_addr->sock != INVALID_SOCKET){
			// Если сокет установлен TCP/IP
			if(this->_addr->_type == SOCK_STREAM){
				/**
				 * Определяем режим применяемой операции
				 */
				switch(static_cast <uint8_t> (mode)){
					// Если необходимо деактивировать алгоритм Нейгла
					case static_cast <uint8_t> (mode_t::ENABLED):
						// Выполняем деактивирование алгоритма Нейгла
						result = this->_addr->_socket.nodelay(this->_addr->sock, socket_t::mode_t::ENABLED);
					break;
					// Если необходимо активировать алгоритм Нейгла
					case static_cast <uint8_t> (mode_t::DISABLED):
						// Выполняем активирование алгоритма Нейгла
						result = this->_addr->_socket.nodelay(this->_addr->sock, socket_t::mode_t::DISABLED);
					break;
				}
			}
		}
	}
	// Сообщаем что сокет в блокирующем режиме
	return result;
}
/**
 * @brief Метод извлечения поддерживаемого протокола
 *
 * @return поддерживаемый протокол подключения
 */
awh::Engine::proto_t awh::Engine::Context::proto() const noexcept {
	// Извлекаем поддерживаемый протокол
	return this->_proto;
}
/**
 * @brief Метод установки поддерживаемого протокола
 *
 * @param proto устанавливаемый протокол
 */
void awh::Engine::Context::proto(const proto_t proto) noexcept {
	// Выполняем установку поддерживаемого протокола
	this->_proto = proto;
}
/**
 * @brief Метод установки таймаута
 *
 * @param msec   количество миллисекунд
 * @param method метод для установки таймаута
 * @return       результат установки таймаута
 */
bool awh::Engine::Context::timeout(const uint32_t msec, const method_t method) noexcept {
	// Если адрес присвоен
	if(this->_addr != nullptr){
		// Если защищённый режим работы разрешён
		if(this->_addr->sock != INVALID_SOCKET){
			/**
			 * Определяем тип метода
			 */
			switch(static_cast <uint8_t> (method)){
				// Если установлен метод чтения
				case static_cast <uint8_t> (method_t::READ):
					// Выполняем установку таймера на чтение данных из сокета
					return this->_addr->_socket.timeout(this->_addr->sock, msec, socket_t::mode_t::READ);
				// Если установлен метод записи
				case static_cast <uint8_t> (method_t::WRITE):
					// Выполняем установку таймера на запись данных в сокет
					return this->_addr->_socket.timeout(this->_addr->sock, msec, socket_t::mode_t::WRITE);
			}
		}
	}
	// Сообщаем, что операция не выполнена
	return false;
}
/**
 * @brief Метод получения размеров буфера
 *
 * @param method метод для выполнения операции с буфером
 * @return       размер буфера
 */
int32_t awh::Engine::Context::buffer(const method_t method) const noexcept {
	// Результат работы функции
	int32_t result = 0;
	// Если адрес присвоен
	if(this->_addr != nullptr){
		// Если защищённый режим работы разрешён
		if(this->_addr->sock != INVALID_SOCKET){
			/**
			 * Определяем метод для работы с буфером
			 */
			switch(static_cast <uint8_t> (method)){
				// Если метод чтения
				case static_cast <uint8_t> (method_t::READ):
					// Получаем размер буфера для чтения
					result = this->_addr->_socket.bufferSize(this->_addr->sock, socket_t::mode_t::READ);
				break;
				// Если метод записи
				case static_cast <uint8_t> (method_t::WRITE):
					// Получаем размер буфера для записи
					result = this->_addr->_socket.bufferSize(this->_addr->sock, socket_t::mode_t::WRITE);
					/*
					// Если защищённый режим работы разрешён
					if(this->_encrypted && (this->_ssl != nullptr))
						// Выполняем определение минимального размера буфера который возможно отправить
						result = min(result, BIO_get_write_guarantee(this->_bio));
					*/
				break;
			}
		}
	}
	// Выводим результат
	return result;
}
/**
 * @brief Метод установки размеров буфера
 *
 * @param read  размер буфера на чтение
 * @param write размер буфера на запись
 * @return      результат работы функции
 */
bool awh::Engine::Context::buffer(const int32_t read, const int32_t write) noexcept {
	// Если адрес присвоен
	if((this->_addr != nullptr) && (read > 0) && (write > 0)){
		// Если сокет удачно инициализирован
		if(this->_addr->sock != INVALID_SOCKET){
			// Выполняем установку буферов сокета на чтение и запись
			return (
				this->_addr->_socket.bufferSize(this->_addr->sock, read, socket_t::mode_t::READ) &&
				this->_addr->_socket.bufferSize(this->_addr->sock, write, socket_t::mode_t::WRITE)
			);
		}
	}
	// Иначе возвращаем неустановленный размер буфера
	return false;
}
/**
 * @brief Метод проверки количества находящихся байт в сокете
 *
 * @param method метод для выполнения операции
 * @return       количество байт в сокете
 */
int32_t awh::Engine::Context::availability(const method_t method) const noexcept {
	// Результат работы функции
	int32_t result = 0;
	// Если адрес присвоен
	if(this->_addr != nullptr){
		// Если сокет инициализирован
		if(this->_addr->sock != INVALID_SOCKET){
			/**
			 * Определяем метод для работы с буфером
			 */
			switch(static_cast <uint8_t> (method)){
				// Если метод чтения
				case static_cast <uint8_t> (method_t::READ): {
					// Получаем размер данных в буфере для чтения
					result = static_cast <int32_t> (this->_addr->_socket.availability(this->_addr->sock, socket_t::mode_t::READ));
					/**
					 * Открытый текст, принятый связью и ещё не отданный вызывающему, лежит
					 * в накопителе контекста: в буфере сокета его уже нет, но читателю он доступен
					 */
					if((this->_tls != nullptr) && !this->_tls->plain.empty())
						// Добавляем неотданный остаток открытого текста
						result += static_cast <int32_t> (this->_tls->plain.size());
				} break;
				// Если метод записи
				case static_cast <uint8_t> (method_t::WRITE): {
					// Получаем размер буфере для записи
					result = static_cast <int32_t> (this->_addr->_socket.availability(this->_addr->sock, socket_t::mode_t::WRITE));
					// Если связь инициализирована, добавляем байты, которые она ещё не передала
					if((this->_tls != nullptr) && (this->_type == type_t::SERVER)){
						// Размер неотправленного накопителя связи
						const size_t size = this->_tls->server.output().size;
						// Если накопитель заполнен, учитываем его
						if(size > 0)
							// Добавляем неотправленные байты
							result += static_cast <int32_t> (size);
					// Если связь клиентская, берём её накопитель
					} else if(this->_tls != nullptr){
						// Размер неотправленного накопителя связи
						const size_t size = this->_tls->client.output().size;
						// Если накопитель заполнен, учитываем его
						if(size > 0)
							// Добавляем неотправленные байты
							result += static_cast <int32_t> (size);
					}
				} break;
			}
		}
	}
	// Выводим результат
	return result;
}
/**
 * @brief Конструктор
 *
 * @param fmk объект фреймворка
 * @param log объект для работы с логами
 *
 */
awh::Engine::Context::Context(const fmk_t * fmk, const log_t * log) noexcept :
 _verb(false), _encrypted(false), _type(type_t::NONE), _proto(proto_t::RAW),
 _tls(nullptr), _addr(nullptr), _fmk(fmk), _log(log) {}

/**
 * @brief Метод выполнения выбора следующего протокола
 *
 * @param out     буфер назначения
 * @param outSize размер буфера назначения
 * @param in      буфер входящих данных
 * @param inSize  размер буфера входящих данных
 * @param key     ключ копирования
 * @param keySize размер ключа для копирования
 * @return        результат переключения протокола
 */
bool awh::Engine::Context::selectProto(uint8_t ** out, uint8_t * outSize, const uint8_t * in, uint32_t inSize, const char * key, uint32_t keySize) const noexcept {
	/**
	 * Ключ передаётся с байтом длины впереди ("\x2h2"), keySize это длина самого имени протокола.
	 * Сравниваем длину и имя целиком: раньше сравнивались лишь keySize байт вместе с префиксом,
	 * и "h2" совпадал с "h3". Выход записи за границу входящего буфера также проверяется.
	 */
	for(uint32_t i = 0; i < inSize; i += (static_cast <uint32_t> (in[i]) + 1)){
		// Если запись протокола выходит за пределы входящего буфера, выходим
		if((i + 1 + static_cast <uint32_t> (in[i])) > inSize)
			// Выходим из цикла
			break;
		// Если длина и имя протокола совпадают с ключом
		if((static_cast <uint32_t> (in[i]) == keySize) && (::memcmp(&in[i + 1], key + 1, keySize) == 0)){
			// Выполняем установку размеров исходящего буфера
			(* outSize) = in[i];
			// Выполняем установку полученных данных в исходящий буфер
			(* out) = const_cast <uint8_t *> (&in[i + 1]);
			// Выходим из функции
			return true;
		}
	}
	// Выводим результат
	return false;
}
 /**
 * @brief Деструктор
 *
 */
awh::Engine::Context::~Context() noexcept {
	// Выполняем очистку выделенных ранее данных
	this->clear();
}
/**
 * @brief Метод установки подлинности стороны связи по файлам сертификатов
 *
 * Подготовка ключа RSA стоит вдвое дороже самой подписи, а ключа ECDSA — вдвое дороже всего
 * рукопожатия, поэтому подготовленная подлинность разделяется всеми связями одной пары файлов
 *
 * @param target контекст назначения
 * @param cert   адреса файлов сертификатов
 * @return       результат установки
 */
bool awh::Engine::credential(ctx_t & target, const cert_t & cert) noexcept {
	// Результат работы функции
	bool result = false;
	// Если связь и адрес файла цепочки сертификатов переданы
	if((target._tls != nullptr) && !cert.pem.empty()){
		// Запись запаса подлинностей, ключом которой служит пара адресов файлов
		const string mark = this->_fmk->format("%s\n%s", cert.pem.c_str(), cert.key.c_str());
		// Разобранные и подготовленные файлы цепочки и ключа
		auto i = this->_keys->held.find(mark);
		// Если подлинность этих файлов ещё не готовилась, разбираем и готовим её
		if(i == this->_keys->held.end()){
			// Разбираемая подлинность стороны связи
			nts::context::credential_t credential;
			// Прочитанные байты файла цепочки сертификатов
			const vector <char> pem = this->_fs.read(cert.pem);
			// Если текст цепочки сертификатов не прочитался либо не разобрался
			if(pem.empty() || (nts::pem::certificates(string(pem.begin(), pem.end()), credential.chain) != nts::pem::error_t::NONE) || credential.chain.empty()){
				// Выводим в лог сообщение о нечитаемом сертификате
				this->_log->print("Certificate cannot be set: %s", log_t::flag_t::CRITICAL, cert.pem.c_str());
				// Выводим признак того, что подлинность не установлена
				return false;
			}
			// Если приватный ключ сертификата задан
			if(!cert.key.empty()){
				// Прочитанные байты файла приватного ключа
				const vector <char> secret = this->_fs.read(cert.key);
				// Если текст приватного ключа не прочитался
				if(secret.empty()){
					// Выводим в лог сообщение о нечитаемом ключе
					this->_log->print("Private key cannot be set: %s", log_t::flag_t::CRITICAL, cert.key.c_str());
					// Выводим признак того, что подлинность не установлена
					return false;
				}
				/**
				 * Секретный ключ передаётся наружному модулю текстом PEM: разбор обёртки
				 * и вида ключа делает сама библиотека, а заготовку подписи — prepare()
				 */
				credential.secret.assign(secret.begin(), secret.end());
				// Отказ подготовки секретного ключа
				const nts::context::error_t error = credential.prepare();
				// Если ключ не подготовлен, подлинность не годится
				if(error != nts::context::error_t::NONE){
					// Выводим в лог сообщение об отказе подготовки ключа
					this->_log->print("Private key is not valid: %s code=%hhu", log_t::flag_t::CRITICAL, cert.key.c_str(), static_cast <uint8_t> (error));
					// Выводим признак того, что подлинность не установлена
					return false;
				}
			}
			// Выполняем установку подготовленной подлинности в запас
			i = this->_keys->held.emplace(mark, std::move(credential)).first;
		}
		// Устанавливаем подлинность в связь активного рода
		if(target._type == type_t::SERVER)
			// Выполняем установку подлинности связи сервера
			target._tls->server.credential(i->second);
		else target._tls->client.credential(i->second);
		// Запоминаем, что подлинность установлена
		result = true;
	}
	// Выводим результат
	return result;
}
/**
 * @brief Метод выбора подлинности сервера по имени, запрошенному клиентом (SNI)
 *
 * Наружный модуль разбирает приветствие и складывает ответ одним шагом, поэтому имя
 * извлекается из байтов канала до подачи их связи — собранием занимается helloBody()
 *
 * @param target контекст назначения
 * @param name   имя, предъявленное клиентом
 * @return       результат выбора
 */
bool awh::Engine::sniSelect(ctx_t & target, const string & name) noexcept {
	// Результат работы функции
	bool result = false;
	// Если сертификаты по доменным именам заданы и имя предъявлено
	if((target._tls != nullptr) && !this->_sni.empty() && !name.empty()){
		// Доменное имя в нижнем регистре
		string host = name;
		// Переводим доменное имя в нижний регистр
		this->_fmk->transform(host, fmk_t::transform_t::LOWER);
		// Выполняем поиск подлинности по точному имени
		auto i = this->_sni.find(host);
		// Если по точному имени подлинность не найдена
		if(i == this->_sni.end()){
			// Позиция первой точки в доменном имени
			const size_t pos = host.find('.');
			// Если домен содержит поддомен
			if(pos != string::npos)
				// Выполняем поиск подлинности по маске домена
				i = this->_sni.find(this->_fmk->format("*%s", host.substr(pos).c_str()));
		}
		/**
		 * Для неизвестного домена остаётся подлинность по умолчанию: рукопожатие
		 * продолжается ею, как это делал OpenSSL, не предъявляя фатального извещения
		 */
		if(i != this->_sni.end()){
			// Выполняем установку подлинности по найденным файлам
			result = this->credential(target, i->second);
			// Если подлинность установить не удалось, рукопожатие прерываем
			if(!result){
				// Выводим в лог сообщение о невозможности выбрать подлинность
				this->_log->print("Certificate for host %s cannot be set", log_t::flag_t::CRITICAL, host.c_str());
				// Запоминаем, что связь дальше вести нельзя
				target._tls->fatal = true;
			}
		}
	}
	// Выводим результат
	return result;
}
/**
 * @brief Метод проверки доменного имени по цепочке сертификатов собеседника
 *
 * Раздел 6.4 RFC 6125: имя разыскивается среди дополнительных имён владельца, а общее имя
 * берётся лишь тогда, когда дополнительных имён нет вовсе; звёздочка дозволена лишь самой
 * левой частью и лишь целой частью — это и делает nts::trust::belongs()
 *
 * @param host  доменное имя
 * @param chain цепочка сертификатов собеседника записями DER
 * @return      результат проверки
 */
awh::Engine::validate_t awh::Engine::validateHostname(const string & host, const vector <vector <uint8_t>> & chain) const noexcept {
	// Результат работы функции
	validate_t result = validate_t::Error;
	// Если хост передан и цепочка сертификатов получена
	if(!host.empty() && !chain.empty()){
		// Отказ сличения имени узла с именами сертификата
		const nts::trust::error_t error = nts::trust::belongs(nts::view_t(chain.front().data(), chain.front().size()), host);
		/**
		 * Определяем полученный отказ
		 */
		switch(static_cast <uint8_t> (error)){
			// Если имя узла отвечает имени сертификата
			case static_cast <uint8_t> (nts::trust::error_t::NONE): result = validate_t::MatchFound; break;
			// Если имя узла ни одному имени сертификата не отвечает
			case static_cast <uint8_t> (nts::trust::error_t::NAME): result = validate_t::MatchNotFound; break;
			// Если сертификат цепочки разобрать не удалось
			case static_cast <uint8_t> (nts::trust::error_t::PARSE): result = validate_t::MalformedCertificate; break;
			// Для остальных отказов
			default: result = validate_t::Error; break;
		}
	}
	// Выводим результат
	return result;
}
/**
 * @brief Метод добавления в запас доверенных сертификатов записей из файла
 *
 * @param fs     объект работы с файловой системой
 * @param log    объект работы с логами
 * @param store  запас доверенных сертификатов
 * @param file   адрес файла сертификатов текстом PEM
 * @return       результат добавления
 */
static bool storeCAFile(const awh::fs_t & fs, const awh::log_t * log, nts::trust::store_t & store, const string & file) noexcept {
	// Результат работы функции
	bool result = false;
	// Прочитанные байты файла сертификатов
	const vector <char> pem = fs.read(file);
	// Разобранные записи сертификатов
	vector <vector <uint8_t>> chain;
	// Если текст файла прочитался и разобрался
	if(!pem.empty() && (nts::pem::certificates(string(pem.begin(), pem.end()), chain) == nts::pem::error_t::NONE)){
		/**
		 * Переход по разобранным записям	
		 */
		for(auto & item : chain){
			// Если запись добавлена в запас, запоминаем успех
			if(store.add(nts::view_t(item.data(), item.size())))
				// Запоминаем, что в запас что-то добавлено
				result = true;
		}
	// Если файл читается, но сертификатов в нём не оказалось, выводим сообщение
	} else if(!pem.empty()) log->print("CAfile is invalid: %s", awh::log_t::flag_t::CRITICAL, file.c_str());
	// Выводим результат
	return result;
}
/**
 * Для операционной системы MS Windows
 */
#if _WIN32 || _WIN64
	/**
	 * @brief Метод добавления в запас доверенных сертификатов системного хранилища
	 *
	 * @param store запас доверенных сертификатов
	 * @param log   объект работы с логами
	 * @param name  название разделителя системного хранилища
	 * @return      результат добавления
	 */
	static bool storeCAWindows(nts::trust::store_t & store, const awh::log_t * log, const char * name) noexcept {
		// Результат работы функции
		bool result = false;
		// Открываем системное хранилище сертификатов
		HCERTSTORE sys = ::CertOpenSystemStore(0, name);
		// Если системное хранилище не открыто
		if(sys == nullptr){
			// Выводим в лог сообщение о недоступном хранилище
			log->print("Failed to open system certificate store", log_t::flag_t::CRITICAL);
			// Выводим признак того, что запас не пополнен
			return false;
		}
		/**
		 * Перебираем все сертификаты системного хранилища	
		 */
		for(PCCERT_CONTEXT ctx = nullptr; (ctx = ::CertEnumCertificatesInStore(sys, ctx)) != nullptr;){
			// Если запись сертификата добавлена в запас, запоминаем успех
			if(store.add(nts::view_t(ctx->pbCertEncoded, static_cast <size_t> (ctx->cbCertEncoded))))
				// Запоминаем, что в запас что-то добавлено
				result = true;
		}
		// Закрываем системное хранилище
		::CertCloseStore(sys, 0);
		// Выводим результат
		return result;
	}
#endif
/**
 * @brief Метод инициализации запаса доверенных сертификатов
 *
 * Прежний OpenSSL подставлял своё встроенное расположение корневых сертификатов; теперь
 * расположения перебирает сам движок, а на MS Windows корни берутся из системного хранилища
 *
 * @param target контекст назначения
 * @return       результат инициализации
 *
 */
bool awh::Engine::storeCA(ctx_t & target) const noexcept {
	// Результат работы функции
	bool result = false;
	// Если связь инициализирована
	if(target._tls != nullptr){
		// Запас доверенных сертификатов связи
		nts::trust::store_t & store = target._tls->store;
		/**
		 * Если указан адрес файла сертификатов центра сертификации либо каталога с ними,
		 * разбираем только их: явный заказ приложения важнее расположений системы
		 */
		if(!this->_cert.ca.empty() || !this->_cert.path.empty()){
			// Если адрес CA-файла существует как файл
			if(!this->_cert.ca.empty() && this->_fs.isFile(this->_cert.ca)){
				// Выполняем добавление сертификатов из файла в запас
				result = storeCAFile(this->_fs, this->_log, store, this->_cert.ca);
			// Если указан каталог с сертификатами центра сертификации, перебираем его содержимое
			} else if(!this->_cert.path.empty() && this->_fs.isDir(this->_cert.path)){
				// Выполняем обход каталога с сертификатами центра сертификации
				static_cast <void> (this->_fs.walkDir(this->_cert.path, "", false, [this, &store, &result](const string & name) noexcept -> bool {
					// Если файл сертификатов разобрался и пополнил запас, запоминаем это
					if(storeCAFile(this->_fs, this->_log, store, name))
						// Запоминаем, что в запас что-то добавлено
						result = true;
					// Продолжаем обход каталога
					return true;
				}));
			// Если указанный адрес не найден, возвращаемся к расположениям системы
			} else this->_log->print("CAfile is not found: %s", log_t::flag_t::CRITICAL, (!this->_cert.ca.empty() ? this->_cert.ca.c_str() : this->_cert.path.c_str()));
		}
		// Если явный заказ не задал или не нашёлся, перебираем расположения системы
		if(!result){
			/**
			 * Для операционной системы MS Windows запас доверенных берётся из системного
			 * хранилища сертификатов, связки файлов там не используются
			 */
			#if _WIN32 || _WIN64
				// Перечень разделителей системного хранилища корневых сертификатов
				static const char * stores[] = {"CA", "AuthRoot", "ROOT"};
				/**
				 * Переход по разделителям системного хранилища			
				 */
				for(const char * name : stores){
					// Если разделитель пополнил запас, запоминаем это
					if(storeCAWindows(store, this->_log, name))
						// Запоминаем, что запас собран
						result = true;
				}
			/**
			 * Для операционной системы не являющейся MS Windows
			 */
			#else
				// Перечень адресов файлов корневых сертификатов этой системы
				vector <string> files;
				// Если адрес файла корневых сертификатов задан сборкой, он идёт первым
				if(SSL_CA_FILE[0])
					// Добавляем адрес файла корневых сертификатов сборки
					files.push_back(SSL_CA_FILE);
				/**
				 * Расположения связки корневых сертификатов, принятые в самих системах
				 */
				#if __linux__ || __unix || __unix__
					// Добавляем связку Debian, Alpine и Gentoo
					files.push_back("/etc/ssl/certs/ca-certificates.crt");
					// Добавляем связку Red Hat и Fedora
					files.push_back("/etc/pki/tls/certs/ca-bundle.crt");
				#endif
				/**
				 * Для операционной системы MacOS X, FreeBSD, OpenBSD или NetBSD
				 */
				#if __APPLE__ || __MACH__ || __FreeBSD__ || __OpenBSD__ || __NetBSD__
					// Добавляем системный файл корневых сертификатов
					files.push_back("/etc/ssl/cert.pem");
					// Добавляем связку, устанавливаемую пакетами
					files.push_back("/usr/local/share/ca-certificates/ca-certificates.crt");
				#endif
				/**
				 * Переход по перечню адресов файлов			
				 */
				for(const string & file : files){
					// Если файл существует и разобрался, останавливаем перебор
					if(this->_fs.isFile(file) && storeCAFile(this->_fs, this->_log, store, file)){
						// Запоминаем, что запас собран
						result = true;
						// Выходим из перебора
						break;
					}
				}
			#endif
		}
		// Если запас доверенных сертификатов остался пуст, предупреждаем: проверка не пройдёт
		if(!result)
			// Выводим в лог сообщение об отсутствии доверенных сертификатов
			this->_log->print("There is no trusted certificates: set the CA file or install the system bundle", log_t::flag_t::CRITICAL);
	}
	// Выводим результат
	return result;
}
/**
 * @brief Метод ожидания рукопожатия
 *
 * @param target контекст назначения
 * @return       результат проверки
 */
bool awh::Engine::wait(ctx_t & target) noexcept {
	/**
	 * Определяем тип входящего сокета
	 */
	switch(target._addr->_type){
		// Если сокет установлен TCP/IP
		case SOCK_STREAM:
			/**
			 * Прежняя SSL_stateless() отвечала, что рукопожатие ещё не началось:
			 * приветствие своей стороны ещё не отправлено
			 */
			return ((target._tls != nullptr) && !target._tls->started);
		/**
		 * Дейтаграммы: разбор приветствия без состояния (печёнки и адрес клиента)
		 * переносится на круг 2 — наружный модуль ведёт печёнки сам, и вызову нужен
		 * накопитель принятой дейтаграммы вместе с адресом собеседника
		 */
		case SOCK_DGRAM: {
			// Выводим в лог сообщение о том, что ожидание без состояния не готово
			this->_log->print("DTLS stateless listen is not implemented yet", log_t::flag_t::WARNING);
		} break;
	}
	// Сообщаем, что ничего не обработано
	return false;
}
/**
 * @brief Метод проверки на активацию режима шифрования
 *
 * @param ctx  контекст подключения
 * @return     результат проверки
 */
bool awh::Engine::encrypted(ctx_t & ctx) const noexcept {
	// Выводим результат проверки
	return ctx._encrypted;
}
/**
 * @brief Метод установки флага режима шифрования
 *
 * @param mode флаг режима шифрования
 * @param ctx  контекст подключения
 */
void awh::Engine::encrypted(const bool mode, ctx_t & ctx) noexcept {
	// Устанавливаем флаг шифрования
	ctx._encrypted = mode;
}
/**
 * @brief Метод извлечения активного протокола
 *
 * @param target контекст назначения
 * @return       метод активного протокола
 */
awh::Engine::proto_t awh::Engine::proto(ctx_t & target) const noexcept {
	// Результат работы функции
	proto_t result = proto_t::HTTP1_1;
	// Если связь инициализирована
	if(target._tls != nullptr){
		// Желаемый протокол подключения
		result = target._proto;
		/**
		 * Продвигаем рукопожатие, но его результат намеренно не проверяем: согласованный
		 * прикладной протокол определяется только по ALPN. Проверкой «> 0» здесь не обойтись:
		 * на сервере ALPN выбирается уже по приветствию клиента, до завершения рукопожатия
		 */
		static_cast <void> (target.pump(false));
		/**
		 * Определяем желаемый активный протокол
		 */
		switch(static_cast <uint8_t> (target._proto)){
			// Если протокол соответствует SPDY/1
			case static_cast <uint8_t> (proto_t::SPDY1):
			// Если протокол соответствует HTTP/2
			case static_cast <uint8_t> (proto_t::HTTP2):
			// Если протокол соответствует HTTP/3
			case static_cast <uint8_t> (proto_t::HTTP3): {
				// Название прикладного протокола, согласованного связью
				const string & name = ((target._type == type_t::SERVER) ? target._tls->server.protocol() : target._tls->client.protocol());
				/**
				 * Определяем согласованное название
				 */
				switch(static_cast <uint8_t> (target._proto)){
					// Если протокол соответствует SPDY/1
					case static_cast <uint8_t> (proto_t::SPDY1):
						// Если активный протокол не соответствует протоколу SPDY/1
						if(name != "spdy/1")
							// Устанавливаем активный протокол
							return proto_t::HTTP1_1;
					break;
					// Если протокол соответствует HTTP/2
					case static_cast <uint8_t> (proto_t::HTTP2):
						// Если активный протокол не соответствует протоколу HTTP/2
						if(name != "h2")
							// Устанавливаем активный протокол
							return proto_t::HTTP1_1;
					break;
					// Если протокол соответствует HTTP/3
					case static_cast <uint8_t> (proto_t::HTTP3):
						// Если активный протокол не соответствует протоколу HTTP/3
						if(name != "h3")
							// Устанавливаем активный протокол
							return proto_t::HTTP1_1;
					break;
				}
			} break;
		}
	}
	// Выводим результат
	return result;
}
/**
 * @brief Метод активации протокола HTTP
 *
 * @param target контекст назначения
 */
void awh::Engine::httpUpgrade(ctx_t & target) const noexcept {
	// Если список поддерживаемых протоколов не установлен
	if(target.protocols.empty()){
		// Создаём идентификатор протокола HTTP/2
		const string http2 = "h2";
		// Создаём идентификатор протокола HTTP/3
		const string http3 = "h3";
		// Создаём идентификатор протокола SPDY/1
		const string spdy1 = "spdy/1";
		// Создаём идентификатор протокола HTTP/1
		const string http1 = "http/1";
		// Создаём идентификатор протокола HTTP/1.1
		const string http1_1 = "http/1.1";
		/**
		 * Если протоколом является HTTP, выполняем переключение на него
		 */
		switch(static_cast <uint8_t> (target._proto)){
			// Если протокол соответствует SPDY/1
			case static_cast <uint8_t> (proto_t::SPDY1): {
				// Устанавливаем количество символов для протокола SPDY/1
				target.protocols.push_back(static_cast <uint8_t> (spdy1.size()));
				// Устанавливаем идентификатор протокола SPDY/1
				target.protocols.insert(target.protocols.end(), spdy1.begin(), spdy1.end());
				// Устанавливаем количество символов для протокола HTTP/1
				target.protocols.push_back(static_cast <uint8_t> (http1.size()));
				// Устанавливаем идентификатор протокола HTTP/1
				target.protocols.insert(target.protocols.end(), http1.begin(), http1.end());
				// Устанавливаем количество символов для протокола HTTP/1.1
				target.protocols.push_back(static_cast <uint8_t> (http1_1.size()));
				// Устанавливаем идентификатор протокола HTTP/1.1
				target.protocols.insert(target.protocols.end(), http1_1.begin(), http1_1.end());
			} break;
			// Если протокол соответствует HTTP/1
			case static_cast <uint8_t> (proto_t::HTTP1): {
				// Устанавливаем количество символов для протокола HTTP/1
				target.protocols.push_back(static_cast <uint8_t> (http1.size()));
				// Устанавливаем идентификатор протокола HTTP/1
				target.protocols.insert(target.protocols.end(), http1.begin(), http1.end());
			} break;
			// Если протокол соответствует HTTP/2
			case static_cast <uint8_t> (proto_t::HTTP2): {
				// Устанавливаем количество символов для протокола HTTP/2
				target.protocols.push_back(static_cast <uint8_t> (http2.size()));
				// Устанавливаем идентификатор протокола HTTP/2
				target.protocols.insert(target.protocols.end(), http2.begin(), http2.end());
				// Устанавливаем количество символов для протокола SPDY/1
				target.protocols.push_back(static_cast <uint8_t> (spdy1.size()));
				// Устанавливаем идентификатор протокола SPDY/1
				target.protocols.insert(target.protocols.end(), spdy1.begin(), spdy1.end());
				// Устанавливаем количество символов для протокола HTTP/1
				target.protocols.push_back(static_cast <uint8_t> (http1.size()));
				// Устанавливаем идентификатор протокола HTTP/1
				target.protocols.insert(target.protocols.end(), http1.begin(), http1.end());
				// Устанавливаем количество символов для протокола HTTP/1.1
				target.protocols.push_back(static_cast <uint8_t> (http1_1.size()));
				// Устанавливаем идентификатор протокола HTTP/1.1
				target.protocols.insert(target.protocols.end(), http1_1.begin(), http1_1.end());
			} break;
			// Если протокол соответствует HTTP/3
			case static_cast <uint8_t> (proto_t::HTTP3): {
				// Устанавливаем количество символов для протокола HTTP/2
				target.protocols.push_back(static_cast <uint8_t> (http2.size()));
				// Устанавливаем идентификатор протокола HTTP/2
				target.protocols.insert(target.protocols.end(), http2.begin(), http2.end());
				// Устанавливаем количество символов для протокола HTTP/3
				target.protocols.push_back(static_cast <uint8_t> (http3.size()));
				// Устанавливаем идентификатор протокола HTTP/3
				target.protocols.insert(target.protocols.end(), http3.begin(), http3.end());
				// Устанавливаем количество символов для протокола SPDY/1
				target.protocols.push_back(static_cast <uint8_t> (spdy1.size()));
				// Устанавливаем идентификатор протокола SPDY/1
				target.protocols.insert(target.protocols.end(), spdy1.begin(), spdy1.end());
				// Устанавливаем количество символов для протокола HTTP/1
				target.protocols.push_back(static_cast <uint8_t> (http1.size()));
				// Устанавливаем идентификатор протокола HTTP/1
				target.protocols.insert(target.protocols.end(), http1.begin(), http1.end());
				// Устанавливаем количество символов для протокола HTTP/1.1
				target.protocols.push_back(static_cast <uint8_t> (http1_1.size()));
				// Устанавливаем идентификатор протокола HTTP/1.1
				target.protocols.insert(target.protocols.end(), http1_1.begin(), http1_1.end());
			} break;
			// Если протокол соответствует HTTP/1.1
			case static_cast <uint8_t> (proto_t::HTTP1_1): {
				// Устанавливаем количество символов для протокола HTTP/1
				target.protocols.push_back(static_cast <uint8_t> (http1.size()));
				// Устанавливаем идентификатор протокола HTTP/1
				target.protocols.insert(target.protocols.end(), http1.begin(), http1.end());
				// Устанавливаем количество символов для протокола HTTP/1.1
				target.protocols.push_back(static_cast <uint8_t> (http1_1.size()));
				// Устанавливаем идентификатор протокола HTTP/1.1
				target.protocols.insert(target.protocols.end(), http1_1.begin(), http1_1.end());
			} break;
		}
	}
	/**
	 * Прежний OpenSSL требовал колбэков выбора прикладного протокола с каждой стороны;
	 * наружный модуль выбирает сам по объявленному перечню, поэтому перечень уходит связи
	 * в wrapTLS(). Переговорщики NPN (draft-agl-tls-nextprotoneg) не включаем: их держал
	 * лишь OpenSSL до издания 3.0, а v4 собирался против 3.x, где их уже нет
	 */
}
/**
 * @brief Метод прикрепления контекста клиента к контексту сервера
 *
 * @param target  контекст назначения
 * @param address объект подключения
 */
void awh::Engine::attach(ctx_t & target, addr_t * address) noexcept {
	// Если данные переданы
	if(address != nullptr){
		// Устанавливаем сетевой сокет
		target._addr = address;
		// Устанавливаем тип приложения
		target._type = type_t::SERVER;
		/**
		 * Прежняя переналадка BIO (BIO_set_fd и BIO_CTRL_DGRAM_SET_CONNECTED) не нужна:
		 * насос сам берёт адрес собеседника из объекта подключения. Для дейтаграмм адрес
		 * связи передаёт наружному модулю отдельно — круг 2
		 */
	}
}
/**
 * @brief Метод заведения связи наружного модуля шифрования
 *
 * Прежний OpenSSL создавал пару SSL_CTX и SSL и вешал на них колбэки; наружный модуль
 * настраивается целиком одним контекстом, поэтому здесь остаётся только перечень настроек
 *
 * @param target контекст назначения
 * @param type   тип активного приложения
 * @param host   хост удалённого сервера
 * @return       результат заведения связи
 *
 */
bool awh::Engine::wrapTLS(ctx_t & target, const type_t type, const string & host) noexcept {
	// Результат работы функции
	bool result = false;
	// Если объект подключения и сокет переданы
	if((target._addr != nullptr) && (target._addr->sock != INVALID_SOCKET)){
		// Выполняем создание состояния связи
		target._tls = std::make_unique <ctx_t::Tls>();
		// Запоминаем движок: по нему связь выбирает подлинность для имени клиента
		target._tls->engine = this;
		// Устанавливаем адрес удалённого сервера для SNI и сверки имени сертификата
		target._tls->host = host;
		// Устанавливаем признак проверки доверия цепочке собеседника
		target._tls->verify = this->_verify;
		/**
		 * Определяем перенос связи: дейтаграммы ведут DTLS, потоки — TLS.
		 * SCTP у v4 заводится только как SOCK_STREAM, поэтому шифруется как обычный TCP
		 */
		const nts::context::transport_t transport = ((target._addr->_type == SOCK_DGRAM) ? nts::context::transport_t::DATAGRAMS : nts::context::transport_t::RECORDS);
		// Перечень наборов шифрования для связи
		vector <nts::protect::suite_t> suites;
		// Выполняем разбор перечня наборов шифрования из строки настроек
		suitesList(this->_cipher, suites, this->_log);
		// Если список настроек пуст, а связь серверная, берём серверный набор по умолчанию
		if(suites.empty() && (type == type_t::SERVER))
			// Выполняем установку серверных наборов по умолчанию
			serverSuites(suites);
		// Перечень групп обмена ключами
		vector <nts::extension::group_t> groups;
		// Выполняем установку перечня групп по умолчанию
		groupList(groups);
		// Перечень прикладных протоколов, объявляемых связью
		vector <string> protocols;
		// Выполняем разбор перечня прикладных протоколов
		protocolList(target.protocols, protocols);
		/**
		 * Определяем род связи
		 */
		if(type == type_t::SERVER){
			// Устанавливаем перенос связи
			target._tls->server.transport(transport);
			// Выполняем установку пределов изданий протокола
			result = (target._tls->server.versions(TLS_VERSION_12, TLS_VERSION_13) == nts::context::error_t::NONE);
			// Выполняем установку групп обмена ключами
			target._tls->server.groups(groups);
			// Если перечень наборов шифрования собран, устанавливаем его
			if(!suites.empty())
				// Выполняем установку наборов шифрования
				target._tls->server.suites(suites);
			// Если перечень прикладных протоколов задан, объявляем его
			if(!protocols.empty())
				// Выполняем установку прикладных протоколов
				target._tls->server.protocols(protocols);
			// Если подлинность сервера задана файлами сертификатов, устанавливаем её
			if(result && !this->_cert.pem.empty())
				// Выполняем установку подлинности по умолчанию
				result = this->credential(target, this->_cert);
			/**
			 * Если разрешена проверка сертификата клиента, собираем запас доверенных:
			 * наружный модуль цепочку не судит, но запас нужен и для сверки имени издателя
			 */
			if(result && this->_verify){
				// Выполняем инициализацию запаса доверенных сертификатов
				static_cast <void> (this->storeCA(target));
				// Устанавливаем запас доверенных сертификатов связи
				target._tls->server.verification(true, 0, target._tls->store);
			}
			// Если указан файл отозванных сертификатов, предупреждаем: отзыв ожидает наружный модуль
			if(!this->_cert.crl.empty())
				// Выводим в лог сообщение об ожидающей проверке отзыва
				this->_log->print("CRL checking is not supported by the cipher module yet", log_t::flag_t::WARNING);
			// Если подлинность сервера не задана, связь завести нельзя
			if(result && this->_cert.pem.empty()){
				// Выводим в лог сообщение об отсутствии сертификата сервера
				this->_log->print("There is no certificate for the server", log_t::flag_t::CRITICAL);
				// Сообщаем, что связь не заведена
				result = false;
			}
		// Если приложение является клиентом
		} else {
			// Устанавливаем перенос связи
			target._tls->client.transport(transport);
			// Выполняем установку пределов изданий протокола
			result = (target._tls->client.versions(TLS_VERSION_12, TLS_VERSION_13) == nts::context::error_t::NONE);
			// Выполняем установку групп и предложений обмена ключами
			target._tls->client.groups(groups);
			// Выполняем установку предложений групп
			target._tls->client.shares(groups);
			// Если перечень наборов шифрования собран, устанавливаем его
			if(!suites.empty())
				// Выполняем установку наборов шифрования
				target._tls->client.suites(suites);
			// Если перечень прикладных протоколов задан, объявляем его
			if(!protocols.empty())
				// Выполняем установку прикладных протоколов
				target._tls->client.protocols(protocols);
			/**
			 * Раздел 3 RFC 6066: адрес в расширении server_name не передают, поэтому имя
			 * отправляем только когда хост не является IP-адресом
			 */
			if(!host.empty() && (ipLiteral(host) == 0))
				// Устанавливаем имя узла, за которым пришли
				target._tls->client.name(host);
			// Выполняем инициализацию запаса доверенных сертификатов
			static_cast <void> (this->storeCA(target));
			// Устанавливаем проверку цепочки собеседника и запас доверенных
			target._tls->client.verification(this->_verify, 0, target._tls->store);
			// Если указан сертификат клиента, устанавливаем его подлинность
			if(!this->_cert.pem.empty())
				// Выполняем установку подлинности клиента
				result = this->credential(target, this->_cert);
			// Если указан файл отозванных сертификатов, предупреждаем: отзыв ожидает наружный модуль
			if(!this->_cert.crl.empty())
				// Выводим в лог сообщение об ожидающей проверке отзыва
				this->_log->print("CRL checking is not supported by the cipher module yet", log_t::flag_t::WARNING);
		}
		// Если связь заведена, запоминаем, что шифрование активно
		if(result){
			// Устанавливаем флаг инициализации шифрования
			target._encrypted = true;
			// Устанавливаем флаг активации TLS
			target._addr->_encrypted = true;
		// Если связь не завелась, очищаем состояние связи
		} else {
			// Выполняем очистку состояния связи
			target._tls.reset(nullptr);
			// Выводим в лог сообщение о невозможности завести связь
			this->_log->print("Could not create SSL/TLS session object", log_t::flag_t::CRITICAL);
		}
	}
	// Выводим результат
	return result;
}
/**
 * @brief Метод обертывания сетевого сокета для сервера
 *
 * @param target контекст назначения
 * @param source исходный контекст
 */
void awh::Engine::wrap(ctx_t & target, ctx_t & source) noexcept {
	// Если объект ещё не обёрнут в контекст шифрования
	if(!source._encrypted && (source._addr != nullptr))
		// Выполняем обёртывание уже активного подключения
		this->wrap(target, source._addr);
}
/**
 * @brief Метод обертывания сетевого сокета для сервера
 *
 * @param target  контекст назначения
 * @param address объект подключения
 */
void awh::Engine::wrap(ctx_t & target, addr_t * address) noexcept {
	// Если данные переданы
	if(address != nullptr){
		// Устанавливаем сетевой сокет
		target._addr = address;
		// Устанавливаем тип приложения
		target._type = type_t::SERVER;
		/**
		 * Определяем семейство протоколов сервера
		 */
		switch(target._addr->_peer.client.ss_family){
			// Если семейство протоколов IPv4
			case AF_INET:
			// Если семейство протоколов IPv6
			case AF_INET6: break;
			// Если семейство протоколов другое, выходим
			default: return;
		}
		// Если тип сокетов установлен не как потоковые, выходим
		if(target._addr->_type != SOCK_STREAM)
			// Выходим из функции
			return;
		/**
		 * Если протоколом является HTTP, выполняем переключение на него
		 */
		switch(static_cast <uint8_t> (target._proto)){
			// Если протокол соответствует SPDY/1
			case static_cast <uint8_t> (proto_t::SPDY1):
			// Если протокол соответствует HTTP/2
			case static_cast <uint8_t> (proto_t::HTTP2):
			// Если протокол соответствует HTTP/3
			case static_cast <uint8_t> (proto_t::HTTP3):
				// Выполняем переключение протокола подключения
				this->httpUpgrade(target);
			break;
		}
		// Выполняем заведение связи наружного модуля
		if(this->wrapTLS(target, type_t::SERVER, "")){
			// Устанавливаем неблокирующий режим ввода/вывода для сокета
			target.blocking(mode_t::DISABLED);
		// Если связь не завелась, очищаем контекст
		} else target.clear();
	}
}
/**
 * @brief Метод обертывания сетевого сокета для клиента/сервера
 *
 * @param target  контекст назначения
 * @param address объект подключения
 * @param type    тип активного приложения
 */
void awh::Engine::wrap(ctx_t & target, addr_t * address, const type_t type) noexcept {
	// Выполняем обёртывание без хоста удалённого сервера
	this->wrap(target, address, type, "");
}
/**
 * @brief Метод обертывания сетевого сокета для клиента/сервера
 *
 * Для клиента DTLS хост удалённого сервера передаётся в расширение SNI
 * и используется для проверки соответствия сертификата доменному имени
 *
 * @param target  контекст назначения
 * @param address объект подключения
 * @param type    тип активного приложения
 * @param host    хост удалённого сервера
 */
void awh::Engine::wrap(ctx_t & target, addr_t * address, const type_t type, const string & host) noexcept {
	// Если данные переданы
	if(address != nullptr){
		// Устанавливаем тип приложения
		target._type = type;
		// Устанавливаем сетевой сокет
		target._addr = address;
		// Если сокет инициализирован
		if(target._addr->sock != INVALID_SOCKET){
			/**
			 * Если протоколом является HTTP, выполняем переключение на него
			 */
			switch(static_cast <uint8_t> (type)){
				// Если приложение является сервером
				case static_cast <uint8_t> (type_t::SERVER): {
					switch(static_cast <uint8_t> (target._proto)){
						// Если протокол соответствует SPDY/1
						case static_cast <uint8_t> (proto_t::SPDY1):
						// Если протокол соответствует HTTP/2
						case static_cast <uint8_t> (proto_t::HTTP2):
						// Если протокол соответствует HTTP/3
						case static_cast <uint8_t> (proto_t::HTTP3):
							// Выполняем переключение протокола подключения
							this->httpUpgrade(target);
						break;
					}
				} break;
				// Если приложение является клиентом
				case static_cast <uint8_t> (type_t::CLIENT): {
					switch(static_cast <uint8_t> (target._proto)){
						// Если протокол соответствует SPDY/1
						case static_cast <uint8_t> (proto_t::SPDY1):
						// Если протокол соответствует HTTP/1
						case static_cast <uint8_t> (proto_t::HTTP1):
						// Если протокол соответствует HTTP/2
						case static_cast <uint8_t> (proto_t::HTTP2):
						// Если протокол соответствует HTTP/3
						case static_cast <uint8_t> (proto_t::HTTP3):
						// Если протокол соответствует HTTP/1.1
						case static_cast <uint8_t> (proto_t::HTTP1_1):
							// Выполняем переключение протокола подключения
							this->httpUpgrade(target);
						break;
					}
				} break;
			}
			// Выполняем заведение связи наружного модуля
			if(this->wrapTLS(target, type, host)){
				// Если приложение является сервером, переводим сокет в неблокирующий режим
				if(type == type_t::SERVER)
					// Устанавливаем неблокирующий режим ввода/вывода для сокета
					target.blocking(mode_t::DISABLED);
			// Если связь не завелась, очищаем контекст
			} else target.clear();
		// Если передан неверный сокет
		} else {
			// Очищаем созданный контекст
			target.clear();
			// Выводим сообщение, что сокет не был инициализирован
			this->_log->print("Socket is not initialized", log_t::flag_t::WARNING);
		}
	}
}
/**
 * @brief Метод обертывания сетевого сокета для клиента
 *
 * @param target контекст назначения
 * @param source исходный контекст
 * @param host   хост удалённого сервера
 */
void awh::Engine::wrap(ctx_t & target, ctx_t & source, const string & host) noexcept {
	// Если объект ещё не обёрнут в контекст шифрования
	if(!source._encrypted && (source._addr != nullptr))
		// Выполняем обёртывание уже активного подключения
		this->wrap(target, source._addr, type_t::CLIENT, host);
}
/**
 * @brief Метод обертывания сетевого сокета для клиента
 *
 * @param target  контекст назначения
 * @param address объект подключения
 * @param host    хост удалённого сервера
 */
void awh::Engine::wrap(ctx_t & target, addr_t * address, const string & host) noexcept {
	// Если данные переданы и хост удалённого сервера известен
	if((address != nullptr) && !host.empty())
		// Выполняем обёртывание сокета клиента
		this->wrap(target, address, type_t::CLIENT, host);
}
/**
 * @brief Метод разрешающий или запрещающий, выполнять проверку соответствия, сертификата домену
 *
 * @param mode флаг состояния разрешения проверки
 */
void awh::Engine::verify(const bool mode) noexcept {
	// Устанавливаем флаг проверки
	this->_verify = mode;
}
/**
 * @brief Метод установки алгоритмов шифрования
 *
 * @param ciphers список алгоритмов шифрования для установки
 */
void awh::Engine::ciphers(const vector <string> & ciphers) noexcept {
	// Если список алгоритмов шифрования передан
	if(!ciphers.empty()){
		// Очищаем установленный список алгоритмов шифрования
		this->_cipher.clear();
		// Переходим по всему списку алгоритмов шифрования
		for(auto & cip : ciphers){
			// Если список алгоритмов шифрования уже не пустой, вставляем разделитель
			if(!this->_cipher.empty())
				// Устанавливаем разделитель
				this->_cipher.append(":");
			// Устанавливаем алгоритм шифрования
			this->_cipher.append(cip);
		}
	}
}
/**
 * @brief Метод установки CRL-файла отозванных сертификатов центром сертификации
 *
 * @param crl адрес CRL-файла отозванных сертификатов центром сертификации
 */
void awh::Engine::crl(const string & crl) noexcept {
	// Если адрес CRL-файла передан
	if(!crl.empty())
		// Устанавливаем адрес сертификата (CRL-файла)
		this->_cert.crl = this->_fs.realPath(crl, false);
}
/**
 * @brief Метод установки сертификата центра сертификации (CA-файла)
 *
 * @param ca   адрес сертификата центра сертификации (CA-файла)
 * @param path адрес каталога где находится сертификат (CA-файл)
 */
void awh::Engine::ca(const string & ca, const string & path) noexcept {
	// Если адрес CA-файла передан
	if(!ca.empty())
		// Устанавливаем адрес сертификата центра сертификации (CA-файла)
		this->_cert.ca = this->_fs.realPath(ca, false);
	// Если адрес CA-файла не передан, выполняем очистку ранее установленного CA-файла
	else this->_cert.ca.clear();
	// Если адрес каталога с сертификатом центра сертификации (CA-файлом) передан, устанавливаем и его
	if(!path.empty() && this->_fs.isDir(path))
		// Устанавливаем адрес каталога с сертификатом центра сертификации (CA-файлом)
		this->_cert.path = this->_fs.realPath(path, false);
	// Если путь хранения CA-файлов не передан, выполняем очистку ранее установленного
	else this->_cert.path.clear();
}
/**
 * @brief Метод установки файлов сертификата
 *
 * @param pem файл цепочки сертификатов
 * @param key приватный ключ сертификата (если требуется)
 */
void awh::Engine::certificate(const string & pem, const string & key) noexcept {
	// Если ключ сертификата передан
	if(!key.empty())
		// Устанавливаем приватный ключ сертификата
		this->_cert.key = this->_fs.realPath(key, false);
	// Если ключ не передан, очищаем установленный ключ сертификата
	else this->_cert.key.clear();
	// Если сертификат передан
	if(!pem.empty())
		// Устанавливаем файл полной цепочки сертификатов
		this->_cert.pem = this->_fs.realPath(pem, false);
	// Если сертификат не передан, очищаем установленный адрес сертификата
	else this->_cert.pem.clear();
	// Если запас подлинностей уже собран, сбрасываем его: файлы сменились
	if(this->_keys != nullptr)
		// Выполняем очистку запаса подготовленных подлинностей
		this->_keys->held.clear();
}
/**
 * @brief Метод установки файлов сертификата сервера для доменного имени (SNI)
 *
 * Если сертификат по умолчанию ещё не задан, этот сертификат становится и сертификатом по умолчанию
 *
 * @param host доменное имя (допускается маска вида *.example.com)
 * @param pem  файл цепочки сертификатов
 * @param key  приватный ключ сертификата
 */
void awh::Engine::certificate(const string & host, const string & pem, const string & key) noexcept {
	// Если доменное имя передано
	if(!host.empty()){
		// Получаем доменное имя в нижнем регистре
		string name = host;
		// Переводим доменное имя в нижний регистр
		this->_fmk->transform(name, fmk_t::transform_t::LOWER);
		// Если сертификат и ключ переданы
		if(!pem.empty() && !key.empty()){
			// Создаём объект файлов сертификата
			cert_t cert;
			// Устанавливаем приватный ключ сертификата
			cert.key = this->_fs.realPath(key, false);
			// Устанавливаем файл полной цепочки сертификатов
			cert.pem = this->_fs.realPath(pem, false);
			// Если сертификат по умолчанию ещё не задан, первый сертификат доменного имени становится им
			if(this->_cert.pem.empty() && this->_cert.key.empty()){
				// Устанавливаем приватный ключ сертификата по умолчанию
				this->_cert.key = cert.key;
				// Устанавливаем файл цепочки сертификатов по умолчанию
				this->_cert.pem = cert.pem;
			}
			// Выполняем установку сертификата для доменного имени
			this->_sni[name] = std::move(cert);
		// Если сертификат не передан, удаляем сертификат доменного имени
		} else this->_sni.erase(name);
		// Если запас подлинностей уже собран, сбрасываем его: файлы сменились
		if(this->_keys != nullptr)
			// Выполняем очистку запаса подготовленных подлинностей
			this->_keys->held.clear();
	}
}
/**
 * @brief Конструктор
 *
 * @param fmk объект фреймворка
 * @param log объект для работы с логами
 * @param uri объект работы с URI
 */
awh::Engine::Engine(const fmk_t * fmk, const log_t * log, const uri_t * uri) noexcept :
 _verify(true), _fs(fmk, log), _cipher{""}, _keys(std::make_unique <Keys>()), _fmk(fmk), _uri(uri), _log(log) {
	/**
	 * Для операционной системы не являющейся MS Windows
	 */
	#if !_WIN32 && !_WIN64
		// Выполняем игнорирование сигналов SIGPIPE
		if(::signal(SIGPIPE, SIG_IGN) == SIG_ERR)
			// Выводим в лог сообщение
			this->_log->print("Failed to ignoring signal SIGPIPE", log_t::flag_t::CRITICAL);
	#endif
	/**
	 * Выполняем установку сертификата центра сертификации (CA-файла).
	 * Инициализации OpenSSL здесь больше нет: наружный модуль не требует глобальной
	 * подготовки, а его случайные значения берутся из энтропии системы сами
	 */
	this->_cert.ca = this->_fs.realPath(this->_cert.ca, false);
	/**
	 * Алгоритмы шифрования по умолчанию не задаются: сервер без списка от приложения берёт
	 * набор ECDHE с AEAD плюс наборы издания 1.3, клиент — набор наружного модуля по умолчанию.
	 * Прежний список допускал 3DES, CAMELLIA и обмен ключами RSA без прямой секретности
	 */
}
/**
 * @brief Деструктор
 *
 */
awh::Engine::~Engine() noexcept {
	/**
	 * Запас подготовленных подлинностей разрушается сам: ключ живёт shared_ptr,
	 * и связь, которой он установлен, держит свою ссылку
	 */
}
