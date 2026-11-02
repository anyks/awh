/**
 * @file portmap.cpp
 * @date 2026-08-02
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
 * @brief Реализация тестовой фикстуры договоров перенаправления портов —
 *        создание объектов тестового окружения перед каждым тестом и их освобождение после его завершения
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Подключаем заголовочный файлы проекта
 */
#include "portmap.hpp"

/**
 * @brief Метод настройки тестового окружения
 *
 */
void PortmapFixture::SetUp(){}

/**
 * @brief Метод очистки тестового окружения
 *
 */
void PortmapFixture::TearDown() {}

/**
 * @brief Фабричный метод создания кодека описания устройства UPnP
 *
 * @return сформированный объект кодека описания устройства UPnP
 *
 */
std::unique_ptr <awh::proto::portmap::device_t> PortmapFixture::makeDevice() const noexcept {
	// Создаём и возвращаем объект кодека описания устройства UPnP
	return std::make_unique <awh::proto::portmap::device_t> ();
}
/**
 * @brief Метод записи адреса записью IPv6, отведённой договором PCP
 *
 * @param address место под записываемый адрес размером ADDRESS_SIZE
 * @param value   записываемый адрес видом обычной записи
 *
 */
void PortmapFixture::encodeAddress(uint8_t * address, const std::string & value) const noexcept {
	// Создаём объект работы с адресами
	awh::net_addr_t addr;
	// Выполняем разбор записываемого адреса
	if(!addr.parse(value))
		// Выходим из записи адреса
		return;
	// Получаем адрес записью IPv6, предписанной договором
	const auto & result = addr.v6(awh::net_addr_t::endian_t::LITTLE);
	// Выполняем копирование полученного адреса
	::memcpy(address, result.data(), result.size());
}
/**
 * @brief Метод извлечения адреса IPv4 из записи, отведённой договором PCP
 *
 * @param address извлекаемый адрес размером ADDRESS_SIZE
 * @param value   ссылка на извлечённый адрес IPv4 в порядке октетов машины
 * @return        признак того, что адрес принадлежит IPv4
 *
 */
bool PortmapFixture::decodeAddress(const uint8_t * address, uint32_t & value) const noexcept {
	// Извлекаемый адрес
	std::array <uint8_t, 16> buffer;
	// Выполняем копирование извлекаемого адреса
	::memcpy(buffer.data(), address, buffer.size());
	// Создаём объект работы с адресами
	awh::net_addr_t addr;
	// Выполняем размещение извлекаемого адреса
	addr.v6(buffer, awh::net_addr_t::endian_t::LITTLE);
	// Запоминаем извлечённый адрес IPv4
	value = addr.v4(awh::net_addr_t::endian_t::BIG);
	// Выводим признак того, что адрес принадлежит IPv4
	return (value > 0);
}
