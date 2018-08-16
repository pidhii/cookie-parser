#!/bin/perl

use strict;
use feature qw/ say /;
use Getopt::Long;

my ($ok, $nok);
GetOptions (
  ok => \$ok,
  nok => \$nok,
) or die $!;
my $MOD = $ok && 'ok' or $nok && 'nok' or say "use: ./tester.pl --ok/--nok" and exit -1;

my (@keys, @values, @patterns);

my $seps = '()<>,;@:\"/[]?={} \t';
my @name_chars = grep { index($seps, $_) == -1 }map { chr } (32..126);
my @val_chars  = map { chr } (0x21, 0x23 .. 0x2B, 0x2D .. 0x3A, 0x3C .. 0x5B, 0x5D .. 0x7E);
# say "name-chars: @name_chars";
# say "value-chars: @val_chars";

my $CRLF = "\r\n";
my @OWS = ("$CRLF ", " ");

sub cookie_name {
  my $len = shift;
  my $ret;
  $ret .= $name_chars[rand @name_chars] for 1..$len;
  return $ret;
}

sub cookie_octets {
  my $len = shift;
  my $ret;
  $ret .= $val_chars[rand @val_chars] for 1..$len;
  return $ret;
}

sub ows {
  return $OWS[rand @OWS];
}

sub process_mod {
  my $mod = shift;
  my $str = shift;

  if ($mod eq "<") {
    return $str;
  } elsif ($mod eq '[') {
    return (int rand 2) ? $str : "";
  } else {
    die "invaid modificator: $mod";
  }
}

sub replace {
  my $in = shift;
  my $match = shift;
  my $generator = shift;

  my $i = 1;
  while ($in =~ /(.)$match-$i(.)/) {
    my $replace = process_mod($1, $generator->(10));
    $in =~ s/(.)$match-$i(.)/$replace/g;
  } continue {
    $i++;
  }

  return $in;
}

my @patterns_ok = (
  '[OWS-1]<cookie-name-1>=[DQ-1][cookie-octets-1][DQ-1][OWS-2]',
  '[OWS-1]<cookie-name-1>=[DQ-1][cookie-octets-1][DQ-1]; <cookie-name-2>=[DQ-2]<cookie-octets-2>[DQ-2][OWS-2]',
);
@patterns_ok = (@patterns_ok, @patterns_ok) x10;

my @patterns_nok = (
  # invalid OWS
  "[OWS-1]$CRLF<cookie-name-1>=[DQ-1][cookie-octets-1][DQ-1][OWS-2]",
  "[OWS-1]<cookie-name-1>=[DQ-1][cookie-octets-1][DQ-1][OWS-2]$CRLF",

  # missing cookie-name
  "[OWS-1]=[DQ-1][cookie-octets-1][DQ-1][OWS-2]",

  # wrong double quotes
  "[OWS-1]<cookie-name-1>=\"[cookie-octets-1][OWS-2]",
  "[OWS-1]<cookie-name-1>=[cookie-octets-1]\"[OWS-2]",

  # '@' is invalid token
  "[OWS-1]<cookie-name-1>@=[DQ-1][cookie-octets-1][DQ-1][OWS-2]",
  "[OWS-1]@<cookie-name-1>=[DQ-1][cookie-octets-1][DQ-1][OWS-2]",
  "[OWS-1]<cookie-name-1>@<cookie-name-2>=[DQ-1][cookie-octets-1][DQ-1][OWS-2]",

  # '\' is invalid cookie-octet
  '[OWS-1]<cookie-name-1>=[DQ-1]<cookie-octets-1>\\[DQ-1][OWS-2]',
  '[OWS-1]<cookie-name-1>=[DQ-1]\\<cookie-octets-1>[DQ-1][OWS-2]',
  '[OWS-1]<cookie-name-1>=[DQ-1]<cookie-octets-1>\\<cookie-octets-2>[DQ-1][OWS-2]',

  # invalid separator
  '[OWS-1]<cookie-name-1>=[DQ-1][cookie-octets-1][DQ-1];<cookie-name-2>=[DQ-2]<cookie-octets-2>[DQ-2][OWS-2]',
  '[OWS-1]<cookie-name-1>=[DQ-1][cookie-octets-1][DQ-1] ;<cookie-name-2>=[DQ-2]<cookie-octets-2>[DQ-2][OWS-2]',
  '[OWS-1]<cookie-name-1>=[DQ-1][cookie-octets-1][DQ-1];$CRLF<cookie-name-2>=[DQ-2]<cookie-octets-2>[DQ-2][OWS-2]',

  # garbage arround
  'akhf<OWS-1><cookie-name-1>=[DQ-1][cookie-octets-1][DQ-1][OWS-2]',
  '[OWS-1]<cookie-name-1>=[DQ-1][cookie-octets-1][DQ-1]<OWS-2>asdfa',
  '[OWS-1]<cookie-name-1>=[DQ-1][cookie-octets-1][DQ-1]; <cookie-name-2>=[DQ-2]<cookie-octets-2>[DQ-2]<OWS-2>asdfasdf',
);
@patterns_nok = (@patterns_nok, @patterns_nok) x10;

  # '[OWS-1][cookie-name-1]=[DQ-1][cookie-octets-1][DQ-2][OWS-2]',
  # '[OWS-1][cookie-name-1]=[DQ-1][cookie-octets-1][DQ-2]; [cookie-name-2]=[DQ-3][cookie-octets-2][DQ-2][OWS-2]',

my @patterns = ($MOD eq 'ok') ? @patterns_ok : @patterns_nok;

open OUT, '>&STDOUT' or die $!;
pipe SUBRD, SUBWR or die $!;

my @res;
STDOUT->fdopen(\*SUBWR, "w") or die $!;
# STDERR->fdopen(\*OUT, "w") or die $!;
foreach my $pat (@patterns) {
  my $cookie = $pat;

  $cookie = replace($cookie, 'cookie-name', \&cookie_name);
  $cookie = replace($cookie, 'cookie-octets', \&cookie_octets);
  $cookie = replace($cookie, 'OWS', \&ows);

  $cookie = replace($cookie, 'DQ', sub { '"'  });
 
  system('./checker', '-c', $cookie);

  chop(my $parse = <SUBRD>);
  chop($parse);
  push @res, { pattern => $pat, cookie => $cookie, parse =>$parse, ok => $? == 0};
}
STDOUT->fdopen(\*OUT, "w");
close OUT or die $!;
close SUBRD or die $!;

foreach my $r (@res) {
  for (qw/pattern cookie/) {
    $r->{$_} =~ s/$CRLF/<CRLF>/g ;
    $r->{$_} =~ s/ /<SP>/g ;
  }

  say '-----------------------';
  say "pattern: $r->{pattern}";
  say "cookie: $r->{cookie}";
  say "output: $r->{parse}";
  # say "status: " . ($r->{ok} ? "\e[38;5;2mok\e[0m" : "\e[38;5;1merror\e[0m");
  say "status: " . ($r->{ok} ? "ok" : "error");
}
