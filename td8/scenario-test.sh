#!/bin/sh

EXEC="./million"
CFG="lottery-test.cfg"

echo "5" >$CFG
echo "2 5 8 12 15" >>$CFG
echo "5 100000" >>$CFG
echo "4 10000" >>$CFG
echo "3 100" >>$CFG
echo "2 10" >>$CFG
echo "1 1" >>$CFG

$EXEC server lottery-test.cfg &

$EXEC client 2 5 9 12 15
if [ $? != 0 ]; then
    echo "<TEST> ERR: false combination does not get right return code"
    exit 1
fi

$EXEC client 2 5 8 12 15
if [ $? != 1 ]; then
    echo "<TEST> ERR: right combination does not get right return code"
    exit 1
fi

$EXEC client 2 5 10 12 15
if [ $? != 2 ]; then
    echo "<TEST> ERR: client without server does not get right return code"
    exit 1
fi

echo "<TEST> All tests were nicely done"
rm $CFG
