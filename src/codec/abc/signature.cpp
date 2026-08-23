/**
 * @file signature.cpp
 * @date 2026-08-19
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
 * @brief Файл реализации подписи владельца бинарного контейнера ABC
 *
 * \~english
 * @brief Implementation file of the signature of the owner of the ABC binary container
 *
 * \~
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Подключаем заголовочный файл модуля
 */
#include <codec/abc/signature.hpp>

/**
 * Стандартные заголовочные файлы
 */
#include <cstring>
#include <limits>

/**
 * Используем стандартное пространство имён
 */
using namespace std;

/**
 * @brief Пространство имён работ, доступных лишь этому файлу
 *
 */
namespace {
	/**
	 * @brief Приставка свёртки листа дерева
	 *
	 */
	constexpr uint8_t Leaf = 0x00;
	/**
	 * @brief Приставка свёртки узла дерева
	 *
	 */
	constexpr uint8_t Node = 0x01;
	/**
	 * @brief Вид хэш-суммы дерева свёрток
	 *
	 */
	constexpr awh::crypto_t::hash_t Digest = awh::crypto_t::hash_t::SHA256;
};

/**
 * @brief Метод объявления отказа работы с деревом свёрток
 *
 * @details Своего кода отказа у дерева нет - вызовы его отвечают одною лишь ложью, -
 *          оттого воронка эта доносит текстом. Место у неё единственное по той же
 *          причине, что и у прочих: запись в каждом пути разошлась бы с остальными
 *
 * @param message текст объявляемого отказа
 * @return        признак успешности, всегда ложь
 *
 */
bool awh::codec::abc::Merkle::fail(const char * message) const noexcept {
	/**
	 * Если объект логирования отдан, доносим об отказе
	 */
	if(this->_log != nullptr){
		/**
		 * Если включён режим отладки
		 */
		#if DEBUG_MODE
			// Записываем ошибку в лог
			this->_log->debug("ABC: %s", __PRETTY_FUNCTION__, make_tuple(this->_leaves.size()),
			 log_t::flag_t::WARNING, message);
		/**
		 * Если режим отладки не включён
		 */
		#else
			// Записываем ошибку в лог
			this->_log->print("ABC: %s", log_t::flag_t::WARNING, message);
		#endif
	}
	// Сообщаем, что работа с деревом отвечена отказом
	return false;
}
/**
 * @brief Метод установки модуля шифрования
 *
 * @param value устанавливаемый модуль шифрования, ноль - снятие модуля
 *
 */
void awh::codec::abc::Merkle::crypto(const crypto_t * value) noexcept {
	// Выполняем установку модуля шифрования
	this->_crypto = value;
}
/**
 * @brief Метод внесения кадра свёрткой в дерево
 *
 * @param buffer буфер октетов вносимого кадра
 * @param size   размер октетов вносимого кадра
 * @return       признак успешного внесения
 *
 */
bool awh::codec::abc::Merkle::add(const void * buffer, const size_t size) noexcept {
	/**
	 * Если модуль шифрования не отдан либо кадр не передан
	 */
	if((this->_crypto == nullptr) || (buffer == nullptr) || (size == 0))
		// Выводим признак неудачного внесения кадра
		return this->fail("Merkle tree: the module of the encryption or the chunk is not given");
	/**
	 * Выполняем сборку буфера свёртки листа с приставкой: без приставки узел
	 * можно было бы выдать за лист, а лист за узел
	 */
	vector <uint8_t> payload;
	// Выполняем заведение места под буфер свёртки листа
	payload.reserve(size + 1);
	// Выполняем внесение приставки свёртки листа
	payload.push_back(Leaf);
	// Выполняем внесение октетов вносимого кадра
	payload.insert(payload.end(),
	 reinterpret_cast <const uint8_t *> (buffer), reinterpret_cast <const uint8_t *> (buffer) + size);
	// Выполняем выработку свёртки листа дерева
	vector <uint8_t> result = this->_crypto->hash <vector <uint8_t>, vector <uint8_t>> (
	 payload, Digest, crypto_t::format_t::RAW);
	/**
	 * Если свёртка листа выработана не той длины
	 */
	if(result.size() != DIGEST_LENGTH)
		// Выводим признак неудачного внесения кадра
		return this->fail("Merkle tree: the digest of a leaf is of a wrong length");
	// Выполняем внесение свёртки листа в дерево
	this->_leaves.push_back(::std::move(result));
	// Выводим признак успешного внесения кадра
	return true;
}
/**
 * @brief Метод сведения дерева к корню
 *
 * @param result буфер, куда следует положить корень дерева
 * @return       признак успешно сведённого дерева
 *
 */
bool awh::codec::abc::Merkle::root(vector <uint8_t> & result, const void * buffer, const size_t size) const noexcept {
	// Выполняем очистку буфера корня дерева
	result.clear();
	/**
	 * Если модуль шифрования не отдан либо приданный кадр не передан
	 */
	if((this->_crypto == nullptr) || (buffer == nullptr) || (size == 0))
		// Выводим признак неудачного сведения дерева
		return this->fail("Merkle tree: the module of the encryption or the appended chunk is not given");
	// Дерево свёрток с приданным кадром
	Merkle merkle(this->_log);
	// Выполняем установку модуля шифрования дереву свёрток
	merkle.crypto(this->_crypto);
	// Выполняем перенесение свёрток нынешнего дерева
	merkle._leaves = this->_leaves;
	/**
	 * Если внести приданный кадр свёрткой в дерево не вышло
	 */
	if(!merkle.add(buffer, size))
		// Выводим признак неудачного сведения дерева
		return false;
	// Выводим результат сведения дерева с приданной свёрткой к корню
	return merkle.root(result);
}
/**
 * @brief Метод сведения дерева к корню
 *
 * @param result буфер, куда следует положить корень дерева
 * @return       признак успешно сведённого дерева
 *
 */
bool awh::codec::abc::Merkle::root(vector <uint8_t> & result) const noexcept {
	// Выполняем очистку буфера корня дерева
	result.clear();
	/**
	 * Если модуль шифрования не отдан либо кадров в дереве нет
	 */
	if((this->_crypto == nullptr) || this->_leaves.empty())
		// Выводим признак неудачного сведения дерева
		return this->fail("Merkle tree: the module of the encryption is not given or the tree is empty");
	// Свёртки нынешнего яруса дерева
	vector <vector <uint8_t>> tier = this->_leaves;
	/**
	 * Выполняем сведение ярусов дерева, покуда свёртка не останется одна
	 */
	while(tier.size() > 1){
		// Свёртки яруса, следующего за нынешним
		vector <vector <uint8_t>> next;
		// Выполняем заведение места под свёртки следующего яруса
		next.reserve((tier.size() + 1) / 2);
		/**
		 * Выполняем перебор свёрток нынешнего яруса парами
		 */
		for(size_t i = 0; i < tier.size(); i += 2){
			/**
			 * Если пары свёртке не досталось, поднимаем её ярусом выше как есть.
			 *
			 * Сдваивать её саму с собой нельзя: сдваивание позволяет двум разным
			 * чередам кадров дать один корень
			 */
			if((i + 1) >= tier.size()){
				// Выполняем поднятие свёртки на ярус выше
				next.push_back(tier.at(i));
				// Прекращаем перебор свёрток нынешнего яруса
				break;
			}
			// Буфер свёртки узла дерева
			vector <uint8_t> payload;
			// Выполняем заведение места под буфер свёртки узла
			payload.reserve((DIGEST_LENGTH * 2) + 1);
			// Выполняем внесение приставки свёртки узла
			payload.push_back(Node);
			// Выполняем внесение свёртки левой ветви узла
			payload.insert(payload.end(), tier.at(i).begin(), tier.at(i).end());
			// Выполняем внесение свёртки правой ветви узла
			payload.insert(payload.end(), tier.at(i + 1).begin(), tier.at(i + 1).end());
			// Выполняем выработку свёртки узла дерева
			vector <uint8_t> node = this->_crypto->hash <vector <uint8_t>, vector <uint8_t>> (
			 payload, Digest, crypto_t::format_t::RAW);
			/**
			 * Если свёртка узла выработана не той длины
			 */
			if(node.size() != DIGEST_LENGTH)
				// Выводим признак неудачного сведения дерева
				return this->fail("Merkle tree: the digest of a node is of a wrong length");
			// Выполняем внесение свёртки узла в следующий ярус
			next.push_back(::std::move(node));
		}
		// Выполняем переход к следующему ярусу дерева
		tier = ::std::move(next);
	}
	// Выполняем выдачу корня сведённого дерева
	result = tier.front();
	// Выводим признак успешно сведённого дерева
	return true;
}
/**
 * @brief Метод извлечения количества внесённых кадров
 *
 * @return количество внесённых кадров
 *
 */
size_t awh::codec::abc::Merkle::leaves() const noexcept {
	// Выводим количество внесённых кадров
	return this->_leaves.size();
}
/**
 * @brief Метод очистки дерева свёрток
 *
 */
void awh::codec::abc::Merkle::clear() noexcept {
	// Выполняем очистку свёрток кадров контейнера
	this->_leaves.clear();
}
/**
 * @brief Функция подбора вида хэш-суммы под вид подписи
 *
 * @param kind вид подписи владельца контейнера
 * @param hash желаемый вид хэш-суммы
 * @return     вид хэш-суммы, годный виду подписи
 *
 */
awh::crypto_t::hash_t awh::codec::abc::digest(const crypto_t::signature_t kind,
 const crypto_t::hash_t hash) noexcept {
	/**
	 * Определяем вид подписи владельца контейнера
	 */
	switch(static_cast <uint8_t> (kind)){
		/**
		 * Если подпись выработана схемой Ed25519 либо по ГОСТ, хэш-сумма не задаётся:
		 * у первой её нет вовсе, у второй она предписана самой схемой
		 */
		case static_cast <uint8_t> (crypto_t::signature_t::ED25519):
		case static_cast <uint8_t> (crypto_t::signature_t::GOST):
			return crypto_t::hash_t::NONE;
		/**
		 * Если подпись выработана схемой RSA либо ECDSA, хэш-сумма обязательна
		 */
		case static_cast <uint8_t> (crypto_t::signature_t::RSA):
		case static_cast <uint8_t> (crypto_t::signature_t::ECDSA):
			// Выводим желаемый вид хэш-суммы, а при отсутствии его - вид по умолчанию
			return ((hash == crypto_t::hash_t::NONE) ? crypto_t::hash_t::SHA256 : hash);
	}
	// Выводим отсутствие вида хэш-суммы
	return crypto_t::hash_t::NONE;
}
/**
 * @brief Функция укладки записи подписи контейнера
 *
 * @param sign   укладываемая подпись контейнера
 * @param result буфер, куда следует уложить запись подписи
 *
 */
void awh::codec::abc::pack(const sign_t & sign, vector <uint8_t> & result) noexcept {
	// Выполняем получение смещения начала укладываемой записи подписи
	const size_t start = result.size();
	// Выполняем заведение места под заголовок записи подписи
	result.resize(start + SIGNATURE_HEADER, 0);
	// Выполняем укладку вида подписи владельца контейнера
	result[start] = static_cast <uint8_t> (sign.kind);
	// Выполняем укладку вида хэш-суммы, какой подпись выработана
	result[start + 1] = static_cast <uint8_t> (sign.hash);
	// Выполняем укладку длины подписи владельца контейнера
	result[start + 2] = static_cast <uint8_t> (sign.signature.size() & 0xFF);
	// Выполняем укладку старшего октета длины подписи владельца контейнера
	result[start + 3] = static_cast <uint8_t> ((sign.signature.size() >> 8) & 0xFF);
	// Выполняем укладку длины корня дерева свёрток
	result[start + 4] = static_cast <uint8_t> (sign.root.size() & 0xFF);
	// Выполняем укладку старшего октета длины корня дерева свёрток
	result[start + 5] = static_cast <uint8_t> ((sign.root.size() >> 8) & 0xFF);
	// Выполняем внесение корня дерева свёрток
	result.insert(result.end(), sign.root.begin(), sign.root.end());
	// Выполняем внесение октетов подписи владельца контейнера
	result.insert(result.end(), sign.signature.begin(), sign.signature.end());
}
/**
 * @brief Функция снятия записи подписи контейнера
 *
 * @param buffer буфер поданных октетов
 * @param size   размер поданных октетов
 * @param sign   снятая подпись контейнера
 * @param error  код отказа, если снять подпись не удалось
 * @return       признак успешно снятой подписи
 *
 */
bool awh::codec::abc::unpack(const void * buffer, const size_t size, sign_t & sign, error_t & error) noexcept {
	// Выполняем сброс кода отказа снятия подписи
	error = error_t::NONE;
	// Выполняем очистку корня дерева свёрток
	sign.root.clear();
	// Выполняем очистку октетов подписи владельца контейнера
	sign.signature.clear();
	/**
	 * Если октеты нам переданы неверно
	 */
	if(buffer == nullptr){
		// Выполняем установку кода отказа снятия подписи
		error = error_t::INTERNAL;
		// Выводим признак неудачного снятия подписи
		return false;
	}
	/**
	 * Если поданных октетов недостаёт на заголовок записи подписи
	 */
	if(size < SIGNATURE_HEADER){
		// Выполняем установку кода отказа обрыва подписи
		error = error_t::TRUNCATED_SIGNATURE;
		// Выводим признак неудачного снятия подписи
		return false;
	}
	// Выполняем получение указателя на поданные октеты
	const uint8_t * octets = reinterpret_cast <const uint8_t *> (buffer);
	// Выполняем снятие вида подписи владельца контейнера
	sign.kind = static_cast <crypto_t::signature_t> (octets[0]);
	// Выполняем снятие вида хэш-суммы, какой подпись выработана
	sign.hash = static_cast <crypto_t::hash_t> (octets[1]);
	// Выполняем снятие длины подписи владельца контейнера
	const size_t length = (static_cast <size_t> (octets[2]) | (static_cast <size_t> (octets[3]) << 8));
	// Выполняем снятие длины корня дерева свёрток
	const size_t width = (static_cast <size_t> (octets[4]) | (static_cast <size_t> (octets[5]) << 8));
	/**
	 * Если поданных октетов недостаёт на объявленную запись подписи
	 */
	if(size < (SIGNATURE_HEADER + width + length)){
		// Выполняем установку кода отказа обрыва подписи
		error = error_t::TRUNCATED_SIGNATURE;
		// Выводим признак неудачного снятия подписи
		return false;
	}
	/**
	 * Если корень дерева свёрток объявлен не той длины либо подпись пуста
	 */
	if((width != DIGEST_LENGTH) || (length == 0)){
		// Выполняем установку кода отказа повреждённой подписи
		error = error_t::INVALID_SIGNATURE;
		// Выводим признак неудачного снятия подписи
		return false;
	}
	// Выполняем снятие корня дерева свёрток
	sign.root.assign(octets + SIGNATURE_HEADER, octets + SIGNATURE_HEADER + width);
	// Выполняем снятие октетов подписи владельца контейнера
	sign.signature.assign(octets + SIGNATURE_HEADER + width, octets + SIGNATURE_HEADER + width + length);
	// Выводим признак успешно снятой подписи
	return true;
}
/**
 * @brief Функция выработки отпечатка открытого ключа владельца
 *
 * @param crypto модуль шифрования, отданный потребителем
 * @param name   имя ключа владельца контейнера
 * @param result буфер, куда следует положить усечённый отпечаток
 * @return       признак успешно выработанного отпечатка
 *
 */
bool awh::codec::abc::fingerprint(const crypto_t & crypto, const string & name, vector <uint8_t> & result) noexcept {
	// Выполняем очистку буфера усечённого отпечатка
	result.clear();
	// Выполняем выработку полного отпечатка открытого ключа владельца
	const vector <uint8_t> full = crypto.fingerprint <vector <uint8_t>> (name, crypto_t::format_t::RAW);
	/**
	 * Если отпечаток открытого ключа выработать не вышло
	 */
	if(full.empty())
		// Выводим признак неудачной выработки отпечатка
		return false;
	/**
	 * Выполняем усечение отпечатка от начала до длины, отведённой заголовком:
	 * усечение именно от начала, ибо сличать его станут с чужим полным
	 */
	result.assign(full.begin(), full.begin() + static_cast <ptrdiff_t> (
	 (full.size() < FINGERPRINT_LENGTH) ? full.size() : FINGERPRINT_LENGTH));
	// Выводим признак успешно выработанного отпечатка
	return true;
}
