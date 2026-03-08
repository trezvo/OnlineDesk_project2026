#!/bin/bash

if [[ -f "build/client/desk_client" ]]
then
    build/client/desk_client
else
    echo "you should build project first"
fi