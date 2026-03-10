#!/bin/bash

if [[ -f "build/server/desk_server" ]]
then
    build/server/desk_server
else
    echo "you should build project first"
fi