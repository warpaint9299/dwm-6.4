#!/usr/bin/env bash

[[ $(pgrep -x screenkey) ]] \
	&& ps -aux \
		| grep 'screenkey' \
		| grep -v 'grep' \
		| awk '{ print $2}' \
		| xargs -I {} kill -s TERM {} \
	|| screenkey &
