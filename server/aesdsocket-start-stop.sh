#!/bin/sh
case $1 in

  start)
    echo "Starting aesdsocket process as deamon"
    start-stop-daemon --start --name aesdsocket --startas /usr/bin/aesdsocket -- -d
    ;;

  stop)
    echo "Stopping aesdsocket deamon"
    start-stop-daemon --stop --name aesdsocket
    ;;

  restart)
    echo "Restarting aesdsocket deamon"
    $0 stop
    sleep 2
    $0 start
    ;;

  *)
   echo "Usage : $0 (start|stop|restart)"
   exit 1
    ;;
esac

exit 0

