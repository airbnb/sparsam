require 'rubygems'
require 'mkmf'

brew = find_executable('foobar')
puts brew

puts enable_config('homebrew', brew)

exit 1

header_dirs_ = [
  ::RbConfig::CONFIG["includedir"],
  "/usr/local/include",
  "/usr/local/sparsam/include",
  "/usr/local/thrift/include",
  "/opt/local/include",
  "/opt/sparsam/include",
  "/opt/thrift/include",
  "/opt/include",
  "/opt/homebrew/include",
  "/usr/include"
]
lib_dirs_ = [
  ::RbConfig::CONFIG["libdir"],
  "/usr/local/lib",
  "/usr/local/lib64",
  "/usr/local/sparsam/lib",
  "/usr/local/thrift/lib",
  "/opt/local/lib",
  "/opt/sparsam/lib",
  "/opt/thrift/lib",
  "/opt/lib",
  "/opt/homebrew/lib",
  "/usr/lib",
  "/usr/lib64"
]

begin
  thrift_prefix = `brew --prefix airbnb/main/thrift`
rescue
  warn "could not find formula for airbnb/main/thrift - trying to proceed without it"
else
  header_dir = thrift_prefix + "/include"
  lib_dir = thrift_prefix + "/lib"

  puts "including header dir #{header_dir} and lib dir #{lib_dir}"

  header_dirs_.unshift(header_dir)
  lib_dirs_.unshift(lib_dir)
end

header_dirs_.delete_if { |path_| !::File.directory?(path_) }
lib_dirs_.delete_if { |path_| !::File.directory?(path_) }
dir_config("sparsam", header_dirs_, lib_dirs_)

if defined?(append_cflags)
  append_cflags(['-fsigned-char', '-O3', '-ggdb3', '-mtune=native'])
  append_cflags(ENV["CFLAGS"].split(/\s+/)) if !ENV["CFLAGS"].nil?
else
  $CFLAGS = " #{$CFLAGS} -fsigned-char -O3 -ggdb3 -mtune=native "
  $CFLAGS = " #{$CFLAGS} #{ENV["CFLAGS"]} "
end

if defined?(append_cppflags)
  append_cppflags(['-std=c++0x', '-O3', '-ggdb3', '-mtune=native', '-I./third-party/sparsepp'])
  append_cppflags(['-std=c++11', '-stdlib=libc++'])
  append_cppflags(ENV["CPPFLAGS"].split(/\s+/)) if !ENV["CPPFLAGS"].nil?
else
  $CPPFLAGS = " #{$CPPFLAGS} -std=c++0x -O3 -ggdb3 -mtune=native -I./third-party/sparsepp "
  $CPPFLAGS = " #{$CPPFLAGS} #{ENV["CPPFLAGS"]} "
end

if defined?(append_ldflags)
  append_ldflags(ENV["LDFLAGS"].split(/\s+/)) if !ENV["LDFLAGS"].nil?
else
  $LDFLAGS = " #{$LDFLAGS} #{ENV["LDFLAGS"]} "
end

if Gem::Version.new(RUBY_VERSION.dup) < Gem::Version.new('2.0.0')
  $CPPFLAGS = " #{$CPPFLAGS} #{$CXXFLAGS} "
end

<<<<<<< HEAD
brew = find_executable('brew')

# Automatically use homebrew to discover thrift package if it is
# available and not disabled via --disable-homebrew.
use_homebrew = enable_config('homebrew', brew)

if use_homebrew
  $stderr.puts "Using #{brew} to locate thrift and boost packages"
  $stderr.puts '(disable Homebrew integration via --disable-homebrew)'

  thrift_package = with_config('homebrew-thrift-package', 'thrift@0.9')
  $stderr.puts "looking for Homebrew thrift package '#{thrift_package}'"
  $stderr.puts '(change Homebrew thrift package name via --with-homebrew-thrift-package=<package>)'

  thrift_prefix = `#{brew} --prefix #{thrift_package}`.strip

  unless File.exists?(thrift_prefix)
    $stderr.puts "#{thrift_prefix} does not exist"
    $stderr.puts "To resolve, `brew install #{thrift_package}` or pass"
    $stderr.puts '--with-homebrew-thrift-package=<package> to this build to specify'
    $stderr.puts 'an alternative thrift package to build against. e.g.:'
    $stderr.puts
    $stderr.puts '  $ bundle config --global build.sparsam --with-homebrew-thrift-package=mythrift'

    raise "Homebrew package #{thrift_package} not installed"
  end

  $stderr.puts "using Homebrew thrift at #{thrift_prefix}"
  $CFLAGS << " -I#{thrift_prefix}/include"
  $CXXFLAGS << " -I#{thrift_prefix}/include"
  $CPPFLAGS << " -I#{thrift_prefix}/include"
  $LDFLAGS << " -L#{thrift_prefix}/lib"

  # Also add boost to the includes search path if it is installed.
  boost_package = with_config('homebrew-boost-package', 'boost')
  $stderr.puts "looking for Homebrew boost package '#{boost_package}'"
  $stderr.puts '(change Homebrew boost package name via --with-homebrew-boost-package=<package>)'

  boost_prefix = `#{brew} --prefix #{boost_package}`.strip

  if File.exists?(boost_prefix)
    $stderr.puts("using Homebrew boost at #{boost_prefix}")
    $CFLAGS << " -I#{boost_prefix}/include"
    $CXXFLAGS << " -I#{boost_prefix}/include"
    $CPPFLAGS << " -I#{boost_prefix}/include"
  else
    $stderr.puts 'Homebrew boost not found; assuming boost is in default search paths'
  end
end
=======
$defs.push('-DSPP_CXX11')
>>>>>>> e72e60b (wip)

have_func("strlcpy", "string.h")

unless have_library("thrift")
  raise 'thrift library not found; aborting since compile would fail'
end

libs = ['-lthrift']

libs.each do |lib|
  $LOCAL_LIBS << "#{lib} "
end

# Ideally we'd validate boost as well. But boost is header only and
# mkmf's have_header() doesn't work with C++ headers.
# https://bugs.ruby-lang.org/issues/4924

create_makefile 'sparsam_native'
