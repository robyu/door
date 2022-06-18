FROM eclipse-mosquitto:2.0.14

WORKDIR /

#
# eclipse-mosquitto doesn't support timezone data, so we have to install it ourselves
RUN apk add --no-cache tzdata && \
    rm -rf /var/cache/apk/*