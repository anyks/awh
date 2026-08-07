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
	portmap.setIface(LOOPBACK_IFACE);
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

/**
 * @brief Проверка снятия и продления перенаправления по договору UPnP
 *
 * @details Действия эти разбираются отдельными ходами: снятие отвечает пустым телом, а
 *          продление заводит перенаправление заново. До обоих прежде не доходило ни одно
 *          испытание
 *
 */
TEST_F(PortmapUnitFixture, PortmapUpnpCloseAndRenew) {
	/**
	 * Выполняем перебор просьб, с которыми ведётся обращение
	 */
	for(const awh::unit::portmap_t::action_t action : {
		awh::unit::portmap_t::action_t::CLOSE, awh::unit::portmap_t::action_t::RENEW
	}){
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
		const outcome_t outcome = this->await(portmap, action);
		// Выполняем остановку поддельного шлюза
		igd.stop();
		// Выполняем проверку того, что устройство ответило
		ASSERT_TRUE(outcome.answered) << "просьба " << static_cast <int32_t> (action) << " код отказа " << static_cast <int32_t> (outcome.error);
		// Выполняем проверку того, что обращение отказом не завершилось
		ASSERT_FALSE(outcome.failed) << "просьба " << static_cast <int32_t> (action);
		// Выполняем проверку договора, по которому получен итог
		ASSERT_EQ(outcome.type, awh::unit::portmap_t::type_t::UPNP);
		// Выполняем проверку того, что действие службы было вызвано
		ASSERT_GT(igd.calls(), 0) << "просьба " << static_cast <int32_t> (action);
	}
}

/**
 * @brief Проверка чтения перечня заведённых перенаправлений
 *
 * @details Перечень выдаёт лишь договор UPnP, и читается он по порядковому номеру записи,
 *          пока служба не ответит отказом «номер вне перечня»: иного признака конца
 *          договор не даёт. Ход этот прежде не исполнялся ни одним испытанием
 *
 * @note Отказ по последнему номеру отказом обмена не считается: им перечень и
 *       заканчивается, и объявить его неудачей значило бы терять всё прочитанное
 *
 */
TEST_F(PortmapUnitFixture, PortmapUpnpList) {
	/**
	 * Выполняем перебор размеров перечня заведённых перенаправлений
	 */
	for(const uint32_t entries : {static_cast <uint32_t> (0), static_cast <uint32_t> (1), static_cast <uint32_t> (5)}){
		// Создаём поддельный шлюз с поддержкой UPnP
		FakeIGD igd;
		// Если гнёзда поддельного шлюза завести не удалось, испытание не проводится
		if(!igd.ready()) GTEST_SKIP() << "SSDP port is occupied or the host has no local network address";
		// Устанавливаем количество перенаправлений в выдаваемом перечне
		igd.entries = entries;
		// Выполняем запуск поддельного шлюза
		igd.start();
		// Создаём объект модуля перенаправления портов
		awh::unit::portmap_t portmap(this->_fmk.get(), this->_log.get());
		// Выполняем настройку модуля на поддельный шлюз
		::setup(portmap);
		// Выполняем ожидание итога обращения к устройству
		const outcome_t outcome = this->await(portmap, awh::unit::portmap_t::action_t::LIST);
		// Выполняем остановку поддельного шлюза
		igd.stop();
		// Выполняем проверку того, что устройство ответило
		ASSERT_TRUE(outcome.answered) << "записей " << entries << " код отказа " << static_cast <int32_t> (outcome.error);
		// Выполняем проверку того, что обращение отказом не завершилось
		ASSERT_FALSE(outcome.failed) << "записей " << entries;
		// Выполняем проверку количества прочитанных перенаправлений
		ASSERT_EQ(outcome.mappings.size(), static_cast <size_t> (entries));
		/**
		 * Выполняем перебор прочитанных перенаправлений
		 */
		for(size_t i = 0; i < outcome.mappings.size(); i++){
			// Выполняем проверку внутреннего порта прочитанного перенаправления
			ASSERT_EQ(outcome.mappings[i].internalPort, static_cast <uint16_t> (8000 + i)) << "запись " << i;
			// Выполняем проверку внешнего порта прочитанного перенаправления
			ASSERT_EQ(outcome.mappings[i].externalPort, static_cast <uint16_t> (40000 + i)) << "запись " << i;
			// Выполняем проверку срока жизни прочитанного перенаправления
			ASSERT_EQ(outcome.mappings[i].lifeTime, 3600u) << "запись " << i;
		}
	}
}

/**
 * @brief Проверка приведения кодов отказа службы UPnP
 *
 * @details Служба отвечает отказом по правилам SOAP, и код его приводится к коду причины
 *          отказа перенаправления. Приведение это исполнялось лишь на одном коде: отказ
 *          настоящего маршрутизатора по заказу не получить
 *
 * @note Проверяется и код, службе неизвестный: перечень кодов договором расширяется, и
 *       приниматься за успех новые коды не должны
 *
 */
TEST_F(PortmapUnitFixture, PortmapUpnpRefusal) {
	/**
	 * @brief Структура сличаемой пары кодов
	 *
	 */
	struct pair_t {
		// Код отказа, выданный службой
		uint32_t fault;
		// Ожидаемый код причины отказа перенаправления
		awh::unit::portmap_t::error_t error;
	};
	/**
	 * Выполняем перебор кодов отказа, выдаваемых службой
	 */
	for(const pair_t & pair : {
		pair_t{0x191, awh::unit::portmap_t::error_t::NOT_SUPPORTED},    // Действие службе неизвестно
		pair_t{0x192, awh::unit::portmap_t::error_t::MALFORMED},        // Доводы вызова построены ошибочно
		pair_t{0x25E, awh::unit::portmap_t::error_t::NOT_AUTHORIZED},   // Отвергнуто настройкой маршрутизатора
		pair_t{0x2D8, awh::unit::portmap_t::error_t::OUT_OF_RESOURCES}, // Не осталось места под перенаправления
		pair_t{0x2C1, awh::unit::portmap_t::error_t::NOT_SUPPORTED},    // Договор пробоя не поддерживается
		pair_t{0x2CE, awh::unit::portmap_t::error_t::REFUSED},          // Перенаправление занято другой машиной
		pair_t{0x3E7, awh::unit::portmap_t::error_t::REFUSED}           // Код отказа службе неизвестен
	}){
		// Создаём поддельный шлюз с поддержкой UPnP
		FakeIGD igd;
		// Если гнёзда поддельного шлюза завести не удалось, испытание не проводится
		if(!igd.ready()) GTEST_SKIP() << "SSDP port is occupied or the host has no local network address";
		// Устанавливаем выдачу отказа службы
		igd.mode = FakeIGD::Mode::FAULT;
		// Устанавливаем код отказа, выдаваемый службой
		igd.fault = pair.fault;
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
		ASSERT_FALSE(outcome.answered) << "код отказа службы " << pair.fault;
		// Выполняем проверку того, что обращение завершилось отказом
		ASSERT_TRUE(outcome.failed) << "код отказа службы " << pair.fault;
		// Выполняем проверку приведённого кода причины отказа перенаправления
		ASSERT_EQ(outcome.error, pair.error) << "код отказа службы " << pair.fault;
	}
}

/**
 * @brief Проверка повтора шага обмена по молчанию устройства
 *
 * @details Устройство принимает вызов действия и держит подключение открытым, ничего не
 *          отвечая. Подключение при этом живо и само по себе не оборвётся: движок заводит
 *          новую попытку по обрыву, а не по молчанию, - и не повтори модуль запрос по
 *          истечении срока, обмен остался бы без итога вовсе, ни ответа, ни отказа
 *
 * @note Проверяется именно повтор, а не отказ: до маршрутизатора вызов доходит столько
 *       раз, сколько отведено попыток, и лишь затем обмен завершается неответом
 *
 */
TEST_F(PortmapUnitFixture, PortmapUpnpRepeat) {
	// Создаём поддельный шлюз с поддержкой UPnP
	FakeIGD igd;
	// Если гнёзда поддельного шлюза завести не удалось, испытание не проводится
	if(!igd.ready()) GTEST_SKIP() << "SSDP port is occupied or the host has no local network address";
	// Устанавливаем оставление вызова действия без ответа
	igd.mode = FakeIGD::Mode::STALL;
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
	ASSERT_FALSE(outcome.answered);
	// Выполняем проверку того, что обмен завершился отказом, а не остался без итога
	ASSERT_TRUE(outcome.failed);
	// Выполняем проверку кода причины отказа перенаправления
	ASSERT_EQ(outcome.error, awh::unit::portmap_t::error_t::NO_RESPONSE);
	// Выполняем проверку того, что вызов действия повторялся по числу отведённых попыток
	ASSERT_GE(igd.calls(), 2);
}

/**
 * @brief Метод настройки модуля на поддельный шлюз сетью IPv6
 *
 * @details Обмен ведётся устройством петли и разновидностью IPv6: рассылка обнаружения
 *          уходит в отведённую договором группу связи, а описание устройства объявляется
 *          адресом машины в местной сети IPv6 - петля под заслон модуля не проходит
 *
 * @param portmap объект модуля перенаправления портов
 *
 */
static void setup6(awh::unit::portmap_t & portmap) noexcept {
	// Выполняем настройку модуля на поддельный шлюз
	::setup(portmap);
	// Устанавливаем разновидность сети, которой ведётся обмен
	portmap.setFamily(awh::unit::portmap_t::family_t::IPV6);
}

/**
 * @brief Проверка проделывания пробоя заслона IPv6
 *
 * @details Сеть IPv6 перенаправления портов не знает вовсе: адреса в ней у каждой машины
 *          свои, и наружу они видны как есть, - а закрывает их заслон устройства доступа.
 *          Открывается путь пробоем заслона, и просьба эта идёт другой службой, другим
 *          действием и с другими доводами. Ход этот прежде проверялся лишь вручную на
 *          живом устройстве
 *
 * @note Проделыванию предшествует спрос о состоянии заслона: он бывает отключён либо
 *       пробои им запрещены, и просить о пробое тогда бесполезно. Оттого действий службы
 *       здесь два, а не одно
 *
 */
TEST_F(PortmapUnitFixture, PortmapUpnpPinhole) {
	// Создаём поддельный шлюз с поддержкой UPnP
	FakeIGD igd;
	// Если обмен сетью IPv6 машине недоступен, испытание не проводится
	if(!igd.ready6()) GTEST_SKIP() << "the host has no local IPv6 network address";
	// Выполняем запуск поддельного шлюза
	igd.start();
	// Создаём объект модуля перенаправления портов
	awh::unit::portmap_t portmap(this->_fmk.get(), this->_log.get());
	// Выполняем настройку модуля на поддельный шлюз сетью IPv6
	::setup6(portmap);
	// Выполняем ожидание итога обращения к устройству
	const outcome_t outcome = this->await(portmap, awh::unit::portmap_t::action_t::OPEN);
	// Выполняем остановку поддельного шлюза
	igd.stop();
	// Выполняем проверку того, что рассылка обнаружения дошла до поддельного шлюза
	ASSERT_GT(igd.searches(), 0) << "рассылка ушла мимо устройства петли";
	// Выполняем проверку того, что устройство ответило
	ASSERT_TRUE(outcome.answered) << "код отказа " << static_cast <int32_t> (outcome.error);
	// Выполняем проверку того, что обращение отказом не завершилось
	ASSERT_FALSE(outcome.failed);
	// Выполняем проверку того, что выдан опознаватель проделанного пробоя
	ASSERT_EQ(outcome.mapping.pinhole, 4242u);
	// Выполняем проверку того, что действий службы было два: спрос о заслоне и пробой
	ASSERT_GE(igd.calls(), 2);
}

/**
 * @brief Проверка заделывания и продления пробоя заслона IPv6
 *
 * @details Пробой заделывается и продлевается по опознавателю, выданному при его
 *          проделывании, и оба действия идут службой заслона, а не перенаправления портов
 *
 */
TEST_F(PortmapUnitFixture, PortmapUpnpPinholeCloseAndRenew) {
	/**
	 * Выполняем перебор просьб, с которыми ведётся обращение
	 */
	for(const awh::unit::portmap_t::action_t action : {
		awh::unit::portmap_t::action_t::CLOSE, awh::unit::portmap_t::action_t::RENEW
	}){
		// Создаём поддельный шлюз с поддержкой UPnP
		FakeIGD igd;
		// Если обмен сетью IPv6 машине недоступен, испытание не проводится
		if(!igd.ready6()) GTEST_SKIP() << "the host has no local IPv6 network address";
		// Выполняем запуск поддельного шлюза
		igd.start();
		// Создаём объект модуля перенаправления портов
		awh::unit::portmap_t portmap(this->_fmk.get(), this->_log.get());
		// Выполняем настройку модуля на поддельный шлюз сетью IPv6
		::setup6(portmap);
		// Выполняем ожидание итога обращения к устройству
		const outcome_t outcome = this->await(portmap, action);
		// Выполняем остановку поддельного шлюза
		igd.stop();
		// Выполняем проверку того, что устройство ответило
		ASSERT_TRUE(outcome.answered) << "просьба " << static_cast <int32_t> (action) << " код отказа " << static_cast <int32_t> (outcome.error);
		// Выполняем проверку того, что обращение отказом не завершилось
		ASSERT_FALSE(outcome.failed) << "просьба " << static_cast <int32_t> (action);
		// Выполняем проверку того, что действие службы было вызвано
		ASSERT_GT(igd.calls(), 0) << "просьба " << static_cast <int32_t> (action);
	}
}

/**
 * @brief Проверка предварительного спроса о состоянии заслона IPv6
 *
 * @details Спрос предшествует проделыванию пробоя и бережёт от заведомо бесполезной
 *          просьбы: у отключённого заслона пробои не нужны вовсе, а запрещённые он не
 *          проделает. Отказ по спросу обязан прийти прежде, чем просьба о пробое уйдёт
 *
 * @note Обе беды разводятся отдельными случаями: заслон отключён и пробои запрещены -
 *       разные состояния устройства, и оба даются ему настройкой
 *
 */
TEST_F(PortmapUnitFixture, PortmapUpnpFirewall) {
	/**
	 * @brief Структура сличаемого состояния заслона
	 *
	 */
	struct pair_t {
		// Признак того, что заслон IPv6 включён
		bool firewall;
		// Признак того, что пробои заслона IPv6 дозволены
		bool pinholes;
	};
	/**
	 * Выполняем перебор состояний заслона устройства
	 */
	for(const pair_t & pair : {
		pair_t{false, false}, // Заслон отключён и пробои запрещены
		pair_t{false, true},  // Заслон отключён, пробои дозволены
		pair_t{true, false}   // Заслон включён, пробои запрещены
	}){
		// Создаём поддельный шлюз с поддержкой UPnP
		FakeIGD igd;
		// Если обмен сетью IPv6 машине недоступен, испытание не проводится
		if(!igd.ready6()) GTEST_SKIP() << "the host has no local IPv6 network address";
		// Устанавливаем состояние заслона устройства
		igd.firewall = pair.firewall;
		// Устанавливаем дозволенность пробоев заслона устройства
		igd.pinholes = pair.pinholes;
		// Выполняем запуск поддельного шлюза
		igd.start();
		// Создаём объект модуля перенаправления портов
		awh::unit::portmap_t portmap(this->_fmk.get(), this->_log.get());
		// Выполняем настройку модуля на поддельный шлюз сетью IPv6
		::setup6(portmap);
		// Выполняем ожидание итога обращения к устройству
		const outcome_t outcome = this->await(portmap, awh::unit::portmap_t::action_t::OPEN);
		// Выполняем остановку поддельного шлюза
		igd.stop();
		// Выполняем проверку того, что пробой проделанным не объявлен
		ASSERT_FALSE(outcome.answered) << "заслон " << pair.firewall << " пробои " << pair.pinholes;
		// Выполняем проверку того, что обращение завершилось отказом
		ASSERT_TRUE(outcome.failed) << "заслон " << pair.firewall << " пробои " << pair.pinholes;
		// Выполняем проверку кода причины отказа перенаправления
		ASSERT_EQ(outcome.error, awh::unit::portmap_t::error_t::NOT_SUPPORTED) << "заслон " << pair.firewall << " пробои " << pair.pinholes;
		// Выполняем проверку того, что действие службы было вызвано лишь одно
		ASSERT_EQ(igd.calls(), 1) << "заслон " << pair.firewall << " пробои " << pair.pinholes;
	}
}
