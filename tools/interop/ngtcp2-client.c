/*
 * Минимальный клиент QUIC на основе ngtcp2 для сверки с реализацией AWH.
 *
 * Основан на examples/simpleclient.c из состава ngtcp2, переведён с бэкенда
 * quictls на boringssl и с libev на poll(), чтобы не тянуть лишних зависимостей.
 *
 * Сборка и запуск описаны в run.sh рядом с этим файлом.
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
static const char * remote_host = "127.0.0.1";
static const char * remote_port = "2222";
/* ALPN в формате протокола TLS: длина + значение */
static const char * alpn = "\x2h3";
static size_t alpnlen = 3;
static const char * message = "GET /interop\r\n";
static int verbose = 0;
static int use_datagram = 0;
static int idle_probe = 0;
static int use_key_update = 0;
static int use_migration = 0;
static size_t payload_size = 0;
static uint32_t proto_version = NGTCP2_PROTO_VER_V1;
static uint64_t deadline_ms = 5000;

/* Полезная нагрузка потока и накопитель эха для побайтовой сверки */
static uint8_t * payload = NULL;
static size_t payloadlen = 0;
static uint8_t * echobuf = NULL;

/* Итоги сеанса, печатаются в конце и служат критерием успеха */
static struct {
	int handshake;
	int echo;
	int datagram;
	int closed;
	int failed;
	int corrupt;
	int keyupdate;
	int migrated;
	int vneg;
	char alpn[64];
	size_t echolen;
} report;

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

struct client {
	ngtcp2_crypto_conn_ref conn_ref;
	int fd;
	struct sockaddr_storage local_addr;
	socklen_t local_addrlen;
	SSL_CTX * ssl_ctx;
	SSL * ssl;
	ngtcp2_conn * conn;
	struct {
		int64_t stream_id;
		const uint8_t * data;
		size_t datalen;
		size_t nwrite;
		int fin_sent;
	} stream;
	int datagram_sent;
	int fin_received;
	int done;
	ngtcp2_ccerr last_error;
};

static int create_sock(struct sockaddr * addr, socklen_t * paddrlen, const char * host, const char * port){
	struct addrinfo hints = {0};
	struct addrinfo * res, * rp;
	int rv, fd = -1;
	hints.ai_flags = AF_UNSPEC;
	hints.ai_socktype = SOCK_DGRAM;
	rv = getaddrinfo(host, port, &hints, &res);
	if(rv != 0){
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return -1;
	}
	for(rp = res; rp; rp = rp->ai_next){
		fd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
		if(fd == -1)
			continue;
		break;
	}
	if(fd == -1)
		goto end;
	*paddrlen = rp->ai_addrlen;
	memcpy(addr, rp->ai_addr, rp->ai_addrlen);
end:
	freeaddrinfo(res);
	return fd;
}

static int connect_sock(struct sockaddr * local_addr, socklen_t * plocal_addrlen, int fd, const struct sockaddr * remote_addr, size_t remote_addrlen){
	socklen_t len;
	if(connect(fd, remote_addr, (socklen_t) remote_addrlen) != 0){
		fprintf(stderr, "connect: %s\n", strerror(errno));
		return -1;
	}
	len = *plocal_addrlen;
	if(getsockname(fd, local_addr, &len) == -1){
		fprintf(stderr, "getsockname: %s\n", strerror(errno));
		return -1;
	}
	*plocal_addrlen = len;
	return 0;
}

static int numeric_host_family(const char * hostname, int family){
	uint8_t dst[sizeof(struct in6_addr)];
	return inet_pton(family, hostname, dst) == 1;
}

static int numeric_host(const char * hostname){
	return numeric_host_family(hostname, AF_INET) || numeric_host_family(hostname, AF_INET6);
}

static int client_ssl_init(struct client * c){
	c->ssl_ctx = SSL_CTX_new(TLS_client_method());
	if(!c->ssl_ctx){
		fprintf(stderr, "SSL_CTX_new: %s\n", ERR_error_string(ERR_get_error(), NULL));
		return -1;
	}
	if(ngtcp2_crypto_boringssl_configure_client_context(c->ssl_ctx) != 0){
		fprintf(stderr, "ngtcp2_crypto_boringssl_configure_client_context failed\n");
		return -1;
	}
	/* Сертификат тестовый и самоподписанный, проверку не выполняем */
	SSL_CTX_set_verify(c->ssl_ctx, SSL_VERIFY_NONE, NULL);
	c->ssl = SSL_new(c->ssl_ctx);
	if(!c->ssl){
		fprintf(stderr, "SSL_new: %s\n", ERR_error_string(ERR_get_error(), NULL));
		return -1;
	}
	SSL_set_app_data(c->ssl, &c->conn_ref);
	SSL_set_connect_state(c->ssl);
	SSL_set_alpn_protos(c->ssl, (const unsigned char *) alpn, (unsigned int) alpnlen);
	if(!numeric_host(remote_host))
		SSL_set_tlsext_host_name(c->ssl, remote_host);
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
	struct client * c = user_data;
	const uint8_t * proto = NULL;
	unsigned int protolen = 0;
	(void) conn;
	SSL_get0_alpn_selected(c->ssl, &proto, &protolen);
	if(protolen > 0 && protolen < sizeof(report.alpn)){
		memcpy(report.alpn, proto, protolen);
		report.alpn[protolen] = 0;
	}
	report.handshake = 1;
	printf("[ngtcp2] рукопожатие завершено, ALPN=%s, шифр=%s\n", report.alpn, SSL_get_cipher_name(c->ssl));
	return 0;
}

static int recv_stream_data_cb(ngtcp2_conn * conn, uint32_t flags, int64_t stream_id, uint64_t offset, const uint8_t * data, size_t datalen, void * user_data, void * stream_user_data){
	struct client * c = user_data;
	(void) conn;
	(void) offset;
	(void) stream_user_data;
	if(datalen > 0){
		report.echo = 1;
		report.echolen += datalen;
		/* Сверяем эхо с отправленным содержимым побайтно */
		if(echobuf != NULL){
			if((offset + datalen) > payloadlen){
				fprintf(stderr, "[ngtcp2] эхо длиннее отправленного: смещение %llu + %zu > %zu\n", (unsigned long long) offset, datalen, payloadlen);
				report.corrupt = 1;
			} else if(memcmp(payload + offset, data, datalen) != 0){
				fprintf(stderr, "[ngtcp2] эхо не совпадает с отправленным на смещении %llu\n", (unsigned long long) offset);
				report.corrupt = 1;
			} else memcpy(echobuf + offset, data, datalen);
		}
		if(payload_size == 0)
			printf("[ngtcp2] поток %lld: принято %zu байт: %.*s\n", (long long) stream_id, datalen, (int) datalen, (const char *) data);
		/*
		 * Сообщаем ngtcp2, что данные потреблены приложением: без этого окна
		 * управления потоком не расширяются и передача встаёт на пределе
		 */
		ngtcp2_conn_extend_max_stream_offset(conn, stream_id, datalen);
		ngtcp2_conn_extend_max_offset(conn, datalen);
	}
	if(flags & NGTCP2_STREAM_DATA_FLAG_FIN){
		printf("[ngtcp2] поток %lld: принят признак завершения, всего %zu байт\n", (long long) stream_id, report.echolen);
		c->fin_received = 1;
	}
	return 0;
}

/*
 * Согласование версии протокола (RFC 9000 §6): сервер, не знающий предложенной
 * версии, отвечает пакетом со списком поддерживаемых им версий.
 */
static int recv_version_negotiation_cb(ngtcp2_conn * conn, const ngtcp2_pkt_hd * hd, const uint32_t * sv, size_t nsv, void * user_data){
	size_t i;
	(void) conn;
	(void) hd;
	(void) user_data;
	report.vneg = 1;
	printf("[ngtcp2] принято согласование версии, сервер поддерживает %zu:", nsv);
	for(i = 0; i < nsv; i++){
		printf(" 0x%08x", sv[i]);
		/* Запоминаем, предложена ли первая версия протокола */
		if(sv[i] == NGTCP2_PROTO_VER_V1)
			report.vneg = 2;
	}
	printf("\n");
	return 0;
}

/*
 * Результат проверки пути после миграции (RFC 9000 §8.2): вызывается, когда
 * приходит PATH_RESPONSE либо истекает время ожидания.
 */
static int path_validation_cb(ngtcp2_conn * conn, uint32_t flags, const ngtcp2_path * path, const ngtcp2_path * old_path, ngtcp2_path_validation_result res, void * user_data){
	(void) conn;
	(void) flags;
	(void) path;
	(void) old_path;
	(void) user_data;
	if(res == NGTCP2_PATH_VALIDATION_RESULT_SUCCESS){
		if(report.migrated == 1){
			report.migrated = 2;
			printf("[ngtcp2] новый путь подтверждён\n");
		}
	} else {
		fprintf(stderr, "[ngtcp2] проверка пути не удалась\n");
		report.failed = 1;
	}
	return 0;
}

/* Сеанс завершён, когда выполнено всё, что было запрошено */
static int client_finished(const struct client * c){
	if(!c->fin_received)
		return 0;
	if(use_datagram && !report.datagram)
		return 0;
	if(use_migration && (report.migrated != 2))
		return 0;
	return 1;
}

static int recv_datagram_cb(ngtcp2_conn * conn, uint32_t flags, const uint8_t * data, size_t datalen, void * user_data){
	(void) conn;
	(void) flags;
	(void) user_data;
	report.datagram = 1;
	printf("[ngtcp2] принята датаграмма: %zu байт: %.*s\n", datalen, (int) datalen, (const char *) data);
	return 0;
}

static int extend_max_local_streams_bidi(ngtcp2_conn * conn, uint64_t max_streams, void * user_data){
	struct client * c = user_data;
	int64_t stream_id;
	(void) max_streams;
	if(c->stream.stream_id != -1 || idle_probe)
		return 0;
	if(ngtcp2_conn_open_bidi_stream(conn, &stream_id, NULL) != 0)
		return 0;
	c->stream.stream_id = stream_id;
	c->stream.data = payload;
	c->stream.datalen = payloadlen;
	printf("[ngtcp2] открыт поток %lld\n", (long long) stream_id);
	return 0;
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

static ngtcp2_conn * get_conn(ngtcp2_crypto_conn_ref * conn_ref){
	struct client * c = conn_ref->user_data;
	return c->conn;
}

static int client_quic_init(struct client * c, const struct sockaddr * remote_addr, socklen_t remote_addrlen, const struct sockaddr * local_addr, socklen_t local_addrlen){
	ngtcp2_path path = {
		.local = {.addr = (struct sockaddr *) local_addr, .addrlen = local_addrlen},
		.remote = {.addr = (struct sockaddr *) remote_addr, .addrlen = remote_addrlen},
	};
	ngtcp2_callbacks callbacks = {
		.client_initial = ngtcp2_crypto_client_initial_cb,
		.recv_crypto_data = ngtcp2_crypto_recv_crypto_data_cb,
		.handshake_completed = handshake_completed_cb,
		.encrypt = ngtcp2_crypto_encrypt_cb,
		.decrypt = ngtcp2_crypto_decrypt_cb,
		.hp_mask = ngtcp2_crypto_hp_mask_cb,
		.recv_stream_data = recv_stream_data_cb,
		.recv_retry = ngtcp2_crypto_recv_retry_cb,
		.extend_max_local_streams_bidi = extend_max_local_streams_bidi,
		.rand = rand_cb,
		.update_key = ngtcp2_crypto_update_key_cb,
		.delete_crypto_aead_ctx = ngtcp2_crypto_delete_crypto_aead_ctx_cb,
		.delete_crypto_cipher_ctx = ngtcp2_crypto_delete_crypto_cipher_ctx_cb,
		.version_negotiation = ngtcp2_crypto_version_negotiation_cb,
		.get_new_connection_id2 = get_new_connection_id_cb,
		.get_path_challenge_data2 = ngtcp2_crypto_get_path_challenge_data2_cb,
		.recv_datagram = recv_datagram_cb,
		.path_validation = path_validation_cb,
		.recv_version_negotiation = recv_version_negotiation_cb,
	};
	ngtcp2_cid dcid, scid;
	ngtcp2_settings settings;
	ngtcp2_transport_params params;
	int rv;
	dcid.datalen = NGTCP2_MIN_INITIAL_DCIDLEN;
	if(RAND_bytes(dcid.data, (int) dcid.datalen) != 1){
		fprintf(stderr, "RAND_bytes failed\n");
		return -1;
	}
	scid.datalen = 8;
	if(RAND_bytes(scid.data, (int) scid.datalen) != 1){
		fprintf(stderr, "RAND_bytes failed\n");
		return -1;
	}
	ngtcp2_settings_default(&settings);
	settings.initial_ts = timestamp();
	settings.log_printf = log_printf;
	ngtcp2_transport_params_default(&params);
	params.initial_max_streams_uni = 3;
	params.initial_max_streams_bidi = 3;
	params.initial_max_stream_data_bidi_local = 128 * 1024;
	params.initial_max_stream_data_bidi_remote = 128 * 1024;
	params.initial_max_stream_data_uni = 128 * 1024;
	params.initial_max_data = 1024 * 1024;
	params.max_datagram_frame_size = 1200;
	rv = ngtcp2_conn_client_new(&c->conn, &dcid, &scid, &path, proto_version, &callbacks, &settings, &params, NULL, c);
	if(rv != 0){
		fprintf(stderr, "ngtcp2_conn_client_new: %s\n", ngtcp2_strerror(rv));
		return -1;
	}
	ngtcp2_conn_set_tls_native_handle(c->conn, c->ssl);
	return 0;
}

static int client_read(struct client * c){
	uint8_t buf[65536];
	struct sockaddr_storage addr;
	struct iovec iov = {.iov_base = buf, .iov_len = sizeof(buf)};
	struct msghdr msg = {0};
	ssize_t nread;
	ngtcp2_path path;
	ngtcp2_pkt_info pi = {0};
	int rv;
	msg.msg_name = &addr;
	msg.msg_iov = &iov;
	msg.msg_iovlen = 1;
	for(;;){
		msg.msg_namelen = sizeof(addr);
		nread = recvmsg(c->fd, &msg, MSG_DONTWAIT);
		if(nread == -1){
			if(errno != EAGAIN && errno != EWOULDBLOCK)
				fprintf(stderr, "recvmsg: %s\n", strerror(errno));
			break;
		}
		path.local.addrlen = c->local_addrlen;
		path.local.addr = (struct sockaddr *) &c->local_addr;
		path.remote.addrlen = msg.msg_namelen;
		path.remote.addr = msg.msg_name;
		rv = ngtcp2_conn_read_pkt(c->conn, &path, &pi, buf, (size_t) nread, timestamp());
		if(rv != 0){
			fprintf(stderr, "[ngtcp2] ngtcp2_conn_read_pkt: %s\n", ngtcp2_strerror(rv));
			if(!c->last_error.error_code){
				if(rv == NGTCP2_ERR_CRYPTO)
					ngtcp2_ccerr_set_tls_alert(&c->last_error, ngtcp2_conn_get_tls_alert2(c->conn), NULL, 0);
				else ngtcp2_ccerr_set_liberr(&c->last_error, rv, NULL, 0);
			}
			report.failed = 1;
			return -1;
		}
	}
	return 0;
}

static int client_send_packet(struct client * c, const uint8_t * data, size_t datalen){
	struct iovec iov = {.iov_base = (uint8_t *) data, .iov_len = datalen};
	struct msghdr msg = {0};
	ssize_t nwrite;
	msg.msg_iov = &iov;
	msg.msg_iovlen = 1;
	do {
		nwrite = sendmsg(c->fd, &msg, 0);
	} while(nwrite == -1 && errno == EINTR);
	if(nwrite == -1){
		fprintf(stderr, "sendmsg: %s\n", strerror(errno));
		return -1;
	}
	return 0;
}

static size_t client_get_message(struct client * c, int64_t * pstream_id, int * pfin, ngtcp2_vec * datav, size_t datavcnt){
	if(datavcnt == 0)
		return 0;
	if(c->stream.stream_id != -1 && c->stream.nwrite < c->stream.datalen){
		*pstream_id = c->stream.stream_id;
		*pfin = 1;
		datav->base = (uint8_t *) c->stream.data + c->stream.nwrite;
		datav->len = c->stream.datalen - c->stream.nwrite;
		return 1;
	}
	*pstream_id = -1;
	*pfin = 0;
	datav->base = NULL;
	datav->len = 0;
	return 0;
}

/* Отправка датаграммы приложения, если она запрошена и рукопожатие завершено */
static int client_write_datagram(struct client * c){
	uint8_t buf[1452];
	ngtcp2_pkt_info pi;
	ngtcp2_path_storage ps;
	ngtcp2_ssize nwrite;
	int accepted = 0;
	ngtcp2_vec datav;
	static const char payload[] = "datagram-interop";
	if(!use_datagram || c->datagram_sent || !report.handshake)
		return 0;
	if(ngtcp2_conn_get_max_tx_udp_payload_size(c->conn) == 0)
		return 0;
	ngtcp2_path_storage_zero(&ps);
	datav.base = (uint8_t *) payload;
	datav.len = sizeof(payload) - 1;
	nwrite = ngtcp2_conn_writev_datagram(c->conn, &ps.path, &pi, buf, sizeof(buf), &accepted, NGTCP2_WRITE_DATAGRAM_FLAG_NONE, 0, &datav, 1, timestamp());
	if(nwrite < 0){
		fprintf(stderr, "[ngtcp2] ngtcp2_conn_writev_datagram: %s\n", ngtcp2_strerror((int) nwrite));
		return 0;
	}
	if(nwrite == 0)
		return 0;
	if(accepted > 0){
		c->datagram_sent = 1;
		printf("[ngtcp2] отправлена датаграмма: %zu байт\n", sizeof(payload) - 1);
	}
	return client_send_packet(c, buf, (size_t) nwrite);
}

static int client_write_streams(struct client * c){
	ngtcp2_tstamp ts = timestamp();
	ngtcp2_pkt_info pi;
	ngtcp2_ssize nwrite;
	uint8_t buf[1452];
	ngtcp2_path_storage ps;
	ngtcp2_vec datav;
	size_t datavcnt;
	int64_t stream_id;
	ngtcp2_ssize wdatalen;
	uint32_t flags;
	int fin, blocked = 0;
	ngtcp2_path_storage_zero(&ps);
	for(;;){
		datavcnt = client_get_message(c, &stream_id, &fin, &datav, 1);
		/*
		 * Поток упёрся в окно управления потоком: продолжаем формировать пакеты
		 * без данных потока, чтобы подтверждения и служебные фреймы уходили
		 */
		if(blocked){
			stream_id = -1;
			datavcnt = 0;
			fin = 0;
		}
		flags = NGTCP2_WRITE_STREAM_FLAG_MORE;
		if(fin)
			flags |= NGTCP2_WRITE_STREAM_FLAG_FIN;
		nwrite = ngtcp2_conn_writev_stream(c->conn, &ps.path, &pi, buf, sizeof(buf), &wdatalen, flags, stream_id, &datav, datavcnt, ts);
		if(nwrite < 0){
			switch(nwrite){
				case NGTCP2_ERR_WRITE_MORE:
					c->stream.nwrite += (size_t) wdatalen;
					continue;
				case NGTCP2_ERR_STREAM_DATA_BLOCKED:
				case NGTCP2_ERR_STREAM_SHUT_WR:
					blocked = 1;
					continue;
				default:
					fprintf(stderr, "[ngtcp2] ngtcp2_conn_writev_stream: %s\n", ngtcp2_strerror((int) nwrite));
					ngtcp2_ccerr_set_liberr(&c->last_error, (int) nwrite, NULL, 0);
					report.failed = 1;
					return -1;
			}
		}
		if(nwrite == 0)
			return 0;
		if(wdatalen > 0)
			c->stream.nwrite += (size_t) wdatalen;
		if(client_send_packet(c, buf, (size_t) nwrite) != 0)
			break;
	}
	return 0;
}

static int client_write(struct client * c){
	if(client_write_datagram(c) != 0)
		return -1;
	if(client_write_streams(c) != 0)
		return -1;
	return 0;
}

static void client_close(struct client * c){
	ngtcp2_ssize nwrite;
	ngtcp2_pkt_info pi;
	ngtcp2_path_storage ps;
	uint8_t buf[1280];
	if(ngtcp2_conn_in_closing_period2(c->conn) || ngtcp2_conn_in_draining_period2(c->conn))
		return;
	ngtcp2_path_storage_zero(&ps);
	nwrite = ngtcp2_conn_write_connection_close(c->conn, &ps.path, &pi, buf, sizeof(buf), &c->last_error, timestamp());
	if(nwrite < 0){
		fprintf(stderr, "[ngtcp2] ngtcp2_conn_write_connection_close: %s\n", ngtcp2_strerror((int) nwrite));
		return;
	}
	if(client_send_packet(c, buf, (size_t) nwrite) == 0){
		report.closed = 1;
		printf("[ngtcp2] отправлено завершение соединения\n");
	}
}

/*
 * Обновление ключей защиты пакетов (RFC 9001 §6): инициируется один раз,
 * после чего обмен продолжается на новом фазовом бите.
 */
static void client_key_update(struct client * c){
	int rv = ngtcp2_conn_initiate_key_update(c->conn, timestamp());
	if(rv != 0){
		fprintf(stderr, "[ngtcp2] ngtcp2_conn_initiate_key_update: %s\n", ngtcp2_strerror(rv));
		return;
	}
	report.keyupdate = 1;
	printf("[ngtcp2] инициировано обновление ключей\n");
}

/*
 * Миграция соединения на новый локальный адрес (RFC 9000 §9): открывается новый
 * сокет со случайным портом, после чего путь проверяется через PATH_CHALLENGE.
 */
static int client_migrate(struct client * c, const struct sockaddr * remote_addr, socklen_t remote_addrlen){
	struct sockaddr_storage local_addr;
	socklen_t local_addrlen = sizeof(local_addr);
	ngtcp2_path path;
	int fd, rv;
	fd = socket(remote_addr->sa_family, SOCK_DGRAM, 0);
	if(fd == -1){
		fprintf(stderr, "socket: %s\n", strerror(errno));
		return -1;
	}
	if(connect_sock((struct sockaddr *) &local_addr, &local_addrlen, fd, remote_addr, remote_addrlen) != 0){
		close(fd);
		return -1;
	}
	path.local.addr = (struct sockaddr *) &local_addr;
	path.local.addrlen = local_addrlen;
	path.remote.addr = (struct sockaddr *) remote_addr;
	path.remote.addrlen = remote_addrlen;
	rv = ngtcp2_conn_initiate_migration(c->conn, &path, timestamp());
	if(rv != 0){
		fprintf(stderr, "[ngtcp2] ngtcp2_conn_initiate_migration: %s\n", ngtcp2_strerror(rv));
		close(fd);
		return -1;
	}
	close(c->fd);
	c->fd = fd;
	memcpy(&c->local_addr, &local_addr, sizeof(c->local_addr));
	c->local_addrlen = local_addrlen;
	report.migrated = 1;
	printf("[ngtcp2] запрошена миграция на новый локальный порт %u\n", (unsigned) ntohs(((struct sockaddr_in *) &local_addr)->sin_port));
	return 0;
}

static void usage(const char * name){
	fprintf(stderr, "Использование: %s [опции]\n"
		"  --host <адрес>     адрес сервера, по умолчанию 127.0.0.1\n"
		"  --port <порт>      порт сервера, по умолчанию 2222\n"
		"  --alpn <протокол>  ALPN без префикса длины, по умолчанию h3\n"
		"  --message <текст>  полезная нагрузка потока\n"
		"  --size <байт>      вместо текста передать блок заданного размера\n"
		"  --datagram         дополнительно отправить датаграмму приложения\n"
		"  --idle             не открывать поток, проверить только рукопожатие\n"
		"  --key-update       обновить ключи в середине передачи\n"
		"  --migrate          мигрировать на новый локальный порт в середине передачи\n"
		"  --version <число>  версия протокола, по умолчанию 1\n"
		"  --deadline <мс>    предельное время сеанса, по умолчанию 5000\n"
		"  --verbose          подробный журнал ngtcp2\n", name);
}

int main(int argc, char * argv[]){
	struct client c;
	struct sockaddr_storage remote_addr, local_addr;
	socklen_t remote_addrlen, local_addrlen = sizeof(local_addr);
	static char alpnbuf[64];
	uint64_t started;
	int i;
	for(i = 1; i < argc; i++){
		if(!strcmp(argv[i], "--host") && (i + 1) < argc)
			remote_host = argv[++i];
		else if(!strcmp(argv[i], "--port") && (i + 1) < argc)
			remote_port = argv[++i];
		else if(!strcmp(argv[i], "--alpn") && (i + 1) < argc){
			const char * value = argv[++i];
			size_t len = strlen(value);
			if(len == 0 || len > sizeof(alpnbuf) - 2){
				fprintf(stderr, "недопустимое значение ALPN\n");
				return EXIT_FAILURE;
			}
			alpnbuf[0] = (char) len;
			memcpy(alpnbuf + 1, value, len);
			alpn = alpnbuf;
			alpnlen = len + 1;
		} else if(!strcmp(argv[i], "--message") && (i + 1) < argc)
			message = argv[++i];
		else if(!strcmp(argv[i], "--deadline") && (i + 1) < argc)
			deadline_ms = strtoull(argv[++i], NULL, 10);
		else if(!strcmp(argv[i], "--size") && (i + 1) < argc)
			payload_size = (size_t) strtoull(argv[++i], NULL, 10);
		else if(!strcmp(argv[i], "--version") && (i + 1) < argc)
			proto_version = (uint32_t) strtoul(argv[++i], NULL, 0);
		else if(!strcmp(argv[i], "--datagram"))
			use_datagram = 1;
		else if(!strcmp(argv[i], "--key-update"))
			use_key_update = 1;
		else if(!strcmp(argv[i], "--migrate"))
			use_migration = 1;
		else if(!strcmp(argv[i], "--idle"))
			idle_probe = 1;
		else if(!strcmp(argv[i], "--verbose"))
			verbose = 1;
		else {
			usage(argv[0]);
			return EXIT_FAILURE;
		}
	}
	/* Готовим полезную нагрузку: либо текст, либо детерминированный блок */
	if(payload_size > 0){
		size_t n;
		payload = malloc(payload_size);
		echobuf = calloc(1, payload_size);
		if((payload == NULL) || (echobuf == NULL)){
			fprintf(stderr, "недостаточно памяти\n");
			return EXIT_FAILURE;
		}
		for(n = 0; n < payload_size; n++)
			payload[n] = (uint8_t) ('a' + (n % 26));
		payloadlen = payload_size;
	} else {
		payload = (uint8_t *) message;
		payloadlen = strlen(message);
		echobuf = calloc(1, payloadlen);
	}
	memset(&c, 0, sizeof(c));
	memset(&report, 0, sizeof(report));
	ngtcp2_ccerr_default(&c.last_error);
	c.fd = create_sock((struct sockaddr *) &remote_addr, &remote_addrlen, remote_host, remote_port);
	if(c.fd == -1)
		return EXIT_FAILURE;
	if(connect_sock((struct sockaddr *) &local_addr, &local_addrlen, c.fd, (struct sockaddr *) &remote_addr, remote_addrlen) != 0)
		return EXIT_FAILURE;
	memcpy(&c.local_addr, &local_addr, sizeof(c.local_addr));
	c.local_addrlen = local_addrlen;
	c.stream.stream_id = -1;
	c.conn_ref.get_conn = get_conn;
	c.conn_ref.user_data = &c;
	if(client_ssl_init(&c) != 0)
		return EXIT_FAILURE;
	if(client_quic_init(&c, (struct sockaddr *) &remote_addr, remote_addrlen, (struct sockaddr *) &local_addr, local_addrlen) != 0)
		return EXIT_FAILURE;
	printf("[ngtcp2] подключение к %s:%s, ALPN=%.*s\n", remote_host, remote_port, (int) alpnlen - 1, alpn + 1);
	started = millis();
	if(client_write(&c) != 0)
		report.failed = 1;
	while(!c.done && !report.failed){
		struct pollfd pfd = {.fd = c.fd, .events = POLLIN};
		ngtcp2_tstamp expiry = ngtcp2_conn_get_expiry2(c.conn);
		uint64_t now = timestamp();
		int wait = 10;
		if((millis() - started) > deadline_ms){
			fprintf(stderr, "[ngtcp2] превышено предельное время сеанса\n");
			break;
		}
		if(expiry != UINT64_MAX){
			uint64_t delta = (expiry <= now ? 0 : (expiry - now) / NGTCP2_MILLISECONDS);
			wait = (delta > 100 ? 100 : (int) delta);
		}
		if(poll(&pfd, 1, wait) > 0){
			if(client_read(&c) != 0)
				break;
		}
		if(ngtcp2_conn_get_expiry2(c.conn) <= timestamp()){
			int rv = ngtcp2_conn_handle_expiry(c.conn, timestamp());
			if(rv != 0){
				fprintf(stderr, "[ngtcp2] ngtcp2_conn_handle_expiry: %s\n", ngtcp2_strerror(rv));
				ngtcp2_ccerr_set_liberr(&c.last_error, rv, NULL, 0);
				report.failed = 1;
				break;
			}
		}
		if(ngtcp2_conn_in_closing_period2(c.conn) || ngtcp2_conn_in_draining_period2(c.conn)){
			printf("[ngtcp2] соединение завершено удалённой стороной\n");
			report.closed = 1;
			break;
		}
		/* В режиме проверки простоя выходим сразу после рукопожатия */
		if(idle_probe && report.handshake)
			break;
		/*
		 * Обновление ключей и миграцию запускаем в середине передачи: к этому
		 * моменту рукопожатие завершено, а обмен ещё продолжается
		 */
		if(report.handshake && (report.echolen > 0)){
			if(use_key_update && !report.keyupdate)
				client_key_update(&c);
			if(use_migration && !report.migrated)
				client_migrate(&c, (struct sockaddr *) &remote_addr, remote_addrlen);
		}
		if(client_write(&c) != 0)
			break;
		if(client_finished(&c))
			c.done = 1;
	}
	if(!report.closed)
		client_close(&c);
	printf("\n=== Итог сверки ===\n");
	printf("рукопожатие:      %s\n", report.handshake ? "да" : "нет");
	printf("ALPN:             %s\n", report.alpn[0] ? report.alpn : "нет");
	if(!idle_probe){
		printf("эхо потока:       %s (%zu из %zu байт)\n", report.echo ? "да" : "нет", report.echolen, payloadlen);
		printf("содержимое эха:   %s\n", report.corrupt ? "РАСХОЖДЕНИЕ" : (report.echolen == payloadlen ? "совпадает" : "неполное"));
	}
	if(use_datagram)
		printf("эхо датаграммы:   %s\n", report.datagram ? "да" : "нет");
	if(use_key_update)
		printf("обновление ключей:%s\n", report.keyupdate ? " выполнено" : " нет");
	if(use_migration)
		printf("миграция:         %s\n", report.migrated == 2 ? "путь подтверждён" : (report.migrated ? "не подтверждена" : "нет"));
	if(proto_version != NGTCP2_PROTO_VER_V1)
		printf("согласование:     %s\n", report.vneg == 2 ? "получено, предложена версия 1" : (report.vneg ? "получено без версии 1" : "нет"));
	printf("завершение:       %s\n", report.closed ? "да" : "нет");
	printf("ошибки:           %s\n", report.failed ? "да" : "нет");
	if(report.corrupt || (!idle_probe && (report.echolen != payloadlen)))
		report.failed = 1;
	if(use_migration && (report.migrated != 2))
		report.failed = 1;
	ngtcp2_conn_del(c.conn);
	SSL_free(c.ssl);
	SSL_CTX_free(c.ssl_ctx);
	return (report.failed || !report.handshake) ? EXIT_FAILURE : EXIT_SUCCESS;
}
