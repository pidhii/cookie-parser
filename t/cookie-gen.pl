#!/bin/perl -wl

use strict;

use Getopt::Long;
use JSON;

my ($ok, $nok);
my $MULTIPLIER = 10;
GetOptions (
  ok => \$ok,
  nok => \$nok,
  'x=i' => \$MULTIPLIER,
) or die $!;
my $MOD;
$MOD = $ok ? 'ok' : $nok ? 'nok' : (print "use: ./tester.pl --ok/--nok" and exit -1);

my $seps = '()<>,;@:\"/[]?={} \t';
my @name_chars = grep { index($seps, $_) == -1 }map { chr } (32..126);
my @val_chars  = map { chr } (0x21, 0x23 .. 0x2B, 0x2D .. 0x3A, 0x3C .. 0x5B, 0x5D .. 0x7E);

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
  my $dump = shift;

  my $i = 1;
  while ($in =~ /(.)$match-$i(.)/) {
    my $replace = process_mod($1, $generator->(10));
    push @$dump, "$replace" if defined $dump;
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
@patterns_ok = (@patterns_ok, @patterns_ok) x$MULTIPLIER;

my @patterns_nok = (
  "",
  "<OWS-1>",
  "=",
  "=; ",
  ";",

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
@patterns_nok = (@patterns_nok, @patterns_nok) x$MULTIPLIER;

  # '[OWS-1][cookie-name-1]=[DQ-1][cookie-octets-1][DQ-2][OWS-2]',
  # '[OWS-1][cookie-name-1]=[DQ-1][cookie-octets-1][DQ-2]; [cookie-name-2]=[DQ-3][cookie-octets-2][DQ-2][OWS-2]',

my @patterns = ($MOD eq 'ok') ? @patterns_ok : @patterns_nok;

my @res;
foreach my $pat (@patterns) {
  my $cookie = $pat;
  my @key_in;
  my @val_in;
  my @key_out;
  my @val_out;

  $cookie = replace($cookie, 'cookie-name', \&cookie_name, \@key_in);
  $cookie = replace($cookie, 'cookie-octets', \&cookie_octets, \@val_in);
  $cookie = replace($cookie, 'OWS', \&ows);
  $cookie = replace($cookie, 'DQ', sub { '"'  });
 
  push @res, {
    pattern => $pat,
    cookie => $cookie,
    expect => [
      map { { key => $key_in[$_], val =>  $val_in[$_] } } 0..$#val_in
    ],
  };
}

print encode_json(\@res);
