# frozen_string_literal: true

require_relative 'lib/wow_dbc/version'

Gem::Specification.new do |spec|
  spec.name = 'wow_dbc'
  spec.version = WowDBC::VERSION
  spec.authors = ['sebi']
  spec.email = ['gore.sebyx@yahoo.com']

  spec.summary = 'A high-performance Ruby gem for reading and manipulating World of Warcraft DBC files'
  spec.description = 'WowDBC provides a Ruby interface to read, write, and manipulate World of Warcraft DBC (Database Client) files. It offers efficient CRUD operations and seamless integration with Ruby projects, making it ideal for WoW addon development, data analysis, and game modding.'
  spec.homepage = 'https://github.com/sebyx07/wow_dbc'
  spec.license = 'MIT'
  spec.required_ruby_version = '>= 3.0.0'

  spec.metadata['allowed_push_host'] = 'https://rubygems.org'

  spec.metadata['homepage_uri'] = spec.homepage
  spec.metadata['source_code_uri'] = spec.homepage

  # Specify which files should be added to the gem when it is released.
  # The `git ls-files -z` loads the files in the RubyGem that have been added into git.
  gemspec = File.basename(__FILE__)
  spec.files = IO.popen(%w[git ls-files -z], chdir: __dir__, err: IO::NULL) do |ls|
    ls.readlines("\x0", chomp: true).reject do |f|
      (f == gemspec) ||
        f.start_with?(*%w[bin/ test/ spec/ features/ .git .github appveyor Gemfile])
    end
  end

  spec.files = spec.files.reject do |file|
    file.end_with?('.so') || file.end_with?('.o') || file.end_with?('.a')
  end

  spec.bindir = 'exe'
  spec.executables = spec.files.grep(%r{\Aexe/}) { |f| File.basename(f) }
  spec.require_paths = ['lib']

  spec.extensions = ['ext/wow_dbc/extconf.rb']

  spec.add_development_dependency 'rake-compiler', '>= 1.2.7'
end
