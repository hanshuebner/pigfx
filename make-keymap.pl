#!/usr/bin/perl -w

use strict;

sub parse_spec {
    my $input = shift;
    $input =~ s/^\s*(.*?)\s*$/$1/;
    if ($input =~ /^$/) {
        return "&dead_key";
    } elsif ($input =~ /^CSI (.*)/) {
        return "new String(\"\\x1b[$1\")";
    } elsif ($input =~ /^(".*")$/) {
        return "new String($1)";
    } elsif ($input =~ /^(0x..)$/) {
        return "new Char($1)";
    } else {
        die "cannot parse key spec $input";
    }
}

while (<>) {
    next if (/^#/);
    chomp;
    my ($code, $solo, $shift, $control, $name) = split(/\t/);
    if ($solo or $shift or $control) {
        printf '_map[%s] = new KeyDefinition("%s", %s, %s, %s);', $code, $name, parse_spec($solo), parse_spec($shift), parse_spec($control);
        print "\n";
    }
}
