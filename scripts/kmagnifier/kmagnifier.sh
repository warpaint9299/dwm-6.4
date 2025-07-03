#!/usr/bin/env bash

[[ $(pgrep -x kmag) ]] \
	&& ps -aux \
		| grep 'kmag' \
		| grep -v 'grep' \
		| awk '{ print $2}' \
		| xargs -I {} kill -s TERM {} \
	|| kmag &
