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
				abc::assembler_t assembler(this->_log.get());
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
	abc::merkle_t merkle(this->_log.get());
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
	abc::merkle_t swapped(this->_log.get());
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
	abc::merkle_t pair(this->_log.get());
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
	abc::merkle_t first(this->_log.get()), second(this->_log.get());
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
	abc::merkle_t single(this->_log.get());
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
		ASSERT_TRUE(abc::verify(* this->_crypto, kind.first, buffer.data(), buffer.size(), error, this->_log.get()))
			<< "вид подписи: " << kind.first << ", код отказа: " << abc::message(error);
		// Заголовок опознания собранного контейнера
		abc::header_t header;
		// Выполняем снятие заголовка опознания контейнера
		ASSERT_TRUE(header.unpack(buffer.data(), buffer.size(), error));
		// Выполняем проверку объявления подписанности контейнера
		ASSERT_TRUE(header.is(abc::flag_t::SIGNED));
		// Выполняем проверку того, что смещение подписи объявлено
		/**
		 * Место подписи сличается с ДЛИНОЙ ЗАГОЛОВКА, а не с нулём: ноль годится
		 * признаком отсутствия по доводу о длине заголовка, и поверяется довод
		 */
		ASSERT_GE(header.signature, static_cast <uint64_t> (abc::HEADER_LENGTH));
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
	ASSERT_TRUE(abc::verify(* this->_crypto, "владелец", buffer.data(), buffer.size(), error, this->_log.get()))
		<< "код отказа: " << abc::message(error);
	// Выполняем порчу одного октета тела контейнера
	buffer.at(abc::HEADER_LENGTH + abc::CHUNK_HEADER + 2) ^= 0xFF;
	// Выполняем проверку отказа поверки подписи после порчи тела
	ASSERT_FALSE(abc::verify(* this->_crypto, "владелец", buffer.data(), buffer.size(), error, this->_log.get()));
	// Выполняем проверку кода отказа поверки подписи
	ASSERT_EQ(error, abc::error_t::REFUSED_SIGNATURE);
	// Выполняем возврат испорченного октета тела контейнера
	buffer.at(abc::HEADER_LENGTH + abc::CHUNK_HEADER + 2) ^= 0xFF;
	// Выполняем проверку того, что возврат октета подпись возвращает
	ASSERT_TRUE(abc::verify(* this->_crypto, "владелец", buffer.data(), buffer.size(), error, this->_log.get()))
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
	ASSERT_FALSE(abc::verify(* this->_crypto, "владелец", buffer.data(), buffer.size(), error, this->_log.get()));
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
	ASSERT_FALSE(abc::verify(* this->_crypto, "чужой", buffer.data(), buffer.size(), error, this->_log.get()));
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
	abc::assembler_t assembler(this->_log.get());
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
	ASSERT_TRUE(abc::verify(stranger, "владелец", buffer.data(), buffer.size(), error, this->_log.get()))
		<< "код отказа: " << abc::message(error);
}
/**
 * @brief Проверка отказа поверки неподписанного контейнера
 *
 */
TEST_F(SignatureFixture, UnsignedRefused) {
	// Сборщик контейнера
	abc::assembler_t assembler(this->_log.get());
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
	ASSERT_FALSE(abc::verify(* this->_crypto, "владелец", buffer.data(), buffer.size(), error, this->_log.get()));
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
	// Выполняем проверку того, что запись подписи уложена
	ASSERT_TRUE(abc::pack(sign, buffer));
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
	abc::merkle_t merkle(this->_log.get());
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

/**
 * @brief Проверка стойкости дерева свёрток к подлогу дублированием
 *
 * @details Известнейший подлог у деревьев свёрток: ярус с нечётным числом свёрток сводят,
 *          ДУБЛИРУЯ последнюю, и тогда набор из трёх кадров даёт тот же корень, что и
 *          набор из четырёх, где четвёртый повторяет третий. Подпись, стоящая под первым
 *          набором, оказывается годна и для второго
 *
 * @note Наше дерево нечётную свёртку ПОДНИМАЕТ на ярус выше, а не дублирует, и подлог
 *       этот ему не грозит. Проверка закрепляет именно выбор: замена поднятия на
 *       дублирование - правка в одну строку, и без сторожа она пройдёт незамеченной,
 *       ибо все прочие проверки подписи от неё не изменятся
 *
 */
TEST_F(SignatureFixture, MerkleDuplicationRefused) {
	/**
	 * @brief Функция сведения дерева по заданному набору кадров
	 *
	 * @param frames набор кадров дерева
	 * @param crypto модуль шифрования
	 * @return       корень сведённого дерева
	 *
	 */
	const auto reduce = [this](const vector <string> & frames, crypto_t * crypto) noexcept -> vector <uint8_t> {
		// Дерево свёрток по кадрам контейнера
		abc::merkle_t merkle(this->_log.get());
		// Выполняем установку модуля шифрования дереву свёрток
		merkle.crypto(crypto);
		/**
		 * Выполняем перебор всех кадров набора
		 */
		for(const string & frame : frames){
			// Если внести очередной кадр в дерево не удалось
			if(!merkle.add(frame.data(), frame.size()))
				// Выводим пустой корень дерева
				return vector <uint8_t> ();
		}
		// Корень сведённого дерева свёрток
		vector <uint8_t> result;
		// Если свести дерево к корню не удалось
		if(!merkle.root(result))
			// Выводим пустой корень дерева
			return vector <uint8_t> ();
		// Выводим корень сведённого дерева
		return result;
	};
	// Корень дерева по трём кадрам
	const vector <uint8_t> three = reduce({"первый", "второй", "третий"}, this->_crypto.get());
	// Выполняем проверку того, что дерево по трём кадрам сведено
	ASSERT_EQ(three.size(), abc::DIGEST_LENGTH);
	/**
	 * Корень дерева по четырём кадрам, где четвёртый повторяет третий.
	 *
	 * Дерево, дублирующее нечётную свёртку, дало бы здесь ТОТ ЖЕ корень, и подпись под
	 * набором из трёх кадров годилась бы для набора из четырёх
	 */
	const vector <uint8_t> four = reduce({"первый", "второй", "третий", "третий"}, this->_crypto.get());
	// Выполняем проверку того, что дерево по четырём кадрам сведено
	ASSERT_EQ(four.size(), abc::DIGEST_LENGTH);
	// Выполняем проверку расхождения корней обоих деревьев
	ASSERT_NE(three, four);
	/**
	 * Выполняем проверку того же подлога на ярусе выше: пять кадров против шести, где
	 * шестой повторяет пятый
	 */
	const vector <uint8_t> five = reduce({"а", "б", "в", "г", "д"}, this->_crypto.get());
	// Выполняем проверку того, что дерево по пяти кадрам сведено
	ASSERT_EQ(five.size(), abc::DIGEST_LENGTH);
	// Корень дерева по шести кадрам с повтором последнего
	const vector <uint8_t> six = reduce({"а", "б", "в", "г", "д", "д"}, this->_crypto.get());
	// Выполняем проверку того, что дерево по шести кадрам сведено
	ASSERT_EQ(six.size(), abc::DIGEST_LENGTH);
	// Выполняем проверку расхождения корней обоих деревьев
	ASSERT_NE(five, six);
}
/**
 * @brief Проверка отказа укладки подписи, не умещающейся в запись
 *
 * @details Длина подписи и длина корня кладутся двумя октетами всякая. Укладка
 *          обрезала их МОЛЧА: подпись о 70 000 октетах давала запись, снятие какой
 *          отвечало успехом и выдавало 4 464 октета - число иное, а с виду целое.
 *          Повод такой подписи искали бы где угодно, только не в укладке
 *
 * @note Предел этот не тесен: подпись RSA-16384 занимает 2048 октетов, а самая
 *       широкая у схем послеквантовых - около 50 000
 *
 */
TEST_F(SignatureFixture, WideSignatureRefused) {
	// Собираемая подпись владельца контейнера
	abc::sign_t sign;
	// Выполняем установку вида подписи владельца контейнера
	sign.kind = crypto_t::signature_t::ED25519;
	// Выполняем установку вида хэш-суммы, какой подпись выработана
	sign.hash = crypto_t::hash_t::NONE;
	// Выполняем установку корня дерева свёрток
	sign.root.assign(abc::DIGEST_LENGTH, 0xAB);
	/**
	 * Выполняем проверку того, что подпись обычной ширины укладывается и снимается
	 */
	{
		// Выполняем установку подписи обычной ширины
		sign.signature.assign(2048, 0xCD);
		// Буфер уложенной записи подписи
		vector <uint8_t> record;
		// Выполняем проверку того, что запись подписи уложена
		ASSERT_TRUE(abc::pack(sign, record));
		// Выполняем проверку того, что уложенная запись подписи не пуста
		ASSERT_FALSE(record.empty());
		// Снятая подпись владельца контейнера
		abc::sign_t taken;
		// Код отказа снятия подписи
		abc::error_t error = abc::error_t::NONE;
		// Выполняем снятие записи подписи владельца контейнера
		ASSERT_TRUE(abc::unpack(record.data(), record.size(), taken, error))
			<< "код отказа: " << abc::message(error);
		// Выполняем проверку того, что снятая подпись сошлась с уложенной
		ASSERT_EQ(taken.signature, sign.signature);
		// Выполняем проверку того, что снятый корень сошёлся с уложенным
		ASSERT_EQ(taken.root, sign.root);
	}
	/**
	 * Выполняем проверку того, что подпись шире шестнадцати разрядов не укладывается
	 */
	{
		// Выполняем установку подписи, не умещающейся в две октеты длины
		sign.signature.assign(70000, 0xCD);
		// Буфер уложенной записи подписи
		vector <uint8_t> record;
		/**
		 * Выполняем проверку того, что укладка отвечает ОТКАЗОМ.
		 *
		 * Прежде отказ объявлялся одною пустотою записи, и проверка эта пустоту же и
		 * закрепляла - то есть стерегла СЛЕДСТВИЕ отказа, не спросив, как поступит с
		 * пустой записью зовущий. Поступал он с нею скверно: заворачивал в кадр и клал
		 * на носитель. Держится это ныне `OversizedRecordIsRefusedOutright`
		 */
		ASSERT_FALSE(abc::pack(sign, record));
		/**
		 * Выполняем проверку того, что запись подписи осталась ПУСТОЙ.
		 *
		 * Пустую запись снятие отвергает заведомо, а обрезанная снималась бы успехом
		 */
		ASSERT_TRUE(record.empty()) << "уложено октетов: " << record.size();
	}
	/**
	 * Выполняем проверку того, что корень шире шестнадцати разрядов не укладывается
	 */
	{
		// Выполняем установку подписи обычной ширины
		sign.signature.assign(2048, 0xCD);
		// Выполняем установку корня, не умещающегося в две октеты длины
		sign.root.assign(70000, 0xAB);
		// Буфер уложенной записи подписи
		vector <uint8_t> record;
		// Выполняем проверку того, что запись подписи отвергнута
		ASSERT_FALSE(abc::pack(sign, record));
		// Выполняем проверку того, что запись подписи осталась пустой
		ASSERT_TRUE(record.empty()) << "уложено октетов: " << record.size();
	}
}
/**
 * @brief Проверка подписания контейнера видом хэш-суммы, отличным от умолчания
 *
 * @details Работа объявления подписи принимает вид хэш-суммы третьим доводом, и вид
 *          этот доходит до самой подписи через приведение его к виду подписи. Схема
 *          ECDSA поданный вид принимает, оттого на ней довод и поверяется насквозь -
 *          от подписания до поверки чужим объектом шифрования
 *
 * @note Проверка заведена находкой 31.08.2026: все двенадцать вызовов `sign` у набора
 *       были ДВУХДОВОДНЫМИ, то есть третий довод открытой работы не испытывался ни разу.
 *       Приведение вида свёртки к виду подписи (`abc::digest`) при этом сличено семью
 *       проверками, но сличено ОТДЕЛЬНО от подписания: работа приведения поверялась,
 *       а путь от довода до подписи - нет
 *
 * @note Дерево свёрток вида хэш-суммы НЕ ведает: листы его вырабатываются постоянной
 *       свёрткой в тридцать два октета, и довод этот правит одной лишь подписью корня.
 *       Оттого проверка и сличает поверку целиком, а не длину свёртки
 */
TEST_F(SignatureFixture, SignWithNonDefaultHash) {
	// Выполняем заведение ключа владельца контейнера
	ASSERT_TRUE(this->_crypto->generateKey("владелец", crypto_t::signature_t::ECDSA));
	// Сборщик контейнера
	abc::assembler_t assembler(this->_log.get());
	// Выполняем установку модуля сжатия сборщику контейнера
	assembler.compressor(this->_compressor.get());
	// Выполняем установку модуля шифрования сборщику контейнера
	assembler.crypto(this->_crypto.get());
	// Выполняем объявление подписи собираемого контейнера видом свёртки SHA384
	assembler.sign(this->_crypto.get(), "владелец", crypto_t::hash_t::SHA384);
	// Выполняем сборку записи контейнера
	const vector <uint8_t> item = abc::value_t(string{"запись под иной свёрткой"}).dump();
	// Выполняем внесение записи в собираемый контейнер
	ASSERT_TRUE(assembler.append(item.data(), item.size(), abc::payload_t::TEXT))
		<< "код отказа: " << abc::message(assembler.error());
	// Буфер собранного контейнера
	vector <uint8_t> buffer;
	// Выполняем завершение сборки контейнера
	ASSERT_TRUE(assembler.complete(buffer)) << "код отказа: " << abc::message(assembler.error());
	// Объект шифрования, знающий один лишь открытый ключ владельца
	crypto_t stranger(this->_fmk.get(), this->_log.get());
	// Выполняем перенесение открытого ключа владельца стороннему объекту
	ASSERT_TRUE(stranger.setKey("владелец", this->_crypto->getKey("владелец", crypto_t::key_type_t::PUBLIC), crypto_t::key_type_t::PUBLIC));
	// Код отказа поверки подписи владельца
	abc::error_t error = abc::error_t::NONE;
	/**
	 * Выполняем сличение вида хэш-суммы, ЗАПИСАННОГО в контейнер
	 *
	 * @note Поверки подписи для того НЕ ДОСТАЁТ: она читает вид свёртки из самого
	 *       контейнера и потому сходится при любом виде, включая умолчание. Замерено
	 *       щупом 31.08.2026 - проверка, поверкой и оканчивавшаяся, проходила и тогда,
	 *       когда третий довод `sign` отбрасывался целиком
	 */
	{
		// Заголовок опознания собранного контейнера
		abc::header_t header;
		// Выполняем снятие заголовка опознания контейнера
		ASSERT_TRUE(header.unpack(buffer.data(), buffer.size(), error))
			<< "код отказа: " << abc::message(error);
		// Снятые сведения о подписи контейнера
		abc::sign_t taken;
		// Смещение самой записи подписи: она завёрнута в кадр, и начало её за его оголовком
		const size_t start = static_cast <size_t> (header.signature) + abc::CHUNK_HEADER;
		// Выполняем снятие записи подписи, лежащей за оголовком кадра подписи
		ASSERT_TRUE(abc::unpack(buffer.data() + start, buffer.size() - start, taken, error))
			<< "код отказа: " << abc::message(error);
		// Вид подписи владельца обязан отвечать заведённому ключу
		ASSERT_EQ(taken.kind, crypto_t::signature_t::ECDSA);
		// Вид хэш-суммы обязан отвечать ПОДАННОМУ, а не умолчанию
		ASSERT_EQ(taken.hash, crypto_t::hash_t::SHA384);
	}
	// Поверка подписи обязана сойтись при виде свёртки, отличном от умолчания
	ASSERT_TRUE(abc::verify(stranger, "владелец", buffer.data(), buffer.size(), error, this->_log.get()))
		<< "код отказа: " << abc::message(error);
}
/**
 * @brief Проверка соответствия корня дерева свёрток объявленному укладу
 *
 * @details Уклад свёрток по октетам объявлен у самого `Merkle`: приставка первым октетом,
 *          `0x00` у листа и `0x01` у узла, лист есть `SHA256(0x00 || октеты кадра)`, узел -
 *          `SHA256(0x01 || левая || правая)`, разряд мусора перед выработкой снимается.
 *          Проверка эта складывает корень ПО ЗАПИСИ, зовом хэша напрямую, и сличает с тем,
 *          что выдаёт дерево
 *
 * @note Опора здесь ВНЕ поверяемого: складывается корень не вторым потребителем дерева, а
 *       по объявленному правилу, и расхождение означает, что запись и работа разошлись.
 *       Прочие проверки дерева сличают корень с корнем и такого расхождения не видят вовсе
 *
 * @note Кадров взято ПЯТЬ намеренно: нечётность поднимает свёртку яруса без пары, и уклад
 *       поверяется вместе с этим правилом. У одного из кадров разряд мусора выставлен -
 *       снятие его тем и закрепляется
 *
 */
TEST_F(SignatureFixture, MerkleRootFollowsRecipe) {
	// Кадры, вносимые в дерево свёрток
	vector <vector <uint8_t>> chunks;
	/**
	 * Выполняем сборку пяти кадров различимого содержимого
	 */
	for(uint8_t i = 0; i < 5; i++){
		// Собираемый кадр контейнера
		vector <uint8_t> chunk(48, static_cast <uint8_t> (0x40 + i));
		// Выполняем внесение собранного кадра
		chunks.push_back(::std::move(chunk));
	}
	// Выполняем пометку третьего кадра мусором
	chunks.at(2).at(abc::CHUNK_FLAGS) = static_cast <uint8_t> (chunks.at(2).at(abc::CHUNK_FLAGS) | abc::CHUNK_WASTE);
	// Дерево свёрток по кадрам контейнера
	abc::merkle_t merkle(this->_log.get());
	// Выполняем установку модуля шифрования дереву свёрток
	merkle.crypto(this->_crypto.get());
	/**
	 * Выполняем внесение собранных кадров в дерево свёрток
	 */
	for(auto & chunk : chunks)
		// Выполняем внесение кадра в дерево свёрток
		ASSERT_TRUE(merkle.add(chunk.data(), chunk.size()));
	// Корень, выданный деревом свёрток
	vector <uint8_t> root;
	// Выполняем сведение дерева свёрток к корню
	ASSERT_TRUE(merkle.root(root));
	// Свёртки нынешнего яруса, складываемые по объявленному укладу
	vector <vector <uint8_t>> tier;
	/**
	 * Выполняем выработку свёрток листьев по объявленному укладу
	 */
	for(auto & chunk : chunks){
		// Буфер, подаваемый хэшу: приставка листа впереди октетов кадра
		vector <uint8_t> payload;
		// Выполняем внесение приставки свёртки листа
		payload.push_back(0x00);
		// Выполняем внесение октетов кадра
		payload.insert(payload.end(), chunk.begin(), chunk.end());
		// Выполняем снятие разряда мусора с признаков кадра
		payload.at(1 + abc::CHUNK_FLAGS) = static_cast <uint8_t> (payload.at(1 + abc::CHUNK_FLAGS) & ~abc::CHUNK_WASTE);
		// Выполняем выработку свёртки листа дерева
		tier.push_back(this->_crypto->hash <vector <uint8_t>, vector <uint8_t>> (
		 payload, crypto_t::hash_t::SHA256, crypto_t::format_t::RAW));
		// Выполняем проверку длины выработанной свёртки листа
		ASSERT_EQ(tier.back().size(), abc::DIGEST_LENGTH);
	}
	/**
	 * Выполняем сведение сложенных свёрток к корню по объявленному укладу
	 */
	while(tier.size() > 1){
		// Свёртки следующего яруса дерева
		vector <vector <uint8_t>> next;
		/**
		 * Выполняем перебор свёрток нынешнего яруса парами
		 */
		for(size_t i = 0; i < tier.size(); i += 2){
			/**
			 * Если пары свёртке не досталось, поднимаем её ярусом выше как есть
			 */
			if((i + 1) >= tier.size()){
				// Выполняем поднятие свёртки на ярус выше
				next.push_back(tier.at(i));
				// Прекращаем перебор свёрток нынешнего яруса
				break;
			}
			// Буфер, подаваемый хэшу: приставка узла впереди пары свёрток
			vector <uint8_t> payload;
			// Выполняем внесение приставки свёртки узла
			payload.push_back(0x01);
			// Выполняем внесение свёртки левой ветви узла
			payload.insert(payload.end(), tier.at(i).begin(), tier.at(i).end());
			// Выполняем внесение свёртки правой ветви узла
			payload.insert(payload.end(), tier.at(i + 1).begin(), tier.at(i + 1).end());
			// Выполняем выработку свёртки узла дерева
			next.push_back(this->_crypto->hash <vector <uint8_t>, vector <uint8_t>> (
			 payload, crypto_t::hash_t::SHA256, crypto_t::format_t::RAW));
		}
		// Выполняем переход к следующему ярусу дерева
		tier = ::std::move(next);
	}
	// Выполняем проверку совпадения корня дерева с корнем, сложенным по записи
	ASSERT_EQ(root, tier.front());
}
/**
 * @brief Проверка соответствия записи подписи объявленной раскладке
 *
 * @details Раскладка записи подписи объявлена у `SIGNATURE_HEADER`: вид подписи, вид
 *          хэша, длина подписи, длина корня и два октета впрок, а за заголовком СНАЧАЛА
 *          корень и лишь ЗА НИМ октеты подписи. Череда эта обратна череде длин, и
 *          сторожить её надобно отдельно: круговой ход укладки со снятием перестановку
 *          переживает, покуда обе стороны переставлены разом
 *
 * @note Октеты впрок поверяются здесь ОБЕИМИ сторонами: укладка обязана класть их
 *       нулями, а снятие - отвергать занятые. Второе равняется на то же требование у
 *       строки оглавления
 *
 */
TEST_F(SignatureFixture, RecordLayout) {
	// Укладываемая подпись контейнера
	abc::sign_t sign;
	// Выполняем установку вида подписи владельца контейнера
	sign.kind = crypto_t::signature_t::ECDSA;
	// Выполняем установку вида хэш-суммы, какой подпись выработана
	sign.hash = crypto_t::hash_t::SHA384;
	// Выполняем сборку корня дерева свёрток различимого содержимого
	sign.root.assign(abc::DIGEST_LENGTH, 0xA5);
	// Выполняем сборку октетов подписи владельца различимого содержимого
	sign.signature.assign(300, 0x5A);
	// Буфер уложенной записи подписи
	vector <uint8_t> buffer;
	// Выполняем проверку того, что запись подписи уложена
	ASSERT_TRUE(abc::pack(sign, buffer));
	// Выполняем проверку длины уложенной записи подписи
	ASSERT_EQ(buffer.size(), abc::SIGNATURE_HEADER + sign.root.size() + sign.signature.size());
	// Вид подписи владельца обязан стоять первым октетом
	ASSERT_EQ(buffer.at(0), static_cast <uint8_t> (crypto_t::signature_t::ECDSA));
	// Вид хэш-суммы обязан стоять вторым октетом
	ASSERT_EQ(buffer.at(1), static_cast <uint8_t> (crypto_t::hash_t::SHA384));
	// Длина подписи обязана стоять на смещении два, от младшего октета к старшему
	ASSERT_EQ(static_cast <size_t> (buffer.at(2)) | (static_cast <size_t> (buffer.at(3)) << 8), 300ul);
	// Длина корня дерева свёрток обязана стоять на смещении четыре
	ASSERT_EQ(static_cast <size_t> (buffer.at(4)) | (static_cast <size_t> (buffer.at(5)) << 8), abc::DIGEST_LENGTH);
	// Октеты впрок обязаны быть уложены нулями
	ASSERT_EQ(buffer.at(6), 0x00);
	// Второй октет впрок обязан быть уложен нулём
	ASSERT_EQ(buffer.at(7), 0x00);
	/**
	 * Выполняем проверку того, что за заголовком идёт КОРЕНЬ, а не подпись
	 */
	for(size_t i = 0; i < abc::DIGEST_LENGTH; i++)
		// Октет корня дерева свёрток обязан стоять сразу за заголовком записи
		ASSERT_EQ(buffer.at(abc::SIGNATURE_HEADER + i), 0xA5) << "смещение октета: " << i;
	/**
	 * Выполняем проверку того, что за корнем идут октеты подписи
	 */
	for(size_t i = 0; i < sign.signature.size(); i++)
		// Октет подписи владельца обязан стоять за корнем дерева свёрток
		ASSERT_EQ(buffer.at(abc::SIGNATURE_HEADER + abc::DIGEST_LENGTH + i), 0x5A) << "смещение октета: " << i;
	// Код отказа снятия записи подписи
	abc::error_t error = abc::error_t::NONE;
	// Снятая запись подписи контейнера
	abc::sign_t taken;
	// Выполняем снятие уложенной записи подписи
	ASSERT_TRUE(abc::unpack(buffer.data(), buffer.size(), taken, error));
	// Снятый корень дерева свёрток обязан отвечать уложенному
	ASSERT_EQ(taken.root, sign.root);
	// Снятые октеты подписи обязаны отвечать уложенным
	ASSERT_EQ(taken.signature, sign.signature);
	// Выполняем занятие первого октета впрок
	buffer.at(6) = 0x01;
	// Занятые октеты впрок обязаны отвечаться отказом повреждённой записи
	ASSERT_FALSE(abc::unpack(buffer.data(), buffer.size(), taken, error));
	// Отказ обязан быть именно о повреждённой записи подписи
	ASSERT_EQ(error, abc::error_t::INVALID_SIGNATURE);
	// Выполняем возврат первого октета впрок и занятие второго
	buffer.at(6) = 0x00;
	// Выполняем занятие второго октета впрок
	buffer.at(7) = 0x01;
	// Занятый второй октет впрок обязан отвечаться тем же отказом
	ASSERT_FALSE(abc::unpack(buffer.data(), buffer.size(), taken, error));
	// Отказ обязан быть именно о повреждённой записи подписи
	ASSERT_EQ(error, abc::error_t::INVALID_SIGNATURE);
}
/**
 * @brief Проверка отказа снятия записи подписи с несовместной парой вида и хэша
 *
 * @details Вид хэш-суммы задаётся не потребителем, а ПОДБОРОМ по виду ключа - тем судит
 *          посредник `digest`, каким выбирает хэш сама укладка. Снятие же брало оба
 *          октета с провода как есть, и запись, объявляющая пару, какой кодек не уложил
 *          бы никогда, уходила работам шифрования неопознанной
 *
 * @note Судья здесь тот же самый намеренно: завести у снятия свой список значило бы
 *       развести двух судей об одном, и разошлись бы они молча. Посредник этот зовётся
 *       УКЛАДКОЙ трижды, а снятием не звался вовсе - судья был односторонний
 *
 * @note Случаи перебираются СТОЛБЦАМИ и утверждаются `EXPECT`, а не `ASSERT`: сторожа
 *       здесь ДВА - сличение вида с перечнем и сличение пары посредником, - и оба
 *       отвечают одним кодом. Прерви проверка перебор на первом же павшем случае, и
 *       подмена одного сторожа была бы неотличима от подмены другого. Замером
 *       разведено: снятие сличения пары валит случаи 2, 4, 5 и 6, а снятие сличения
 *       вида - ОДИН случай 7, ради него и заведённый. Первая редакция случая этого не
 *       имела вовсе, и снятие сличения вида не роняло НИЧЕГО: сторож стоял
 *       незакреплённым, а красное на случаях 5 и 6 принадлежало сличению пары
 *
 * @note Пары взяты с ОБЕИХ сторон договора: у Ed25519 хэша нет вовсе, у RSA он
 *       обязателен, и отказ обязан приходить как на объявленный лишний, так и на
 *       недостающий. Сторож, писанный под один край, второй пропустил бы
 *
 */
TEST_F(SignatureFixture, HashKindMustMatchSignatureKind) {
	/**
	 * @brief Структура поверяемого случая пары вида и хэша
	 *
	 */
	struct probe_t {
		// Вид подписи владельца контейнера
		crypto_t::signature_t kind;
		// Вид хэш-суммы, какой подпись выработана
		crypto_t::hash_t hash;
		// Признак подмены октета вида подписи поверх собранного
		bool patch;
		// Октет вида подписи, укладываемый поверх собранного
		uint8_t replace;
		// Признак того, что запись обязана сниматься без отказа
		bool accept;
		// Имя поверяемого случая
		const char * name;
	};
	// Перечень поверяемых случаев пары вида и хэша
	const vector <probe_t> probes = {
		{crypto_t::signature_t::ECDSA, crypto_t::hash_t::SHA384, false, 0x00, true, "1: ECDSA с хэшем - пара совместна"},
		{crypto_t::signature_t::ED25519, crypto_t::hash_t::SHA256, false, 0x00, false, "2: Ed25519 с хэшем - хэша схема не принимает"},
		{crypto_t::signature_t::ED25519, crypto_t::hash_t::NONE, false, 0x00, true, "3: Ed25519 без хэша - пара совместна"},
		{crypto_t::signature_t::RSA, crypto_t::hash_t::NONE, false, 0x00, false, "4: RSA без хэша - хэш обязателен"},
		{crypto_t::signature_t::ECDSA, crypto_t::hash_t::SHA256, true, 0x7F, false, "5: вид подписи неведом"},
		{crypto_t::signature_t::ECDSA, crypto_t::hash_t::SHA256, true,
		 static_cast <uint8_t> (crypto_t::signature_t::NONE), false, "6: вид подписи не задан"},
		{crypto_t::signature_t::ED25519, crypto_t::hash_t::NONE, true, 0x7F, false,
		 "7: вид подписи неведом, а хэша нет - пара для посредника совместна"}
	};
	/**
	 * Выполняем перебор поверяемых случаев пары вида и хэша
	 */
	for(auto & probe : probes){
		// Укладываемая подпись контейнера
		abc::sign_t sign;
		// Выполняем установку вида подписи владельца контейнера
		sign.kind = probe.kind;
		// Выполняем установку вида хэш-суммы, какой подпись выработана
		sign.hash = probe.hash;
		// Выполняем сборку корня дерева свёрток
		sign.root.assign(abc::DIGEST_LENGTH, 0xA5);
		// Выполняем сборку октетов подписи владельца
		sign.signature.assign(64, 0x5A);
		// Буфер уложенной записи подписи
		vector <uint8_t> record;
		// Выполняем проверку того, что запись подписи уложена
		ASSERT_TRUE(abc::pack(sign, record));
		// Выполняем проверку того, что запись подписи уложена
		ASSERT_FALSE(record.empty()) << probe.name;
		// Если вид подписи владельца подменяется поверх уложенного
		if(probe.patch)
			// Выполняем подмену октета вида подписи владельца
			record.at(0) = probe.replace;
		// Код отказа снятия записи подписи
		abc::error_t error = abc::error_t::NONE;
		// Снятая запись подписи контейнера
		abc::sign_t taken;
		// Запись обязана сниматься согласно совместности объявленной пары
		EXPECT_EQ(abc::unpack(record.data(), record.size(), taken, error), probe.accept) << probe.name;
		/**
		 * Если запись обязана отвечаться отказом
		 *
		 * @note Скобки обязательны, а не украшают: утверждение развёртывается в `if` со
		 *       своим `else`, и без скобок он повисает на этом условии. Нынешнему коду
		 *       вреда нет, но `else`, дописанный сюда потом, привяжется НЕ К ЭТОМУ `if`.
		 *       Ловится собирателем GNU (`-Wdangling-else`), собиратель clang молчит
		 */
		if(!probe.accept){
			// Отказ обязан быть именно о повреждённой записи подписи
			EXPECT_EQ(error, abc::error_t::INVALID_SIGNATURE) << probe.name;
		}
	}
}
/**
 * @brief Проверка того, что отказ укладки записи подписи не проходит молча
 *
 * @details Укладка отвергает подпись шире шестнадцати разрядов, и отказ этот ОБЯЗАН
 *          доходить до зовущего выдачей, а не пустотою записи. Прежде он объявлялся
 *          пустотою, и довод при укладке ссылался на сторожа у зовущего - сторожа
 *          того не было НИ У ОДНОГО из троих (`Container::commit`, `Editor::commit`,
 *          `Editor::vacuum`): все трое звали укладку и тут же заворачивали её плод
 *          кадром, не спросив ни о чём
 *
 * @note Проверка ведёт ОБЕ половины договора, и вторая здесь важнее первой. Первая -
 *       что укладка отвечает отказом. Вторая - что стало бы, отказ пропустив: пустое
 *       содержимое обёртка кадром принимает безропотно и отвечает УСПЕХОМ, кадр
 *       выходит безупречным по виду и снимается без единого нарекания, а расходится
 *       договор лишь у снятия самой подписи, да ещё и не тем кодом. Без второй
 *       половины проверка закрепляла бы возврат `false` как безделицу, тогда как цена
 *       ему - контейнер, объявленный подписанным и неповеряемый
 *
 * @note Досягаемости у отказа сегодня нет: самая широкая подпись поддержанных схем -
 *       2048 октетов у RSA-16384, а сторож заведён впрок под схемы послеквантовые.
 *       Оттого чинилось здесь не падение, а ДОГОВОР, разошедшийся с кодом, - и
 *       закрепляется он тем же порядком
 *
 */
TEST_F(SignatureFixture, OversizedRecordIsRefusedOutright) {
	// Собираемая подпись владельца контейнера
	abc::sign_t sign;
	// Выполняем установку вида подписи владельца контейнера
	sign.kind = crypto_t::signature_t::ED25519;
	// Выполняем установку вида хэш-суммы, какой подпись выработана
	sign.hash = crypto_t::hash_t::NONE;
	// Выполняем установку корня дерева свёрток
	sign.root.assign(abc::DIGEST_LENGTH, 0x5A);
	// Выполняем установку подписи, не умещающейся в две октеты длины
	sign.signature.assign(0x10000, 0x77);
	// Буфер уложенной записи подписи
	vector <uint8_t> record;
	// Выполняем проверку того, что укладка записи подписи отвечает отказом
	ASSERT_FALSE(abc::pack(sign, record));
	// Выполняем проверку того, что отвергнутая запись подписи осталась пустой
	ASSERT_TRUE(record.empty()) << "уложено октетов: " << record.size();
	/**
	 * Выполняем проверку того, чем обошёлся бы пропуск отказа: пустое содержимое
	 * обёртка кадром принимает безропотно, и кадр выходит неотличимым от годного
	 */
	{
		// Выполняем проверку того, что обёртка пустой записи кадром УДАЁТСЯ
		ASSERT_TRUE(abc::envelope(record, 1, 0));
		// Выполняем проверку того, что кадр вышел ровно в заголовок свой
		ASSERT_EQ(record.size(), abc::CHUNK_HEADER);
		// Укладчик кадров, каким кадр этот снимается
		abc::packer_t packer(this->_log.get());
		// Смещение снятия кадра
		size_t offset = 0;
		// Снятое содержимое кадра
		vector <uint8_t> content;
		// Снятые сведения о кадре
		abc::chunk_t chunk;
		// Выполняем проверку того, что кадр снимается БЕЗ единого нарекания
		ASSERT_TRUE(packer.unpack(record.data(), record.size(), offset, content, chunk))
			<< "код отказа: " << abc::message(packer.error());
		// Выполняем проверку того, что содержимое снятого кадра пусто
		ASSERT_TRUE(content.empty());
		// Снятая подпись владельца контейнера
		abc::sign_t taken;
		// Код отказа снятия подписи
		abc::error_t error = abc::error_t::NONE;
		/**
		 * Выполняем проверку того, что расходится договор лишь ЗДЕСЬ, у читающего:
		 * пишущий к этому мигу отчитался успехом всюду, где мог
		 */
		ASSERT_FALSE(abc::unpack(content.data(), content.size(), taken, error));
	}
}

/**
 * @brief Проверка отказов дерева свёрток при недостающем снаряжении
 *
 * @details Внесение кадра требует троих: модуля шифрования, буфера и ненулевой длины.
 *          Недостача любого из них есть отказ, и отказ этот - ЕДИНСТВЕННАЯ дорога, какою
 *          у сборки контейнера и правки его берётся причина `SIGNING_FAILED`
 *
 * @note Заведена разбором 05.09.2026: сплошной щуп показал двадцать мест `SIGNING_FAILED`
 *       у вместилища и правщика, каких не стерёг никто. Разбор же показал, что стеречь их
 *       ПОРОЗНЬ нельзя: внутри тех работ модуль отдан, а кадр непуст всегда, - то есть
 *       заслоны там суть последняя рука, и достижимы они лишь отсюда. Проверка стережёт
 *       дорогу, а не двадцать её устий
 *
 */
TEST_F(SignatureFixture, MerkleRefusesWithoutItsGear) {
	// Дерево свёрток без модуля шифрования
	abc::merkle_t bare(this->_log.get());
	// Выполняем проверку отказа внесения кадра без модуля шифрования
	ASSERT_FALSE(bare.add("кадр", 4));
	// Выполняем проверку того, что листьев у дерева не прибавилось
	ASSERT_EQ(bare.leaves(), 0ul);
	// Дерево свёрток с модулем шифрования
	abc::merkle_t merkle(this->_log.get());
	// Выполняем установку модуля шифрования дерева свёрток
	merkle.crypto(this->_crypto.get());
	// Выполняем проверку отказа внесения кадра, поданного пустым указателем
	ASSERT_FALSE(merkle.add(nullptr, 4));
	// Выполняем проверку отказа внесения кадра нулевой длины
	ASSERT_FALSE(merkle.add("кадр", 0));
	// Выполняем проверку того, что листьев у дерева не прибавилось
	ASSERT_EQ(merkle.leaves(), 0ul);
	/**
	 * Внесение кадра годного при том работает: без этой половины проверка прошла бы и у
	 * дерева, не принимающего ничего вовсе
	 */
	ASSERT_TRUE(merkle.add("кадр", 4));
	// Выполняем проверку того, что лист дерева прибавился
	ASSERT_EQ(merkle.leaves(), 1ul);
	// Корень собранного дерева свёрток
	vector <uint8_t> root;
	// Выполняем проверку того, что корень дерева снимается
	ASSERT_TRUE(merkle.root(root));
	// Выполняем проверку того, что корень дерева свёрток непуст
	ASSERT_FALSE(root.empty());
}
