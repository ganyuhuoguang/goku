#!/bin/bash

while getopts ":p:" opt; do
  case $opt in
    p) path="$OPTARG";;
    \?) echo "Invalid option -$OPTARG" >&2;;
  esac
done

docker kill mysql

docker rm mysql

docker run -it -e "MYSQL_ROOT_PASSWORD=novumind" --name mysql --network host -d registry.corp.novumind.com/3rdparty/mysql

if [ -n "$path" ]; then

  sleep 30

  docker exec -i mysql mysql -uroot -pnovumind < "$path"

fi
