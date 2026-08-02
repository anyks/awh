/**
 * @file: portmap.cpp
 * @date: 2026-08-02
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @brief Реализация тестовой фикстуры договоров перенаправления портов —
 *        создание объектов тестового окружения перед каждым тестом и их освобождение после его завершения
 *
 * @copyright: Copyright © 2026
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
void PortmapFixture::SetUp(){
	// Создаём объект фреймворка
	this->_fmk = std::make_unique <awh::fmk_t> ();
	// Создаём объект логгера
	this->_log = std::make_unique <awh::log_t> (this->_fmk.get());
}

/**
 * @brief Метод очистки тестового окружения
 *
 */
void PortmapFixture::TearDown() {}

/**
 * @brief Фабричный метод создания кодека договора NAT-PMP
 *
 * @return сформированный объект кодека договора NAT-PMP
 *
 */
std::unique_ptr <awh::proto::portmap::natpmp_t> PortmapFixture::makeNatPmp() const noexcept {
	// Создаём и возвращаем объект кодека договора NAT-PMP
	return std::make_unique <awh::proto::portmap::natpmp_t> (this->_fmk.get(), this->_log.get());
}
/**
 * @brief Фабричный метод создания кодека договора PCP
 *
 * @return сформированный объект кодека договора PCP
 *
 */
std::unique_ptr <awh::proto::portmap::pcp_t> PortmapFixture::makePcp() const noexcept {
	// Создаём и возвращаем объект кодека договора PCP
	return std::make_unique <awh::proto::portmap::pcp_t> (this->_fmk.get(), this->_log.get());
}
/**
 * @brief Фабричный метод создания кодека договора SSDP
 *
 * @return сформированный объект кодека договора SSDP
 *
 */
std::unique_ptr <awh::proto::portmap::ssdp_t> PortmapFixture::makeSsdp() const noexcept {
	// Создаём и возвращаем объект кодека договора SSDP
	return std::make_unique <awh::proto::portmap::ssdp_t> (this->_fmk.get(), this->_log.get());
}
/**
 * @brief Фабричный метод создания кодека описания устройства UPnP
 *
 * @return сформированный объект кодека описания устройства UPnP
 *
 */
std::unique_ptr <awh::proto::portmap::device_t> PortmapFixture::makeDevice() const noexcept {
	// Создаём и возвращаем объект кодека описания устройства UPnP
	return std::make_unique <awh::proto::portmap::device_t> (this->_fmk.get(), this->_log.get());
}
