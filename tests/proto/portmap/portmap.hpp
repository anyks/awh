/**
 * @file: portmap.hpp
 * @date: 2026-08-02
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @brief Заголовочный файл тестовой фикстуры договоров перенаправления портов —
 *        создание объектов тестового окружения перед каждым тестом и их освобождение после его завершения
 *
 * @copyright: Copyright © 2026
 *
 */

/**
 * Экранируем повторную инициализацию модуля
 */
#ifndef __AWH_PROTO_PORTMAP_TESTS__
#define __AWH_PROTO_PORTMAP_TESTS__

/**
 * Подключаем заголовочный файлы проекта
 */
#include "../../main.hpp"
#include "../../../include/proto/portmap/pcp.hpp"
#include "../../../include/proto/portmap/ssdp.hpp"
#include "../../../include/proto/portmap/soap.hpp"
#include "../../../include/proto/portmap/upnp.hpp"
#include "../../../include/proto/portmap/device.hpp"
#include "../../../include/proto/portmap/natpmp.hpp"

/**
 * @brief Класс фикстуры для тестов подмодуля договоров перенаправления портов
 *
 */
class PortmapFixture : public testing::Test {
	protected:
		// Объект фреймворка
		std::unique_ptr <awh::fmk_t> _fmk;
		// Объект логов
		std::unique_ptr <awh::log_t> _log;
	public:
		/**
		 * @brief Метод настройки тестового окружения
		 *
		 */
		void SetUp();
		/**
		 * @brief Метод очистки тестового окружения
		 *
		 */
		void TearDown();
	protected:
		/**
		 * @brief Фабричный метод создания кодека договора NAT-PMP
		 *
		 * @return сформированный объект кодека договора NAT-PMP
		 *
		 */
		std::unique_ptr <awh::proto::portmap::natpmp_t> makeNatPmp() const noexcept;
		/**
		 * @brief Фабричный метод создания кодека договора PCP
		 *
		 * @return сформированный объект кодека договора PCP
		 *
		 */
		std::unique_ptr <awh::proto::portmap::pcp_t> makePcp() const noexcept;
		/**
		 * @brief Фабричный метод создания кодека договора SSDP
		 *
		 * @return сформированный объект кодека договора SSDP
		 *
		 */
		std::unique_ptr <awh::proto::portmap::ssdp_t> makeSsdp() const noexcept;
		/**
		 * @brief Фабричный метод создания кодека описания устройства UPnP
		 *
		 * @return сформированный объект кодека описания устройства UPnP
		 *
		 */
		std::unique_ptr <awh::proto::portmap::device_t> makeDevice() const noexcept;
		/**
		 * @brief Фабричный метод создания кодека договора SOAP
		 *
		 * @return сформированный объект кодека договора SOAP
		 *
		 */
		std::unique_ptr <awh::proto::portmap::soap_t> makeSoap() const noexcept;
		/**
		 * @brief Фабричный метод создания кодека действий службы перенаправления UPnP
		 *
		 * @return сформированный объект кодека действий службы перенаправления UPnP
		 *
		 */
		std::unique_ptr <awh::proto::portmap::upnp_t> makeUpnp() const noexcept;
};

#endif // __AWH_PROTO_PORTMAP_TESTS__
