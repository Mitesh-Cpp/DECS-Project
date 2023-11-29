IP=$1
PORT=$2
FILE=$3
TIMEOUT=$4
PROG_ID=$5
./client $IP $PORT 0 $FILE $TIMEOUT $PROG_ID
REQ_ID=$(cat ./client_files/client_request_id_$PROG_ID.txt)
TYPE=0
TRY=0
while [ $TYPE -ne 3 ] && [ $TRY -le 10 ];
do
    sleep 2
    TRY=$(($TRY+1))
    ./client $IP $PORT 1 $REQ_ID $TIMEOUT $PROG_ID
    TYPE=$?
done
if [ $TRY -gt 5]; then 
    exit(1)
fi
echo "DONE"
exit(0) 