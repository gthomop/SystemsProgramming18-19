#!/bin/bash

read log_line
if [ "$log_line" = "" ]; then
	echo "No input, script ends"
	exit 1
fi

connected=0
min=$(echo $log_line|awk '{print $2}'|tr -d ':')		#expect the first line of the first log file to be "Client X: Connection successful"
min=$((min))
max=$min
byteswr=0
bytesrd=0
fileswr=0
filesrd=0
leftcl=0

while [ "$log_line" != "" ]; do
	tmp=$(echo $log_line|awk '{print $1}')
	if [ "$tmp" == "Client" ]; then
		let connected=connected+1
		tmpid=$(echo $log_line|awk '{print $2}'|tr -d ':')
		tmpid=$((tmpid))
		if [ $tmpid -lt $min ]; then
			min=$tmpid
		elif [ $tmpid -gt $max ]; then
			max=$tmpid
		fi
	elif [ "$tmp" == "Exiting" ]; then
		let leftcl=leftcl+1
	elif [ "$tmp" == "From" ]; then
		tmp=$(echo $log_line|awk '{print $4}')
		if [ "$tmp" != "Initiating" ] && [ "$tmp" != "Failed" ] && [ "$tmp" != "No" ] && [ "$tmp" != "File" ]; then
			b=$(echo $log_line|awk '{print $6}')
			let bytesrd=bytesrd+b
			let filesrd=filesrd+1
		fi
	elif [ "$tmp" == "To" ]; then
		tmp=$(echo $log_line|awk '{print $4}')
		if [ "$tmp" != "Initiating" ] && [ "$tmp" != "Failed" ] && [ "$tmp" != "File" ]; then
			b=$(echo $log_line|awk '{print $6}')
			let byteswr=byteswr+b
			let fileswr=fileswr+1
		fi
	fi
	read log_line
done

echo "Clients connected = $connected"
echo "Minimum ID = $min, Maximum ID = $max"
echo "Bytes sent = $byteswr, Bytes read = $bytesrd"
echo "Files sent = $fileswr, Files read = $filesrd"
echo "Client disconnected = $leftcl"
