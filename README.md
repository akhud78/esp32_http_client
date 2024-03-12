# esp32_http_client
HTTP client component

## Links

- [ESP HTTP Client](https://docs.espressif.com/projects/esp-idf/en/v5.1.1/esp32s3/api-reference/protocols/esp_http_client.html)
- [esp_http_client_example.c](https://github.com/espressif/esp-idf/blob/v5.1.1/examples/protocols/esp_http_client/main/esp_http_client_example.c)
- [How to use esp_http_client to send chunked data?](https://docs.espressif.com/projects/esp-faq/en/latest/software-framework/protocols/http.html#how-to-use-esp-http-client-to-send-chunked-data)


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

## Test

```
Here's the test menu, pick your combo:
(1)	"get" [client]
(2)	"post" [client]
(3)	"image reader" [client]
(4)	"image get native" [client]
(5)	"list reader" [client]
```
### Local file server
#### Image reader
- [data/test](https://github.com/akhud78/esp32_face/tree/main/data/test) - download images
- [Python SimpleHTTPServer](https://www.digitalocean.com/community/tutorials/python-simplehttpserver-http-server) - use as local file server

```
$ cd data/test
$ python3 -m http.server 9000
Serving HTTP on 0.0.0.0 port 9000 (http://0.0.0.0:9000/) ...
```
- Check [localhost:9000](http://localhost:9000/) - `Directory listing for`
- Set `Image Uri` as an example `http://192.168.1.40:9000/soto_2.jpg`
- Run test
```
$ idf.py -p /dev/ttyUSB0 -D TEST_COMPONENTS="http_client" flash monitor
...
Enter test for running.
3
Running image reader...
...
I (29873) http_client: HTTP url: http://192.168.1.40:9000/soto_2.jpg 
I (30223) http_client: read_len = 15218
I (30223) http_client: ff d8 ff e0 00 10 4a 46 
I (30223) http_client: HTTP Stream reader Status = 200, content_length = 15218
...
```
#### Image get native
```
I (44369) test_http_client: bytes: 35588 chunked: 0
I (46209) test_http_client: bytes: 35588 chunked: 1
```

#### List reader
- Create list of files in server directory
- Run server
```
$ cd data/test
$ ls -p | grep -v / | grep -v "list.txt" > list.txt
$ python3 -m http.server 9000
```
- Check [localhost:9000](http://localhost:9000/)
- Set `Server Uri` as `http://192.168.1.40:9000`
- Run test
```
$ idf.py -p /dev/ttyUSB0 -D TEST_COMPONENTS="http_client" flash monitor
...
Enter test for running.
5
Running list reader...
...
I (4453) http_client: HTTP url: http://192.168.1.40:9000/list.txt 
I (4753) http_client: read_len = 74
I (4753) http_client: 49 4d 33 31 36 5f 31 2e 
I (4753) http_client: HTTP Stream reader Status = 200, content_length = 74
I (4753) http_client: HTTP url: http://192.168.1.40:9000/IM316_1.JPG 
I (4793) http_client: read_len = 8793
I (4793) http_client: ff d8 ff e0 00 10 4a 46 
I (4793) http_client: HTTP Stream reader Status = 200, content_length = 8793
I (4803) http_client: HTTP url: http://192.168.1.40:9000/IM319_1.JPG 
I (4833) http_client: read_len = 8576
I (4833) http_client: ff d8 ff e0 00 10 4a 46 
I (4833) http_client: HTTP Stream reader Status = 200, content_length = 8576
...
Test ran in 1992ms
```