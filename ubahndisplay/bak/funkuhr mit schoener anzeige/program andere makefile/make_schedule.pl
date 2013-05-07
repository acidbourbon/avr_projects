#!/usr/bin/perl -w



sub encode_train_data {


	foreach my $train (@{$_[0]}) {

		$linecounter = 0;
		foreach (@line_names) {
			if ($train->[0] eq $_) {
				last;
			}
			$linecounter ++;
		}
	
		$endoflinecounter = 0;
		foreach (@endofline_names) {
			if ($train->[2] eq $_) {
				last;
			}
			$endoflinecounter ++;
		}
	
		# write line + endofline code to file
		# upper Nibble = index of corresponding line string in the line string array
		# lower Nibble = index of corresponding end of line string in the end of line string array
	
		$line_and_endofline = sprintf("0x%02X",$linecounter*16 + $endoflinecounter);

		# code format : "{line_and_endofline, weekly_schedule, depart_time},"

		print SCHREIBEN "{$line_and_endofline,$train->[3],$train->[4]},\n";
	}


}









#################### End Of SUBS!







open(LESEN,"$ARGV[0]")
  or die "Fehler beim oeffnen von $ARGV[0]: $!\n";
                      
open(SCHREIBEN,"> train_data.h")
  or die "Fehler beim oeffnen von $ARGV[0].c: $!\n";


print SCHREIBEN "//this is automaticly generated code, do not alter manually!";

# aquire data from .csv schedule    
while(defined(my $i = <LESEN>)) {
	$i =~ s/\"//g;
	if (!($i =~ m/^\/\//  or $i =~ m/^,,/)) {
		chomp($i);
		@line_entries = split(/,/, $i);
		if(!($line_entries[0] eq "")){
			$linie = uc($line_entries[0]);
			$direction = $line_entries[1];
			$endofline = $line_entries[2];
			$weekly_schedule = $line_entries[3];
			$hour = int($line_entries[4]);
			for ($j =5;defined($line_entries[$j]);$j++) {
				$depart_time = $hour*60 + int($line_entries[$j]);
				push(@train_data,[$linie,$direction,$endofline,$weekly_schedule,$depart_time]);
			}
		}
	}


}

# find strings for Lines and end of lines

foreach my $train (@train_data) {
	
	$match = 0;
	foreach(@line_names) {
		if ($_ eq $train->[0]) {
			{
				$match = 1;
				last;
			}
		}
	}
	if($match == 0) {
		push(@line_names,$train->[0]);
	}

	$match = 0;
	foreach(@endofline_names) {
		if ($_ eq $train->[2]) {
			{
				$match = 1;
				last;
			}
		}
	}
	if($match == 0) {
		push(@endofline_names,$train->[2]);
	}
}

# generate c code for progmem strings

# underground lines
$counter =0;

print SCHREIBEN "\n\n//Names of the underground lines:\n\n";

foreach (@line_names) {
	
print SCHREIBEN <<EOF;
const char line_name$counter\[] PROGMEM = "$_ ";
EOF
	$counter++;
}
print SCHREIBEN <<EOF;

const char * line_names[] PROGMEM = 
{
EOF

for($i = 0;$i<$counter;$i++) {
	print SCHREIBEN "line_name$i,\n";
}

print SCHREIBEN "};\n";

# underground end of lines

$counter =0;

print SCHREIBEN "\n\n//Names of the underground end of lines:\n\n";

foreach (@endofline_names) {
	
print SCHREIBEN <<EOF;
const char endofline_name$counter\[] PROGMEM = "$_ ";
EOF
	$counter++;
}
print SCHREIBEN <<EOF;

const char * endofline_names[] PROGMEM = 
{
EOF

for($i = 0;$i<$counter;$i++) {
	print SCHREIBEN "endofline_name$i,\n";
}

print SCHREIBEN "};\n\n\n";

# split data by direction

$north_schedule_size = 0;
$south_schedule_size = 0;

foreach(@train_data) {
	if ($_->[1] eq "nord") {
		push(@train_data_north,$_);
		$north_schedule_size ++;
	}
	if ($_->[1] eq "sued") {
		push(@train_data_south,$_);
		$south_schedule_size ++;
	}	
}

# sort data by depart_time

@train_data_north = sort {$a->[4] <=> $b->[4]} @train_data_north;
@train_data_south = sort {$a->[4] <=> $b->[4]} @train_data_south;


# print data to file

print SCHREIBEN "\n\n";
print SCHREIBEN <<EOF;
struct departure {
	uint8_t line_and_endofline;
		/* upper Nibble = index of corresponding line string in the line string array
		lower Nibble = index of corresponding end of line string in the end of line string array */
	uint8_t weekly_schedule;
		/* one Byte, it's lower 7 bit form the digital pattern of the weekly schedule, Monday is LSB */
	uint16_t depart_time;
		/* departure time, measured in minutes after midnight */
};


EOF
print SCHREIBEN "\n\n";

print SCHREIBEN "#define NORTH_SCHEDULE_SIZE $north_schedule_size\n";
print SCHREIBEN "struct departure north_schedule[] PROGMEM = {\n";
print SCHREIBEN "//first element is schedule size\n";
print SCHREIBEN "{255,255,$north_schedule_size},\n";
encode_train_data(\@train_data_north);
print SCHREIBEN "};\n";


print SCHREIBEN "\n\n";

print SCHREIBEN "#define SOUTH_SCHEDULE_SIZE $south_schedule_size\n";
print SCHREIBEN "struct departure south_schedule[] PROGMEM = {\n";
print SCHREIBEN "//first element is schedule size\n";
print SCHREIBEN "{255,255,$south_schedule_size},\n";
encode_train_data(\@train_data_south);
print SCHREIBEN "};\n";





# make short report:

$memory_usage = ($north_schedule_size+$south_schedule_size)*4;

foreach(@line_names) {
	$memory_usage += length($_)+2;
}
foreach(@endofline_names) {
	$memory_usage += length($_)+2;
}



print  <<EOF;

UBahn-Parser

Report:

Nordgleis: $north_schedule_size Abfahrten
Südgleis: $south_schedule_size Abfahrten

Linien:

@line_names

Endstationen:

@endofline_names

Speicherbedarf: $memory_usage Byte

EOF











close(LESEN);
close(SCHREIBEN)
  or die "Fehler beim Schließen von 'neu.dat': $! \n"

