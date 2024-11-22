#!/usr/bin/env perl

use strict;
use warnings;
use MIDI;

# Process each file provided as an argument
foreach my $file (@ARGV) {
	process_midi_file($file);
}

# Subroutine to process a MIDI file and print specific text events
sub process_midi_file
{
	my($file) = @_;

	# Create a new MIDI Opus object from the file
	my $opus = MIDI::Opus->new({ 'from_file' => $file, 'no_parse' => 0 });

	# Iterate through each track in the MIDI file
	for my $track ($opus->tracks()) {
		# Iterate through each event in the track
		for my $event ($track->events()) {
			my($type, undef, $text) = @{$event};

			# Check for text or name events and print non-empty text
			if(($type =~ /^(text|name)$/) && (defined $text) && ($text ne '')) {
				print "$text\n";
			}
		}
	}
}
