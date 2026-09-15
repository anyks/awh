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
 * Стандартные заголовочные файлы
 */
#include <atomic>
#include <thread>
#include <stdexcept>

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
/**
 * @brief Тест перезаписи тайны на складе
 *
 * @note Утверждается здесь ЗАМЕНА, а не затирание прежнего шифротекста: затирание
 *       следствий наружу не даёт вовсе - взятие отдаёт новое содержимое и с ним, и
 *       без него, - и проверка эта, проведённая мутацией, снятие затирания переживает.
 *       Затирание прежнего шифротекста при перезаписи держится на разборе кода, а не
 *       на этой проверке; закрывающей проверки у него нет
 *
 */
TEST_F(VaultFixture, RewrittenSecretReplacesTheFormerOne){
	// Прежнее содержимое тайны
	const std::string former = "прежний пароль";
	// Новое содержимое тайны
	const std::string latter = "новый пароль, какому прежний уступает место";
	// Укладываем прежнюю тайну на склад
	ASSERT_TRUE(this->_vault->store("password", former.data(), former.size()));
	// Снимаем шифротекст прежней тайны
	std::vector <char> before;
	ASSERT_TRUE(this->_vault->sealed("password", before));
	// Перезаписываем тайну новым содержимым
	ASSERT_TRUE(this->_vault->store("password", latter.data(), latter.size()));
	// Число тайн на складе от перезаписи не растёт
	EXPECT_EQ(this->_vault->count(), static_cast <size_t> (1));
	// Взятие обязано отдать НОВОЕ содержимое, а не прежнее
	awh::vault_t::Handle handle = this->_vault->borrow("password");
	ASSERT_TRUE(handle.valid());
	EXPECT_EQ(std::string(handle.data(), handle.size()), latter);
	// Шифротекст обязан смениться целиком
	std::vector <char> after;
	ASSERT_TRUE(this->_vault->sealed("password", after));
	EXPECT_NE(std::string(after.data(), after.size()), std::string(before.data(), before.size()));
}
/**
 * @brief Тест сведений о состоявшейся защите памяти склада
 *
 * @note Проверяется здесь не сама защита - обещания у систем разные, и утверждать
 *       `wired` значило бы валить проверку там, где система права такого не даёт, - а
 *       то, что склад об этом СПРАШИВАЕТ: не спроси он, оба признака остались бы
 *       ложными на всякой системе, и молчаливое понижение защиты было бы неотличимо
 *       от честного
 *
 */
TEST_F(VaultFixture, VaultReportsAchievedShelter){
	// Снимаем сведения о состоявшейся защите
	const awh::alloc::shelter_t & shelter = this->_vault->shelter();
	// Сведения обязаны совпасть с тем, что отвечает сам распределитель
	awh::alloc::shelter_t expected;
	void * probe = awh::alloc::Allocator::secure(64, &expected);
	ASSERT_NE(probe, nullptr);
	awh::alloc::Allocator::release(probe);
	// Склад обязан отвечать то же, что и распределитель
	EXPECT_EQ(shelter.hidden, expected.hidden);
	EXPECT_EQ(shelter.wired, expected.wired);
	/**
	 * Хоть одна защита обязана состояться на заявленных системах
	 *
	 * Укрытия от снимка нет у Linux, macOS и NetBSD, а запрет подкачки требует прав у
	 * illumos: порознь каждый признак вправе быть ложным, но оба ложных разом означали
	 * бы, что укрытая выдача не даёт ничего сверх обычной
	 */
	EXPECT_TRUE(shelter.hidden || shelter.wired);
	// Защита памяти к готовности склада отношения не имеет: шифрование состоится и без неё
	EXPECT_TRUE(this->_vault->ready());
}
/**
 * @brief Тест обращения к содержимому тайны через обработчик
 *
 * @note Обработчик получает содержимое на время вызова и наружу его не выносит: так
 *       граница договора перестаёт быть местом, где открытый текст сам себя отдаёт
 *
 */
TEST_F(VaultFixture, HandleAppliesContentToHandler){
	// Содержимое тайны
	const std::string secret = "ключ, какой за границу договора выходить не должен";
	// Укладываем тайну на склад
	ASSERT_TRUE(this->_vault->store("applied", secret.data(), secret.size()));
	// Берём тайну рукоятью
	awh::vault_t::Handle handle = this->_vault->borrow("applied");
	// Взятие обязано удаться
	ASSERT_TRUE(handle.valid());
	// Содержимое, снятое обработчиком
	std::string seen;
	// Число вызовов обработчика
	size_t calls = 0;
	// Обращаемся к содержимому тайны
	EXPECT_TRUE(handle.apply([&seen, &calls](const char * data, const size_t size) noexcept -> void {
		// Считаем вызов обработчика
		calls++;
		// Снимаем содержимое тайны
		seen.assign(data, size);
	}));
	// Обработчик обязан быть зван ровно однажды
	EXPECT_EQ(calls, static_cast <size_t> (1));
	// Содержимое обязано совпасть с уложенным
	EXPECT_EQ(seen, secret);
}
/**
 * @brief Тест обращения к содержимому негодной рукояти
 *
 * @note Отказ здесь обязан быть ЗАМЕТЕН вызывающему, а обработчик не зван вовсе:
 *       негодная рукоять содержимого не имеет, и подать ему нечего
 *
 */
TEST_F(VaultFixture, HandleAppliesNothingWhenInvalid){
	// Берём со склада тайну, которой там нет
	awh::vault_t::Handle handle = this->_vault->borrow("отсутствующая");
	// Взятие обязано не удаться
	ASSERT_FALSE(handle.valid());
	// Число вызовов обработчика
	size_t calls = 0;
	// Обращение обязано ответить отказом
	EXPECT_FALSE(handle.apply([&calls](const char *, const size_t) noexcept -> void {
		// Считаем вызов обработчика
		calls++;
	}));
	// Обработчик обязан быть не зван вовсе
	EXPECT_EQ(calls, static_cast <size_t> (0));
}
/**
 * @brief Тест обращения к содержимому пустой тайны
 *
 * @note Пустая тайна - законное содержимое, и судить о взятии по длине нельзя: обращение
 *       обязано состояться, а обработчик получить длину нуль
 *
 */
TEST_F(VaultFixture, HandleAppliesEmptySecret){
	// Укладываем пустую тайну на склад
	ASSERT_TRUE(this->_vault->store("пустая", "", 0));
	// Берём тайну рукоятью
	awh::vault_t::Handle handle = this->_vault->borrow("пустая");
	// Взятие обязано удаться
	ASSERT_TRUE(handle.valid());
	// Длина, снятая обработчиком
	size_t length = 1;
	// Число вызовов обработчика
	size_t calls = 0;
	// Обращение обязано состояться
	EXPECT_TRUE(handle.apply([&length, &calls](const char *, const size_t size) noexcept -> void {
		// Считаем вызов обработчика
		calls++;
		// Снимаем длину содержимого
		length = size;
	}));
	// Обработчик обязан быть зван ровно однажды
	EXPECT_EQ(calls, static_cast <size_t> (1));
	// Длина содержимого обязана быть нулевой
	EXPECT_EQ(length, static_cast <size_t> (0));
}
/**
 * @brief Тест ухода из обработчика исключением
 *
 * @note Утверждается здесь СОСТОЯНИЕ рукояти после ухода, а не то, что программа не
 *       упала: пометка `noexcept` на обращении обратила бы уход в аварийный останов
 *       ПРЕЖДЕ раскрутки, и проверка, судящая по «не упало», такого не поймала бы вовсе
 *
 */
TEST_F(VaultFixture, HandleStaysUsableWhenHandlerThrows){
	// Содержимое тайны
	const std::string secret = "тайна, переживающая уход обработчика исключением";
	// Укладываем тайну на склад
	ASSERT_TRUE(this->_vault->store("throwing", secret.data(), secret.size()));
	// Берём тайну рукоятью
	awh::vault_t::Handle handle = this->_vault->borrow("throwing");
	// Взятие обязано удаться
	ASSERT_TRUE(handle.valid());
	// Признак ушедшего наружу исключения
	bool thrown = false;
	/**
	 * Уходим из обработчика исключением
	 */
	try {
		// Обращаемся к содержимому тайны
		static_cast <void> (handle.apply([](const char *, const size_t) -> void {
			// Уходим из обработчика исключением
			throw std::runtime_error("уход из обработчика");
		}));
	/**
	 * Если возникает ошибка
	 */
	} catch(const std::runtime_error & error) {
		// Запоминаем признак ушедшего наружу исключения
		thrown = true;
		// Исключение обязано уйти наружу нетронутым
		EXPECT_EQ(std::string(error.what()), std::string("уход из обработчика"));
	}
	// Исключение обязано уйти наружу
	ASSERT_TRUE(thrown);
	/**
	 * Состояние рукояти после ухода исключением
	 *
	 * Рукоять обязана остаться целой, а содержимое - доступным вновь: запечатывание
	 * и затирание ведутся деструктором, и уход исключением их порядка не меняет
	 */
	EXPECT_TRUE(handle.valid());
	// Содержимое, снятое обработчиком после ухода исключением
	std::string seen;
	// Обращение обязано состояться вновь
	EXPECT_TRUE(handle.apply([&seen](const char * data, const size_t size) noexcept -> void {
		// Снимаем содержимое тайны
		seen.assign(data, size);
	}));
	// Содержимое обязано совпасть с уложенным
	EXPECT_EQ(seen, secret);
}
/**
 * @brief Тест укладки тайны из приёмника тайн
 *
 * @note Смысл хода в том, чтобы содержимое НЕ проходило через вместилище языка: всякое
 *       перевыделение `std::string` оставляет в куче копию, какую никто не затирает
 *
 */
TEST_F(VaultFixture, SecretIsStoredOutOfVessel){
	// Содержимое тайны, вводимое побайтно
	const std::string secret = "пароль, набираемый с терминала посимвольно";
	// Заводим приёмник тайн под содержимое
	awh::alloc::vessel_t vessel;
	// Приёмник обязан завестись
	ASSERT_TRUE(vessel.reserve(secret.size()));
	/**
	 * Перебираем октеты содержимого
	 */
	for(size_t i = 0; i < secret.size(); i++)
		// Принимаем очередной октет содержимого
		ASSERT_TRUE(vessel.pour(static_cast <uint8_t> (secret.at(i))));
	// Занятое в приёмнике обязано совпасть с уложенным
	ASSERT_EQ(vessel.size(), secret.size());
	// Укладываем тайну на склад прямо из приёмника
	ASSERT_TRUE(this->_vault->store("poured", vessel));
	// Тайна обязана оказаться на складе
	EXPECT_TRUE(this->_vault->has("poured"));
	// Берём тайну рукоятью
	awh::vault_t::Handle handle = this->_vault->borrow("poured");
	// Взятие обязано удаться
	ASSERT_TRUE(handle.valid());
	// Содержимое, снятое обработчиком
	std::string seen;
	// Обращаемся к содержимому тайны
	ASSERT_TRUE(handle.apply([&seen](const char * data, const size_t size) noexcept -> void {
		// Снимаем содержимое тайны
		seen.assign(data, size);
	}));
	// Содержимое обязано совпасть с влитым в приёмник
	EXPECT_EQ(seen, secret);
	/**
	 * Приёмник после укладки НЕ затирается сам
	 *
	 * Распоряжается им заведший: держать тайну дольше надобности либо отдать её вновь -
	 * его решение, а не склада
	 */
	EXPECT_EQ(vessel.size(), secret.size());
}
/**
 * @brief Тест укладки тайны из незаведённого приёмника
 *
 * @note Отказ здесь обязан быть ЗАМЕТЕН: незаведённый приёмник содержимого не имеет, и
 *       уложить на склад пустоту вместо тайны значило бы солгать о состоявшейся укладке
 *
 */
TEST_F(VaultFixture, EmptyVesselStoresNothing){
	// Заводим приёмник тайн, ёмкости ему не задавая
	awh::alloc::vessel_t vessel;
	// Укладка обязана ответить отказом
	EXPECT_FALSE(this->_vault->store("неоткуда", vessel));
	// Тайна на складе оказаться не обязана
	EXPECT_FALSE(this->_vault->has("неоткуда"));
}
/**
 * @brief Тест строгости склада умолчанием
 *
 * @note Проверяется здесь не сама защита, а то, что склад СУДИТ по состоявшемуся: где
 *       запрет подкачки состоялся, строгий склад заводится, а где не состоялся -
 *       отвечает отказом. Послабленный же работает и там, лишь обещая меньше
 *
 */
TEST_F(VaultFixture, StrictVaultJudgesByAchievedShelter){
	// Заводим склад умолчанием
	awh::vault_t strict;
	// Заводим склад послабленным
	awh::vault_t relaxed(awh::alloc::secrecy_t::RELAXED);
	/**
	 * Послабленный склад заводится ВСЕГДА
	 *
	 * Обещания у систем разные, и отказывать из-за них значило бы закрыть склад там,
	 * где защита лишь ниже, а не отсутствует
	 */
	ASSERT_TRUE(relaxed.ready());
	// Строгость судится по запрету подкачки, состоявшемуся НА ДЕЛЕ
	EXPECT_EQ(strict.ready(), relaxed.shelter().wired);
	/**
	 * Если запрет подкачки состоялся
	 */
	if(relaxed.shelter().wired){
		// Строгий склад обязан работать наравне с послабленным
		ASSERT_TRUE(strict.ready());
		// Содержимое тайны
		const std::string secret = "тайна строгого склада";
		// Укладываем тайну на строгий склад
		ASSERT_TRUE(strict.store("strict", secret.data(), secret.size()));
		// Берём тайну рукоятью
		awh::vault_t::Handle handle = strict.borrow("strict");
		// Взятие обязано удаться
		EXPECT_TRUE(handle.valid());
	/**
	 * Если запрета подкачки система не даёт
	 */
	} else {
		// Строгий склад обязан отказаться работать целиком
		EXPECT_FALSE(strict.ready());
		// Укладка на незаведённый склад обязана ответить отказом
		EXPECT_FALSE(strict.store("strict", "тайна", 5));
	}
}
/**
 * @brief Тест склада, общего на процесс
 *
 * @note Общий склад один на всю программу: заводится при первом обращении и тайны
 *       переживают возврат из хода, его выдавшего
 *
 */
TEST_F(VaultFixture, SharedVaultIsOneForTheProcess){
	// Содержимое тайны
	const std::string secret = "тайна общего склада";
	// Укладываем тайну на общий склад
	ASSERT_TRUE(awh::vault_t::shared().ready());
	ASSERT_TRUE(awh::vault_t::shared().store("общая", secret.data(), secret.size()));
	/**
	 * Склад обязан быть ТЕМ ЖЕ при повторном обращении
	 *
	 * Сличаются здесь адреса, а не содержимое: склад, заводимый заново на всякое
	 * обращение, тайну тоже отдал бы - но лишь потому, что мы её только что положили
	 */
	EXPECT_EQ(&awh::vault_t::shared(), &awh::vault_t::shared());
	// Тайна обязана лежать на складе
	EXPECT_TRUE(awh::vault_t::shared().has("общая"));
	// Берём тайну рукоятью
	awh::vault_t::Handle handle = awh::vault_t::shared().borrow("общая");
	// Взятие обязано удаться
	ASSERT_TRUE(handle.valid());
	// Содержимое, снятое обработчиком
	std::string seen;
	// Обращаемся к содержимому тайны
	ASSERT_TRUE(handle.apply([&seen](const char * data, const size_t size) noexcept -> void {
		// Снимаем содержимое тайны
		seen.assign(data, size);
	}));
	// Содержимое обязано совпасть с уложенным
	EXPECT_EQ(seen, secret);
	// Снимаем тайну со склада: склад общий и переживёт эту проверку
	EXPECT_TRUE(awh::vault_t::shared().erase("общая"));
}
/**
 * @brief Тест склада, своего у каждого потока
 *
 * @note Тайны, уложенные одним потоком, другому НЕ видны вовсе: это разные склады с
 *       разными ключами, а не один склад под разными замками
 *
 */
TEST_F(VaultFixture, LocalVaultIsNotSharedBetweenThreads){
	// Содержимое тайны
	const std::string secret = "тайна своего склада";
	// Укладываем тайну на свой склад
	ASSERT_TRUE(awh::vault_t::local().ready());
	ASSERT_TRUE(awh::vault_t::local().store("своя", secret.data(), secret.size()));
	// Тайна обязана лежать на своём складе
	ASSERT_TRUE(awh::vault_t::local().has("своя"));
	// Адрес склада текущего потока
	const awh::vault_t * ours = &awh::vault_t::local();
	// Адрес склада стороннего потока
	const awh::vault_t * theirs = nullptr;
	// Признак видимости нашей тайны стороннему потоку
	bool visible = true;
	/**
	 * Заводим сторонний поток
	 */
	std::thread worker([&theirs, &visible]() noexcept -> void {
		// Запоминаем адрес склада стороннего потока
		theirs = &awh::vault_t::local();
		// Снимаем видимость чужой тайны
		visible = awh::vault_t::local().has("своя");
	});
	// Дожидаемся окончания стороннего потока
	worker.join();
	// Склады обязаны быть РАЗНЫМИ
	EXPECT_NE(ours, theirs);
	// Наша тайна стороннему потоку видна быть не обязана
	EXPECT_FALSE(visible);
	// У себя же тайна обязана остаться на месте
	EXPECT_TRUE(awh::vault_t::local().has("своя"));
	// Снимаем тайну со своего склада: он переживёт эту проверку
	EXPECT_TRUE(awh::vault_t::local().erase("своя"));
}
/**
 * @brief Тест замка состава склада, поделённого между потоками
 *
 * @note Проверка эта стережёт СОГЛАСОВАННОСТЬ СОСТАВА: без замка укладка и снятие из
 *       нескольких потоков правят одно дерево тайн разом, и порча его - поведение
 *       неопределённое, а не редкий отказ. Доказана мутацией: со снятым замком
 *       (`threading_t::LOCAL` у общего склада) прогон падает
 *
 */
TEST_F(VaultFixture, SharedVaultKeepsCompositionUnderThreads){
	// Склад, поделённый между потоками
	awh::vault_t vault(awh::alloc::secrecy_t::STRICT, awh::vault_t::threading_t::SHARED);
	// Склад обязан завестись
	ASSERT_TRUE(vault.ready());
	// Число потоков
	static constexpr size_t THREADS = 8;
	// Число тайн на поток
	static constexpr size_t SECRETS = 128;
	// Набор потоков
	std::vector <std::thread> workers;
	// Отводим место под потоки
	workers.reserve(THREADS);
	/**
	 * Потоки пускаются ОДНОВРЕМЕННО, а не как заведутся
	 *
	 * Заведение потока стоит дороже сотни укладок, и пущенные по мере заведения они
	 * разошлись бы по времени, почти не пересекаясь: проверка тогда стерегла бы
	 * согласованность там, где её никто и не нарушает
	 */
	// Признак пуска потоков
	std::atomic <bool> started(false);
	// Число изготовившихся потоков
	std::atomic <size_t> ready(0);
	/**
	 * Заводим потоки
	 */
	for(size_t t = 0; t < THREADS; t++){
		// Заводим очередной поток
		workers.emplace_back([&vault, &started, &ready, t]() noexcept -> void {
			// Отмечаемся изготовившимся
			ready.fetch_add(1, std::memory_order_relaxed);
			// Дожидаемся общего пуска
			while(!started.load(std::memory_order_acquire))
				// Уступаем время прочим потокам
				std::this_thread::yield();
			/**
			 * Перебираем тайны потока
			 */
			for(size_t i = 0; i < SECRETS; i++){
				// Название тайны потока
				const std::string name = ("тайна-" + std::to_string(t) + "-" + std::to_string(i));
				// Содержимое тайны потока
				const std::string secret = ("содержимое " + name);
				// Укладываем тайну на склад
				static_cast <void> (vault.store(name, secret.data(), secret.size()));
				// Спрашиваем состав склада
				static_cast <void> (vault.count());
				// Берём тайну рукоятью
				awh::vault_t::Handle handle = vault.borrow(name);
				// Если взятие удалось
				if(handle.valid())
					// Обращаемся к содержимому тайны
					static_cast <void> (handle.apply([](const char *, const size_t) noexcept -> void {}));
			}
		});
	}
	// Дожидаемся изготовности всех потоков
	while(ready.load(std::memory_order_relaxed) < THREADS)
		// Уступаем время прочим потокам
		std::this_thread::yield();
	// Пускаем потоки разом
	started.store(true, std::memory_order_release);
	/**
	 * Перебираем заведённые потоки
	 */
	for(auto & worker : workers)
		// Дожидаемся окончания очередного потока
		worker.join();
	// Состав склада обязан сойтись до единой тайны
	EXPECT_EQ(vault.count(), (THREADS * SECRETS));
	/**
	 * Перебираем потоки
	 */
	for(size_t t = 0; t < THREADS; t++){
		/**
		 * Перебираем тайны потока
		 */
		for(size_t i = 0; i < SECRETS; i++){
			// Название тайны потока
			const std::string name = ("тайна-" + std::to_string(t) + "-" + std::to_string(i));
			// Тайна обязана лежать на складе
			ASSERT_TRUE(vault.has(name)) << name;
			// Берём тайну рукоятью
			awh::vault_t::Handle handle = vault.borrow(name);
			// Взятие обязано удаться
			ASSERT_TRUE(handle.valid()) << name;
			// Содержимое, снятое обработчиком
			std::string seen;
			// Обращаемся к содержимому тайны
			ASSERT_TRUE(handle.apply([&seen](const char * data, const size_t size) noexcept -> void {
				// Снимаем содержимое тайны
				seen.assign(data, size);
			}));
			// Содержимое обязано совпасть с уложенным
			EXPECT_EQ(seen, ("содержимое " + name));
		}
	}
}
