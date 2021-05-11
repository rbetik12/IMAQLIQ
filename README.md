# TCP Client-Server file saver

## Сборка проекта

```bash
cmake .
make
```
Затем в директориях server и client будут находится исполняемые файлы для сервера
и клиента.
## Запуск клиента
Запуск клиентской части осуществляется с помощью команды:

```bash
./client <номер порта> <путь до файла>
```
Поддерживаются любые типы путей до файла (относительные и абсолютные).

### Пример работы клиента
```bash
Starting file transmission.
%100.00	[|||||||||||]	17/17 Bytes
Successfully sent file kek.txt to server!
```

## Запуск сервера
Сервер автоматически запускается как демон. 

Чтобы запустить сервер нужно выполнить команду:

```bash
./server <номер порта>
```

```bash
# cat /var/log/syslog | grep -i "cool"
cooldaemon[12723]: Started accept thread
cooldaemon[12723]: Started main server thread
cooldaemon[12732]: Accepted new user!
cooldaemon[12732]: Got new file: picture.png. Saving to /tmp/picture.png
```

Чтобы остановить сервер нужно выполнить:

```bash
kill --signal TERM <server pid>
# Или
kill --signal HUP <server pid>
```