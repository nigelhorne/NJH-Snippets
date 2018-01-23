# NJH-Snippets

# SYNOPSIS

Perl snippets used across many of my programs.

## NJH::Snippets::DB

Provides read-only access to databases.
Usually all that's neeeded to access a database called 'bar' is a class declaration such as this:

    package Foo::Bar;

    # The bar database

    use NJH::Snippets::DB;

    our @ISA = ('NJH::Snippets::DB');

    1;

and code such as this in the main program

    use Foo::Bar;

    my $foo = Foo::Bar->new({ directory => "/var/lib/db/foo" });

    # TODO: add documentation on how to access the database

# LICENSE AND COPYRIGHT

Copyright 2015-2018 Nigel Horne.

This program is released under the following licence: GPL2 for personal use on
a single computer.
All other users (for example Commercial, Charity, Educational, Government)
must apply in writing for a licence for use from Nigel Horne at `<njh at nigelhorne.com>`.
