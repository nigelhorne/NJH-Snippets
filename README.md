# NJH-Snippets

# SYNOPSIS

Code snippets

## NJH::Snippets::DB

Abstract class giving read-only access to CSV, XML and SQLite databases via Perl without writing any SQL.
Usually all that's neeeded to access a database called 'bar' is a class declaration such as this:

    package Foo::Bar;

    # The bar database

    use NJH::Snippets::DB;

    our @ISA = ('NJH::Snippets::DB');

    1;

and code such as this in the main program

    use Foo::Bar;

    my $foo = Foo::Bar->new(directory => "/var/lib/db/foo");
    my $row = $foo->fetchrow_hashref(customer_id => '12345);
    print Data::Dumper->new([$row])->Dump();

## format

Short C program to show the different formatting options with printf.

## geo-free-test

Use addresses in a Gedcom to generate addresses to test Geo::Coder::Free:

## geotag

Show where photographs were taken.
It reads the GPS (geotag) information from a JPG and reverse geo-codes it to
print the address.

## wof-bundle-download

Download and unpack the latest files from https://dist.whosonfirst.org/bundles/

## wof-sqlite-download

Download and unpack the latest files from https://dist.whosonfirst.org/sqlite/

# LICENSE AND COPYRIGHT

Copyright 2015-2019 Nigel Horne.

This program is released under the following licence: GPL2 for personal use on
a single computer.
All other users (for example Commercial, Charity, Educational, Government)
must apply in writing for a licence for use from Nigel Horne at `<njh at nigelhorne.com>`.
