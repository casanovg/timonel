require_relative "./hexprogram.rb"

data = HexProgram.new(open ARGV.first)

#puts data.instance_variable_get(:@bytes).inspect

data = data.bytes

# find start address
start_address = 16 # skip past baked in trampoline - upgrade firmware generates one anyway!
# TODO: Verify jump table? or store it in the upgrade firmware for verbatim installation?
start_address += 1 while data[start_address] == 0xFF

raise "Seems to be junk data quite early in the bootloader" unless start_address > 100

# trim blank padding data from start of data
start_address.times { data.shift }

# if data is an odd number of bytes make it even
data.push 0xFF while (data.length % 2) != 0

#puts "Length: #{data.length}"
#puts "Start address: #{start_address}"
puts "const uint16_t bootloader_data[#{data.length / 2}] PROGMEM = {...};"
puts "uint16_t bootloader_address = #{start_address};"

File.open "bootloader_data.c", "w" do |file|
  file.puts "// This file contains the Timonel bootloader data itself and the"
  file.puts "// address to install it into flash memory."
  file.puts "//"
  file.puts "// Use generate-data.rb with ruby 1.9 or 2.0 to generate these"
  file.puts "// values from a Timonel bootloader hex file (tml-bootloader.hex)."
  file.puts "// The MAKE_RELEASE script generates this automatically ..."
  file.puts "//"
  file.puts "// Timonel starting address: #{start_address}"
  file.puts "//"  
  file.puts "// Generated from #{ARGV.first} at #{Time.now} by #{ENV['USER']}"
  file.puts ""
  file.puts "const uint16_t bootloader_data[#{data.length / 2}] PROGMEM = {"
  file.puts data.each_slice(2).map { |big_end, little_end|
   "0x#{ ((little_end * 256) + big_end).to_s(16).rjust(4, '0') }"
  }.join(', ')
  file.puts "};"
  file.puts ""
  file.puts "uint16_t bootloader_address = #{start_address};"
end

data = HexProgram.new(open ARGV.first)

data = data.bytes

# find start address
start_address = 0 # skip past baked in trampoline - upgrade firmware generates one anyway!
# TODO: Verify jump table? or store it in the upgrade firmware for verbatim installation?
start_address += 1 while data[start_address] == 0xFF

File.open "tml-payload.h", "w" do |file|
  file.puts "// This file contains a new Timonel bootloader version and the updater"
  file.puts "// code in C header format to include into the TWI master source code."
  file.puts "// Please verify that the TWI master source code has a line like this (make"
  file.puts "// sure to include the correct path):"
  file.puts "//"  
  file.puts "// #include \"payload.h\""
  file.puts "//"
  file.puts "// Use \"generate-data.rb\" with ruby 1.9 or 2.0 to generate these"
  file.puts "// data values from a Timonel+Updater hex file (tml-updater.hex)."
  file.puts "// The \"MAKE_RELEASE.sh\" script makes this for you automatically ..."
  file.puts "//"
  file.puts "// Timonel updater starting address: #{start_address}"
  file.puts "//"
  file.puts "// Generated from \"#{ARGV.first}\" at #{Time.now} by #{ENV['USER']}"
  file.puts ""
  file.puts "uint8_t payload[#{data.length}] = {"
  file.puts data.each.map { |data_byte|
	"0x#{ (data_byte).to_s(16).rjust(2, '0') }"
  }.join(', ')  
  file.puts "};"
  file.puts ""
end


