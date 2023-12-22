# https://github.com/bocchilorenzo/ntscraper

from ntscraper import Nitter
import sys

scraper = Nitter(log_level=0, skip_instance_check=False)

# TODO: work out how to say a week ago
bbportal_tweets = scraper.get_tweets(sys.argv[1], mode='user', since='2023-12-17')

print(bbportal_tweets)
