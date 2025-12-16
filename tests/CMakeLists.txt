# Unit test settings
find_package(GTest)

# Если библиотека GTest найдена
if(${GTest_FOUND})

	# Enable C++17
	SET(CMAKE_CXX_STANDARD 17)
	SET(CMAKE_CXX_STANDARD_REQUIRED ON)

	# Убираем ненужные и глупые варинги которые не имеют ничего общего с реальностью
	SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-implicitly-unsigned-literal -Wno-reserved-user-defined-literal")

	# Источники для сборки юнит-тестов
	SET(${PROJECT_NAME}_UNITTEST_SOURCES
		main.cpp
		sys/os/os.cpp
		sys/os/static.cpp
		sys/fmk/fmk.cpp
		sys/fmk/static.cpp
		sys/fmk/parameterized.cpp
		sys/log/log.cpp
		sys/log/static.cpp
		sys/log/parameterized.cpp
		sys/reg/reg.cpp
		sys/reg/static.cpp
		sys/reg/parameterized.cpp
		sys/queue/queue.cpp
		sys/queue/static.cpp
		sys/queue/parameterized.cpp
		sys/buffer/buffer.cpp
		sys/buffer/static.cpp
		sys/buffer/parameterized.cpp
		sys/chrono/chrono.cpp
		sys/chrono/parameterized.cpp
		sys/threadpool/threadpool.cpp
		sys/threadpool/static.cpp
		sys/threadpool/parameterized.cpp
		net/addr/addr.cpp
		net/addr/static.cpp
		net/addr/parameterized.cpp
		net/nwt/nwt.cpp
		net/nwt/static.cpp
		net/nwt/parameterized.cpp
		net/fds/fds.cpp
		net/fds/static.cpp
		net/eth/eth.cpp
		net/eth/static.cpp
		net/io/io.cpp
		net/io/static.cpp
	)

	# Библиотеки GTest для линковки
	SET(${PROJECT_NAME}_GTEST_LIBS
		GTest::gtest
		GTest::gmock
	)

	# Сборка бинарного приложения тестов
	add_executable(${PROJECT_NAME}_UNITTEST
		${${PROJECT_NAME}_UNITTEST_SOURCES}
	)

	# Если нужно собрать и динамическую библиотеку
	if (CMAKE_SHARED_BUILD_LIB)
		target_link_libraries(${PROJECT_NAME}_UNITTEST PUBLIC
			${${PROJECT_NAME}_GTEST_LIBS}
			${AWH_LIB_GCOV_NAME}
			${GCC_COVERAGE_LIBS}
			${OPENSSL_LIBRARIES}
			${TCMALLOC_LIBRARIES}
		)
	# Если нужно собрать и статическую библиотеку
	else (CMAKE_SHARED_BUILD_LIB)
		# Если режим отладки не активирован
        if (NOT CMAKE_BUILD_TYPE MATCHES Debug)
			target_link_libraries(${PROJECT_NAME}_UNITTEST PUBLIC
				${${PROJECT_NAME}_GTEST_LIBS}
				${AWH_LIB_GCOV_NAME}
				${PCRE_LIBRARY}
				${GCC_COVERAGE_LIBS}
				${OPENSSL_LIBRARIES}
				${TCMALLOC_LIBRARIES}
			)
		# Если режим отладки активирован
        else()
			target_link_libraries(${PROJECT_NAME}_UNITTEST PUBLIC
				${${PROJECT_NAME}_GTEST_LIBS}
				${AWH_LIB_GCOV_NAME}
				${DEPEND_LIBRARY}
				${PCRE_LIBRARY}
				${GCC_COVERAGE_LIBS}
				${OPENSSL_LIBRARIES}
				${TCMALLOC_LIBRARIES}
			)
		endif()
	endif (CMAKE_SHARED_BUILD_LIB)

	# Включаем флаги компиляции для сбора статистики покрытия кода
	target_compile_options(${PROJECT_NAME}_UNITTEST PRIVATE ${GCC_COVERAGE_COMPILE_FLAGS})
endif()
