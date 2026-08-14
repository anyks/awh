/**
 * @file ngtcp2.cpp
 * @date 2026-07-26
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
 * @brief Эталонный стенд сравнения транспорта QUIC на основе ngtcp2 —
 *        прогон пары соединений «клиент ↔ сервер» в памяти по синтетическим
 *        часам, повторяющий сценарии `benchmark/proto/quic` библиотеки AWH
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Стандартные заголовочные файлы
 */
#include <vector>
#include <string>
#include <cstring>
#include <netinet/in.h>

/**
 * Заголовочные файлы ngtcp2 и его криптографического бэкенда BoringSSL
 */
#include <ngtcp2/ngtcp2.h>
#include <ngtcp2/ngtcp2_crypto.h>
#include <ngtcp2/ngtcp2_crypto_boringssl.h>

/**
 * Заголовочные файлы BoringSSL
 */
#include <openssl/ssl.h>
#include <openssl/rand.h>
#include <openssl/err.h>

/**
 * Подключаем общее окружение стендов сравнения транспорта QUIC
 */
#include "common.hpp"

/**
 * Пути к тестовому сертификату и приватному ключу сервера задаются при сборке:
 * стенд их только загружает, к измеряемой работе рукопожатие отношения не имеет
 */
#ifndef QUIC_CERT_FILE
	#define QUIC_CERT_FILE "sh/certificates/server/cert.pem"
#endif
#ifndef QUIC_KEY_FILE
	#define QUIC_KEY_FILE "sh/certificates/server/key.pem"
#endif

/**
 * @brief Предлагаемый и выбираемый протокол уровня приложения
 *
 */
// Протокол в проводном формате: октет длины, затем название
static const unsigned char ALPN_WIRE[] = { 0x02, 'h', '3' };
// Название протокола без октета длины
static const char * ALPN_NAME = "h3";
// Длина названия протокола
static const size_t ALPN_NAME_LEN = 2;

/**
 * @brief Транспортные параметры, совпадающие с `benchmark/proto/quic`
 *
 * @details Лимиты выбраны заведомо широкими: сравнивается стоимость обработки,
 *          а не поведение управления потоком, поэтому упираться в окна передача
 *          не должна. Значения повторяют `configure()` набора бенчмарков AWH
 *
 */
static constexpr uint64_t MAX_DATA = 1073741824;
static constexpr uint64_t MAX_STREAM_DATA = 268435456;
static constexpr uint64_t MAX_STREAMS = 1024;

/**
 * @brief Учитывающий распределитель памяти движка транспорта
 *
 * @details ngtcp2 проводит все свои выделения через переданный распределитель.
 *          Учёт включается ровно вокруг измеряемого участка, поэтому счётчик
 *          охватывает только выделения транспорта на установившемся режиме.
 *          Криптографический слой BoringSSL пользуется своим распределителем и
 *          в учёт не попадает - так же, как у стенда AWH
 *
 */
static void * memMalloc(size_t size, void *){
	// Учитываем выполненное выделение памяти
	rival::account(size);
	// Выполняем выделение памяти
	return ::malloc(size);
}
static void memFree(void * ptr, void *){
	// Выполняем освобождение памяти
	::free(ptr);
}
static void * memCalloc(size_t nmemb, size_t size, void *){
	// Учитываем выполненное выделение памяти
	rival::account(nmemb * size);
	// Выполняем выделение памяти
	return ::calloc(nmemb, size);
}
static void * memRealloc(void * ptr, size_t size, void *){
	// Учитываем выделение памяти только для нового блока: перевыделение существующего
	// блока к появлению попакетного выделения в горячем пути отношения не имеет
	if(ptr == nullptr)
		// Учитываем выполненное выделение памяти
		rival::account(size);
	// Выполняем перевыделение памяти
	return ::realloc(ptr, size);
}
// Экземпляр учитывающего распределителя памяти
static const ngtcp2_mem COUNTING_MEM = { nullptr, memMalloc, memFree, memCalloc, memRealloc };

/**
 * @brief Счётчик принятого сервером объёма данных приложения
 *
 * @details Заполняется функцией обратного вызова приёма данных потока и служит
 *          критерием завершения передачи - тем же, что и в сценарии AWH
 *
 */
static size_t gReceived = 0;

/**
 * @brief Структура эндпоинта соединения QUIC
 *
 */
typedef struct Endpoint {
	// Ссылка на соединение для криптографического слоя BoringSSL
	ngtcp2_crypto_conn_ref ref;
	// Соединение ngtcp2
	ngtcp2_conn * conn;
	// Контекст транспортной безопасности
	SSL_CTX * ctx;
	// Объект транспортной безопасности
	SSL * ssl;
	// Путь соединения (локальный и удалённый адреса)
	ngtcp2_path_storage path;
	/**
	 * @brief Конструктор
	 *
	 */
	explicit Endpoint() noexcept : conn(nullptr), ctx(nullptr), ssl(nullptr) {
		// Обнуляем ссылку на соединение
		::memset(&this->ref, 0, sizeof(this->ref));
	}
} endpoint_t;

/**
 * @brief Функция получения соединения для криптографического слоя
 *
 * @param ref ссылка на соединение
 * @return    соединение ngtcp2
 *
 */
static ngtcp2_conn * getConn(ngtcp2_crypto_conn_ref * ref) noexcept {
	// Выводим соединение эндпоинта
	return static_cast <endpoint_t *> (ref->user_data)->conn;
}

/**
 * @brief Функция обратного вызова получения случайных данных
 *
 */
static void randCb(uint8_t * dest, size_t destlen, const ngtcp2_rand_ctx *) noexcept {
	// Заполняем буфер случайными данными
	::RAND_bytes(dest, static_cast <int> (destlen));
}
/**
 * @brief Функция обратного вызова выдачи нового идентификатора соединения
 *
 */
static int newConnectionIdCb(ngtcp2_conn *, ngtcp2_cid * cid, ngtcp2_stateless_reset_token * token, size_t cidlen, void *) noexcept {
	// Заполняем идентификатор соединения случайными данными
	if(::RAND_bytes(cid->data, static_cast <int> (cidlen)) != 1)
		// Выводим ошибку функции обратного вызова
		return NGTCP2_ERR_CALLBACK_FAILURE;
	// Устанавливаем длину идентификатора соединения
	cid->datalen = cidlen;
	// Заполняем токен сброса случайными данными
	if(::RAND_bytes(token->data, sizeof(token->data)) != 1)
		// Выводим ошибку функции обратного вызова
		return NGTCP2_ERR_CALLBACK_FAILURE;
	// Выводим успешный результат
	return 0;
}
/**
 * @brief Функция обратного вызова приёма данных потока
 *
 * @details Приёмник учитывает принятый объём и расширяет окна управления потоком:
 *          без выдачи данных приложению лимиты не продвигаются и передача встаёт
 *
 */
static int recvStreamDataCb(ngtcp2_conn * conn, uint32_t, int64_t stream_id, uint64_t, const uint8_t * data, size_t datalen, void *, void *) noexcept {
	// Буфер выдачи данных приложению
	static uint8_t sink[rival::BLOCK_SIZE];
	// Если приняты данные потока
	if(datalen > 0){
		// Копируем принятые данные в буфер приложения тем же объёмом, что и стенд AWH
		// выдачей server.receive(): без равной работы потребителя сравнение измеряло бы
		// не транспорт, а разную стоимость выдачи принятого приложению
		::memcpy(sink, data, (datalen <= sizeof(sink) ? datalen : sizeof(sink)));
		// Учитываем принятый объём данных
		gReceived += datalen;
		// Сообщаем о потреблении данных потока приложением
		::ngtcp2_conn_extend_max_stream_offset(conn, stream_id, datalen);
		// Сообщаем о потреблении данных соединения приложением
		::ngtcp2_conn_extend_max_offset(conn, datalen);
	}
	// Выводим успешный результат
	return 0;
}
/**
 * @brief Функция обратного вызова выбора протокола уровня приложения
 *
 */
static int alpnSelectCb(SSL *, const unsigned char ** out, unsigned char * outlen, const unsigned char * in, unsigned int inlen, void *) noexcept {
	// Длина названия предлагаемого протокола
	const size_t len = (ALPN_NAME_LEN);
	// Перебираем предложенные клиентом протоколы
	for(unsigned int i = 0; i < inlen;){
		// Длина очередного предложенного протокола
		const unsigned int n = in[i];
		// Если протокол совпал с предлагаемым сервером
		if((n == len) && (::memcmp(in + i + 1, ALPN_NAME, len) == 0)){
			// Устанавливаем выбранный протокол
			(* out) = (in + i + 1);
			// Устанавливаем длину выбранного протокола
			(* outlen) = static_cast <unsigned char> (n);
			// Выводим успешный результат
			return SSL_TLSEXT_ERR_OK;
		}
		// Переходим к следующему предложенному протоколу
		i += (n + 1);
	}
	// Выводим отказ в согласовании протокола
	return SSL_TLSEXT_ERR_ALERT_FATAL;
}

/**
 * @brief Функция построения транспортных параметров эндпоинта
 *
 * @param params транспортные параметры
 *
 */
static void configure(ngtcp2_transport_params & params) noexcept {
	// Устанавливаем транспортные параметры по умолчанию
	::ngtcp2_transport_params_default(&params);
	// Устанавливаем лимит данных соединения
	params.initial_max_data = MAX_DATA;
	// Устанавливаем лимит данных локально инициируемых двунаправленных потоков
	params.initial_max_stream_data_bidi_local = MAX_STREAM_DATA;
	// Устанавливаем лимит данных удалённо инициируемых двунаправленных потоков
	params.initial_max_stream_data_bidi_remote = MAX_STREAM_DATA;
	// Устанавливаем лимит данных однонаправленных потоков
	params.initial_max_stream_data_uni = MAX_STREAM_DATA;
	// Устанавливаем лимит числа двунаправленных потоков
	params.initial_max_streams_bidi = MAX_STREAMS;
	// Устанавливаем лимит числа однонаправленных потоков
	params.initial_max_streams_uni = MAX_STREAMS;
}
/**
 * @brief Функция заполнения пути соединения
 *
 * @param path   путь соединения
 * @param local  локальный порт
 * @param remote удалённый порт
 *
 */
static void makePath(ngtcp2_path_storage & path, const uint16_t local, const uint16_t remote) noexcept {
	// Локальный адрес эндпоинта
	static sockaddr_in localAddr[2];
	// Удалённый адрес эндпоинта
	static sockaddr_in remoteAddr[2];
	// Индекс набора адресов
	static size_t index = 0;
	// Выбираем очередной набор адресов
	sockaddr_in & la = localAddr[index];
	sockaddr_in & ra = remoteAddr[index];
	// Переходим к следующему набору адресов
	index = ((index + 1) % 2);
	// Обнуляем адреса
	::memset(&la, 0, sizeof(la));
	::memset(&ra, 0, sizeof(ra));
	// Заполняем локальный адрес эндпоинта
	la.sin_family = AF_INET;
	la.sin_port = htons(local);
	la.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
	// Заполняем удалённый адрес эндпоинта
	ra.sin_family = AF_INET;
	ra.sin_port = htons(remote);
	ra.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
	// Заполняем путь соединения
	::ngtcp2_path_storage_init(&path, reinterpret_cast <sockaddr *> (&la), sizeof(la), reinterpret_cast <sockaddr *> (&ra), sizeof(ra), nullptr);
}
/**
 * @brief Функция создания соединения клиента
 *
 * @param client   эндпоинт клиента
 * @param ts       текущее время тестовых часов
 * @return         результат создания соединения
 *
 */
static bool createClient(endpoint_t & client, const ngtcp2_tstamp ts) noexcept {
	// Создаём контекст транспортной безопасности клиента
	client.ctx = ::SSL_CTX_new(TLS_client_method());
	// Если контекст не создан
	if(client.ctx == nullptr)
		// Выводим отрицательный результат
		return false;
	// Настраиваем контекст под криптографический слой ngtcp2
	if(::ngtcp2_crypto_boringssl_configure_client_context(client.ctx) != 0)
		// Выводим отрицательный результат
		return false;
	// Сертификат тестовый и самоподписанный, проверку не выполняем
	::SSL_CTX_set_verify(client.ctx, SSL_VERIFY_NONE, nullptr);
	// Создаём объект транспортной безопасности клиента
	client.ssl = ::SSL_new(client.ctx);
	// Если объект не создан
	if(client.ssl == nullptr)
		// Выводим отрицательный результат
		return false;
	// Связываем объект безопасности со ссылкой на соединение
	client.ref.get_conn = getConn;
	client.ref.user_data = &client;
	SSL_set_app_data(client.ssl, &client.ref);
	// Переводим объект безопасности в режим клиента
	::SSL_set_connect_state(client.ssl);
	// Предлагаем протокол уровня приложения
	::SSL_set_alpn_protos(client.ssl, ALPN_WIRE, static_cast <unsigned int> (sizeof(ALPN_WIRE)));
	// Функции обратного вызова соединения клиента
	ngtcp2_callbacks callbacks;
	// Обнуляем функции обратного вызова
	::memset(&callbacks, 0, sizeof(callbacks));
	callbacks.client_initial = ngtcp2_crypto_client_initial_cb;
	callbacks.recv_crypto_data = ngtcp2_crypto_recv_crypto_data_cb;
	callbacks.encrypt = ngtcp2_crypto_encrypt_cb;
	callbacks.decrypt = ngtcp2_crypto_decrypt_cb;
	callbacks.hp_mask = ngtcp2_crypto_hp_mask_cb;
	callbacks.recv_stream_data = recvStreamDataCb;
	callbacks.recv_retry = ngtcp2_crypto_recv_retry_cb;
	callbacks.rand = randCb;
	callbacks.get_new_connection_id2 = newConnectionIdCb;
	callbacks.update_key = ngtcp2_crypto_update_key_cb;
	callbacks.delete_crypto_aead_ctx = ngtcp2_crypto_delete_crypto_aead_ctx_cb;
	callbacks.delete_crypto_cipher_ctx = ngtcp2_crypto_delete_crypto_cipher_ctx_cb;
	callbacks.version_negotiation = ngtcp2_crypto_version_negotiation_cb;
	callbacks.get_path_challenge_data2 = ngtcp2_crypto_get_path_challenge_data2_cb;
	// Идентификаторы соединения клиента
	ngtcp2_cid dcid, scid;
	// Заполняем идентификатор назначения случайными данными
	dcid.datalen = NGTCP2_MIN_INITIAL_DCIDLEN;
	::RAND_bytes(dcid.data, static_cast <int> (dcid.datalen));
	// Заполняем идентификатор источника случайными данными
	scid.datalen = 8;
	::RAND_bytes(scid.data, static_cast <int> (scid.datalen));
	// Параметры соединения клиента
	ngtcp2_settings settings;
	// Транспортные параметры клиента
	ngtcp2_transport_params params;
	// Устанавливаем параметры соединения по умолчанию
	::ngtcp2_settings_default(&settings);
	// Устанавливаем начальное время тестовых часов
	settings.initial_ts = ts;
	// Строим транспортные параметры клиента
	configure(params);
	// Заполняем путь соединения клиента (клиент 4433 → сервер 4432)
	makePath(client.path, 4433, 4432);
	// Создаём соединение клиента
	if(::ngtcp2_conn_client_new(&client.conn, &dcid, &scid, &client.path.path, NGTCP2_PROTO_VER_V1, &callbacks, &settings, &params, &COUNTING_MEM, &client) != 0)
		// Выводим отрицательный результат
		return false;
	// Связываем соединение с объектом транспортной безопасности
	::ngtcp2_conn_set_tls_native_handle(client.conn, client.ssl);
	// Выводим положительный результат
	return true;
}
/**
 * @brief Функция создания соединения сервера
 *
 * @param server эндпоинт сервера
 * @param hd     заголовок первого принятого пакета
 * @param ts     текущее время тестовых часов
 * @return       результат создания соединения
 *
 */
static bool createServer(endpoint_t & server, const ngtcp2_pkt_hd & hd, const ngtcp2_tstamp ts) noexcept {
	// Создаём контекст транспортной безопасности сервера
	server.ctx = ::SSL_CTX_new(TLS_server_method());
	// Если контекст не создан
	if(server.ctx == nullptr)
		// Выводим отрицательный результат
		return false;
	// Настраиваем контекст под криптографический слой ngtcp2
	if(::ngtcp2_crypto_boringssl_configure_server_context(server.ctx) != 0)
		// Выводим отрицательный результат
		return false;
	// Загружаем цепочку сертификатов сервера
	if(::SSL_CTX_use_certificate_chain_file(server.ctx, QUIC_CERT_FILE) != 1){
		// Выводим сообщение об ошибке загрузки сертификата
		::fprintf(stderr, "[ngtcp2] не загружен сертификат \"%s\": %s\n", QUIC_CERT_FILE, ::ERR_error_string(::ERR_get_error(), nullptr));
		// Выводим отрицательный результат
		return false;
	}
	// Загружаем приватный ключ сервера
	if(::SSL_CTX_use_PrivateKey_file(server.ctx, QUIC_KEY_FILE, SSL_FILETYPE_PEM) != 1){
		// Выводим сообщение об ошибке загрузки приватного ключа
		::fprintf(stderr, "[ngtcp2] не загружен приватный ключ \"%s\": %s\n", QUIC_KEY_FILE, ::ERR_error_string(::ERR_get_error(), nullptr));
		// Выводим отрицательный результат
		return false;
	}
	// Устанавливаем функцию выбора протокола уровня приложения
	::SSL_CTX_set_alpn_select_cb(server.ctx, alpnSelectCb, nullptr);
	// Создаём объект транспортной безопасности сервера
	server.ssl = ::SSL_new(server.ctx);
	// Если объект не создан
	if(server.ssl == nullptr)
		// Выводим отрицательный результат
		return false;
	// Связываем объект безопасности со ссылкой на соединение
	server.ref.get_conn = getConn;
	server.ref.user_data = &server;
	SSL_set_app_data(server.ssl, &server.ref);
	// Переводим объект безопасности в режим сервера
	::SSL_set_accept_state(server.ssl);
	// Функции обратного вызова соединения сервера
	ngtcp2_callbacks callbacks;
	// Обнуляем функции обратного вызова
	::memset(&callbacks, 0, sizeof(callbacks));
	callbacks.recv_client_initial = ngtcp2_crypto_recv_client_initial_cb;
	callbacks.recv_crypto_data = ngtcp2_crypto_recv_crypto_data_cb;
	callbacks.encrypt = ngtcp2_crypto_encrypt_cb;
	callbacks.decrypt = ngtcp2_crypto_decrypt_cb;
	callbacks.hp_mask = ngtcp2_crypto_hp_mask_cb;
	callbacks.recv_stream_data = recvStreamDataCb;
	callbacks.rand = randCb;
	callbacks.get_new_connection_id2 = newConnectionIdCb;
	callbacks.update_key = ngtcp2_crypto_update_key_cb;
	callbacks.delete_crypto_aead_ctx = ngtcp2_crypto_delete_crypto_aead_ctx_cb;
	callbacks.delete_crypto_cipher_ctx = ngtcp2_crypto_delete_crypto_cipher_ctx_cb;
	callbacks.version_negotiation = ngtcp2_crypto_version_negotiation_cb;
	callbacks.get_path_challenge_data2 = ngtcp2_crypto_get_path_challenge_data2_cb;
	// Идентификатор источника соединения сервера
	ngtcp2_cid scid;
	// Заполняем идентификатор источника случайными данными
	scid.datalen = 8;
	::RAND_bytes(scid.data, static_cast <int> (scid.datalen));
	// Параметры соединения сервера
	ngtcp2_settings settings;
	// Транспортные параметры сервера
	ngtcp2_transport_params params;
	// Устанавливаем параметры соединения по умолчанию
	::ngtcp2_settings_default(&settings);
	// Устанавливаем начальное время тестовых часов
	settings.initial_ts = ts;
	// Строим транспортные параметры сервера
	configure(params);
	// Устанавливаем исходный идентификатор назначения (RFC 9000 §7.3)
	params.original_dcid = hd.dcid;
	params.original_dcid_present = 1;
	// Заполняем путь соединения сервера (сервер 4432 → клиент 4433)
	makePath(server.path, 4432, 4433);
	// Создаём соединение сервера
	if(::ngtcp2_conn_server_new(&server.conn, &hd.scid, &scid, &server.path.path, hd.version, &callbacks, &settings, &params, &COUNTING_MEM, &server) != 0)
		// Выводим отрицательный результат
		return false;
	// Связываем соединение с объектом транспортной безопасности
	::ngtcp2_conn_set_tls_native_handle(server.conn, server.ssl);
	// Выводим положительный результат
	return true;
}

/**
 * @brief Функция выкачивания служебных пакетов эндпоинта в сторону получателя
 *
 * @details Формирует пакеты без данных потока (подтверждения, служебные фреймы,
 *          рукопожатие) и передаёт их получателю. Используется при рукопожатии
 *          и для доставки подтверждений в измеряемой передаче
 *
 * @param from      эндпоинт-источник
 * @param to        эндпоинт-получатель
 * @param ts        текущее время тестовых часов
 * @param datagrams счётчик переданных датаграмм (nullptr - не считать)
 * @return          количество переданных пакетов
 *
 */
static size_t drain(endpoint_t & from, endpoint_t & to, const ngtcp2_tstamp ts, size_t * datagrams) noexcept {
	// Количество переданных пакетов
	size_t moved = 0;
	// Буфер передаваемого пакета
	static uint8_t packet[1452];
	// Информация о передаваемом пакете
	ngtcp2_pkt_info pi;
	/**
	 * Выкачиваем служебные пакеты эндпоинта-источника
	 */
	for(;;){
		// Формируем очередной служебный пакет
		const ngtcp2_ssize nwrite = ::ngtcp2_conn_writev_stream(from.conn, &from.path.path, &pi, packet, sizeof(packet), nullptr, NGTCP2_WRITE_STREAM_FLAG_NONE, -1, nullptr, 0, ts);
		// Если формировать больше нечего либо возникла ошибка
		if(nwrite <= 0)
			// Прекращаем выкачивание
			break;
		// Считаем переданный пакет
		moved++;
		// Если требуется учёт переданных датаграмм
		if(datagrams != nullptr)
			// Считаем переданную датаграмму
			(* datagrams)++;
		// Передаём пакет получателю
		::ngtcp2_conn_read_pkt(to.conn, &to.path.path, &pi, packet, static_cast <size_t> (nwrite), ts);
	}
	// Выводим количество переданных пакетов
	return moved;
}
/**
 * @brief Функция продвижения тестовых часов за ближайший дедлайн таймеров
 *
 * @param client эндпоинт клиента
 * @param server эндпоинт сервера
 * @param ts     текущее время тестовых часов
 *
 */
static void expire(endpoint_t & client, endpoint_t & server, ngtcp2_tstamp & ts) noexcept {
	// Ближайший дедлайн таймеров клиента
	const ngtcp2_tstamp ce = ::ngtcp2_conn_get_expiry2(client.conn);
	// Ближайший дедлайн таймеров сервера
	const ngtcp2_tstamp se = ::ngtcp2_conn_get_expiry2(server.conn);
	// Выбираем ближайший дедлайн из двух
	const ngtcp2_tstamp expiry = ((ce < se) ? ce : se);
	// Продвигаем часы за ближайший дедлайн, но не менее чем на миллисекунду
	if((expiry != UINT64_MAX) && (expiry > ts))
		// Устанавливаем время ближайшего дедлайна
		ts = expiry;
	// Продвигаем часы на миллисекунду
	ts += NGTCP2_MILLISECONDS;
	// Обрабатываем просроченные таймеры клиента
	::ngtcp2_conn_handle_expiry(client.conn, ts);
	// Обрабатываем просроченные таймеры сервера
	::ngtcp2_conn_handle_expiry(server.conn, ts);
}
/**
 * @brief Функция выполнения рукопожатия между клиентом и сервером
 *
 * @param client эндпоинт клиента
 * @param server эндпоинт сервера
 * @param ts     текущее время тестовых часов
 * @return       результат рукопожатия
 *
 */
static bool handshake(endpoint_t & client, endpoint_t & server, ngtcp2_tstamp & ts) noexcept {
	// Буфер первого пакета клиента
	static uint8_t packet[1452];
	// Информация о первом пакете
	ngtcp2_pkt_info pi;
	// Формируем первый пакет клиента
	const ngtcp2_ssize nwrite = ::ngtcp2_conn_writev_stream(client.conn, &client.path.path, &pi, packet, sizeof(packet), nullptr, NGTCP2_WRITE_STREAM_FLAG_NONE, -1, nullptr, 0, ts);
	// Если первый пакет не сформирован
	if(nwrite <= 0)
		// Выводим отрицательный результат
		return false;
	// Заголовок первого пакета клиента
	ngtcp2_pkt_hd hd;
	// Разбираем заголовок первого пакета клиента
	if(::ngtcp2_accept(&hd, packet, static_cast <size_t> (nwrite)) != 0)
		// Выводим отрицательный результат
		return false;
	// Создаём соединение сервера по заголовку первого пакета
	if(!createServer(server, hd, ts))
		// Выводим отрицательный результат
		return false;
	// Передаём первый пакет клиента серверу
	::ngtcp2_conn_read_pkt(server.conn, &server.path.path, &pi, packet, static_cast <size_t> (nwrite), ts);
	/**
	 * Обмениваемся пакетами рукопожатия до его завершения на обоих эндпоинтах
	 */
	for(size_t i = 0; i < 64; i++){
		// Выкачиваем пакеты сервера клиенту
		const size_t a = drain(server, client, ts, nullptr);
		// Выкачиваем пакеты клиента серверу
		const size_t b = drain(client, server, ts, nullptr);
		// Если рукопожатие завершено на обоих эндпоинтах
		if((::ngtcp2_conn_get_handshake_completed(client.conn) != 0) && (::ngtcp2_conn_get_handshake_completed(server.conn) != 0))
			// Выводим положительный результат
			return true;
		// Если пакеты на шаге не передавались
		if((a + b) == 0)
			// Продвигаем часы за ближайший дедлайн таймеров
			expire(client, server, ts);
	}
	// Выводим отрицательный результат - обмен не сошёлся
	return false;
}
/**
 * @brief Функция прогона передачи данных между клиентом и сервером
 *
 * @param streams количество потоков передачи
 * @param output  итоги прогона передачи
 * @return        результат прогона
 *
 */
static bool transfer(const size_t streams, rival::transfer_t & output) noexcept {
	// Сбрасываем счётчик принятого объёма данных
	gReceived = 0;
	// Эндпоинт клиента
	endpoint_t client;
	// Эндпоинт сервера
	endpoint_t server;
	// Начальное время тестовых часов
	ngtcp2_tstamp ts = NGTCP2_SECONDS;
	// Создаём соединение клиента
	if(!createClient(client, ts))
		// Выводим отрицательный результат
		return false;
	// Выполняем рукопожатие между клиентом и сервером
	if(!handshake(client, server, ts))
		// Выводим отрицательный результат
		return false;
	// Список идентификаторов открытых потоков
	std::vector <int64_t> identifiers(streams, -1);
	/**
	 * Открываем потоки передачи данных на клиенте
	 */
	for(size_t i = 0; i < streams; i++){
		// Открываем двунаправленный поток на клиенте
		if(::ngtcp2_conn_open_bidi_stream(client.conn, &identifiers[i], nullptr) != 0)
			// Выводим отрицательный результат
			return false;
	}
	// Блок данных, ставящийся приложением в очередь отправки
	static const std::string block(rival::BLOCK_SIZE, 'x');
	// Объём данных на один поток
	const size_t target = (rival::PAYLOAD_SIZE / streams);
	// Объёмы отправленных данных по потокам
	std::vector <size_t> sent(streams, 0);
	// Буфер передаваемого пакета
	static uint8_t packet[1452];
	// Информация о передаваемом пакете
	ngtcp2_pkt_info pi;
	// Включаем учёт выделений памяти
	rival::counting(true);
	// Запоминаем момент начала измерения
	const auto start = std::chrono::steady_clock::now();
	/**
	 * Выполняем передачу данных до приёма всего объёма
	 */
	while(gReceived < rival::PAYLOAD_SIZE){
		// Флаг передачи хотя бы одного пакета на шаге
		bool moved = false;
		/**
		 * Выкачиваем данные потоков клиента до упора в окно перегрузки
		 */
		for(;;){
			// Флаг продвижения хотя бы одного потока на проходе
			bool progress = false;
			/**
			 * Перебираем открытые потоки передачи
			 */
			for(size_t i = 0; i < streams; i++){
				// Если объём данных потока ещё не отправлен целиком
				if(sent[i] < target){
					// Размер очередного блока данных потока
					const size_t length = ::std::min(rival::BLOCK_SIZE, target - sent[i]);
					// Данные очередного блока потока
					ngtcp2_vec datav = { const_cast <uint8_t *> (reinterpret_cast <const uint8_t *> (block.data())), length };
					// Объём данных потока, вошедший в пакет
					ngtcp2_ssize wdatalen = 0;
					// Формируем очередной пакет с данными потока
					const ngtcp2_ssize nwrite = ::ngtcp2_conn_writev_stream(client.conn, &client.path.path, &pi, packet, sizeof(packet), &wdatalen, NGTCP2_WRITE_STREAM_FLAG_NONE, identifiers[i], &datav, 1, ts);
					// Если поток упёрся в окно управления потоком
					if((nwrite == NGTCP2_ERR_STREAM_DATA_BLOCKED) || (nwrite == NGTCP2_ERR_STREAM_SHUT_WR))
						// Переходим к следующему потоку
						continue;
					// Если возникла ошибка формирования пакета
					if(nwrite < 0)
						// Выводим отрицательный результат
						return false;
					// Если формировать больше нечего
					if(nwrite == 0)
						// Переходим к следующему потоку
						continue;
					// Учитываем отправленный объём данных потока
					if(wdatalen > 0)
						// Продвигаем объём отправленных данных потока
						sent[i] += static_cast <size_t> (wdatalen);
					// Считаем переданную датаграмму
					output.datagrams++;
					// Устанавливаем флаги передачи пакета
					moved = true;
					progress = true;
					// Передаём пакет серверу
					::ngtcp2_conn_read_pkt(server.conn, &server.path.path, &pi, packet, static_cast <size_t> (nwrite), ts);
				}
			}
			// Если ни один поток не продвинулся
			if(!progress)
				// Прекращаем выкачивание данных потоков
				break;
		}
		// Выкачиваем подтверждения и служебные фреймы сервера клиенту
		if(drain(server, client, ts, &output.datagrams) > 0)
			// Устанавливаем флаг передачи пакета
			moved = true;
		// Выкачиваем подтверждения и служебные фреймы клиента серверу
		if(drain(client, server, ts, &output.datagrams) > 0)
			// Устанавливаем флаг передачи пакета
			moved = true;
		// Если пакеты на шаге не передавались
		if(!moved)
			// Продвигаем часы за ближайший дедлайн таймеров
			expire(client, server, ts);
	}
	// Запоминаем момент окончания измерения
	const auto finish = std::chrono::steady_clock::now();
	// Отключаем учёт выделений памяти
	rival::counting(false);
	// Устанавливаем количество принятых октетов
	output.received = gReceived;
	// Устанавливаем затраченное время
	output.seconds = std::chrono::duration <double> (finish - start).count();
	// Устанавливаем количество выполненных выделений памяти
	output.allocations = rival::counter::count;
	// Устанавливаем объём выделенной памяти
	output.bytes = rival::counter::bytes;
	// Освобождаем соединение сервера
	if(server.conn != nullptr)
		// Удаляем соединение сервера
		::ngtcp2_conn_del(server.conn);
	// Освобождаем соединение клиента
	if(client.conn != nullptr)
		// Удаляем соединение клиента
		::ngtcp2_conn_del(client.conn);
	// Освобождаем объекты транспортной безопасности
	if(server.ssl != nullptr) ::SSL_free(server.ssl);
	if(client.ssl != nullptr) ::SSL_free(client.ssl);
	if(server.ctx != nullptr) ::SSL_CTX_free(server.ctx);
	if(client.ctx != nullptr) ::SSL_CTX_free(client.ctx);
	// Выводим положительный результат
	return true;
}
/**
 * @brief Функция выполнения сценария передачи данных
 *
 * @param name    название сценария
 * @param streams количество потоков передачи
 * @param metric  измеряемая характеристика (true - выделения, false - пропускная способность)
 * @param mask    фильтр названий сценариев
 *
 */
static void execute(const char * name, const size_t streams, const bool metric, const char * mask) noexcept {
	// Если название сценария не соответствует фильтру
	if(!rival::selected(name, mask))
		// Выходим без выполнения сценария
		return;
	// Итоги прогона передачи данных
	rival::transfer_t transfer;
	// Если прогон передачи данных не выполнен
	if(!::transfer(streams, transfer)){
		// Выводим сообщение о неудачном прогоне сценария
		rival::skip(name, "прогон не выполнен: соединение не установлено");
		// Выходим из сценария
		return;
	}
	// Если измеряется количество выделений памяти
	if(metric)
		// Выводим результат прогона выделений памяти
		rival::allocations(name, transfer);
	// Если измеряется пропускная способность
	else rival::throughput(name, transfer);
}
/**
 * @brief Точка входа стенда сравнения транспорта QUIC на основе ngtcp2
 *
 * @param argc длина массива параметров
 * @param argv массив параметров
 * @return     код возврата
 *
 */
int32_t main(int32_t argc, char ** argv) noexcept {
	// Получаем фильтр названий сценариев
	const char * mask = rival::filter(argc, argv);
	// Выполняем сценарий передачи по одному потоку
	execute("quic/throughput/single-stream", 1, false, mask);
	// Выполняем сценарий передачи по множеству потоков
	execute("quic/throughput/multi-stream", rival::STREAM_COUNT, false, mask);
	// Парный ограниченному сценарию awh: у ngtcp2 стейджинг отправки ограничен всегда
	// (модель zero-copy), отдельного безлимитного режима нет - прогон тот же
	execute("quic/throughput/multi-stream-bounded", rival::STREAM_COUNT, false, mask);
	// Выполняем сценарий количества выделений памяти на датаграмму
	execute("quic/allocations/per-datagram", 1, true, mask);
	// Выводим успешный код возврата
	return 0;
}
