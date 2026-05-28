#!/bin/bash

set -e

echo "Cleaning previous test data..."
rm -rf .minigit
rm -f a.txt b.txt c.txt

echo
echo "Building project..."
make clean
make

echo
echo "Initializing repository..."
./minigit init

echo
echo "Creating files..."
echo "aaa version 1" > a.txt
echo "bbb version 1" > b.txt

echo
echo "Adding files and creating first commit..."
./minigit add a.txt
./minigit add b.txt
./minigit status
./minigit commit "initial commit"

echo
echo "Commit history after first commit:"
./minigit log

echo
echo "Files in commit 1:"
./minigit files 1

echo
echo "Updating a.txt and creating second commit..."
echo "aaa version 2" > a.txt
./minigit add a.txt
./minigit status
./minigit commit "update a"

echo
echo "Commit history after second commit:"
./minigit log

echo
echo "Files in commit 1:"
./minigit files 1

echo
echo "Files in commit 2:"
./minigit files 2

echo
echo "Showing a.txt from commit 1:"
./minigit show 1 a.txt

echo
echo "Showing a.txt from commit 2:"
./minigit show 2 a.txt

echo
echo "Checking file existence:"
./minigit exists 1 a.txt
./minigit exists 2 a.txt
./minigit exists 2 c.txt

echo
echo "Removing b.txt and creating third commit..."
./minigit rm b.txt
./minigit status
./minigit commit "remove b"

echo
echo "Files in commit 3:"
./minigit files 3

echo
echo "Checking b.txt existence:"
./minigit exists 1 b.txt
./minigit exists 3 b.txt

echo
echo "Commit information:"
./minigit commit-info 1
./minigit commit-info 2
./minigit commit-info 3

echo
echo "Repository statistics:"
./minigit stats

echo
echo "Manual test completed successfully."