require 'rubygems'
require 'mkmf'

$CFLAGS = " -fsigned-char -O3 -ggdb3 -mtune=native "
$CXXFLAGS = " -std=c++0x -O3 -ggdb3 -mtune=native -I./third-party/sparsepp "
if Gem::Version.new(RUBY_VERSION.dup) < Gem::Version.new('2.0.0')
  $CPPFLAGS += $CXXFLAGS
end

have_func("strlcpy", "string.h")
have_library("thrift")
libs = ['-lthrift']

libs.each do |lib|
  $LOCAL_LIBS << "#{lib} "
end

create_makefile 'sparsam_native'
