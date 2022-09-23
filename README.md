# esp32_http_client
HTTP client component


## Links

- [ESP HTTP Client](https://docs.espressif.com/projects/esp-idf/en/v4.4/esp32s3/api-reference/protocols/esp_http_client.html) - API v4.4
- [esp_http_client_example.c](https://github.com/espressif/esp-idf/blob/v4.4/examples/protocols/esp_http_client/main/esp_http_client_example.c)

## Setup
- Add [wi-fi component](https://github.com/akhud78/esp32_wifi) into `components` folder.
- Add [http client component](https://github.com/akhud78/esp32_http_client) into `components` folder.
```
$ cd ~/esp/my_project/components
$ git submodule add https://github.com/akhud78/esp32_http_client http_client
```

## Examples
### cURL 
- [curl.1 the man page](https://curl.se/docs/manpage.html)
- [httpbin.org](http://httpbin.org/) - A simple HTTP Request & Response Service
```
$ curl -v http://httpbin.org/get
$ curl -v http://httpbin.org/get -o body.txt
$ cat body.txt
$ curl -v http://user:passwd@httpbin.org/basic-auth/user/passwd
$ curl -v http://httpbin.org/image/jpeg -o image.jpg
$ ls -la image.jpg 
-rw-rw-r-- 1 avk avk 35588 фев 10 16:57 image.jpg
$ hexdump -n 15 image.jpg 
0000000 d8ff e0ff 1000 464a 4649 0100 0201 0000
000000f
```

### Snapshot
- Getting snapshot image from IP Camera
- Set HTTP Client Configuration **Image Uri** and run test **image reader**
#### URI examples
- TRASSIR TR-D7121IR1W
```
http://admin:uzh2ndcd@192.168.1.72/action/snap?cam=0
```
- D-Link DCS-930L
```
http://admin:uzh2ndcd@192.168.1.103/image/jpeg.cgi
```
- Foscam FI8918W
```
http://admin:@192.168.4.2/snapshot.cgi
```
## Configuration


- Set HTTP Client Configuration
```
(Top) -> Component config -> HTTP Client Configuration
(http://httpbin.org/get) Server Uri
(http://httpbin.org/image/jpeg) Image Uri
```
