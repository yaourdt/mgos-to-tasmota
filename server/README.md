## Build the docker image
Run the following commad to build the docker image with the tag mos
```
docker build -t mos .
```

## Build update firmware
In the mos.yml replace <HOSTNAME OR IP> with the IP or HOSTNAME of your webserver.
```yaml
- when: build_vars.TARGETFW == "url"
    apply:
      config_schema:
        - ["mg2x.url", "s", "<HOSTNAME OR IP>:8000/ota.bin" , {"title": "URL of target firmware"}]

```
To build the firmware run the following command via docker.
 ```
 docker run --rm -it -v $PWD:/sources mos build --build-var MODEL=Shelly25 --build-var TARGETFW=url --platform esp8266
 ```
It will result in a file build/fw.zip. This needs to be copied to the webserver directory e.g. server/fw.zip.

## Start the python webserver
First place your firmware as server/ota.bin in the webserver directory. To start the python webserver run the following command.
```
cd server/
python3 -m http.server 8000
```

## Run the update on your shelly device
Call your device from the webbrowser with the update url http://<IP of the shelly>/ota?url=http://<HOSTNAME OR IP>:8000/fw.zip. Replace <IP of the shelly> with the IP of your shelly device and <HOSTNAME OR IP> with the dns name or IP of your webserver.
