# -*- encoding: utf-8 -*-
$:.push File.expand_path("../lib", __FILE__)

Gem::Specification.new do |s|
  s.name        = 'sparsam'
  s.version     = '0.2.1'
  s.authors     = ['Airbnb Thrift Developers']
  s.email       = ['foundation.performance-eng@airbnb.com']
  s.homepage    = 'http://thrift.apache.org'
  s.summary     = %q(Ruby bindings for Apache Thrift)
  s.description = %q(Ruby bindings for the Apache Thrift RPC system)
  s.license = 'Apache 2.0'
  s.extensions = ['ext/extconf.rb']

  s.has_rdoc      = true
  s.rdoc_options  = %w(--line-numbers --inline-source --title Thrift --main README)

  s.rubyforge_project = 'thrift'

  s.files = Dir.glob("{lib,spec,ext}/**/*.{c,h,rb,cpp}")
  s.test_files = Dir.glob("{test,spec,benchmark}/**/*")
  s.executables = Dir.glob("{bin}/**/*")

  s.extra_rdoc_files = ["README.md"] + Dir.glob("{ext,lib}/**/*.{c,h,rb,cpp}")

  s.require_paths = %w(lib ext)

  s.add_development_dependency 'rspec', '~> 2.10.0'
  s.add_development_dependency "bundler"
  s.add_development_dependency 'rake', '< 11.0'

  # specify versions with 1.9 support
  s.add_development_dependency 'rubocop', '0.41.2'
  s.add_development_dependency 'rubocop-rspec', '1.5.0'
end
