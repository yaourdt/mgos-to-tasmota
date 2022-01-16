FROM alpine
#copy from cruftlab/mongoose-os-docker
MAINTAINER Morten Aasbak loket@cruftlab.io

# Install mos tool dependencies
RUN apk add --no-cache curl

# Install mos tool
ADD https://mongoose-os.com/downloads/mos/install.sh /tmp/install-mos.sh
RUN sh /tmp/install-mos.sh

# Add volume and workdir
VOLUME /sources/
WORKDIR /sources/

# Set entrypoint
ENTRYPOINT ["/root/.mos/bin/mos"]

# docker build -t mos .
# docker run --rm -it -v $PWD:/sources mos build --build-var MODEL=$model --build-var TARGETFW=url TARGETURL=http://localhost --platform esp8266
