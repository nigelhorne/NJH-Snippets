package NJH::Snippets::DB::people;

use strict;
use warnings;

use NJH::Snippets::DB;

our @ISA = ('NJH::Snippets::DB');

sub _open {
	my $self = shift;

	return $self->SUPER::_open(sep_char => "\t", column_names => ['column1','column2']);
}

1;
