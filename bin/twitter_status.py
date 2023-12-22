# https://github.com/bocchilorenzo/ntscraper

from ntscraper import Nitter
import sys

scraper = Nitter(log_level=0, skip_instance_check=False)

bbportal_tweets = scraper.get_tweets(sys.argv[1], mode='user', since='2023-12-20')

print(bbportal_tweets)
