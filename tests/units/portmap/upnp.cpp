/**
 * @file: upnp.cpp
 * @date: 2026-08-05
 * @license: LicenseRef-AWH-1.0
 *
 * @telegram: @forman
 * @author: Yuriy Lobarev
 * @phone: +7 (910) 983-95-90
 * @email: forman@anyks.com
 * @site: https://anyks.com
 *
 * @brief Проверка обмена с устройством UPnP на поддельном шлюзе —
 *        исправный ход обмена и внесение отказов, которых исправное устройство не даёт
 *
 * @copyright: Copyright © 2026
 *
 */

/**
 * Подключаем заголовочный файлы проекта
 */
#include "igd.hpp"
#include "portmap.hpp"

/**
 * @brief Метод настройки модуля на поддельный шлюз с поддержкой UPnP
 *
 * @details Обмен ведётся устройством петли: рассылка обнаружения уходит им же, и
 *          отвечает на неё поддельный шлюз, а не устройство настоящей сети. Без
 *          указания устройства рассылка ушла бы маршрутом до внешней сети, и ответить
 *          на неё мог бы живой маршрутизатор - испытание тогда завело бы на нём
 *          перенаправление порта, о котором никто не просил
 *
 * @param portmap объект модуля перенаправления портов
 *
 */
static void setup(awh::unit::portmap_t & portmap) noexcept {
	// Устанавливаем вид опроса маршрутизатора
	portmap.setType(awh::unit::portmap_t::type_t::UPNP);
	// Устанавливаем сетевое устройство петли, которым ведётся обмен
	portmap.setIface("lo0");
	// Устанавливаем срок ожидания ответа устройства
	portmap.setTimeout(600);
	// Устанавливаем количество попыток обращения к устройству
	portmap.setAttempts(2);
}

/**
 * @brief Проверка исправного хода обмена по договору UPnP
 *
 * @details Поддельный шлюз отвечает на рассылку обнаружения, выдаёт описание устройства
 *          со службой перенаправления и принимает вызов действия. Ход этот прежде не
 *          исполнялся ни одним испытанием: рассылка обнаружения устройством не
 *          направлялась, и обмен уходил в настоящую сеть
 *
 */
TEST_F(PortmapUnitFixture, PortmapUpnpMapping) {
	// Создаём поддельный шлюз с поддержкой UPnP
	FakeIGD igd;
	// Если гнёзда поддельного шлюза завести не удалось, испытание не проводится
	if(!igd.ready()) GTEST_SKIP() << "SSDP port is occupied or the host has no local network address";
	// Выполняем запуск поддельного шлюза
	igd.start();
	// Создаём объект модуля перенаправления портов
	awh::unit::portmap_t portmap(this->_fmk.get(), this->_log.get());
	// Выполняем настройку модуля на поддельный шлюз
	::setup(portmap);
	// Выполняем ожидание итога обращения к устройству
	const outcome_t outcome = this->await(portmap, awh::unit::portmap_t::action_t::OPEN);
	// Выполняем остановку поддельного шлюза
	igd.stop();
	// Выполняем проверку того, что рассылка обнаружения дошла до поддельного шлюза
	ASSERT_GT(igd.searches(), 0) << "рассылка ушла мимо устройства петли";
	// Выполняем проверку того, что описание устройства было прочитано
	ASSERT_GT(igd.fetches(), 0);
	// Выполняем проверку того, что действие службы было вызвано
	ASSERT_GT(igd.calls(), 0);
	// Выполняем проверку того, что устройство ответило
	ASSERT_TRUE(outcome.answered) << "код отказа " << static_cast <int32_t> (outcome.error);
	// Выполняем проверку того, что обращение отказом не завершилось
	ASSERT_FALSE(outcome.failed);
	// Выполняем проверку договора, по которому получен итог
	ASSERT_EQ(outcome.type, awh::unit::portmap_t::type_t::UPNP);
}

/**
 * @brief Проверка запроса внешнего адреса по договору UPnP
 *
 * @details Действие это перенаправления не заводит и отвечает одним лишь адресом:
 *          разбирается ответ на него отдельным ходом, до которого прежде не доходило
 *          ни одно испытание
 *
 */
TEST_F(PortmapUnitFixture, PortmapUpnpExternal) {
	// Создаём поддельный шлюз с поддержкой UPnP
	FakeIGD igd;
	// Если гнёзда поддельного шлюза завести не удалось, испытание не проводится
	if(!igd.ready()) GTEST_SKIP() << "SSDP port is occupied or the host has no local network address";
	// Выполняем запуск поддельного шлюза
	igd.start();
	// Создаём объект модуля перенаправления портов
	awh::unit::portmap_t portmap(this->_fmk.get(), this->_log.get());
	// Выполняем настройку модуля на поддельный шлюз
	::setup(portmap);
	// Выполняем ожидание итога обращения к устройству
	const outcome_t outcome = this->await(portmap, awh::unit::portmap_t::action_t::EXTERNAL);
	// Выполняем остановку поддельного шлюза
	igd.stop();
	// Выполняем проверку того, что устройство ответило
	ASSERT_TRUE(outcome.answered) << "код отказа " << static_cast <int32_t> (outcome.error);
	// Выполняем проверку того, что обращение отказом не завершилось
	ASSERT_FALSE(outcome.failed);
	// Выполняем проверку договора, по которому получен итог
	ASSERT_EQ(outcome.type, awh::unit::portmap_t::type_t::UPNP);
	// Выполняем проверку того, что действие службы было вызвано
	ASSERT_GT(igd.calls(), 0);
}

/**
 * @brief Проверка обхождения с отказами устройства UPnP
 *
 * @details Вносятся отказы, которых исправное устройство не даёт: молчание на рассылку,
 *          описание без единой пригодной службы, описание, разметкой не являющееся,
 *          отказ службы по правилам SOAP, отказ по правилам HTTP, ответ не разметкой и
 *          обрыв подключения посреди ответа. Обмен обязан завершиться отказом, а не
 *          остаться без итога вовсе
 *
 * @note Обрыв на каждом шаге проверяется отдельно: до ответа службы обмен успевает
 *       пройти обнаружение, чтение описания и вызов действия, и оборвать его можно
 *       в любой из этих точек
 *
 */
TEST_F(PortmapUnitFixture, PortmapUpnpFailures) {
	/**
	 * @brief Структура сличаемого обхождения с отказом устройства
	 *
	 */
	struct pair_t {
		// Вид отказа, вносимого поддельным шлюзом
		FakeIGD::Mode mode;
		// Признак того, что рассылка обнаружения отвергается
		bool silent;
		// Ожидаемый код причины отказа перенаправления
		awh::unit::portmap_t::error_t error;
	};
	/**
	 * Выполняем перебор отказов, вносимых поддельным шлюзом
	 */
	for(const pair_t & pair : {
		// Устройство на рассылку обнаружения не отвечает
		pair_t{FakeIGD::Mode::OK, true, awh::unit::portmap_t::error_t::NO_RESPONSE},
		// Описание устройства не содержит ни одной пригодной службы
		pair_t{FakeIGD::Mode::NO_SERVICE, false, awh::unit::portmap_t::error_t::NOT_SUPPORTED},
		// Описание устройства разметкой не является
		pair_t{FakeIGD::Mode::BAD_XML, false, awh::unit::portmap_t::error_t::MALFORMED},
		// Служба отвечает отказом по правилам SOAP
		pair_t{FakeIGD::Mode::FAULT, false, awh::unit::portmap_t::error_t::REFUSED},
		// Устройство отвечает отказом по правилам HTTP
		pair_t{FakeIGD::Mode::HTTP_ERROR, false, awh::unit::portmap_t::error_t::MALFORMED},
		// Ответ службы разметкой не является
		pair_t{FakeIGD::Mode::GARBAGE, false, awh::unit::portmap_t::error_t::MALFORMED},
		// Подключение обрывается на вызове действия службы
		pair_t{FakeIGD::Mode::DROP, false, awh::unit::portmap_t::error_t::NO_RESPONSE}
	}){
		// Создаём поддельный шлюз с поддержкой UPnP
		FakeIGD igd;
		// Если гнёзда поддельного шлюза завести не удалось, испытание не проводится
		if(!igd.ready()) GTEST_SKIP() << "SSDP port is occupied or the host has no local network address";
		// Устанавливаем вид отказа, вносимого поддельным шлюзом
		igd.mode = pair.mode;
		// Устанавливаем признак ответа на рассылку обнаружения
		igd.answerSearch = !pair.silent;
		// Выполняем запуск поддельного шлюза
		igd.start();
		// Создаём объект модуля перенаправления портов
		awh::unit::portmap_t portmap(this->_fmk.get(), this->_log.get());
		// Выполняем настройку модуля на поддельный шлюз
		::setup(portmap);
		// Выполняем ожидание итога обращения к устройству
		const outcome_t outcome = this->await(portmap, awh::unit::portmap_t::action_t::OPEN);
		// Выполняем остановку поддельного шлюза
		igd.stop();
		// Выполняем проверку того, что перенаправление заведённым не объявлено
		ASSERT_FALSE(outcome.answered) << "вид отказа " << static_cast <int32_t> (pair.mode);
		// Выполняем проверку того, что обмен завершился отказом, а не остался без итога
		ASSERT_TRUE(outcome.failed) << "вид отказа " << static_cast <int32_t> (pair.mode);
		// Выполняем проверку кода причины отказа перенаправления
		ASSERT_EQ(outcome.error, pair.error) << "вид отказа " << static_cast <int32_t> (pair.mode);
	}
}
