#!/bin/perl -lw

use strict;
# use autodie qw/:all/;
use JSON;
use Term::ProgressBar;

my $JSON = JSON::XS->new->canonical;

my $bin = "$ENV{BUILD}/code-cov";
die "no executable at: \"$bin\"" unless -x $bin;

sub test_ok {
  my $cookies = $JSON->decode(`./cookie-gen.pl --ok -x 20`);
  die $! unless $? == 0;
  my $success = 1;

  my $progress = Term::ProgressBar->new({
      name => "\e[38;5;5;3mTesting valid cookies\e[0m",
      count => ~~@$cookies,
      term_width => `tput cols` / 2,
  });
  foreach my $sample (@$cookies) {
    open(my $pipe, "-|", $bin, $sample->{cookie}) or die $!;
    $sample->{output} = decode_json(do { local $/; <$pipe> });
    $sample->{status} = int close $pipe;

    # Check if parsed key/value pairs are correct.
    my $in  = $JSON->encode($sample->{expect});
    my $out = $JSON->encode($sample->{output});
    $sample->{correct} = int($in eq $out);
    $success = 0 unless $sample->{status} == 1 && $sample->{correct};

    $progress->update;
  }
  $success ? print " -> \e[38;5;2;3mok\e[0m" : print " -> \e[38;5;1;1;3mfailure\e[0m";

  $cookies
}

sub test_nok {
  my $cookies = $JSON->decode(`./cookie-gen.pl --nok -x 10`);
  die $! unless $? == 0;
  my $success = 1;

  my $progress = Term::ProgressBar->new({
      name => "\e[38;5;5;3mTesting wrong cookies\e[0m",
      count => ~~@$cookies,
      term_width => `tput cols` / 2,
  });
  foreach my $sample (@$cookies) {
    open(my $pipe, "-|", $bin, $sample->{cookie}) or die $!;
    $sample->{status} = int close $pipe;
    $success = 0 if $sample->{status} == 1;

    $progress->update;
  }
  $success ? print " -> \e[38;5;2;3mok\e[0m" : print " -> \e[38;5;1;3mfailure\e[0m";

  $cookies
}

my $oks  = test_ok();
my $noks = test_nok();

if ($ENV{DUMP}) {
  open my $dump_ok, ">$ENV{DUMP}/test-ok.json";
  print $dump_ok $JSON->pretty->encode($oks);

  open my $dump_nok, ">$ENV{DUMP}/test-nok.json";
  print $dump_nok $JSON->pretty->encode($noks);
}

my $gcov = `gcov $ENV{COOKIESRC} | grep -e File -e Lines`;
die $! unless $? == 0;

printf "%s", "\e[38;5;5;3mCode coverage\e[0m: ";
my @lines = split /\n/, $gcov;
for my $i (1 .. @lines / 2) {
  my $file = @lines[($i-1) * 2];
  my $exec = @lines[($i-1) * 2 + 1];
  $file =~ s/^File '(.+\/)+(.+)'$/$2/;
  $exec =~ s/^Lines executed:(.+)% of.*$/$1/;
  $exec = $exec;
  if ($exec == 100) {
    printf "%s", "\"\e[38;5;3;3m$file\e[0m\": \e[38;5;2m$exec\e[0m\e[3m%\e[0m";
  } else {
    printf "%s", "\"\e[38;5;3;3m$file\e[0m\": $exec\e[3m%\e[0m";
  }

  printf ", " unless $i == @lines / 2;
}
print "";
