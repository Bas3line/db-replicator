#!/bin/bash

set -e

CONTAINER_NAME=enigma-postgres

docker exec -it $CONTAINER_NAME psql -U enigma -d enigma -c "ALTER SYSTEM SET wal_level = 'logical';"
docker exec -it $CONTAINER_NAME psql -U enigma -d enigma -c "ALTER SYSTEM SET max_replication_slots = 4;"
docker exec -it $CONTAINER_NAME psql -U enigma -d enigma -c "ALTER SYSTEM SET max_wal_senders = 4;"

docker restart $CONTAINER_NAME
sleep 10

PUBLICATION_EXISTS=$(docker exec -it $CONTAINER_NAME psql -U enigma -d enigma -tAc "SELECT 1 FROM pg_publication WHERE pubname = 'db_changes'")

if [ "$PUBLICATION_EXISTS" = "1" ]; then
  echo "Publication 'db_changes' already exists, skipping creation."
else
  echo "Creating publication 'db_changes'."
  docker exec -it $CONTAINER_NAME psql -U enigma -d enigma -c "CREATE PUBLICATION db_changes FOR ALL TABLES;"
fi

REPLICATION_EXISTS=$(docker exec -it $CONTAINER_NAME psql -U enigma -d enigma -tAc "SELECT 1 FROM pg_replication_slots WHERE slot_name='cdc_slot'")

if [ "$REPLICATION_EXISTS" = "1" ]; then
  echo "Replication slot 'cdc_slot' already exists, skipping creation."
else
  echo "Creating replication slot 'cdc_slot'."
  docker exec -it $CONTAINER_NAME psql -U enigma -d enigma -c "SELECT pg_create_logical_replication_slot('cdc_slot', 'pgoutput');"
fi

docker exec -it $CONTAINER_NAME psql -U enigma -d enigma -c "SELECT * FROM pg_publication;"
docker exec -it $CONTAINER_NAME psql -U enigma -d enigma -c "SELECT * FROM pg_replication_slots;"

