/*
 *******************************************************
 * Timonel Intel Hex Parser                            *
 * Version: 0.3 "Cati" | For Unix & Windows            *
 * ................................................... *
 * 2018-04-07 gustavo.casanova@nicebots.com            *
 * ................................................... *
 * Based on code from BootloadHID project              *
 * ................................................... *
 * NOTE:                                               *
 * -----                                               *
 * To compile on Windows, you need to install "MinGW"  *
 * with, at least the "mingw32-base" package from      *
 * the installation manager. You also have to verify   *
 * that your path variable includes "C:\MinGW\bin".    *
 *******************************************************
 */ 

#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>

#define FILE_TYPE_INTEL_HEX 1
#define FILE_TYPE_RAW 2
#define DEBUGLVL 1
#define BYTESPERLINE 8

#define TML_HEXPARSER_VERSION " Timonel Hex Parser version: 0.3"

// Global definitions
unsigned char dataBuffer[65536 + 256];    /* file data buffer */

// Function prototypes
static int parseRaw(char *hexfile, unsigned char *buffer, int *startAddr, int *endAddr);
static int parseIntelHex(char *hexfile, unsigned char *buffer, int *startAddr, int *endAddr); /* taken from bootloadHID */
static int parseUntilColon(FILE *fp); /* taken from bootloadHID */
static int parseHex(FILE *fp, int numDigits); /* taken from bootloadHID */
static int use_ansi = 0;

// Main function
int main(int argc, char *argv[]) {
  int res;
  char *file = NULL;

  // Command argument parsing
  int run = 0;
  int file_type = FILE_TYPE_INTEL_HEX;
  int arg_pointer = 1;
  #if defined(WIN)
    char* usage = "\n Timonel Intel Hex Parser\n ========================\n usage: tml-hexparser [--help] [--type intel-hex|raw] filename";
  #else
    char* usage = "\n Timonel Intel Hex Parser\n ========================\n usage: tml-hexparser [--help] [--type intel-hex|raw] filename [--no-ansi]\n";
  #endif 
  #if defined(WIN)
    use_ansi = 0;
  #else
    use_ansi = 1;
  #endif

  // Command argument handling
  while (arg_pointer < argc) {
    if (strcmp(argv[arg_pointer], "--run") == 0) {
      run = 1;
    } else if (strcmp(argv[arg_pointer], "--type") == 0) {
      arg_pointer += 1;
      if (strcmp(argv[arg_pointer], "intel-hex") == 0) {
        file_type = FILE_TYPE_INTEL_HEX;
      } else if (strcmp(argv[arg_pointer], "raw") == 0) {
        file_type = FILE_TYPE_RAW;
      } else {
        printf("Unknown File Type specified with --type option");
        return EXIT_FAILURE;
      }
    } else if (strcmp(argv[arg_pointer], "--help") == 0 || strcmp(argv[arg_pointer], "-h") == 0) {
      puts(usage);
      puts("");
      puts("  --type [intel-hex, raw]: Set file type to either Intel Hex or Raw");
      puts("                           bytes (Intel Hex is default)");
      #ifndef WIN
      puts("                --no-ansi: Don't use ANSI in terminal output");
      #endif
      puts("                 filename: Path to Intel Hex or Raw data file,");
      puts("                           or \"-\" to read from stdin");
      puts("");
      puts(TML_HEXPARSER_VERSION);
      return EXIT_SUCCESS;
    } else if (strcmp(argv[arg_pointer], "--no-ansi") == 0) {
      use_ansi = 0;
      arg_pointer += 1;
    } else {
      file = argv[arg_pointer];
    }

    arg_pointer += 1;
  }

  if (argc < 2) {
    puts(usage);
    return EXIT_FAILURE;
  }

  // Parsing user .Hex program file ...
  int startAddress = 1, endAddress = 0;

  memset(dataBuffer, 0xFF, sizeof(dataBuffer));

  if (file_type == FILE_TYPE_INTEL_HEX) {
    if (parseIntelHex(file, dataBuffer, &startAddress, &endAddress)) {
      printf("// Error loading or parsing hex file!\n");
      return EXIT_FAILURE;
    }
  } 
  else if (file_type == FILE_TYPE_RAW) {
    if (parseRaw(file, dataBuffer, &startAddress, &endAddress)) {
      printf("// Error loading raw file!\n");
      return EXIT_FAILURE;
    }

    if (startAddress >= endAddress) {
      printf("// No data in input file, exiting!\n");
      return EXIT_FAILURE;
    }

  }

  printf("// Timonel Hex Parser done. Thank you!\n//\n");

  return EXIT_SUCCESS;
}

// Function parseUntilColon
static int parseUntilColon(FILE *file_pointer) {
  int character;

  do {
    character = getc(file_pointer);
  } while(character != ':' && character != EOF);

  return character;
}
// Function parseHex
static int parseHex(FILE *file_pointer, int num_digits) {
  int iter;
  char temp[9];

  for(iter = 0; iter < num_digits; iter++) {
    temp[iter] = getc(file_pointer);
  }
  temp[iter] = 0;

  return strtol(temp, NULL, 16);
}

// Function parseIntelHex
static int parseIntelHex(char *hexfile, unsigned char *buffer, int *startAddr, int *endAddr) {
  int address, base, d, segment, i, lineLen, sum;
  FILE *input;

  input = strcmp(hexfile, "-") == 0 ? stdin : fopen(hexfile, "r");
  if (input == NULL) {
    printf("//> Error opening %s: %s\n", hexfile, strerror(errno));
    return 1;
  }

  while (parseUntilColon(input) == ':') {
    sum = 0;
    sum += lineLen = parseHex(input, 2);
    base = address = parseHex(input, 4);
    sum += address >> 8;
    sum += address;
    sum += segment = parseHex(input, 2);  /* segment value? */
    if (segment != 0) {   /* ignore lines where this byte is not 0 */
      continue;
    }

    for (i = 0; i < lineLen; i++) {
      d = parseHex(input, 2);
      buffer[address++] = d;
      sum += d;
    }

    sum += parseHex(input, 2);
    if ((sum & 0xff) != 0) {
      printf("//> Warning: Checksum error between address 0x%x and 0x%x\n", base, address);
    }

    if(*startAddr > base) {
      *startAddr = base;
    }
    if(*endAddr < address) {
      *endAddr = address;
    }

  }
  
#if ( DEBUGLVL > 0 )
  // GC: Printing addresses
  printf("\n//\n");
  printf("// Start Address: 0x%x \n", *startAddr);
  printf("// End Address: 0x%x \n//\n", *endAddr);
  // GC: Printing payload array definition ...
  //printf("const byte payldType = 0;    /* Application Payload */\n\n");
  //printf("const byte payload[%i] = {", *endAddr + 1);
  printf("uint8_t payload[%i] = {", *endAddr + 1);  

  
  // GC: Printing loaded buffer ...
  printf("\n    ");
    int l = 0;
	for(i = 0; i <= *endAddr; i++) {
		printf("0x%02x", buffer[i]);
		if (i <= (*endAddr) - 1) {
			printf(", ");
		}
		if (l++ == BYTESPERLINE - 1) {
			printf("\n    ");
			l = 0;
		}
	}
	printf("\n};\n\n//\n");  
#endif
 
  fclose(input);
  return 0;
}

//Function parseRaw
static int parseRaw(char *filename, unsigned char *data_buffer, int *start_address, int *end_address) {
  FILE *input;

  input = strcmp(filename, "-") == 0 ? stdin : fopen(filename, "r");

  if (input == NULL) {
    printf("//> Error reading %s: %s\n", filename, strerror(errno));
    return 1;
  }

  *start_address = 0;
  *end_address = 0;

  // Read file bytes
  int byte = 0;
  while (1) {
    byte = getc(input);
    if (byte == EOF) break;

    *data_buffer = byte;
    data_buffer += 1;
    *end_address += 1;
  }

  fclose(input);
  return 0;
}
