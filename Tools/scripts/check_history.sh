#!/bin/bash

# Redirect output to stderr.
exec 1>&2

disallowed=$(git show private:disallowed.txt) || { echo "$0 could not read disallowed file"; exit 1; }
log_exceptions=$(git show private:log_exceptions.txt) || { echo "$0 could not read exceptions file"; exit 1; }
log_disallowed=$(git log -p | grep -wiE "$disallowed")

# GREP full Word, Ignore case, Extended regex
if [ "$log_disallowed" != "$log_exceptions" ]; then
	echo "FAIL - New $disallowed have been added"
	diff -u <(echo "$log_exceptions") <(echo "$log_disallowed")
	exit 1
fi
exit 0
