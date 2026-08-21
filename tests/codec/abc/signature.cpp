/**
 * @file signature.cpp
 * @date 2026-08-19
 *
 * @license{LicenseRef-AWH-1.0}
 *
 * @author Yuriy Lobarev
 *
 * @brief Проверки подписи владельца бинарного контейнера ABC — дерево свёрток, подпись
 *        всеми четырьмя видами, обнаружение подмены и поверка без ключа расшифровки
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Стандартные заголовочные файлы
 */
#include <vector>
#include <string>
#include <memory>
#include <cstdint>

/**
 * Подключаем заголовочные файлы проекта
 */
#include <gtest/gtest.h>
#include <codec/abc/abc.hpp>

/**
 * Используем стандартное пространство имён
 */
using namespace std;
using namespace awh;
using namespace awh::codec;

/**
 * @brief Пространство имён работ, доступных лишь этому файлу
 *
 */
namespace {
	/**
	 * @brief Класс опоры проверок подписи контейнера
	 *
	 */
	class SignatureFixture : public testing::Test {
		protected:
			// Объект фреймворка
			unique_ptr <fmk_t> _fmk;
			// Объект журнала
			unique_ptr <log_t> _log;
			// Объект сжатия данных
			unique_ptr <compressor::block_t> _compressor;
			// Объект шифрования данных
			unique_ptr <crypto_t> _crypto;
		public:
			/**
			 * @brief Метод заведения опоры проверок
			 *
			 */
			void SetUp() override {
				// Выполняем заведение объекта фреймворка
				this->_fmk = make_unique <fmk_t> ();
				// Выполняем заведение объекта журнала
				this->_log = make_unique <log_t> (this->_fmk.get());
				// Выполняем заведение объекта сжатия данных
				this->_compressor = make_unique <compressor::block_t> (this->_log.get());
				// Выполняем заведение объекта шифрования данных
				this->_crypto = make_unique <crypto_t> (this->_fmk.get(), this->_log.get());
				// Выполняем установку соли шифрования
				this->_crypto->salt("соль контейнера");
				// Выполняем установку пароля шифрования
				this->_crypto->password("пароль владельца");
			}
		public:
			/**
			 * @brief Метод сборки подписанного контейнера
			 *
			 * @param name   имя ключа владельца контейнера
			 * @param buffer буфер, куда следует уложить собранный контейнер
			 * @return       признак успешно собранного контейнера
			 *
			 */
			bool build(const string & name, vector <uint8_t> & buffer) noexcept {
				// Сборщик контейнера
				abc::assembler_t assembler;
				// Выполняем установку модуля сжатия сборщику контейнера
				assembler.compressor(this->_compressor.get());
				// Выполняем объявление подписи собираемого контейнера
				assembler.sign(this->_crypto.get(), name);
				// Получаем настройки сборки контейнера
				abc::assembler_t::settings_t settings = assembler.settings();
				// Выполняем установку порога накопления, дающего несколько кадров
				settings.block = 64;
				// Выполняем установку настроек сборки контейнера
				assembler.settings(settings);
				// Выполняем перебор всех собираемых записей контейнера
				for(size_t i = 0; i < 10; i++){
					// Выполняем сборку очередной записи
					const vector <uint8_t> item = abc::value_t(string{"запись номер "} + to_string(i)).dump();
					// Выполняем внесение очередной записи в собираемый контейнер
					if(!assembler.append(item.data(), item.size(), abc::payload_t::TEXT))
						// Выводим признак неудачной сборки контейнера
						return false;
				}
				// Выводим результат завершения сборки контейнера
				return assembler.complete(buffer);
			}
	};
};

/**
 * @brief Проверка сведения дерева свёрток к корню
 *
 */
TEST_F(SignatureFixture, MerkleRoot) {
	// Дерево свёрток по кадрам контейнера
	abc::merkle_t merkle;
	// Выполняем установку модуля шифрования дереву свёрток
	merkle.crypto(this->_crypto.get());
	// Выполняем проверку того, что пустое дерево к корню не сводится
	vector <uint8_t> root;
	// Выполняем проверку отказа сведения пустого дерева
	ASSERT_FALSE(merkle.root(root));
	// Выполняем внесение трёх кадров в дерево свёрток
	ASSERT_TRUE(merkle.add("первый кадр", 11));
	// Выполняем внесение второго кадра в дерево свёрток
	ASSERT_TRUE(merkle.add("второй кадр", 11));
	// Выполняем внесение третьего кадра в дерево свёрток
	ASSERT_TRUE(merkle.add("третий кадр", 11));
	// Выполняем проверку количества внесённых кадров
	ASSERT_EQ(merkle.leaves(), 3ul);
	// Выполняем сведение дерева свёрток к корню
	ASSERT_TRUE(merkle.root(root));
	// Выполняем проверку длины корня дерева свёрток
	ASSERT_EQ(root.size(), abc::DIGEST_LENGTH);
	// Дерево свёрток с иной чередою тех же кадров
	abc::merkle_t swapped;
	// Выполняем установку модуля шифрования дереву свёрток
	swapped.crypto(this->_crypto.get());
	// Выполняем внесение второго кадра первым
	ASSERT_TRUE(swapped.add("второй кадр", 11));
	// Выполняем внесение первого кадра вторым
	ASSERT_TRUE(swapped.add("первый кадр", 11));
	// Выполняем внесение третьего кадра третьим
	ASSERT_TRUE(swapped.add("третий кадр", 11));
	// Корень дерева с иной чередою кадров
	vector <uint8_t> other;
	// Выполняем сведение дерева свёрток к корню
	ASSERT_TRUE(swapped.root(other));
	/**
	 * Выполняем проверку того, что череда кадров корнем учтена: иначе подмена
	 * кадров местами прошла бы мимо подписи
	 */
	ASSERT_NE(root, other);
}
/**
 * @brief Проверка того, что узел дерева нельзя выдать за лист
 *
 * @details Приставки листа и узла разные намеренно: без них поддельщик, подобрав кадр,
 *          чьи октеты равны паре свёрток, дал бы тот же корень при иной череде кадров
 *
 */
TEST_F(SignatureFixture, LeafIsNotNode) {
	// Дерево свёрток из двух кадров
	abc::merkle_t pair;
	// Выполняем установку модуля шифрования дереву свёрток
	pair.crypto(this->_crypto.get());
	// Выполняем внесение первого кадра в дерево свёрток
	ASSERT_TRUE(pair.add("левая ветвь", 11));
	// Выполняем внесение второго кадра в дерево свёрток
	ASSERT_TRUE(pair.add("правая ветвь", 12));
	// Корень дерева из двух кадров
	vector <uint8_t> root;
	// Выполняем сведение дерева свёрток к корню
	ASSERT_TRUE(pair.root(root));
	// Свёртки кадров, собранные порознь
	abc::merkle_t first, second;
	// Выполняем установку модуля шифрования первому дереву
	first.crypto(this->_crypto.get());
	// Выполняем установку модуля шифрования второму дереву
	second.crypto(this->_crypto.get());
	// Выполняем внесение первого кадра в первое дерево
	ASSERT_TRUE(first.add("левая ветвь", 11));
	// Выполняем внесение второго кадра во второе дерево
	ASSERT_TRUE(second.add("правая ветвь", 12));
	// Корни деревьев, собранных порознь
	vector <uint8_t> left, right;
	// Выполняем сведение первого дерева к корню
	ASSERT_TRUE(first.root(left));
	// Выполняем сведение второго дерева к корню
	ASSERT_TRUE(second.root(right));
	// Собираемый поддельный кадр из пары свёрток
	string forged;
	// Выполняем внесение свёртки левой ветви в поддельный кадр
	forged.append(reinterpret_cast <const char *> (left.data()), left.size());
	// Выполняем внесение свёртки правой ветви в поддельный кадр
	forged.append(reinterpret_cast <const char *> (right.data()), right.size());
	// Дерево свёрток из одного поддельного кадра
	abc::merkle_t single;
	// Выполняем установку модуля шифрования дереву свёрток
	single.crypto(this->_crypto.get());
	// Выполняем внесение поддельного кадра в дерево свёрток
	ASSERT_TRUE(single.add(forged.data(), forged.size()));
	// Корень дерева из поддельного кадра
	vector <uint8_t> counterfeit;
	// Выполняем сведение дерева свёрток к корню
	ASSERT_TRUE(single.root(counterfeit));
	/**
	 * Выполняем проверку того, что кадр из пары свёрток корня узла не даёт:
	 * приставки листа и узла разошлись, и подмена не сходится
	 */
	ASSERT_NE(root, counterfeit);
}
/**
 * @brief Проверка кругового обхода подписи всеми видами ключей
 *
 */
TEST_F(SignatureFixture, SignedRoundtrip) {
	// Виды подписей, какими подписывается контейнер
	const vector <pair <string, crypto_t::signature_t>> kinds = {
		{"ed25519", crypto_t::signature_t::ED25519},
		{"ecdsa", crypto_t::signature_t::ECDSA},
		{"rsa", crypto_t::signature_t::RSA},
		{"gost", crypto_t::signature_t::GOST}
	};
	// Выполняем перебор всех видов подписей
	for(const auto & kind : kinds){
		// Выполняем заведение ключа владельца контейнера
		ASSERT_TRUE(this->_crypto->generateKey(kind.first, kind.second))
			<< "вид подписи: " << kind.first;
		// Буфер собранного контейнера
		vector <uint8_t> buffer;
		// Выполняем сборку подписанного контейнера
		ASSERT_TRUE(this->build(kind.first, buffer)) << "вид подписи: " << kind.first;
		// Код отказа поверки подписи владельца
		abc::error_t error = abc::error_t::NONE;
		// Выполняем поверку подписи владельца контейнера
		ASSERT_TRUE(abc::verify(* this->_crypto, kind.first, buffer.data(), buffer.size(), error))
			<< "вид подписи: " << kind.first << ", код отказа: " << abc::message(error);
		// Заголовок опознания собранного контейнера
		abc::header_t header;
		// Выполняем снятие заголовка опознания контейнера
		ASSERT_TRUE(header.unpack(buffer.data(), buffer.size(), error));
		// Выполняем проверку объявления подписанности контейнера
		ASSERT_TRUE(header.is(abc::flag_t::SIGNED));
		// Выполняем проверку того, что смещение подписи объявлено
		ASSERT_GT(header.signature, 0ull);
		// Буфер усечённого отпечатка открытого ключа владельца
		vector <uint8_t> print;
		// Выполняем выработку усечённого отпечатка открытого ключа
		ASSERT_TRUE(abc::fingerprint(* this->_crypto, kind.first, print));
		// Выполняем проверку длины усечённого отпечатка
		ASSERT_EQ(print.size(), abc::FINGERPRINT_LENGTH);
		/**
		 * Выполняем проверку того, что отпечаток лёг в заголовок: по нему узнают,
		 * которым из ключей владельца подписан контейнер
		 */
		ASSERT_EQ(::memcmp(header.fingerprint, print.data(), print.size()), 0);
	}
}
/**
 * @brief Проверка обнаружения подмены содержимого контейнера
 *
 */
TEST_F(SignatureFixture, TamperRefused) {
	// Выполняем заведение ключа владельца контейнера
	ASSERT_TRUE(this->_crypto->generateKey("владелец", crypto_t::signature_t::ED25519));
	// Буфер собранного контейнера
	vector <uint8_t> buffer;
	// Выполняем сборку подписанного контейнера
	ASSERT_TRUE(this->build("владелец", buffer));
	// Код отказа поверки подписи владельца
	abc::error_t error = abc::error_t::NONE;
	// Выполняем поверку подписи владельца контейнера
	ASSERT_TRUE(abc::verify(* this->_crypto, "владелец", buffer.data(), buffer.size(), error))
		<< "код отказа: " << abc::message(error);
	// Выполняем порчу одного октета тела контейнера
	buffer.at(abc::HEADER_LENGTH + abc::CHUNK_HEADER + 2) ^= 0xFF;
	// Выполняем проверку отказа поверки подписи после порчи тела
	ASSERT_FALSE(abc::verify(* this->_crypto, "владелец", buffer.data(), buffer.size(), error));
	// Выполняем проверку кода отказа поверки подписи
	ASSERT_EQ(error, abc::error_t::REFUSED_SIGNATURE);
	// Выполняем возврат испорченного октета тела контейнера
	buffer.at(abc::HEADER_LENGTH + abc::CHUNK_HEADER + 2) ^= 0xFF;
	// Выполняем проверку того, что возврат октета подпись возвращает
	ASSERT_TRUE(abc::verify(* this->_crypto, "владелец", buffer.data(), buffer.size(), error))
		<< "код отказа: " << abc::message(error);
	// Заголовок опознания собранного контейнера
	abc::header_t header;
	// Выполняем снятие заголовка опознания контейнера
	ASSERT_TRUE(header.unpack(buffer.data(), buffer.size(), error));
	/**
	 * Выполняем порчу одного октета оглавления контейнера: оглавление подписано
	 * наравне с телом, и подмена его обязана быть замечена
	 */
	buffer.at(static_cast <size_t> (header.index) + abc::CHUNK_HEADER) ^= 0xFF;
	// Выполняем проверку отказа поверки подписи после порчи оглавления
	ASSERT_FALSE(abc::verify(* this->_crypto, "владелец", buffer.data(), buffer.size(), error));
	// Выполняем проверку кода отказа поверки подписи
	ASSERT_EQ(error, abc::error_t::REFUSED_SIGNATURE);
}
/**
 * @brief Проверка отказа поверки подписи чужим ключом
 *
 */
TEST_F(SignatureFixture, ForeignKeyRefused) {
	// Выполняем заведение ключа владельца контейнера
	ASSERT_TRUE(this->_crypto->generateKey("владелец", crypto_t::signature_t::ED25519));
	// Выполняем заведение чужого ключа
	ASSERT_TRUE(this->_crypto->generateKey("чужой", crypto_t::signature_t::ED25519));
	// Буфер собранного контейнера
	vector <uint8_t> buffer;
	// Выполняем сборку подписанного контейнера
	ASSERT_TRUE(this->build("владелец", buffer));
	// Код отказа поверки подписи владельца
	abc::error_t error = abc::error_t::NONE;
	// Выполняем проверку отказа поверки подписи чужим ключом
	ASSERT_FALSE(abc::verify(* this->_crypto, "чужой", buffer.data(), buffer.size(), error));
	// Выполняем проверку кода отказа поверки подписи
	ASSERT_EQ(error, abc::error_t::REFUSED_SIGNATURE);
	/**
	 * Выполняем проверку того, что отпечатки ключей разошлись: по отпечатку и
	 * узнают, которым ключом подписан контейнер
	 */
	vector <uint8_t> own, foreign;
	// Выполняем выработку отпечатка ключа владельца
	ASSERT_TRUE(abc::fingerprint(* this->_crypto, "владелец", own));
	// Выполняем выработку отпечатка чужого ключа
	ASSERT_TRUE(abc::fingerprint(* this->_crypto, "чужой", foreign));
	// Выполняем проверку того, что отпечатки разошлись
	ASSERT_NE(own, foreign);
}
/**
 * @brief Проверка поверки подписи без ключа расшифровки
 *
 * @details Подписан шифротекст, оттого поверка идёт по октетам, как они лежат, и
 *          открывать их ради неё незачем
 *
 */
TEST_F(SignatureFixture, VerifyWithoutPassword) {
	// Выполняем заведение ключа владельца контейнера
	ASSERT_TRUE(this->_crypto->generateKey("владелец", crypto_t::signature_t::ED25519));
	// Сборщик контейнера
	abc::assembler_t assembler;
	// Выполняем установку модуля сжатия сборщику контейнера
	assembler.compressor(this->_compressor.get());
	// Выполняем установку модуля шифрования сборщику контейнера
	assembler.crypto(this->_crypto.get());
	// Выполняем объявление подписи собираемого контейнера
	assembler.sign(this->_crypto.get(), "владелец");
	// Получаем настройки укладки кадра
	abc::packer_t::settings_t packing = assembler.packer().settings();
	// Выполняем установку признака шифрования содержимого кадра
	packing.encrypt = true;
	// Выполняем установку настроек укладки кадра
	assembler.packer().settings(packing);
	// Выполняем сборку записи контейнера
	const vector <uint8_t> item = abc::value_t(string{"тайная запись"}).dump();
	// Выполняем внесение записи в собираемый контейнер
	ASSERT_TRUE(assembler.append(item.data(), item.size(), abc::payload_t::TEXT))
		<< "код отказа: " << abc::message(assembler.error());
	// Буфер собранного контейнера
	vector <uint8_t> buffer;
	// Выполняем завершение сборки контейнера
	ASSERT_TRUE(assembler.complete(buffer)) << "код отказа: " << abc::message(assembler.error());
	// Объект шифрования, не знающий пароля контейнера
	crypto_t stranger(this->_fmk.get(), this->_log.get());
	// Выполняем перенесение открытого ключа владельца стороннему объекту
	ASSERT_TRUE(stranger.setKey("владелец", this->_crypto->getKey("владелец", crypto_t::key_type_t::PUBLIC), crypto_t::key_type_t::PUBLIC));
	// Код отказа поверки подписи владельца
	abc::error_t error = abc::error_t::NONE;
	/**
	 * Выполняем поверку подписи владельца без пароля расшифровки: подпись стоит
	 * на шифротексте, и открывать его ради поверки не приходится
	 */
	ASSERT_TRUE(abc::verify(stranger, "владелец", buffer.data(), buffer.size(), error))
		<< "код отказа: " << abc::message(error);
}
/**
 * @brief Проверка отказа поверки неподписанного контейнера
 *
 */
TEST_F(SignatureFixture, UnsignedRefused) {
	// Сборщик контейнера
	abc::assembler_t assembler;
	// Выполняем сборку записи контейнера
	const vector <uint8_t> item = abc::value_t(string{"запись без подписи"}).dump();
	// Выполняем внесение записи в собираемый контейнер
	ASSERT_TRUE(assembler.append(item.data(), item.size(), abc::payload_t::TEXT));
	// Буфер собранного контейнера
	vector <uint8_t> buffer;
	// Выполняем завершение сборки контейнера
	ASSERT_TRUE(assembler.complete(buffer));
	// Код отказа поверки подписи владельца
	abc::error_t error = abc::error_t::NONE;
	// Выполняем проверку отказа поверки неподписанного контейнера
	ASSERT_FALSE(abc::verify(* this->_crypto, "владелец", buffer.data(), buffer.size(), error));
	// Выполняем проверку кода отказа поверки подписи
	ASSERT_EQ(error, abc::error_t::UNSIGNED_CONTAINER);
}
/**
 * @brief Проверка укладки и снятия записи подписи контейнера
 *
 * @details Запись подписи лежит хвостом контейнера и приходит извне: доверять её
 * объявленным длинам нельзя, иначе снятие ушло бы читать за поданные октеты
 *
 */
TEST_F(SignatureFixture, SignRecordRoundtrip) {
	// Укладываемая подпись контейнера
	abc::sign_t sign;
	// Выполняем установку вида подписи владельца контейнера
	sign.kind = crypto_t::signature_t::ECDSA;
	// Выполняем установку вида хэш-суммы, какой подпись выработана
	sign.hash = crypto_t::hash_t::SHA512;
	// Выполняем сборку корня дерева свёрток по кадрам контейнера
	sign.root.assign(abc::DIGEST_LENGTH, 0xA5);
	// Выполняем сборку октетов подписи владельца контейнера
	sign.signature.assign(72, 0x5A);
	// Октеты уложенной записи подписи
	vector <uint8_t> buffer;
	// Выполняем укладку записи подписи контейнера
	abc::pack(sign, buffer);
	// Выполняем проверку длины уложенной записи подписи
	ASSERT_EQ(buffer.size(), abc::SIGNATURE_HEADER + abc::DIGEST_LENGTH + sign.signature.size());
	// Снятая подпись контейнера
	abc::sign_t taken;
	// Код отказа снятия записи подписи
	abc::error_t error = abc::error_t::NONE;
	// Выполняем снятие уложенной записи подписи
	ASSERT_TRUE(abc::unpack(buffer.data(), buffer.size(), taken, error))
		<< "код отказа: " << abc::message(error);
	// Выполняем проверку снятого вида подписи владельца контейнера
	ASSERT_EQ(taken.kind, sign.kind);
	// Выполняем проверку снятого вида хэш-суммы
	ASSERT_EQ(taken.hash, sign.hash);
	// Выполняем проверку снятого корня дерева свёрток
	ASSERT_EQ(taken.root, sign.root);
	// Выполняем проверку снятых октетов подписи владельца
	ASSERT_EQ(taken.signature, sign.signature);
	/**
	 * Выполняем проверку того, что запись подписи самоограничена: длины объявлены
	 * заголовком её, и приписанный хвост снятию не мешает
	 */
	vector <uint8_t> tailed = buffer;
	// Выполняем приписывание хвоста к записи подписи
	tailed.insert(tailed.end(), 16, 0xFF);
	// Выполняем снятие записи подписи с приписанным хвостом
	ASSERT_TRUE(abc::unpack(tailed.data(), tailed.size(), taken, error))
		<< "код отказа: " << abc::message(error);
	// Выполняем проверку снятых октетов подписи владельца
	ASSERT_EQ(taken.signature, sign.signature);
	// Выполняем проверку отказа снятия несуществующих октетов
	ASSERT_FALSE(abc::unpack(nullptr, buffer.size(), taken, error));
	// Выполняем проверку кода внутреннего отказа
	ASSERT_EQ(error, abc::error_t::INTERNAL);
	// Выполняем проверку отказа снятия записи короче заголовка её
	ASSERT_FALSE(abc::unpack(buffer.data(), abc::SIGNATURE_HEADER - 1, taken, error));
	// Выполняем проверку кода отказа обрыва записи подписи
	ASSERT_EQ(error, abc::error_t::TRUNCATED_SIGNATURE);
	// Выполняем проверку отказа снятия записи, оборванной посреди подписи
	ASSERT_FALSE(abc::unpack(buffer.data(), buffer.size() - 1, taken, error));
	// Выполняем проверку кода отказа обрыва записи подписи
	ASSERT_EQ(error, abc::error_t::TRUNCATED_SIGNATURE);
	/**
	 * Выполняем проверку того, что снятие поверяет длину корня дерева свёрток:
	 * длина эта закреплена договором, и объявленная иною означает порчу
	 */
	vector <uint8_t> damaged = buffer;
	// Выполняем порчу объявленной длины корня дерева свёрток
	damaged.at(4) = static_cast <uint8_t> (abc::DIGEST_LENGTH - 1);
	// Выполняем проверку отказа снятия повреждённой записи подписи
	ASSERT_FALSE(abc::unpack(damaged.data(), damaged.size(), taken, error));
	// Выполняем проверку кода отказа повреждённой записи подписи
	ASSERT_EQ(error, abc::error_t::INVALID_SIGNATURE);
	// Выполняем восстановление объявленной длины корня дерева свёрток
	damaged.at(4) = static_cast <uint8_t> (abc::DIGEST_LENGTH);
	// Выполняем порчу объявленной длины подписи владельца контейнера
	damaged.at(2) = 0;
	// Выполняем порчу старшего октета объявленной длины подписи владельца
	damaged.at(3) = 0;
	// Выполняем проверку отказа снятия записи с пустой подписью
	ASSERT_FALSE(abc::unpack(damaged.data(), damaged.size(), taken, error));
	// Выполняем проверку кода отказа повреждённой записи подписи
	ASSERT_EQ(error, abc::error_t::INVALID_SIGNATURE);
}
/**
 * @brief Проверка подбора вида хэш-суммы под вид подписи
 *
 * @details Вид хэш-суммы ставится подбором, а не потребителем напрямую: поданный не
 * тому виду ключа обратился бы в отказ подписи посреди фиксации
 *
 */
TEST_F(SignatureFixture, DigestSelection) {
	/**
	 * Выполняем проверку того, что у Ed25519 хэш-суммы нет вовсе: желаемая
	 * поданная отвергается, а не принимается
	 */
	ASSERT_EQ(abc::digest(crypto_t::signature_t::ED25519, crypto_t::hash_t::SHA512), crypto_t::hash_t::NONE);
	// Выполняем проверку того, что у подписи по ГОСТ хэш-сумма предписана схемой
	ASSERT_EQ(abc::digest(crypto_t::signature_t::GOST, crypto_t::hash_t::SHA256), crypto_t::hash_t::NONE);
	// Выполняем проверку того, что у RSA поданный вид хэш-суммы принимается
	ASSERT_EQ(abc::digest(crypto_t::signature_t::RSA, crypto_t::hash_t::SHA512), crypto_t::hash_t::SHA512);
	// Выполняем проверку того, что у ECDSA поданный вид хэш-суммы принимается
	ASSERT_EQ(abc::digest(crypto_t::signature_t::ECDSA, crypto_t::hash_t::SHA384), crypto_t::hash_t::SHA384);
	/**
	 * Выполняем проверку вида хэш-суммы по умолчанию: у RSA и ECDSA она
	 * обязательна, и отсутствие поданной отказом отвечать незачем
	 */
	ASSERT_EQ(abc::digest(crypto_t::signature_t::RSA, crypto_t::hash_t::NONE), crypto_t::hash_t::SHA256);
	// Выполняем проверку вида хэш-суммы по умолчанию у ECDSA
	ASSERT_EQ(abc::digest(crypto_t::signature_t::ECDSA, crypto_t::hash_t::NONE), crypto_t::hash_t::SHA256);
	// Выполняем проверку того, что отсутствие вида подписи хэш-суммы не даёт
	ASSERT_EQ(abc::digest(crypto_t::signature_t::NONE, crypto_t::hash_t::SHA256), crypto_t::hash_t::NONE);
}
/**
 * @brief Проверка очистки дерева свёрток по кадрам контейнера
 *
 * @details Дерево живёт у сборщика и у правщика, и уборка перестраивает контейнер
 * наново: неочищенное дерево свело бы к корню кадры обоих контейнеров разом
 *
 */
TEST_F(SignatureFixture, MerkleClear) {
	// Дерево свёрток по кадрам контейнера
	abc::merkle_t merkle;
	// Выполняем установку модуля шифрования дереву свёрток
	merkle.crypto(this->_crypto.get());
	// Выполняем внесение двух кадров в дерево свёрток
	ASSERT_TRUE(merkle.add("первый кадр", 11));
	// Выполняем внесение второго кадра в дерево свёрток
	ASSERT_TRUE(merkle.add("второй кадр", 11));
	// Корень дерева свёрток по кадрам контейнера
	vector <uint8_t> first;
	// Выполняем сведение дерева свёрток к корню
	ASSERT_TRUE(merkle.root(first));
	// Выполняем очистку дерева свёрток по кадрам контейнера
	merkle.clear();
	// Выполняем проверку того, что кадров в дереве не осталось
	ASSERT_EQ(merkle.leaves(), 0ul);
	// Корень очищенного дерева свёрток
	vector <uint8_t> empty;
	// Выполняем проверку того, что очищенное дерево к корню не сводится
	ASSERT_FALSE(merkle.root(empty));
	// Выполняем внесение тех же двух кадров в очищенное дерево свёрток
	ASSERT_TRUE(merkle.add("первый кадр", 11));
	// Выполняем внесение второго кадра в очищенное дерево свёрток
	ASSERT_TRUE(merkle.add("второй кадр", 11));
	// Корень дерева свёрток, собранного наново
	vector <uint8_t> second;
	// Выполняем сведение дерева свёрток к корню
	ASSERT_TRUE(merkle.root(second));
	/**
	 * Выполняем проверку того, что очистка кадры ЗАБЫЛА, а не отбросила счёт:
	 * уцелевшие свёртки дали бы дереву вчетверо больше кадров и иной корень
	 */
	ASSERT_EQ(second, first);
}
