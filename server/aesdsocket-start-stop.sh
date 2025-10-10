#!/bin/sh

DAEMON=/usr/bin/aesdsocket
OPTS="-d"

case "$1" in
  start)
    echo "Starting $DAEMON..."
    start-stop-daemon --start --quiet \
    --pidfile /var/run/$DAEMON.pid --make-pidfile \
    --background --exec $DAEMON -- $OPTS
    ;;
  stop)
    echo "Stopping $DAEMON..."
    start-stop-daemon --stop --quiet --pidfile /var/run/$DAEMON.pid
    ;;
esac

exit 0