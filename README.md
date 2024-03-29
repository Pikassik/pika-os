# Кросс-компилятор
В принципе, можно использовать компилятор, который стоит в системе, но только тогда ему нужно будет передавать определённые флаги. Проще всего будет использовать кросс-компилятор, чтобы не заморачиваться со всем этим. Для наших нужд его можно либо скомпилировать вручную, либо взять готовый. В любом случае, есть хорошая [инструкция](https://wiki.osdev.org/GCC_Cross_Compiler) на osdev wiki, как всё это настроить.

# QEMU
Чтобы тестировать ОС, потребуется эмулятор. Мы пишем ОС под i686, поэтому потребуется эмулятор для это платформы. Предлагается взять QEMU, потому что он умеет запускать ядро по ELF-файлу (без установки GRUB). [Ссылка](https://www.qemu.org/download/) на официальный сайт. Проверьте, что у вас появился исполняемый файл `qemu-system-i386`.

# Самое простое ядро
В этой папке лежит самое простое ядро, которое может загрузить QEMU. Всё, что оно делает — выводит на VGA-экран примитивный текст. Пока никаких прерываний, переключений контекста и ядерных штук :).

## boot.s
Этот файл содержит в себе описание специальной секции `.multiboot`. [Multiboot](https://www.gnu.org/software/grub/manual/multiboot/multiboot.html) — это стандарт (или можно сказать протокол), который определяет порядок загрузки ядер из исполняемых файлов.

В самом начале файла располагается multiboot header, он состоит из трёх чисел:
* `magic` всегда равно `0x1BADB002`
* `flags` содержит битовую маску флагов, мы использует только первые два флага: ALIGN и MEMINFO. Первый говорит о включении выравнивания по границе страницы, а второй требует от загрузчика специальной информации о памяти.
* `checksum` — просто контрольное число.

Загрузчик загружает ядро, в соответствии с тем, как указано в ELF-файле. А затем передаёт управление по адресу, который указан как entry point (в нашем случае _start).

## kernel.c
Содержит простой код, который работает с VGA-фреймбуффером. По-сути, это двумерный массив, который описывает каждый пиксель. Когда в определённую ячейку что-то записывается, картинка на экране обновляется. Такая модель работы с периферией называется memory-mapped I/O или MMIO.

## linker.ld
Вы программировали когда-нибудь линкер? Нет? Ну... Теперь самое время. Этот файл — linker script. Он описывает как располагаются секции внутри итогового исполняемого файла, а также какие у них выравнивания, флаги и по каким адресам они загружаются. [Документация](https://ftp.gnu.org/old-gnu/Manuals/ld-2.9.1/html_chapter/ld_3.html).

# Что дальше?
1. Напишите более умный терминал: как минимум, вам потребяется скроллинг :). Как максимум — более приятный API.
2. Сделайте удобный printf — его сейчас нет, но как-то отлаживать нужно будет. Ещё понадобятся strlen, memset, memcpy.
