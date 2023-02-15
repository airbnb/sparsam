require 'rubygems'
require 'rake'
require 'rake/clean'
require 'rspec/core/rake_task'

THRIFT = './compiler/build/sparsam-gen'.freeze

task :default => [:gem]
task :spec => [:'gen-ruby', :build_ext, :realspec]

RSpec::Core::RakeTask.new(:realspec) do |t|
  t.rspec_opts = ['--color', '--format d']
end

RSpec::Core::RakeTask.new(:'spec:rcov') do |t|
  t.rspec_opts = ['--color', '--format d']
  t.rcov = true
  t.rcov_opts = ['--exclude', '^spec,/gems/']
end

desc 'Build the compiler for .thrift files'
file THRIFT do
  sh './build.sh'
end

desc 'Compile the .thrift files for the specs'
task :'gen-ruby' => [THRIFT, :'gen-ruby:spec']
namespace :'gen-ruby' do
  task :spec do
    dir = File.dirname(__FILE__) + '/spec'
    sh THRIFT, '-o', dir, "#{dir}/user.thrift"
  end
end

desc "Build the native library"
task :build_ext => :'gen-ruby' do
  Dir.chdir(File.dirname('ext/extconf.rb')) do
    unless sh "ruby #{File.basename('ext/extconf.rb')}"
      warn "Failed to run extconf"
      break
    end
    unless sh "make"
      warn "make failed"
      break
    end
  end
end

# desc 'Run the compiler tests (requires full thrift checkout)'
# task :test do
#   # ensure this is a full thrift checkout and not a tarball of the ruby libs
#   cmd = 'head -1 ../../README.md 2>/dev/null | grep Thrift >/dev/null 2>/dev/null'
#   system(cmd) or fail "rake test requires a full thrift checkout"
#   sh 'make', '-C', File.dirname(__FILE__) + "/../../test/rb", "check"
# end

desc 'Builds the thrift gem'
task :gem => [:spec, :build_ext] do
  unless sh 'gem', 'build', 'sparsam.gemspec'
    warn "Failed to build thrift gem"
    break
  end
end

desc 'Install the thrift gem'
task :install => [:gem] do
  unless sh 'gem', 'install', Dir.glob('sparsam-*.gem').last
    warn "Failed to install thrift gem"
    break
  end
end

CLEAN.include [
  '.bundle', 'benchmark/gen-ruby', 'coverage', 'ext/*.{o,bundle,so,dll}', 'ext/mkmf.log',
  'ext/Makefile', 'ext/conftest.dSYM', 'Gemfile.lock', 'mkmf.log', 'pkg', 'spec/gen-ruby',
  'test', 'thrift-*.gem',
]
