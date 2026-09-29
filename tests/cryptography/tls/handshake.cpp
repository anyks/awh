/**
 * @file handshake.cpp
 * @date 2026-07-22
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
 * @brief Тесты защищённого рукопожатия — проверка полного цикла согласования TLS и DTLS между клиентом и сервером,
 *        верификации сертификатов и выбора протокола ALPN
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Стандартные заголовочные файлы
 */
#include <string>
#include <vector>

/**
 * Заголовочные файлы сжатия и криптографической библиотеки
 */
#include <zlib.h>
#include <zstd.h>
#include <brotli/encode.h>
#include <openssl/ssl.h>

/**
 * Заголовочные файлы учёта ресурсов процесса
 */
#if !defined(_WIN32) && !defined(_WIN64)
	#include <sys/resource.h>
#endif

/**
 * Подключаем заголовочный файл тестов кодера
 */
#include "tls.hpp"

/**
 * Подключаем заголовочный файл блочного компрессора
 */
#include "../../../include/compressor/block.hpp"

/**
 * Подписываемся на пространство имён проекта
 */
using namespace awh;

/**
 * @brief Внутренние вспомогательные средства тестов рукопожатия
 *
 */
namespace {
	/**
	 * @brief Структура эндпоинта тестового обмена
	 *
	 * @details Кодер отдаёт исходящий шифртекст функцией обратного вызова чтения
	 *          с событием шифрования, а входящий принимает методом расшифровки.
	 *          Эндпоинт накапливает исходящий шифртекст и выданный приложению
	 *          открытый текст, что позволяет прогнать рукопожатие без сокетов
	 *
	 */
	typedef struct Endpoint {
		// Идентификатор шаблона контекста безопасности
		tls::Coder::id_t context;
		// Идентификатор транспортного уровня передачи
		tls::Coder::id_t transport;
		// Накопленный исходящий шифртекст
		std::string outgoing;
		// Накопленный принятый открытый текст
		std::string incoming;
		// Флаг выполненного рукопожатия
		bool handshaked;
		// Флаг отказа рукопожатия
		bool failed;
		// Количество переданных пиру октетов шифртекста
		size_t sent;
		/**
		 * @brief Конструктор
		 *
		 */
		explicit Endpoint() noexcept :
		 context(0), transport(0), handshaked(false), failed(false), sent(0) {}
	} endpoint_t;

	/**
	 * @brief Функция подключения функций обратного вызова эндпоинта
	 *
	 * @param coder    объект кодера транспортной безопасности
	 * @param endpoint эндпоинт тестового обмена
	 *
	 */
	static void subscribe(tls::Coder & coder, endpoint_t & endpoint) noexcept {
		// Устанавливаем функцию обратного вызова чтения
		coder.on(endpoint.transport, [&endpoint](const tls::Coder::id_t, const tls::Coder::event_t event, const uint8_t * buffer, const size_t size) noexcept -> void {
			/**
			 * Определяем тип события кодера
			 */
			switch(static_cast <uint8_t> (event)){
				// Если событие является шифрованием - данные готовы к отправке пиру
				case static_cast <uint8_t> (tls::Coder::event_t::ENCRYPTION):
					// Накапливаем исходящий шифртекст
					endpoint.outgoing.append(reinterpret_cast <const char *> (buffer), size);
				break;
				// Если событие является расшифровкой - данные предназначены приложению
				case static_cast <uint8_t> (tls::Coder::event_t::DECRYPTION):
					// Накапливаем принятый открытый текст
					endpoint.incoming.append(reinterpret_cast <const char *> (buffer), size);
				break;
			}
		});
		// Устанавливаем функцию обратного вызова изменения состояния
		coder.on(endpoint.transport, [&endpoint](const tls::Coder::id_t, const tls::Coder::state_t state) noexcept -> void {
			/**
			 * Определяем состояние кодера
			 */
			switch(static_cast <uint8_t> (state)){
				// Если рукопожатие выполнено
				case static_cast <uint8_t> (tls::Coder::state_t::HANDSHAKED):
					// Устанавливаем флаг выполненного рукопожатия
					endpoint.handshaked = true;
				break;
				// Если рукопожатие завершилось отказом
				case static_cast <uint8_t> (tls::Coder::state_t::HANDSHAKE_FAILED):
				// Если работа завершилась ошибкой
				case static_cast <uint8_t> (tls::Coder::state_t::FAILED):
					// Устанавливаем флаг отказа рукопожатия
					endpoint.failed = true;
				break;
			}
		});
	}
	/**
	 * @brief Функция передачи накопленного шифртекста между эндпоинтами
	 *
	 * @param coder объект кодера транспортной безопасности
	 * @param from  эндпоинт-отправитель
	 * @param to    эндпоинт-получатель
	 * @return      количество переданных октетов
	 *
	 */
	static size_t transfer(tls::Coder & coder, endpoint_t & from, endpoint_t & to) noexcept {
		// Если исходящих данных нет
		if(from.outgoing.empty())
			// Выводим нулевое количество переданных октетов
			return 0;
		// Забираем накопленный исходящий шифртекст
		const std::string buffer = from.outgoing;
		// Очищаем буфер исходящего шифртекста
		from.outgoing.clear();
		// Учитываем переданные пиру октеты
		from.sent += buffer.size();
		// Передаём шифртекст получателю на расшифровку
		coder.decrypt(to.transport, buffer.data(), buffer.size());
		// Выводим количество переданных октетов
		return buffer.size();
	}
	/**
	 * @brief Функция выполнения рукопожатия между эндпоинтами
	 *
	 * @param coder  объект кодера транспортной безопасности
	 * @param client эндпоинт клиента
	 * @param server эндпоинт сервера
	 * @return       результат выполнения рукопожатия
	 *
	 */
	static bool establish(tls::Coder & coder, endpoint_t & client, endpoint_t & server) noexcept {
		/**
		 * Инициируем рукопожатие на обоих эндпоинтах однократно: далее оно
		 * продвигается подачей принятого шифртекста на расшифровку, как это
		 * устроено в рабочих примерах модуля
		 */
		coder.handshake(client.transport);
		// Инициируем рукопожатие сервера
		coder.handshake(server.transport);
		/**
		 * Выполняем обмен до завершения рукопожатия на обоих эндпоинтах
		 */
		for(size_t i = 0; i < 32; i++){
			// Передаём исходящий шифртекст клиента серверу
			const size_t sent = ::transfer(coder, client, server);
			// Передаём исходящий шифртекст сервера клиенту
			const size_t received = ::transfer(coder, server, client);
			// Если рукопожатие выполнено на обоих эндпоинтах
			if(client.handshaked && server.handshaked)
				// Выводим положительный результат
				return true;
			// Если рукопожатие завершилось отказом
			if(client.failed || server.failed)
				// Выводим отрицательный результат
				return false;
			// Если обмен данными прекратился
			if((sent == 0) && (received == 0))
				// Выводим отрицательный результат - обмен не сошёлся
				return false;
		}
		// Выводим отрицательный результат - обмен не сошёлся
		return false;
	}
	/**
	 * @brief Функция настройки пары эндпоинтов одностороннего TLS
	 *
	 * @param coder       объект кодера транспортной безопасности
	 * @param client      эндпоинт клиента
	 * @param server      эндпоинт сервера
	 * @param certificate сертификат сервера
	 * @param privateKey  приватный ключ сервера
	 *
	 */
	static void contexts(tls::Coder & coder, endpoint_t & client, endpoint_t & server, const std::string & certificate, const std::string & privateKey) noexcept {
		// Создаём шаблоны контекста безопасности
		client.context = coder.context(event::node_t::CLIENT, event::protocol_t::TCP);
		server.context = coder.context(event::node_t::SERVER, event::protocol_t::TCP);
		// Устанавливаем сертификат и приватный ключ сервера
		coder.certificate(server.context, certificate);
		coder.privateKey(server.context, privateKey);
		// Устанавливаем доверенный центр сертификации клиента
		coder.ca(client.context, certificate);
		// Устанавливаем доменное имя удалённого узла на клиенте
		coder.serverNameIndication(client.context, "localhost");
		// Снимаем требование клиентского сертификата на сервере
		coder.validateServerNameIndication(server.context, false);
	}
	/**
	 * @brief Функция запуска транспортных уровней пары эндпоинтов
	 *
	 * @param coder  объект кодера транспортной безопасности
	 * @param client эндпоинт клиента
	 * @param server эндпоинт сервера
	 *
	 */
	static void transports(tls::Coder & coder, endpoint_t & client, endpoint_t & server) noexcept {
		// Создаём транспортные уровни
		client.transport = coder.transport(client.context);
		server.transport = coder.transport(server.context);
		// Подключаем функции обратного вызова эндпоинтов
		::subscribe(coder, client);
		::subscribe(coder, server);
	}
	/**
	 * Размер распакованной бомбы сжатия. Сжатая бомба обязана уложиться в
	 * предел сообщения рукопожатия, который криптографическая библиотека
	 * ставит клиенту TLS: SSL_MAX_CERT_LIST_DEFAULT = 100 КБ. Бомба длиннее
	 * отвергается до распаковки (EXCESSIVE_MESSAGE_SIZE) - и проверка слепнет:
	 * так и случилось на 512 и 128 МиБ, поскольку zlib сжимает нули не плотнее
	 * ~1000:1. 90 МиБ дают ~92 КБ
	 */
	static constexpr size_t BOMB = (90ull << 20);
	/**
	 * Готовая бомба сжатия в формате zlib (RFC 1950)
	 */
	static std::vector <uint8_t> bomb;
	/**
	 * @brief Функция сборки бомбы сжатия потоком
	 *
	 * @details Нули подаются кусками по 1 МиБ, поэтому сама сборка пика памяти
	 *          не поднимает: иначе замер пика в проверке ослеп бы. Размер
	 *          распакованных данных в заголовок потока не пишется - распаковщик
	 *          не может отвести память по нему заранее
	 *
	 * @param size   размер распакованных данных
	 * @param method метод сжатия
	 * @return       сжатые данные либо пустой буфер при отказе
	 */
	static std::vector <uint8_t> detonator(const size_t size, const compressor::method_t method) noexcept {
		// Результат сжатия
		std::vector <uint8_t> result;
		// Кусок нулей на подачу
		const std::vector <uint8_t> zeros(1 << 20, 0);
		// Буфер выхода
		uint8_t out[65536];
		/**
		 * Определяем метод сжатия
		 */
		switch(static_cast <uint8_t> (method)){
			// Если метод сжатия Zlib (RFC 1950)
			case static_cast <uint8_t> (compressor::method_t::ZLIB): {
				// Поток сжатия
				z_stream zs{};
				// Если поток инициализировать не удалось
				if(::deflateInit2(&zs, Z_BEST_COMPRESSION, Z_DEFLATED, MAX_WBITS, MAX_MEM_LEVEL, Z_DEFAULT_STRATEGY) != Z_OK)
					// Выводим пустой результат
					return result;
				/**
				 * Подаём нули кусками до заданного размера
				 */
				for(size_t fed = 0; fed < size; fed += zeros.size()){
					// Признак последней подачи
					const bool last = ((fed + zeros.size()) >= size);
					// Устанавливаем вход
					zs.next_in  = const_cast <Bytef *> (zeros.data());
					zs.avail_in = static_cast <uInt> (std::min(zeros.size(), size - fed));
					/**
					 * Забираем выход до опустошения входа
					 */
					do {
						// Устанавливаем выход
						zs.next_out  = out;
						zs.avail_out = sizeof(out);
						// Выполняем сжатие
						::deflate(&zs, (last ? Z_FINISH : Z_NO_FLUSH));
						// Добавляем полученный выход
						result.insert(result.end(), out, out + (sizeof(out) - zs.avail_out));
					} while(zs.avail_out == 0);
				}
				// Завершаем поток
				::deflateEnd(&zs);
			} break;
			// Если метод сжатия Brotli (RFC 7932)
			case static_cast <uint8_t> (compressor::method_t::BROTLI): {
				// Поток сжатия
				BrotliEncoderState * state = ::BrotliEncoderCreateInstance(nullptr, nullptr, nullptr);
				// Если поток создать не удалось
				if(state == nullptr)
					// Выводим пустой результат
					return result;
				/**
				 * Подаём нули кусками до заданного размера
				 */
				for(size_t fed = 0; fed < size; fed += zeros.size()){
					// Признак последней подачи
					const bool last = ((fed + zeros.size()) >= size);
					// Вход подачи
					const uint8_t * next = zeros.data();
					size_t available = std::min(zeros.size(), size - fed);
					/**
					 * Забираем выход до опустошения входа и завершения потока
					 */
					do {
						// Выход подачи
						uint8_t * target = out;
						size_t room = sizeof(out);
						// Если сжатие не удалось
						if(!::BrotliEncoderCompressStream(state, (last ? BROTLI_OPERATION_FINISH : BROTLI_OPERATION_PROCESS), &available, &next, &room, &target, nullptr)){
							// Удаляем поток
							::BrotliEncoderDestroyInstance(state);
							// Выводим пустой результат
							return std::vector <uint8_t> ();
						}
						// Добавляем полученный выход
						result.insert(result.end(), out, out + (sizeof(out) - room));
					} while((available > 0) || ::BrotliEncoderHasMoreOutput(state) || (last && !::BrotliEncoderIsFinished(state)));
				}
				// Удаляем поток
				::BrotliEncoderDestroyInstance(state);
			} break;
			// Если метод сжатия Zstandard (RFC 8878)
			case static_cast <uint8_t> (compressor::method_t::ZSTD): {
				// Поток сжатия
				ZSTD_CCtx * context = ::ZSTD_createCCtx();
				// Если поток создать не удалось
				if(context == nullptr)
					// Выводим пустой результат
					return result;
				/**
				 * Подаём нули кусками до заданного размера
				 */
				for(size_t fed = 0; fed < size; fed += zeros.size()){
					// Признак последней подачи
					const bool last = ((fed + zeros.size()) >= size);
					// Вход подачи
					ZSTD_inBuffer input = {zeros.data(), std::min(zeros.size(), size - fed), 0};
					// Остаток сброса
					size_t remaining = 0;
					/**
					 * Забираем выход до опустошения входа и завершения кадра
					 */
					do {
						// Выход подачи
						ZSTD_outBuffer output = {out, sizeof(out), 0};
						// Выполняем сжатие
						remaining = ::ZSTD_compressStream2(context, &output, &input, (last ? ZSTD_e_end : ZSTD_e_continue));
						// Если сжатие не удалось
						if(::ZSTD_isError(remaining)){
							// Удаляем поток
							::ZSTD_freeCCtx(context);
							// Выводим пустой результат
							return std::vector <uint8_t> ();
						}
						// Добавляем полученный выход
						result.insert(result.end(), out, out + output.pos);
					} while((input.pos < input.size) || (last && (remaining > 0)));
				}
				// Удаляем поток
				::ZSTD_freeCCtx(context);
			} break;
		}
		// Выводим результат
		return result;
	}
	/**
	 * @brief Функция сжатия сертификата враждебного сервера
	 *
	 * @details Настоящий сертификат не сжимается: вместо него отдаётся бомба.
	 *          Размер uncompressed_length криптографическая библиотека ставит
	 *          сама по настоящему сообщению Certificate - то есть ЧЕСТНЫЙ,
	 *          а распакованные данные много его больше
	 *
	 */
	static int32_t explode(SSL *, CBB * out, const uint8_t *, const size_t) noexcept {
		// Выводим бомбу сжатия
		return ::CBB_add_bytes(out, ::bomb.data(), ::bomb.size());
	}
	/**
	 * @brief Функция получения пика занятой процессом памяти в октетах
	 *
	 * @return пик занятой памяти либо 0, если учёт не поддерживается
	 */
	static size_t peak() noexcept {
		#if !defined(_WIN32) && !defined(_WIN64)
			// Учёт ресурсов процесса
			struct rusage usage{};
			// Если учёт получить не удалось
			if(::getrusage(RUSAGE_SELF, &usage) != 0)
				// Выводим отсутствие учёта
				return 0;
			/**
			 * У macOS пик выдаётся в октетах, у прочих систем - в килобайтах
			 */
			#if defined(__APPLE__)
				// Выводим пик в октетах
				return static_cast <size_t> (usage.ru_maxrss);
			#else
				// Выводим пик в октетах
				return (static_cast <size_t> (usage.ru_maxrss) * 1024);
			#endif
		#else
			// Выводим отсутствие учёта
			return 0;
		#endif
	}
};

/**
 * @brief Тест полного рукопожатия между клиентом и сервером
 *
 * @details Прогон без сокетов: шифртекст переносится между эндпоинтами вручную.
 *          Тест задействует весь тракт кодера - создание контекстов, настройку
 *          сертификата, функции обратного вызова уровня контекста и передачу
 *          прикладных данных после рукопожатия
 *
 */
TEST_F(TlsFixture, HandshakeLoopbackTest){
	// Проверяем что сертификат тестового узла сгенерирован
	ASSERT_FALSE(this->_certificate.empty());
	// Эндпоинт клиента
	::endpoint_t client;
	// Эндпоинт сервера
	::endpoint_t server;
	// Создаём шаблон контекста безопасности клиента
	client.context = this->_coder->context(event::node_t::CLIENT, event::protocol_t::TCP);
	// Создаём шаблон контекста безопасности сервера
	server.context = this->_coder->context(event::node_t::SERVER, event::protocol_t::TCP);
	// Проверяем что шаблоны контекста созданы
	ASSERT_NE(client.context, 0u);
	ASSERT_NE(server.context, 0u);
	// Устанавливаем сертификат сервера
	this->_coder->certificate(server.context, this->_certificate);
	// Устанавливаем приватный ключ сервера
	this->_coder->privateKey(server.context, this->_privateKey);
	// Устанавливаем доверенный центр сертификации клиента
	this->_coder->ca(client.context, this->_certificate);
	// Устанавливаем доменное имя удалённого узла на клиенте
	this->_coder->serverNameIndication(client.context, "localhost");
	/**
	 * Снимаем проверку сертификата на сервере: шаблон контекста создаётся
	 * с включённой проверкой, а на серверном узле это означает требование
	 * клиентского сертификата, то есть взаимную аутентификацию. Тесты
	 * проверяют односторонний TLS, поэтому требование снимается явно
	 */
	this->_coder->validateServerNameIndication(server.context, false);
	// Создаём транспортный уровень клиента
	client.transport = this->_coder->transport(client.context);
	// Создаём транспортный уровень сервера
	server.transport = this->_coder->transport(server.context);
	// Проверяем что транспортные уровни созданы
	ASSERT_NE(client.transport, 0u);
	ASSERT_NE(server.transport, 0u);
	// Подключаем функции обратного вызова клиента
	::subscribe(* this->_coder, client);
	// Подключаем функции обратного вызова сервера
	::subscribe(* this->_coder, server);
	// Выполняем рукопожатие между эндпоинтами
	ASSERT_TRUE(::establish(* this->_coder, client, server));
	// Проверяем что рукопожатие выполнено на обоих эндпоинтах
	ASSERT_TRUE(client.handshaked);
	ASSERT_TRUE(server.handshaked);
	// Проверяем что согласованный шифр доступен
	ASSERT_FALSE(this->_coder->cipherInfo(client.transport).empty());
	// Передаваемое прикладное сообщение
	const std::string message = "прикладные данные поверх установленного соединения";
	// Шифруем прикладное сообщение на клиенте
	ASSERT_TRUE(this->_coder->encrypt(client.transport, message.data(), message.size()));
	// Передаём шифртекст серверу
	ASSERT_GT(::transfer(* this->_coder, client, server), 0u);
	// Проверяем что сервер принял сообщение без искажений
	ASSERT_EQ(server.incoming, message);
	// Выполняем удаление созданных объектов
	ASSERT_TRUE(this->_coder->destroy(client.transport));
	ASSERT_TRUE(this->_coder->destroy(server.transport));
	ASSERT_TRUE(this->_coder->destroy(client.context));
	ASSERT_TRUE(this->_coder->destroy(server.context));
}

/**
 * @brief Тест согласования протокола приложения при рукопожатии
 *
 * @details Задействует функцию обратного вызова выбора протокола на сервере -
 *          одну из устанавливаемых на шаблон контекста безопасности
 *
 */
TEST_F(TlsFixture, HandshakeAlpnTest){
	// Эндпоинт клиента
	::endpoint_t client;
	// Эндпоинт сервера
	::endpoint_t server;
	// Создаём шаблоны контекста безопасности
	client.context = this->_coder->context(event::node_t::CLIENT, event::protocol_t::TCP);
	server.context = this->_coder->context(event::node_t::SERVER, event::protocol_t::TCP);
	// Проверяем что шаблоны контекста созданы
	ASSERT_NE(client.context, 0u);
	ASSERT_NE(server.context, 0u);
	// Устанавливаем сертификат и приватный ключ сервера
	this->_coder->certificate(server.context, this->_certificate);
	this->_coder->privateKey(server.context, this->_privateKey);
	// Устанавливаем доверенный центр сертификации клиента
	this->_coder->ca(client.context, this->_certificate);
	// Устанавливаем доменное имя удалённого узла на клиенте
	this->_coder->serverNameIndication(client.context, "localhost");
	// Устанавливаем список протоколов приложения клиента
	this->_coder->alpn(client.context, {tls::Coder::alpn_t{0, "h2"}, tls::Coder::alpn_t{0, "http/1.1"}});
	// Устанавливаем список протоколов приложения сервера
	this->_coder->alpn(server.context, {tls::Coder::alpn_t{0, "h2"}});
	/**
	 * Снимаем проверку сертификата на сервере: шаблон контекста создаётся
	 * с включённой проверкой, а на серверном узле это означает требование
	 * клиентского сертификата, то есть взаимную аутентификацию. Тесты
	 * проверяют односторонний TLS, поэтому требование снимается явно
	 */
	this->_coder->validateServerNameIndication(server.context, false);
	// Создаём транспортные уровни
	client.transport = this->_coder->transport(client.context);
	server.transport = this->_coder->transport(server.context);
	// Проверяем что транспортные уровни созданы
	ASSERT_NE(client.transport, 0u);
	ASSERT_NE(server.transport, 0u);
	// Подключаем функции обратного вызова эндпоинтов
	::subscribe(* this->_coder, client);
	::subscribe(* this->_coder, server);
	// Выполняем рукопожатие между эндпоинтами
	ASSERT_TRUE(::establish(* this->_coder, client, server));
	// Проверяем что рукопожатие выполнено на обоих эндпоинтах
	ASSERT_TRUE(client.handshaked);
	ASSERT_TRUE(server.handshaked);
	// Выполняем удаление созданных объектов
	ASSERT_TRUE(this->_coder->destroy(client.transport));
	ASSERT_TRUE(this->_coder->destroy(server.transport));
	ASSERT_TRUE(this->_coder->destroy(client.context));
	ASSERT_TRUE(this->_coder->destroy(server.context));
}

/**
 * @brief Тест отказа рукопожатия при недоверенном сертификате
 *
 * @details Клиент без указанного доверенного центра сертификации не должен
 *          принимать самоподписанный сертификат сервера
 *
 */
TEST_F(TlsFixture, HandshakeUntrustedCertificateTest){
	// Эндпоинт клиента
	::endpoint_t client;
	// Эндпоинт сервера
	::endpoint_t server;
	// Создаём шаблоны контекста безопасности
	client.context = this->_coder->context(event::node_t::CLIENT, event::protocol_t::TCP);
	server.context = this->_coder->context(event::node_t::SERVER, event::protocol_t::TCP);
	// Проверяем что шаблоны контекста созданы
	ASSERT_NE(client.context, 0u);
	ASSERT_NE(server.context, 0u);
	// Устанавливаем сертификат и приватный ключ сервера
	this->_coder->certificate(server.context, this->_certificate);
	this->_coder->privateKey(server.context, this->_privateKey);
	// Устанавливаем доменное имя удалённого узла на клиенте
	this->_coder->serverNameIndication(client.context, "localhost");
	/**
	 * Снимаем проверку сертификата на сервере: шаблон контекста создаётся
	 * с включённой проверкой, а на серверном узле это означает требование
	 * клиентского сертификата, то есть взаимную аутентификацию. Тест проверяет
	 * односторонний TLS, поэтому требование снимается явно
	 */
	this->_coder->validateServerNameIndication(server.context, false);
	/**
	 * Доверенный центр сертификации клиенту намеренно не задаётся: сертификат
	 * сервера самоподписанный и в системном хранилище отсутствует
	 */
	client.transport = this->_coder->transport(client.context);
	// Создаём транспортный уровень сервера
	server.transport = this->_coder->transport(server.context);
	// Проверяем что транспортные уровни созданы
	ASSERT_NE(client.transport, 0u);
	ASSERT_NE(server.transport, 0u);
	// Подключаем функции обратного вызова эндпоинтов
	::subscribe(* this->_coder, client);
	::subscribe(* this->_coder, server);
	// Проверяем что рукопожатие не выполнено
	ASSERT_FALSE(::establish(* this->_coder, client, server));
	// Проверяем что рукопожатие клиента не завершилось успехом
	ASSERT_FALSE(client.handshaked);
	/**
	 * Транспортные уровни после неудачного рукопожатия удалению не подлежат:
	 * кодер помечает их на удаление самостоятельно, поэтому повторный вызов
	 * отвергается. Удаляются только шаблоны контекста
	 */
	ASSERT_TRUE(this->_coder->destroy(client.context));
	ASSERT_TRUE(this->_coder->destroy(server.context));
}

/**
 * @brief Тест рукопожатия со сжатым сертификатом (RFC 8879)
 *
 * @details Прогоняется каждым из трёх методов. Сжатие обязано действительно
 *          случиться: поток сервера со сжатым сертификатом короче, чем без него,
 *          иначе проверка прошла бы и при молча отключённом сжатии
 *
 */
TEST_F(TlsFixture, HandshakeCompressedCertificateTest){
	// Проверяем что сертификат тестового узла сгенерирован
	ASSERT_FALSE(this->_certificate.empty());
	// Количество октетов сервера без сжатия сертификата
	size_t plain = 0;
	{
		// Эндпоинты обмена
		::endpoint_t client, server;
		// Настраиваем контексты
		::contexts(* this->_coder, client, server, this->_certificate, this->_privateKey);
		// Запускаем транспортные уровни
		::transports(* this->_coder, client, server);
		// Выполняем рукопожатие без сжатия
		ASSERT_TRUE(::establish(* this->_coder, client, server));
		// Запоминаем объём потока сервера
		plain = server.sent;
		// Выполняем удаление созданных объектов
		ASSERT_TRUE(this->_coder->destroy(client.transport));
		ASSERT_TRUE(this->_coder->destroy(server.transport));
		ASSERT_TRUE(this->_coder->destroy(client.context));
		ASSERT_TRUE(this->_coder->destroy(server.context));
	}
	/**
	 * Перебираем методы сжатия сертификата
	 */
	for(const auto method : {compressor::method_t::ZLIB, compressor::method_t::BROTLI, compressor::method_t::ZSTD}){
		// Эндпоинты обмена
		::endpoint_t client, server;
		// Настраиваем контексты
		::contexts(* this->_coder, client, server, this->_certificate, this->_privateKey);
		// Разрешаем сжатие сертификата обеим сторонам
		this->_coder->compressors(client.context, {method});
		this->_coder->compressors(server.context, {method});
		// Запускаем транспортные уровни
		::transports(* this->_coder, client, server);
		// Выполняем рукопожатие со сжатым сертификатом
		ASSERT_TRUE(::establish(* this->_coder, client, server)) << "метод " << static_cast <uint32_t> (method);
		// Проверяем что сертификат действительно ушёл сжатым
		ASSERT_LT(server.sent, plain) << "метод " << static_cast <uint32_t> (method) << ": сжатие не случилось";
		// Выполняем удаление созданных объектов
		ASSERT_TRUE(this->_coder->destroy(client.transport));
		ASSERT_TRUE(this->_coder->destroy(server.transport));
		ASSERT_TRUE(this->_coder->destroy(client.context));
		ASSERT_TRUE(this->_coder->destroy(server.context));
	}
}

/**
 * @brief Тест отказа от бомбы сжатия в сертификате (RFC 8879, раздел 4)
 *
 * @details Враждебный сервер объявляет ЧЕСТНЫЙ uncompressed_length, а отдаёт
 *          поток, распаковывающийся в 90 МиБ, - каждым из трёх методов. Клиент обязан отказать в
 *          рукопожатии и при этом не отводить память сверх объявленного размера:
 *          объявленный размер идёт компрессору пределом.
 *
 *          Отказ сам по себе предела НЕ доказывает - его дала бы и сверка после
 *          полной распаковки. Предел доказывает пик памяти процесса: без него
 *          пик вырастает на сотни мегабайт. Пик монотонен, поэтому проверка
 *          ослепнуть может лишь при прежнем пике, уже превышающем нынешний на
 *          размер бомбы, а ложно отказать - нет
 *
 */
TEST_F(TlsFixture, HandshakeCompressedCertificateBombTest){
	/**
	 * Методы сжатия и их номера по RFC 8879: передача предела проверяется
	 * в каждой из трёх веток распаковки отдельно
	 */
	const std::pair <compressor::method_t, uint16_t> methods[] = {
		{compressor::method_t::ZLIB, 0x01},
		{compressor::method_t::BROTLI, 0x02},
		{compressor::method_t::ZSTD, 0x03}
	};
	/**
	 * Перебираем методы сжатия
	 */
	for(const auto & [method, number] : methods){
		// Название метода для сообщений
		const std::string name = std::to_string(static_cast <uint32_t> (method));
		/**
		 * Сверяем формат бомбы с компрессором на малом образце того же сборщика:
		 * иначе отказ пришёл бы от чужого формата, а не от предела
		 */
		{
			// Малый образец
			const std::vector <uint8_t> sample = ::detonator(4096, method);
			// Результат распаковки
			std::vector <uint8_t> result;
			// Выполняем распаковку образца компрессором
			compressor::block_t().decompress(sample.data(), sample.size(), method, result);
			// Проверяем что образец распакован целиком
			ASSERT_EQ(result.size(), 4096u) << "метод " << name << ": формат бомбы не сошёлся с компрессором";
		}
		// Собираем бомбу сжатия
		::bomb = ::detonator(::BOMB, method);
		// Проверяем что бомба собрана
		ASSERT_FALSE(::bomb.empty()) << "метод " << name;
		// Проверяем что бомба пройдёт предел сообщения рукопожатия и дойдёт до распаковки
		ASSERT_LT(::bomb.size() + 1024, static_cast <size_t> (SSL_MAX_CERT_LIST_DEFAULT)) << "метод " << name << ": бомба отвергнется до распаковки, проверка ослепнет";
		// Эндпоинты обмена
		::endpoint_t client, server;
		// Настраиваем контексты
		::contexts(* this->_coder, client, server, this->_certificate, this->_privateKey);
		// Разрешаем клиенту распаковку сертификата проверяемым методом
		this->_coder->compressors(client.context, {method});
		// Устанавливаем враждебному серверу сжатие бомбой под номером метода (RFC 8879)
		ASSERT_EQ(::SSL_CTX_add_cert_compression_alg(this->_coder->native(server.context), number, &::explode, nullptr), 1) << "метод " << name;
		// Запускаем транспортные уровни
		::transports(* this->_coder, client, server);
		// Запоминаем пик памяти до рукопожатия
		const size_t before = ::peak();
		// Проверяем что рукопожатие отвергнуто
		ASSERT_FALSE(::establish(* this->_coder, client, server)) << "метод " << name;
		// Проверяем что клиент рукопожатия не завершил
		ASSERT_FALSE(client.handshaked) << "метод " << name;
		// Получаем пик памяти после рукопожатия
		const size_t after = ::peak();
		// Освобождаем бомбу
		std::vector <uint8_t>().swap(::bomb);
		// Если учёт памяти поддерживается
		if((before > 0) && (after > 0))
			// Проверяем что распаковка не отвела память сверх объявленного размера
			ASSERT_LT(after - before, (64ull << 20)) << "метод " << name << ": пик вырос на " << ((after - before) >> 20) << " МиБ";
		// Выполняем удаление шаблонов контекста
		ASSERT_TRUE(this->_coder->destroy(client.context));
		ASSERT_TRUE(this->_coder->destroy(server.context));
	}
}
