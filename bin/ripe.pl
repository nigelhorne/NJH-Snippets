#!/usr/bin/perl -wT

use strict;

if (eval "require Net::Whois::RIPE") {
	import Net::Whois::RIPE;
} else {
	die "Sorry, you need Net::Whois::RIPE";
}

my $ripe = new Net::Whois::RIPE;

foreach (@ARGV) {
	$ripe->whois_query(-ip=>$_);
	print "Country: " . $ripe->country() . "\n";
	print "Netname: " . $ripe->netname() . "\n";
	print "Descr: "   . $ripe->descr()   . "\n";
	print "Status: "  . $ripe->status()  . "\n";
	print "Source: "  . $ripe->source()  . "\n";
	print "Server: "  . $ripe->server()  . "\n";
	print "Inetnum: " . $ripe->inetnum() . "\n";
}
