# Русскоязычная сборка документации
#
# Наследует все настройки основного файла Doxyfile и переопределяет
# лишь язык вывода, каталог назначения и подписи собственных меток шапки.
# Из многоязычных комментариев (\~russian / \~english) выбирается русская ветвь.

@INCLUDE = Doxyfile

OUTPUT_LANGUAGE  = Russian
OUTPUT_DIRECTORY = ./doc/ru

# Подписи меток шапки на русском языке: сами метки заведены в Doxyfile,
# здесь переопределяются только их названия, исходники этого не касается

ALIASES = "license{1}=\par Лицензия^^\ref awh_license \"\1\"" \
          "telegram{1}=\par Телеграм^^<a href=\"https://t.me/\1\">\1</a>" \
          "phone{1}=\par Телефон^^\htmlonly<a href=\"tel:\1\">\endhtmlonly\1\htmlonly</a>\endhtmlonly" \
          "email=\par Почта^^" \
          "site=\par Сайт^^"
