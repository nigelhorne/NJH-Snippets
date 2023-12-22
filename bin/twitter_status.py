#!/usr/bin/env python3

# Show the tweets from a given account over the last week
# https://github.com/bocchilorenzo/ntscraper

from ntscraper import Nitter
import datetime
import json
import sys

scraper = Nitter(log_level=0, skip_instance_check=False)

timestamp = datetime.datetime.now() - datetime.timedelta(days=7)

bbportal_tweets = scraper.get_tweets(sys.argv[1], mode='user', since=timestamp.strftime('%F'))

print(json.dumps(bbportal_tweets))
