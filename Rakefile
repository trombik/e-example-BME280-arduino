raise "THINGSPEAK_API_KEY environment variable must be defined" unless ENV["THINGSPEAK_API_KEY"]
raise "THINGSPEAK_API_CHANNEL environment variable must be defined" unless ENV["THINGSPEAK_API_CHANNEL"]

task default: %i[astyle build] do
end

task :cppcheck do
  sh "cppcheck --version"
  sh "cppcheck --error-exitcode=1 --enable=warning,style,performance,portability -v --language=c src/*.ino"
end

task :astyle do
  sh "astyle --version"
  sh "astyle --formatted --options=.astylerc --recursive 'src/*.ino'"
end

task build: %i{cppcheck} do
  sh "pio --version"
  channel = ENV['THINGSPEAK_API_CHANNEL']
  key = ENV["THINGSPEAK_API_KEY"]
  sh format('PLATFORMIO_SRC_BUILD_FLAGS="-DTHINGSPEAK_API_CHANNEL=%<channel>d -DTHINGSPEAK_API_KEY=%<key>s" pio run -v', { channel: channel, key: key })
end
