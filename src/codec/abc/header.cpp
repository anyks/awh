/**
 * @file header.cpp
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
 * @brief Файл реализации заголовка опознания бинарного контейнера ABC
 *
 * \~english
 * @brief Implementation file of the identifying header of the ABC binary container
 *
 * \~
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Подключаем заголовочный файл модуля
 */
#include <codec/abc/header.hpp>
#include <codec/abc/encoding.hpp>

/**
 * Подключаем заголовочные файлы проекта
 */
#include <cryptography/hash.hpp>

/**
 * Стандартные заголовочные файлы
 */
#include <cstring>

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
	 * @brief Опознавательная запись контейнера
	 *
	 */
	const uint8_t Magic[4] = {'A', 'B', 'C', 0x00};
	/**
	 * @brief Зерно свёртки контрольной суммы заголовка
	 *
	 * @details Зерно закреплено постоянным: смена его обратила бы прежние контейнеры в
	 *          негодные, ибо сумма их перестала бы сходиться
	 *
	 */
	constexpr uint64_t Seed = 0x4142433130303031ull;
	/**
	 * @brief Смещение контрольной суммы в уложенном заголовке
	 *
	 */
	constexpr size_t Checksum = (awh::codec::abc::HEADER_LENGTH - 8);
};

/**
 * @brief Конструктор
 *
 */
awh::codec::abc::Header::Header() noexcept :
 version(VERSION_MAJOR), revision(VERSION_MINOR), flags(static_cast <uint16_t> (flag_t::NONE)),
 content(0), length(0), records(0), index(0), extent(0), signature(0), generation(0) {
	// Выполняем обнуление признака владельца контейнера
	::memset(this->owner, 0, OWNER_LENGTH);
	// Выполняем обнуление отпечатка открытого ключа
	::memset(this->fingerprint, 0, FINGERPRINT_LENGTH);
}
/**
 * @brief Метод проверки объявленного свойства контейнера
 *
 * @param flag проверяемое свойство контейнера
 * @return     признак объявленности свойства
 *
 */
bool awh::codec::abc::Header::is(const flag_t flag) const noexcept {
	// Выводим признак объявленности свойства контейнера
	return ((this->flags & static_cast <uint16_t> (flag)) != 0);
}
/**
 * @brief Метод объявления свойства контейнера
 *
 * @param flag  объявляемое свойство контейнера
 * @param value устанавливаемое значение свойства
 *
 */
void awh::codec::abc::Header::set(const flag_t flag, const bool value) noexcept {
	// Если свойство контейнера объявляется
	if(value)
		// Выполняем объявление свойства контейнера
		this->flags |= static_cast <uint16_t> (flag);
	// Выполняем снятие свойства контейнера
	else this->flags &= static_cast <uint16_t> (~static_cast <uint16_t> (flag));
}
/**
 * @brief Метод укладки заголовка в октеты
 *
 * @param result буфер, куда следует уложить заголовок
 *
 */
void awh::codec::abc::Header::pack(vector <uint8_t> & result) const noexcept {
	// Выполняем получение смещения начала укладываемого заголовка
	const size_t start = result.size();
	// Выполняем заведение места под укладываемый заголовок
	result.resize(start + HEADER_LENGTH, 0);
	// Выполняем получение указателя на укладываемый заголовок
	uint8_t * buffer = (result.data() + start);
	// Выполняем укладку опознавательной записи контейнера
	::memcpy(buffer, Magic, sizeof(Magic));
	// Выполняем укладку старшей версии вида записи
	buffer[4] = this->version;
	// Выполняем укладку младшей версии вида записи
	buffer[5] = this->revision;
	// Выполняем укладку разрядов свойств контейнера
	abc::fixed(buffer + 6, static_cast <uint64_t> (this->flags), 2);
	// Выполняем укладку признака владельца контейнера
	::memcpy(buffer + 8, this->owner, OWNER_LENGTH);
	// Выполняем укладку вида содержимого контейнера
	abc::fixed(buffer + 24, static_cast <uint64_t> (this->content), 4);
	// Выполняем укладку длины содержимого кадра оглавления
	abc::fixed(buffer + 28, static_cast <uint64_t> (this->extent), 4);
	// Выполняем укладку длины тела контейнера
	abc::fixed(buffer + 32, this->length, 8);
	// Выполняем укладку количества записей в теле контейнера
	abc::fixed(buffer + 40, this->records, 8);
	// Выполняем укладку смещения оглавления
	abc::fixed(buffer + 48, this->index, 8);
	// Выполняем укладку смещения подписи
	abc::fixed(buffer + 56, this->signature, 8);
	// Выполняем укладку отпечатка открытого ключа
	::memcpy(buffer + 64, this->fingerprint, FINGERPRINT_LENGTH);
	// Выполняем укладку поколения записи контейнера
	abc::fixed(buffer + 80, this->generation, 8);
	/**
	 * Выполняем укладку контрольной суммы заголовка.
	 *
	 * Сумма считается по уже уложенным октетам, а не по полям: считать её по полям
	 * значило бы стеречь не то, что ляжет на носитель, и всякая правка укладки
	 * проходила бы мимо суммы незамеченной
	 */
	abc::fixed(buffer + Checksum, awh::hashing::generate(buffer, Checksum, Seed), 8);
}
/**
 * @brief Метод снятия заголовка с октетов
 *
 * @param buffer буфер поданных октетов
 * @param size   размер поданных октетов
 * @param error  код отказа, если снять заголовок не удалось
 * @return       признак успешно снятого заголовка
 *
 */
bool awh::codec::abc::Header::unpack(const void * buffer, const size_t size, error_t & error) noexcept {
	// Выполняем сброс кода отказа
	error = error_t::NONE;
	// Если буфер поданных октетов не существует
	if(buffer == nullptr){
		// Выполняем установку кода внутреннего отказа
		error = error_t::INTERNAL;
		// Сообщаем, что заголовок не снят
		return false;
	}
	// Если поданных октетов недостаёт на заголовок
	if(size < HEADER_LENGTH){
		// Выполняем установку кода отказа обрыва заголовка
		error = error_t::TRUNCATED_HEADER;
		// Сообщаем, что заголовок не снят
		return false;
	}
	// Выполняем получение указателя на поданные октеты
	const uint8_t * octets = reinterpret_cast <const uint8_t *> (buffer);
	// Если опознавательная запись контейнера не сошлась
	if(::memcmp(octets, Magic, sizeof(Magic)) != 0){
		// Выполняем установку кода отказа опознания
		error = error_t::INVALID_MAGIC;
		// Сообщаем, что заголовок не снят
		return false;
	}
	/**
	 * Если контрольная сумма заголовка не сошлась.
	 *
	 * Сумма сличается прежде разбора полей: поля повреждённого заголовка толковать
	 * незачем, а смещения из него увели бы чтение в произвольное место записи
	 */
	if(abc::gather(octets + Checksum, 8) != awh::hashing::generate(octets, Checksum, Seed)){
		// Выполняем установку кода отказа контрольной суммы
		error = error_t::INVALID_CHECKSUM;
		// Сообщаем, что заголовок не снят
		return false;
	}
	// Выполняем снятие старшей версии вида записи
	const uint8_t group = octets[4];
	// Если вид записи контейнера не поддерживается
	if(group != VERSION_MAJOR){
		// Выполняем установку кода отказа вида записи
		error = error_t::INVALID_VERSION;
		// Сообщаем, что заголовок не снят
		return false;
	}
	// Выполняем установку старшей версии вида записи
	this->version = group;
	// Выполняем снятие младшей версии вида записи
	this->revision = octets[5];
	// Выполняем снятие разрядов свойств контейнера
	this->flags = static_cast <uint16_t> (abc::gather(octets + 6, 2));
	// Выполняем снятие признака владельца контейнера
	::memcpy(this->owner, octets + 8, OWNER_LENGTH);
	// Выполняем снятие вида содержимого контейнера
	this->content = static_cast <uint32_t> (abc::gather(octets + 24, 4));
	// Выполняем снятие длины содержимого кадра оглавления
	this->extent = static_cast <uint32_t> (abc::gather(octets + 28, 4));
	// Выполняем снятие длины тела контейнера
	this->length = abc::gather(octets + 32, 8);
	// Выполняем снятие количества записей в теле контейнера
	this->records = abc::gather(octets + 40, 8);
	// Выполняем снятие смещения оглавления
	this->index = abc::gather(octets + 48, 8);
	// Выполняем снятие смещения подписи
	this->signature = abc::gather(octets + 56, 8);
	// Выполняем снятие отпечатка открытого ключа
	::memcpy(this->fingerprint, octets + 64, FINGERPRINT_LENGTH);
	// Выполняем снятие поколения записи контейнера
	this->generation = abc::gather(octets + 80, 8);
	// Сообщаем, что заголовок снят
	return true;
}
/**
 * @brief Функция быстрой проверки поданных октетов на признак контейнера
 *
 * @param buffer буфер поданных октетов
 * @param size   размер поданных октетов
 * @return       признак того, что октеты начинают контейнер
 *
 */
bool awh::codec::abc::probe(const void * buffer, const size_t size) noexcept {
	// Если буфер поданных октетов не существует
	if(buffer == nullptr)
		// Сообщаем, что октеты контейнера не начинают
		return false;
	// Если поданных октетов недостаёт на заголовок
	if(size < HEADER_LENGTH)
		// Сообщаем, что октеты контейнера не начинают
		return false;
	// Выполняем получение указателя на поданные октеты
	const uint8_t * octets = reinterpret_cast <const uint8_t *> (buffer);
	// Если опознавательная запись контейнера не сошлась
	if(::memcmp(octets, Magic, sizeof(Magic)) != 0)
		// Сообщаем, что октеты контейнера не начинают
		return false;
	// Выводим признак схождения контрольной суммы заголовка
	return (abc::gather(octets + Checksum, 8) == awh::hashing::generate(octets, Checksum, Seed));
}
