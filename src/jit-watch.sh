#
# General code for kwatcher
#
DATA_DIR=$TOP/data/
BELL_DIR=$TOP/bell
LOGFILE=$TOP/KWATCHLOG
PID_FILE=$TOP/KWATCH_PID
CONF_FILE=$TOP/etc/conf
OPTIONS="-D -d -v -c $CONF_FILE -f $LOGFILE $BELL_DIR $DATA_DIR"
OPTIONS2="-d -v -c $CONF_FILE -f $LOGFILE $BELL_DIR $DATA_DIR"

case "$1" in
  start)
        echo -n "Removing data: "
	rm -f $DATADIR/*
	echo "done"
        echo -n "Starting kwatcher: "
	echo "$DAEMON $OPTIONS $*"
	$DAEMON $OPTIONS $* >$PID_FILE
        echo "done"
	;;
  start2)
        echo -n "Removing data: "
	rm -f $DATA_DIR/*
	echo -n "done"
        echo -n "Starting kwatcher: "
	echo "$DAEMON $OPTIONS2 $*"
	$DAEMON $OPTIONS2 $*
        ;;
  stop)
        echo -n "Stopping kwatcher: "
	kill -KILL `cat $PID_FILE`
        echo "done"
        ;;
  *)
        echo "Usage: $0 start [<options>]" >&2
        echo "       $0 stop" >&2
        ;;
esac

exit 0
