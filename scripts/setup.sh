#!/bin/bash
set -e
echo "Setting up PostgreSQL CDC Replicator..."
docker-compose up -d sm-backup
echo "Waiting for backup database..."
sleep 10
docker-compose up --build cdc-replicator
echo "CDC Replicator is running!"
