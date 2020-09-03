#!bin/bash

savefile=$1
if [[ "$1" == "" ]]
then
	echo "Error Empty String"
else
	git add *
	git commit -m "Finally finished q1 - q6!"
	git push
fi

