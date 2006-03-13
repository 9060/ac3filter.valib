use strict;

my @chs      = qw(1 2 3 4 5 6);
my @formats  = qw(FORMAT_PCM16 FORMAT_PCM24 FORMAT_PCM32 FORMAT_PCM16_BE FORMAT_PCM24_BE FORMAT_PCM32_BE FORMAT_PCMFLOAT );
my @names    = qw(pcm16    pcm24    pcm32    pcm16_be pcm24_be pcm32_be pcmfloat );
my @types    = qw(int16_t  int24_t  int32_t  int16_t  int24_t  int32_t  float    );
my @funcs    = qw(le2int16 le2int24 le2int32 be2int16 be2int24 be2int32);

my $ch;
my $i;
my $format;
my $name;
my $type;
my $func;

my @template = <>;
my $convert;
my $text;


###############################################################################
# class members

foreach $ch (@chs)
{
  foreach $name (@names)
  {
    print "  void ".$name."_linear_".$ch."ch();\n";
  }
  print "\n";
}

###############################################################################
# array of functions

print "typedef void (Converter::*convert_t)();\n\n";
print "static const int formats_tbl[] = { ".join(", ", @formats)." };\n\n";
print "static const int formats = ".join(" | ", @formats).";\n\n";

print "static const convert_t pcm2linear_tbl[NCHANNELS][".($#formats+1)."] = {\n";
foreach $ch (@chs)
{
  print " { ";
  print join ", ", map { "&Converter::".$_."_linear_".$ch."ch" } @names;
  print " },\n";
}
print "};\n\n";

###############################################################################
# function implementation

foreach $ch (@chs)
{
  for ($i = 0; $i <= $#formats; $i++)
  {
    $format = $formats[$i];
    $name = $names[$i];
    $type = $types[$i];
    $func = $funcs[$i];
    $convert = "";
    $convert = $convert."    dst[nsamples * $_] = $func(src[$_]);\n" foreach (0..$ch-1);
    $text = join('', @template);
    $text =~ s/(\$\w+)/$1/gee;

    print "void\n";
    print "Converter::".$name."_linear_".$ch."ch()\n";
    print $text;
  }
  print "\n";
}
