/**
 * @file secrets.cpp
 * @date 2026-08-22
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
 * @brief Тесты склада тайн: укладка, взятие рукоятью и снятие
 *
 * @copyright Copyright © 2026
 *
 */

/**
 * Подключаем заголовочный файлы проекта
 */
#include "vault.hpp"

/**
 * Подключаем наши модули
 */
#include "../../../include/alloc/alloc.hpp"

/**
 * @brief Тест заведения склада
 *
 */
TEST_F(VaultFixture, VaultIsPrepared){
	// Склад обязан завестись: ключ его берётся случайным при заведении
	ASSERT_TRUE(this->_vault->ready());
	// Пустой склад тайн не содержит
	EXPECT_EQ(this->_vault->count(), static_cast <size_t> (0));
}
/**
 * @brief Тест укладки и взятия тайны
 *
 */
TEST_F(VaultFixture, StoredSecretIsBorrowedBack){
	// Содержимое тайны
	const std::string secret = "ключ подписи, какому в общей куче лежать не положено";
	// Укладываем тайну на склад
	ASSERT_TRUE(this->_vault->store("signature", secret.data(), secret.size()));
	// Тайна обязана оказаться на складе
	EXPECT_TRUE(this->_vault->has("signature"));
	EXPECT_EQ(this->_vault->count(), static_cast <size_t> (1));
	// Берём тайну рукоятью
	awh::vault_t::Handle handle = this->_vault->borrow("signature");
	// Взятие обязано удаться
	ASSERT_TRUE(handle.valid());
	// Содержимое обязано совпасть с уложенным
	ASSERT_EQ(handle.size(), secret.size());
	EXPECT_EQ(::memcmp(handle.data(), secret.data(), secret.size()), 0);
	/**
	 * Открытый текст обязан лежать в памяти НАШЕГО распределителя
	 *
	 * Обычное хранилище языка отдало бы его в общую кучу - оттуда он ушёл бы в
	 * подкачку и попал бы в снимок памяти при падении
	 */
	EXPECT_GE(awh::alloc::Allocator::resolve(handle.data()).size, handle.size());
}
/**
 * @brief Тест хранения тайны в шифрованном виде
 *
 * @note Склад, хранящий открытый текст, отличить от рабочего можно лишь так: содержимое
 *       обязано быть НЕ найдено в памяти склада
 *
 */
TEST_F(VaultFixture, StoredSecretIsNotPlain){
	// Содержимое тайны, заметное собою
	const std::string secret = "ОЧЕНЬ-ЗАМЕТНАЯ-ТАЙНА-0123456789";
	// Укладываем тайну на склад
	ASSERT_TRUE(this->_vault->store("token", secret.data(), secret.size()));
	// Снимаем шифротекст тайны
	std::vector <char> cipher;
	ASSERT_TRUE(this->_vault->sealed("token", cipher));
	// Шифротекст обязан быть непуст
	ASSERT_FALSE(cipher.empty());
	/**
	 * Открытого текста в шифротексте быть не должно
	 *
	 * Проверяется это поиском, а не доверием: склад, забывший зашифровать, отличить
	 * от рабочего иначе нельзя - рукоять и там, и там отдаёт верное содержимое
	 */
	EXPECT_EQ(std::string(cipher.data(), cipher.size()).find(secret), std::string::npos);
	/**
	 * Две укладки одного содержимого обязаны дать РАЗНЫЙ шифротекст
	 *
	 * Совпадение их означало бы шифрование без вектора инициализации: одинаковые
	 * тайны выдавали бы себя равенством шифротекста
	 */
	ASSERT_TRUE(this->_vault->store("first", secret.data(), secret.size()));
	ASSERT_TRUE(this->_vault->store("second", secret.data(), secret.size()));
	// Снимаем шифротекст обеих тайн
	std::vector <char> left, right;
	ASSERT_TRUE(this->_vault->sealed("first", left));
	ASSERT_TRUE(this->_vault->sealed("second", right));
	// Шифротексты одного содержимого обязаны различаться
	EXPECT_NE(std::string(left.data(), left.size()), std::string(right.data(), right.size()));
	// Открытый текст обеих тайн при этом обязан совпасть
	awh::vault_t::Handle first = this->_vault->borrow("first");
	awh::vault_t::Handle second = this->_vault->borrow("second");
	ASSERT_TRUE(first.valid() && second.valid());
	EXPECT_EQ(std::string(first.data(), first.size()), std::string(second.data(), second.size()));
}
/**
 * @brief Тест взятия неизвестной тайны
 *
 */
TEST_F(VaultFixture, UnknownSecretGivesInvalidHandle){
	// Взятие неизвестной тайны обязано дать негодную рукоять, а не пустую годную
	awh::vault_t::Handle handle = this->_vault->borrow("unknown");
	EXPECT_FALSE(handle.valid());
	EXPECT_EQ(handle.size(), static_cast <size_t> (0));
	EXPECT_EQ(handle.data(), nullptr);
}
/**
 * @brief Тест укладки пустой тайны
 *
 * @note Пустая тайна - законное содержимое, и судить о взятии по размеру нельзя: ровно
 *       ради этого у рукояти есть признак годности
 *
 */
TEST_F(VaultFixture, EmptySecretIsLegitimate){
	// Укладываем пустую тайну на склад
	ASSERT_TRUE(this->_vault->store("empty", nullptr, 0));
	// Берём пустую тайну рукоятью
	awh::vault_t::Handle handle = this->_vault->borrow("empty");
	// Взятие обязано удаться, а размер остаться нулевым
	EXPECT_TRUE(handle.valid());
	EXPECT_EQ(handle.size(), static_cast <size_t> (0));
}
/**
 * @brief Тест снятия тайны со склада
 *
 */
TEST_F(VaultFixture, ErasedSecretIsGone){
	// Содержимое тайны
	const std::string secret = "пароль";
	// Укладываем тайну на склад
	ASSERT_TRUE(this->_vault->store("password", secret.data(), secret.size()));
	// Снимаем тайну со склада
	EXPECT_TRUE(this->_vault->erase("password"));
	// Снятая тайна на складе больше не значится
	EXPECT_FALSE(this->_vault->has("password"));
	EXPECT_EQ(this->_vault->count(), static_cast <size_t> (0));
	// Повторное снятие обязано ответить отказом, а не успехом
	EXPECT_FALSE(this->_vault->erase("password"));
	// Взятие снятой тайны обязано дать негодную рукоять
	EXPECT_FALSE(this->_vault->borrow("password").valid());
}
/**
 * @brief Тест переноса рукояти
 *
 * @note Рукоять переносима, но не копируема: копия открытого текста тайны в памяти
 *       означала бы вторую его жизнь, о какой звавший не знает
 *
 */
TEST_F(VaultFixture, HandleIsMovableOnly){
	// Содержимое тайны
	const std::string secret = "переносимая тайна";
	// Укладываем тайну на склад
	ASSERT_TRUE(this->_vault->store("moved", secret.data(), secret.size()));
	// Берём тайну рукоятью
	awh::vault_t::Handle source = this->_vault->borrow("moved");
	ASSERT_TRUE(source.valid());
	// Переносим рукоять
	awh::vault_t::Handle target(std::move(source));
	// Содержимое обязано перейти к новой рукояти
	EXPECT_TRUE(target.valid());
	EXPECT_EQ(std::string(target.data(), target.size()), secret);
	// Отданная рукоять обязана стать негодной
	EXPECT_FALSE(source.valid());
	EXPECT_EQ(source.size(), static_cast <size_t> (0));
	// Копировать рукоять язык не позволяет вовсе
	EXPECT_FALSE(std::is_copy_constructible <awh::vault_t::Handle>::value);
	EXPECT_FALSE(std::is_copy_assignable <awh::vault_t::Handle>::value);
}
