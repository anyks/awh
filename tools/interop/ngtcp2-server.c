/*
 * Минимальный эхо-сервер QUIC на основе ngtcp2 для сверки с реализацией AWH.
 *
 * Обслуживает одно соединение за запуск: этого достаточно, чтобы прогнать
 * клиента AWH против чужой реализации транспорта. Данные потоков и датаграммы
 * приложения возвращаются отправителю без изменений.
 */

#include <time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <poll.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <errno.h>
#include <assert.h>

#include <ngtcp2/ngtcp2.h>
#include <ngtcp2/ngtcp2_crypto.h>
#include <ngtcp2/ngtcp2_crypto_boringssl.h>

#include <openssl/ssl.h>
#include <openssl/rand.h>
#include <openssl/err.h>

/* Параметры запуска, задаются аргументами командной строки */
static const char * listen_host = "127.0.0.1";
static const char * listen_port = "2222";
static const char * cert_file = "../sh/certificates/server/cert.pem";
static const char * key_file = "../sh/certificates/server/key.pem";
static const char * alpn = "h3";
static int verbose = 0;
static int use_retry = 0;
static const char * broken_retry = "";
static uint64_t deadline_ms = 30000;

/* Итоги сеанса, печатаются в конце и служат критерием успеха */
static struct {
	int handshake;
	int closed;
	int failed;
	size_t streamlen;
	size_t datagrams;
	char alpn[64];
} report;

/* Максимум одновременно обслуживаемых исходящих буферов потока */
#define MAX_STREAMS 8

/*
 * Состояние выполненной проверки адреса пакетом Retry: клиент обязан убедиться,
 * что сервер вернул исходный идентификатор назначения и идентификатор источника
 * пакета Retry в транспортных параметрах (RFC 9000 §7.3)
 */
static int retry_sent = 0;
static ngtcp2_cid retry_ocid;

struct stream_out {
	int64_t id;
	int used;
	int fin;
	uint8_t * data;
	size_t datalen;
	size_t nwrite;
};

struct server {
	ngtcp2_crypto_conn_ref conn_ref;
	int fd;
	struct sockaddr_storage local_addr;
	socklen_t local_addrlen;
	struct sockaddr_storage remote_addr;
	socklen_t remote_addrlen;
	SSL_CTX * ssl_ctx;
	SSL * ssl;
	ngtcp2_conn * conn;
	struct stream_out streams[MAX_STREAMS];
	/* Очередь исходящих датаграмм приложения */
	uint8_t dgram[8][1200];
	size_t dgramlen[8];
	size_t dgramhead, dgramtail;
	int done;
	ngtcp2_ccerr last_error;
};

static uint64_t timestamp(void){
	struct timespec tp;
	if(clock_gettime(CLOCK_MONOTONIC, &tp) != 0){
		fprintf(stderr, "clock_gettime: %s\n", strerror(errno));
		exit(EXIT_FAILURE);
	}
	return (uint64_t) tp.tv_sec * NGTCP2_SECONDS + (uint64_t) tp.tv_nsec;
}

static uint64_t millis(void){
	return timestamp() / NGTCP2_MILLISECONDS;
}

static void log_printf(void * user_data, const char * fmt, ...){
	va_list ap;
	(void) user_data;
	if(!verbose)
		return;
	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	va_end(ap);
	fprintf(stderr, "\n");
}

/* Выбор протокола прикладного уровня: принимаем только заданный ALPN */
static int alpn_select_cb(SSL * ssl, const unsigned char ** out, unsigned char * outlen, const unsigned char * in, unsigned int inlen, void * arg){
	unsigned int i = 0;
	size_t len = strlen(alpn);
	(void) ssl;
	(void) arg;
	while(i < inlen){
		unsigned int n = in[i];
		if((n == len) && (memcmp(in + i + 1, alpn, len) == 0)){
			*out = in + i + 1;
			*outlen = (unsigned char) n;
			return SSL_TLSEXT_ERR_OK;
		}
		i += (n + 1);
	}
	fprintf(stderr, "[ngtcp2] клиент не предложил протокол %s\n", alpn);
	return SSL_TLSEXT_ERR_ALERT_FATAL;
}

static int server_ssl_init(struct server * s){
	s->ssl_ctx = SSL_CTX_new(TLS_server_method());
	if(!s->ssl_ctx){
		fprintf(stderr, "SSL_CTX_new: %s\n", ERR_error_string(ERR_get_error(), NULL));
		return -1;
	}
	if(ngtcp2_crypto_boringssl_configure_server_context(s->ssl_ctx) != 0){
		fprintf(stderr, "ngtcp2_crypto_boringssl_configure_server_context failed\n");
		return -1;
	}
	if(SSL_CTX_use_certificate_chain_file(s->ssl_ctx, cert_file) != 1){
		fprintf(stderr, "SSL_CTX_use_certificate_chain_file(%s): %s\n", cert_file, ERR_error_string(ERR_get_error(), NULL));
		return -1;
	}
	if(SSL_CTX_use_PrivateKey_file(s->ssl_ctx, key_file, SSL_FILETYPE_PEM) != 1){
		fprintf(stderr, "SSL_CTX_use_PrivateKey_file(%s): %s\n", key_file, ERR_error_string(ERR_get_error(), NULL));
		return -1;
	}
	SSL_CTX_set_alpn_select_cb(s->ssl_ctx, alpn_select_cb, NULL);
	s->ssl = SSL_new(s->ssl_ctx);
	if(!s->ssl){
		fprintf(stderr, "SSL_new: %s\n", ERR_error_string(ERR_get_error(), NULL));
		return -1;
	}
	SSL_set_app_data(s->ssl, &s->conn_ref);
	SSL_set_accept_state(s->ssl);
	return 0;
}

static void rand_cb(uint8_t * dest, size_t destlen, const ngtcp2_rand_ctx * rand_ctx){
	(void) rand_ctx;
	if(RAND_bytes(dest, (int) destlen) != 1){
		assert(0);
		abort();
	}
}

static int get_new_connection_id_cb(ngtcp2_conn * conn, ngtcp2_cid * cid, ngtcp2_stateless_reset_token * token, size_t cidlen, void * user_data){
	(void) conn;
	(void) user_data;
	if(RAND_bytes(cid->data, (int) cidlen) != 1)
		return NGTCP2_ERR_CALLBACK_FAILURE;
	cid->datalen = cidlen;
	if(RAND_bytes(token->data, sizeof(token->data)) != 1)
		return NGTCP2_ERR_CALLBACK_FAILURE;
	return 0;
}

static int handshake_completed_cb(ngtcp2_conn * conn, void * user_data){
	struct server * s = user_data;
	const uint8_t * proto = NULL;
	unsigned int protolen = 0;
	(void) conn;
	SSL_get0_alpn_selected(s->ssl, &proto, &protolen);
	if(protolen > 0 && protolen < sizeof(report.alpn)){
		memcpy(report.alpn, proto, protolen);
		report.alpn[protolen] = 0;
	}
	report.handshake = 1;
	printf("[ngtcp2] рукопожатие завершено, ALPN=%s, шифр=%s\n", report.alpn, SSL_get_cipher_name(s->ssl));
	return 0;
}

/* Поиск или заведение исходящего буфера эха для потока */
static struct stream_out * stream_slot(struct server * s, int64_t id){
	size_t i;
	for(i = 0; i < MAX_STREAMS; i++){
		if(s->streams[i].used && (s->streams[i].id == id))
			return &s->streams[i];
	}
	for(i = 0; i < MAX_STREAMS; i++){
		if(!s->streams[i].used){
			s->streams[i].used = 1;
			s->streams[i].id = id;
			s->streams[i].fin = 0;
			s->streams[i].data = NULL;
			s->streams[i].datalen = 0;
			s->streams[i].nwrite = 0;
			return &s->streams[i];
		}
	}
	return NULL;
}

static int recv_stream_data_cb(ngtcp2_conn * conn, uint32_t flags, int64_t stream_id, uint64_t offset, const uint8_t * data, size_t datalen, void * user_data, void * stream_user_data){
	struct server * s = user_data;
	struct stream_out * out = stream_slot(s, stream_id);
	(void) offset;
	(void) stream_user_data;
	if(out == NULL){
		fprintf(stderr, "[ngtcp2] исчерпан запас исходящих буферов потоков\n");
		return NGTCP2_ERR_CALLBACK_FAILURE;
	}
	if(datalen > 0){
		uint8_t * grown = realloc(out->data, out->datalen + datalen);
		if(grown == NULL)
			return NGTCP2_ERR_CALLBACK_FAILURE;
		out->data = grown;
		memcpy(out->data + out->datalen, data, datalen);
		out->datalen += datalen;
		report.streamlen += datalen;
	}
	if(flags & NGTCP2_STREAM_DATA_FLAG_FIN){
		out->fin = 1;
		printf("[ngtcp2] поток %lld: принято %zu байт, признак завершения\n", (long long) stream_id, out->datalen);
	}
	/* Данные потреблены приложением, окна управления потоком расширяются */
	ngtcp2_conn_extend_max_stream_offset(conn, stream_id, datalen);
	ngtcp2_conn_extend_max_offset(conn, datalen);
	return 0;
}

static int recv_datagram_cb(ngtcp2_conn * conn, uint32_t flags, const uint8_t * data, size_t datalen, void * user_data){
	struct server * s = user_data;
	size_t slot = (s->dgramtail % 8);
	(void) conn;
	(void) flags;
	if(datalen > sizeof(s->dgram[0]))
		datalen = sizeof(s->dgram[0]);
	memcpy(s->dgram[slot], data, datalen);
	s->dgramlen[slot] = datalen;
	s->dgramtail++;
	report.datagrams++;
	printf("[ngtcp2] принята датаграмма: %zu байт: %.*s\n", datalen, (int) datalen, (const char *) data);
	return 0;
}

static ngtcp2_conn * get_conn(ngtcp2_crypto_conn_ref * conn_ref){
	struct server * s = conn_ref->user_data;
	return s->conn;
}

static int server_send_packet(struct server * s, const uint8_t * data, size_t datalen){
	struct iovec iov = {.iov_base = (uint8_t *) data, .iov_len = datalen};
	struct msghdr msg = {0};
	ssize_t nwrite;
	msg.msg_name = &s->remote_addr;
	msg.msg_namelen = s->remote_addrlen;
	msg.msg_iov = &iov;
	msg.msg_iovlen = 1;
	do {
		nwrite = sendmsg(s->fd, &msg, 0);
	} while(nwrite == -1 && errno == EINTR);
	if(nwrite == -1){
		fprintf(stderr, "sendmsg: %s\n", strerror(errno));
		return -1;
	}
	return 0;
}

/* Отправка накопленных эхо-датаграмм приложения */
static int server_write_datagrams(struct server * s){
	uint8_t buf[1452];
	ngtcp2_pkt_info pi;
	ngtcp2_path_storage ps;
	ngtcp2_ssize nwrite;
	int accepted;
	while(s->dgramhead < s->dgramtail){
		size_t slot = (s->dgramhead % 8);
		ngtcp2_vec datav = {.base = s->dgram[slot], .len = s->dgramlen[slot]};
		accepted = 0;
		ngtcp2_path_storage_zero(&ps);
		nwrite = ngtcp2_conn_writev_datagram(s->conn, &ps.path, &pi, buf, sizeof(buf), &accepted, NGTCP2_WRITE_DATAGRAM_FLAG_NONE, 0, &datav, 1, timestamp());
		if(nwrite < 0){
			fprintf(stderr, "[ngtcp2] ngtcp2_conn_writev_datagram: %s\n", ngtcp2_strerror((int) nwrite));
			return 0;
		}
		if(nwrite == 0)
			return 0;
		if(accepted){
			s->dgramhead++;
			printf("[ngtcp2] отправлена эхо-датаграмма: %zu байт\n", datav.len);
		}
		if(server_send_packet(s, buf, (size_t) nwrite) != 0)
			return -1;
	}
	return 0;
}

static int server_write_streams(struct server * s){
	ngtcp2_tstamp ts = timestamp();
	ngtcp2_pkt_info pi;
	ngtcp2_ssize nwrite, wdatalen;
	uint8_t buf[1452];
	ngtcp2_path_storage ps;
	ngtcp2_vec datav;
	size_t datavcnt, i;
	int64_t stream_id;
	uint32_t flags;
	int fin, blocked = 0;
	ngtcp2_path_storage_zero(&ps);
	for(;;){
		struct stream_out * out = NULL;
		stream_id = -1;
		datavcnt = 0;
		fin = 0;
		datav.base = NULL;
		datav.len = 0;
		/* Выбираем поток, по которому осталось что отправить */
		if(!blocked){
			for(i = 0; i < MAX_STREAMS; i++){
				struct stream_out * item = &s->streams[i];
				if(!item->used)
					continue;
				if(item->nwrite < item->datalen){
					out = item;
					break;
				}
				/* Данные исчерпаны, остаётся передать признак завершения */
				if(item->fin == 1){
					out = item;
					break;
				}
			}
		}
		if(out != NULL){
			stream_id = out->id;
			datav.base = out->data + out->nwrite;
			datav.len = out->datalen - out->nwrite;
			datavcnt = 1;
			fin = out->fin;
		}
		flags = NGTCP2_WRITE_STREAM_FLAG_MORE;
		if(fin)
			flags |= NGTCP2_WRITE_STREAM_FLAG_FIN;
		wdatalen = 0;
		nwrite = ngtcp2_conn_writev_stream(s->conn, &ps.path, &pi, buf, sizeof(buf), &wdatalen, flags, stream_id, &datav, datavcnt, ts);
		if(nwrite < 0){
			switch(nwrite){
				case NGTCP2_ERR_WRITE_MORE:
					if((out != NULL) && (wdatalen > 0))
						out->nwrite += (size_t) wdatalen;
					continue;
				case NGTCP2_ERR_STREAM_DATA_BLOCKED:
				case NGTCP2_ERR_STREAM_SHUT_WR:
					blocked = 1;
					continue;
				default:
					fprintf(stderr, "[ngtcp2] ngtcp2_conn_writev_stream: %s\n", ngtcp2_strerror((int) nwrite));
					ngtcp2_ccerr_set_liberr(&s->last_error, (int) nwrite, NULL, 0);
					report.failed = 1;
					return -1;
			}
		}
		if((out != NULL) && (wdatalen > 0))
			out->nwrite += (size_t) wdatalen;
		/* Признак завершения записан вместе с последним фрагментом данных */
		if((out != NULL) && (out->nwrite == out->datalen) && out->fin && (nwrite > 0))
			out->fin = 2;
		if(nwrite == 0)
			return 0;
		if(server_send_packet(s, buf, (size_t) nwrite) != 0)
			return -1;
	}
	return 0;
}

static int server_write(struct server * s){
	if(server_write_datagrams(s) != 0)
		return -1;
	return server_write_streams(s);
}

static int server_quic_init(struct server * s, const ngtcp2_pkt_hd * hd, const struct sockaddr * remote_addr, socklen_t remote_addrlen){
	ngtcp2_path path = {
		.local = {.addr = (struct sockaddr *) &s->local_addr, .addrlen = s->local_addrlen},
		.remote = {.addr = (struct sockaddr *) remote_addr, .addrlen = remote_addrlen},
	};
	ngtcp2_callbacks callbacks = {
		.recv_client_initial = ngtcp2_crypto_recv_client_initial_cb,
		.recv_crypto_data = ngtcp2_crypto_recv_crypto_data_cb,
		.handshake_completed = handshake_completed_cb,
		.encrypt = ngtcp2_crypto_encrypt_cb,
		.decrypt = ngtcp2_crypto_decrypt_cb,
		.hp_mask = ngtcp2_crypto_hp_mask_cb,
		.recv_stream_data = recv_stream_data_cb,
		.rand = rand_cb,
		.update_key = ngtcp2_crypto_update_key_cb,
		.delete_crypto_aead_ctx = ngtcp2_crypto_delete_crypto_aead_ctx_cb,
		.delete_crypto_cipher_ctx = ngtcp2_crypto_delete_crypto_cipher_ctx_cb,
		.version_negotiation = ngtcp2_crypto_version_negotiation_cb,
		.get_new_connection_id2 = get_new_connection_id_cb,
		.get_path_challenge_data2 = ngtcp2_crypto_get_path_challenge_data2_cb,
		.recv_datagram = recv_datagram_cb,
	};
	ngtcp2_cid scid;
	ngtcp2_settings settings;
	ngtcp2_transport_params params;
	int rv;
	/*
	 * После проверки адреса пакетом Retry клиент адресуется идентификатором,
	 * который сервер выдал в этом пакете, и ожидает его же в параметре
	 * retry_source_connection_id вместе с исходным идентификатором назначения
	 */
	if(retry_sent)
		scid = hd->dcid;
	else {
		scid.datalen = 8;
		if(RAND_bytes(scid.data, (int) scid.datalen) != 1){
			fprintf(stderr, "RAND_bytes failed\n");
			return -1;
		}
	}
	ngtcp2_settings_default(&settings);
	settings.initial_ts = timestamp();
	settings.log_printf = log_printf;
	ngtcp2_transport_params_default(&params);
	params.initial_max_streams_uni = 3;
	params.initial_max_streams_bidi = 100;
	params.initial_max_stream_data_bidi_local = 128 * 1024;
	params.initial_max_stream_data_bidi_remote = 128 * 1024;
	params.initial_max_stream_data_uni = 128 * 1024;
	params.initial_max_data = 1024 * 1024;
	params.max_datagram_frame_size = 1200;
	if(retry_sent){
		params.original_dcid = retry_ocid;
		params.retry_scid = hd->dcid;
		params.retry_scid_present = 1;
		/* Порча параметров проверки адреса для отрицательных прогонов */
		if(!strcmp(broken_retry, "ocid")){
			params.original_dcid.data[0] ^= 0xFF;
			printf("[ngtcp2] умышленно искажён original_destination_connection_id\n");
		} else if(!strcmp(broken_retry, "noscid")){
			params.retry_scid_present = 0;
			printf("[ngtcp2] умышленно опущен retry_source_connection_id\n");
		} else if(!strcmp(broken_retry, "scid")){
			params.retry_scid.data[0] ^= 0xFF;
			printf("[ngtcp2] умышленно искажён retry_source_connection_id\n");
		}
	} else params.original_dcid = hd->dcid;
	params.original_dcid_present = 1;
	rv = ngtcp2_conn_server_new(&s->conn, &hd->scid, &scid, &path, hd->version, &callbacks, &settings, &params, NULL, s);
	if(rv != 0){
		fprintf(stderr, "ngtcp2_conn_server_new: %s\n", ngtcp2_strerror(rv));
		return -1;
	}
	ngtcp2_conn_set_tls_native_handle(s->conn, s->ssl);
	return 0;
}

/* Отправка пакета Retry для проверки адреса клиента (RFC 9000 §8.1.2) */
static int server_send_retry(struct server * s, const ngtcp2_pkt_hd * hd, const struct sockaddr * remote_addr, socklen_t remote_addrlen){
	uint8_t buf[NGTCP2_MAX_UDP_PAYLOAD_SIZE];
	uint8_t token[NGTCP2_CRYPTO_MAX_RETRY_TOKENLEN];
	static uint8_t secret[32];
	static int secret_ready = 0;
	ngtcp2_cid scid;
	ngtcp2_ssize tokenlen, nwrite;
	if(!secret_ready){
		if(RAND_bytes(secret, sizeof(secret)) != 1)
			return -1;
		secret_ready = 1;
	}
	scid.datalen = 8;
	if(RAND_bytes(scid.data, (int) scid.datalen) != 1)
		return -1;
	tokenlen = ngtcp2_crypto_generate_retry_token(token, secret, sizeof(secret), hd->version, remote_addr, remote_addrlen, &scid, &hd->dcid, timestamp());
	if(tokenlen < 0)
		return -1;
	nwrite = ngtcp2_crypto_write_retry(buf, sizeof(buf), hd->version, &hd->scid, &scid, &hd->dcid, token, (size_t) tokenlen);
	if(nwrite < 0)
		return -1;
	memcpy(&s->remote_addr, remote_addr, remote_addrlen);
	s->remote_addrlen = remote_addrlen;
	/* Запоминаем исходный идентификатор назначения для транспортных параметров */
	retry_ocid = hd->dcid;
	retry_sent = 1;
	printf("[ngtcp2] отправлен Retry для проверки адреса клиента\n");
	return server_send_packet(s, buf, (size_t) nwrite);
}

static int create_listen_sock(struct server * s){
	struct addrinfo hints = {0};
	struct addrinfo * res, * rp;
	int rv, fd = -1, on = 1;
	hints.ai_flags = AI_PASSIVE;
	hints.ai_socktype = SOCK_DGRAM;
	rv = getaddrinfo(listen_host, listen_port, &hints, &res);
	if(rv != 0){
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return -1;
	}
	for(rp = res; rp; rp = rp->ai_next){
		fd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
		if(fd == -1)
			continue;
		setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
		if(bind(fd, rp->ai_addr, rp->ai_addrlen) == 0)
			break;
		close(fd);
		fd = -1;
	}
	if(fd == -1){
		fprintf(stderr, "bind: %s\n", strerror(errno));
		freeaddrinfo(res);
		return -1;
	}
	s->local_addrlen = sizeof(s->local_addr);
	if(getsockname(fd, (struct sockaddr *) &s->local_addr, &s->local_addrlen) == -1){
		fprintf(stderr, "getsockname: %s\n", strerror(errno));
		close(fd);
		freeaddrinfo(res);
		return -1;
	}
	freeaddrinfo(res);
	s->fd = fd;
	return 0;
}

static void server_close(struct server * s){
	ngtcp2_ssize nwrite;
	ngtcp2_pkt_info pi;
	ngtcp2_path_storage ps;
	uint8_t buf[1280];
	if(s->conn == NULL)
		return;
	if(ngtcp2_conn_in_closing_period2(s->conn) || ngtcp2_conn_in_draining_period2(s->conn))
		return;
	ngtcp2_path_storage_zero(&ps);
	nwrite = ngtcp2_conn_write_connection_close(s->conn, &ps.path, &pi, buf, sizeof(buf), &s->last_error, timestamp());
	if(nwrite < 0)
		return;
	if(server_send_packet(s, buf, (size_t) nwrite) == 0){
		report.closed = 1;
		printf("[ngtcp2] отправлено завершение соединения\n");
	}
}

static void usage(const char * name){
	fprintf(stderr, "Использование: %s [опции]\n"
		"  --host <адрес>     адрес прослушивания, по умолчанию 127.0.0.1\n"
		"  --port <порт>      порт прослушивания, по умолчанию 2222\n"
		"  --cert <файл>      сертификат сервера\n"
		"  --key <файл>       приватный ключ сервера\n"
		"  --alpn <протокол>  принимаемый ALPN, по умолчанию h3\n"
		"  --retry            проверять адрес клиента пакетом Retry\n"
		"  --deadline <мс>    предельное время ожидания, по умолчанию 30000\n"
		"  --verbose          подробный журнал ngtcp2\n", name);
}

int main(int argc, char * argv[]){
	struct server s;
	uint8_t buf[65536];
	uint64_t started;
	int i, retried = 0;
	for(i = 1; i < argc; i++){
		if(!strcmp(argv[i], "--host") && (i + 1) < argc)
			listen_host = argv[++i];
		else if(!strcmp(argv[i], "--port") && (i + 1) < argc)
			listen_port = argv[++i];
		else if(!strcmp(argv[i], "--cert") && (i + 1) < argc)
			cert_file = argv[++i];
		else if(!strcmp(argv[i], "--key") && (i + 1) < argc)
			key_file = argv[++i];
		else if(!strcmp(argv[i], "--alpn") && (i + 1) < argc)
			alpn = argv[++i];
		else if(!strcmp(argv[i], "--deadline") && (i + 1) < argc)
			deadline_ms = strtoull(argv[++i], NULL, 10);
		else if(!strcmp(argv[i], "--retry"))
			use_retry = 1;
		else if(!strcmp(argv[i], "--broken-retry") && (i + 1) < argc){
			broken_retry = argv[++i];
			use_retry = 1;
		}
		else if(!strcmp(argv[i], "--verbose"))
			verbose = 1;
		else {
			usage(argv[0]);
			return EXIT_FAILURE;
		}
	}
	memset(&s, 0, sizeof(s));
	memset(&report, 0, sizeof(report));
	ngtcp2_ccerr_default(&s.last_error);
	s.conn_ref.get_conn = get_conn;
	s.conn_ref.user_data = &s;
	if(create_listen_sock(&s) != 0)
		return EXIT_FAILURE;
	if(server_ssl_init(&s) != 0)
		return EXIT_FAILURE;
	printf("[ngtcp2] эхо-сервер слушает %s:%s, ALPN=%s\n", listen_host, listen_port, alpn);
	fflush(stdout);
	started = millis();
	while(!s.done && !report.failed){
		struct pollfd pfd = {.fd = s.fd, .events = POLLIN};
		struct sockaddr_storage addr;
		struct iovec iov = {.iov_base = buf, .iov_len = sizeof(buf)};
		struct msghdr msg = {0};
		int wait = 10;
		if((millis() - started) > deadline_ms){
			fprintf(stderr, "[ngtcp2] превышено предельное время ожидания\n");
			break;
		}
		if(s.conn != NULL){
			ngtcp2_tstamp expiry = ngtcp2_conn_get_expiry2(s.conn);
			uint64_t now = timestamp();
			if(expiry != UINT64_MAX){
				uint64_t delta = (expiry <= now ? 0 : (expiry - now) / NGTCP2_MILLISECONDS);
				wait = (delta > 100 ? 100 : (int) delta);
			}
		}
		msg.msg_name = &addr;
		msg.msg_iov = &iov;
		msg.msg_iovlen = 1;
		if(poll(&pfd, 1, wait) > 0){
			for(;;){
				ssize_t nread;
				ngtcp2_path path;
				ngtcp2_pkt_info pi = {0};
				int rv;
				msg.msg_namelen = sizeof(addr);
				nread = recvmsg(s.fd, &msg, MSG_DONTWAIT);
				if(nread == -1){
					if(errno != EAGAIN && errno != EWOULDBLOCK)
						fprintf(stderr, "recvmsg: %s\n", strerror(errno));
					break;
				}
				/* Первая датаграмма создаёт соединение */
				if(s.conn == NULL){
					ngtcp2_pkt_hd hd;
					rv = ngtcp2_accept(&hd, buf, (size_t) nread);
					if(rv != 0){
						fprintf(stderr, "[ngtcp2] ngtcp2_accept: %s\n", ngtcp2_strerror(rv));
						continue;
					}
					/* Проверка адреса клиента пакетом Retry до создания соединения */
					if(use_retry && !retried){
						retried = 1;
						if(server_send_retry(&s, &hd, (struct sockaddr *) &addr, msg.msg_namelen) != 0)
							report.failed = 1;
						continue;
					}
					memcpy(&s.remote_addr, &addr, msg.msg_namelen);
					s.remote_addrlen = msg.msg_namelen;
					if(server_quic_init(&s, &hd, (struct sockaddr *) &s.remote_addr, s.remote_addrlen) != 0){
						report.failed = 1;
						break;
					}
					printf("[ngtcp2] принято соединение\n");
				}
				path.local.addrlen = s.local_addrlen;
				path.local.addr = (struct sockaddr *) &s.local_addr;
				path.remote.addrlen = msg.msg_namelen;
				path.remote.addr = (struct sockaddr *) &addr;
				rv = ngtcp2_conn_read_pkt(s.conn, &path, &pi, buf, (size_t) nread, timestamp());
				if(rv != 0){
					fprintf(stderr, "[ngtcp2] ngtcp2_conn_read_pkt: %s\n", ngtcp2_strerror(rv));
					if(!s.last_error.error_code){
						if(rv == NGTCP2_ERR_CRYPTO)
							ngtcp2_ccerr_set_tls_alert(&s.last_error, ngtcp2_conn_get_tls_alert2(s.conn), NULL, 0);
						else ngtcp2_ccerr_set_liberr(&s.last_error, rv, NULL, 0);
					}
					if(ngtcp2_conn_in_draining_period2(s.conn)){
						printf("[ngtcp2] соединение завершено удалённой стороной\n");
						report.closed = 1;
						s.done = 1;
					} else report.failed = 1;
					break;
				}
			}
		}
		if(s.conn == NULL)
			continue;
		if(ngtcp2_conn_get_expiry2(s.conn) <= timestamp()){
			int rv = ngtcp2_conn_handle_expiry(s.conn, timestamp());
			if(rv != 0){
				fprintf(stderr, "[ngtcp2] ngtcp2_conn_handle_expiry: %s\n", ngtcp2_strerror(rv));
				report.failed = 1;
				break;
			}
		}
		if(ngtcp2_conn_in_closing_period2(s.conn) || ngtcp2_conn_in_draining_period2(s.conn)){
			printf("[ngtcp2] соединение завершено удалённой стороной\n");
			report.closed = 1;
			break;
		}
		if(server_write(&s) != 0)
			break;
	}
	if(!report.closed)
		server_close(&s);
	printf("\n=== Итог сверки ===\n");
	printf("рукопожатие:      %s\n", report.handshake ? "да" : "нет");
	printf("ALPN:             %s\n", report.alpn[0] ? report.alpn : "нет");
	printf("данные потоков:   %zu байт\n", report.streamlen);
	printf("датаграммы:       %zu\n", report.datagrams);
	printf("завершение:       %s\n", report.closed ? "да" : "нет");
	printf("ошибки:           %s\n", report.failed ? "да" : "нет");
	if(s.conn != NULL)
		ngtcp2_conn_del(s.conn);
	SSL_free(s.ssl);
	SSL_CTX_free(s.ssl_ctx);
	return (report.failed || !report.handshake) ? EXIT_FAILURE : EXIT_SUCCESS;
}
