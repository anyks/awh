# @file: coverage.cmake
# @brief: Прогон юнит-тестов и формирование HTML-отчёта о покрытии кода
#
# Скрипт запускается через `cmake -P` целью `coverage` и принимает параметры:
#   SRC_DIR    - корневой каталог исходного кода проекта
#   BIN_DIR    - каталог сборки (где лежат данные покрытия *.gcda/*.gcno)
#   REPORT_DIR - каталог для HTML-отчёта о покрытии
#   GCOVR      - путь к утилите gcovr
#   GCOV_TOOL  - команда gcov для разбора данных покрытия (например "xcrun llvm-cov gcov")
#   TEST_BINS  - список исполняемых файлов тестов (разделитель ";")

# Создаём каталог для HTML-отчёта о покрытии
file(MAKE_DIRECTORY "${REPORT_DIR}")

# Перебираем все исполняемые файлы тестов
foreach(TEST_BIN IN LISTS TEST_BINS)
	# Если исполняемый файл теста существует
	if(EXISTS "${TEST_BIN}")
		# Выводим сообщение о запуске теста
		message(STATUS "Running tests: ${TEST_BIN}")
		# Запускаем тест (падение теста не прерывает формирование отчёта)
		execute_process(
			COMMAND "${TEST_BIN}"
			WORKING_DIRECTORY "${BIN_DIR}"
			RESULT_VARIABLE TEST_RESULT
		)
		# Если тест завершился с ошибкой
		if(NOT TEST_RESULT EQUAL 0)
			# Выводим предупреждение (отчёт о покрытии всё равно будет сформирован)
			message(WARNING "Tests ${TEST_BIN} exited with code ${TEST_RESULT}")
		endif()
	endif()
endforeach()

# Формируем команду запуска gcovr
set(GCOVR_CMD
	"${GCOVR}"
	--root "${SRC_DIR}"
	--filter "${SRC_DIR}/src/"
	--filter "${SRC_DIR}/include/"
	--gcov-executable "${GCOV_TOOL}"
	--exclude-unreachable-branches
	--exclude-throw-branches
	--print-summary
	--html-details "${REPORT_DIR}/index.html"
	"${BIN_DIR}"
)

# Выводим сообщение о формировании отчёта о покрытии
message(STATUS "Generating coverage report into ${REPORT_DIR}/index.html")

# Запускаем формирование отчёта о покрытии
execute_process(
	COMMAND ${GCOVR_CMD}
	WORKING_DIRECTORY "${BIN_DIR}"
	RESULT_VARIABLE GCOVR_RESULT
)

# Если формирование отчёта завершилось с ошибкой
if(NOT GCOVR_RESULT EQUAL 0)
	# Прерываем выполнение с ошибкой
	message(FATAL_ERROR "gcovr failed with code ${GCOVR_RESULT}")
endif()

# Выводим сообщение об успешном формировании отчёта
message(STATUS "Coverage report is ready: ${REPORT_DIR}/index.html")
