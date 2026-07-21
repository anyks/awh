/**
 * @file: crypto.cpp
 * @date: 2026-07-21
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
 * Стандартные заголовочные файлы
 */
#include <string>
#include <cstring>
#include <cstdint>

/**
 * Подключаем заголовочный файлы проекта
 */
#include "quic.hpp"
#include "../../../include/proto/quic/crypto.hpp"

/**
 * Подписываемся на пространство имён протокола QUIC
 */
using namespace awh::quic;

/**
 * @brief нутренние константы эталонных данных RFC 9001 Appendix A
 *
 */
namespace {
	/**
	 * @brief DCID первого пакета Initial клиента (RFC 9001 §A)
	 *
	 */
	static const char * CLIENT_DCID = "8394c8f03e515708";
	/**
	 * @brief Незащищённый заголовок клиентского пакета Initial (RFC 9001 §A.2)
	 *
	 */
	static const char * CLIENT_INITIAL_HEADER = "c300000001088394c8f03e5157080000449e00000002";
	/**
	 * @brief CRYPTO-фрейм нагрузки клиентского пакета Initial (RFC 9001 §A.2)
	 *
	 */
	static const char * CLIENT_INITIAL_CRYPTO =
		"060040f1010000ed0303ebf8fa56f12939b9584a3896472ec40bb863cfd3e868"
		"04fe3a47f06a2b69484c00000413011302010000c000000010000e00000b6578"
		"616d706c652e636f6dff01000100000a00080006001d00170018001000070005"
		"04616c706e000500050100000000003300260024001d00209370b2c9caa47fba"
		"baf4559fedba753de171fa71f50f1ce15d43e994ec74d748002b000302030400"
		"0d0010000e0403050306030203080408050806002d00020101001c0002400100"
		"3900320408ffffffffffffffff05048000ffff07048000ffff08011001048000"
		"75300901100f088394c8f03e51570806048000ffff";
	/**
	 * @brief Защищённый клиентский пакет Initial целиком (RFC 9001 §A.2)
	 *
	 */
	static const char * CLIENT_INITIAL_PROTECTED =
		"c000000001088394c8f03e5157080000449e7b9aec34d1b1c98dd7689fb8ec11"
		"d242b123dc9bd8bab936b47d92ec356c0bab7df5976d27cd449f63300099f399"
		"1c260ec4c60d17b31f8429157bb35a1282a643a8d2262cad67500cadb8e7378c"
		"8eb7539ec4d4905fed1bee1fc8aafba17c750e2c7ace01e6005f80fcb7df6212"
		"30c83711b39343fa028cea7f7fb5ff89eac2308249a02252155e2347b63d58c5"
		"457afd84d05dfffdb20392844ae812154682e9cf012f9021a6f0be17ddd0c208"
		"4dce25ff9b06cde535d0f920a2db1bf362c23e596d11a4f5a6cf3948838a3aec"
		"4e15daf8500a6ef69ec4e3feb6b1d98e610ac8b7ec3faf6ad760b7bad1db4ba3"
		"485e8a94dc250ae3fdb41ed15fb6a8e5eba0fc3dd60bc8e30c5c4287e53805db"
		"059ae0648db2f64264ed5e39be2e20d82df566da8dd5998ccabdae053060ae6c"
		"7b4378e846d29f37ed7b4ea9ec5d82e7961b7f25a9323851f681d582363aa5f8"
		"9937f5a67258bf63ad6f1a0b1d96dbd4faddfcefc5266ba6611722395c906556"
		"be52afe3f565636ad1b17d508b73d8743eeb524be22b3dcbc2c7468d54119c74"
		"68449a13d8e3b95811a198f3491de3e7fe942b330407abf82a4ed7c1b311663a"
		"c69890f4157015853d91e923037c227a33cdd5ec281ca3f79c44546b9d90ca00"
		"f064c99e3dd97911d39fe9c5d0b23a229a234cb36186c4819e8b9c5927726632"
		"291d6a418211cc2962e20fe47feb3edf330f2c603a9d48c0fcb5699dbfe58964"
		"25c5bac4aee82e57a85aaf4e2513e4f05796b07ba2ee47d80506f8d2c25e50fd"
		"14de71e6c418559302f939b0e1abd576f279c4b2e0feb85c1f28ff18f58891ff"
		"ef132eef2fa09346aee33c28eb130ff28f5b766953334113211996d20011a198"
		"e3fc433f9f2541010ae17c1bf202580f6047472fb36857fe843b19f5984009dd"
		"c324044e847a4f4a0ab34f719595de37252d6235365e9b84392b061085349d73"
		"203a4a13e96f5432ec0fd4a1ee65accdd5e3904df54c1da510b0ff20dcc0c77f"
		"cb2c0e0eb605cb0504db87632cf3d8b4dae6e705769d1de354270123cb11450e"
		"fc60ac47683d7b8d0f811365565fd98c4c8eb936bcab8d069fc33bd801b03ade"
		"a2e1fbc5aa463d08ca19896d2bf59a071b851e6c239052172f296bfb5e724047"
		"90a2181014f3b94a4e97d117b438130368cc39dbb2d198065ae3986547926cd2"
		"162f40a29f0c3c8745c0f50fba3852e566d44575c29d39a03f0cda721984b6f4"
		"40591f355e12d439ff150aab7613499dbd49adabc8676eef023b15b65bfc5ca0"
		"6948109f23f350db82123535eb8a7433bdabcb909271a6ecbcb58b936a88cd4e"
		"8f2e6ff5800175f113253d8fa9ca8885c2f552e657dc603f252e1a8e308f76f0"
		"be79e2fb8f5d5fbbe2e30ecadd220723c8c0aea8078cdfcb3868263ff8f09400"
		"54da48781893a7e49ad5aff4af300cd804a6b6279ab3ff3afb64491c85194aab"
		"760d58a606654f9f4400e8b38591356fbf6425aca26dc85244259ff2b19c41b9"
		"f96f3ca9ec1dde434da7d2d392b905ddf3d1f9af93d1af5950bd493f5aa731b4"
		"056df31bd267b6b90a079831aaf579be0a39013137aac6d404f518cfd4684064"
		"7e78bfe706ca4cf5e9c5453e9f7cfd2b8b4c8d169a44e55c88d4a9a7f9474241"
		"e221af44860018ab0856972e194cd934";
	/**
	 * @brief Незащищённый заголовок серверного пакета Initial (RFC 9001 §A.3)
	 *
	 */
	static const char * SERVER_INITIAL_HEADER = "c1000000010008f067a5502a4262b50040750001";
	/**
	 * @brief Нагрузка серверного пакета Initial (RFC 9001 §A.3)
	 *
	 */
	static const char * SERVER_INITIAL_PAYLOAD =
		"02000000000600405a020000560303eefce7f7b37ba1d1632e96677825ddf739"
		"88cfc79825df566dc5430b9a045a1200130100002e00330024001d00209d3c94"
		"0d89690b84d08a60993c144eca684d1081287c834d5311bcf32bb9da1a002b00"
		"020304";
	/**
	 * @brief Защищённый серверный пакет Initial целиком (RFC 9001 §A.3)
	 *
	 */
	static const char * SERVER_INITIAL_PROTECTED =
		"cf000000010008f067a5502a4262b5004075c0d95a482cd0991cd25b0aac406a"
		"5816b6394100f37a1c69797554780bb38cc5a99f5ede4cf73c3ec2493a1839b3"
		"dbcba3f6ea46c5b7684df3548e7ddeb9c3bf9c73cc3f3bded74b562bfb19fb84"
		"022f8ef4cdd93795d77d06edbb7aaf2f58891850abbdca3d20398c276456cbc4"
		"2158407dd074ee";
	/**
	 * @brief Эталонный пакет Retry целиком (RFC 9001 §A.4)
	 *
	 */
	static const char * RETRY_PACKET = "ff000000010008f067a5502a4262b5746f6b656e04a265ba2eff4d829058fb3f0f2496ba";
};

/**
 * @brief Метод проверки вывода ключей Initial по эталонам RFC 9001 §A.1
 *
 */
TEST_F(QuicFixture, CryptoInitialKeysTest){
	// Ключи защиты пакетов обоих направлений
	crypto::keys_t client, server;
	// Выводим ключи Initial из DCID клиента
	ASSERT_TRUE(crypto::initial(this->makeCid(this->unhex(CLIENT_DCID)), client, server));
	// Проверяем секрет направления клиента
	ASSERT_EQ(this->hex(client.secret), "c00cf151ca5be075ed0ebfb5c80323c42d6b7db67881289af4008f1f6c357aea");
	// Проверяем ключ AEAD клиента
	ASSERT_EQ(this->hex(client.key), "1f369613dd76d5467730efcbe3b1a22d");
	// Проверяем вектор инициализации клиента
	ASSERT_EQ(this->hex(client.iv), "fa044b2f42a3fd3b46fb255c");
	// Проверяем ключ защиты заголовка клиента
	ASSERT_EQ(this->hex(client.hp), "9f50449e04a0e810283a1e9933adedd2");
	// Проверяем секрет направления сервера
	ASSERT_EQ(this->hex(server.secret), "3c199828fd139efd216c155ad844cc81fb82fa8d7446fa7d78be803acdda951b");
	// Проверяем ключ AEAD сервера
	ASSERT_EQ(this->hex(server.key), "cf3a5331653c364c88f0f379b6067e37");
	// Проверяем вектор инициализации сервера
	ASSERT_EQ(this->hex(server.iv), "0ac1493ca1905853b0bba03e");
	// Проверяем ключ защиты заголовка сервера
	ASSERT_EQ(this->hex(server.hp), "c206b8d9b9f0f37644430b490eeaa314");
}

/**
 * @brief Метод проверки защиты клиентского пакета Initial по эталону RFC 9001 §A.2
 *
 */
TEST_F(QuicFixture, CryptoSealClientInitialTest){
	// Ключи защиты пакетов обоих направлений
	crypto::keys_t client, server;
	// Выводим ключи Initial из DCID клиента
	ASSERT_TRUE(crypto::initial(this->makeCid(this->unhex(CLIENT_DCID)), client, server));
	// Формируем незащищённую нагрузку: CRYPTO-фрейм с заполнением PADDING до 1162 октетов
	std::string payload = this->unhex(CLIENT_INITIAL_CRYPTO);
	// Дополняем нагрузку заполнением PADDING
	payload.append(1162 - payload.size(), '\0');
	// Выходной буфер защищённого пакета
	std::string output;
	// Защищаем пакет ключами клиента (номер пакета 2)
	ASSERT_TRUE(crypto::seal(output, client, 2, this->unhex(CLIENT_INITIAL_HEADER), payload));
	// Проверяем совпадение с эталонным защищённым пакетом
	ASSERT_EQ(this->hex(output), CLIENT_INITIAL_PROTECTED);
}

/**
 * @brief Метод проверки снятия защиты клиентского пакета Initial по эталону RFC 9001 §A.2
 *
 */
TEST_F(QuicFixture, CryptoOpenClientInitialTest){
	// Ключи защиты пакетов обоих направлений
	crypto::keys_t client, server;
	// Выводим ключи Initial из DCID клиента
	ASSERT_TRUE(crypto::initial(this->makeCid(this->unhex(CLIENT_DCID)), client, server));
	// Загружаем эталонный защищённый пакет в модифицируемый буфер
	std::string packet = this->unhex(CLIENT_INITIAL_PROTECTED);
	// Разобранный заголовок пакета
	packet::header_t header;
	// Код ошибки транспорта
	error_t error = error_t::NO_ERROR;
	// Разбираем заголовок защищённого пакета
	ASSERT_EQ(packet::parser::header(reinterpret_cast <const uint8_t *> (packet.data()), packet.size(), 0, header, error), status_t::OK);
	// Восстановленный полный номер пакета
	uint64_t pn = 0;
	// Расшифрованная нагрузка пакета
	std::string payload;
	// Снимаем защиту пакета ключами клиента
	ASSERT_EQ(crypto::open(reinterpret_cast <uint8_t *> (&packet[0]), header.size, header.pnOffset, 0, client, pn, payload, error), status_t::OK);
	// Проверяем восстановленный номер пакета
	ASSERT_EQ(pn, 2u);
	// Проверяем размер расшифрованной нагрузки
	ASSERT_EQ(payload.size(), 1162u);
	// Формируем эталонную незащищённую нагрузку
	std::string expected = this->unhex(CLIENT_INITIAL_CRYPTO);
	// Дополняем эталонную нагрузку заполнением PADDING
	expected.append(1162 - expected.size(), '\0');
	// Проверяем совпадение расшифрованной нагрузки с эталонной
	ASSERT_EQ(payload, expected);
	// Проверяем что нагрузка начинается с CRYPTO-фрейма
	frame_t type = frame_t::UNKNOWN;
	// Определяем тип первого фрейма нагрузки
	ASSERT_TRUE(frame::parser::type(reinterpret_cast <const uint8_t *> (payload.data()), payload.size(), type));
	// Проверяем тип первого фрейма
	ASSERT_EQ(type, frame_t::CRYPTO);
}

/**
 * @brief Метод проверки защиты и снятия защиты серверного пакета Initial по эталону RFC 9001 §A.3
 *
 */
TEST_F(QuicFixture, CryptoServerInitialTest){
	// Ключи защиты пакетов обоих направлений
	crypto::keys_t client, server;
	// Выводим ключи Initial из DCID клиента
	ASSERT_TRUE(crypto::initial(this->makeCid(this->unhex(CLIENT_DCID)), client, server));
	// Выходной буфер защищённого пакета
	std::string output;
	// Защищаем пакет ключами сервера (номер пакета 1)
	ASSERT_TRUE(crypto::seal(output, server, 1, this->unhex(SERVER_INITIAL_HEADER), this->unhex(SERVER_INITIAL_PAYLOAD)));
	// Проверяем совпадение с эталонным защищённым пакетом
	ASSERT_EQ(this->hex(output), SERVER_INITIAL_PROTECTED);
	// Разобранный заголовок пакета
	packet::header_t header;
	// Код ошибки транспорта
	error_t error = error_t::NO_ERROR;
	// Разбираем заголовок защищённого пакета
	ASSERT_EQ(packet::parser::header(reinterpret_cast <const uint8_t *> (output.data()), output.size(), 0, header, error), status_t::OK);
	// Восстановленный полный номер пакета
	uint64_t pn = 0;
	// Расшифрованная нагрузка пакета
	std::string payload;
	// Снимаем защиту пакета ключами сервера
	ASSERT_EQ(crypto::open(reinterpret_cast <uint8_t *> (&output[0]), header.size, header.pnOffset, 0, server, pn, payload, error), status_t::OK);
	// Проверяем восстановленный номер пакета
	ASSERT_EQ(pn, 1u);
	// Проверяем совпадение расшифрованной нагрузки с эталонной
	ASSERT_EQ(this->hex(payload), SERVER_INITIAL_PAYLOAD);
}

/**
 * @brief Метод проверки тега целостности пакета Retry по эталону RFC 9001 §A.4
 *
 */
TEST_F(QuicFixture, CryptoRetryTagTest){
	// Исходный DCID первого пакета Initial клиента
	const cid_t odcid = this->makeCid(this->unhex(CLIENT_DCID));
	// Загружаем эталонный пакет Retry
	const std::string packet = this->unhex(RETRY_PACKET);
	// Проверяем тег целостности эталонного пакета
	ASSERT_TRUE(crypto::retryVerify(odcid, packet));
	// Вычисляем тег целостности по пакету без тега
	uint8_t tag[proto::RETRY_TAG_SIZE];
	// Проверяем вычисление тега целостности
	ASSERT_TRUE(crypto::retryTag(odcid, std::string_view(packet).substr(0, packet.size() - proto::RETRY_TAG_SIZE), tag));
	// Проверяем совпадение с эталонным тегом
	ASSERT_EQ(this->hex(std::string(reinterpret_cast <const char *> (tag), proto::RETRY_TAG_SIZE)), "04a265ba2eff4d829058fb3f0f2496ba");
	// Повреждаем один октет токена пакета
	std::string tampered = packet;
	// Инвертируем октет токена
	tampered[16] = static_cast <char> (~tampered[16]);
	// Проверяем что повреждённый пакет отклонён
	ASSERT_FALSE(crypto::retryVerify(odcid, tampered));
	// Проверяем что пакет с чужим ODCID отклонён
	ASSERT_FALSE(crypto::retryVerify(this->makeCid(this->unhex("0102030405060708")), packet));
}

/**
 * @brief Метод проверки защиты короткого пакета ChaCha20-Poly1305 по эталону RFC 9001 §A.5
 *
 */
TEST_F(QuicFixture, CryptoChaChaShortHeaderTest){
	// Ключи защиты пакетов направления сервера
	crypto::keys_t keys;
	// Устанавливаем криптографический набор ChaCha20-Poly1305
	keys.suite = crypto::suite_t::CHACHA20_POLY1305_SHA256;
	// Устанавливаем секрет направления из эталона
	keys.secret = this->unhex("9ac312a7f877468ebe69422748ad00a15443f18203a07d6060f688f30f21632b");
	// Выводим ключи защиты пакетов из секрета
	ASSERT_TRUE(crypto::derive(keys));
	// Проверяем ключ AEAD
	ASSERT_EQ(this->hex(keys.key), "c6d98ff3441c3fe1b2182094f69caa2ed4b716b65488960a7a984979fb23e1c8");
	// Проверяем вектор инициализации
	ASSERT_EQ(this->hex(keys.iv), "e0459b3474bdd0e44a41c144");
	// Проверяем ключ защиты заголовка
	ASSERT_EQ(this->hex(keys.hp), "25a282b9e82f06f21f488917a4fc8f1b73573685608597d0efcb076b0ab7a7a4");
	// Ключи следующей фазы
	crypto::keys_t next;
	// Выполняем обновление ключей
	ASSERT_TRUE(crypto::update(keys, next));
	// Проверяем секрет следующей фазы ("quic ku")
	ASSERT_EQ(this->hex(next.secret), "1223504755036d556342ee9361d253421a826c9ecdf3c7148684b36b714881f9");
	// Проверяем что ключ защиты заголовка не изменился
	ASSERT_EQ(next.hp, keys.hp);
	// Выходной буфер защищённого пакета
	std::string output;
	// Защищаем минимальный пакет с фреймом PING (номер пакета 654360564)
	ASSERT_TRUE(crypto::seal(output, keys, 654360564, this->unhex("4200bff4"), this->unhex("01")));
	// Проверяем совпадение с эталонным защищённым пакетом
	ASSERT_EQ(this->hex(output), "4cfe4189655e5cd55c41f69080575d7999c25a5bfb");
	// Разобранный заголовок пакета
	packet::header_t header;
	// Код ошибки транспорта
	error_t error = error_t::NO_ERROR;
	// Разбираем заголовок защищённого пакета (пустой идентификатор соединения)
	ASSERT_EQ(packet::parser::header(reinterpret_cast <const uint8_t *> (output.data()), output.size(), 0, header, error), status_t::OK);
	// Восстановленный полный номер пакета
	uint64_t pn = 0;
	// Расшифрованная нагрузка пакета
	std::string payload;
	// Снимаем защиту пакета
	ASSERT_EQ(crypto::open(reinterpret_cast <uint8_t *> (&output[0]), header.size, header.pnOffset, 654360563, keys, pn, payload, error), status_t::OK);
	// Проверяем восстановленный номер пакета
	ASSERT_EQ(pn, 654360564u);
	// Проверяем расшифрованную нагрузку (фрейм PING)
	ASSERT_EQ(this->hex(payload), "01");
}

/**
 * @brief Метод проверки отклонения пакета с повреждённой нагрузкой
 *
 */
TEST_F(QuicFixture, CryptoOpenTamperedTest){
	// Ключи защиты пакетов обоих направлений
	crypto::keys_t client, server;
	// Выводим ключи Initial из DCID клиента
	ASSERT_TRUE(crypto::initial(this->makeCid(this->unhex(CLIENT_DCID)), client, server));
	// Загружаем эталонный защищённый пакет
	std::string packet = this->unhex(CLIENT_INITIAL_PROTECTED);
	// Повреждаем октет зашифрованной нагрузки
	packet[100] = static_cast <char> (~packet[100]);
	// Разобранный заголовок пакета
	packet::header_t header;
	// Код ошибки транспорта
	error_t error = error_t::NO_ERROR;
	// Разбираем заголовок повреждённого пакета
	ASSERT_EQ(packet::parser::header(reinterpret_cast <const uint8_t *> (packet.data()), packet.size(), 0, header, error), status_t::OK);
	// Восстановленный полный номер пакета
	uint64_t pn = 0;
	// Расшифрованная нагрузка пакета
	std::string payload;
	// Проверяем что тег AEAD повреждённого пакета не сходится
	ASSERT_EQ(crypto::open(reinterpret_cast <uint8_t *> (&packet[0]), header.size, header.pnOffset, 0, client, pn, payload, error), status_t::ERROR);
	// Проверяем что расшифровка чужими ключами отклонена
	std::string original = this->unhex(CLIENT_INITIAL_PROTECTED);
	// Разбираем заголовок оригинального пакета
	ASSERT_EQ(packet::parser::header(reinterpret_cast <const uint8_t *> (original.data()), original.size(), 0, header, error), status_t::OK);
	// Проверяем что серверные ключи не подходят к клиентскому пакету
	ASSERT_EQ(crypto::open(reinterpret_cast <uint8_t *> (&original[0]), header.size, header.pnOffset, 0, server, pn, payload, error), status_t::ERROR);
}
