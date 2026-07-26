/**
 * @file: parameterized.cpp
 * @date: 2026-01-26
 * @license: LicenseRef-AWH-1.0
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
 * Подключаем заголовочный файл
 */
#include "holder.hpp"

/**
 * @brief Структура параметров теста
 *
 */
struct HoldTestParameter {
	HolderFixture::status_t status;
	std::unordered_set <HolderFixture::status_t> items;
};

/**
 * @brief Класс параметризованного теста холдера
 *
 */
class HoldParameterizedFixture : public HolderFixture, public ::testing::WithParamInterface <HoldTestParameter> {
	public:
		HoldTestParameter _parameter = GetParam();
};

/**
 * @brief Тестирование холдера с параметрами
 *
 */
TEST_P(HoldParameterizedFixture, HoldTest){
	// Создаём объект холдера
	awh::holder_t <HolderFixture::status_t> holder(this->_status, this->_mtx);
	ASSERT_TRUE(holder.access(this->_parameter.items, this->_parameter.status));
}

/**
 * @brief Инициализация параметров теста
 *
 */
INSTANTIATE_TEST_SUITE_P(TestParameters, HoldParameterizedFixture,
	::testing::Values(
		HoldTestParameter({HolderFixture::status_t::STATUS1, {}}),
		HoldTestParameter({HolderFixture::status_t::STATUS2, {HolderFixture::status_t::STATUS1}}),
		HoldTestParameter({HolderFixture::status_t::STATUS3, {HolderFixture::status_t::STATUS1, HolderFixture::status_t::STATUS2}}),
		HoldTestParameter({HolderFixture::status_t::STATUS4, {HolderFixture::status_t::STATUS1, HolderFixture::status_t::STATUS2, HolderFixture::status_t::STATUS3}})
	)
);
