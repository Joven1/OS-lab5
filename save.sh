#!bin/bash

savefile=$1
if [[ "$1" == "" ]]
then
	echo "Error Empty String"
else
	git add *
	git commit -m "$1"
	git push
fi

