# NJH-Snippets

# SYNOPSIS

Code snippets

## dbpedia\_location

Query DBpedia for Geocoding.
Also produces OSM HTML.

## eml\_to\_clamav

Produce **ClamAV** signatures from emails

## format

Short C program to show the different formatting options with printf.

## geo-free-test

Use addresses in a Gedcom to generate addresses to test Geo::Coder::Free:

## geotag

Show where photographs were taken.
It reads the GPS (geotag) information from a JPG and reverse geo-codes it to
print the address.

## peek.c

Reads input files, converts them to S-records, and sends them 
to a specified destination via TCP, UDP, or a serial port.
Additionally,
it supports outputting the S-records to standard output for debugging or testing.

## server\_dashboard

This script creates a web dashboard that monitors the status of a list of web servers.
It uses the **Mojolicious** framework to build a simple web interface that shows the server status, HTTP response codes, response time, and color-coded indicators (green for healthy servers, yellow for errors like 404 or 500, and red for unreachable servers).
The list of servers to monitor is read from a `servers.txt` file, and the script uses `LWP::UserAgent` to send HTTP requests and check the servers’ availability.
It also calculates the response time of each server and assigns a color to the response time, helping to quickly identify slow servers (orange for moderate delays and red for slow responses).
The dashboard auto-refreshes every 60 seconds to display up-to-date server status information.

The script also logs detailed error information into a `server_status.log` file whenever it encounters an unreachable server or HTTP error.
It records the timestamp, the URL, the status (e.g., "Unreachable" or "HTTP Error"), and the error message (e.g., connection refusal or timeout).
The logging helps with troubleshooting by providing a history of server failures and errors.
Users can filter the dashboard to show only healthy servers or only servers with errors.
Additionally, each server URL is clickable, allowing users to directly visit the server’s webpage for more details.
This combination of real-time monitoring and logging ensures that users have both a live status overview and a historical log of issues.

## tapechk

Verify a tar or cpio archive written to a device such as a tape drive

## wof-bundle-download

Download and unpack the latest files from https://dist.whosonfirst.org/bundles/

## wof-sqlite-download

Download and unpack the latest files from https://dist.whosonfirst.org/sqlite/

## xml2hash

Convert an XML document into a hash

## zlib.c

Sample program for the zlib library.

# LICENSE AND COPYRIGHT

Copyright 2015-2025 Nigel Horne.

This program is released under the following licence: GPL2 for personal use on
a single computer.
All other users (for example Commercial, Charity, Educational, Government)
must apply in writing for a licence for use from Nigel Horne at `<njh at nigelhorne.com>`.
